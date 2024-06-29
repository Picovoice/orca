//
// Copyright 2024 Picovoice Inc.
//
// You may not use this file except in compliance with the license. A copy of the license is located in the "LICENSE"
// file accompanying this source.
//
// Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
// an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
// specific language governing permissions and limitations under the License.
//
'use strict';

import PvStatus from './pv_status_t';

export class OrcaError extends Error {
  private readonly _message: string;
  private readonly _messageStack: string[];

  constructor(message: string, messageStack: string[] = []) {
    super(OrcaError.errorToString(message, messageStack));
    this._message = message;
    this._messageStack = messageStack;
  }

  get message(): string {
    return this._message;
  }

  get messageStack(): string[] {
    return this._messageStack;
  }

  private static errorToString(
    initial: string,
    messageStack: string[],
  ): string {
    let msg = initial;

    if (messageStack.length > 0) {
      msg += `: ${messageStack.reduce(
        (acc, value, index) => acc + '\n  [' + index + '] ' + value,
        '',
      )}`;
    }

    return msg;
  }
}

export class OrcaOutOfMemoryError extends OrcaError {
}

export class OrcaIOError extends OrcaError {
}

export class OrcaInvalidArgumentError extends OrcaError {
}

export class OrcaStopIterationError extends OrcaError {
}

export class OrcaKeyError extends OrcaError {
}

export class OrcaInvalidStateError extends OrcaError {
}

export class OrcaRuntimeError extends OrcaError {
}

export class OrcaActivationError extends OrcaError {
}

export class OrcaActivationLimitReachedError extends OrcaError {
}

export class OrcaActivationThrottledError extends OrcaError {
}

export class OrcaActivationRefusedError extends OrcaError {
}

export function pvStatusToException(
  pvStatus: PvStatus,
  errorMessage: string,
  messageStack: string[] = [],
): void {
  switch (pvStatus) {
    case PvStatus.OUT_OF_MEMORY:
      throw new OrcaOutOfMemoryError(errorMessage, messageStack);
    case PvStatus.IO_ERROR:
      throw new OrcaIOError(errorMessage, messageStack);
    case PvStatus.INVALID_ARGUMENT:
      throw new OrcaInvalidArgumentError(errorMessage, messageStack);
    case PvStatus.STOP_ITERATION:
      throw new OrcaStopIterationError(errorMessage, messageStack);
    case PvStatus.KEY_ERROR:
      throw new OrcaKeyError(errorMessage, messageStack);
    case PvStatus.INVALID_STATE:
      throw new OrcaInvalidStateError(errorMessage, messageStack);
    case PvStatus.RUNTIME_ERROR:
      throw new OrcaRuntimeError(errorMessage, messageStack);
    case PvStatus.ACTIVATION_ERROR:
      throw new OrcaActivationError(errorMessage, messageStack);
    case PvStatus.ACTIVATION_LIMIT_REACHED:
      throw new OrcaActivationLimitReachedError(errorMessage, messageStack);
    case PvStatus.ACTIVATION_THROTTLED:
      throw new OrcaActivationThrottledError(errorMessage, messageStack);
    case PvStatus.ACTIVATION_REFUSED:
      throw new OrcaActivationRefusedError(errorMessage, messageStack);
    default:
      // eslint-disable-next-line no-console
      console.warn(`Unmapped error code: ${pvStatus}`);
      throw new OrcaError(errorMessage);
  }
}
