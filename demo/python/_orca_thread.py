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

import platform
import threading
import time
from dataclasses import dataclass
from queue import Queue
from typing import (
    Callable,
    Optional,
    Sequence,
)

import pvorca
from pvorca import OrcaInvalidArgumentError

from _util import JETSON_MACHINES, linux_machine, RASPBERRY_PI_MACHINES


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
        self._orca_stream = self._orca.open_stream()
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
            if machine in JETSON_MACHINES:
                wait_chunks = 1
            elif machine in RASPBERRY_PI_MACHINES:
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


__all__ = [
    "OrcaThread",
]
