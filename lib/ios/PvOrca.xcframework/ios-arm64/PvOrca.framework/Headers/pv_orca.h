/*
    Copyright 2024 Picovoice Inc.

    You may not use this file except in compliance with the license. A copy of the license is located in the "LICENSE"
    file accompanying this source.

    Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on
    an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the
    specific language governing permissions and limitations under the License.
*/

#ifndef PV_ORCA_H
#define PV_ORCA_H

#include <stdint.h>

#include "picovoice.h"

#ifdef __cplusplus

extern "C" {

#endif

/**
 * Forward declaration for Orca text-to-speech engine. Orca converts text to spoken audio without network latency.
 */
typedef struct pv_orca pv_orca_t;

/**
 * Constructor.
 *
 * @param access_key AccessKey obtained from Picovoice Console (https://console.picovoice.ai/)
 * @param model_path Absolute path to the file containing Orca's model parameters.
 * @param[out] object Constructed instance of Orca.
 * @return A status code indicating the result of the initialization. Possible values include:
 *         - `PV_STATUS_OUT_OF_MEMORY`: Memory allocation failure.
 *         - `PV_STATUS_IO_ERROR`: Input/output error.
 *         - `PV_STATUS_INVALID_ARGUMENT`: Invalid input argument.
 *         - `PV_STATUS_RUNTIME_ERROR`: Error during runtime.
 *         - `PV_STATUS_ACTIVATION_ERROR`: Activation-related error.
 *         - `PV_STATUS_ACTIVATION_LIMIT_REACHED`: Activation limit reached.
 *         - `PV_STATUS_ACTIVATION_THROTTLED`: Activation throttled.
 *         - `PV_STATUS_ACTIVATION_REFUSED`: Activation refused.
 */
PV_API pv_status_t pv_orca_init(
        const char *access_key,
        const char *model_path,
        pv_orca_t **object);

/**
 * Destructor.
 *
 * @param object The Orca object.
 */
PV_API void pv_orca_delete(pv_orca_t *object);

/**
 * Gets an array of characters that are accepted as input to Orca synthesize functions.
 *
 * @param object Constructed instance of Orca.
 * @param[out] num_characters Number of valid characters.
 * @param[out] characters An array of valid characters for Orca.
 * @return Status code. Returns `PV_STATUS_INVALID_ARGUMENT` on failure.
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
 * @return Maximum character limit
 */
PV_API int32_t pv_orca_max_character_limit(void);

/**
 * Forward declaration for pv_orca_synthesize_params object. This object can be parsed to Orca synthesize functions to
 * control the synthesized audio. An instance can be created with `pv_orca_synthesize_params_init` and deleted with
 * `pv_orca_synthesize_params_delete`. The object's properties can be set with `pv_orca_synthesize_params_set_*`
 * and returned with `pv_orca_synthesize_params_get_*`.
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
 * Generates audio from text. The returned audio contains the speech representation of the text.
 * The memory of the returned audio is allocated by Orca and can be deleted with `pv_orca_delete_pcm`
 *
 * @param object The Orca object.
 * @param text Text to be converted to audio. The maximum length can be attained by calling
 * `pv_orca_max_character_limit()`. Allowed characters can be retrieved by calling `pv_orca_valid_characters()`.
 * Custom pronunciations can be embedded in the text via the syntax `{word|pronunciation}`.
 * The pronunciation is expressed in ARPAbet format, e.g.: `I {liv|L IH V} in {Sevilla|S EH V IY Y AH}`.
 * @param synthesize_params Global parameters for synthesized text. See 'pv_orca_synthesize_text_params_t' for details.
 * @param[out] num_samples The length of the pcm.
 * @param[out] pcm The output audio.
 * @return Status code. Returns `PV_STATUS_INVALID_ARGUMENT` or `PV_STATUS_OUT_OF_MEMORY`,
 * `PV_STATUS_RUNTIME_ERROR`, `PV_STATUS_ACTIVATION_ERROR`, `PV_STATUS_ACTIVATION_LIMIT_REACHED`,
 * `PV_STATUS_ACTIVATION_THROTTLED`, or `PV_STATUS_ACTIVATION_REFUSED` on failure.
 */
PV_API pv_status_t pv_orca_synthesize(
        const pv_orca_t *object,
        const char *text,
        const pv_orca_synthesize_params_t *synthesize_params,
        int32_t *num_samples,
        int16_t **pcm);

/**
 * Generates audio from text and saves it to a file. The file contains the speech representation of the text.
 *
 * @param object The Orca object.
 * @param text Text to be converted to audio. The maximum length can be attained by calling
 * `pv_orca_max_character_limit()`. Allowed characters can be retrieved by calling `pv_orca_valid_characters()`.
 * Custom pronunciations can be embedded in the text via the syntax `{word|pronunciation}`.
 * The pronunciation is expressed in ARPAbet format, e.g.: `I {liv|L IH V} in {Sevilla|S EH V IY Y AH}`.
 * @param synthesize_params Global parameters for synthesized text. See 'pv_orca_synthesize_text_params_t()' for details.
 * @param output_path Absolute path to the output audio file. The output file is saved as `WAV (.wav)`
 * and consists of a single mono channel.
 * @return Status code. Returns `PV_STATUS_INVALID_ARGUMENT` or `PV_STATUS_OUT_OF_MEMORY`,
 * `PV_STATUS_RUNTIME_ERROR`, `PV_STATUS_ACTIVATION_ERROR`, `PV_STATUS_ACTIVATION_LIMIT_REACHED`,
 * `PV_STATUS_ACTIVATION_THROTTLED`, or `PV_STATUS_ACTIVATION_REFUSED` on failure.
 */
PV_API pv_status_t pv_orca_synthesize_to_file(
        const pv_orca_t *object,
        const char *text,
        const pv_orca_synthesize_params_t *synthesize_params,
        const char *output_path);

/**
 * Deletes the audio previously generated by the Orca synthesize functions.
 *
 * @param object The pcm generated by orca synthesize functions.
 */
PV_API void pv_orca_delete_pcm(int16_t *pcm);

/**
 * Getter for version.
 *
 * @return Version.
 */
PV_API const char *pv_orca_version(void);

#ifdef __cplusplus

}

#endif

#endif // PV_ORCA_H
