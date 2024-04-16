from enum import Enum
from typing import (
    Any,
    Optional,
    Sequence,
    Tuple,
    Union,
)

from pvcheetah import CheetahActivationLimitError, create


class Transcribers(Enum):
    PICOVOICE_CHEETAH = "picovoice_cheetah"


class Transcriber:
    def process(self, pcm_frame: Sequence[int]) -> Tuple[str, bool]:
        raise NotImplementedError()

    def flush(self) -> str:
        raise NotImplementedError()

    @property
    def frame_length(self) -> int:
        raise NotImplementedError()

    @classmethod
    def create(cls, x: Union[str, Transcribers], **kwargs: Any) -> 'Transcriber':
        try:
            x = Transcribers(x)
            subclass = {
                Transcribers.PICOVOICE_CHEETAH: PicovoiceCheetahTranscriber,
            }[x]
        except KeyError:
            raise ValueError(f"Invalid transcriber type `{x}`")

        return subclass(**kwargs)


class PicovoiceCheetahTranscriber(Transcriber):
    def __init__(
            self,
            access_key: str,
            library_path: Optional[str] = None,
            model_path: Optional[str] = None,
            endpoint_duration_sec: float = 1.0,
            enable_automatic_punctuation: bool = True
    ) -> None:
        self._cheetah = create(
            access_key=access_key,
            library_path=library_path,
            model_path=model_path,
            endpoint_duration_sec=endpoint_duration_sec,
            enable_automatic_punctuation=enable_automatic_punctuation)

    def process(self, pcm_frame: Sequence[int]) -> Tuple[str, bool]:
        try:
            partial_transcript, is_endpoint = self._cheetah.process(pcm_frame)
        except CheetahActivationLimitError:
            raise ValueError("Cheetah activation limit reached.")
        return partial_transcript, is_endpoint

    def flush(self) -> str:
        try:
            return self._cheetah.flush()
        except CheetahActivationLimitError:
            raise ValueError("Cheetah activation limit reached.")

    @property
    def frame_length(self) -> int:
        return self._cheetah.frame_length


__all__ = [
    "Transcriber",
    "Transcribers",
]
