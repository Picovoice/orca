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
import random
import sys
import termios  # Unix-specific
import time
import tty
import threading
import queue
from dataclasses import dataclass
from typing import *

import numpy as np
import sounddevice as sd
from numpy.typing import NDArray

from pvorca import create, Orca

SENTENCES = [
    "Your flight to Paris is scheduled for tomorrow morning at nine AM. Don't forget to pack your passport, and arrive at the airport at least two hours before departure to allow time for security checks.",
    "Your meeting with the marketing team has been rescheduled to Friday at two PM. Be sure to review the updated agenda and prepare any necessary materials beforehand.",
    "The new restaurant in town, Culinary Delights, has received excellent reviews for its fusion cuisine. Would you like me to make a reservation for dinner this weekend?",
    "Your fitness tracker data indicates that you've achieved your weekly step goal. Congratulations! How about rewarding yourself with a relaxing massage or a movie night?",
    "Your car's maintenance reminder just popped up. It's time for an oil change and tire rotation. Shall I schedule an appointment with your preferred auto service center?",
    "Your friend's birthday is coming up soon. Would you like assistance in selecting the perfect gift or planning a surprise celebration?",
    "Your meditation app membership expires in three days. Would you like to renew it, or shall I explore other mindfulness resources for you?",
    "Your credit card statement shows a higher-than-usual balance this month. Shall I provide a breakdown of your expenses to help you identify any unnecessary purchases?",
    "Your favorite book club is hosting a discussion on the latest bestseller next Thursday. Would you like me to add it to your calendar, or perhaps you'd prefer to join virtually?"
]

KEYBOARD_INTERRUPT_KEY = '\x03'
ENTER_KEY = '\r'


@dataclass
class TimestampDeltas:
    time_to_first_llm_token: float = -1.0
    time_to_last_llm_token: float = -1.0
    time_to_first_audio: float = -1.0


class SpeakerOutput:
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
            blocksize=self._blocksize,
        )

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


class ThreadedOrcaStream:
    @dataclass
    class OrcaTextInput:
        text: str
        flush: bool

    def __init__(
            self,
            orca_stream: Orca.Stream,
            play_audio_callback: Callable,
    ) -> None:
        self._orca_stream = orca_stream

        self._queue: queue.Queue[Optional[ThreadedOrcaStream.OrcaTextInput]] = queue.Queue()
        self._play_audio_callback = play_audio_callback

        self._thread = threading.Thread(target=self._run)

        self._timestamp_first_audio = -1.0
        self._total_processing_time = 0.0

    def _run(self) -> None:
        while True:
            orca_input = self._queue.get()
            if orca_input is None:
                break

            start = time.time()
            if not orca_input.flush:
                pcm = self._orca_stream.synthesize(orca_input.text)
            else:
                pcm = self._orca_stream.flush()
            self._total_processing_time += time.time() - start

            if len(pcm) > 0:
                if self._timestamp_first_audio < 0.0:
                    self._timestamp_first_audio = time.time()
                self._play_audio_callback(pcm)

    @property
    def timestamp_first_audio(self) -> float:
        return self._timestamp_first_audio

    @property
    def total_processing_time(self) -> float:
        return self._total_processing_time

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


def main(args: argparse.Namespace) -> None:
    access_key = args.access_key
    model_path = args.model_path
    library_path = args.library_path
    simulate_llm_response = args.simulate_llm_response

    orca = create(access_key=access_key, model_path=model_path, library_path=library_path)

    output_device = SpeakerOutput.from_default_device(sample_rate=orca.sample_rate)

    orca_stream = orca.open_stream()
    orca_thread = ThreadedOrcaStream(orca_stream=orca_stream, play_audio_callback=output_device.play)

    with contextlib.redirect_stdout(None):  # to avoid stdouts from ALSA lib
        orca_thread.start()
        output_device.start()
        time.sleep(0.5)

    timestamp_deltas = TimestampDeltas()

    if not simulate_llm_response:
        print("Start typing. Orca will generate speech continuously (press CTRL+C to exit and ENTER to flush):")
    else:
        try:
            res = input("Press ENTER to run a demo LLM response (CTRL+C to exit): ")
            if res != "":
                sys.exit(0)
        except KeyboardInterrupt:
            print()
            sys.exit(0)
        print()

    def read_char():
        fd = sys.stdin.fileno()
        old_settings = termios.tcgetattr(fd)
        try:
            tty.setraw(sys.stdin.fileno())
            ch = sys.stdin.read(1)
        finally:
            termios.tcsetattr(fd, termios.TCSADRAIN, old_settings)
        return ch

    def text_generator() -> Generator[str, None, None]:
        for c in SENTENCES[random.randint(0, len(SENTENCES) - 1)]:
            time.sleep(0.02)
            if c == '\n':
                yield ' '
            yield c

    generator = text_generator()

    text = ""
    num_tokens = 0
    start = time.time()
    while True:
        if simulate_llm_response:
            try:
                char = next(generator)
                timestamp_deltas.time_to_first_llm_token = time.time() - start
            except StopIteration:
                char = ENTER_KEY
                timestamp_deltas.time_to_last_llm_token = time.time() - start
        else:
            char = read_char()

        print(char, end="", flush=True)

        if char == KEYBOARD_INTERRUPT_KEY:
            break
        elif char == ENTER_KEY:  # Ctrl+C or Enter key
            orca_thread.flush()
            break
        elif char == " " or char in {".", ",", "!", "?"}:
            num_tokens += 1

            token = f"{text}{char}"
            orca_thread.synthesize(text=token)

            text = ""
        else:
            text += char

    print("\n(waiting for audio to finish ...)")

    end = time.time()

    timestamp_deltas.time_to_first_audio = orca_thread.timestamp_first_audio - start

    orca_thread.wait()
    orca_thread.terminate()

    output_device.wait()
    output_device.terminate()
    print()

    orca_stream.close()

    if simulate_llm_response:
        print(f"Simulated tokens / second: ~{num_tokens / (end - start):.2f}")
        print(f"Time to generate text: {timestamp_deltas.time_to_last_llm_token:.2f} seconds")
        print(f"Time to first audio: {timestamp_deltas.time_to_first_audio:.2f} seconds")

    orca.delete()


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Text-to-speech streaming synthesis")
    parser.add_argument("--access_key", "-a", required=True, help="AccessKey obtained from Picovoice Console")
    parser.add_argument("--model_path", "-m", help="Path to the model parameters file")
    parser.add_argument("--library_path", "-l", help="Path to Orca's dynamic library")
    parser.add_argument(
        "--simulate-llm-response",
        action="store_true",
        help="Simulate LLM response (for testing purposes)")

    main(parser.parse_args())
