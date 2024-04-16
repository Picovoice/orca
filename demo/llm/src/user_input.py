import time
from enum import Enum
from typing import (
    Any,
    Dict,
)

from pvrecorder import PvRecorder

from .transcriber import Transcriber, Transcribers
from .util import ListeningAnimation


class UserInputs(Enum):
    VOICE = "voice"
    TEXT = "text"


class UserInput:
    def get_user_prompt(self) -> str:
        raise NotImplementedError()

    @classmethod
    def create(cls, x: UserInputs, **kwargs: Any) -> 'UserInput':
        try:
            subclass = {
                UserInputs.VOICE: VoiceUserInput,
                UserInputs.TEXT: TextUserInput,
            }[x]
        except KeyError:
            raise ValueError(f"Invalid input type `{x}`")

        return subclass(**kwargs)


class VoiceUserInput(UserInput):
    STOP_MESSAGE = "Sent question to LLM!"

    def __init__(
            self,
            audio_device_index: int,
            transcriber: Transcribers,
            transcriber_params: Dict[str, Any],
    ) -> None:
        self._transcriber = Transcriber.create(transcriber, **transcriber_params)
        self._recorder = PvRecorder(frame_length=self._transcriber.frame_length, device_index=audio_device_index)

    def get_user_prompt(self) -> str:
        animation = ListeningAnimation()
        animation.start()
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
                    animation.stop(message=self.STOP_MESSAGE)
                    return transcript
        except Exception as e:
            self._recorder.stop()
            animation.stop(message=self.STOP_MESSAGE)
            raise e


class TextUserInput(UserInput):
    def __init__(self, prompt: str) -> None:
        self._prompt = prompt

    def get_user_prompt(self) -> str:
        return input(self._prompt)


__all__ = [
    "UserInput",
    "UserInputs",
]
