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
from dataclasses import dataclass
from typing import Sequence, Tuple

from typing import List


@dataclass
class TestSentences:
    text: str
    text_no_punctuation: str
    text_custom_pronunciation: str
    text_invalid: Sequence[str]


def get_test_data() -> Tuple[TestSentences, float]:
    data_file_path = os.path.join(os.path.dirname(__file__), "../../resources/.test/test_data.json")
    with open(data_file_path, encoding="utf8") as data_file:
        test_data = json.loads(data_file.read())
    return TestSentences(**test_data["test_sentences"]), test_data["wer_threshold"]


def get_model_paths() -> List[str]:
    model_folder = os.path.join(os.path.dirname(__file__), "../../lib/common")
    return [os.path.join(model_folder, model_name) for model_name in os.listdir(model_folder)]


__all__ = [
    "get_test_data",
    "get_model_paths",
]
