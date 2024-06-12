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

import * as fs from 'fs';
import * as path from 'path';

import PvStatus from './pv_status_t';
import {
  OrcaInvalidArgumentError,
  OrcaInvalidStateError,
  pvStatusToException,
} from './errors';

import {
  OrcaOptions,
  OrcaSynthesizeParams,
  OrcaSynthesizeResult,
  OrcaSynthesizeToFileResult,
  OrcaStreamSynthesizeResult,
} from './types';

import { getSystemLibraryPath } from './platforms';

const DEFAULT_MODEL_PATH = '../lib/common/orca_params_female.pv';

type OrcaHandleAndStatus = { handle: any; status: PvStatus };
type OrcaResult = OrcaSynthesizeResult & {
  status: PvStatus;
};
type OrcaStreamResult = {
  status: PvStatus;
  pcm: Int16Array;
};

class Stream {
  private readonly _stream: any;
  private readonly _streamSynthesize: any;
  private readonly _streamFlush: any;
  private readonly _streamCloseFunction: any;
  private readonly _handlePvStatus: any;

  constructor(
    stream: any,
    streamSynthesize: any,
    streamFlush: any,
    streamCloseFunction: any,
    handlePvStatus: any,
  ) {
    this._stream = stream;
    this._streamSynthesize = streamSynthesize;
    this._streamFlush = streamFlush;
    this._streamCloseFunction = streamCloseFunction;
    this._handlePvStatus = handlePvStatus;
  }

  /**
   * Adds a chunk of text to the Stream object and generates audio if enough text has been added.
   * This function is expected to be called multiple times with consecutive chunks of text from a text stream.
   * The incoming text is buffered as it arrives until there is enough context to convert a chunk of the
   * buffered text into audio. The caller needs to use `OrcaStream.flush()` to generate the audio chunk
   * for the remaining text that has not yet been synthesized.
   *
   * @param {string} text A chunk of text from a text input stream, comprised of valid characters.
   * Valid characters can be retrieved by calling `validCharacters`.
   * Custom pronunciations can be embedded in the text via the syntax `{word|pronunciation}`.
   * They need to be added in a single call to this function.
   * The pronunciation is expressed in ARPAbet format, e.g.: `I {liv|L IH V} in {Sevilla|S EH V IY Y AH}`.
   * @returns {OrcaStreamSynthesizeResult} The generated audio as a sequence of 16-bit linearly-encoded integers,
   * `null` if no audio chunk has been produced.
   */
  synthesize(text: string): OrcaStreamSynthesizeResult {
    if (text === undefined || text === null) {
      throw new OrcaInvalidArgumentError(
        `Text provided to 'Orca.synthesize()' is undefined or null`,
      );
    }

    let orcaResult: OrcaStreamResult | null = null;
    try {
      orcaResult = this._streamSynthesize(this._stream, text);
    } catch (err: any) {
      pvStatusToException(<PvStatus>err.code, err);
    }

    const status = orcaResult!.status;
    if (status !== PvStatus.SUCCESS) {
      this._handlePvStatus(status, 'Orca failed to synthesize streaming speech');
    }

    return orcaResult!.pcm.length > 0 ? orcaResult!.pcm : null;
  }

  /**
   * Generates audio for all the buffered text that was added to the OrcaStream object via `OrcaStream.synthesize()`.
   * @returns {OrcaStreamSynthesizeResult} The generated audio as a sequence of 16-bit linearly-encoded integers,
   * `null` if no audio chunk has been produced.
   */
  flush(): OrcaStreamSynthesizeResult {
    let orcaResult: OrcaStreamResult | null = null;
    try {
      orcaResult = this._streamFlush(
        this._stream,
      );
    } catch (err: any) {
      pvStatusToException(<PvStatus>err.code, err);
    }

    const status = orcaResult!.status;
    if (status !== PvStatus.SUCCESS) {
      this._handlePvStatus(status, 'Orca failed to flush streaming speech');
    }

    return orcaResult!.pcm.length > 0 ? orcaResult!.pcm : null;
  }

  /**
   * Releases the resources acquired by the OrcaStream object.
   */
  close(): void {
    this._streamCloseFunction(this._stream);
  }
}

/**
 * OrcaStream object that converts a stream of text to a stream of audio.
 */
export type OrcaStream = Stream;

/**
 * Node.js binding for Orca streaming text-to-speech engine.
 *
 * Performs the calls to the Orca node library. Does some basic parameter validation to prevent
 * errors occurring in the library layer. Provides clearer error messages in native JavaScript.
 */
export class Orca {
  private _pvOrca: any;

  private _handle: any;

  private readonly _version: string;
  private readonly _sampleRate: number;
  private readonly _validCharacters: string[];
  private readonly _maxCharacterLimit: number;

  /**
   * Creates an instance of Orca.
   * @param {string} accessKey AccessKey obtained from Picovoice Console (https://console.picovoice.ai/).
   * @param {OrcaOptions} options Optional configuration arguments.
   * @param {string} options.modelPath The path to save and use the model from (.pv extension)
   * @param {string} options.libraryPath the path to the Orca dynamic library (.node extension)
   */
  constructor(accessKey: string, options: OrcaOptions = {}) {
    if (
      accessKey === null ||
      accessKey === undefined ||
      accessKey.length === 0
    ) {
      throw new OrcaInvalidArgumentError(`No AccessKey provided to Orca`);
    }

    const {
      modelPath = path.resolve(__dirname, DEFAULT_MODEL_PATH),
      libraryPath = getSystemLibraryPath(),
    } = options;

    if (!fs.existsSync(libraryPath)) {
      throw new OrcaInvalidArgumentError(
        `File not found at 'libraryPath': ${libraryPath}`,
      );
    }

    if (!fs.existsSync(modelPath)) {
      throw new OrcaInvalidArgumentError(
        `File not found at 'modelPath': ${modelPath}`,
      );
    }

    const pvOrca = require(libraryPath); // eslint-disable-line
    this._pvOrca = pvOrca;

    let orcaHandleAndStatus: OrcaHandleAndStatus | null = null;
    try {
      pvOrca.set_sdk('nodejs');

      orcaHandleAndStatus = pvOrca.init(accessKey, modelPath);
    } catch (err: any) {
      pvStatusToException(<PvStatus>err.code, err);
    }

    let status = orcaHandleAndStatus!.status;
    if (status !== PvStatus.SUCCESS) {
      this.handlePvStatus(status, 'Orca failed to initialize');
    }

    const sampleRate = pvOrca.sample_rate(orcaHandleAndStatus!.handle);
    status = sampleRate.status;
    if (status !== PvStatus.SUCCESS) {
      this.handlePvStatus(status, 'Orca failed to initialize');
    }

    const validCharacters = pvOrca.valid_characters(orcaHandleAndStatus!.handle);
    status = validCharacters.status;
    if (status !== PvStatus.SUCCESS) {
      this.handlePvStatus(status, 'Orca failed to initialize');
    }

    const maxCharacterLimit = pvOrca.max_character_limit(orcaHandleAndStatus!.handle);
    status = maxCharacterLimit.status;
    if (status !== PvStatus.SUCCESS) {
      this.handlePvStatus(status, 'Orca failed to initialize');
    }

    this._handle = orcaHandleAndStatus!.handle;
    this._version = pvOrca.version();
    this._sampleRate = sampleRate.sample_rate;
    this._validCharacters = validCharacters.characters;
    this._maxCharacterLimit = maxCharacterLimit.max_character_limit;
  }

  /**
   * @returns the version of the Orca engine
   */
  get version(): string {
    return this._version;
  }

  /**
   * @returns the sample rate of the produced audio
   */
  get sampleRate(): number {
    return this._sampleRate;
  }

  /**
   * @returns the valid characters accepted as input to the synthesize functions
   */
  get validCharacters(): string[] {
    return this._validCharacters;
  }

  /**
   * @returns the maximum number of characters that can be synthesized at once
   */
  get maxCharacterLimit(): number {
    return this._maxCharacterLimit;
  }

  private getSynthesizeParams(synthesizeParams: OrcaSynthesizeParams): any {
    let synthesizeParamsResult = null;
    try {
      synthesizeParamsResult = this._pvOrca.synthesize_params_init();
      if (synthesizeParams.speechRate !== null && synthesizeParams.speechRate !== undefined) {
        this._pvOrca.synthesize_params_set_speech_rate(
          synthesizeParamsResult.synthesize_params,
          synthesizeParams.speechRate,
        );
      }
      if (synthesizeParams.randomState !== null && synthesizeParams.randomState !== undefined) {
        this._pvOrca.synthesize_params_set_random_state(
          synthesizeParamsResult.synthesize_params,
          synthesizeParams.randomState,
        );
      }
    } catch (err: any) {
      pvStatusToException(<PvStatus>err.code, err);
    }

    const status = synthesizeParamsResult!.status;
    if (status !== PvStatus.SUCCESS) {
      this.handlePvStatus(status, 'Orca failed to set synthesize params');
    }

    return synthesizeParamsResult!.synthesize_params;
  }

  /**
   * Generates audio from text. The returned audio contains the speech representation of the text.
   * The maximum number of characters per call to `.synthesize()` is `.maxCharacterLimit`.
   * Allowed characters are lower-case and upper-case letters and punctuation marks that can be retrieved with `.validCharacters`.
   * Custom pronunciations can be embedded in the text via the syntax `{word|pronunciation}`.
   * The pronunciation is expressed in ARPAbet format, e.g.: "I {live|L IH V} in {Sevilla|S EH V IY Y AH}".
   *
   * @param {string} text A string of text.
   * @param {OrcaSynthesizeParams} synthesizeParams Optional configuration arguments.
   * @param {number} synthesizeParams.speechRate Configure the rate of speech of the synthesized speech.
   * @param {number} synthesizeParams.randomState Configure the random seed for the synthesized speech.
   * @returns {OrcaSynthesizeResult} object which contains the generated audio as a sequence of 16-bit linearly-encoded integers
   * and a sequence of OrcaAlignment objects representing the word alignments.
   */
  synthesize(text: string, synthesizeParams: OrcaSynthesizeParams = {
    speechRate: 1.0,
    randomState: null,
  }): OrcaSynthesizeResult {
    if (
      this._handle === 0 ||
      this._handle === null ||
      this._handle === undefined
    ) {
      throw new OrcaInvalidStateError('Orca is not initialized');
    }

    if (text === undefined || text === null) {
      throw new OrcaInvalidArgumentError(
        `Text provided to 'Orca.synthesize()' is undefined or null`,
      );
    } else if (text.length === 0) {
      throw new OrcaInvalidArgumentError(
        `Text provided to 'Orca.synthesize()' is an empty string`,
      );
    }

    let orcaResult: OrcaResult | null = null;
    try {
      const synthesizeParamsObj = this.getSynthesizeParams(synthesizeParams);
      orcaResult = this._pvOrca.synthesize(
        this._handle,
        text,
        synthesizeParamsObj,
      );
      this._pvOrca.synthesize_params_delete(synthesizeParamsObj);
    } catch (err: any) {
      pvStatusToException(<PvStatus>err.code, err);
    }

    const status = orcaResult!.status;
    if (status !== PvStatus.SUCCESS) {
      this.handlePvStatus(status, 'Orca failed to synthesize speech');
    }

    return {
      pcm: orcaResult!.pcm,
      alignments: orcaResult!.alignments,
    };
  }

  /**
   * Generates audio from text and saves it to a file. The file contains the speech representation of the text.
   * The maximum number of characters per call to `.synthesize()` is `.maxCharacterLimit`.
   * Allowed characters are lower-case and upper-case letters and punctuation marks that can be retrieved with `.validCharacters`.
   * Custom pronunciations can be embedded in the text via the syntax `{word|pronunciation}`.
   * The pronunciation is expressed in ARPAbet format, e.g.: "I {live|L IH V} in {Sevilla|S EH V IY Y AH}".
   *
   * @param {string} text A string of text.
   * @param {string} outputPath Absolute path to save the generated audio as a single-channel 16-bit PCM WAV file.
   * @param {OrcaSynthesizeParams} synthesizeParams Optional configuration arguments.
   * @param {number} synthesizeParams.speechRate Configure the rate of speech of the synthesized speech.
   * @param {number} synthesizeParams.randomState Configure the random seed for the synthesized speech.
   * @returns {OrcaSynthesizeToFileResult} object which contains a sequence of OrcaAlignment objects representing the word alignments.
   */
  synthesizeToFile(text: string, outputPath: string, synthesizeParams: OrcaSynthesizeParams = {
    speechRate: 1.0,
    randomState: null,
  }): OrcaSynthesizeToFileResult {
    if (
      this._handle === 0 ||
      this._handle === null ||
      this._handle === undefined
    ) {
      throw new OrcaInvalidStateError('Orca is not initialized');
    }

    if (text === undefined || text === null) {
      throw new OrcaInvalidArgumentError(
        `Text provided to 'Orca.synthesizeToFile()' is undefined or null`,
      );
    } else if (text.trim().length === 0) {
      throw new OrcaInvalidArgumentError(
        `Text provided to 'Orca.synthesizeToFile()' is an empty string`,
      );
    }

    let orcaResult: OrcaResult | null = null;
    try {
      const synthesizeParamsObj = this.getSynthesizeParams(synthesizeParams);
      orcaResult = this._pvOrca.synthesize_to_file(
        this._handle,
        text,
        synthesizeParamsObj,
        outputPath,
      );
      this._pvOrca.synthesize_params_delete(synthesizeParamsObj);
    } catch (err: any) {
      pvStatusToException(<PvStatus>err.code, err);
    }

    const status = orcaResult!.status;
    if (status !== PvStatus.SUCCESS) {
      this.handlePvStatus(status, 'Orca failed to synthesize speech');
    }

    return orcaResult!.alignments;
  }

  /**
   * Opens a stream for streaming text synthesis.
   *
   * @param synthesizeParams Optional configuration arguments.
   * @param synthesizeParams.speechRate Configure the rate of speech of the synthesized speech.
   * @param synthesizeParams.randomState Configure the random seed for the synthesized speech.
   * @returns An instance of OrcaStream.
   */
  streamOpen(synthesizeParams: OrcaSynthesizeParams = {
    speechRate: 1.0,
    randomState: null,
  }): OrcaStream {
    if (
      this._handle === 0 ||
      this._handle === null ||
      this._handle === undefined
    ) {
      throw new OrcaInvalidStateError('Orca is not initialized');
    }

    let orcaResult: any | null = null;
    try {
      const synthesizeParamsObj = this.getSynthesizeParams(synthesizeParams);
      orcaResult = this._pvOrca.stream_open(
        this._handle,
        synthesizeParamsObj,
      );
      this._pvOrca.synthesize_params_delete(synthesizeParamsObj);
    } catch (err: any) {
      pvStatusToException(<PvStatus>err.code, err);
    }

    const status = orcaResult!.status;
    if (status !== PvStatus.SUCCESS) {
      this.handlePvStatus(status, 'Orca failed to open stream');
    }

    return new Stream(
      orcaResult!.stream,
      this._pvOrca.stream_synthesize,
      this._pvOrca.stream_flush,
      this._pvOrca.stream_close,
      this.handlePvStatus,
    );
  }

  /**
   * Releases the resources acquired by Orca.
   *
   * Be sure to call this when finished with the instance
   * to reclaim the memory that was allocated by the C library.
   */
  release(): void {
    if (this._handle !== 0) {
      try {
        this._pvOrca.delete(this._handle);
      } catch (err: any) {
        pvStatusToException(<PvStatus>err.code, err);
      }
      this._handle = 0;
    } else {
      // eslint-disable-next-line no-console
      console.warn('Orca is not initialized');
    }
  }

  private handlePvStatus(status: PvStatus, message: string): void {
    const errorObject = this._pvOrca.get_error_stack();
    if (errorObject.status === PvStatus.SUCCESS) {
      pvStatusToException(status, message, errorObject.message_stack);
    } else {
      pvStatusToException(status, 'Unable to get Orca error state');
    }
  }
}
