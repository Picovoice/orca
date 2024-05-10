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

from enum import Enum
from typing import (
    Any,
    Dict,
    Optional,
)

from pvrecorder import PvRecorder

from .llm import LLMs
from .transcriber import Transcriber, Transcribers


class UserInputs(Enum):
    VOICE = "voice"
    TEXT = "text"


class UserInput:
    def get_user_input(self) -> str:
        raise NotImplementedError()

    @classmethod
    def create(cls, x: UserInputs, **kwargs: Any) -> 'UserInput':
        subclasses = {
            UserInputs.VOICE: VoiceUserInput,
            UserInputs.TEXT: TextUserInput,
        }

        if x not in subclasses:
            raise NotImplementedError(f"Cannot create {cls.__name__} of type `{x.value}`")

        return subclasses[x](**kwargs)


class VoiceUserInput(UserInput):
    def __init__(
            self,
            audio_device_index: int,
            transcriber: Transcribers,
            transcriber_params: Dict[str, Any],
    ) -> None:
        self._transcriber = Transcriber.create(transcriber, **transcriber_params)
        self._recorder = PvRecorder(frame_length=self._transcriber.frame_length, device_index=audio_device_index)

    def get_user_input(self) -> str:
        print("Listening ...")
        if not self._recorder.is_recording:
            self._recorder.start()

        transcript = ""
        try:
            while True:
                partial_transcript, is_endpoint = self._transcriber.process(self._recorder.read())
                transcript += partial_transcript
                if is_endpoint:
                    final_transcript = self._transcriber.flush()
                    transcript += final_transcript
                    self._recorder.stop()
                    return transcript
        except Exception as e:
            self._recorder.stop()
            raise e


class TextUserInput(UserInput):
    USER_PROMPT = "Your question: "
    USER_PROMPT_DUMMY_LLM = "Press ENTER to generate a demo LLM response "

    def __init__(self, llm_type: LLMs, prompt: Optional[str] = None) -> None:
        if prompt is not None:
            self._prompt = prompt
        else:
            self._prompt = self.USER_PROMPT_DUMMY_LLM if llm_type is LLMs.DUMMY else self.USER_PROMPT

    def get_user_input(self) -> str:
        return input(self._prompt)


__all__ = [
    "UserInput",
    "UserInputs",
]
