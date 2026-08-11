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
import subprocess

from dataclasses import dataclass
from typing import List, Sequence, Optional


def android_first_device_id():
    res = subprocess.run(["adb", "devices"], capture_output=True, text=True)
    serial_ids = [x.split("\t")[0] for x in res.stdout.replace("\r\n", "\n").split("\n")[1:]]
    return serial_ids[0].strip()


def get_platform_and_architecture() -> tuple[str, str]:
    platform_name = os.getenv("PLATFORM_NAME")
    if platform_name is None:
        raise Exception("Expected PLATFORM_NAME to exit. Is this being run in a pipeline?")

    arch = platform.machine()

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


def get_data_file_path(platform_name: str, arch: str) -> Optional[str]:
    data_file_path = os.path.join(
        os.path.dirname(__file__),
        f"../../../resources/.test/{platform_name}-{arch}_test_data.json")
    if os.path.isfile(data_file_path):
        return data_file_path

    print(f"WARNING: test data for {platform_name}-{arch} does not exist. Falling back to less accurate test data")

    parent_dir = os.path.join(os.path.dirname(__file__), "../../../resources/.test/")
    for name in os.listdir(parent_dir):
        is_file = os.path.isfile(os.path.join(parent_dir, name))
        if is_file and name.startswith(f"{platform_name}-") and name.endswith(".json"):
            data_file_path = os.path.join(os.path.dirname(__file__), parent_dir, name)
            return data_file_path

    return None


def get_test_data() -> SentenceTestData:
    arch, platform_name = get_platform_and_architecture()
    data_file_path = get_data_file_path(platform_name, arch)
    if data_file_path is None:
        print(f"ERROR: unable to find test_data.json for the current platform `{platform_name}`")
        exit(1)

    with open(data_file_path, encoding="utf8") as data_file:
        test_data = json.loads(data_file.read())

    sentence_tests = [SentenceTestData(**data) for data in test_data['tests']['sentence_tests']]
    return sentence_tests[0]


def get_model_paths() -> List[str]:
    model_folder = os.path.join(os.path.dirname(__file__), "../../..", "lib/common")
    return [os.path.join(model_folder, model_name) for model_name in os.listdir(model_folder)]


__all__ = [
    "get_model_paths",
    "get_test_data",
]