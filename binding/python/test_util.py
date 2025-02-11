#
# Copyright 2024-2025 Picovoice Inc.
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
class SentenceTestData:
    language: str
    models: Sequence[str]
    random_state: int
    text: str
    text_no_punctuation: str
    text_custom_pronunciation: str


@dataclass
class AlignmentTestData:
    language: str
    model: str
    random_state: int
    text_alignment: str
    alignments: Sequence[Orca.WordAlignment]


@dataclass
class InvalidTestData:
    language: str
    models: Sequence[str]
    text_invalid: Sequence[str]


@dataclass
class TestData:
    sentence_tests: Sequence[SentenceTestData]
    alignment_tests: Sequence[AlignmentTestData]
    invalid_tests: Sequence[InvalidTestData]
    audio_data_folder: str


def read_wav_file(path: str) -> Sequence[int]:
    with wave.open(path, 'rb') as f:
        buffer = f.readframes(f.getnframes())
        # minus 4 because of the header
        return struct.unpack(f"{f.getnframes() - 4}h", buffer)


def get_model_path(model_name) -> List[str]:
    return os.path.join(os.path.dirname(__file__), "../../lib/common", model_name)


def get_test_data() -> TestData:
    data_file_path = os.path.join(os.path.dirname(__file__), "../../resources/.test/test_data.json")
    with open(data_file_path, encoding="utf8") as data_file:
        test_data = json.loads(data_file.read())

    sentence_tests = [SentenceTestData(**data) for data in test_data['tests']['sentence_tests']]

    alignment_tests = []
    for alignment_test_data in test_data['tests']['alignment_tests']:
        alignments = []
        for word_data in alignment_test_data.pop("alignments"):
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
        alignment_tests.append(AlignmentTestData(alignments=alignments, **alignment_test_data))

    invalid_tests = [InvalidTestData(**data) for data in test_data['tests']['invalid_tests']]

    test_data = TestData(
        sentence_tests=sentence_tests,
        alignment_tests=alignment_tests,
        invalid_tests=invalid_tests,
        audio_data_folder=test_data['audio_data_folder'])

    return test_data


__all__ = [
    "get_test_data",
    "get_model_paths",
    "read_wav_file",
]
