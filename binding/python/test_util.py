#
# Copyright 2024 Picovoice Inc.
#
# You may not use this file except in compliance with the license. A copy of the license is located in the "LICENSE"
# file accompanying this source.
#
# Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
# an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
# specific language governing permissions and limitations under the License.
#

import json
import os
import struct
import wave
from dataclasses import dataclass
from typing import List, Sequence

from _orca import Orca


@dataclass
class TestData:
    text: str
    text_no_punctuation: str
    text_custom_pronunciation: str
    text_alignment: str
    text_invalid: Sequence[str]
    alignments: Sequence[Orca.WordAlignment]
    random_state: int
    audio_data_folder: str
    exact_alignment_test_model_identifier: str


def read_wav_file(path: str) -> Sequence[int]:
    with wave.open(path, 'rb') as f:
        buffer = f.readframes(f.getnframes())
        # minus 4 because of the header
        return struct.unpack(f"{f.getnframes() - 4}h", buffer)


def get_model_paths() -> List[str]:
    model_folder = os.path.join(os.path.dirname(__file__), "../../lib/common")
    return [os.path.join(model_folder, model_name) for model_name in os.listdir(model_folder)]


def get_test_data() -> TestData:
    data_file_path = os.path.join(os.path.dirname(__file__), "../../resources/.test/test_data.json")
    with open(data_file_path, encoding="utf8") as data_file:
        test_data = json.loads(data_file.read())

    alignments = []
    for word_data in test_data.pop("alignments"):
        phonemes = []
        for phoneme_data in word_data["phonemes"]:
            phoneme = Orca.PhonemeAlignment(
                phoneme=phoneme_data["phoneme"],
                start_sec=phoneme_data["start_sec"],
                end_sec=phoneme_data["end_sec"])
            phonemes.append(phoneme)

        word = Orca.WordAlignment(
            word=word_data["word"],
            start_sec=word_data["start_sec"],
            end_sec=word_data["end_sec"],
            phonemes=phonemes)
        alignments.append(word)

    test_sentences = test_data.pop("test_sentences")
    test_data = TestData(
        alignments=alignments,
        **test_data,
        **test_sentences)

    return test_data


__all__ = [
    "get_test_data",
    "get_model_paths",
    "read_wav_file",
]
