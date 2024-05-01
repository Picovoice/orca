import math
import threading
from dataclasses import dataclass
from queue import Queue
from time import sleep, time
from typing import (
    Any,
    Callable,
    Optional,
    Sequence,
)

import numpy as np
import pvorca
from numpy.typing import NDArray
from pvorca import OrcaInvalidArgumentError
from sounddevice import OutputStream, query_devices


class OrcaThread:
    NUM_TOKENS_NEXT_PCM = 3
    PROCESSING_TIME_MULTIPLIER = 3

    @dataclass
    class OrcaTextElement:
        text: str
        flush: bool

    def __init__(
            self,
            play_audio_callback: Callable[[Sequence[int]], None],
            access_key: str,
            num_tokens_per_second: int,
            model_path: Optional[str] = None,
            library_path: Optional[str] = None,
    ) -> None:

        self._orca = pvorca.create(access_key=access_key, model_path=model_path, library_path=library_path)
        self._orca_stream = self._orca.open_stream()
        self._sample_rate = self._orca.sample_rate

        self._play_audio_callback = play_audio_callback
        self._num_tokens_per_second = num_tokens_per_second
        assert self._num_tokens_per_second > 0

        self._queue: Queue[Optional[OrcaThread.OrcaTextElement]] = Queue()
        self._thread = None
        self._start_thread()

        self._time_first_audio_available = None

        self._pcm_buffer: Queue[Sequence[int]] = Queue()

        self._set_time = False

    def _start_thread(self) -> None:
        self._thread = threading.Thread(target=self._run)
        self._thread.start()

    def _close_thread_blocking(self):
        self._queue.put_nowait(None)
        self._thread.join()

    def _compute_first_audio_wait_chunks(self, pcm: Sequence[int], processing_time: float) -> int:
        seconds_audio = len(pcm) / self._sample_rate
        llm_wait_seconds = self.NUM_TOKENS_NEXT_PCM / self._num_tokens_per_second
        orca_wait_seconds = self.PROCESSING_TIME_MULTIPLIER * processing_time
        wait_seconds = max(llm_wait_seconds + orca_wait_seconds - seconds_audio, 0)
        wait_chunks = math.ceil(wait_seconds)
        # print(f"|- wait {wait_chunks} chunks|", flush=True, end="")
        return wait_chunks

    def _run(self) -> None:
        while True:
            orca_input = self._queue.get()
            if orca_input is None:
                break

            start = time()
            try:
                if not orca_input.flush:
                    pcm = self._orca_stream.synthesize(orca_input.text)
                else:
                    pcm = self._orca_stream.flush()
                # if pcm is not None and self._time_first_audio_available is not None:
                #     sleep(0.6)
            except OrcaInvalidArgumentError as e:
                print(f"Orca could not synthesize text input `{orca_input.text}`: `{e}`")
                continue

            if pcm is not None:
                wait_chunks = 0
                if self._time_first_audio_available is None:
                    self._time_first_audio_available = time()

                    # sleep(0.2)

                    processing_time = time() - start

                    wait_chunks = self._compute_first_audio_wait_chunks(pcm=pcm, processing_time=processing_time)

                self._pcm_buffer.put_nowait(pcm)

                while not self._pcm_buffer.empty() and wait_chunks == 0:
                    self._play_audio_callback(self._pcm_buffer.get())

    def synthesize(self, text: str) -> None:
        self._queue.put_nowait(self.OrcaTextElement(text=text, flush=False))

    def flush(self) -> None:
        self._queue.put_nowait(self.OrcaTextElement(text="", flush=True))
        self._close_thread_blocking()
        self._start_thread()

    def get_time_first_audio_available(self) -> Optional[float]:
        return self._time_first_audio_available

    def delete(self) -> None:
        self._close_thread_blocking()
        self._orca_stream.close()
        self._orca.delete()

    @property
    def sample_rate(self) -> int:
        return self._sample_rate

    @property
    def version(self) -> str:
        return self._orca.version


class StreamingAudioDevice:
    def __init__(self, device_info: dict) -> None:
        self._device_info = device_info
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
            device=int(self._device_info["index"]),
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
            sleep(time_interval)

        sleep(time_interval)

    def terminate(self) -> None:
        self._stream.stop()
        self._stream.close()

    @classmethod
    def from_default_device(cls) -> 'StreamingAudioDevice':
        return cls(device_info=query_devices(kind="output"))


__all__ = [
    "StreamingAudioDevice",
    "OrcaThread",
]
