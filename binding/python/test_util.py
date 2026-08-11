#
# Copyright 2024-2026 Picovoice Inc.
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
import platform
import struct
import subprocess
import wave
from dataclasses import dataclass
from typing import Sequence, Optional

from _orca import Orca


def android_first_device_id():
    res = subprocess.run(["adb", "devices"], capture_output=True, text=True)
    serial_ids = [x.split("\t")[0] for x in res.stdout.replace("\r\n", "\n").split("\n")[1:]]
    return serial_ids[0].strip()


def get_platform_and_architecture() -> tuple[str, str]:
    platform_name = os.getenv("PLATFORM_NAME")
    if platform_name is None:
        raise Exception("Expected PLATFORM_NAME to exit. Is this being run in a pipeline?")

    arch = platform.machine()

    if platform_name == "ios":
        platform_name = "mac"

    if platform_name == "android":
        serial_id = android_first_device_id()
        arch = subprocess.run([
            "adb", "-s", serial_id, "shell", "getprop", "ro.product.cpu.abi"
        ], capture_output=True, text=True).stdout.strip()

    return arch, platform_name


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


def get_model_path(model_name) -> str:
    return os.path.join(os.path.dirname(__file__), "../../lib/common", model_name)


def get_data_file_path(platform_name: str, arch: str) -> Optional[str]:
    data_file_path = os.path.join(os.path.dirname(__file__), f"../../resources/.test/{platform_name}-{arch}_test_data.json")
    if os.path.isfile(data_file_path):
        return data_file_path

    print(f"WARNING: test data for {platform_name}-{arch} does not exist. Falling back to less accurate test data")

    parent_dir = os.path.join(os.path.dirname(__file__), "../../resources/.test/")
    for name in os.listdir(parent_dir):
        is_file = os.path.isfile(os.path.join(parent_dir, name))
        if is_file and name.startswith(f"{platform_name}-") and name.endswith(".json"):
            data_file_path = os.path.join(os.path.dirname(__file__), f"../../resources/.test/{name}")
            return data_file_path

    return None


def get_test_data() -> TestData:
    arch, platform_name = get_platform_and_architecture()
    data_file_path = get_data_file_path(platform_name, arch)
    if data_file_path is None:
        print(f"ERROR: unable to find test_data.json for the current platform `{platform_name}`")
        exit(1)

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
    "get_model_path",
    "read_wav_file",
]
