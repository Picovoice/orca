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

import { simd } from 'wasm-feature-detect';

import {
  aligned_alloc_type,
  arrayBufferToStringAtIndex,
  buildWasm,
  isAccessKeyValid,
  loadModel,
  pv_free_type,
  PvError,
} from '@picovoice/web-utils';

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
type pv_orca_init_type = (accessKey: number, modelPath: number, object: number) => Promise<number>;
type pv_orca_delete_type = (object: number) => Promise<void>;
type pv_orca_valid_characters_type = (object: number, numCharacters: number, validCharacters: number) => Promise<number>;
type pv_orca_valid_characters_delete_type = (validCharacters: number) => Promise<void>;
type pv_orca_sample_rate_type = (object: number, sampleRate: number) => Promise<number>;
type pv_orca_max_character_limit_type = (object: number, maxCharacterLimit: number) => Promise<number>;
type pv_orca_synthesize_params_init_type = (object: number) => Promise<number>;
type pv_orca_synthesize_params_delete_type = (object: number) => Promise<void>;
type pv_orca_synthesize_params_set_speech_rate_type = (object: number, speechRate: number) => Promise<number>;
type pv_orca_synthesize_params_set_random_state_type = (object: number, randomState: bigint) => Promise<number>;
type pv_orca_synthesize_type = (object: number, text: number, synthesizeParams: number, numSamples: number, pcm: number, numAlignments: number, alignments: number) => Promise<number>;
type pv_orca_pcm_delete_type = (object: number) => Promise<void>;
type pv_orca_word_alignments_delete_type = (numAlignments: number, alignments: number) => Promise<void>;
type pv_orca_stream_open_type = (object: number, synthesizeParams: number, stream: number) => Promise<number>;
type pv_orca_stream_synthesize_type = (object: number, text: number, numSamples: number, pcm: number) => Promise<number>;
type pv_orca_stream_flush_type = (object: number, numSamples: number, pcm: number) => Promise<number>;
type pv_orca_stream_close_type = (object: number) => Promise<void>;
type pv_orca_version_type = () => Promise<number>;
type pv_set_sdk_type = (sdk: number) => Promise<void>;
type pv_get_error_stack_type = (messageStack: number, messageStackDepth: number) => Promise<number>;
type pv_free_error_stack_type = (messageStack: number) => Promise<void>;

type OrcaWasmOutput = {
  version: string;
  sampleRate: number;
  validCharacters: string[];
  maxCharacterLimit: number;

  memory: WebAssembly.Memory;
  alignedAlloc: aligned_alloc_type;
  pvFree: pv_free_type;
  pvGetErrorStack: pv_get_error_stack_type;
  pvFreeErrorStack: pv_free_error_stack_type;
  messageStackAddressAddressAddress: number;
  messageStackDepthAddress: number;

  objectAddress: number;
  pvOrcaDelete: pv_orca_delete_type;
  pvOrcaSynthesize: pv_orca_synthesize_type;
  pvOrcaSynthesizeParamsInit: pv_orca_synthesize_params_init_type;
  pvOrcaSynthesizeParamsDelete: pv_orca_synthesize_params_delete_type;
  pvOrcaSynthesizeParamsSetSpeechRate: pv_orca_synthesize_params_set_speech_rate_type
  pvOrcaSynthesizeParamsSetRandomState: pv_orca_synthesize_params_set_random_state_type
  pvOrcaPcmDelete: pv_orca_pcm_delete_type;
  pvOrcaWordAlignmentsDelete: pv_orca_word_alignments_delete_type;

  streamPcmAddressAddress: number;
  pvOrcaStreamOpen: pv_orca_stream_open_type;
  pvOrcaStreamSynthesize: pv_orca_stream_synthesize_type;
  pvOrcaStreamFlush: pv_orca_stream_flush_type;
  pvOrcaStreamClose: pv_orca_stream_close_type;
};

/**
 * OrcaStream object that converts a stream of text to a stream of audio.
 */
class Stream {
  private _wasmMemory: WebAssembly.Memory;
  private readonly _alignedAlloc: CallableFunction;
  private readonly _pvFree: pv_free_type;
  private readonly _pvGetErrorStack: pv_get_error_stack_type;
  private readonly _pvFreeErrorStack: pv_free_error_stack_type;
  private readonly _messageStackAddressAddressAddress: number;
  private readonly _messageStackDepthAddress: number;

  private readonly _functionMutex: Mutex;
  private readonly _streamPcmAddressAddress: number;
  private readonly _pvOrcaPcmDelete: pv_orca_pcm_delete_type;
  private readonly _pvOrcaStreamSynthesize: pv_orca_stream_synthesize_type;
  private readonly _pvOrcaStreamFlush: pv_orca_stream_flush_type;
  private readonly _pvOrcaStreamClose: pv_orca_stream_close_type;
  private readonly _streamAddress: number;
  private readonly _getMessageStack: any;

  constructor(
    wasmMemory: WebAssembly.Memory,
    alignedAlloc: CallableFunction,
    pvFree: pv_free_type,
    pvGetErrorStack: pv_get_error_stack_type,
    pvFreeErrorStack: pv_free_error_stack_type,
    messageStackAddressAddressAddress: number,
    messageStackDepthAddress: number,
    functionMutex: Mutex,
    streamPcmAddressAddress: number,
    pvOrcaPcmDelete: pv_orca_pcm_delete_type,
    pvOrcaStreamSynthesize: pv_orca_stream_synthesize_type,
    pvOrcaStreamFlush: pv_orca_stream_flush_type,
    pvOrcaStreamClose: pv_orca_stream_close_type,
    streamAddress: number,
    getMessageStack: any,
  ) {
    this._wasmMemory = wasmMemory;
    this._alignedAlloc = alignedAlloc;
    this._pvFree = pvFree;
    this._pvGetErrorStack = pvGetErrorStack;
    this._pvFreeErrorStack = pvFreeErrorStack;
    this._messageStackAddressAddressAddress = messageStackAddressAddressAddress;
    this._messageStackDepthAddress = messageStackDepthAddress;
    this._functionMutex = functionMutex;
    this._streamPcmAddressAddress = streamPcmAddressAddress;
    this._pvOrcaPcmDelete = pvOrcaPcmDelete;
    this._pvOrcaStreamSynthesize = pvOrcaStreamSynthesize;
    this._pvOrcaStreamFlush = pvOrcaStreamFlush;
    this._pvOrcaStreamClose = pvOrcaStreamClose;
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
          if (this._wasmMemory === undefined) {
            throw new OrcaErrors.OrcaInvalidStateError(
              'Attempted to call Orca stream synthesize after release.',
            );
          }

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

          const numSamplesAddress = await this._alignedAlloc(
            Int32Array.BYTES_PER_ELEMENT,
            Int32Array.BYTES_PER_ELEMENT,
          );
          if (numSamplesAddress === 0) {
            throw new OrcaErrors.OrcaOutOfMemoryError('malloc failed: Cannot allocate memory');
          }

          const streamSynthesizeStatus = await this._pvOrcaStreamSynthesize(
            this._streamAddress,
            textAddress,
            numSamplesAddress,
            this._streamPcmAddressAddress,
          );
          await this._pvFree(textAddress);

          const memoryBufferView = new DataView(this._wasmMemory.buffer);
          const memoryBufferUint8 = new Uint8Array(this._wasmMemory.buffer);

          if (streamSynthesizeStatus !== PvStatus.SUCCESS) {
            const messageStack = await this._getMessageStack(
              this._pvGetErrorStack,
              this._pvFreeErrorStack,
              this._messageStackAddressAddressAddress,
              this._messageStackDepthAddress,
              memoryBufferView,
              memoryBufferUint8,
            );

            throw pvStatusToException(streamSynthesizeStatus, 'Stream synthesize failed', messageStack);
          }

          const pcmAddress = memoryBufferView.getInt32(
            this._streamPcmAddressAddress,
            true,
          );

          const numSamples = memoryBufferView.getInt32(
            numSamplesAddress,
            true,
          );
          await this._pvFree(numSamplesAddress);

          const outputMemoryBuffer = new Int16Array(this._wasmMemory.buffer);
          const pcm = outputMemoryBuffer.slice(
            pcmAddress / Int16Array.BYTES_PER_ELEMENT,
            (pcmAddress / Int16Array.BYTES_PER_ELEMENT) + numSamples,
          );
          await this._pvOrcaPcmDelete(pcmAddress);

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
          if (this._wasmMemory === undefined) {
            throw new OrcaErrors.OrcaInvalidStateError('Attempted to call OrcaStream flush after release.');
          }

          const numSamplesAddress = await this._alignedAlloc(
            Int32Array.BYTES_PER_ELEMENT,
            Int32Array.BYTES_PER_ELEMENT,
          );
          if (numSamplesAddress === 0) {
            throw new OrcaErrors.OrcaOutOfMemoryError('malloc failed: Cannot allocate memory');
          }

          const pcmAddressAddress = await this._alignedAlloc(
            Int32Array.BYTES_PER_ELEMENT,
            Int32Array.BYTES_PER_ELEMENT,
          );
          if (pcmAddressAddress === 0) {
            throw new OrcaErrors.OrcaOutOfMemoryError('malloc failed: Cannot allocate memory');
          }

          const streamFlushStatus = await this._pvOrcaStreamFlush(
            this._streamAddress,
            numSamplesAddress,
            pcmAddressAddress,
          );

          const memoryBufferView = new DataView(this._wasmMemory.buffer);
          const memoryBufferUint8 = new Uint8Array(this._wasmMemory.buffer);

          if (streamFlushStatus !== PvStatus.SUCCESS) {
            const messageStack = await this._getMessageStack(
              this._pvGetErrorStack,
              this._pvFreeErrorStack,
              this._messageStackAddressAddressAddress,
              this._messageStackDepthAddress,
              memoryBufferView,
              memoryBufferUint8,
            );

            throw pvStatusToException(streamFlushStatus, 'Flush failed', messageStack);
          }

          const pcmAddress = memoryBufferView.getInt32(
            pcmAddressAddress,
            true,
          );
          await this._pvFree(pcmAddressAddress);

          const numSamples = memoryBufferView.getInt32(
            numSamplesAddress,
            true,
          );
          await this._pvFree(numSamplesAddress);

          const outputMemoryBuffer = new Int16Array(this._wasmMemory.buffer);
          const pcm = outputMemoryBuffer.slice(
            pcmAddress / Int16Array.BYTES_PER_ELEMENT,
            (pcmAddress / Int16Array.BYTES_PER_ELEMENT) + numSamples,
          );
          await this._pvOrcaPcmDelete(pcmAddress);

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
    await this._pvOrcaStreamClose(this._streamAddress);
  }
}

export type OrcaStream = Stream

/**
 * JavaScript/WebAssembly Binding for Orca
 */
export class Orca {
  private static _version: string;
  private static _sampleRate: number;
  private static _validCharacters: string[];
  private static _maxCharacterLimit: number;

  private _wasmMemory?: WebAssembly.Memory;
  private readonly _alignedAlloc: CallableFunction;
  private readonly _pvFree: pv_free_type;
  private readonly _pvGetErrorStack: pv_get_error_stack_type;
  private readonly _pvFreeErrorStack: pv_free_error_stack_type;
  private readonly _messageStackAddressAddressAddress: number;
  private readonly _messageStackDepthAddress: number;

  private readonly _objectAddress: number;
  private readonly _pvOrcaDelete: pv_orca_delete_type;
  private readonly _pvOrcaSynthesize: pv_orca_synthesize_type;
  private readonly _pvOrcaSynthesizeParamsInit: pv_orca_synthesize_params_init_type;
  private readonly _pvOrcaSynthesizeParamsDelete: pv_orca_synthesize_params_delete_type;
  private readonly _pvOrcaSynthesizeParamsSetSpeechRate: pv_orca_synthesize_params_set_speech_rate_type;
  private readonly _pvOrcaSynthesizeParamsSetRandomState: pv_orca_synthesize_params_set_random_state_type;
  private readonly _pvOrcaPcmDelete: pv_orca_pcm_delete_type;
  private readonly _pvOrcaWordAlignmentsDelete: pv_orca_word_alignments_delete_type;

  private readonly _streamPcmAddressAddress: number;
  private readonly _pvOrcaStreamOpen: pv_orca_stream_open_type;
  private readonly _pvOrcaStreamSynthesize: pv_orca_stream_synthesize_type;
  private readonly _pvOrcaStreamFlush: pv_orca_stream_flush_type;
  private readonly _pvOrcaStreamClose: pv_orca_stream_close_type;
  private readonly _functionMutex: Mutex;

  private static _wasm: string;
  private static _wasmSimd: string;
  private static _sdk: string = 'web';

  private static _orcaMutex = new Mutex();

  private constructor(handleWasm: OrcaWasmOutput) {
    Orca._version = handleWasm.version;
    Orca._sampleRate = handleWasm.sampleRate;
    Orca._validCharacters = handleWasm.validCharacters;
    Orca._maxCharacterLimit = handleWasm.maxCharacterLimit;

    this._wasmMemory = handleWasm.memory;
    this._alignedAlloc = handleWasm.alignedAlloc;
    this._pvFree = handleWasm.pvFree;
    this._pvGetErrorStack = handleWasm.pvGetErrorStack;
    this._pvFreeErrorStack = handleWasm.pvFreeErrorStack;
    this._messageStackAddressAddressAddress = handleWasm.messageStackAddressAddressAddress;
    this._messageStackDepthAddress = handleWasm.messageStackDepthAddress;

    this._objectAddress = handleWasm.objectAddress;
    this._pvOrcaDelete = handleWasm.pvOrcaDelete;
    this._pvOrcaSynthesize = handleWasm.pvOrcaSynthesize;
    this._pvOrcaSynthesizeParamsInit = handleWasm.pvOrcaSynthesizeParamsInit;
    this._pvOrcaSynthesizeParamsDelete = handleWasm.pvOrcaSynthesizeParamsDelete;
    this._pvOrcaSynthesizeParamsSetSpeechRate = handleWasm.pvOrcaSynthesizeParamsSetSpeechRate;
    this._pvOrcaSynthesizeParamsSetRandomState = handleWasm.pvOrcaSynthesizeParamsSetRandomState;
    this._pvOrcaPcmDelete = handleWasm.pvOrcaPcmDelete;
    this._pvOrcaWordAlignmentsDelete = handleWasm.pvOrcaWordAlignmentsDelete;

    this._streamPcmAddressAddress = handleWasm.streamPcmAddressAddress;
    this._pvOrcaStreamOpen = handleWasm.pvOrcaStreamOpen;
    this._pvOrcaStreamSynthesize = handleWasm.pvOrcaStreamSynthesize;
    this._pvOrcaStreamFlush = handleWasm.pvOrcaStreamFlush;
    this._pvOrcaStreamClose = handleWasm.pvOrcaStreamClose;

    this._functionMutex = new Mutex();
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
   * Get valid characters.
   */
  get validCharacters(): string[] {
    return Orca._validCharacters;
  }

  /**
   * Get maximum character limit.
   */
  get maxCharacterLimit(): number {
    return Orca._maxCharacterLimit;
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

  public static _init(
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
          const wasmOutput = await Orca.initWasm(accessKey.trim(), modelPath, (isSimd) ? this._wasmSimd : this._wasm);
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

    if (text.trim().length > Orca._maxCharacterLimit) {
      throw new OrcaErrors.OrcaInvalidArgumentError(`
        'text' length must be smaller than ${Orca._maxCharacterLimit}
      `);
    }

    return new Promise<OrcaSynthesizeResult>((resolve, reject) => {
      this._functionMutex
        .runExclusive(async () => {
          if (this._wasmMemory === undefined) {
            throw new OrcaErrors.OrcaInvalidStateError(
              'Attempted to call Orca synthesize after release.',
            );
          }

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

          let memoryBufferView = new DataView(this._wasmMemory.buffer);
          let memoryBufferUint8 = new Uint8Array(this._wasmMemory.buffer);

          const initStatus = await this._pvOrcaSynthesizeParamsInit(synthesizeParamsAddressAddress);
          if (initStatus !== PvStatus.SUCCESS) {
            const messageStack = await Orca.getMessageStack(
              this._pvGetErrorStack,
              this._pvFreeErrorStack,
              this._messageStackAddressAddressAddress,
              this._messageStackDepthAddress,
              memoryBufferView,
              memoryBufferUint8,
            );

            throw pvStatusToException(initStatus, 'Synthesize failed', messageStack);
          }

          const synthesizeParamsAddress = memoryBufferView.getInt32(synthesizeParamsAddressAddress, true);
          await this._pvFree(synthesizeParamsAddressAddress);

          if (synthesizeParams.speechRate !== null && synthesizeParams.speechRate !== undefined) {
            const setSpeechRateStatus = await this._pvOrcaSynthesizeParamsSetSpeechRate(
              synthesizeParamsAddress,
              synthesizeParams.speechRate,
            );
            if (setSpeechRateStatus !== PvStatus.SUCCESS) {
              const messageStack = await Orca.getMessageStack(
                this._pvGetErrorStack,
                this._pvFreeErrorStack,
                this._messageStackAddressAddressAddress,
                this._messageStackDepthAddress,
                memoryBufferView,
                memoryBufferUint8,
              );

              throw pvStatusToException(setSpeechRateStatus, 'Synthesize failed', messageStack);
            }
          }

          if (synthesizeParams.randomState !== null && synthesizeParams.randomState !== undefined) {
            const setRandomStateStatus = await this._pvOrcaSynthesizeParamsSetRandomState(
              synthesizeParamsAddress,
              BigInt(synthesizeParams.randomState),
            );
            if (setRandomStateStatus !== PvStatus.SUCCESS) {
              const messageStack = await Orca.getMessageStack(
                this._pvGetErrorStack,
                this._pvFreeErrorStack,
                this._messageStackAddressAddressAddress,
                this._messageStackDepthAddress,
                memoryBufferView,
                memoryBufferUint8,
              );
              throw pvStatusToException(setRandomStateStatus, 'Synthesize failed', messageStack);
            }
          }

          const numSamplesAddress = await this._alignedAlloc(
            Int32Array.BYTES_PER_ELEMENT,
            Int32Array.BYTES_PER_ELEMENT,
          );
          if (numSamplesAddress === 0) {
            throw new OrcaErrors.OrcaOutOfMemoryError('malloc failed: Cannot allocate memory');
          }

          const pcmAddressAddress = await this._alignedAlloc(
            Int32Array.BYTES_PER_ELEMENT,
            Int32Array.BYTES_PER_ELEMENT,
          );
          if (pcmAddressAddress === 0) {
            throw new OrcaErrors.OrcaOutOfMemoryError('malloc failed: Cannot allocate memory');
          }

          const numAlignmentsAddress = await this._alignedAlloc(
            Int32Array.BYTES_PER_ELEMENT,
            Int32Array.BYTES_PER_ELEMENT,
          );
          if (numAlignmentsAddress === 0) {
            throw new OrcaErrors.OrcaOutOfMemoryError('malloc failed: Cannot allocate memory');
          }

          const alignmentsAddressAddressAddress = await this._alignedAlloc(
            Int32Array.BYTES_PER_ELEMENT,
            Int32Array.BYTES_PER_ELEMENT,
          );
          if (alignmentsAddressAddressAddress === 0) {
            throw new OrcaErrors.OrcaOutOfMemoryError('malloc failed: Cannot allocate memory');
          }

          const synthesizeStatus = await this._pvOrcaSynthesize(
            this._objectAddress,
            textAddress,
            synthesizeParamsAddress,
            numSamplesAddress,
            pcmAddressAddress,
            numAlignmentsAddress,
            alignmentsAddressAddressAddress,
          );
          await this._pvFree(textAddress);
          await this._pvOrcaSynthesizeParamsDelete(synthesizeParamsAddress);

          memoryBufferView = new DataView(this._wasmMemory.buffer);
          memoryBufferUint8 = new Uint8Array(this._wasmMemory.buffer);

          if (synthesizeStatus !== PvStatus.SUCCESS) {
            const messageStack = await Orca.getMessageStack(
              this._pvGetErrorStack,
              this._pvFreeErrorStack,
              this._messageStackAddressAddressAddress,
              this._messageStackDepthAddress,
              memoryBufferView,
              memoryBufferUint8,
            );

            throw pvStatusToException(synthesizeStatus, 'Synthesize failed', messageStack);
          }

          const pcmAddress = memoryBufferView.getInt32(
            pcmAddressAddress,
            true,
          );
          await this._pvFree(pcmAddressAddress);

          const numSamples = memoryBufferView.getInt32(
            numSamplesAddress,
            true,
          );
          await this._pvFree(numSamplesAddress);

          const outputMemoryBuffer = new Int16Array(this._wasmMemory.buffer);
          const pcm = outputMemoryBuffer.slice(
            pcmAddress / Int16Array.BYTES_PER_ELEMENT,
            (pcmAddress / Int16Array.BYTES_PER_ELEMENT) + numSamples,
          );
          await this._pvOrcaPcmDelete(pcmAddress);

          const numAlignments = memoryBufferView.getInt32(numAlignmentsAddress, true);
          const alignmentsAddressAddress = memoryBufferView.getInt32(alignmentsAddressAddressAddress, true);

          let ptr = memoryBufferView.getInt32(alignmentsAddressAddress, true);
          const alignments: OrcaAlignment[] = [];
          for (let i = 1; i <= numAlignments; i++) {
            const wordAddress = memoryBufferView.getInt32(ptr, true);
            const word = arrayBufferToStringAtIndex(
              memoryBufferUint8,
              wordAddress,
            );
            ptr += Uint32Array.BYTES_PER_ELEMENT;
            const startSec = memoryBufferView.getFloat32(ptr, true);
            ptr += Float32Array.BYTES_PER_ELEMENT;
            const endSec = memoryBufferView.getFloat32(ptr, true);
            ptr += Float32Array.BYTES_PER_ELEMENT;
            const numPhonemes = memoryBufferView.getInt32(ptr, true);
            ptr += Uint32Array.BYTES_PER_ELEMENT;
            const phonemesAddress = memoryBufferView.getInt32(ptr, true);
            ptr = memoryBufferView.getInt32(alignmentsAddressAddress + (i * Uint32Array.BYTES_PER_ELEMENT), true);

            let phonemesPtr = memoryBufferView.getInt32(phonemesAddress, true);
            const phonemes: OrcaPhoneme[] = [];
            for (let j = 1; j <= numPhonemes; j++) {
              const phonemeAddress = memoryBufferView.getInt32(phonemesPtr, true);
              const phoneme = arrayBufferToStringAtIndex(
                memoryBufferUint8,
                phonemeAddress,
              );
              phonemesPtr += Uint32Array.BYTES_PER_ELEMENT;
              const pStartSec = memoryBufferView.getFloat32(phonemesPtr, true);
              phonemesPtr += Float32Array.BYTES_PER_ELEMENT;
              const pEndSec = memoryBufferView.getFloat32(phonemesPtr, true);
              phonemesPtr = memoryBufferView.getInt32(phonemesAddress + (j * Uint32Array.BYTES_PER_ELEMENT), true);
              phonemes.push({ phoneme, startSec: pStartSec, endSec: pEndSec });
            }
            alignments.push({ word, startSec, endSec, phonemes });
          }
          await this._pvFree(numAlignmentsAddress);
          await this._pvFree(alignmentsAddressAddressAddress);
          await this._pvOrcaWordAlignmentsDelete(numAlignments, alignmentsAddressAddress);

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
          if (this._wasmMemory === undefined) {
            throw new OrcaErrors.OrcaInvalidStateError('Attempted to call Orca stream open after release.');
          }

          const synthesizeParamsAddressAddress = await this._alignedAlloc(
            Int32Array.BYTES_PER_ELEMENT,
            Int32Array.BYTES_PER_ELEMENT,
          );
          if (synthesizeParamsAddressAddress === 0) {
            throw new OrcaErrors.OrcaOutOfMemoryError('malloc failed: Cannot allocate memory');
          }

          const memoryBufferView = new DataView(this._wasmMemory.buffer);
          const memoryBufferUint8 = new Uint8Array(this._wasmMemory.buffer);

          const initStatus = await this._pvOrcaSynthesizeParamsInit(synthesizeParamsAddressAddress);
          if (initStatus !== PvStatus.SUCCESS) {
            const messageStack = await Orca.getMessageStack(
              this._pvGetErrorStack,
              this._pvFreeErrorStack,
              this._messageStackAddressAddressAddress,
              this._messageStackDepthAddress,
              memoryBufferView,
              memoryBufferUint8,
            );

            throw pvStatusToException(initStatus, 'Stream open failed', messageStack);
          }

          const synthesizeParamsAddress = memoryBufferView.getInt32(synthesizeParamsAddressAddress, true);
          await this._pvFree(synthesizeParamsAddressAddress);

          if (synthesizeParams.speechRate !== null && synthesizeParams.speechRate !== undefined) {
            const setSpeechRateStatus = await this._pvOrcaSynthesizeParamsSetSpeechRate(synthesizeParamsAddress, synthesizeParams.speechRate);
            if (setSpeechRateStatus !== PvStatus.SUCCESS) {
              const messageStack = await Orca.getMessageStack(
                this._pvGetErrorStack,
                this._pvFreeErrorStack,
                this._messageStackAddressAddressAddress,
                this._messageStackDepthAddress,
                memoryBufferView,
                memoryBufferUint8,
              );

              throw pvStatusToException(setSpeechRateStatus, 'Stream open failed', messageStack);
            }
          }

          if (synthesizeParams.randomState !== null && synthesizeParams.randomState !== undefined) {
            const setRandomStateStatus = await this._pvOrcaSynthesizeParamsSetRandomState(synthesizeParamsAddress, BigInt(synthesizeParams.randomState));
            if (setRandomStateStatus !== PvStatus.SUCCESS) {
              const messageStack = await Orca.getMessageStack(
                this._pvGetErrorStack,
                this._pvFreeErrorStack,
                this._messageStackAddressAddressAddress,
                this._messageStackDepthAddress,
                memoryBufferView,
                memoryBufferUint8,
              );

              throw pvStatusToException(setRandomStateStatus, 'Stream open failed', messageStack);
            }
          }

          const streamAddressAddress = await this._alignedAlloc(
            Int32Array.BYTES_PER_ELEMENT,
            Int32Array.BYTES_PER_ELEMENT,
          );
          if (streamAddressAddress === 0) {
            throw new OrcaErrors.OrcaOutOfMemoryError('malloc failed: Cannot allocate memory');
          }

          const streamOpenStatus = await this._pvOrcaStreamOpen(
            this._objectAddress,
            synthesizeParamsAddress,
            streamAddressAddress,
          );
          await this._pvOrcaSynthesizeParamsDelete(synthesizeParamsAddress);

          if (streamOpenStatus !== PvStatus.SUCCESS) {
            const messageStack = await Orca.getMessageStack(
              this._pvGetErrorStack,
              this._pvFreeErrorStack,
              this._messageStackAddressAddressAddress,
              this._messageStackDepthAddress,
              memoryBufferView,
              memoryBufferUint8,
            );

            throw pvStatusToException(streamOpenStatus, 'Stream open failed', messageStack);
          }
          const streamAddress = memoryBufferView.getInt32(streamAddressAddress, true);
          await this._pvFree(streamAddressAddress);

          return new Stream(
            this._wasmMemory,
            this._alignedAlloc,
            this._pvFree,
            this._pvGetErrorStack,
            this._pvFreeErrorStack,
            this._messageStackAddressAddressAddress,
            this._messageStackDepthAddress,
            this._functionMutex,
            this._streamPcmAddressAddress,
            this._pvOrcaPcmDelete,
            this._pvOrcaStreamSynthesize,
            this._pvOrcaStreamFlush,
            this._pvOrcaStreamClose,
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
    await this._pvOrcaDelete(this._objectAddress);
    await this._pvFree(this._messageStackAddressAddressAddress);
    await this._pvFree(this._messageStackDepthAddress);
    await this._pvFree(this._streamPcmAddressAddress);
    delete this._wasmMemory;
    this._wasmMemory = undefined;
  }

  private static async initWasm(accessKey: string, modelPath: string, wasmBase64: string): Promise<OrcaWasmOutput> {
    // A WebAssembly page has a constant size of 64KiB. -> 1MiB ~= 16 pages
    const memory = new WebAssembly.Memory({ initial: 1600 });
    let memoryBufferUint8 = new Uint8Array(memory.buffer);
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
    const pv_orca_synthesize_params_set_random_state = exports.pv_orca_synthesize_params_set_random_state as pv_orca_synthesize_params_set_random_state_type;
    const pv_orca_synthesize = exports.pv_orca_synthesize as pv_orca_synthesize_type;
    const pv_orca_pcm_delete = exports.pv_orca_pcm_delete as pv_orca_pcm_delete_type;
    const pv_orca_word_alignments_delete = exports.pv_orca_word_alignments_delete as pv_orca_word_alignments_delete_type;
    const pv_orca_stream_open = exports.pv_orca_stream_open as pv_orca_stream_open_type;
    const pv_orca_stream_synthesize = exports.pv_orca_stream_synthesize as pv_orca_stream_synthesize_type;
    const pv_orca_stream_flush = exports.pv_orca_stream_flush as pv_orca_stream_flush_type;
    const pv_orca_stream_close = exports.pv_orca_stream_close as pv_orca_stream_close_type;
    const pv_orca_version = exports.pv_orca_version as pv_orca_version_type;
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

    const initStatus = await pv_orca_init(
      accessKeyAddress,
      modelPathAddress,
      objectAddressAddress);
    await pv_free(accessKeyAddress);
    await pv_free(modelPathAddress);

    const memoryBufferView = new DataView(memory.buffer);
    memoryBufferUint8 = new Uint8Array(memory.buffer);

    if (initStatus !== PvStatus.SUCCESS) {
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
    if (sampleRateStatus !== PvStatus.SUCCESS) {
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

    const maxCharacterLimitAddress = await aligned_alloc(
      Int32Array.BYTES_PER_ELEMENT,
      Int32Array.BYTES_PER_ELEMENT,
    );
    if (maxCharacterLimitAddress === 0) {
      throw new OrcaErrors.OrcaOutOfMemoryError('malloc failed: Cannot allocate memory');
    }
    const maxCharacterLimitStatus = await pv_orca_max_character_limit(objectAddress, maxCharacterLimitAddress);
    if (maxCharacterLimitStatus !== PvStatus.SUCCESS) {
      const messageStack = await Orca.getMessageStack(
        pv_get_error_stack,
        pv_free_error_stack,
        messageStackAddressAddressAddress,
        messageStackDepthAddress,
        memoryBufferView,
        memoryBufferUint8,
      );

      throw pvStatusToException(maxCharacterLimitStatus, 'Get max character limit failed', messageStack, pvError);
    }

    const maxCharacterLimit = memoryBufferView.getInt32(maxCharacterLimitAddress, true);
    await pv_free(maxCharacterLimitAddress);

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
    if (validCharactersStatus !== PvStatus.SUCCESS) {
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

    const versionAddress = await pv_orca_version();
    const version = arrayBufferToStringAtIndex(
      memoryBufferUint8,
      versionAddress,
    );

    const streamPcmAddressAddress = await aligned_alloc(
      Int32Array.BYTES_PER_ELEMENT,
      Int32Array.BYTES_PER_ELEMENT,
    );
    if (streamPcmAddressAddress === 0) {
      throw new OrcaErrors.OrcaOutOfMemoryError('malloc failed: Cannot allocate memory');
    }

    return {
      memory: memory,
      pvFree: pv_free,
      alignedAlloc: aligned_alloc,

      objectAddress: objectAddress,
      version: version,
      sampleRate: sampleRate,
      maxCharacterLimit: maxCharacterLimit,
      validCharacters: validCharacters,
      streamPcmAddressAddress: streamPcmAddressAddress,
      messageStackAddressAddressAddress: messageStackAddressAddressAddress,
      messageStackDepthAddress: messageStackDepthAddress,

      pvOrcaDelete: pv_orca_delete,
      pvOrcaSynthesizeParamsInit: pv_orca_synthesize_params_init,
      pvOrcaSynthesizeParamsDelete: pv_orca_synthesize_params_delete,
      pvOrcaSynthesizeParamsSetSpeechRate: pv_orca_synthesize_params_set_speech_rate,
      pvOrcaSynthesizeParamsSetRandomState: pv_orca_synthesize_params_set_random_state,
      pvOrcaSynthesize: pv_orca_synthesize,
      pvOrcaPcmDelete: pv_orca_pcm_delete,
      pvOrcaWordAlignmentsDelete: pv_orca_word_alignments_delete,
      pvOrcaStreamOpen: pv_orca_stream_open,
      pvOrcaStreamSynthesize: pv_orca_stream_synthesize,
      pvOrcaStreamFlush: pv_orca_stream_flush,
      pvOrcaStreamClose: pv_orca_stream_close,
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

    await pv_free_error_stack(messageStackAddressAddress);

    return messageStack;
  }
}
