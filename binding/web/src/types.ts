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

/**
 * OrcaModel types
 */
export type OrcaModel = PvModel;

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

export type OrcaStreamSynthesizeResult = Int16Array | null

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
  synthesizeParams: OrcaSynthesizeParams;
};

export type OrcaWorkerReleaseRequest = {
  command: 'release';
};

export type OrcaWorkerStreamOpenRequest = {
  command: 'streamOpen';
  synthesizeParams: OrcaSynthesizeParams;
}

export type OrcaWorkerStreamSynthesizeRequest = {
  command: 'streamSynthesize';
  text: string;
};

export type OrcaWorkerStreamFlushRequest = {
  command: 'streamFlush';
};

export type OrcaWorkerStreamCloseRequest = {
  command: 'streamClose';
};

export type OrcaWorkerRequest =
  | OrcaWorkerInitRequest
  | OrcaWorkerSynthesizeRequest
  | OrcaWorkerReleaseRequest
  | OrcaWorkerStreamOpenRequest
  | OrcaWorkerStreamSynthesizeRequest
  | OrcaWorkerStreamFlushRequest
  | OrcaWorkerStreamCloseRequest;

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
  result: OrcaSynthesizeResult;
};

export type OrcaWorkerReleaseResponse =
  | OrcaWorkerFailureResponse
  | {
  command: 'ok';
};

export type OrcaWorkerStreamOpenResponse =
  | OrcaWorkerFailureResponse
  | {
  command: 'ok';
  result: any;
};

export type OrcaWorkerStreamSynthesizeResponse =
  | OrcaWorkerFailureResponse
  | {
  command: 'ok';
  result: OrcaStreamSynthesizeResult;
};

export type OrcaWorkerStreamFlushResponse =
  | OrcaWorkerFailureResponse
  | {
  command: 'ok';
  result: OrcaStreamSynthesizeResult;
};

export type OrcaWorkerStreamCloseResponse =
  | OrcaWorkerFailureResponse
  | {
  command: 'ok';
};

export type OrcaWorkerResponse =
  | OrcaWorkerInitResponse
  | OrcaWorkerSynthesizeResponse
  | OrcaWorkerReleaseResponse
  | OrcaWorkerStreamOpenResponse
  | OrcaWorkerStreamSynthesizeResponse
  | OrcaWorkerStreamFlushResponse;
