import threading
import time
from dataclasses import dataclass
from enum import Enum
from io import BytesIO
from queue import Queue
from typing import Callable, Any, Union, Optional, Sequence

import numpy as np
import pvorca
from numpy.typing import NDArray
from openai import OpenAI
from pvorca import OrcaInvalidArgumentError

from .util import Timestamps


class Synthesizers(Enum):
    OPENAI = "openai"
    PICOVOICE_ORCA = "picovoice_orca"


class Synthesizer:
    def __init__(
            self,
            sample_rate: int,
            play_audio_callback: Callable[[Union[Sequence[int], NDArray]], None],
            timestamps: Timestamps,
            input_streamable: bool = False,
    ) -> None:
        self.sample_rate = sample_rate
        self.input_streamable = input_streamable

        self._play_audio_callback = play_audio_callback
        self._timestamps = timestamps

    def synthesize(self, text: str) -> None:
        raise NotImplementedError(
            f"Method `synthesize` must be implemented in a subclass of {self.__class__.__name__}")

    def flush(self) -> None:
        pass

    def terminate(self) -> None:
        pass

    @classmethod
    def create(cls, engine: Union[str, Synthesizers], **kwargs: Any) -> 'Synthesizer':
        classes = {
            Synthesizers.PICOVOICE_ORCA: PicovoiceOrcaSynthesizer,
            Synthesizers.OPENAI: OpenAISynthesizer,
        }

        if engine not in classes:
            raise NotImplementedError(f"Cannot create {cls.__name__} of type `{engine.value}`")

        return classes[engine](**kwargs)

    def __str__(self) -> str:
        raise NotImplementedError(
            f"Method `__str__` must be implemented in a subclass of {self.__class__.__name__}")


class OpenAISynthesizer(Synthesizer):
    MODEL_NAME = "tts-1"
    VOICE_NAME = "shimmer"
    SAMPLE_RATE = 24000
    NAME = "OpenAI TTS"

    def __init__(
            self,
            access_key: str,
            model_name: str = MODEL_NAME,
            **kwargs: Any
    ) -> None:
        super().__init__(sample_rate=self.SAMPLE_RATE, **kwargs)

        self._model_name = model_name
        self._client = OpenAI(api_key=access_key)

    @staticmethod
    def _decode(b: bytes) -> NDArray:
        pcm = np.frombuffer(BytesIO(b).read(), dtype=np.int16)
        return pcm

    def synthesize(self, text: str) -> None:
        self._timestamps.maybe_log_time_first_synthesis_request()

        response = self._client.audio.speech.create(
            model=self._model_name,
            voice=self.VOICE_NAME,
            response_format="pcm",
            input=text)

        for chunk in response.iter_bytes(chunk_size=1024):
            self._timestamps.maybe_log_time_first_audio()

            pcm = self._decode(chunk)
            self._play_audio_callback(pcm)

    def __str__(self) -> str:
        return f"{self.NAME}, model={self.MODEL_NAME}, voice={self.VOICE_NAME}"


class PicovoiceOrcaSynthesizer(Synthesizer):
    NUM_TOKENS_PER_PCM_CHUNK = 8

    @dataclass
    class OrcaTextInput:
        text: str
        flush: bool

    def __init__(
            self,
            play_audio_callback: Callable[[Union[Sequence[int], NDArray]], None],
            timestamps: Timestamps,
            access_key: str,
            model_path: Optional[str] = None,
            library_path: Optional[str] = None,
    ) -> None:
        self._orca = pvorca.create(access_key=access_key, model_path=model_path, library_path=library_path)
        super().__init__(
            sample_rate=self._orca.sample_rate,
            play_audio_callback=play_audio_callback,
            timestamps=timestamps,
            input_streamable=True)

        self._orca_stream = self._orca.open_stream()
        self._sample_rate = self._orca.sample_rate

        self._queue: Queue[Optional[PicovoiceOrcaSynthesizer.OrcaTextInput]] = Queue()
        self._play_audio_callback = play_audio_callback

        self._num_tokens = 0
        self._timestamps = timestamps

        self._thread = None
        self._start_thread()

    def _start_thread(self) -> None:
        self._thread = threading.Thread(target=self._run)
        self._thread.start()

    def _close_thread_blocking(self):
        self._queue.put_nowait(None)
        self._thread.join()

    def _reset_state(self) -> None:
        self._num_tokens = 0

    def _compute_first_audio_delay(self, pcm: Sequence[int], processing_time: float) -> float:
        seconds_audio = len(pcm) / self._sample_rate
        tokens_per_sec = self._num_tokens / (time.time() - self._timestamps.time_first_synthesis_request)
        time_delay = \
            max(((self.NUM_TOKENS_PER_PCM_CHUNK / (
                    tokens_per_sec + 1e-4)) - seconds_audio) + processing_time,
                0)
        return time_delay

    def _run(self) -> None:
        while True:
            orca_input = self._queue.get()
            if orca_input is None:
                break

            self._timestamps.maybe_log_time_first_synthesis_request()

            self._num_tokens += 1

            start = time.time()
            try:
                if not orca_input.flush:
                    pcm = self._orca_stream.synthesize(orca_input.text)
                else:
                    pcm = self._orca_stream.flush()
            except OrcaInvalidArgumentError as e:
                print(f"Orca could not synthesize text input `{orca_input.text}`: `{e}`")
                continue
            processing_time = time.time() - start

            if len(pcm) > 0:
                if self._timestamps.is_first_audio:
                    self._timestamps.maybe_log_time_first_audio()

                    initial_audio_delay = self._compute_first_audio_delay(pcm=pcm, processing_time=processing_time)
                    self._timestamps.set_initial_audio_delay(initial_audio_delay)

                    time.sleep(initial_audio_delay)

                self._play_audio_callback(pcm)

    def synthesize(self, text: str) -> None:
        self._queue.put_nowait(self.OrcaTextInput(text=text, flush=False))

    def flush(self) -> None:
        self._queue.put_nowait(self.OrcaTextInput(text="", flush=True))
        self._close_thread_blocking()
        self._reset_state()
        self._start_thread()

    def terminate(self):
        self._close_thread_blocking()
        self._orca_stream.close()
        self._orca.delete()

    def __str__(self) -> str:
        return f"Picovoice Orca v{self._orca.version}"


__all__ = [
    "Synthesizers",
    "Synthesizer",
]
