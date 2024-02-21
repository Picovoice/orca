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

import { PvError } from "@picovoice/web-utils";
import { PvStatus } from "./types";

class OrcaError extends Error {
  private readonly _status: PvStatus;
  private readonly _shortMessage: string;
  private readonly _messageStack: string[];

  constructor(status: PvStatus, message: string, messageStack: string[] = [], pvError: PvError | null = null) {
    super(OrcaError.errorToString(message, messageStack, pvError));
    this._status = status;
    this.name = 'OrcaError';
    this._shortMessage = message;
    this._messageStack = messageStack;
  }

  get status(): PvStatus {
    return this._status;
  }

  get shortMessage(): string {
    return this._shortMessage;
  }

  get messageStack(): string[] {
    return this._messageStack;
  }

  private static errorToString(
    initial: string,
    messageStack: string[],
    pvError: PvError | null = null,
  ): string {
    let msg = initial;

    if (pvError) {
      const pvErrorMessage = pvError.getErrorString();
      if (pvErrorMessage.length > 0) {
        msg += `\nDetails: ${pvErrorMessage}`;
      }
    }

    if (messageStack.length > 0) {
      msg += `: ${messageStack.reduce((acc, value, index) =>
        acc + '\n  [' + index + '] ' + value, '')}`;
    }

    return msg;
  }
}

class OrcaOutOfMemoryError extends OrcaError {
  constructor(message: string, messageStack?: string[], pvError: PvError | null = null) {
    super(PvStatus.OUT_OF_MEMORY, message, messageStack, pvError);
    this.name = 'OrcaOutOfMemoryError';
  }
}

class OrcaIOError extends OrcaError {
  constructor(message: string, messageStack: string[] = [], pvError: PvError | null = null) {
    super(PvStatus.IO_ERROR, message, messageStack, pvError);
    this.name = 'OrcaIOError';
  }
}

class OrcaInvalidArgumentError extends OrcaError {
  constructor(message: string, messageStack: string[] = [], pvError: PvError | null = null) {
    super(PvStatus.INVALID_ARGUMENT, message, messageStack, pvError);
    this.name = 'OrcaInvalidArgumentError';
  }
}

class OrcaStopIterationError extends OrcaError {
  constructor(message: string, messageStack: string[] = [], pvError: PvError | null = null) {
    super(PvStatus.STOP_ITERATION, message, messageStack, pvError);
    this.name = 'OrcaStopIterationError';
  }
}

class OrcaKeyError extends OrcaError {
  constructor(message: string, messageStack: string[] = [], pvError: PvError | null = null) {
    super(PvStatus.KEY_ERROR, message, messageStack, pvError);
    this.name = 'OrcaKeyError';
  }
}

class OrcaInvalidStateError extends OrcaError {
  constructor(message: string, messageStack: string[] = [], pvError: PvError | null = null) {
    super(PvStatus.INVALID_STATE, message, messageStack, pvError);
    this.name = 'OrcaInvalidStateError';
  }
}

class OrcaRuntimeError extends OrcaError {
  constructor(message: string, messageStack: string[] = [], pvError: PvError | null = null) {
    super(PvStatus.RUNTIME_ERROR, message, messageStack, pvError);
    this.name = 'OrcaRuntimeError';
  }
}

class OrcaActivationError extends OrcaError {
  constructor(message: string, messageStack: string[] = [], pvError: PvError | null = null) {
    super(PvStatus.ACTIVATION_ERROR, message, messageStack, pvError);
    this.name = 'OrcaActivationError';
  }
}

class OrcaActivationLimitReachedError extends OrcaError {
  constructor(message: string, messageStack: string[] = [], pvError: PvError | null = null) {
    super(PvStatus.ACTIVATION_LIMIT_REACHED, message, messageStack, pvError);
    this.name = 'OrcaActivationLimitReachedError';
  }
}

class OrcaActivationThrottledError extends OrcaError {
  constructor(message: string, messageStack: string[] = [], pvError: PvError | null = null) {
    super(PvStatus.ACTIVATION_THROTTLED, message, messageStack, pvError);
    this.name = 'OrcaActivationThrottledError';
  }
}

class OrcaActivationRefusedError extends OrcaError {
  constructor(message: string, messageStack: string[] = [], pvError: PvError | null = null) {
    super(PvStatus.ACTIVATION_REFUSED, message, messageStack, pvError);
    this.name = 'OrcaActivationRefusedError';
  }
}

export {
  OrcaError,
  OrcaOutOfMemoryError,
  OrcaIOError,
  OrcaInvalidArgumentError,
  OrcaStopIterationError,
  OrcaKeyError,
  OrcaInvalidStateError,
  OrcaRuntimeError,
  OrcaActivationError,
  OrcaActivationLimitReachedError,
  OrcaActivationThrottledError,
  OrcaActivationRefusedError,
};


export function pvStatusToException(
  pvStatus: PvStatus,
  errorMessage: string,
  messageStack: string[] = [],
  pvError: PvError | null = null
): OrcaError {
  switch (pvStatus) {
    case PvStatus.OUT_OF_MEMORY:
      return new OrcaOutOfMemoryError(errorMessage, messageStack, pvError);
    case PvStatus.IO_ERROR:
      return new OrcaIOError(errorMessage, messageStack, pvError);
    case PvStatus.INVALID_ARGUMENT:
      return new OrcaInvalidArgumentError(errorMessage, messageStack, pvError);
    case PvStatus.STOP_ITERATION:
      return new OrcaStopIterationError(errorMessage, messageStack, pvError);
    case PvStatus.KEY_ERROR:
      return new OrcaKeyError(errorMessage, messageStack, pvError);
    case PvStatus.INVALID_STATE:
      return new OrcaInvalidStateError(errorMessage, messageStack, pvError);
    case PvStatus.RUNTIME_ERROR:
      return new OrcaRuntimeError(errorMessage, messageStack, pvError);
    case PvStatus.ACTIVATION_ERROR:
      return new OrcaActivationError(errorMessage, messageStack, pvError);
    case PvStatus.ACTIVATION_LIMIT_REACHED:
      return new OrcaActivationLimitReachedError(errorMessage, messageStack, pvError);
    case PvStatus.ACTIVATION_THROTTLED:
      return new OrcaActivationThrottledError(errorMessage, messageStack, pvError);
    case PvStatus.ACTIVATION_REFUSED:
      return new OrcaActivationRefusedError(errorMessage, messageStack, pvError);
    default:
      // eslint-disable-next-line no-console
      console.warn(`Unmapped error code: ${pvStatus}`);
      return new OrcaError(pvStatus, errorMessage);
  }
}
