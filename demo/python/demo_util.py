import json
import os
import random
import threading
import time
from dataclasses import dataclass
from datetime import datetime
from enum import Enum
from queue import Queue
from typing import *

import numpy as np
import pvorca
import sounddevice as sd
import tiktoken
from numpy.typing import NDArray
from pvorca import Orca, OrcaInvalidArgumentError


@dataclass
class Timestamps:
    time_llm_request: float = -1.0
    time_first_llm_token: float = -1.0
    time_last_llm_token: float = -1.0
    time_first_synthesis_request: float = -1.0
    time_first_audio: float = -1.0
    initial_audio_delay: float = 0.0

    def reset(self) -> None:
        self.time_llm_request = -1.0
        self.time_first_llm_token = -1.0
        self.time_last_llm_token = -1.0
        self.time_first_synthesis_request = -1.0
        self.time_first_audio = -1.0
        self.initial_audio_delay = 0.0

    def pretty_print_diffs(self, num_tokens: int) -> None:
        print("\033[90m", end="")
        print("\n** Responsiveness metrics **")

        api_request_delay = max(self.time_first_llm_token - self.time_llm_request, 0.01)
        print(
            f"Estimated tokens / second: ~{num_tokens / (self.time_last_llm_token - self.time_llm_request):.0f}",
            end="")
        print(
            "" if api_request_delay == 0.01 else
            f" (includes delay of API request of `{api_request_delay:.2f}` seconds).")

        print(f"Time to generate text: {self.time_last_llm_token - self.time_first_llm_token:.2f} seconds")
        print(f"Time to first audio: {self.time_first_audio - self.time_first_llm_token:.2f} seconds", end="")
        if self.initial_audio_delay > 0:
            print(
                f" (+ applied initial delay of `{self.initial_audio_delay:.2f} seconds` "
                "to ensure continuous audio)")

        else:
            print()
        print("\033[0m")

    def debug_print(self) -> None:
        def to_hms(t: float) -> str:
            date_object = datetime.fromtimestamp(t)
            return date_object.strftime("%H:%M:%S.%f")[:-3]

        print(f"time first LLM token {to_hms(self.time_first_llm_token)}")
        print(f"time last LLM token {to_hms(self.time_last_llm_token)}")
        print(f"time first synthesis request {to_hms(self.time_first_synthesis_request)}")
        print(f"time first audio {to_hms(self.time_first_audio)}")


class LLMs(Enum):
    DUMMY = "dummy"
    OPENAI = "openai"


class LLM:
    SYSTEM_PROMPT = """
    You are a voice assistant. 
    Use natural, conversational language that are clear and easy to follow (short sentences, simple words).
    Only use english letters and punctuation, no special characters. 
    Don't ever use numbers directly. Verbalize them (e.g. "five" instead of "5").
    Keep the conversation flowing. 
    Ask relevant follow-up questions. 
    """
    DEFAULT_USER_PROMPT = "USER PROMPT:\n"

    def __init__(
            self,
            synthesize_text_callback: Optional[Callable[[str], None]],
            user_prompt: Optional[str] = None,
    ) -> None:
        self._synthesize_text_callback = synthesize_text_callback
        self._user_prompt = user_prompt if user_prompt is not None else self.DEFAULT_USER_PROMPT

    def chat(self, prompt: str) -> Generator[str, None, None]:
        print("LLM RESPONSE:")
        for token in self._chat(prompt=prompt):
            if token is not None:
                if self._synthesize_text_callback is not None:
                    self._synthesize_text_callback(token)
                print(token, end="", flush=True)
            yield token

    def _chat(self, prompt: str) -> Generator[str, None, None]:
        raise NotImplementedError(
            f"Method `chat_stream` must be implemented in a subclass of {self.__class__.__name__}")

    def user_prompt(self) -> str:
        return input(self._user_prompt)

    @classmethod
    def create(cls, llm_type: LLMs, **kwargs) -> 'LLM':
        classes = {
            LLMs.DUMMY: DummyLLM,
            LLMs.OPENAI: OpenAILLM,
        }

        if llm_type not in classes:
            raise NotImplementedError(f"Cannot create {cls.__name__} of type `{llm_type.value}`")

        return classes[llm_type](**kwargs)


class DummyLLM(LLM):
    USER_PROMPT = "Press ENTER to generate a demo LLM response\n"

    def __init__(self, tokens_per_second: int = 8, **kwargs: Any) -> None:
        super().__init__(user_prompt=self.USER_PROMPT, **kwargs)

        self._encoder = tiktoken.encoding_for_model("gpt-4")
        self._tokens_delay = 1 / tokens_per_second

        data_file_path = os.path.join(os.path.dirname(__file__), "../../resources/demo/demo_data.json")
        with open(data_file_path, encoding="utf8") as data_file:
            demo_data = json.loads(data_file.read())
        self._sentences = demo_data["demo_sentences"]

    def _tokenize(self, text: str) -> Sequence[str]:
        tokens = [self._encoder.decode([i]) for i in self._encoder.encode(text)]
        return tokens

    def _chat(self, prompt: str) -> Generator[str, None, None]:
        try:
            text_index = int(prompt)
            sentence = self._sentences[text_index]
        except ValueError:
            sentence = self._sentences[random.randint(0, len(self._sentences) - 1)]

        for i in self._tokenize(text=sentence):
            time.sleep(self._tokens_delay)
            yield i


class OpenAILLM(LLM):
    def __init__(
            self,
            access_key: str,
            model_name: str = "gpt-3.5-turbo",
            **kwargs: Any) -> None:
        super().__init__(**kwargs)

        from openai import OpenAI
        self._model_name = model_name
        self._client = OpenAI(api_key=access_key)

        self._message_stack = [{"role": "system", "content": self.SYSTEM_PROMPT}]

    def _append_user_message(self, message: str) -> None:
        self._message_stack.append({"role": "user", "content": message})

    def _append_assistant_message(self, message: str) -> None:
        self._message_stack.append({"role": "assistant", "content": message})

    def _chat(self, prompt: str) -> Generator[str, None, None]:
        self._append_user_message(prompt)
        stream = self._client.chat.completions.create(
            model=self._model_name,
            messages=self._message_stack,
            stream=True)
        assistant_message = ""
        for chunk in stream:
            token = chunk.choices[0].delta.content
            yield token
            if token is not None:
                assistant_message += token
        self._append_assistant_message(message=assistant_message)


class Synthesizers(Enum):
    OPENAI = "openai"
    PICOVOICE_ORCA = "picovoice_orca"
    PICOVOICE_ORCA_STREAMING = "picovoice_orca_streaming"


class Synthesizer:
    def __init__(
            self,
            samplerate: int,
            play_audio_callback: Callable,
            timestamps: Timestamps,
            input_streamable: bool = False,
    ) -> None:
        self.samplerate = samplerate
        self.input_streamable = input_streamable

        self._play_audio_callback = play_audio_callback
        self._timestamps = timestamps

    def synthesize(self, text: str, **kwargs: Any) -> None:
        self._timestamps.time_first_synthesis_request = time.time()
        pcm = self._synthesize(text=text, **kwargs)
        self._timestamps.time_first_audio = time.time()
        self._play_audio_callback(pcm)

    def flush(self) -> None:
        pass

    def reset(self) -> None:
        pass

    def start(self) -> None:
        pass

    def wait(self) -> None:
        pass

    def wait_and_terminate(self) -> None:
        pass

    def delete(self) -> None:
        pass

    def _synthesize(self, text: str, **kwargs: Any) -> NDArray:
        raise NotImplementedError(
            f"Method `_synthesize` must be implemented in a subclass of {self.__class__.__name__}")

    @classmethod
    def create(cls, engine: Union[str, Synthesizers], **kwargs: Any) -> 'Synthesizer':
        classes = {
            Synthesizers.PICOVOICE_ORCA: PicovoiceOrcaSynthesizer,
            Synthesizers.PICOVOICE_ORCA_STREAMING: PicovoiceOrcaStreamingSynthesizer,
            Synthesizers.OPENAI: OpenAISynthesizer,
        }

        if engine not in classes:
            raise NotImplementedError(f"Cannot create {cls.__name__} of type `{engine.value}`")

        return classes[engine](**kwargs)


class OpenAISynthesizer(Synthesizer):
    pass


class PicovoiceOrcaSynthesizer(Synthesizer):
    def __init__(
            self,
            access_key: str,
            model_path: Optional[str] = None,
            library_path: Optional[str] = None,
            **kwargs: Any,
    ) -> None:
        self._orca = pvorca.create(access_key=access_key, model_path=model_path, library_path=library_path)

        super().__init__(samplerate=self._orca.sample_rate, **kwargs)

    @staticmethod
    def _clean_text(text: str) -> str:
        text = text.replace("\n", " ")
        text = text.replace("\r", " ")
        text = text.replace("\t", " ")
        return text

    def _synthesize(self, text: str, **kwargs: Any) -> Sequence[int]:
        cleaned_text = self._clean_text(text)
        return self._orca.synthesize(text=cleaned_text)[0]

    def delete(self) -> None:
        self._orca.delete()


class PicovoiceOrcaStreamingSynthesizer(Synthesizer):
    NUM_TOKENS_PER_PCM_CHUNK = 8

    @dataclass
    class OrcaTextInput:
        text: str
        flush: bool

    def __init__(
            self,
            play_audio_callback: Callable,
            timestamps: Timestamps,
            access_key: str,
            model_path: Optional[str] = None,
            library_path: Optional[str] = None,
    ) -> None:
        self._orca = pvorca.create(access_key=access_key, model_path=model_path, library_path=library_path)
        super().__init__(
            samplerate=self._orca.sample_rate,
            play_audio_callback=play_audio_callback,
            timestamps=timestamps,
            input_streamable=True)

        self._orca_stream = self._orca.open_stream()
        self._sample_rate = self._orca.sample_rate

        self._queue: Queue[Optional[PicovoiceOrcaStreamingSynthesizer.OrcaTextInput]] = Queue()
        self._play_audio_callback = play_audio_callback

        self._thread = threading.Thread(target=self._run)

        self._timestamps = timestamps
        self._total_processing_time = 0.0

        self._num_tokens = 0
        self._first_token = True

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

            if self._first_token:
                self._timestamps.time_first_synthesis_request = time.time()
                self._first_token = False

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
            self._total_processing_time += processing_time

            if len(pcm) > 0:
                if self._timestamps.time_first_audio < 0.0:
                    self._timestamps.time_first_audio = time.time()

                    self._timestamps.initial_audio_delay = \
                        self._compute_first_audio_delay(pcm=pcm, processing_time=processing_time)
                    time.sleep(self._timestamps.initial_audio_delay)

                self._play_audio_callback(pcm)

    @property
    def total_processing_time(self) -> float:
        return self._total_processing_time

    def synthesize(self, text: str, **kwargs: Any) -> None:
        self._queue.put_nowait(self.OrcaTextInput(text=text, flush=False))

    def flush(self) -> None:
        self._queue.put_nowait(self.OrcaTextInput(text="", flush=True))

    def start(self) -> None:
        self._thread.start()

    def reset(self) -> None:
        self._num_tokens = 0
        self._first_token = True

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

    def delete(self) -> None:
        self._orca.delete()


class StreamingAudioOutput:
    def __init__(self, device_info: dict) -> None:
        self._device_info = device_info
        self._queue: Queue[NDArray] = Queue()

        self._buffer = None
        self._stream = None
        self._sample_rate = None
        self._blocksize = None

    def _callback(self, outdata: NDArray, frames: int, time: Any, status: Any) -> None:
        if self._queue.empty():
            outdata[:] = 0
            return
        data = self._queue.get()
        outdata[:, 0] = data

    def set_sample_rate(self, sample_rate: int) -> None:
        self._sample_rate = sample_rate

    def play(self, pcm_chunk: Sequence[int]) -> None:
        if self._stream is None:
            raise ValueError("Stream is not started. Call `start` method first.")

        pcm_chunk = np.array(pcm_chunk, dtype=np.int16)

        if self._buffer is not None:
            pcm_chunk = np.concatenate([self._buffer, pcm_chunk])
            self._buffer = None

        length = pcm_chunk.shape[0]
        for index_block in range(0, length, self._blocksize):
            if (length - index_block) < self._blocksize:
                self._buffer = pcm_chunk[index_block: index_block + (length - index_block)]
            else:
                self._queue.put_nowait(pcm_chunk[index_block: index_block + self._blocksize])

    def start(self, sample_rate: int) -> None:
        self._sample_rate = sample_rate
        self._blocksize = self._sample_rate // 20
        self._stream = sd.OutputStream(
            channels=1,
            samplerate=self._sample_rate,
            dtype=np.int16,
            device=int(self._device_info["index"]),
            callback=self._callback,
            blocksize=self._blocksize)
        self._stream.start()

    def wait_and_terminate(self) -> None:
        self.wait()
        self.terminate()

    def wait(self):
        if self._buffer is not None:
            chunk = np.zeros(self._blocksize, dtype=np.int16)
            chunk[:self._buffer.shape[0]] = self._buffer
            self._queue.put_nowait(chunk)

        time_interval = self._blocksize / self._sample_rate
        while not self._queue.empty():
            time.sleep(time_interval)

        time.sleep(time_interval)

    def terminate(self):
        self._stream.stop()
        self._stream.close()

    @classmethod
    def from_default_device(cls):
        return cls(device_info=sd.query_devices(kind="output"))


__all__ = [
    "LLMs",
    "LLM",
    "StreamingAudioOutput",
    "Synthesizers",
    "Synthesizer",
    "Timestamps",
]
