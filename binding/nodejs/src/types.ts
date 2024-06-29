/*
  Copyright 2024 Picovoice Inc.
  You may not use this file except in compliance with the license. A copy of the license is located in the "LICENSE"
  file accompanying this source.
  Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
  an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
  specific language governing permissions and limitations under the License.
*/

export type OrcaSynthesizeParams = {
  speechRate?: number;
  randomState?: number | null;
}

export type OrcaPhoneme = {
  phoneme: string;
  startSec: number;
  endSec: number;
}

export type OrcaAlignment = {
  word: string;
  startSec: number;
  endSec: number;
  phonemes: OrcaPhoneme[];
}

export type OrcaSynthesizeResult = {
  pcm: Int16Array;
  alignments: OrcaAlignment[];
}

export type OrcaSynthesizeToFileResult = OrcaAlignment[]

export type OrcaStreamSynthesizeResult = Int16Array | null

export type OrcaOptions = {
  modelPath?: string;
  libraryPath?: string;
};
