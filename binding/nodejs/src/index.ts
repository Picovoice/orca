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
'use strict';

import { Orca, OrcaStream } from './orca';

import {
  OrcaActivationError,
  OrcaActivationLimitReachedError,
  OrcaActivationRefusedError,
  OrcaActivationThrottledError,
  OrcaError,
  OrcaInvalidArgumentError,
  OrcaInvalidStateError,
  OrcaIOError,
  OrcaKeyError,
  OrcaOutOfMemoryError,
  OrcaRuntimeError,
  OrcaStopIterationError,
} from './errors';

import {
  OrcaSynthesizeParams,
  OrcaPhoneme,
  OrcaAlignment,
  OrcaSynthesizeResult,
  OrcaSynthesizeToFileResult,
  OrcaStreamSynthesizeResult,
  OrcaInputOptions,
  OrcaOptions,
} from './types';

export {
  Orca,
  OrcaStream,
  OrcaPhoneme,
  OrcaAlignment,
  OrcaSynthesizeParams,
  OrcaSynthesizeResult,
  OrcaSynthesizeToFileResult,
  OrcaStreamSynthesizeResult,
  OrcaInputOptions,
  OrcaOptions,
  OrcaActivationError,
  OrcaActivationLimitReachedError,
  OrcaActivationRefusedError,
  OrcaActivationThrottledError,
  OrcaError,
  OrcaInvalidArgumentError,
  OrcaInvalidStateError,
  OrcaIOError,
  OrcaKeyError,
  OrcaOutOfMemoryError,
  OrcaRuntimeError,
  OrcaStopIterationError,
};
