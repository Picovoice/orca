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
from dataclasses import dataclass

from typing import List, Sequence


@dataclass
class SentenceTestData:
    language: str
    models: Sequence[str]
    random_state: int
    text: str
    text_no_punctuation: str
    text_custom_pronunciation: str


def _load_test_data() -> str:
    data_file_path = os.path.join(os.path.dirname(__file__), "../../../resources/.test/test_data.json")
    with open(data_file_path, encoding="utf8") as data_file:
        test_data = json.loads(data_file.read())
    return test_data


def get_test_data() -> SentenceTestData:
    test_data = _load_test_data()
    sentence_tests = [SentenceTestData(**data) for data in test_data['tests']['sentence_tests']]
    return sentence_tests[0]


def get_available_languages() -> List[str]:
    test_data = _load_test_data()
    return [x["language"] for x in test_data["tests"]["sentence_tests"]]


def get_available_genders() -> List[str]:
    return ["male", "female"]


def get_model_path(language: str, gender: str) -> str:
    model_name = f'orca_params_{language}_{gender}.pv'
    model_path = os.path.join(os.path.dirname(__file__), f"../../../lib/common/{model_name}")
    if os.path.exists(model_path):
        return model_path
    else:
        return None


__all__ = [
    "get_test_data",
    "get_available_languages",
    "get_available_genders",
    "get_model_path",
]
