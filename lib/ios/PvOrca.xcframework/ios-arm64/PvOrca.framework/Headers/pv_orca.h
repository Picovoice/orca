/*
    Copyright 2024-2025 Picovoice Inc.

    You may not use this file except in compliance with the license. A copy of the license is located in the "LICENSE"
    file accompanying this source.

    Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
    an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
    specific language governing permissions and limitations under the License.
*/

#ifndef PV_ORCA_H
#define PV_ORCA_H

#include <stdbool.h>
#include <stdint.h>

#include "picovoice.h"

#ifdef __cplusplus

extern "C" {

#endif

/**
 * Forward declaration for Orca text-to-speech engine. Orca converts text to spoken audio without network latency.
 * It has two modes of operation.
 *     1) Single synthesis: converts a given text to audio. Function `pv_orca_synthesize()` returns the raw audio data,
 *        function `pv_orca_synthesize_to_file()` saves the audio to a file.
 *     2) Streaming synthesis: Converts a stream of text to a stream of audio. An OrcaStream object can be opened with
 *        `pv_orca_stream_open()` and text chunks can be added with `pv_orca_stream_synthesize()`.
 *        The incoming text is buffered internally and only when enough context is available will an audio chunk
 *        be generated. When the text stream has concluded, the caller needs to use `pv_orca_stream_flush()`
 *        to generate the audio for the remaining buffer that has yet to be synthesized. The stream can be closed
 *        with `pv_orca_stream_close()`. Single synthesis functions cannot be called while a stream is open.
 */
typedef struct pv_orca pv_orca_t;

/**
 * Constructor.
 *
 * @param access_key AccessKey obtained from Picovoice Console (https://console.picovoice.ai/)
 * @param model_path Absolute path to the file containing Orca's model parameters.
 * @param device String representation of the device (e.g., CPU or GPU) to use. If set to `best`, the most
 * suitable device is selected automatically. If set to `gpu`, the engine uses the first available GPU device.
 * To select a specific GPU device, set this argument to `gpu:${GPU_INDEX}`, where `${GPU_INDEX}` is the index
 * of the target GPU. If set to `cpu`, the engine will run on the CPU with the default number of threads.
 * To specify the number of threads, set this argument to `cpu:${NUM_THREADS}`, where `${NUM_THREADS}` is the
 * desired number of threads.
 * @param[out] object Constructed instance of Orca.
 * @return Status code. Returns `PV_STATUS_OUT_OF_MEMORY`, `PV_STATUS_IO_ERROR`, `PV_STATUS_INVALID_ARGUMENT`,
 * `PV_STATUS_RUNTIME_ERROR`, `PV_STATUS_ACTIVATION_ERROR`, `PV_STATUS_ACTIVATION_LIMIT_REACHED`,
 * `PV_STATUS_ACTIVATION_THROTTLED`, or `PV_STATUS_ACTIVATION_REFUSED` on failure.
 */
PV_API pv_status_t pv_orca_init(
        const char *access_key,
        const char *model_path,
        const char *device,
        pv_orca_t **object);

/**
 * Destructor.
 *
 * @param object The Orca object.
 */
PV_API void pv_orca_delete(pv_orca_t *object);

/**
 * Returns an array of characters that are accepted as input to Orca synthesize functions.
 *
 * @param object Constructed instance of Orca.
 * @param[out] num_characters Number of valid characters.
 * @param[out] characters An array of valid characters for Orca.
 * @return Status code. Returns `PV_STATUS_INVALID_ARGUMENT` or `PV_STATUS_OUT_OF_MEMORY` on failure.
 */
PV_API pv_status_t pv_orca_valid_characters(
        const pv_orca_t *object,
        int32_t *num_characters,
        const char *const **characters);

/**
 * Deletes the characters previously created by `pv_orca_valid_characters()`.
 *
 * @param characters The characters returned from `pv_orca_valid_characters()`.
 */
PV_API void pv_orca_valid_characters_delete(const char *const *characters);

/**
 * Gets the sampling rate of the audio produced by Orca.
 *
 * @param object Constructed instance of Orca.
 * @param[out] sample_rate Sampling rate of the audio produced by Orca.
 * @return Status code. Returns `PV_STATUS_INVALID_ARGUMENT` on failure.
 */
PV_API pv_status_t pv_orca_sample_rate(const pv_orca_t *object, int32_t *sample_rate);

/**
 * Gets the maximum number of characters that can be synthesized at once.
 *
 * @param object Constructed instance of Orca.
 * @param[out] max_character_limit Maximum number of characters that can be synthesized at once.
 * @return Status code. Returns `PV_STATUS_INVALID_ARGUMENT` on failure.
 */
PV_API pv_status_t pv_orca_max_character_limit(const pv_orca_t *object, int32_t *max_character_limit);

/**
 * Forward declaration for pv_orca_synthesize_params object. This object can be parsed to Orca synthesize functions to
 * control the synthesized audio. An instance can be created with `pv_orca_synthesize_params_init()` and deleted with
 * `pv_orca_synthesize_params_delete()`. The object's properties can be set with `pv_orca_synthesize_params_set_*()`
 * and returned with `pv_orca_synthesize_params_get_*()`.
 */
typedef struct pv_orca_synthesize_params pv_orca_synthesize_params_t;

/**
 * Constructor for the pv_orca_synthesize_params object.
 *
 * @param[out] object Constructed instance of pv_orca_synthesize_params.
 * @return Status code. Returns `PV_STATUS_INVALID_ARGUMENT` or `PV_STATUS_OUT_OF_MEMORY`  on failure.
 */
PV_API pv_status_t pv_orca_synthesize_params_init(pv_orca_synthesize_params_t **object);

/**
 * Destructor for the pv_orca_synthesize_params object.
 *
 * @param object The pv_orca_synthesize_params object.
*/
PV_API void pv_orca_synthesize_params_delete(pv_orca_synthesize_params_t *object);

/**
 * Setter for the speech rate.
 *
 * @param object Constructed instance of pv_orca_synthesize_params.
 * @param speech_rate The pace of the speech. Valid values are within [0.7, 1.3].
 * @return Returns `PV_STATUS_INVALID_ARGUMENT` on failure.
 */
PV_API pv_status_t pv_orca_synthesize_params_set_speech_rate(
        pv_orca_synthesize_params_t *object,
        float speech_rate);

/**
 * Getter for the speech rate.
 *
 * @param object Constructed instance of pv_orca_synthesize_params.
 * @param[out] speech_rate The pace of the speech.
 * @return Returns `PV_STATUS_INVALID_ARGUMENT` on failure.
 */
PV_API pv_status_t pv_orca_synthesize_params_get_speech_rate(
        const pv_orca_synthesize_params_t *object,
        float *speech_rate);

/**
 * Setter for the random state used in synthesize functions.
 *
 * @param object Constructed instance of pv_orca_synthesize_params.
 * @param random_state The random state used in synthesize functions.
 * @return Returns `PV_STATUS_INVALID_ARGUMENT` on failure.
 */
PV_API pv_status_t pv_orca_synthesize_params_set_random_state(
        pv_orca_synthesize_params_t *object,
        int64_t random_state);

/**
 * Getter for random state used in synthesize functions. If no state has been set via
 * `pv_orca_synthesize_params_set_random_state()`, the default value of the state is -1, which means a
 * random state is used in the synthesize functions.
 *
 * @param object Constructed instance of pv_orca_synthesize_params.
 * @param[out] random_state The random state used in synthesize functions.
 * @return Returns `PV_STATUS_INVALID_ARGUMENT` on failure.
 */
PV_API pv_status_t pv_orca_synthesize_params_get_random_state(
        const pv_orca_synthesize_params_t *object,
        int64_t *random_state);

/**
 * A synthesized phoneme and its associated metadata.
 */
typedef struct {
    char *phoneme; /** Synthesized phoneme. */
    float start_sec; /** Start of phoneme in seconds. */
    float end_sec; /** End of phoneme in seconds. */
} pv_orca_phoneme_alignment_t;

/**
 * A synthesized word and its associated metadata.
 */
typedef struct {
    char *word; /** Synthesized word. */
    float start_sec; /** Start of word in seconds. */
    float end_sec; /** End of word in seconds. */

    int32_t num_phonemes; /** Number of phonemes in the word. */
    pv_orca_phoneme_alignment_t **phonemes; /** Array of phonemes in the word. */
} pv_orca_word_alignment_t;

/**
 * Generates audio from text. The returned audio contains the speech representation of the text.
 * This function returns `PV_STATUS_INVALID_STATE` if an OrcaStream object is open.
 * The memory of the returned audio and the alignment metadata is allocated by Orca and can be deleted with
 * `pv_orca_pcm_delete()` and `pv_orca_word_alignments_delete()`, respectively.
 *
 * @param object The Orca object.
 * @param text Text to be converted to audio. The maximum length can be attained by calling
 * `pv_orca_max_character_limit()`. Allowed characters can be retrieved by calling `pv_orca_valid_characters()`.
 * Custom pronunciations can be embedded in the text via the syntax `{word|pronunciation}`.
 * The pronunciation is expressed in ARPAbet format, e.g.: `I {liv|L IH V} in {Sevilla|S EH V IY Y AH}`.
 * @param synthesize_params Global parameters for synthesized text. See 'pv_orca_synthesize_params_t' for details.
 * @param[out] num_samples The length of the pcm.
 * @param[out] pcm The output audio.
 * @param[out] num_alignments Number of returned alignments.
 * @param[out] alignments Alignments of synthesized words, phonemes, and their associated metadata.
 * @return Status code. Returns `PV_STATUS_INVALID_ARGUMENT` or `PV_STATUS_OUT_OF_MEMORY`,
 * `PV_STATUS_RUNTIME_ERROR`, `PV_STATUS_ACTIVATION_ERROR`, `PV_STATUS_ACTIVATION_LIMIT_REACHED`,
 * `PV_STATUS_ACTIVATION_THROTTLED`, or `PV_STATUS_ACTIVATION_REFUSED` on failure.
 * Returns `PV_STATUS_INVALID_STATE` if an OrcaStream object is open.
 */
PV_API pv_status_t pv_orca_synthesize(
        const pv_orca_t *object,
        const char *text,
        const pv_orca_synthesize_params_t *synthesize_params,
        int32_t *num_samples,
        int16_t **pcm,
        int32_t *num_alignments,
        pv_orca_word_alignment_t ***alignments);

/**
 * Generates audio from text and saves it to a file. The file contains the speech representation of the text.
 * This function returns `PV_STATUS_INVALID_STATE` if an OrcaStream object is open.
 * The memory of the returned alignment metadata is allocated by Orca and can be deleted with
 * `pv_orca_word_alignments_delete()`.
 *
 * @param object The Orca object.
 * @param text Text to be converted to audio. The maximum length can be attained by calling
 * `pv_orca_max_character_limit()`. Allowed characters can be retrieved by calling `pv_orca_valid_characters()`.
 * Custom pronunciations can be embedded in the text via the syntax `{word|pronunciation}`.
 * The pronunciation is expressed in ARPAbet format, e.g.: `I {liv|L IH V} in {Sevilla|S EH V IY Y AH}`.
 * @param synthesize_params Global parameters for synthesized text. See 'pv_orca_synthesize_params_t' for details.
 * @param output_path Absolute path to the output audio file. The output file is saved as `WAV (.wav)`
 * and consists of a single mono channel.
 * @param[out] num_alignments Number of returned alignments.
 * @param[out] alignments Alignments of synthesized words, phonemes, and their associated metadata.
 * @return Status code. Returns `PV_STATUS_INVALID_ARGUMENT` or `PV_STATUS_OUT_OF_MEMORY`,
 * `PV_STATUS_RUNTIME_ERROR`, `PV_STATUS_ACTIVATION_ERROR`, `PV_STATUS_ACTIVATION_LIMIT_REACHED`,
 * `PV_STATUS_ACTIVATION_THROTTLED`, or `PV_STATUS_ACTIVATION_REFUSED` on failure.
 * Returns `PV_STATUS_INVALID_STATE` if an OrcaStream object is open.
 */
PV_API pv_status_t pv_orca_synthesize_to_file(
        const pv_orca_t *object,
        const char *text,
        const pv_orca_synthesize_params_t *synthesize_params,
        const char *output_path,
        int32_t *num_alignments,
        pv_orca_word_alignment_t ***alignments);

/**
 * Forward declaration for OrcaStream object for converting a text stream into a spoken audio stream.
 */
typedef struct pv_orca_stream pv_orca_stream_t;

/**
 * Opens a new OrcaStream object.
 *
 * @param object The Orca object.
 * @param synthesize_params Global parameters for synthesized text. See 'pv_orca_synthesize_params_t' for details.
 * @param[out] stream The OrcaStream object.
 * @return Status code. Returns `PV_STATUS_INVALID_ARGUMENT` or `PV_STATUS_OUT_OF_MEMORY` on failure.
  */
PV_API pv_status_t pv_orca_stream_open(
        pv_orca_t *object,
        const pv_orca_synthesize_params_t *synthesize_params,
        pv_orca_stream_t **stream);

/**
 * Adds a chunk of text to the OrcaStream object and generates audio if enough text has been added.
 * This function is expected to be called multiple times with consecutive chunks of text from a text stream.
 * The incoming text is buffered as it arrives until there is enough context to convert a chunk of the buffered
 * text into audio. The caller needs to use `pv_orca_stream_flush()` to generate the audio chunk for the remaining
 * text that has not yet been synthesized.
 * The caller is responsible for deleting the generated audio with `pv_orca_pcm_delete()`.
 *
 * @param object The OrcaStream object.
 * @param text A chunk of text from a text input stream. Characters not supported by Orca will be ignored.
 * Valid characters can be retrieved by calling `pv_orca_valid_characters()`.
 * Custom pronunciations can be embedded in the text via the syntax `{word|pronunciation}`. They need to be
 * added in a single call to this function. The pronunciation is expressed in ARPAbet format,
 * e.g.: `I {liv|L IH V} in {Sevilla|S EH V IY Y AH}`.
 * @param[out] num_samples The length of the pcm produced, `0` if no audio chunk has been produced.
 * @param[out] pcm The output audio chunk, `NULL` if no audio chunk has been produced.
 * @return Status code. Returns `PV_STATUS_INVALID_ARGUMENT`, `PV_STATUS_OUT_OF_MEMORY`,
 * `PV_STATUS_RUNTIME_ERROR`, `PV_STATUS_ACTIVATION_ERROR`, `PV_STATUS_ACTIVATION_LIMIT_REACHED`,
 * `PV_STATUS_ACTIVATION_THROTTLED`, `PV_STATUS_ACTIVATION_REFUSED`, or `PV_STATUS_INVALID_STATE` on failure.
 */
PV_API pv_status_t pv_orca_stream_synthesize(
        pv_orca_stream_t *object,
        const char *text,
        int32_t *num_samples,
        int16_t **pcm);

/**
 * Generates audio for all of the buffered text that was added to the OrcaStream object
 * via `pv_orca_stream_synthesize()`.
 * The caller is responsible for deleting the generated audio with `pv_orca_pcm_delete()`.
 *
 * @param object The OrcaStream object.
 * @param[out] num_samples The length of the pcm, `0` if no audio chunk has been produced.
 * @param[out] pcm The output audio, `NULL` if no audio chunk has been produced.
 * @return Status code. Returns `PV_STATUS_INVALID_ARGUMENT`, `PV_STATUS_OUT_OF_MEMORY`,
 * `PV_STATUS_RUNTIME_ERROR`, `PV_STATUS_ACTIVATION_ERROR`, `PV_STATUS_ACTIVATION_LIMIT_REACHED`,
 * `PV_STATUS_ACTIVATION_THROTTLED`, `PV_STATUS_ACTIVATION_REFUSED`, or `PV_STATUS_INVALID_STATE` on failure.
 */
PV_API pv_status_t pv_orca_stream_flush(
        pv_orca_stream_t *object,
        int32_t *num_samples,
        int16_t **pcm);

/**
 * Deletes the OrcaStream object.
 *
 * @param object The OrcaStream object.
 */
PV_API void pv_orca_stream_close(pv_orca_stream_t *object);

/**
 * Deletes the audio previously generated by the Orca synthesize functions.
 *
 * @param object The pcm generated by Orca synthesize functions.
 */
PV_API void pv_orca_pcm_delete(int16_t *pcm);

/**
 * Deletes word alignments returned from Orca synthesize functions.
 *
 * @param num_alignments Number of alignments.
 * @param alignments Alignments returned from Orca synthesize functions.
 * @return Status code. Returns `PV_STATUS_INVALID_ARGUMENT` on failure.
 */
PV_API pv_status_t pv_orca_word_alignments_delete(
        int32_t num_alignments,
        pv_orca_word_alignment_t **alignments);

/**
 * Getter for version.
 *
 * @return Version.
 */
PV_API const char *pv_orca_version(void);

/**
* Gets a list of hardware devices that can be specified when calling `pv_orca_init`
*
* @param[out] hardware_devices Array of available hardware devices. Devices are NULL terminated strings.
*                              The array must be freed using `pv_orca_free_hardware_devices`.
* @param[out] num_hardware_devices The number of devices in the `hardware_devices` array.
* @return Status code. Returns `PV_STATUS_OUT_OF_MEMORY`, `PV_STATUS_INVALID_ARGUMENT`, `PV_STATUS_INVALID_STATE`,
* `PV_STATUS_RUNTIME_ERROR`, `PV_STATUS_ACTIVATION_ERROR`, `PV_STATUS_ACTIVATION_LIMIT_REACHED`,
* `PV_STATUS_ACTIVATION_THROTTLED`, or `PV_STATUS_ACTIVATION_REFUSED` on failure.
*/
PV_API pv_status_t pv_orca_list_hardware_devices(
    char ***hardware_devices,
    int32_t *num_hardware_devices);

/**
* Frees memory allocated by `pv_orca_list_hardware_devices`.
*
* @param[out] hardware_devices Array of available hardware devices allocated by `pv_orca_list_hardware_devices`.
* @param[out] num_hardware_devices The number of devices in the `hardware_devices` array.
*/
PV_API void pv_orca_free_hardware_devices(
    char **hardware_devices,
    int32_t num_hardware_devices);

#ifdef __cplusplus
}

#endif

#endif // PV_ORCA_H
