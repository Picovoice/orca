#
#    Copyright 2024 Picovoice Inc.
#
#    You may not use this file except in compliance with the license. A copy of the license is located in the "LICENSE"
#    file accompanying this source.
#
#    Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
#    an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
#    specific language governing permissions and limitations under the License.
#

import argparse
import platform
import re
import subprocess
import threading
import time
import traceback
from dataclasses import dataclass
from queue import Queue
from typing import (
    Any,
    Callable,
    Dict,
    Optional,
    Sequence,
)

import numpy as np
import pvorca
import tiktoken
from numpy.typing import NDArray
from pvorca import OrcaActivationLimitError, OrcaInvalidArgumentError
from sounddevice import (
    OutputStream,
    query_devices,
    PortAudioError,
)

CUSTOM_PRON_PATTERN = r"\{(.*?\|.*?)\}"
CUSTOM_PRON_PATTERN_NO_WHITESPACE = r"\{(.*?\|.*?)\}(?!\s)"


class StreamingAudioDevice:
    def __init__(self, device_index: Optional[int] = None) -> None:
        if device_index is None:
            device_info = query_devices(kind="output")
            device_index = int(device_info["index"])

        self._device_index = device_index
        self._queue: Queue[Sequence[int]] = Queue()

        self._buffer = None
        self._stream = None
        self._sample_rate = None
        self._blocksize = None

    def start(self, sample_rate: int) -> None:
        self._sample_rate = sample_rate
        self._blocksize = self._sample_rate // 20
        self._stream = OutputStream(
            channels=1,
            samplerate=self._sample_rate,
            dtype=np.int16,
            device=self._device_index,
            callback=self._callback,
            blocksize=self._blocksize)
        self._stream.start()

    # noinspection PyShadowingNames
    # noinspection PyUnusedLocal
    def _callback(self, outdata: NDArray, frames: int, time: Any, status: Any) -> None:
        if self._queue.empty():
            outdata[:] = 0
            return

        pcm = self._queue.get()
        outdata[:, 0] = pcm

    def play(self, pcm_chunk: Sequence[int]) -> None:
        if self._stream is None:
            raise ValueError("Stream is not started. Call `start` method first.")

        pcm_chunk = np.array(pcm_chunk, dtype=np.int16)

        if self._buffer is not None:
            if pcm_chunk is not None:
                pcm_chunk = np.concatenate([self._buffer, pcm_chunk])
            else:
                pcm_chunk = self._buffer
            self._buffer = None

        length = pcm_chunk.shape[0]
        for index_block in range(0, length, self._blocksize):
            if (length - index_block) < self._blocksize:
                self._buffer = pcm_chunk[index_block: index_block + (length - index_block)]
            else:
                self._queue.put_nowait(pcm_chunk[index_block: index_block + self._blocksize])

    def flush_and_terminate(self) -> None:
        self.flush()
        self.terminate()

    def flush(self) -> None:
        if self._buffer is not None:
            chunk = np.zeros(self._blocksize, dtype=np.int16)
            chunk[:self._buffer.shape[0]] = self._buffer
            self._queue.put_nowait(chunk)

        time_interval = self._blocksize / self._sample_rate
        while not self._queue.empty():
            time.sleep(time_interval)

        time.sleep(time_interval)

    def terminate(self) -> None:
        self._stream.stop()
        self._stream.close()

    @staticmethod
    def list_output_devices() -> Dict[str, Any]:
        return query_devices(kind="output")


def linux_machine() -> str:
    machine = platform.machine()
    if machine == "x86_64":
        return machine
    elif machine in ["aarch64", "armv7l"]:
        arch_info = ("-" + machine) if "64bit" in platform.architecture()[0] else ""
    else:
        raise NotImplementedError("Unsupported CPU architecture: `%s`" % machine)

    cpu_info = ""
    try:
        cpu_info = subprocess.check_output(["cat", "/proc/cpuinfo"]).decode("utf-8")
        cpu_part_list = [x for x in cpu_info.split("\n") if "CPU part" in x]
        cpu_part = cpu_part_list[0].split(" ")[-1].lower()
    except Exception as e:
        raise RuntimeError("Failed to identify the CPU with `%s`\nCPU info: `%s`" % (e, cpu_info))

    if "0xd03" == cpu_part:
        return "cortex-a53" + arch_info
    elif "0xd07" == cpu_part:
        return "cortex-a57" + arch_info
    elif "0xd08" == cpu_part:
        return "cortex-a72" + arch_info
    elif "0xd0b" == cpu_part:
        return "cortex-a76" + arch_info
    else:
        raise NotImplementedError("Unsupported CPU: `%s`." % cpu_part)


class OrcaThread:
    @dataclass
    class OrcaInput:
        text: str
        flush: bool

    def __init__(
            self,
            play_audio_callback: Callable[[Sequence[int]], None],
            access_key: str,
            num_tokens_per_second: int,
            model_path: Optional[str] = None,
            library_path: Optional[str] = None,
            audio_wait_chunks: Optional[int] = None,
    ) -> None:

        self._orca = pvorca.create(access_key=access_key, model_path=model_path, library_path=library_path)
        self._orca_stream = self._orca.stream_open()
        self._sample_rate = self._orca.sample_rate

        self._play_audio_callback = play_audio_callback
        self._num_tokens_per_second = num_tokens_per_second
        assert self._num_tokens_per_second > 0

        self._queue: Queue[Optional[OrcaThread.OrcaInput]] = Queue()
        self._thread = None

        self._time_first_audio_available = -1
        self._pcm_buffer: Queue[Sequence[int]] = Queue()

        self._wait_chunks = audio_wait_chunks or self._get_first_audio_wait_chunks()
        self._num_pcm_chunks_processed = 0

    @staticmethod
    def _get_first_audio_wait_chunks() -> int:
        wait_chunks = 0
        if platform.system() == "Linux":
            machine = linux_machine()
            if "cortex" in machine:
                wait_chunks = 1
        return wait_chunks

    def _run(self) -> None:
        while True:
            orca_input = self._queue.get()
            if orca_input is None:
                while not self._pcm_buffer.empty():
                    self._play_audio_callback(self._pcm_buffer.get())
                break

            try:
                if not orca_input.flush:
                    pcm = self._orca_stream.synthesize(orca_input.text)
                else:
                    pcm = self._orca_stream.flush()
            except OrcaInvalidArgumentError as e:
                raise ValueError(f"Orca could not synthesize text input `{orca_input.text}`: `{e}`")

            if pcm is not None:
                if self._num_pcm_chunks_processed < self._wait_chunks:
                    self._pcm_buffer.put_nowait(pcm)
                else:
                    while not self._pcm_buffer.empty():
                        self._play_audio_callback(self._pcm_buffer.get())
                    self._play_audio_callback(pcm)

                if self._num_pcm_chunks_processed == 0:
                    self._time_first_audio_available = time.time()

                self._num_pcm_chunks_processed += 1

    def _close_thread_blocking(self):
        self._queue.put_nowait(None)
        self._thread.join()

    def start(self) -> None:
        self._thread = threading.Thread(target=self._run)
        self._thread.start()

    def synthesize(self, text: str) -> None:
        self._queue.put_nowait(self.OrcaInput(text=text, flush=False))

    def flush(self) -> None:
        self._queue.put_nowait(self.OrcaInput(text="", flush=True))
        self._close_thread_blocking()
        self.start()

    def delete(self) -> None:
        self._close_thread_blocking()
        self._orca_stream.close()
        self._orca.delete()

    def get_time_first_audio_available(self) -> float:
        return self._time_first_audio_available

    @property
    def sample_rate(self) -> int:
        return self._sample_rate

    @property
    def version(self) -> str:
        return self._orca.version


def tokenize_text(text: str) -> Sequence[str]:
    text = re.sub(CUSTOM_PRON_PATTERN_NO_WHITESPACE, r'{\1} ', text)

    custom_pronunciations = re.findall(CUSTOM_PRON_PATTERN, text)
    custom_pronunciations = set(["{" + pron + "}" for pron in custom_pronunciations])

    encoder = tiktoken.encoding_for_model("gpt-4")
    tokens_raw = [encoder.decode([i]) for i in encoder.encode(text)]

    custom_pron = ""
    tokens_with_custom_pronunciations = []
    for i, token in enumerate(tokens_raw):
        in_custom_pron = False
        for pron in custom_pronunciations:
            in_custom_pron_global = len(custom_pron) > 0
            current_match = token.strip() if not in_custom_pron_global else custom_pron + token
            if pron.startswith(current_match):
                custom_pron += token.strip() if not in_custom_pron_global else token
                in_custom_pron = True

        if not in_custom_pron:
            if custom_pron != "":
                tokens_with_custom_pronunciations.append(f" {custom_pron}" if i != 0 else custom_pron)
                custom_pron = ""
            tokens_with_custom_pronunciations.append(token)

    return tokens_with_custom_pronunciations


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "--access_key",
        "-a",
        required=True,
        help="AccessKey obtained from Picovoice Console (https://console.picovoice.ai/)")
    parser.add_argument(
        "--library_path",
        "-l",
        help="Absolute path to dynamic library. Default: using the library provided by `pvorca`")
    parser.add_argument(
        "--model_path",
        "-m",
        help="Absolute path to Orca model. Default: using the model provided by `pvorca`")
    parser.add_argument(
        "--text-to-stream",
        "-t",
        required=True,
        help="Text to be streamed to Orca")
    parser.add_argument(
        "--tokens-per-second",
        type=int,
        default=15,
        help="Number of tokens per second to be streamed to Orca, simulating an LLM response.")
    parser.add_argument(
        "--audio-wait-chunks",
        type=int,
        default=None,
        help="Number of PCM chunks to wait before starting to play audio. Default: system-dependent.")
    parser.add_argument(
        "--show-audio-devices",
        action="store_true",
        help="Only list available audio output devices and exit")
    parser.add_argument('--audio-device-index', type=int, default=None, help='Index of input audio device')
    args = parser.parse_args()

    if args.show_audio_devices:
        print(StreamingAudioDevice.list_output_devices())
        exit(0)

    access_key = args.access_key
    model_path = args.model_path
    library_path = args.library_path
    text = args.text_to_stream
    tokens_per_second = args.tokens_per_second
    audio_wait_chunks = args.audio_wait_chunks
    audio_device_index = args.audio_device_index

    try:
        audio_device = StreamingAudioDevice(device_index=audio_device_index)
        # Some systems may have issues with PortAudio only when starting the audio device. Test it here.
        audio_device.start(sample_rate=16000)
        audio_device.terminate()
        play_audio_callback = audio_device.play
    except PortAudioError:
        print(traceback.format_exc())
        print(
            "WARNING: Failed to initialize audio device, see details above. Falling back to running "
            "the demo without audio playback.\n")
        audio_device = None

        # noinspection PyUnusedLocal
        def play_audio_callback(pcm: Sequence[int]):
            pass

    orca = OrcaThread(
        play_audio_callback=play_audio_callback,
        num_tokens_per_second=tokens_per_second,
        access_key=access_key,
        model_path=model_path,
        library_path=library_path,
        audio_wait_chunks=audio_wait_chunks,
    )

    orca.start()
    if audio_device is not None:
        audio_device.start(sample_rate=orca.sample_rate)

    try:
        print(f"Orca version: {orca.version}\n")

        print(f"Simulated text stream:")
        tokens = tokenize_text(text=text)

        time_start_text_stream = time.time()
        for token in tokens:
            print(f"{token}", end="", flush=True)

            orca.synthesize(text=token)

            time.sleep(1 / tokens_per_second)

        text_stream_duration_seconds = time.time() - time_start_text_stream

        orca.flush()

        first_audio_available_seconds = orca.get_time_first_audio_available() - time_start_text_stream
        print(f"\n\nTime to finish text stream:  {text_stream_duration_seconds:.2f} seconds")
        print(f"Time to receive first audio: {first_audio_available_seconds:.2f} seconds after text stream started\n")

        if audio_device is not None:
            print("Waiting for audio to finish ...")
            audio_device.flush_and_terminate()

    except OrcaActivationLimitError:
        print("AccessKey has reached its processing limit")
    finally:
        orca.delete()


if __name__ == "__main__":
    main()
