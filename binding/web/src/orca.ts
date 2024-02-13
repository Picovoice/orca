/*
  Copyright 2024 Picovoice Inc.

  You may not use this file except in compliance with the license. A copy of the license is located in the "LICENSE"
  file accompanying this source.

  Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
  an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
  specific language governing permissions and limitations under the License.
*/

/* eslint camelcase: 0 */

import { Mutex } from 'async-mutex';

import {
  aligned_alloc_type,
  pv_free_type,
  buildWasm,
  arrayBufferToStringAtIndex,
  isAccessKeyValid,
  loadModel,
  PvError,
} from '@picovoice/web-utils';

import { simd } from 'wasm-feature-detect';

import { OrcaModel, OrcaOptions, OrcaSpeech, PvStatus } from './types';

import * as OrcaErrors from './orca_errors';
import { pvStatusToException } from './orca_errors';

/**
 * WebAssembly function types
 */
type pv_orca_init_type = (accessKey: number, modelPath: number, object: number) => Promise<number>;
type pv_orca_delete_type = (object: number) => Promise<void>;
type pv_orca_valid_characters_type = (object: number, numCharacters: number, validCharacters: number) => Promise<number>;
type pv_orca_valid_characters_delete_type = (validCharacters: number) => Promise<void>;
type pv_orca_sample_rate_type = (object: number, sampleRate: number) => Promise<number>;
type pv_orca_max_character_limit_type = () => Promise<number>;
type pv_orca_synthesize_params_init_type = (object: number) => Promise<number>;
type pv_orca_synthesize_params_delete_type = (object: number) => Promise<void>;
type pv_orca_synthesize_params_set_speech_rate_type = (object: number, speechRate: number) => Promise<number>;
type pv_orca_synthesize_type = (object: number, text: number, synthesizeParams: number, numSamples: number, pcm: number) => Promise<number>;
type pv_orca_delete_pcm_type = (object: number) => Promise<void>;
type pv_orca_version_type = () => Promise<number>;
type pv_status_to_string_type = (status: number) => Promise<number>
type pv_set_sdk_type = (sdk: number) => Promise<void>;
type pv_get_error_stack_type = (messageStack: number, messageStackDepth: number) => Promise<number>;
type pv_free_error_stack_type = (messageStack: number) => Promise<void>;

/**
 * JavaScript/WebAssembly Binding for Orca
 */

type OrcaWasmOutput = {
  alignedAlloc: aligned_alloc_type;
  memory: WebAssembly.Memory;
  pvFree: pv_free_type;

  version: string;
  sampleRate: number;
  maxCharacterLimit: number;
  validCharacters: string[];

  objectAddress: number;
  inputBufferAddress: number;
  synthesizeParamsAddressAddress: number;
  speechRateAddress: number;
  messageStackAddressAddressAddress: number;
  messageStackDepthAddress: number;

  pvOrcaDelete: pv_orca_delete_type;
  pvOrcaSynthesize: pv_orca_synthesize_type;
  pvOrcaSynthesizeParamsInit: pv_orca_synthesize_params_init_type;
  pvOrcaSynthesizeParamsDelete: pv_orca_synthesize_params_delete_type;
  pvOrcaSynthesizeParamsSetSpeechRate: pv_orca_synthesize_params_set_speech_rate_type
  pvOrcaDeletePcm: pv_orca_delete_pcm_type;
  pvStatusToString: pv_status_to_string_type;
  pvGetErrorStack: pv_get_error_stack_type;
  pvFreeErrorStack: pv_free_error_stack_type;
};

const PV_STATUS_SUCCESS = 10000;

export class Orca {
  private readonly _pvOrcaDelete: pv_orca_delete_type;
  private readonly _pvOrcaSynthesize: pv_orca_synthesize_type;
  private readonly _pvOrcaSynthesizeParamsInit: pv_orca_synthesize_params_init_type;
  private readonly _pvOrcaSynthesizeParamsDelete: pv_orca_synthesize_params_delete_type;
  private readonly _pvOrcaSynthesizeParamsSetSpeechRate: pv_orca_synthesize_params_set_speech_rate_type;
  private readonly _pvOrcaDeletePcm: pv_orca_delete_pcm_type;
  private readonly _pvGetErrorStack: pv_get_error_stack_type;
  private readonly _pvFreeErrorStack: pv_free_error_stack_type;

  private _wasmMemory: WebAssembly.Memory | undefined;

  private readonly _pvFree: pv_free_type;
  private readonly _synthesizeMutex: Mutex;

  private readonly _objectAddress: number;
  private readonly _alignedAlloc: CallableFunction;
  private readonly _inputBufferAddress: number;
  private readonly _messageStackAddressAddressAddress: number;
  private readonly _messageStackDepthAddress: number;

  private static _version: string;
  private static _sampleRate: number;
  private static _maxCharacterLimit: number;
  private static _validCharacters: string[];
  private static _wasm: string;
  private static _wasmSimd: string;
  private static _sdk: string = 'web';

  private static _orcaMutex = new Mutex();

  private readonly _speechCallback: (orcaSpeech: OrcaSpeech) => void;
  private readonly _synthesizeErrorCallback?: (error: OrcaErrors.OrcaError) => void;

  private constructor(
    handleWasm: OrcaWasmOutput,
    speechCallback: (orcaSpeech: OrcaSpeech) => void,
    synthesizeErrorCallback?: (error: OrcaErrors.OrcaError) => void,
  ) {
    Orca._version = handleWasm.version;
    Orca._sampleRate = handleWasm.sampleRate;
    Orca._maxCharacterLimit = handleWasm.maxCharacterLimit;
    Orca._validCharacters = handleWasm.validCharacters;

    this._pvOrcaDelete = handleWasm.pvOrcaDelete;
    this._pvOrcaSynthesize = handleWasm.pvOrcaSynthesize;
    this._pvOrcaSynthesizeParamsInit = handleWasm.pvOrcaSynthesizeParamsInit;
    this._pvOrcaSynthesizeParamsDelete = handleWasm.pvOrcaSynthesizeParamsDelete;
    this._pvOrcaSynthesizeParamsSetSpeechRate = handleWasm.pvOrcaSynthesizeParamsSetSpeechRate;
    this._pvOrcaDeletePcm = handleWasm.pvOrcaDeletePcm;
    this._pvGetErrorStack = handleWasm.pvGetErrorStack;
    this._pvFreeErrorStack = handleWasm.pvFreeErrorStack;

    this._alignedAlloc = handleWasm.alignedAlloc;
    this._wasmMemory = handleWasm.memory;
    this._pvFree = handleWasm.pvFree;
    this._objectAddress = handleWasm.objectAddress;
    this._inputBufferAddress = handleWasm.inputBufferAddress;
    this._messageStackAddressAddressAddress = handleWasm.messageStackAddressAddressAddress;
    this._messageStackDepthAddress = handleWasm.messageStackDepthAddress;

    this._synthesizeMutex = new Mutex();

    this._speechCallback = speechCallback;
    this._synthesizeErrorCallback = synthesizeErrorCallback;
  }

  /**
   * Get Orca engine version.
   */
  get version(): string {
    return Orca._version;
  }

  /**
   * Get sample rate.
   */
  get sampleRate(): number {
    return Orca._sampleRate;
  }

  /**
   * Get maximum character limit.
   */
  get maxCharacterLimit(): number {
    return Orca._maxCharacterLimit;
  }

  /**
   * Get valid characters.
   */
  get validCharacters(): string[] {
    return Orca._validCharacters;
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
    Orca._sdk = sdk;
  }

  /**
   * Creates an instance of the Picovoice Orca Text-to-Speech engine.
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
   * @returns An instance of the Orca engine.
   */
  public static async create(
    accessKey: string,
    speechCallback: (orcaSpeech: OrcaSpeech) => void,
    model: OrcaModel,
    options: OrcaOptions = {},
  ): Promise<Orca> {
    const customWritePath = (model.customWritePath) ? model.customWritePath : 'orca_model';
    const modelPath = await loadModel({ ...model, customWritePath });

    return Orca._init(
      accessKey,
      speechCallback,
      modelPath,
      options,
    );
  }

  public static async _init(
    accessKey: string,
    speechCallback: (orcaSpeech: OrcaSpeech) => void,
    modelPath: string,
    options: OrcaOptions = {},
  ): Promise<Orca> {
    const { synthesizeErrorCallback } = options;

    if (!isAccessKeyValid(accessKey)) {
      throw new OrcaErrors.OrcaInvalidArgumentError('Invalid AccessKey');
    }

    return new Promise<Orca>((resolve, reject) => {
      Orca._orcaMutex
        .runExclusive(async () => {
          const isSimd = await simd();
          const wasmOutput = await Orca.initWasm(accessKey.trim(), (isSimd) ? this._wasmSimd : this._wasm, modelPath);
          return new Orca(wasmOutput, speechCallback, synthesizeErrorCallback);
        })
        .then((result: Orca) => {
          resolve(result);
        })
        .catch((error: any) => {
          reject(error);
        });
    });
  }

  /**
   * Synthesizes a string of text.
   *
   * @param text A frame of audio with properties described above.
   * The maximum number of characters per call to `.synthesize()` is `.maxCharacterLimit`.
   * Allowed characters are lower-case and upper-case letters and punctuation marks that can be retrieved with `.validCharacters`.
   * Custom pronunciations can be embedded in the text via the syntax `{word|pronunciation}`.
   * The pronunciation is expressed in ARPAbet format, e.g.: "I {live|L IH V} in {Sevilla|S EH V IY Y AH}".
   * @param speechRate Optional argument to configure the rate of speech of the synthesized audio.
   */
  public async synthesize(text: string, speechRate: number = 1.0): Promise<void> {
    if (typeof text !== 'string') {
      const error = new OrcaErrors.OrcaInvalidArgumentError('The argument \'text\' must be provided as a string');
      if (this._synthesizeErrorCallback) {
        this._synthesizeErrorCallback(error);
      } else {
        // eslint-disable-next-line no-console
        console.error(error);
      }
    }

    this._synthesizeMutex
      .runExclusive(async () => {
        if (this._wasmMemory === undefined) {
          throw new OrcaErrors.OrcaInvalidStateError('Attempted to call Orca synthesize after release.');
        }

        const memoryBufferView = new DataView(this._wasmMemory.buffer);

        const memoryBufferText = new Uint8Array(this._wasmMemory.buffer);
        const encodedText = new TextEncoder().encode(text);
        const textAddress = await this._alignedAlloc(
          Uint8Array.BYTES_PER_ELEMENT,
          (encodedText.length + 1) * Uint8Array.BYTES_PER_ELEMENT,
        );
        if (textAddress === 0) {
          throw new OrcaErrors.OrcaOutOfMemoryError(
            'malloc failed: Cannot allocate memory',
          );
        }
        memoryBufferText.set(encodedText, textAddress);
        memoryBufferText[textAddress + encodedText.length] = 0;

        const synthesizeParamsAddressAddress = await this._alignedAlloc(
          Int32Array.BYTES_PER_ELEMENT,
          Int32Array.BYTES_PER_ELEMENT,
        );
        if (synthesizeParamsAddressAddress === 0) {
          throw new OrcaErrors.OrcaOutOfMemoryError('malloc failed: Cannot allocate memory');
        }

        const initStatus = await this._pvOrcaSynthesizeParamsInit(synthesizeParamsAddressAddress);
        if (initStatus !== PV_STATUS_SUCCESS) {
          const error = pvStatusToException(initStatus, 'Unable to create Orca synthesize params object');
          if (this._synthesizeErrorCallback) {
            this._synthesizeErrorCallback(error);
          } else {
            // eslint-disable-next-line no-console
            console.error(error);
          }
          return;
        }

        const synthesizeParamsAddress = memoryBufferView.getInt32(synthesizeParamsAddressAddress, true);
        await this._pvFree(synthesizeParamsAddressAddress);
        const setSpeechRateStatus = await this._pvOrcaSynthesizeParamsSetSpeechRate(synthesizeParamsAddress, speechRate);
        if (setSpeechRateStatus !== PV_STATUS_SUCCESS) {
          const error = pvStatusToException(setSpeechRateStatus, 'Unable to set Orca speech rate');
          if (this._synthesizeErrorCallback) {
            this._synthesizeErrorCallback(error);
          } else {
            // eslint-disable-next-line no-console
            console.error(error);
          }
          return;
        }

        const numSamplesAddress = await this._alignedAlloc(
          Int32Array.BYTES_PER_ELEMENT,
          Int32Array.BYTES_PER_ELEMENT,
        );
        if (numSamplesAddress === 0) {
          throw new OrcaErrors.OrcaOutOfMemoryError('malloc failed: Cannot allocate memory');
        }

        const speechAddressAddress = await this._alignedAlloc(
          Int32Array.BYTES_PER_ELEMENT,
          Int32Array.BYTES_PER_ELEMENT,
        );
        if (speechAddressAddress === 0) {
          throw new OrcaErrors.OrcaOutOfMemoryError('malloc failed: Cannot allocate memory');
        }

        const synthesizeStatus = await this._pvOrcaSynthesize(
          this._objectAddress,
          textAddress,
          synthesizeParamsAddress,
          numSamplesAddress,
          speechAddressAddress,
        );
        await this._pvFree(textAddress);
        await this._pvOrcaSynthesizeParamsDelete(synthesizeParamsAddress);

        const memoryBufferUint8 = new Uint8Array(this._wasmMemory.buffer);
        if (synthesizeStatus !== PV_STATUS_SUCCESS) {
          const messageStack = await Orca.getMessageStack(
            this._pvGetErrorStack,
            this._pvFreeErrorStack,
            this._messageStackAddressAddressAddress,
            this._messageStackDepthAddress,
            memoryBufferView,
            memoryBufferUint8,
          );

          const error = pvStatusToException(synthesizeStatus, 'Synthesizing failed', messageStack);
          if (this._synthesizeErrorCallback) {
            this._synthesizeErrorCallback(error);
          } else {
            // eslint-disable-next-line no-console
            console.error(error);
          }
          return;
        }

        const speechAddress = memoryBufferView.getInt32(
          speechAddressAddress,
          true,
        );
        await this._pvFree(speechAddressAddress);

        const numSamples = memoryBufferView.getInt32(
          numSamplesAddress,
          true,
        );
        await this._pvFree(numSamplesAddress);

        const outputMemoryBuffer = new Int16Array(this._wasmMemory.buffer);
        const speech = outputMemoryBuffer.slice(
          speechAddress / Int16Array.BYTES_PER_ELEMENT,
          (speechAddress / Int16Array.BYTES_PER_ELEMENT) + numSamples,
        );
        await this._pvOrcaDeletePcm(speechAddress);
        this._speechCallback({ speech });
      })
      .catch(async (error: any) => {
        if (this._synthesizeErrorCallback) {
          this._synthesizeErrorCallback(error);
        } else {
          // eslint-disable-next-line no-console
          console.error(error);
        }
      });
  }

  /**
   * Releases resources acquired by WebAssembly module.
   */
  public async release(): Promise<void> {
    await this._pvOrcaDelete(this._objectAddress);
    await this._pvFree(this._messageStackAddressAddressAddress);
    await this._pvFree(this._messageStackDepthAddress);
    await this._pvFree(this._inputBufferAddress);
    delete this._wasmMemory;
    this._wasmMemory = undefined;
  }

  async onmessage(e: MessageEvent): Promise<void> {
    switch (e.data.command) {
      case 'synthesize':
        await this.synthesize(e.data.text, e.data.speechRate);
        break;
      default:
        // eslint-disable-next-line no-console
        console.warn(`Unrecognized command: ${e.data.command}`);
    }
  }

  private static async initWasm(accessKey: string, wasmBase64: string, modelPath: string): Promise<any> {
    // A WebAssembly page has a constant size of 64KiB. -> 1MiB ~= 16 pages
    const memory = new WebAssembly.Memory({ initial: 60000 });

    const memoryBufferUint8 = new Uint8Array(memory.buffer);

    const pvError = new PvError();

    const exports = await buildWasm(memory, wasmBase64, pvError);

    const aligned_alloc = exports.aligned_alloc as aligned_alloc_type;
    const pv_free = exports.pv_free as pv_free_type;
    const pv_orca_init = exports.pv_orca_init as pv_orca_init_type;
    const pv_orca_delete = exports.pv_orca_delete as pv_orca_delete_type;
    const pv_orca_valid_characters = exports.pv_orca_valid_characters as pv_orca_valid_characters_type;
    const pv_orca_valid_characters_delete = exports.pv_orca_valid_characters_delete as pv_orca_valid_characters_delete_type;
    const pv_orca_sample_rate = exports.pv_orca_sample_rate as pv_orca_sample_rate_type;
    const pv_orca_max_character_limit = exports.pv_orca_max_character_limit as pv_orca_max_character_limit_type;
    const pv_orca_synthesize_params_init = exports.pv_orca_synthesize_params_init as pv_orca_synthesize_params_init_type;
    const pv_orca_synthesize_params_delete = exports.pv_orca_synthesize_params_delete as pv_orca_synthesize_params_delete_type;
    const pv_orca_synthesize_params_set_speech_rate = exports.pv_orca_synthesize_params_set_speech_rate as pv_orca_synthesize_params_set_speech_rate_type;
    const pv_orca_synthesize = exports.pv_orca_synthesize as pv_orca_synthesize_type;
    const pv_orca_delete_pcm = exports.pv_orca_delete_pcm as pv_orca_delete_pcm_type;
    const pv_orca_version = exports.pv_orca_version as pv_orca_version_type;
    const pv_status_to_string = exports.pv_status_to_string_type as pv_status_to_string_type;
    const pv_set_sdk = exports.pv_set_sdk as pv_set_sdk_type;
    const pv_get_error_stack = exports.pv_get_error_stack as pv_get_error_stack_type;
    const pv_free_error_stack = exports.pv_free_error_stack as pv_free_error_stack_type;

    const objectAddressAddress = await aligned_alloc(
      Int32Array.BYTES_PER_ELEMENT,
      Int32Array.BYTES_PER_ELEMENT,
    );
    if (objectAddressAddress === 0) {
      throw new OrcaErrors.OrcaOutOfMemoryError('malloc failed: Cannot allocate memory');
    }

    const accessKeyAddress = await aligned_alloc(
      Uint8Array.BYTES_PER_ELEMENT,
      (accessKey.length + 1) * Uint8Array.BYTES_PER_ELEMENT,
    );
    if (accessKeyAddress === 0) {
      throw new OrcaErrors.OrcaOutOfMemoryError('malloc failed: Cannot allocate memory');
    }
    for (let i = 0; i < accessKey.length; i++) {
      memoryBufferUint8[accessKeyAddress + i] = accessKey.charCodeAt(i);
    }
    memoryBufferUint8[accessKeyAddress + accessKey.length] = 0;

    const modelPathEncoded = new TextEncoder().encode(modelPath);
    const modelPathAddress = await aligned_alloc(
      Uint8Array.BYTES_PER_ELEMENT,
      (modelPathEncoded.length + 1) * Uint8Array.BYTES_PER_ELEMENT,
    );
    if (modelPathAddress === 0) {
      throw new OrcaErrors.OrcaOutOfMemoryError('malloc failed: Cannot allocate memory');
    }
    memoryBufferUint8.set(modelPathEncoded, modelPathAddress);
    memoryBufferUint8[modelPathAddress + modelPathEncoded.length] = 0;

    const sdkEncoded = new TextEncoder().encode(this._sdk);
    const sdkAddress = await aligned_alloc(
      Uint8Array.BYTES_PER_ELEMENT,
      (sdkEncoded.length + 1) * Uint8Array.BYTES_PER_ELEMENT,
    );
    if (!sdkAddress) {
      throw new OrcaErrors.OrcaOutOfMemoryError('malloc failed: Cannot allocate memory');
    }
    memoryBufferUint8.set(sdkEncoded, sdkAddress);
    memoryBufferUint8[sdkAddress + sdkEncoded.length] = 0;
    await pv_set_sdk(sdkAddress);
    await pv_free(sdkAddress);

    const messageStackDepthAddress = await aligned_alloc(
      Int32Array.BYTES_PER_ELEMENT,
      Int32Array.BYTES_PER_ELEMENT,
    );
    if (!messageStackDepthAddress) {
      throw new OrcaErrors.OrcaOutOfMemoryError('malloc failed: Cannot allocate memory');
    }

    const messageStackAddressAddressAddress = await aligned_alloc(
      Int32Array.BYTES_PER_ELEMENT,
      Int32Array.BYTES_PER_ELEMENT,
    );
    if (!messageStackAddressAddressAddress) {
      throw new OrcaErrors.OrcaOutOfMemoryError('malloc failed: Cannot allocate memory');
    }

    const memoryBufferView = new DataView(memory.buffer);

    const initStatus = await pv_orca_init(
      accessKeyAddress,
      modelPathAddress,
      objectAddressAddress);
    await pv_free(accessKeyAddress);
    await pv_free(modelPathAddress);
    if (initStatus !== PV_STATUS_SUCCESS) {
      const messageStack = await Orca.getMessageStack(
        pv_get_error_stack,
        pv_free_error_stack,
        messageStackAddressAddressAddress,
        messageStackDepthAddress,
        memoryBufferView,
        memoryBufferUint8,
      );

      throw pvStatusToException(initStatus, 'Initialization failed', messageStack, pvError);
    }

    const objectAddress = memoryBufferView.getInt32(objectAddressAddress, true);
    await pv_free(objectAddressAddress);

    const sampleRateAddress = await aligned_alloc(
      Int32Array.BYTES_PER_ELEMENT,
      Int32Array.BYTES_PER_ELEMENT,
    );
    if (sampleRateAddress === 0) {
      throw new OrcaErrors.OrcaOutOfMemoryError('malloc failed: Cannot allocate memory');
    }
    const sampleRateStatus = await pv_orca_sample_rate(objectAddress, sampleRateAddress);
    if (sampleRateStatus !== PV_STATUS_SUCCESS) {
      const messageStack = await Orca.getMessageStack(
        pv_get_error_stack,
        pv_free_error_stack,
        messageStackAddressAddressAddress,
        messageStackDepthAddress,
        memoryBufferView,
        memoryBufferUint8,
      );

      throw pvStatusToException(sampleRateStatus, 'Get sample rate failed', messageStack, pvError);
    }

    const sampleRate = memoryBufferView.getInt32(sampleRateAddress, true);
    await pv_free(sampleRateAddress);

    const numCharactersAddress = await aligned_alloc(
      Int32Array.BYTES_PER_ELEMENT,
      Int32Array.BYTES_PER_ELEMENT,
    );
    if (numCharactersAddress === 0) {
      throw new OrcaErrors.OrcaOutOfMemoryError('malloc failed: Cannot allocate memory');
    }

    const validCharactersAddressAddressAddress = await aligned_alloc(
      Int32Array.BYTES_PER_ELEMENT,
      Int32Array.BYTES_PER_ELEMENT,
    );
    if (validCharactersAddressAddressAddress === 0) {
      throw new OrcaErrors.OrcaOutOfMemoryError('malloc failed: Cannot allocate memory');
    }

    const validCharactersStatus = await pv_orca_valid_characters(
      objectAddress,
      numCharactersAddress,
      validCharactersAddressAddressAddress,
    );
    if (validCharactersStatus !== PV_STATUS_SUCCESS) {
      const messageStack = await Orca.getMessageStack(
        pv_get_error_stack,
        pv_free_error_stack,
        messageStackAddressAddressAddress,
        messageStackDepthAddress,
        memoryBufferView,
        memoryBufferUint8,
      );

      throw pvStatusToException(validCharactersStatus, 'Get valid characters failed', messageStack, pvError);
    }

    const numCharacters = memoryBufferView.getInt32(numCharactersAddress, true);
    const validCharactersAddressAddress = memoryBufferView.getInt32(validCharactersAddressAddressAddress, true);

    const validCharacters: string[] = [];
    for (let i = 0; i < numCharacters; i++) {
      const charIndex = memoryBufferView.getInt32(validCharactersAddressAddress + i * Int32Array.BYTES_PER_ELEMENT, true);
      validCharacters.push(
        arrayBufferToStringAtIndex(
          memoryBufferUint8,
          charIndex,
        ),
      );
    }

    await pv_free(numCharactersAddress);
    await pv_free(validCharactersAddressAddressAddress);
    await pv_orca_valid_characters_delete(validCharactersAddressAddress);

    const maxCharacterLimit = await pv_orca_max_character_limit();

    const versionAddress = await pv_orca_version();
    const version = arrayBufferToStringAtIndex(
      memoryBufferUint8,
      versionAddress,
    );

    return {
      memory: memory,
      pvFree: pv_free,
      alignedAlloc: aligned_alloc,

      objectAddress: objectAddress,
      version: version,
      sampleRate: sampleRate,
      maxCharacterLimit: maxCharacterLimit,
      validCharacters: validCharacters,
      messageStackAddressAddressAddress: messageStackAddressAddressAddress,
      messageStackDepthAddress: messageStackDepthAddress,

      pvOrcaDelete: pv_orca_delete,
      pvOrcaSynthesizeParamsInit: pv_orca_synthesize_params_init,
      pvOrcaSynthesizeParamsDelete: pv_orca_synthesize_params_delete,
      pvOrcaSynthesizeParamsSetSpeechRate: pv_orca_synthesize_params_set_speech_rate,
      pvOrcaSynthesize: pv_orca_synthesize,
      pvStatusToString: pv_status_to_string,
      pvOrcaDeletePcm: pv_orca_delete_pcm,
      pvGetErrorStack: pv_get_error_stack,
      pvFreeErrorStack: pv_free_error_stack,
    };
  }

  private static async getMessageStack(
    pv_get_error_stack: pv_get_error_stack_type,
    pv_free_error_stack: pv_free_error_stack_type,
    messageStackAddressAddressAddress: number,
    messageStackDepthAddress: number,
    memoryBufferView: DataView,
    memoryBufferUint8: Uint8Array,
  ): Promise<string[]> {
    const status = await pv_get_error_stack(messageStackAddressAddressAddress, messageStackDepthAddress);
    if (status !== PvStatus.SUCCESS) {
      throw pvStatusToException(status, 'Unable to get Orca error state');
    }

    const messageStackAddressAddress = memoryBufferView.getInt32(messageStackAddressAddressAddress, true);

    const messageStackDepth = memoryBufferView.getInt32(messageStackDepthAddress, true);
    const messageStack: string[] = [];
    for (let i = 0; i < messageStackDepth; i++) {
      const messageStackAddress = memoryBufferView.getInt32(
        messageStackAddressAddress + (i * Int32Array.BYTES_PER_ELEMENT), true);
      const message = arrayBufferToStringAtIndex(memoryBufferUint8, messageStackAddress);
      messageStack.push(message);
    }

    pv_free_error_stack(messageStackAddressAddress);

    return messageStack;
  }
}
