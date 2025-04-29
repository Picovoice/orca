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
from dataclasses import dataclass
from queue import Queue
from collections import deque
from itertools import chain
from typing import (
    Callable,
    Optional,
    Sequence,
)

from orca_demo import (
    get_available_languages,
    get_available_genders,
    get_model_path,
)

import pvorca
from pvorca import (
    Orca,
    OrcaActivationLimitError,
    OrcaInvalidArgumentError,
)
from pvspeaker import PvSpeaker

# TODO: Remove once tiktoken supports windows-arm64
try:
    import tiktoken
except:
    pass

CUSTOM_PRON_PATTERN = r"\{(.*?\|.*?)\}"
CUSTOM_PRON_PATTERN_NO_WHITESPACE = r"\{(.*?\|.*?)\}(?!\s)"


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
            orca: Orca,
            flush_audio_callback: Callable[[Sequence[int]], None],
            play_audio_callback: Callable[[Sequence[int]], int],
            num_tokens_per_second: int,
            audio_wait_chunks: Optional[int] = None,
    ) -> None:

        self._orca = orca
        self._orca_stream = self._orca.stream_open()

        self._play_audio_callback = play_audio_callback
        self._flush_audio_callback = flush_audio_callback
        self._num_tokens_per_second = num_tokens_per_second
        assert self._num_tokens_per_second > 0

        self._queue: Queue[Optional[OrcaThread.OrcaInput]] = Queue()
        self._thread = None

        self._time_first_audio_available = -1
        self._pcm_buffer = deque()

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
                break

            try:
                if not orca_input.flush:
                    pcm = self._orca_stream.synthesize(orca_input.text)
                else:
                    pcm = self._orca_stream.flush()
            except OrcaInvalidArgumentError as e:
                raise ValueError(f"Orca could not synthesize text input `{orca_input.text}`: `{e}`")

            if pcm is not None:
                if self._num_pcm_chunks_processed == 0:
                    self._time_first_audio_available = time.time()
                self._num_pcm_chunks_processed += 1

                self._pcm_buffer.append(pcm)

            if self._num_pcm_chunks_processed > self._wait_chunks:
                if len(self._pcm_buffer) > 0:
                    pcm = self._pcm_buffer.popleft()
                    written = self._play_audio_callback(pcm)
                    if written < len(pcm):
                        self._pcm_buffer.appendleft(pcm[written:])

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

    def flush_audio(self) -> None:
        remaining_pcm = list(chain.from_iterable(self._pcm_buffer))
        self._thread = threading.Thread(target=self._flush_audio_callback, args=(remaining_pcm,))
        self._thread.start()
        self._thread.join()

    def delete(self) -> None:
        self._close_thread_blocking()
        self._orca_stream.close()
        self._orca.delete()

    def get_time_first_audio_available(self) -> float:
        return self._time_first_audio_available


def tokenize_text(text: str, language: str) -> Sequence[str]:
    text = re.sub(CUSTOM_PRON_PATTERN_NO_WHITESPACE, r'{\1} ', text)

    custom_pronunciations = re.findall(CUSTOM_PRON_PATTERN, text)
    custom_pronunciations = set(["{" + pron + "}" for pron in custom_pronunciations])

    # TODO: Update once Orca supports passing in partial bytes
    if language == "ko" or language == "ja":
        tokens_raw = list(text)
    else:
        # TODO: Remove once tiktoken supports windows-arm64
        try:
            encoder = tiktoken.encoding_for_model("gpt-4")
            tokens_raw = [encoder.decode([i]) for i in encoder.encode(text)]
        except:
            ALPHA_NUMERIC = 'abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 '
            PUNCTUATION = '!"#$%&\'()*+,-./:;<=>?@[\]^_`{|}~ '
            tokens_raw = [text[0]]
            for ch in text[1:]:
                if (ch in ALPHA_NUMERIC and tokens_raw[-1][-1] not in ALPHA_NUMERIC) or ch in PUNCTUATION:
                    tokens_raw.append(ch)
                else:
                    tokens_raw[-1] += ch

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
    available_languages = get_available_languages()
    available_genders = get_available_genders()

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
        help="Absolute path to Orca model")
    parser.add_argument(
        "--language",
        help=f"The language you would like to run the demo in. "
             f"Available languages are {', '.join(get_available_languages())}.")
    parser.add_argument(
        "--gender",
        help=f"The gender of the synthesized voice. "
             f"Available genders are {', '.join(get_available_genders())}.")
    parser.add_argument(
        "--text_to_stream",
        "-t",
        required=True,
        help="Text to be streamed to Orca")
    parser.add_argument(
        "--tokens_per_second",
        type=int,
        default=15,
        help="Number of tokens per second to be streamed to Orca, simulating an LLM response.")
    parser.add_argument(
        "--audio_wait_chunks",
        type=int,
        default=None,
        help="Number of PCM chunks to wait before starting to play audio. Default: system-dependent.")
    parser.add_argument(
        "--buffer_size_secs",
        type=int,
        default=20,
        help="The size in seconds of the internal buffer used by pvspeaker to play audio.")
    parser.add_argument(
        "--show_audio_devices",
        action="store_true",
        help="Only list available audio output devices and exit")
    parser.add_argument(
        '--audio-device-index',
        type=int,
        default=-1,
        help='Index of input audio device')
    args = parser.parse_args()

    if args.show_audio_devices:
        devices = PvSpeaker.get_available_devices()
        for i in range(len(devices)):
            print("index: %d, device name: %s" % (i, devices[i]))
        exit(0)

    access_key = args.access_key
    model_path = args.model_path
    language = args.language
    gender = args.gender
    library_path = args.library_path
    text = args.text_to_stream
    tokens_per_second = args.tokens_per_second
    audio_wait_chunks = args.audio_wait_chunks
    buffer_size_secs = args.buffer_size_secs
    audio_device_index = args.audio_device_index

    if not model_path:
        if language not in available_languages:
            raise ValueError(f"Given argument --language `{language}` is not an available language. "
                             f"Available languages are {', '.join(available_languages)}.")
        if gender not in available_genders:
            raise ValueError(f"Given argument --gender `{gender}` is not an available gender. "
                             f"Available genders are {', '.join(available_genders)}.")
        model_path = get_model_path(language, gender)

    orca = pvorca.create(access_key=access_key, model_path=model_path, library_path=library_path)

    speaker = None
    try:
        speaker = PvSpeaker(
            sample_rate=orca.sample_rate,
            bits_per_sample=16,
            buffer_size_secs=buffer_size_secs,
            device_index=audio_device_index)
        speaker.start()
    except RuntimeError or ValueError:
        print(
            "\nWarning: Failed to initialize PvSpeaker. Orca will still generate PCM data, "
            "but it will not be played.\n")

    def play_audio_callback(pcm: Sequence[int]) -> int:
        try:
            if speaker is not None:
                return speaker.write(pcm)
            return len(pcm)
        except ValueError:
            pass
        return len(pcm)

    def flush_audio_callback(pcm: Sequence[int]) -> None:
        try:
            if speaker is not None:
                speaker.flush(pcm)
        except MemoryError:
            pass

    orca_thread = OrcaThread(
        orca=orca,
        play_audio_callback=play_audio_callback,
        flush_audio_callback=flush_audio_callback,
        num_tokens_per_second=tokens_per_second,
        audio_wait_chunks=audio_wait_chunks,
    )

    orca_thread.start()
    try:
        print(f"Orca version: {orca.version}\n")

        tokens = tokenize_text(text=text, language=language)

        print(f"Simulated text stream:")
        time_start_text_stream = time.time()
        for token in tokens:
            print(f"{token}", end="", flush=True)

            orca_thread.synthesize(text=token)

            time.sleep(1 / tokens_per_second)

        text_stream_duration_seconds = time.time() - time_start_text_stream

        orca_thread.flush()
        first_audio_available_seconds = orca_thread.get_time_first_audio_available() - time_start_text_stream
        print(f"\n\nTime to finish text stream:  {text_stream_duration_seconds:.2f} seconds")
        print(f"Time to receive first audio: {first_audio_available_seconds:.2f} seconds after text stream started\n")

        if speaker is not None:
            print("Waiting for audio to finish ...")
        orca_thread.flush_audio()

        if speaker is not None:
            speaker.delete()
    except KeyboardInterrupt:
        print("\nStopped...")
        if speaker is not None:
            speaker.stop()
    except OrcaActivationLimitError:
        print("\nAccessKey has reached its processing limit")
    finally:
        orca_thread.delete()


if __name__ == "__main__":
    main()
