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

import time
from queue import Queue
from typing import (
    Any,
    Sequence,
)

import numpy as np
from numpy.typing import NDArray
from sounddevice import OutputStream, query_devices


class StreamingAudioDevice:
    def __init__(self, device_index: int) -> None:
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

    @classmethod
    def from_default_device(cls) -> 'StreamingAudioDevice':
        device_info = query_devices(kind="output")
        device_index = int(device_info["index"])
        return cls(device_index=device_index)


__all__ = [
    "StreamingAudioDevice",
]
