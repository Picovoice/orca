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

import json
import os
import random
import time
from enum import Enum
from typing import (
    Any,
    Generator,
    Sequence,
)

import tiktoken


class LLMs(Enum):
    DUMMY = "dummy"
    OPENAI = "openai"


class LLM:
    SYSTEM_MESSAGE = """
    You are a friendly voice assistant in customer service of an e-commerce platform.
    Use natural, conversational language that are clear and easy to follow (short sentences, simple words).
    Only use english letters and punctuation, no special characters.
    Keep the conversation flowing naturally.
    """

    def __init__(self, system_message: str = SYSTEM_MESSAGE) -> None:
        self._system_message = system_message

    def _chat(self, user_input: str) -> Generator[str, None, None]:
        raise NotImplementedError(
            f"Method `chat_stream` must be implemented in a subclass of {self.__class__.__name__}")

    def chat(self, user_input: str) -> Generator[str, None, None]:
        for token in self._chat(user_input=user_input):
            yield token

    @classmethod
    def create(cls, llm_type: LLMs, **kwargs) -> 'LLM':
        classes = {
            LLMs.DUMMY: DummyLLM,
            LLMs.OPENAI: OpenAILLM,
        }

        if llm_type not in classes:
            raise NotImplementedError(f"Cannot create {cls.__name__} of type `{llm_type.value}`")

        return classes[llm_type](**kwargs)

    def __str__(self) -> str:
        raise NotImplementedError()


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

        self._history = [{"role": "system", "content": self._system_message}]

    def _append_user_message(self, message: str) -> None:
        self._history.append({"role": "user", "content": message})

    def _append_assistant_message(self, message: str) -> None:
        self._history.append({"role": "assistant", "content": message})

    def _chat(self, user_input: str) -> Generator[str, None, None]:
        self._append_user_message(user_input)
        stream = self._client.chat.completions.create(
            model=self._model_name,
            messages=self._history,
            seed=self.RANDOM_SEED,
            temperature=0,
            top_p=0.05,
            stream=True)
        assistant_message = ""
        for chunk in stream:
            token = chunk.choices[0].delta.content
            yield token
            if token is not None:
                assistant_message += token
        self._append_assistant_message(assistant_message)

    def __str__(self) -> str:
        return f"ChatGPT ({self._model_name})"


class DummyLLM(LLM):
    TOKENS_PER_SECOND = 25

    def __init__(self, tokens_per_second: int = TOKENS_PER_SECOND) -> None:
        super().__init__(system_message="")

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

    def __str__(self) -> str:
        return "Dummy LLM"


__all__ = [
    "LLMs",
    "LLM",
]
