/*
  Copyright 2024-2025 Picovoice Inc.

  You may not use this file except in compliance with the license. A copy of the license is located in the "LICENSE"
  file accompanying this source.

  Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
  an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
  specific language governing permissions and limitations under the License.
*/

/* eslint camelcase: 0 */

import { Mutex } from 'async-mutex';

import { simd } from 'wasm-feature-detect';

import {
  arrayBufferToStringAtIndex,
  base64ToUint8Array,
  isAccessKeyValid,
  loadModel,
} from '@picovoice/web-utils';

import createModule from "./lib/pv_orca";
import createModuleSimd from "./lib/pv_orca_simd";

import {
  OrcaAlignment,
  OrcaModel,
  OrcaPhoneme,
  OrcaStreamSynthesizeResult,
  OrcaSynthesizeParams,
  OrcaSynthesizeResult,
  PvStatus,
} from './types';

import * as OrcaErrors from './orca_errors';
import { pvStatusToException } from './orca_errors';

/**
 * WebAssembly function types
 */
type pv_orca_init_type = (
  accessKey: number,
  modelPath: number,
  object: number
) => Promise<number>;
type pv_orca_delete_type = (object: number) => void;
type pv_orca_valid_characters_type = (
  object: number,
  numCharacters: number,
  validCharacters: number
) => number;
type pv_orca_valid_characters_delete_type = (validCharacters: number) => void;
type pv_orca_sample_rate_type = (object: number, sampleRate: number) => number;
type pv_orca_max_character_limit_type = (object: number, maxCharacterLimit: number) => number;
type pv_orca_synthesize_params_init_type = (object: number) => number;
type pv_orca_synthesize_params_delete_type = (object: number) => void;
type pv_orca_synthesize_params_set_speech_rate_type = (object: number, speechRate: number) => number;
type pv_orca_synthesize_params_set_random_state_type = (object: number, randomState: bigint) => number;
type pv_orca_synthesize_type = (
  object: number,
  text: number,
  synthesizeParams: number,
  numSamples: number,
  pcm: number,
  numAlignments: number,
  alignments: number
) => Promise<number>;
type pv_orca_pcm_delete_type = (object: number) => void;
type pv_orca_word_alignments_delete_type = (numAlignments: number, alignments: number) => void;
type pv_orca_stream_open_type = (
  object: number,
  synthesizeParams: number,
  stream: number
) => number;
type pv_orca_stream_synthesize_type = (
  object: number,
  text: number,
  numSamples: number,
  pcm: number
) => Promise<number>;
type pv_orca_stream_flush_type = (
  object: number,
  numSamples: number,
  pcm: number
) => Promise<number>;
type pv_orca_stream_close_type = (object: number) => void;
type pv_orca_version_type = () => number;
type pv_set_sdk_type = (sdk: number) => void;
type pv_get_error_stack_type = (messageStack: number, messageStackDepth: number) => number;
type pv_free_error_stack_type = (messageStack: number) => void;

type OrcaModule = EmscriptenModule & {
  _pv_free: (address: number) => void;

  _pv_orca_delete: pv_orca_delete_type
  _pv_orca_valid_characters: pv_orca_valid_characters_type
  _pv_orca_valid_characters_delete: pv_orca_valid_characters_delete_type
  _pv_orca_sample_rate: pv_orca_sample_rate_type
  _pv_orca_max_character_limit: pv_orca_max_character_limit_type
  _pv_orca_synthesize_params_init: pv_orca_synthesize_params_init_type
  _pv_orca_synthesize_params_delete: pv_orca_synthesize_params_delete_type
  _pv_orca_synthesize_params_set_speech_rate: pv_orca_synthesize_params_set_speech_rate_type
  _pv_orca_synthesize_params_set_random_state: pv_orca_synthesize_params_set_random_state_type
  _pv_orca_pcm_delete: pv_orca_pcm_delete_type
  _pv_orca_word_alignments_delete: pv_orca_word_alignments_delete_type
  _pv_orca_stream_open: pv_orca_stream_open_type
  _pv_orca_stream_close: pv_orca_stream_close_type
  _pv_orca_version: pv_orca_version_type

  _pv_set_sdk: pv_set_sdk_type;
  _pv_get_error_stack: pv_get_error_stack_type;
  _pv_free_error_stack: pv_free_error_stack_type;

  // em default functions
  addFunction: typeof addFunction;
  ccall: typeof ccall;
  cwrap: typeof cwrap;
}

type OrcaWasmOutput = {
  module: OrcaModule;

  pv_orca_synthesize: pv_orca_synthesize_type;
  pv_orca_stream_synthesize: pv_orca_stream_synthesize_type;
  pv_orca_stream_flush: pv_orca_stream_flush_type;

  version: string;
  sampleRate: number;
  validCharacters: string[];
  maxCharacterLimit: number;

  objectAddress: number;
  messageStackAddressAddressAddress: number;
  messageStackDepthAddress: number;
  streamPcmAddressAddress: number;
};

/**
 * OrcaStream object that converts a stream of text to a stream of audio.
 */
class Stream {
  private readonly _module: OrcaModule;
  private readonly _pv_orca_stream_synthesize: pv_orca_stream_synthesize_type;
  private readonly _pv_orca_stream_flush: pv_orca_stream_flush_type;
  private readonly _messageStackAddressAddressAddress: number;
  private readonly _messageStackDepthAddress: number;
  private readonly _functionMutex: Mutex;
  private readonly _streamPcmAddressAddress: number;
  private readonly _streamAddress: number;
  private readonly _getMessageStack: any;

  constructor(
    module: OrcaModule,
    pv_orca_stream_synthesize: pv_orca_stream_synthesize_type,
    pv_orca_stream_flush: pv_orca_stream_flush_type,
    messageStackAddressAddressAddress: number,
    messageStackDepthAddress: number,
    functionMutex: Mutex,
    streamPcmAddressAddress: number,
    streamAddress: number,
    getMessageStack: any,
  ) {
    this._module = module;
    this._pv_orca_stream_synthesize = pv_orca_stream_synthesize;
    this._pv_orca_stream_flush = pv_orca_stream_flush;
    this._messageStackAddressAddressAddress = messageStackAddressAddressAddress;
    this._messageStackDepthAddress = messageStackDepthAddress;
    this._functionMutex = functionMutex;
    this._streamPcmAddressAddress = streamPcmAddressAddress;
    this._streamAddress = streamAddress;
    this._getMessageStack = getMessageStack;
  }

  /**
   * Adds a chunk of text to the Stream object and generates audio if enough text has been added.
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
   *             audio chunk has been produced.
   */
  public async synthesize(text: string): Promise<OrcaStreamSynthesizeResult> {
    if (typeof text !== 'string') {
      throw new OrcaErrors.OrcaInvalidArgumentError(
        'The argument \'text\' must be provided as a string',
      );
    }

    return new Promise<OrcaStreamSynthesizeResult>((resolve, reject) => {
      this._functionMutex
        .runExclusive(async () => {
          if (this._module === undefined) {
            throw new OrcaErrors.OrcaInvalidStateError(
              'Attempted to call Orca stream synthesize after release.',
            );
          }

          const encodedText = new TextEncoder().encode(text);
          const textAddress = this._module._malloc((encodedText.length + 1) * Uint8Array.BYTES_PER_ELEMENT);
          if (textAddress === 0) {
            throw new OrcaErrors.OrcaOutOfMemoryError(
              'malloc failed: Cannot allocate memory',
            );
          }
          this._module.HEAPU8.set(encodedText, textAddress);
          this._module.HEAPU8[textAddress + encodedText.length] = 0;

          const numSamplesAddress = this._module._malloc(Int32Array.BYTES_PER_ELEMENT);
          if (numSamplesAddress === 0) {
            throw new OrcaErrors.OrcaOutOfMemoryError('malloc failed: Cannot allocate memory');
          }

          const streamSynthesizeStatus = await this._pv_orca_stream_synthesize(
            this._streamAddress,
            textAddress,
            numSamplesAddress,
            this._streamPcmAddressAddress,
          );
          this._module._pv_free(textAddress);
          if (streamSynthesizeStatus !== PvStatus.SUCCESS) {
            const messageStack = this._getMessageStack(
              this._module._pv_get_error_stack,
              this._module._pv_free_error_stack,
              this._messageStackAddressAddressAddress,
              this._messageStackDepthAddress,
              this._module.HEAP32,
              this._module.HEAPU8
            );

            throw pvStatusToException(streamSynthesizeStatus, 'Stream synthesize failed', messageStack);
          }

          const pcmAddress = this._module.HEAP32[this._streamPcmAddressAddress / Int32Array.BYTES_PER_ELEMENT];

          const numSamples = this._module.HEAP32[numSamplesAddress / Int32Array.BYTES_PER_ELEMENT];
          this._module._pv_free(numSamplesAddress);

          const pcm = this._module.HEAP16.slice(
            pcmAddress / Int16Array.BYTES_PER_ELEMENT,
            (pcmAddress / Int16Array.BYTES_PER_ELEMENT) + numSamples);
          this._module._pv_orca_pcm_delete(pcmAddress);

          return pcm.length > 0 ? pcm : null;
        })
        .then((result: OrcaStreamSynthesizeResult) => {
          resolve(result);
        })
        .catch(async (error: any) => {
          reject(error);
        });
    });
  }

  /**
   * Generates audio for all the buffered text that was added to the OrcaStream object
   * via `OrcaStream.synthesize()`.
   *
   * @return The generated audio as a sequence of 16-bit linearly-encoded integers, `null` if no
   *         audio chunk has been produced.
   */
  public async flush(): Promise<OrcaStreamSynthesizeResult> {
    return new Promise<OrcaStreamSynthesizeResult>((resolve, reject) => {
      this._functionMutex
        .runExclusive(async () => {
          if (this._module === undefined) {
            throw new OrcaErrors.OrcaInvalidStateError('Attempted to call OrcaStream flush after release.');
          }

          const numSamplesAddress = this._module._malloc(Int32Array.BYTES_PER_ELEMENT);
          if (numSamplesAddress === 0) {
            throw new OrcaErrors.OrcaOutOfMemoryError('malloc failed: Cannot allocate memory');
          }

          const pcmAddressAddress = this._module._malloc(Int32Array.BYTES_PER_ELEMENT);
          if (pcmAddressAddress === 0) {
            throw new OrcaErrors.OrcaOutOfMemoryError('malloc failed: Cannot allocate memory');
          }

          const streamFlushStatus = await this._pv_orca_stream_flush(
            this._streamAddress,
            numSamplesAddress,
            pcmAddressAddress,
          );
          if (streamFlushStatus !== PvStatus.SUCCESS) {
            const messageStack = await this._getMessageStack(
              this._module._pv_get_error_stack,
              this._module._pv_free_error_stack,
              this._messageStackAddressAddressAddress,
              this._messageStackDepthAddress,
              this._module.HEAP32,
              this._module.HEAPU8
            );

            throw pvStatusToException(streamFlushStatus, 'Flush failed', messageStack);
          }

          const pcmAddress = this._module.HEAP32[pcmAddressAddress / Int32Array.BYTES_PER_ELEMENT];
          this._module._pv_free(pcmAddressAddress);

          const numSamples = this._module.HEAP32[numSamplesAddress / Int32Array.BYTES_PER_ELEMENT];
          this._module._pv_free(numSamplesAddress);

          const pcm = this._module.HEAP16.slice(
            pcmAddress / Int16Array.BYTES_PER_ELEMENT,
            (pcmAddress / Int16Array.BYTES_PER_ELEMENT) + numSamples,
          );
          this._module._pv_orca_pcm_delete(pcmAddress);

          return pcm.length > 0 ? pcm : null;
        })
        .then((result: OrcaStreamSynthesizeResult) => {
          resolve(result);
        })
        .catch(async (error: any) => {
          reject(error);
        });
    });
  }

  /**
   * Releases the resources acquired by the OrcaStream object.
   */
  public async close(): Promise<void> {
    this._module._pv_orca_stream_close(this._streamAddress);
  }
}

export type OrcaStream = Stream

/**
 * JavaScript/WebAssembly Binding for Orca
 */
export class Orca {
  private readonly _module: OrcaModule;

  private readonly _pv_orca_synthesize: pv_orca_synthesize_type;
  private readonly _pv_orca_stream_synthesize: pv_orca_stream_synthesize_type;
  private readonly _pv_orca_stream_flush: pv_orca_stream_flush_type;

  private readonly _version: string;
  private readonly _sampleRate: number;
  private readonly _validCharacters: string[];
  private readonly _maxCharacterLimit: number;

  private readonly _functionMutex: Mutex;

  private readonly _objectAddress: number;
  private readonly _messageStackAddressAddressAddress: number;
  private readonly _messageStackDepthAddress: number;
  private readonly _streamPcmAddressAddress: number;

  private static _wasm: string;
  private static _wasmLib: string;
  private static _wasmSimd: string;
  private static _wasmSimdLib: string;
  private static _sdk: string = 'web';

  private static _orcaMutex = new Mutex();

  private constructor(handleWasm: OrcaWasmOutput) {
    this._module = handleWasm.module;

    this._pv_orca_synthesize = handleWasm.pv_orca_synthesize;
    this._pv_orca_stream_synthesize = handleWasm.pv_orca_stream_synthesize;
    this._pv_orca_stream_flush = handleWasm.pv_orca_stream_flush;

    this._version = handleWasm.version;
    this._sampleRate = handleWasm.sampleRate;
    this._validCharacters = handleWasm.validCharacters;
    this._maxCharacterLimit = handleWasm.maxCharacterLimit;

    this._objectAddress = handleWasm.objectAddress;
    this._messageStackAddressAddressAddress = handleWasm.messageStackAddressAddressAddress;
    this._messageStackDepthAddress = handleWasm.messageStackDepthAddress;
    this._streamPcmAddressAddress = handleWasm.streamPcmAddressAddress;

    this._functionMutex = new Mutex();
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
   * Get valid characters.
   */
  get validCharacters(): string[] {
    return this._validCharacters;
  }

  /**
   * Get maximum character limit.
   */
  get maxCharacterLimit(): number {
    return this._maxCharacterLimit;
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
   * Set base64 wasm file in text format.
   * @param wasmLib Base64'd wasm file in text format.
   */
  public static setWasmLib(wasmLib: string): void {
    if (this._wasmLib === undefined) {
      this._wasmLib = wasmLib;
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

  /**
   * Set base64 SIMD wasm file in text format.
   * @param wasmSimdLib Base64'd SIMD wasm file in text format.
   */
  public static setWasmSimdLib(wasmSimdLib: string): void {
    if (this._wasmSimdLib === undefined) {
      this._wasmSimdLib = wasmSimdLib;
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
   * @param model Orca model options.
   * @param model.base64 The model in base64 string to initialize Orca.
   * @param model.publicPath The model path relative to the public directory.
   * @param model.customWritePath Custom path to save the model in storage.
   * Set to a different name to use multiple models across `orca` instances.
   * @param model.forceWrite Flag to overwrite the model in storage even if it exists.
   * @param model.version Version of the model file. Increment to update the model file in storage.
   *
   * @returns An instance of the Orca engine.
   */
  public static async create(
    accessKey: string,
    model: OrcaModel,
  ): Promise<Orca> {
    const customWritePath = (model.customWritePath) ? model.customWritePath : 'orca_model';
    const modelPath = await loadModel({ ...model, customWritePath });

    return Orca._init(accessKey, modelPath);
  }

  public static async _init(
    accessKey: string,
    modelPath: string,
  ): Promise<Orca> {
    if (!isAccessKeyValid(accessKey)) {
      throw new OrcaErrors.OrcaInvalidArgumentError('Invalid AccessKey');
    }

    return new Promise<Orca>((resolve, reject) => {
      Orca._orcaMutex
        .runExclusive(async () => {
          const isSimd = await simd();
          const wasmOutput = await Orca.initWasm(
            accessKey.trim(),
            modelPath.trim(),
            (isSimd) ? this._wasmSimd : this._wasm,
            (isSimd) ? this._wasmSimdLib : this._wasmLib,
            (isSimd) ? createModuleSimd : createModule);
          return new Orca(wasmOutput);
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
   * Generates audio from text. The returned audio contains the speech representation of the text.
   * The maximum number of characters per call to `.synthesize()` is `.maxCharacterLimit`.
   * Allowed characters are lower-case and upper-case letters and punctuation marks that can be retrieved with `.validCharacters`.
   * Custom pronunciations can be embedded in the text via the syntax `{word|pronunciation}`.
   * The pronunciation is expressed in ARPAbet format, e.g.: "I {live|L IH V} in {Sevilla|S EH V IY Y AH}".
   *
   * @param text A string of text.
   * @param synthesizeParams Optional configuration arguments.
   * @param synthesizeParams.speechRate Configure the rate of speech of the synthesized speech.
   * @param synthesizeParams.randomState Configure the random seed for the synthesized speech.
   *
   * @return A result object containing the generated audio as a sequence of 16-bit linearly-encoded integers
   *         and a sequence of OrcaAlignment objects representing the word alignments.
   */
  public async synthesize(
    text: string,
    synthesizeParams: OrcaSynthesizeParams = {
      speechRate: 1.0,
      randomState: null,
    },
  ): Promise<OrcaSynthesizeResult> {
    if (typeof text !== 'string') {
      throw new OrcaErrors.OrcaInvalidArgumentError(
        `The argument 'text' must be provided as a string`,
      );
    }

    if (text.trim().length > this._maxCharacterLimit) {
      throw new OrcaErrors.OrcaInvalidArgumentError(`
        'text' length must be smaller than ${this._maxCharacterLimit}
      `);
    }

    return new Promise<OrcaSynthesizeResult>((resolve, reject) => {
      this._functionMutex
        .runExclusive(async () => {
          if (this._module === undefined) {
            throw new OrcaErrors.OrcaInvalidStateError(
              'Attempted to call Orca synthesize after release.',
            );
          }

          const encodedText = new TextEncoder().encode(text);
          const textAddress = this._module._malloc(
            (encodedText.length + 1) * Uint8Array.BYTES_PER_ELEMENT,
          );
          if (textAddress === 0) {
            throw new OrcaErrors.OrcaOutOfMemoryError(
              'malloc failed: Cannot allocate memory for text',
            );
          }
          this._module.HEAPU8.set(encodedText, textAddress);
          this._module.HEAPU8[textAddress + encodedText.length] = 0;

          const synthesizeParamsAddressAddress = this._module._malloc(Int32Array.BYTES_PER_ELEMENT);
          if (synthesizeParamsAddressAddress === 0) {
            throw new OrcaErrors.OrcaOutOfMemoryError(
              'malloc failed: Cannot allocate memory for synthesizeParams'
            );
          }

          const initStatus = this._module._pv_orca_synthesize_params_init(synthesizeParamsAddressAddress);
          if (initStatus !== PvStatus.SUCCESS) {
            const messageStack = Orca.getMessageStack(
              this._module._pv_get_error_stack,
              this._module._pv_free_error_stack,
              this._messageStackAddressAddressAddress,
              this._messageStackDepthAddress,
              this._module.HEAP32,
              this._module.HEAPU8
            );

            throw pvStatusToException(initStatus, 'Init synthesize params failed', messageStack);
          }

          const synthesizeParamsAddress =
            this._module.HEAP32[synthesizeParamsAddressAddress / Int32Array.BYTES_PER_ELEMENT];
          this._module._pv_free(synthesizeParamsAddressAddress);

          if (synthesizeParams.speechRate !== null && synthesizeParams.speechRate !== undefined) {
            const setSpeechRateStatus = this._module._pv_orca_synthesize_params_set_speech_rate(
              synthesizeParamsAddress,
              synthesizeParams.speechRate,
            );
            if (setSpeechRateStatus !== PvStatus.SUCCESS) {
              const messageStack = Orca.getMessageStack(
                this._module._pv_get_error_stack,
                this._module._pv_free_error_stack,
                this._messageStackAddressAddressAddress,
                this._messageStackDepthAddress,
                this._module.HEAP32,
                this._module.HEAPU8
              );

              throw pvStatusToException(setSpeechRateStatus, 'Set speech rate failed', messageStack);
            }
          }

          if (synthesizeParams.randomState !== null && synthesizeParams.randomState !== undefined) {
            const setRandomStateStatus = this._module._pv_orca_synthesize_params_set_random_state(
              synthesizeParamsAddress,
              BigInt(synthesizeParams.randomState),
            );
            if (setRandomStateStatus !== PvStatus.SUCCESS) {
              const messageStack = Orca.getMessageStack(
                this._module._pv_get_error_stack,
                this._module._pv_free_error_stack,
                this._messageStackAddressAddressAddress,
                this._messageStackDepthAddress,
                this._module.HEAP32,
                this._module.HEAPU8
              );
              throw pvStatusToException(setRandomStateStatus, 'Set random state failed', messageStack);
            }
          }

          const numSamplesAddress = this._module._malloc(Int32Array.BYTES_PER_ELEMENT);
          if (numSamplesAddress === 0) {
            throw new OrcaErrors.OrcaOutOfMemoryError('malloc failed: Cannot allocate memory for numSamples');
          }

          const pcmAddressAddress = this._module._malloc(Int32Array.BYTES_PER_ELEMENT);
          if (pcmAddressAddress === 0) {
            throw new OrcaErrors.OrcaOutOfMemoryError('malloc failed: Cannot allocate memory for pcm');
          }

          const numAlignmentsAddress = this._module._malloc(Int32Array.BYTES_PER_ELEMENT);
          if (numAlignmentsAddress === 0) {
            throw new OrcaErrors.OrcaOutOfMemoryError(
              'malloc failed: Cannot allocate memory for numAlignments'
            );
          }

          const alignmentsAddressAddressAddress = this._module._malloc(Int32Array.BYTES_PER_ELEMENT);
          if (alignmentsAddressAddressAddress === 0) {
            throw new OrcaErrors.OrcaOutOfMemoryError(
              'malloc failed: Cannot allocate memory for alignments'
            );
          }

          const synthesizeStatus = await this._pv_orca_synthesize(
            this._objectAddress,
            textAddress,
            synthesizeParamsAddress,
            numSamplesAddress,
            pcmAddressAddress,
            numAlignmentsAddress,
            alignmentsAddressAddressAddress,
          );
          this._module._pv_free(textAddress);
          this._module._pv_orca_synthesize_params_delete(synthesizeParamsAddress);

          if (synthesizeStatus !== PvStatus.SUCCESS) {
            const messageStack = Orca.getMessageStack(
              this._module._pv_get_error_stack,
              this._module._pv_free_error_stack,
              this._messageStackAddressAddressAddress,
              this._messageStackDepthAddress,
              this._module.HEAP32,
              this._module.HEAPU8
            );

            throw pvStatusToException(synthesizeStatus, 'Synthesize failed', messageStack);
          }

          const pcmAddress = this._module.HEAP32[pcmAddressAddress / Int32Array.BYTES_PER_ELEMENT];
          this._module._pv_free(pcmAddressAddress);

          const numSamples = this._module.HEAP32[numSamplesAddress / Int32Array.BYTES_PER_ELEMENT];
          this._module._pv_free(numSamplesAddress);

          const pcm = this._module.HEAP16.slice(
            pcmAddress / Int16Array.BYTES_PER_ELEMENT,
            (pcmAddress / Int16Array.BYTES_PER_ELEMENT) + numSamples
          );
          this._module._pv_orca_pcm_delete(pcmAddress);

          const numAlignments = this._module.HEAP32[numAlignmentsAddress / Int32Array.BYTES_PER_ELEMENT];
          const alignmentsAddressAddress =
            this._module.HEAP32[alignmentsAddressAddressAddress / Int32Array.BYTES_PER_ELEMENT];

          let ptr = this._module.HEAP32[alignmentsAddressAddress / Int32Array.BYTES_PER_ELEMENT];
          const alignments: OrcaAlignment[] = [];
          for (let i = 1; i <= numAlignments; i++) {
            const wordAddress = this._module.HEAP32[ptr / Int32Array.BYTES_PER_ELEMENT];
            const word = arrayBufferToStringAtIndex(
              this._module.HEAPU8,
              wordAddress,
            );
            ptr += Uint32Array.BYTES_PER_ELEMENT;
            const startSec = this._module.HEAPF32[ptr / Float32Array.BYTES_PER_ELEMENT];
            ptr += Float32Array.BYTES_PER_ELEMENT;
            const endSec = this._module.HEAPF32[ptr / Float32Array.BYTES_PER_ELEMENT];
            ptr += Float32Array.BYTES_PER_ELEMENT;
            const numPhonemes = this._module.HEAP32[ptr / Int32Array.BYTES_PER_ELEMENT];
            ptr += Uint32Array.BYTES_PER_ELEMENT;
            const phonemesAddress = this._module.HEAP32[ptr / Int32Array.BYTES_PER_ELEMENT];
            ptr = this._module.HEAP32[(alignmentsAddressAddress / Int32Array.BYTES_PER_ELEMENT) + i];

            let phonemesPtr = this._module.HEAP32[phonemesAddress / Int32Array.BYTES_PER_ELEMENT];
            const phonemes: OrcaPhoneme[] = [];
            for (let j = 1; j <= numPhonemes; j++) {
              const phonemeAddress = this._module.HEAP32[phonemesPtr / Int32Array.BYTES_PER_ELEMENT];
              const phoneme = arrayBufferToStringAtIndex(
                this._module.HEAPU8,
                phonemeAddress,
              );
              phonemesPtr += Uint32Array.BYTES_PER_ELEMENT;
              const pStartSec = this._module.HEAPF32[phonemesPtr / Float32Array.BYTES_PER_ELEMENT];
              phonemesPtr += Float32Array.BYTES_PER_ELEMENT;
              const pEndSec = this._module.HEAPF32[phonemesPtr / Float32Array.BYTES_PER_ELEMENT];
              phonemesPtr = this._module.HEAP32[(phonemesAddress / Int32Array.BYTES_PER_ELEMENT) + j];
              phonemes.push({ phoneme, startSec: pStartSec, endSec: pEndSec });
            }
            alignments.push({ word, startSec, endSec, phonemes });
          }
          this._module._pv_free(numAlignmentsAddress);
          this._module._pv_free(alignmentsAddressAddressAddress);
          this._module._pv_orca_word_alignments_delete(numAlignments, alignmentsAddressAddress);

          return { pcm, alignments };
        })
        .then((result: OrcaSynthesizeResult) => {
          resolve(result);
        })
        .catch(async (error: any) => {
          reject(error);
        });
    });
  }

  /**
   * Opens a stream for streaming text synthesis.
   *
   * @param synthesizeParams Optional configuration arguments.
   * @param synthesizeParams.speechRate Configure the rate of speech of the synthesized speech.
   * @param synthesizeParams.randomState Configure the random seed for the synthesized speech.
   *
   * @returns An instance of OrcaStream.
   */
  public async streamOpen(
    synthesizeParams: OrcaSynthesizeParams = {
      speechRate: 1.0,
      randomState: null,
    },
  ): Promise<OrcaStream> {
    return new Promise<OrcaStream>((resolve, reject) => {
      this._functionMutex
        .runExclusive(async () => {
          if (this._module === undefined) {
            throw new OrcaErrors.OrcaInvalidStateError('Attempted to call Orca stream open after release.');
          }

          const synthesizeParamsAddressAddress = this._module._malloc(Int32Array.BYTES_PER_ELEMENT);
          if (synthesizeParamsAddressAddress === 0) {
            throw new OrcaErrors.OrcaOutOfMemoryError('malloc failed: Cannot allocate memory');
          }

          const initStatus = this._module._pv_orca_synthesize_params_init(synthesizeParamsAddressAddress);
          if (initStatus !== PvStatus.SUCCESS) {
            const messageStack = Orca.getMessageStack(
              this._module._pv_get_error_stack,
              this._module._pv_free_error_stack,
              this._messageStackAddressAddressAddress,
              this._messageStackDepthAddress,
              this._module.HEAP32,
              this._module.HEAPU8
            );

            throw pvStatusToException(initStatus, 'Stream open failed', messageStack);
          }

          const synthesizeParamsAddress = this._module.HEAP32[
            synthesizeParamsAddressAddress / Int32Array.BYTES_PER_ELEMENT
          ];
          this._module._pv_free(synthesizeParamsAddressAddress);

          if (synthesizeParams.speechRate !== null && synthesizeParams.speechRate !== undefined) {
            const setSpeechRateStatus = this._module._pv_orca_synthesize_params_set_speech_rate(
              synthesizeParamsAddress,
              synthesizeParams.speechRate);
            if (setSpeechRateStatus !== PvStatus.SUCCESS) {
              const messageStack = Orca.getMessageStack(
                this._module._pv_get_error_stack,
                this._module._pv_free_error_stack,
                this._messageStackAddressAddressAddress,
                this._messageStackDepthAddress,
                this._module.HEAP32,
                this._module.HEAPU8
              );

              throw pvStatusToException(setSpeechRateStatus, 'Stream open failed', messageStack);
            }
          }

          if (synthesizeParams.randomState !== null && synthesizeParams.randomState !== undefined) {
            const setRandomStateStatus = this._module._pv_orca_synthesize_params_set_random_state(
              synthesizeParamsAddress,
              BigInt(synthesizeParams.randomState));
            if (setRandomStateStatus !== PvStatus.SUCCESS) {
              const messageStack = Orca.getMessageStack(
                this._module._pv_get_error_stack,
                this._module._pv_free_error_stack,
                this._messageStackAddressAddressAddress,
                this._messageStackDepthAddress,
                this._module.HEAP32,
                this._module.HEAPU8
              );

              throw pvStatusToException(setRandomStateStatus, 'Stream open failed', messageStack);
            }
          }

          const streamAddressAddress = this._module._malloc(Int32Array.BYTES_PER_ELEMENT);
          if (streamAddressAddress === 0) {
            throw new OrcaErrors.OrcaOutOfMemoryError('malloc failed: Cannot allocate memory');
          }

          const streamOpenStatus = this._module._pv_orca_stream_open(
            this._objectAddress,
            synthesizeParamsAddress,
            streamAddressAddress,
          );
          this._module._pv_orca_synthesize_params_delete(synthesizeParamsAddress);

          if (streamOpenStatus !== PvStatus.SUCCESS) {
            const messageStack = Orca.getMessageStack(
              this._module._pv_get_error_stack,
              this._module._pv_free_error_stack,
              this._messageStackAddressAddressAddress,
              this._messageStackDepthAddress,
              this._module.HEAP32,
              this._module.HEAPU8
            );

            throw pvStatusToException(streamOpenStatus, 'Stream open failed', messageStack);
          }
          const streamAddress = this._module.HEAP32[streamAddressAddress / Int32Array.BYTES_PER_ELEMENT];
          this._module._pv_free(streamAddressAddress);

          return new Stream(
            this._module,
            this._pv_orca_stream_synthesize,
            this._pv_orca_stream_flush,
            this._messageStackAddressAddressAddress,
            this._messageStackDepthAddress,
            this._functionMutex,
            this._streamPcmAddressAddress,
            streamAddress,
            Orca.getMessageStack,
          );
        })
        .then(result => {
          resolve(result);
        })
        .catch(async (error: any) => {
          reject(error);
        });
    });
  }

  /**
   * Releases resources acquired by WebAssembly module.
   */
  public async release(): Promise<void> {
    if (!this._module) {
      return;
    }
    this._module._pv_orca_delete(this._objectAddress);
    this._module._pv_free(this._messageStackAddressAddressAddress);
    this._module._pv_free(this._messageStackDepthAddress);
    this._module._pv_free(this._streamPcmAddressAddress);
  }

  private static async initWasm(
    accessKey: string,
    modelPath: string,
    wasmBase64: string,
    wasmLibBase64: string,
    createModuleFunc: any
  ): Promise<OrcaWasmOutput> {
    const blob = new Blob(
      [base64ToUint8Array(wasmLibBase64)],
      { type: 'application/javascript' }
    );
    const module: OrcaModule = await createModuleFunc({
      mainScriptUrlOrBlob: blob,
      wasmBinary: base64ToUint8Array(wasmBase64),
    });

    const pv_orca_init: pv_orca_init_type = this.wrapAsyncFunction(
      module,
      "pv_orca_init",
      3);
    const pv_orca_synthesize: pv_orca_synthesize_type = this.wrapAsyncFunction(
      module,
      "pv_orca_synthesize",
      7);
    const pv_orca_stream_synthesize: pv_orca_stream_synthesize_type = this.wrapAsyncFunction(
      module,
      "pv_orca_stream_synthesize",
      4);
    const pv_orca_stream_flush: pv_orca_stream_flush_type = this.wrapAsyncFunction(
      module,
      "pv_orca_stream_flush",
      3);

    const objectAddressAddress = module._malloc(Int32Array.BYTES_PER_ELEMENT);
    if (objectAddressAddress === 0) {
      throw new OrcaErrors.OrcaOutOfMemoryError('malloc failed: Cannot allocate memory');
    }

    const accessKeyAddress = module._malloc((accessKey.length + 1) * Uint8Array.BYTES_PER_ELEMENT);
    if (accessKeyAddress === 0) {
      throw new OrcaErrors.OrcaOutOfMemoryError('malloc failed: Cannot allocate memory');
    }
    for (let i = 0; i < accessKey.length; i++) {
      module.HEAPU8[accessKeyAddress + i] = accessKey.charCodeAt(i);
    }
    module.HEAPU8[accessKeyAddress + accessKey.length] = 0;

    const modelPathEncoded = new TextEncoder().encode(modelPath);
    const modelPathAddress = module._malloc((modelPathEncoded.length + 1) * Uint8Array.BYTES_PER_ELEMENT);
    if (modelPathAddress === 0) {
      throw new OrcaErrors.OrcaOutOfMemoryError('malloc failed: Cannot allocate memory');
    }
    module.HEAPU8.set(modelPathEncoded, modelPathAddress);
    module.HEAPU8[modelPathAddress + modelPathEncoded.length] = 0;

    const sdkEncoded = new TextEncoder().encode(this._sdk);
    const sdkAddress = module._malloc((sdkEncoded.length + 1) * Uint8Array.BYTES_PER_ELEMENT);
    if (!sdkAddress) {
      throw new OrcaErrors.OrcaOutOfMemoryError('malloc failed: Cannot allocate memory');
    }
    module.HEAPU8.set(sdkEncoded, sdkAddress);
    module.HEAPU8[sdkAddress + sdkEncoded.length] = 0;
    module._pv_set_sdk(sdkAddress);
    module._pv_free(sdkAddress);

    const messageStackDepthAddress = module._malloc(Int32Array.BYTES_PER_ELEMENT);
    if (!messageStackDepthAddress) {
      throw new OrcaErrors.OrcaOutOfMemoryError('malloc failed: Cannot allocate memory');
    }

    const messageStackAddressAddressAddress = module._malloc(Int32Array.BYTES_PER_ELEMENT);
    if (!messageStackAddressAddressAddress) {
      throw new OrcaErrors.OrcaOutOfMemoryError('malloc failed: Cannot allocate memory');
    }

    const initStatus = await pv_orca_init(
      accessKeyAddress,
      modelPathAddress,
      objectAddressAddress);
    module._pv_free(accessKeyAddress);
    module._pv_free(modelPathAddress);

    if (initStatus !== PvStatus.SUCCESS) {
      const messageStack = Orca.getMessageStack(
        module._pv_get_error_stack,
        module._pv_free_error_stack,
        messageStackAddressAddressAddress,
        messageStackDepthAddress,
        module.HEAP32,
        module.HEAPU8,
      );

      throw pvStatusToException(initStatus, 'Initialization failed', messageStack);
    }

    const objectAddress = module.HEAP32[objectAddressAddress / Int32Array.BYTES_PER_ELEMENT];
    module._pv_free(objectAddressAddress);

    const sampleRateAddress = module._malloc(Int32Array.BYTES_PER_ELEMENT);
    if (sampleRateAddress === 0) {
      throw new OrcaErrors.OrcaOutOfMemoryError('malloc failed: Cannot allocate memory');
    }
    const sampleRateStatus = module._pv_orca_sample_rate(objectAddress, sampleRateAddress);
    if (sampleRateStatus !== PvStatus.SUCCESS) {
      const messageStack = Orca.getMessageStack(
        module._pv_get_error_stack,
        module._pv_free_error_stack,
        messageStackAddressAddressAddress,
        messageStackDepthAddress,
        module.HEAP32,
        module.HEAPU8,
      );

      throw pvStatusToException(sampleRateStatus, 'Get sample rate failed', messageStack);
    }

    const sampleRate = module.HEAP32[sampleRateAddress / Int32Array.BYTES_PER_ELEMENT];
    module._pv_free(sampleRateAddress);

    const maxCharacterLimitAddress = module._malloc(Int32Array.BYTES_PER_ELEMENT);
    if (maxCharacterLimitAddress === 0) {
      throw new OrcaErrors.OrcaOutOfMemoryError('malloc failed: Cannot allocate memory');
    }
    const maxCharacterLimitStatus = module._pv_orca_max_character_limit(objectAddress, maxCharacterLimitAddress);
    if (maxCharacterLimitStatus !== PvStatus.SUCCESS) {
      const messageStack = Orca.getMessageStack(
        module._pv_get_error_stack,
        module._pv_free_error_stack,
        messageStackAddressAddressAddress,
        messageStackDepthAddress,
        module.HEAP32,
        module.HEAPU8,
      );

      throw pvStatusToException(maxCharacterLimitStatus, 'Get max character limit failed', messageStack);
    }

    const maxCharacterLimit = module.HEAP32[maxCharacterLimitAddress / Int32Array.BYTES_PER_ELEMENT];
    module._pv_free(maxCharacterLimitAddress);

    const numCharactersAddress = module._malloc(Int32Array.BYTES_PER_ELEMENT);
    if (numCharactersAddress === 0) {
      throw new OrcaErrors.OrcaOutOfMemoryError('malloc failed: Cannot allocate memory');
    }

    const validCharactersAddressAddressAddress = module._malloc(Int32Array.BYTES_PER_ELEMENT);
    if (validCharactersAddressAddressAddress === 0) {
      throw new OrcaErrors.OrcaOutOfMemoryError('malloc failed: Cannot allocate memory');
    }

    const validCharactersStatus = module._pv_orca_valid_characters(
      objectAddress,
      numCharactersAddress,
      validCharactersAddressAddressAddress,
    );
    if (validCharactersStatus !== PvStatus.SUCCESS) {
      const messageStack = Orca.getMessageStack(
        module._pv_get_error_stack,
        module._pv_free_error_stack,
        messageStackAddressAddressAddress,
        messageStackDepthAddress,
        module.HEAP32,
        module.HEAPU8,
      );

      throw pvStatusToException(validCharactersStatus, 'Get valid characters failed', messageStack);
    }

    const numCharacters = module.HEAP32[numCharactersAddress / Int32Array.BYTES_PER_ELEMENT];
    const validCharactersAddressAddress = module.HEAP32[
      validCharactersAddressAddressAddress / Int32Array.BYTES_PER_ELEMENT
    ];

    const validCharacters: string[] = [];
    for (let i = 0; i < numCharacters; i++) {
      const charIndex = module.HEAP32[(validCharactersAddressAddress / Int32Array.BYTES_PER_ELEMENT) + i];
      validCharacters.push(
        arrayBufferToStringAtIndex(
          module.HEAPU8,
          charIndex,
        ),
      );
    }

    module._pv_free(numCharactersAddress);
    module._pv_free(validCharactersAddressAddressAddress);
    module._pv_orca_valid_characters_delete(validCharactersAddressAddress);

    const versionAddress = module._pv_orca_version();
    const version = arrayBufferToStringAtIndex(
      module.HEAPU8,
      versionAddress,
    );

    const streamPcmAddressAddress = module._malloc(Int32Array.BYTES_PER_ELEMENT);
    if (streamPcmAddressAddress === 0) {
      throw new OrcaErrors.OrcaOutOfMemoryError('malloc failed: Cannot allocate memory');
    }

    return {
      module: module,

      pv_orca_synthesize: pv_orca_synthesize,
      pv_orca_stream_synthesize: pv_orca_stream_synthesize,
      pv_orca_stream_flush: pv_orca_stream_flush,

      version: version,
      sampleRate: sampleRate,
      maxCharacterLimit: maxCharacterLimit,
      validCharacters: validCharacters,

      objectAddress: objectAddress,
      streamPcmAddressAddress: streamPcmAddressAddress,
      messageStackAddressAddressAddress: messageStackAddressAddressAddress,
      messageStackDepthAddress: messageStackDepthAddress,
    };
  }

  private static getMessageStack(
    pv_get_error_stack: pv_get_error_stack_type,
    pv_free_error_stack: pv_free_error_stack_type,
    messageStackAddressAddressAddress: number,
    messageStackDepthAddress: number,
    memoryBufferInt32: Int32Array,
    memoryBufferUint8: Uint8Array,
  ): string[] {
    const status = pv_get_error_stack(messageStackAddressAddressAddress, messageStackDepthAddress);
    if (status !== PvStatus.SUCCESS) {
      throw new Error(`Unable to get error state: ${status}`);
    }

    const messageStackAddressAddress = memoryBufferInt32[messageStackAddressAddressAddress / Int32Array.BYTES_PER_ELEMENT];

    const messageStackDepth = memoryBufferInt32[messageStackDepthAddress / Int32Array.BYTES_PER_ELEMENT];
    const messageStack: string[] = [];
    for (let i = 0; i < messageStackDepth; i++) {
      const messageStackAddress = memoryBufferInt32[
        (messageStackAddressAddress / Int32Array.BYTES_PER_ELEMENT) + i
      ];
      const message = arrayBufferToStringAtIndex(memoryBufferUint8, messageStackAddress);
      messageStack.push(message);
    }

    pv_free_error_stack(messageStackAddressAddress);

    return messageStack;
  }

  private static wrapAsyncFunction(module: OrcaModule, functionName: string, numArgs: number): (...args: any[]) => any {
    // @ts-ignore
    return module.cwrap(
      functionName,
      "number",
      Array(numArgs).fill("number"),
      { async: true }
    );
  }
}
