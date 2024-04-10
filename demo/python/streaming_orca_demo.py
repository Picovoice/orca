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
import contextlib
import sys
import time
import threading
import queue
from dataclasses import dataclass
from typing import *

import numpy as np
import sounddevice as sd
from numpy.typing import NDArray

from pvorca import create, Orca

from demo_util import get_text_generator


@dataclass
class TimestampDeltas:
    time_to_first_llm_token: float = -1.0
    time_to_last_llm_token: float = -1.0
    time_to_first_audio: float = -1.0


class StreamingAudioOutput:
    def __init__(
            self,
            device_info: dict,
            sample_rate: int,
    ) -> None:
        self._sample_rate = sample_rate

        self._blocksize = self._sample_rate // 20

        self.queue: queue.Queue[NDArray] = queue.Queue()
        self._buffer = None

        self.stream = sd.OutputStream(
            channels=1,
            samplerate=self._sample_rate,
            dtype=np.int16,
            device=int(device_info["index"]),
            callback=self._callback,
            blocksize=self._blocksize)

    def _callback(self, outdata: NDArray, frames: int, time: Any, status: Any) -> None:
        if self.queue.empty():
            outdata[:] = 0
            return
        data = self.queue.get()
        outdata[:, 0] = data

    def play(self, pcm_chunk: Sequence[int]) -> None:
        pcm_chunk = np.array(pcm_chunk, dtype=np.int16)

        if self._buffer is not None:
            pcm_chunk = np.concatenate([self._buffer, pcm_chunk])
            self._buffer = None

        length = pcm_chunk.shape[0]
        for index_block in range(0, length, self._blocksize):
            if (length - index_block) < self._blocksize:
                self._buffer = pcm_chunk[index_block: index_block + (length - index_block)]
            else:
                self.queue.put_nowait(pcm_chunk[index_block: index_block + self._blocksize])

    def start(self) -> None:
        self.stream.start()

    def wait_and_terminate(self) -> None:
        self.wait()
        self.terminate()

    def wait(self):
        if self._buffer is not None:
            chunk = np.zeros(self._blocksize, dtype=np.int16)
            chunk[:self._buffer.shape[0]] = self._buffer
            self.queue.put_nowait(chunk)

        time_interval = self._blocksize / self._sample_rate
        while not self.queue.empty():
            time.sleep(time_interval)

        time.sleep(time_interval)
        self.stream.stop()

    def terminate(self):
        self.stream.close()

    @classmethod
    def from_default_device(cls, **kwargs):
        return cls(sd.query_devices(kind="output"), **kwargs)


class StreamingOrcaThread:
    NUM_TOKENS_PER_PCM_CHUNK = 6

    @dataclass
    class OrcaTextInput:
        text: str
        flush: bool

    def __init__(self, orca: Orca, play_audio_callback: Callable) -> None:
        self._orca_stream = orca.open_stream()
        self._sample_rate = orca.sample_rate

        self._queue: queue.Queue[Optional[StreamingOrcaThread.OrcaTextInput]] = queue.Queue()
        self._play_audio_callback = play_audio_callback

        self._thread = threading.Thread(target=self._run)

        self._timestamp_first_audio = -1.0
        self._timestamp_start_text_stream = 0.0
        self._first_audio_time_delay = 0.0

        self._total_processing_time = 0.0

        self._num_tokens = 0
        self._first_token = True

    def _compute_first_audio_delay(self, pcm: Sequence[int]) -> float:
        seconds_audio = len(pcm) / self._sample_rate
        tokens_per_sec = self._num_tokens / (time.time() - self._timestamp_start_text_stream)
        time_delay = \
            max(((self.NUM_TOKENS_PER_PCM_CHUNK / tokens_per_sec) - seconds_audio) + self._total_processing_time, 0)
        return time_delay

    def _run(self) -> None:
        while True:
            orca_input = self._queue.get()
            if orca_input is None:
                break

            if self._first_token:
                self._timestamp_start_text_stream = time.time()
                self._first_token = False

            self._num_tokens += 1
            start = time.time()
            if not orca_input.flush:
                pcm = self._orca_stream.synthesize(orca_input.text)
            else:
                pcm = self._orca_stream.flush()
            self._total_processing_time += time.time() - start

            if len(pcm) > 0:
                if self._timestamp_first_audio < 0.0:
                    self._timestamp_first_audio = time.time()

                    self._first_audio_time_delay = self._compute_first_audio_delay(pcm)
                    time.sleep(self._first_audio_time_delay)

                self._play_audio_callback(pcm)

    @property
    def timestamp_first_audio(self) -> float:
        return self._timestamp_first_audio

    @property
    def total_processing_time(self) -> float:
        return self._total_processing_time

    @property
    def first_audio_time_delay(self) -> float:
        return self._first_audio_time_delay

    def synthesize(self, text: str) -> None:
        self._queue.put_nowait(self.OrcaTextInput(text=text, flush=False))

    def flush(self) -> None:
        self._queue.put_nowait(self.OrcaTextInput(text="", flush=True))

    def start(self) -> None:
        self._thread.start()

    def wait_and_terminate(self) -> None:
        self.wait()
        self.terminate()

    def wait(self):
        while not self._queue.empty():
            time.sleep(0.1)

    def terminate(self):
        self._queue.put_nowait(None)
        self._thread.join()
        self._orca_stream.close()


def main(args: argparse.Namespace) -> None:
    access_key = args.access_key
    model_path = args.model_path
    library_path = args.library_path
    tokens_per_second = args.tokens_per_second

    orca = create(access_key=access_key, model_path=model_path, library_path=library_path)

    output_device = StreamingAudioOutput.from_default_device(sample_rate=orca.sample_rate)

    orca_thread = StreamingOrcaThread(orca=orca, play_audio_callback=output_device.play)

    with contextlib.redirect_stdout(None):  # to avoid stdouts from ALSA lib
        orca_thread.start()
        output_device.start()
        time.sleep(0.5)

    def terminate():
        orca_thread.wait_and_terminate()
        output_device.wait_and_terminate()
        orca.delete()
        sys.exit()

    try:
        res = input("Press ENTER to run a demo LLM response (CTRL+C to exit)\n")
        if res != "":
            terminate()
    except KeyboardInterrupt:
        terminate()

    generator = get_text_generator(token_delay=1 / tokens_per_second)
    timestamp_deltas = TimestampDeltas()

    text = ""
    num_tokens = 0
    start = time.time()
    while True:
        try:
            token = next(generator)
            timestamp_deltas.time_to_first_llm_token = time.time() - start
        except StopIteration:
            timestamp_deltas.time_to_last_llm_token = time.time() - start
            orca_thread.synthesize(text=text)
            orca_thread.flush()
            break

        orca_thread.synthesize(text=token)

        print(token, end="", flush=True)
        num_tokens += 1

    end = time.time()

    timestamp_deltas.time_to_first_audio = orca_thread.timestamp_first_audio - start

    print("\n(waiting for audio to finish ...)")

    orca_thread.wait_and_terminate()
    output_device.wait_and_terminate()

    print(f"\nImitated tokens / second: ~{num_tokens / (end - start):.0f}")
    print(f"Time to generate text: {timestamp_deltas.time_to_last_llm_token:.2f} seconds")
    print(f"Time to first audio: {timestamp_deltas.time_to_first_audio:.2f} seconds", end="")
    if orca_thread.first_audio_time_delay > 0:
        print(
            f" (+ applied initial delay of `{orca_thread.first_audio_time_delay:.2f} seconds` "
            "to ensure continuous audio)")
    else:
        print()

    orca.delete()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Text-to-speech streaming synthesis")
    parser.add_argument("--access_key", "-a", required=True, help="AccessKey obtained from Picovoice Console")
    parser.add_argument("--model_path", "-m", help="Path to the model parameters file")
    parser.add_argument("--library_path", "-l", help="Path to Orca's dynamic library")
    parser.add_argument(
        "--tokens-per-second",
        "-t",
        default=8,
        type=float,
        help="Imitated tokens per second")

    main(parser.parse_args())
