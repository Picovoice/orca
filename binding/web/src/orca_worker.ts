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
  OrcaSynthesizeParams,
  OrcaSynthesizeResult,
  OrcaStreamOpenOptions,
  OrcaStreamSynthesizeResult,
  OrcaWorkerInitResponse,
  OrcaWorkerSynthesizeResponse,
  OrcaWorkerReleaseResponse,
  OrcaWorkerStreamOpenResponse,
  OrcaWorkerStreamSynthesizeResponse,
  OrcaWorkerStreamFlushResponse,
  PvStatus,
} from './types';
import { loadModel } from '@picovoice/web-utils';

import { pvStatusToException } from './orca_errors';

import * as OrcaErrors from './orca_errors';

export class OrcaWorker {
  private readonly _worker: Worker;
  private readonly _version: string;
  private readonly _sampleRate: number;
  private readonly _maxCharacterLimit: number;
  private readonly _validCharacters: string[];

  private static _wasm: string;
  private static _wasmSimd: string;
  private static _sdk: string = 'web';

  private readonly _OrcaStream: any;

  private constructor(
    worker: Worker,
    version: string,
    sampleRate: number,
    maxCharacterLimit: number,
    validCharacters: string[],
  ) {
    this._worker = worker;
    this._version = version;
    this._sampleRate = sampleRate;
    this._maxCharacterLimit = maxCharacterLimit;
    this._validCharacters = validCharacters;

    this._OrcaStream = class OrcaStream {
      private readonly _worker: Worker;
      private readonly _synthesizeCallback: (synthesizeResult: OrcaStreamSynthesizeResult) => void;
      private readonly _synthesizeErrorCallback?: (error: OrcaErrors.OrcaError) => void;

      private constructor(
        orcaWorker: Worker,
        synthesizeCallback: (synthesizeResult: OrcaStreamSynthesizeResult) => void,
        synthesizeErrorCallback?: (error: OrcaErrors.OrcaError) => void,
      ) {
        this._worker = orcaWorker;
        this._synthesizeCallback = synthesizeCallback;
        this._synthesizeErrorCallback = synthesizeErrorCallback;
      }

      /**
       * Generates audio from text in a worker.
       * The speech result will be supplied with the callback provided when initializing the worker either
       * by 'fromBase64' or 'fromPublicDirectory'.
       * Can also send a message directly using 'this.worker.postMessage({command: "synthesize", text: "..."})'.
       *
       * @param text A string of text.
       */
      public async synthesize(text: string): Promise<void> {
        const returnPromise: Promise<void> = new Promise(
          (resolve, reject) => {
            this._worker.onmessage = (
              event: MessageEvent<OrcaWorkerStreamSynthesizeResponse>,
            ): void => {
              switch (event.data.command) {
                case 'ok':
                  this._synthesizeCallback(event.data.result);
                  resolve();
                  break;
                case 'failed':
                case 'error':
                  // eslint-disable-next-line no-case-declarations
                  const error = pvStatusToException(
                    event.data.status,
                    event.data.shortMessage,
                    event.data.messageStack,
                  );
                  if (this._synthesizeErrorCallback) {
                    this._synthesizeErrorCallback(error);
                  } else {
                    // eslint-disable-next-line no-console
                    console.error(error);
                  }
                  reject(error);
                  break;
                default:
                  // eslint-disable-next-line no-case-declarations
                  const defaultError = pvStatusToException(
                    PvStatus.RUNTIME_ERROR,
                    // @ts-ignore
                    `Unrecognized command: ${event.data.command}`,
                  );
                  if (this._synthesizeErrorCallback) {
                    this._synthesizeErrorCallback(defaultError);
                  } else {
                    // eslint-disable-next-line no-console
                    console.error(defaultError);
                  }
                  reject(defaultError);
              }
            };
          },
        );

        this._worker.postMessage(
          {
            command: 'streamSynthesize',
            text: text,
          },
        );

        return returnPromise;
      }

      public async flush(): Promise<void> {
        const returnPromise: Promise<void> = new Promise(
          (resolve, reject) => {
            this._worker.onmessage = (
              event: MessageEvent<OrcaWorkerStreamFlushResponse>,
            ): void => {
              switch (event.data.command) {
                case 'ok':
                  this._synthesizeCallback(event.data.result);
                  resolve();
                  break;
                case 'failed':
                case 'error':
                  // eslint-disable-next-line no-case-declarations
                  const error = pvStatusToException(
                    event.data.status,
                    event.data.shortMessage,
                    event.data.messageStack,
                  );
                  if (this._synthesizeErrorCallback) {
                    this._synthesizeErrorCallback(error);
                  } else {
                    // eslint-disable-next-line no-console
                    console.error(error);
                  }
                  reject(error);
                  break;
                default:
                  // eslint-disable-next-line no-case-declarations
                  const defaultError = pvStatusToException(
                    PvStatus.RUNTIME_ERROR,
                    // @ts-ignore
                    `Unrecognized command: ${event.data.command}`,
                  );
                  if (this._synthesizeErrorCallback) {
                    this._synthesizeErrorCallback(defaultError);
                  } else {
                    // eslint-disable-next-line no-console
                    console.error(defaultError);
                  }
                  reject(defaultError);
              }
            };
          },
        );

        this._worker.postMessage({
          command: 'streamFlush',
        });

        return returnPromise;
      }

      public close(): void {
        this._worker.postMessage({
          command: 'streamClose',
        });
      }
    };
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
   * Get valid characters.
   */
  get validCharacters(): string[] {
    return this._validCharacters;
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
   * @param model Orca model options.
   * @param model.base64 The model in base64 string to initialize Orca.
   * @param model.publicPath The model path relative to the public directory.
   * @param model.customWritePath Custom path to save the model in storage.
   * Set to a different name to use multiple models across `orca` instances.
   * @param model.forceWrite Flag to overwrite the model in storage even if it exists.
   * @param model.version Version of the model file. Increment to update the model file in storage.
   *
   * @returns An instance of OrcaWorker.
   */
  public static async create(
    accessKey: string,
    model: OrcaModel,
  ): Promise<OrcaWorker> {
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
              resolve(
                new OrcaWorker(
                  worker,
                  event.data.version,
                  event.data.sampleRate,
                  event.data.maxCharacterLimit,
                  event.data.validCharacters,
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
      wasm: this._wasm,
      wasmSimd: this._wasmSimd,
      sdk: this._sdk,
    });

    return returnPromise;
  }

  /**
   * Generates audio from text in a worker.
   * The speech result will be supplied with the callback provided when initializing the worker either
   * by 'fromBase64' or 'fromPublicDirectory'.
   * Can also send a message directly using 'this.worker.postMessage({command: "synthesize", text: "..."})'.
   *
   * @param text A string of text with properties described above.
   * @param synthesizeParams Optional configuration arguments.
   * @param synthesizeParams.speechRate Configure the rate of speech of the synthesized speech.
   * @param synthesizeParams.randomState Configure the random seed for the synthesized speech.
   *
   * @return A synthesize result containing the raw pcm and alignments data.
   */
  public async synthesize(
    text: string,
    synthesizeParams: OrcaSynthesizeParams = {},
  ): Promise<OrcaSynthesizeResult> {
    const returnPromise: Promise<OrcaSynthesizeResult> = new Promise(
      (resolve, reject) => {
        this._worker.onmessage = (
          event: MessageEvent<OrcaWorkerSynthesizeResponse>,
        ): void => {
          switch (event.data.command) {
            case 'ok':
              resolve(event.data.result);
              break;
            case 'failed':
            case 'error':
              reject(
                pvStatusToException(
                  event.data.status,
                  event.data.shortMessage,
                  event.data.messageStack,
                ),
              );
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

    this._worker.postMessage(
      {
        command: 'synthesize',
        text: text,
        synthesizeParams: synthesizeParams,
      },
    );

    return returnPromise;
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
   * Opens a new OrcaStream object in a worker.
   *
   * @param synthesizeCallback User-defined callback to run after receiving pcm result.
   * @param options Optional configuration arguments.
   * @param options.synthesizeParams.speechRate Configure the rate of speech of the synthesized speech.
   * @param options.synthesizeParams.randomState Configure the random seed for the synthesized speech.
   * @param options.synthesizeErrorCallback User-defined error callback to run if error.
   */
  public async streamOpen(
    synthesizeCallback: (synthesizeResult: OrcaStreamSynthesizeResult) => void,
    options: OrcaStreamOpenOptions,
  ): Promise<any> {
    const returnPromise: Promise<any> = new Promise(
      (resolve, reject) => {
        this._worker.onmessage = (
          event: MessageEvent<OrcaWorkerStreamOpenResponse>,
        ): void => {
          switch (event.data.command) {
            case 'ok':
              resolve(new this._OrcaStream(
                this._worker,
                synthesizeCallback,
                options.synthesizeErrorCallback,
              ));
              break;
            case 'failed':
            case 'error':
              reject(
                pvStatusToException(
                  event.data.status,
                  event.data.shortMessage,
                  event.data.messageStack,
                ),
              );
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

    this._worker.postMessage(
      {
        command: 'streamOpen',
        synthesizeParams: options.synthesizeParams,
      },
    );

    return returnPromise;
  }

  /**
   * Terminates the active worker. Stops all requests being handled by worker.
   */
  public terminate(): void {
    this._worker.terminate();
  }
}
