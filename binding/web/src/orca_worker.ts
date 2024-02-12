/*
  Copyright 2024 Picovoice Inc.

  You may not use this file except in compliance with the license. A copy of the license is located in the "LICENSE"
  file accompanying this source.

  Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
  an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
  specific language governing permissions and limitations under the License.
*/

import PvWorker from 'web-worker:./orca_worker_handler.ts';

import {
  OrcaModel,
  OrcaOptions,
  OrcaSpeech,
  OrcaWorkerInitResponse,
  OrcaWorkerSynthesizeResponse,
  OrcaWorkerReleaseResponse,
  PvStatus,
} from './types';
import { loadModel } from '@picovoice/web-utils';

import { pvStatusToException } from './orca_errors';

export class OrcaWorker {
  private readonly _worker: Worker;
  private readonly _version: string;
  private readonly _sampleRate: number;
  private readonly _maxCharacterLimit: number;
  private readonly _validPunctuationSymbols: string[];

  private static _wasm: string;
  private static _wasmSimd: string;
  private static _sdk: string = 'web';

  private constructor(
    worker: Worker,
    version: string,
    sampleRate: number,
    maxCharacterLimit: number,
    validPunctuationSymbols: string[],
  ) {
    this._worker = worker;
    this._version = version;
    this._sampleRate = sampleRate;
    this._maxCharacterLimit = maxCharacterLimit;
    this._validPunctuationSymbols = validPunctuationSymbols;
  }

  /**
   * Get Orca engine version.
   */
  get version(): string {
    return this._version;
  }

  /**
   * Get sample rate.
   */
  get sampleRate(): number {
    return this._sampleRate;
  }

  /**
   * Get maximum character limit.
   */
  get maxCharacterLimit(): number {
    return this._maxCharacterLimit;
  }

  /**
   * Get valid punctuation symbols.
   */
  get validPunctuationSymbols(): string[] {
    return this._validPunctuationSymbols;
  }

  /**
   * Get Orca worker instance.
   */
  get worker(): Worker {
    return this._worker;
  }

  /**
   * Set base64 wasm file.
   * @param wasm Base64'd wasm file to use to initialize wasm.
   */
  public static setWasm(wasm: string): void {
    if (this._wasm === undefined) {
      this._wasm = wasm;
    }
  }

  /**
   * Set base64 wasm file with SIMD feature.
   * @param wasmSimd Base64'd wasm file to use to initialize wasm.
   */
  public static setWasmSimd(wasmSimd: string): void {
    if (this._wasmSimd === undefined) {
      this._wasmSimd = wasmSimd;
    }
  }

  public static setSdk(sdk: string): void {
    OrcaWorker._sdk = sdk;
  }

  /**
   * Creates a worker instance of the Picovoice Orca Text-to-Speech engine.
   * Behind the scenes, it requires the WebAssembly code to load and initialize before
   * it can create an instance.
   *
   * @param accessKey AccessKey obtained from Picovoice Console (https://console.picovoice.ai/)
   * @param speechCallback User-defined callback to run after receiving speech result.
   * @param model Orca model options.
   * @param model.base64 The model in base64 string to initialize Orca.
   * @param model.publicPath The model path relative to the public directory.
   * @param model.customWritePath Custom path to save the model in storage.
   * Set to a different name to use multiple models across `orca` instances.
   * @param model.forceWrite Flag to overwrite the model in storage even if it exists.
   * @param model.version Version of the model file. Increment to update the model file in storage.
   * @param options Optional configuration arguments.
   * @param options.synthesizeErrorCallback User-defined callback invoked if any error happens
   * while synthesizing speech. Its only input argument is the error message.
   *
   * @returns An instance of OrcaWorker.
   */
  public static async create(
    accessKey: string,
    speechCallback: (orcaSpeech: OrcaSpeech) => void,
    model: OrcaModel,
    options: OrcaOptions = {},
  ): Promise<OrcaWorker> {
    const { synthesizeErrorCallback, ...workerOptions } = options;

    const customWritePath = model.customWritePath
      ? model.customWritePath
      : 'orca_model';
    const modelPath = await loadModel({ ...model, customWritePath });

    const worker = new PvWorker();
    const returnPromise: Promise<OrcaWorker> = new Promise(
      (resolve, reject) => {
        // @ts-ignore - block from GC
        this.worker = worker;
        worker.onmessage = (
          event: MessageEvent<OrcaWorkerInitResponse>,
        ): void => {
          switch (event.data.command) {
            case 'ok':
              worker.onmessage = (
                ev: MessageEvent<OrcaWorkerSynthesizeResponse>,
              ): void => {
                switch (ev.data.command) {
                  case 'ok':
                    speechCallback(ev.data.orcaSpeech);
                    break;
                  case 'failed':
                  case 'error':
                    // eslint-disable-next-line no-case-declarations
                    const error = pvStatusToException(
                      ev.data.status,
                      ev.data.shortMessage,
                      ev.data.messageStack,
                    );
                    if (synthesizeErrorCallback) {
                      synthesizeErrorCallback(error);
                    } else {
                      // eslint-disable-next-line no-console
                      console.error(error);
                    }
                    break;
                  default:
                    // @ts-ignore
                    synthesizeErrorCallback(
                      pvStatusToException(
                        PvStatus.RUNTIME_ERROR,
                        `Unrecognized command: ${event.data.command}`,
                      ),
                    );
                }
              };
              resolve(
                new OrcaWorker(
                  worker,
                  event.data.version,
                  event.data.sampleRate,
                  event.data.maxCharacterLimit,
                  event.data.validPunctuationSymbols,
                ),
              );
              break;
            case 'failed':
            case 'error':
              reject(pvStatusToException(
                event.data.status,
                event.data.shortMessage,
                event.data.messageStack,
              ));
              break;
            default:
              reject(
                pvStatusToException(
                  PvStatus.RUNTIME_ERROR,
                  // @ts-ignore
                  `Unrecognized command: ${event.data.command}`,
                ),
              );
          }
        };
      },
    );

    worker.postMessage({
      command: 'init',
      accessKey: accessKey,
      modelPath: modelPath,
      options: workerOptions,
      wasm: this._wasm,
      wasmSimd: this._wasmSimd,
      sdk: this._sdk,
    });

    return returnPromise;
  }

  /**
   * Processes text in a worker.
   * The speech result will be supplied with the callback provided when initializing the worker either
   * by 'fromBase64' or 'fromPublicDirectory'.
   * Can also send a message directly using 'this.worker.postMessage({command: "process", pcm: [...]})'.
   *
   * @param text A string of text.
   * @param speechRate Optional rate of speech desired for the output audio.
   */
  public synthesize(text: string, speechRate: number = 1.0): void {
    this._worker.postMessage({
      command: 'synthesize',
      text: text,
      speechRate: speechRate,
    });
  }

  /**
   * Releases resources acquired by WebAssembly module.
   */
  public release(): Promise<void> {
    const returnPromise: Promise<void> = new Promise((resolve, reject) => {
      this._worker.onmessage = (
        event: MessageEvent<OrcaWorkerReleaseResponse>,
      ): void => {
        switch (event.data.command) {
          case 'ok':
            resolve();
            break;
          case 'failed':
          case 'error':
            reject(pvStatusToException(
              event.data.status,
              event.data.shortMessage,
              event.data.messageStack,
            ));
            break;
          default:
            reject(
              pvStatusToException(
                PvStatus.RUNTIME_ERROR,
                // @ts-ignore
                `Unrecognized command: ${event.data.command}`,
              ),
            );
        }
      };
    });

    this._worker.postMessage({
      command: 'release',
    });

    return returnPromise;
  }

  /**
   * Terminates the active worker. Stops all requests being handled by worker.
   */
  public terminate(): void {
    this._worker.terminate();
  }
}
