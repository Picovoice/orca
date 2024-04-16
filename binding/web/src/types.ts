/*
  Copyright 2024 Picovoice Inc.

  You may not use this file except in compliance with the license. A copy of the license is located in the "LICENSE"
  file accompanying this source.

  Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
  an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
  specific language governing permissions and limitations under the License.
*/

import { PvModel } from '@picovoice/web-utils';

// eslint-disable-next-line no-shadow
export enum PvStatus {
  SUCCESS = 10000,
  OUT_OF_MEMORY,
  IO_ERROR,
  INVALID_ARGUMENT,
  STOP_ITERATION,
  KEY_ERROR,
  INVALID_STATE,
  RUNTIME_ERROR,
  ACTIVATION_ERROR,
  ACTIVATION_LIMIT_REACHED,
  ACTIVATION_THROTTLED,
  ACTIVATION_REFUSED,
}

export type SynthesizeParams = {
  speechRate?: number
  randomState?: number
}

/**
 * OrcaModel types
 */
export type OrcaModel = PvModel;

export type OrcaWorkerInitRequest = {
  command: 'init';
  accessKey: string;
  modelPath: string;
  wasm: string;
  wasmSimd: string;
  sdk: string;
};

export type OrcaWorkerSynthesizeRequest = {
  command: 'synthesize';
  text: string;
  synthesizeParams?: SynthesizeParams;
};

export type OrcaWorkerReleaseRequest = {
  command: 'release';
};

export type OrcaWorkerRequest =
  | OrcaWorkerInitRequest
  | OrcaWorkerSynthesizeRequest
  | OrcaWorkerReleaseRequest;

export type OrcaWorkerFailureResponse = {
  command: 'failed' | 'error';
  status: PvStatus;
  shortMessage: string;
  messageStack: string[];
};

export type OrcaWorkerInitResponse =
  | OrcaWorkerFailureResponse
  | {
  command: 'ok';
  version: string;
  numSymbols: number;
  validCharacters: string[];
  maxCharacterLimit: number;
  sampleRate: number;
};

export type OrcaWorkerSynthesizeResponse =
  | OrcaWorkerFailureResponse
  | {
  command: 'ok';
  result: Int16Array;
};

export type OrcaWorkerReleaseResponse =
  | OrcaWorkerFailureResponse
  | {
  command: 'ok';
};

export type OrcaWorkerResponse =
  | OrcaWorkerInitResponse
  | OrcaWorkerSynthesizeResponse
  | OrcaWorkerReleaseResponse;
