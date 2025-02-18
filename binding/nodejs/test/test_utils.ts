//
// Copyright 2024-2025 Picovoice Inc.
//
// You may not use this file except in compliance with the license. A copy of the license is located in the "LICENSE"
// file accompanying this source.
//
// Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
// an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.
//
import * as path from 'path';

const ROOT_DIR = path.join(__dirname, '../../..');
const TEST_DATA_JSON = require(path.join(
  ROOT_DIR,
  'resources/.test/test_data.json',
));

export function getAudioFile(audioFile: string): string {
  return path.join(ROOT_DIR, 'resources/.test/wav', audioFile);
}

export function getTestData() {
  return TEST_DATA_JSON;
}

export function getModelPath(name: string): string {
  return path.join(
    ROOT_DIR,
    'lib/common/',
    name,
  );
}
