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
  OrcaStreamSynthesizeResult,
  OrcaWorkerInitResponse,
  OrcaWorkerSynthesizeResponse,
  OrcaWorkerReleaseResponse,
  OrcaWorkerStreamOpenResponse,
  OrcaWorkerStreamSynthesizeResponse,
  OrcaWorkerStreamFlushResponse,
  OrcaWorkerStreamCloseResponse,
  PvStatus,
} from './types';
import { loadModel } from '@picovoice/web-utils';

import { pvStatusToException } from './orca_errors';

class StreamWorker {
  readonly _worker: Worker;

  constructor(orcaWorker: Worker) {
    this._worker = orcaWorker;
  }

  /**
   * Adds a chunk of text to the Stream object in a worker and generates audio if enough text has been added.
   * This function is expected to be called multiple times with consecutive chunks of text from a text stream.
   * The incoming text is buffered as it arrives until there is enough context to convert a chunk of the
   * buffered text into audio. The caller needs to use `OrcaStream.flush()` to generate the audio chunk
   * for the remaining text that has not yet been synthesized.
   *
   * @param text A chunk of text from a text input stream, comprised of valid characters.
   *             Valid characters can be retrieved by calling `validCharacters`.
   *             Custom pronunciations can be embedded in the text via the syntax `{word|pronunciation}`.
   *             They need to be added in a single call to this function.
   *             The pronunciation is expressed in ARPAbet format, e.g.: `I {liv|L IH V} in {Sevilla|S EH V IY Y AH}`.
   * @return The generated audio as a sequence of 16-bit linearly-encoded integers, `null` if no
   *         audio chunk has been produced.
   */
  public synthesize(text: string): Promise<OrcaStreamSynthesizeResult> {
    const returnPromise: Promise<OrcaStreamSynthesizeResult> = new Promise(
      (resolve, reject) => {
        this._worker.onmessage = (
          event: MessageEvent<OrcaWorkerStreamSynthesizeResponse>,
        ): void => {
          switch (event.data.command) {
            case 'ok':
              resolve(event.data.result);
              break;
            case 'failed':
            case 'error':
              // eslint-disable-next-line no-case-declarations
              reject(pvStatusToException(
                event.data.status,
                event.data.shortMessage,
                event.data.messageStack,
              ));
              break;
            default:
              reject(pvStatusToException(
                PvStatus.RUNTIME_ERROR,
                // @ts-ignore
                `Unrecognized command: ${event.data.command}`,
              ));
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

  /**
   * Generates audio for all the buffered text that was added to the OrcaStream object
   * via `OrcaStream.synthesize()`.
   *
   * @return The generated audio as a sequence of 16-bit linearly-encoded integers, `null` if no
   *         audio chunk has been produced.
   */
  public flush(): Promise<OrcaStreamSynthesizeResult> {
    const returnPromise: Promise<OrcaStreamSynthesizeResult> = new Promise(
      (resolve, reject) => {
        this._worker.onmessage = (
          event: MessageEvent<OrcaWorkerStreamFlushResponse>,
        ): void => {
          switch (event.data.command) {
            case 'ok':
              resolve(event.data.result);
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
              reject(pvStatusToException(
                PvStatus.RUNTIME_ERROR,
                // @ts-ignore
                `Unrecognized command: ${event.data.command}`,
              ));
          }
        };
      },
    );

    this._worker.postMessage({
      command: 'streamFlush',
    });

    return returnPromise;
  }

  /**
   * Releases the resources acquired by the OrcaStream object.
   */
  public close(): Promise<void> {
    const returnPromise: Promise<void> = new Promise((resolve, reject) => {
      this._worker.onmessage = (
        event: MessageEvent<OrcaWorkerStreamCloseResponse>,
      ): void => {
        switch (event.data.command) {
          case 'ok':
            resolve();
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
    });

    this._worker.postMessage({
      command: 'streamClose',
    });

    return returnPromise;
  }
}

export type OrcaStreamWorker = StreamWorker

export class OrcaWorker {
  private readonly _worker: Worker;
  private readonly _version: string;
  private readonly _sampleRate: number;
  private readonly _maxCharacterLimit: number;
  private readonly _validCharacters: string[];

  private static _wasm: string;
  private static _wasmSimd: string;
  private static _sdk: string = 'web';

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
   * Generates audio from text in a worker. The returned audio contains the speech representation of the text.
   * The maximum number of characters per call to `.synthesize()` is `.maxCharacterLimit`.
   * Allowed characters are lower-case and upper-case letters and punctuation marks that can be retrieved with `.validCharacters`.
   * Custom pronunciations can be embedded in the text via the syntax `{word|pronunciation}`.
   * The pronunciation is expressed in ARPAbet format, e.g.: "I {live|L IH V} in {Sevilla|S EH V IY Y AH}".
   *
   * @param text A string of text with properties described above.
   * @param synthesizeParams Optional configuration arguments.
   * @param synthesizeParams.speechRate Configure the rate of speech of the synthesized speech.
   * @param synthesizeParams.randomState Configure the random seed for the synthesized speech.
   *
   * @return A result object containing the generated audio as a sequence of 16-bit linearly-encoded integers
   *         and a sequence of OrcaAlignment objects representing the word alignments.
   */
  public synthesize(
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
   * @param synthesizeParams Optional configuration arguments.
   * @param synthesizeParams.speechRate Configure the rate of speech of the synthesized speech.
   * @param synthesizeParams.randomState Configure the random seed for the synthesized speech.
   */
  public streamOpen(synthesizeParams: OrcaSynthesizeParams = {}): Promise<OrcaStreamWorker> {
    const returnPromise: Promise<OrcaStreamWorker> = new Promise(
      (resolve, reject) => {
        this._worker.onmessage = (
          event: MessageEvent<OrcaWorkerStreamOpenResponse>,
        ): void => {
          switch (event.data.command) {
            case 'ok':
              resolve(new StreamWorker(this._worker));
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
        synthesizeParams: synthesizeParams,
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
