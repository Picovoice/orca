/*
  Copyright 2024 Picovoice Inc.

  You may not use this file except in compliance with the license. A copy of the license is located in the "LICENSE"
  file accompanying this source.

  Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
  an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
  specific language governing permissions and limitations under the License.
*/

/// <reference no-default-lib="false"/>
/// <reference lib="webworker" />

import { Orca } from './orca';
import { OrcaStreamSynthesizeResult, OrcaWorkerRequest, PvStatus } from './types';
import { OrcaError } from './orca_errors';

let orca: Orca | null = null;
let orcaStream: any = null;

const streamSynthesizeCallback = (synthesizeResult: OrcaStreamSynthesizeResult): void => {
  self.postMessage({
    command: 'ok',
    result: synthesizeResult,
  });
};

const streamSynthesizeErrorCallback = (error: OrcaError): void => {
  self.postMessage({
    command: 'error',
    status: error.status,
    shortMessage: error.shortMessage,
    messageStack: error.messageStack,
  });
};

/**
 * Orca worker handler.
 */
self.onmessage = async function(
  event: MessageEvent<OrcaWorkerRequest>,
): Promise<void> {
  switch (event.data.command) {
    case 'init':
      if (orca !== null) {
        self.postMessage({
          command: 'error',
          status: PvStatus.INVALID_STATE,
          shortMessage: 'Orca already initialized',
        });
        return;
      }
      try {
        Orca.setWasm(event.data.wasm);
        Orca.setWasmSimd(event.data.wasmSimd);
        orca = await Orca._init(
          event.data.accessKey,
          event.data.modelPath,
        );
        self.postMessage({
          command: 'ok',
          version: orca.version,
          sampleRate: orca.sampleRate,
          maxCharacterLimit: orca.maxCharacterLimit,
          validCharacters: orca.validCharacters,
        });
      } catch (e: any) {
        if (e instanceof OrcaError) {
          self.postMessage({
            command: 'error',
            status: e.status,
            shortMessage: e.shortMessage,
            messageStack: e.messageStack,
          });
        } else {
          self.postMessage({
            command: 'error',
            status: PvStatus.RUNTIME_ERROR,
            shortMessage: e.message,
          });
        }
      }
      break;
    case 'synthesize':
      if (orca === null) {
        self.postMessage({
          command: 'error',
          status: PvStatus.INVALID_STATE,
          shortMessage: 'Orca not initialized',
        });
        return;
      }
      try {
        self.postMessage({
          command: 'ok',
          result: await orca.synthesize(event.data.text, event.data.synthesizeParams),
        });
      } catch (e: any) {
        if (e instanceof OrcaError) {
          self.postMessage({
            command: 'error',
            status: e.status,
            shortMessage: e.shortMessage,
            messageStack: e.messageStack,
          });
        } else {
          self.postMessage({
            command: 'error',
            status: PvStatus.INVALID_STATE,
            shortMessage: 'Orca synthesize error',
          });
        }
      }
      break;
    case 'release':
      if (orca !== null) {
        await orca.release();
        orca = null;
        close();
      }
      self.postMessage({
        command: 'ok',
      });
      break;
    case 'streamOpen':
      if (orca === null) {
        self.postMessage({
          command: 'error',
          status: PvStatus.INVALID_STATE,
          shortMessage: 'Orca not initialized',
        });
        return;
      }
      try {
        orcaStream = await orca.streamOpen(
          streamSynthesizeCallback,
          event.data.synthesizeParams,
          streamSynthesizeErrorCallback,
        );
        self.postMessage({
          command: 'ok',
        });
      } catch (e: any) {
        if (e instanceof OrcaError) {
          self.postMessage({
            command: 'error',
            status: e.status,
            shortMessage: e.shortMessage,
            messageStack: e.messageStack,
          });
        } else {
          self.postMessage({
            command: 'error',
            status: PvStatus.INVALID_STATE,
            shortMessage: 'Orca stream open error',
          });
        }
      }
      break;
    case 'streamSynthesize':
      if (orca === null) {
        self.postMessage({
          command: 'error',
          status: PvStatus.INVALID_STATE,
          shortMessage: 'Orca not initialized',
        });
        return;
      }
      if (orcaStream === null) {
        self.postMessage({
          command: 'error',
          status: PvStatus.INVALID_STATE,
          shortMessage: 'Orca stream not initialized',
        });
        return;
      }
      try {
        await orcaStream.synthesize(event.data.text);
      } catch (e: any) {
        if (e instanceof OrcaError) {
          self.postMessage({
            command: 'error',
            status: e.status,
            shortMessage: e.shortMessage,
            messageStack: e.messageStack,
          });
        } else {
          self.postMessage({
            command: 'error',
            status: PvStatus.INVALID_STATE,
            shortMessage: 'Orca synthesize error',
          });
        }
      }
      break;
    case 'streamFlush':
      if (orca === null) {
        self.postMessage({
          command: 'error',
          status: PvStatus.INVALID_STATE,
          shortMessage: 'Orca not initialized',
        });
        return;
      }
      if (orcaStream === null) {
        self.postMessage({
          command: 'error',
          status: PvStatus.INVALID_STATE,
          shortMessage: 'Orca stream not initialized',
        });
        return;
      }
      await orcaStream.flush();
      break;
    case 'streamClose':
      if (orca === null) {
        self.postMessage({
          command: 'error',
          status: PvStatus.INVALID_STATE,
          shortMessage: 'Orca not initialized',
        });
        return;
      }
      if (orcaStream === null) {
        self.postMessage({
          command: 'error',
          status: PvStatus.INVALID_STATE,
          shortMessage: 'Orca stream not initialized',
        });
        return;
      }
      await orcaStream.close();
      break;
    default:
      self.postMessage({
        command: 'failed',
        status: PvStatus.RUNTIME_ERROR,
        // @ts-ignore
        shortMessage: `Unrecognized command: ${event.data.command}`,
      });
  }
};
