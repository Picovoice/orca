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
    Optional,
    Sequence,
    Tuple,
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
    def create(cls, x: Transcribers, **kwargs: Any) -> 'Transcriber':
        subclasses = {
            Transcribers.PICOVOICE_CHEETAH: PicovoiceCheetahTranscriber,
        }

        if x not in subclasses:
            raise NotImplementedError(f"Cannot create {cls.__name__} of type `{x.value}`")

        return subclasses[x](**kwargs)


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
