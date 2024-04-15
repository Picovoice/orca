import json
import os
import random
import time
from enum import Enum
from typing import Any, Generator, Optional, Sequence

import tiktoken


class LLMs(Enum):
    DUMMY = "dummy"
    OPENAI = "openai"


class LLM:
    SYSTEM_PROMPT = """
    You are a voice assistant in customer service.
    Use natural, conversational language that are clear and easy to follow (short sentences, simple words).
    Only use english letters and punctuation, no special characters.
    Don't ever use numbers directly. Verbalize them (e.g. "five" instead of "5").
    If the user asks for anything specific, invent some example data that would be typical in customer service.
    """
    DEFAULT_USER_PROMPT = "Your question: "
    DEFAULT_ASSISTANT_PROMPT = "Assistant: "

    def __init__(
            self,
            user_prompt: Optional[str] = None,
            assistant_prompt: Optional[str] = None,
    ) -> None:
        self._user_prompt = user_prompt if user_prompt is not None else self.DEFAULT_USER_PROMPT
        self._assistant_prompt = assistant_prompt if assistant_prompt is not None else self.DEFAULT_ASSISTANT_PROMPT

    def chat(self, user_input: str) -> Generator[str, None, None]:
        for token in self._chat(user_input=user_input):
            yield token

    def _chat(self, user_input: str) -> Generator[str, None, None]:
        raise NotImplementedError(
            f"Method `chat_stream` must be implemented in a subclass of {self.__class__.__name__}")

    def get_user_input(self, previous_prompt: Optional[str] = None, synthesizer_name: Optional[str] = None) -> str:
        if previous_prompt is not None:
            assert synthesizer_name
            input(f"Press Enter to run the same program again but using {synthesizer_name} ")
            return previous_prompt

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
    USER_PROMPT = "Press ENTER to generate a demo LLM response "
    ASSISTANT_PROMPT = ""

    def __init__(self, tokens_per_second: int = 8) -> None:
        super().__init__(user_prompt=self.USER_PROMPT, assistant_prompt=self.ASSISTANT_PROMPT)

        self._encoder = tiktoken.encoding_for_model("gpt-4")
        self._tokens_delay = 1 / tokens_per_second

        data_file_path = os.path.join(os.path.dirname(__file__), "../../../resources/demo/demo_data.json")
        with open(data_file_path, encoding="utf8") as data_file:
            self._sentences = json.loads(data_file.read())["demo_sentences"]

        random.seed(7777)

    def _tokenize(self, text: str) -> Sequence[str]:
        tokens = [self._encoder.decode([i]) for i in self._encoder.encode(text)]
        return tokens

    def _chat(self, user_input: str) -> Generator[str, None, None]:
        sentence = self._sentences[random.randint(0, len(self._sentences) - 1)]

        for i in self._tokenize(text=sentence):
            time.sleep(self._tokens_delay)
            yield i


class OpenAILLM(LLM):
    MODEL_NAME = "gpt-3.5-turbo"
    RANDOM_SEED = 7777

    def __init__(
            self,
            access_key: str,
            model_name: str = MODEL_NAME,
            **kwargs: Any,
    ) -> None:
        super().__init__(**kwargs)

        from openai import OpenAI
        self._model_name = model_name
        self._client = OpenAI(api_key=access_key)

        self._message_history = [{"role": "system", "content": self.SYSTEM_PROMPT}]

    def _remove_last_user_message(self) -> None:
        self._message_history = self._message_history[:-1]

    def _append_user_message(self, message: str) -> None:
        self._message_history.append({"role": "user", "content": message})

    def _append_assistant_message(self, message: str) -> None:
        self._message_history.append({"role": "assistant", "content": message})

    def _chat(self, user_input: str, append_to_history: bool = False) -> Generator[str, None, None]:
        self._append_user_message(user_input)
        stream = self._client.chat.completions.create(
            model=self._model_name,
            messages=self._message_history,
            seed=self.RANDOM_SEED,
            stream=True)
        assistant_message = ""
        for chunk in stream:
            token = chunk.choices[0].delta.content
            yield token
            if token is not None:
                assistant_message += token
        if not append_to_history:
            self._remove_last_user_message()


__all__ = [
    "LLMs",
    "LLM",
]
