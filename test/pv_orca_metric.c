#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "audio/pv_resampler.h"
#include "core/picovoice.h"
#include "core/pv_error.h"
#include "core/pv_error_messages.h"
#include "core/pv_language_json.h"
#include "core/pv_type.h"
#include "model/pv_filterbank.h"
#include "io/pv_log.h"
#include "model/pv_online_token_classifier.h"
#include "orca/pv_orca_metric_internal.h"
#include "util/pv_check_status.h"
#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

static const char *ORCA_METRIC_CLASSIFIER_MAGIC_TOKEN = "porcupine_classifier";
static const char *ORCA_METRIC_CLASSIFIER_VERSION = "3.0.X";
static const int32_t ORCA_METRIC_CLASSIFIER_FRAME_LENGTH = 512;
static const int32_t ORCA_METRIC_CLASSIFIER_FRAME_SHIFT = 256;

struct pv_orca_metric {
    pv_memory_t *memory;
    pv_filterbank_t *feature_factory;

    pv_online_token_classifier_param_t *classifier_param;
    pv_online_token_classifier_t *classifier;
    int32_t frame_length;

    pv_resampler_t *resampler;
    int32_t sample_rate;

    pv_language_info_t *language_info;
    int32_t num_phonemes;
};

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_orca_metric_classifier_param_serialize)(
        pv_ypu_t *ypu,
        const pv_online_token_classifier_param_t *param,
        const char *language_info_path,
        const char *param_path) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(language_info_path);
    PV_ASSERT(param_path);

    pv_status_t status = pv_online_token_classifier_param_serialize(
            ypu,
            param,
            ORCA_METRIC_CLASSIFIER_MAGIC_TOKEN,
            ORCA_METRIC_CLASSIFIER_VERSION,
            param_path);
    PV_CHECK_STATUS(status);

    int32_t num_ppn_bytes = 0;
    void *ppn_bytes = NULL;
    status = pv_file_load(param_path, &num_ppn_bytes, &ppn_bytes);
    PV_CHECK_STATUS(status);

    pv_language_info_t *language_info = NULL;
    status = pv_language_info_load_json(language_info_path, &language_info, true, false);
    PV_CHECK_STATUS(status);

    FILE *f = fopen(param_path, "wb");
    if (!f) {
        free(ppn_bytes);
        pv_language_info_delete(language_info);
        return PV_STATUS_IO_ERROR;
    }

    size_t num_write = fwrite(ppn_bytes, 1, num_ppn_bytes, f);
    free(ppn_bytes);
    if (num_write != (size_t) num_ppn_bytes) {
        pv_language_info_delete(language_info);
        (void) fclose(f);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    status = pv_serialized_serialize_file(
        pv_language_info_serialized_vtable(),
        NULL,
        true,
        f,
        language_info);
    pv_language_info_delete(language_info);
    (void) fclose(f);

    return status;
}

#endif

pv_status_t PV_MOCKABLE(pv_orca_metric_init)(
        pv_ypu_t *ypu,
        const char *model_path,
        int32_t sample_rate,
        pv_orca_metric_t **object) {
    PV_ASSERT(ypu);
    PV_ASSERT(model_path);
    PV_ASSERT(sample_rate > 0);
    PV_ASSERT(object);

    *object = NULL;

    pv_orca_metric_t *o = calloc(1, sizeof(pv_orca_metric_t));
    if (!o) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_memory_t *memory = NULL;
    pv_status_t status = pv_memory_init(&memory);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_module_init_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE(
                        "pv_memory",
                        pv_status_to_string(status)));
        return status;
    }

    o->memory = memory;

    status = pv_filterbank_init(memory, ORCA_METRIC_CLASSIFIER_FRAME_SHIFT, &(o->feature_factory));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_module_init_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE(
                        "pv_filterbank_init",
                        pv_status_to_string(status)));
        pv_orca_metric_delete(ypu, o);
        return status;
    }

    FILE *f = pv_fopen(model_path, "rb");
    if (!f) {
        PV_ERROR_REPORT(
                &pv_error_msg_fopen_failure,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE(
                        "pv_fopen",
                        pv_status_to_string(status)));
        pv_orca_metric_delete(ypu, o);
        return PV_STATUS_IO_ERROR;
    }

    status = pv_online_token_classifier_param_load2(
            ypu,
            f,
            ORCA_METRIC_CLASSIFIER_MAGIC_TOKEN,
            ORCA_METRIC_CLASSIFIER_VERSION,
            &(o->classifier_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_parameters_load_failure,
                PV_ERROR_ARGS_PUBLIC(pv_status_to_string(status)),
                PV_ERROR_ARGS_PRIVATE(
                        pv_status_to_string(status),
                        "pv_online_token_classifier_param_load2"));
        (void) fclose(f);
        pv_orca_metric_delete(ypu, o);
        return status;
    }

    status = pv_serialized_deserialize_file(
            pv_language_info_serialized_vtable(),
            NULL,
            true,
            f,
            (void **) &(o->language_info));
    (void) fclose(f);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_module_init_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE(
                        "pv_serialized_deserialize_file",
                        pv_status_to_string(status)));
        pv_orca_metric_delete(ypu, o);
        return status;
    }

    if (!pv_language_info_equals_language_code(o->language_info, o->classifier_param->language_code)) {
        PV_ERROR_REPORT(
                &pv_error_msg_language_info_mismatch_internal,
                PV_ERROR_ARGS_PUBLIC(
                        o->classifier_param->language_code,
                        pv_language_info_language_code(o->language_info)),
                PV_ERROR_ARGS_PRIVATE(
                        o->classifier_param->language_code,
                        pv_language_info_language_code(o->language_info),
                        "classifier",
                        "language_info"));
        pv_orca_metric_delete(ypu, o);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    status = pv_online_token_classifier_init(ypu, o->classifier_param, &(o->classifier));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_module_init_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE(
                        "pv_online_token_classifier_init",
                        pv_status_to_string(status)));
        pv_orca_metric_delete(ypu, o);
        return status;
    }

    status = pv_resampler_init(sample_rate, pv_sample_rate(), 50, &(o->resampler));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_module_init_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE(
                        "pv_resampler_init",
                        pv_status_to_string(status)));
        pv_orca_metric_delete(ypu, o);
        return status;
    }

    o->frame_length = ORCA_METRIC_CLASSIFIER_FRAME_LENGTH;
    o->num_phonemes = pv_language_info_num_phonemes(o->language_info);
    o->sample_rate = sample_rate;

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_orca_metric_delete)(pv_ypu_t *ypu, pv_orca_metric_t *object) {
    PV_ASSERT(ypu);

    if (object) {
        pv_resampler_delete(object->resampler);

        pv_online_token_classifier_delete(ypu, object->classifier);

        pv_online_token_classifier_param_delete(ypu, object->classifier_param);

        pv_language_info_delete(object->language_info);

        pv_filterbank_delete(object->feature_factory);

        free(object);
    }
}

pv_status_t PV_MOCKABLE(pv_orca_metric_get_posterior_frame)(
        pv_ypu_t *ypu,
        pv_orca_metric_t *object,
        const int16_t *pcm,
        q31_t *posterior) {
    PV_ASSERT(object);
    PV_ASSERT(pcm);
    PV_ASSERT(posterior);

    pv_filterbank_t *feature_factory = object->feature_factory;
    const int32_t feature_length = 2 * PV_FILTERBANK_LENGTH;

    pv_ypu_mem_t *posterior_ypu_mem = pv_ypu_buffer_get(
        ypu,
        object->classifier_param->softmax_param->output_length * sizeof(q31_t),
        false);
    if (!posterior_ypu_mem) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_mem_t *feature_ypu_mem = pv_ypu_buffer_get(
        ypu,
        feature_length * sizeof(q510_t),
        false);
    if (!feature_ypu_mem) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    q510_t *feature = pv_ypu_mem_get_host_view(ypu, feature_ypu_mem, false);

    pv_status_t status = pv_filterbank_compute(feature_factory, pcm, feature);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }
    status = pv_filterbank_compute(
            feature_factory,
            pcm + ORCA_METRIC_CLASSIFIER_FRAME_SHIFT,
            feature + PV_FILTERBANK_LENGTH);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_mem_release_host_view(ypu, feature_ypu_mem, true);

    pv_online_token_classifier_preprocess(ypu, object->classifier, feature_ypu_mem, 2);

    status = pv_online_token_classifier_process(ypu, object->classifier, feature_ypu_mem, posterior_ypu_mem);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_mem_copy_from(ypu, posterior_ypu_mem, posterior, 0, object->classifier_param->softmax_param->output_length * sizeof(q31_t));

    pv_ypu_buffer_release(ypu, feature_ypu_mem);
    pv_ypu_buffer_release(ypu, posterior_ypu_mem);

    return PV_STATUS_SUCCESS;
}

static pv_status_t ctc_path(
        int32_t posterior_size,
        int32_t num_frames,
        const float *posterior,
        int32_t *num_labels,
        int32_t **labels) {
    PV_ASSERT(posterior_size > 0);
    PV_ASSERT(num_frames > 0);
    PV_ASSERT(posterior);
    PV_ASSERT(num_labels);
    PV_ASSERT(labels);

    *num_labels = 0;
    *labels = NULL;

    int32_t prev_token = -1;
    int32_t count = 0;

    int32_t *labels_buffer = calloc(num_frames, sizeof(int32_t));
    if (!labels_buffer) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    for (int32_t i = 0; i < num_frames; ++i) {
        const int32_t frame_offset = i * posterior_size;

        int32_t max_index = 0;
        for (int j = 1; j < posterior_size; ++j) {
            if (posterior[frame_offset + j] > posterior[frame_offset + max_index]) {
                max_index = j;
            }
        }

        if (max_index != 0 && max_index != prev_token) {
            labels_buffer[count++] = max_index;
        }

        prev_token = max_index;
    }

    *num_labels = count;
    *labels = malloc(*num_labels * sizeof(int32_t));
    if (!*labels) {
        free(labels_buffer);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    memcpy(*labels, labels_buffer, *num_labels * sizeof(int32_t));

    free(labels_buffer);
    return PV_STATUS_SUCCESS;
}

static pv_status_t edit_distance(
        int32_t X_length,
        const int32_t *X,
        int32_t Y_length,
        const int32_t *Y,
        int32_t *num_edits) {
    PV_ASSERT(X_length > 0);
    PV_ASSERT(X);
    PV_ASSERT(Y_length > 0);
    PV_ASSERT(Y);
    PV_ASSERT(num_edits);

    const int32_t n = X_length;
    const int32_t m = Y_length;

    int32_t **cost = calloc(n + 1, sizeof(int32_t *));
    if (!cost) {
        return PV_STATUS_OUT_OF_MEMORY;
    }
    for (int32_t i = 0; i <= n; i++) {
        cost[i] = calloc(m + 1, sizeof(int32_t));
        if (!(cost[i])) {
            for (int32_t j = 0; j < i; j++) {
                free(cost[j]);
            }
            free(cost);
            return PV_STATUS_OUT_OF_MEMORY;
        }
    }

    for (int32_t i = 0; i <= n; i++) {
        cost[i][0] = i;
    }

    for (int32_t i = 0; i <= m; i++) {
        cost[0][i] = i;
    }

    for (int32_t i = 1; i <= n; i++) {
        for (int32_t j = 1; j <= m; j++) {
            const int32_t insertion_cost = cost[i - 1][j] + 1;
            cost[i][j] = insertion_cost;

            const int32_t deletion_cost = cost[i][j - 1] + 1;
            if (deletion_cost < cost[i][j]) {
                cost[i][j] = deletion_cost;
            }

            const int32_t match_cost = cost[i - 1][j - 1] + ((X[i - 1] == Y[j - 1]) ? 0 : 1);
            if (match_cost < cost[i][j]) {
                cost[i][j] = match_cost;
            }
        }
    }

    *num_edits = cost[n][m];

    for (int32_t i = 0; i <= n; i++) {
        free(cost[i]);
    }
    free(cost);

    return PV_STATUS_SUCCESS;
}

static void phoneme_sequence_padded_aos_free(char **padded_aos, int32_t length) {
    for (int32_t i = 0; i < length; ++i) {
        free(padded_aos[i]);
        padded_aos[i] = NULL;
    }
}

static pv_status_t phoneme_sequence_string_to_padded_aos(
        const char *phoneme_sequence,
        char ***padded_aos,
        int32_t *len_padding) {
    PV_ASSERT(phoneme_sequence);
    PV_ASSERT(padded_aos);
    PV_ASSERT(len_padding);

    int32_t padded_aos_len = 0;
    ++padded_aos_len; // For the first padded space.
    int32_t phoneme_sequence_length = (int32_t) strlen(phoneme_sequence);
    for (int32_t i = 0; i < phoneme_sequence_length; ++i) {
        if (phoneme_sequence[i] == ' ') {
            padded_aos_len += 2;
        }
    }
    ++padded_aos_len; // For the last phoneme.
    ++padded_aos_len; // For the last padded space.
    *len_padding = padded_aos_len;

    *padded_aos = (char **) calloc(padded_aos_len, sizeof(char *));
    if (!padded_aos) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("padded_aos"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    (*padded_aos)[0] = (char *) calloc(1 + 1, sizeof(char));
    if (!(*padded_aos)[0]) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("(*padded_aos)[0]"));
        phoneme_sequence_padded_aos_free(*padded_aos, 0); // Equivalent to not making the call.
        free(*padded_aos);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    strcpy((*padded_aos)[0], " ");

    int32_t prev_start_index = 0;
    int32_t phoneme_size = 0;
    int32_t padded_aos_index = 1;
    for (int32_t i = 0; i < phoneme_sequence_length; ++i) {
        if (phoneme_sequence[i] == ' ') {
            phoneme_size = i - prev_start_index;
            (*padded_aos)[padded_aos_index] = (char *) calloc(phoneme_size + 1, sizeof(char));
            if (!(*padded_aos)[padded_aos_index]) {
                PV_ERROR_REPORT(
                        &pv_error_msg_alloc,
                        PV_ERROR_ARGS_PUBLIC_EMPTY(),
                        PV_ERROR_ARGS_PRIVATE("(*padded_aos)[padded_aos_index]"));
                phoneme_sequence_padded_aos_free(*padded_aos, padded_aos_index);
                free(*padded_aos);
                return PV_STATUS_OUT_OF_MEMORY;
            }
            memcpy((*padded_aos)[padded_aos_index], phoneme_sequence + prev_start_index, phoneme_size * sizeof(char));
            (*padded_aos)[padded_aos_index][phoneme_size] = '\0';

            prev_start_index = i + 1;
            ++padded_aos_index;

            // Pad space:
            (*padded_aos)[padded_aos_index] = (char *) calloc(1 + 1, sizeof(char));
            if (!(*padded_aos)[padded_aos_index]) {
                PV_ERROR_REPORT(
                        &pv_error_msg_alloc,
                        PV_ERROR_ARGS_PUBLIC_EMPTY(),
                        PV_ERROR_ARGS_PRIVATE("(*padded_aos)[padded_aos_index]"));
                phoneme_sequence_padded_aos_free(*padded_aos, padded_aos_index);
                free(*padded_aos);
                return PV_STATUS_OUT_OF_MEMORY;
            }
            strcpy((*padded_aos)[padded_aos_index], " ");
            ++padded_aos_index;
        }
    }

    // Padd last phoneme:
    phoneme_size = phoneme_sequence_length - prev_start_index;
    (*padded_aos)[padded_aos_index] = (char *) calloc(phoneme_size + 1, sizeof(char));
    if (!(*padded_aos)[padded_aos_index]) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("(*padded_aos)[padded_aos_index]"));
        phoneme_sequence_padded_aos_free(*padded_aos, padded_aos_index);
        free(*padded_aos);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    memcpy((*padded_aos)[padded_aos_index], phoneme_sequence + prev_start_index, phoneme_size * sizeof(char));
    (*padded_aos)[padded_aos_index][phoneme_size] = '\0';
    ++padded_aos_index;

    // Padd last space:
    PV_ASSERT(padded_aos_index == padded_aos_len - 1);
    (*padded_aos)[padded_aos_index] = (char *) calloc(1 + 1, sizeof(char));
    if (!(*padded_aos)[padded_aos_index]) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("(*padded_aos)[padded_aos_len - 1]"));
        phoneme_sequence_padded_aos_free(*padded_aos, padded_aos_index);
        free(*padded_aos);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    strcpy((*padded_aos)[padded_aos_index], " ");

    return PV_STATUS_SUCCESS;
}


pv_status_t PV_MOCKABLE(pv_orca_metric_process)(
        pv_ypu_t *ypu,
        pv_orca_metric_t *object,
        int32_t pcm_length,
        const int16_t *pcm,
        int32_t num_target_char_labels,
        const char **target_char_labels,
        float *phoneme_error_rate) {
    PV_ASSERT(ypu);
    PV_ASSERT(object);
    PV_ASSERT(pcm_length > 0);
    PV_ASSERT(pcm);
    PV_ASSERT(num_target_char_labels > 0);
    PV_ASSERT(target_char_labels);
    PV_ASSERT(phoneme_error_rate);

    *phoneme_error_rate = 0.f;

    int16_t *resampled_pcm = NULL;
    if (object->sample_rate != pv_sample_rate()) {
        // Add 20 frames to pad pcm with zeros in the beginning and end (each for 10 frames).
        // This improves the classifier's phoneme recognition at the start and end.
        int32_t num_frames_to_padd = 20;
        int32_t max_output_pcm_length =
                pv_resampler_convert_num_samples_to_output_sample_rate(object->resampler, pcm_length) +
                (num_frames_to_padd * object->frame_length);
        resampled_pcm = calloc(max_output_pcm_length, sizeof(int16_t));
        if (!resampled_pcm) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("resampled_pcm"));
            return PV_STATUS_OUT_OF_MEMORY;
        }
        pv_resampler_process(object->resampler,
                             pcm,
                             pcm_length,
                             resampled_pcm + (num_frames_to_padd / 2) * object->frame_length);
        pcm_length = max_output_pcm_length;
    }

    int16_t *pcm_chunk = calloc(object->frame_length, sizeof(int16_t));
    if (!pcm_chunk) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("pcm_chunk"));
        free(resampled_pcm);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    int32_t num_frames = pcm_length / object->frame_length;
    num_frames = (pcm_length % object->frame_length) ? num_frames : num_frames + 1;

    int32_t posterior_size = object->num_phonemes + 1;
    q31_t *posteriors = calloc(num_frames * posterior_size, sizeof(q31_t));
    if (!posteriors) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("posteriors"));
        free(pcm_chunk);
        free(resampled_pcm);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    int16_t *pcm_src = resampled_pcm != NULL ? resampled_pcm : (int16_t *) pcm;
    for (int32_t i = 0; i < num_frames; i++) {
        if ((i + 1) * object->frame_length > pcm_length) {
            memset(pcm_chunk, 0, object->frame_length * sizeof(int16_t));
            memcpy(pcm_chunk, pcm_src + (i * object->frame_length),
                   (pcm_length - (i * object->frame_length)) * sizeof(int16_t));
        } else {
            memcpy(pcm_chunk, pcm_src + (i * object->frame_length), object->frame_length * sizeof(int16_t));
        }

        pv_status_t status = pv_orca_metric_get_posterior_frame(
            ypu,
            object,
            pcm_chunk,
            &posteriors[i * posterior_size]);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_orca_metric_get_posterior_frame,
                    pv_status_to_string(status));
            free(posteriors);
            free(pcm_chunk);
            free(resampled_pcm);
            return status;
        }
    }
    free(resampled_pcm);
    free(pcm_chunk);

    float *posteriors_log10 = calloc(posterior_size * num_frames, sizeof(float));
    if (!posteriors_log10) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("posteriors_log10"));
        free(posteriors);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    for (int32_t i = 0; i < (posterior_size * num_frames); i++) {
        posteriors_log10[i] = log10f(pv_q31_to_float(posteriors[i]));
    }
    free(posteriors);

    int32_t num_predicted_labels = 0;
    int32_t *predicted_labels = NULL;
    ctc_path(posterior_size, num_frames, posteriors_log10, &num_predicted_labels, &predicted_labels);

    int32_t num_target_labels = num_target_char_labels;
    int32_t *target_labels = calloc(num_target_labels, sizeof(int32_t));
    if (!target_labels) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("target_labels"));
        free(posteriors_log10);
        free(predicted_labels);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    for (int32_t i = 0; i < num_target_char_labels; ++i) {
        pv_language_info_phoneme_index_from_string(object->language_info, target_char_labels[i], &target_labels[i]);
    }

#ifdef __ORCA_PHONEMIZER_LOG_DEBUG__

    LOG_DEBUG_SIMPLE("Predicted phoneme labels");
    for (int32_t i = 0; i < num_predicted_labels; ++i) {
        if (i == num_predicted_labels - 1) {
            LOG_DEBUG_INLINE("%d\n", predicted_labels[i]);
        } else {
            LOG_DEBUG_INLINE("%d ", predicted_labels[i]);
        }
    }
    LOG_DEBUG_SIMPLE("Target phoneme labels");
    for (int32_t i = 0; i < num_target_labels; ++i) {
        if (i == num_target_labels - 1) {
            LOG_DEBUG_INLINE("%d\n", target_labels[i]);
        } else {
            LOG_DEBUG_INLINE("%d ", target_labels[i]);
        }
    }

#endif

    int32_t num_edits = 0;
    pv_status_t status = edit_distance(
            num_predicted_labels,
            predicted_labels,
            num_target_labels,
            target_labels,
            &num_edits);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                edit_distance,
                pv_status_to_string(status));
        free(posteriors_log10);
        free(predicted_labels);
        free(target_labels);
        return status;
    }
    float edit_distance = (float) num_edits / (float) num_target_labels;

    *phoneme_error_rate = edit_distance;

    free(posteriors_log10);
    free(predicted_labels);
    free(target_labels);

    return PV_STATUS_SUCCESS;
}


pv_status_t PV_MOCKABLE(pv_orca_metric_pcm_frame_level_error_evaluation)(
        pv_ypu_t *ypu,
        pv_orca_metric_t *object,
        int32_t pcm_length,
        const int16_t *pcm,
        const char *truth_phoneme_sequence,
        float threshold_worst_case,
        float threshold_average_case,
        bool *passed) {
    PV_ASSERT(ypu);
    PV_ASSERT(object);
    PV_ASSERT(pcm_length > 0);
    PV_ASSERT(pcm);
    PV_ASSERT(truth_phoneme_sequence);
    PV_ASSERT(passed);
    *passed = true;

    int16_t *resampled_pcm = NULL;
    if (object->sample_rate != pv_sample_rate()) {
        // Add 20 frames to pad pcm with zeros in the beginning and end (each for 10 frames).
        // This improves the classifier's phoneme recognition at the start and end.
        int32_t num_frames_to_padd = 20;
        int32_t max_output_pcm_length =
                pv_resampler_convert_num_samples_to_output_sample_rate(object->resampler, pcm_length) +
                (num_frames_to_padd * object->frame_length);
        resampled_pcm = (int16_t *) calloc(max_output_pcm_length, sizeof(int16_t));
        if (!resampled_pcm) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("resampled_pcm"));
            return PV_STATUS_OUT_OF_MEMORY;
        }
        pv_resampler_process(object->resampler,
                             pcm,
                             pcm_length,
                             resampled_pcm + (num_frames_to_padd / 2) * object->frame_length);
        pcm_length = max_output_pcm_length;
    }

    int16_t *pcm_chunk = (int16_t *) calloc(object->frame_length, sizeof(int16_t));
    if (!pcm_chunk) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("pcm_chunk"));
        free(resampled_pcm);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    // (Ted & Matthew): Need `num_frames` to be `size_t` as opposed to `int32_t` to pass wasm-linux-release compiler, because otherwise, the compiler complains about overflow at a latter calloc that uses `num_frames`, and it looks like the compiler think it is `int64_t`.
    size_t num_frames = (size_t) pcm_length / (size_t) object->frame_length;
    num_frames = (pcm_length % object->frame_length) ? num_frames : num_frames + 1;

    int32_t posterior_size = object->num_phonemes + 1;
    q31_t *posteriors = (q31_t *) calloc(num_frames * posterior_size, sizeof(q31_t));
    if (!posteriors) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("posteriors"));
        free(pcm_chunk);
        free(resampled_pcm);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    int16_t *pcm_src = resampled_pcm != NULL ? resampled_pcm : (int16_t *) pcm;
    for (int32_t i = 0; i < (int32_t) num_frames; i++) {
        if ((i + 1) * object->frame_length > pcm_length) {
            memset(pcm_chunk, 0, object->frame_length * sizeof(int16_t));
            memcpy(pcm_chunk, pcm_src + (i * object->frame_length),
                   (pcm_length - (i * object->frame_length)) * sizeof(int16_t));
        } else {
            memcpy(pcm_chunk, pcm_src + (i * object->frame_length), object->frame_length * sizeof(int16_t));
        }

        pv_status_t status = pv_orca_metric_get_posterior_frame(
            ypu,
            object,
            pcm_chunk,
            &posteriors[i * posterior_size]);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_orca_metric_get_posterior_frame,
                    pv_status_to_string(status));
            free(posteriors);
            free(pcm_chunk);
            free(resampled_pcm);
            return status;
        }
    }
    free(pcm_chunk);
    free(resampled_pcm);

    // 1. Use dynamic programming to compute forward variables "alphas".
    // 2. Backtrack to derive the most probable alpha that is allowed in the CTC rule (as opposed to summing up the potential alphas because for at least one reason, we assume there exists a dominant path.
    // 3. For each frame in the (uniquely derived) alpha-path, as opposed to compare the threshold with the alpha, we compare with the probability derived from the posterior matrix.

    // Dequantization:
    float *posteriors_float = (float *) calloc(posterior_size * num_frames, sizeof(float));
    if (!posteriors_float) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("posteriors_float"));
        free(posteriors);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    for (int32_t i = 0; i < (posterior_size * (int32_t) num_frames); ++i) {
        posteriors_float[i] = pv_q31_to_float(posteriors[i]);
    }
    free(posteriors);
    posteriors = NULL;

    float prob = 0.0f;
    int32_t phoneme_index = 0;
    // Below padded phoneme sequence is of form: <SPACE> -> <PHONEME> -> <SPACE> -> <PHONEME> -> ... -> <PHONEME> -> <SPACE>.
    char **truth_phoneme_sequence_aos_padded = NULL;
    int32_t len_padding = 0;
    phoneme_sequence_string_to_padded_aos(truth_phoneme_sequence, &truth_phoneme_sequence_aos_padded, &len_padding);
    float *alpha_hat_table = (float *) calloc(num_frames * len_padding, sizeof(float));
    if (!alpha_hat_table) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("alpha_hat_table"));
        free(posteriors_float);
        phoneme_sequence_padded_aos_free(truth_phoneme_sequence_aos_padded, len_padding);
        free(truth_phoneme_sequence_aos_padded);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    float *entry = NULL;
    float C_t = 0.0f;
    float *posteriors_float_frame = NULL;
    pv_status_t status = PV_STATUS_SUCCESS;

    for (int32_t t = 0; t < (int32_t) num_frames; ++t) {
        prob = 0.0f;
        phoneme_index = 0;
        entry = NULL;
        C_t = 0.0f;
        posteriors_float_frame = NULL;
        posteriors_float_frame = posteriors_float + t * posterior_size;

        for (int32_t s = 0; s < len_padding; ++s) {
            // Blank symbol that is padded to target phoneme sequence.
            if (s % 2 == 0) {
                phoneme_index = 0;
                prob = posteriors_float_frame[phoneme_index];
            } else {
                status = pv_language_info_phoneme_index_from_string(object->language_info, truth_phoneme_sequence_aos_padded[s], &phoneme_index);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_language_info_phoneme_index_from_string,
                            pv_status_to_string(status));
                    free(posteriors_float);
                    phoneme_sequence_padded_aos_free(truth_phoneme_sequence_aos_padded, len_padding);
                    free(truth_phoneme_sequence_aos_padded);
                    free(alpha_hat_table);
                    return status;
                }
                prob = posteriors_float_frame[phoneme_index];
            }
            entry = alpha_hat_table + t * len_padding + s;
            if (s == 0 && t == 0) {
                // "blank" is assumed to be indexed/encoded to 0.
                *entry = posteriors_float_frame[0];
            } else if (s == 1 && t == 0) {
                *entry = posteriors_float_frame[phoneme_index];
            } else if (((t == 0) && (s > 1)) || (s + 1 < len_padding - 2 * ((int32_t) num_frames - (t + 1)) - 1)) {
                *entry = 0.0f;
            } else if (s == 0 && t != 0) {
                *entry = prob * alpha_hat_table[(t - 1) * len_padding + s];
            } else if (s == 1 && t != 0) {
                *entry = prob * (alpha_hat_table[(t - 1) * len_padding + s] +
                                 alpha_hat_table[(t - 1) * len_padding + (s - 1)]);
            } else if (strcmp(truth_phoneme_sequence_aos_padded[s], " ") == 0 ||
                       strcmp(truth_phoneme_sequence_aos_padded[s - 2], truth_phoneme_sequence_aos_padded[s]) == 0) {
                *entry = prob * (alpha_hat_table[(t - 1) * len_padding + s] +
                                 alpha_hat_table[(t - 1) * len_padding + (s - 1)]);
            } else {
                *entry = prob * (alpha_hat_table[(t - 1) * len_padding + s] +
                                 alpha_hat_table[(t - 1) * len_padding + (s - 1)] +
                                 alpha_hat_table[(t - 1) * len_padding + (s - 2)]);
            }
        }

        // Compute the denominator <C_t>.
        for (int32_t s = 0; s < len_padding; ++s) {
            C_t += alpha_hat_table[t * len_padding + s];
        }

        for (int32_t s = 0; s < len_padding; ++s) {
            // Normalize <alpha_hat_table>'s frame to prevent underflow due to multiplication.
            alpha_hat_table[t * len_padding + s] /= C_t;
        }
    }

    int32_t *backtrack_seq = (int32_t *) calloc(num_frames, sizeof(int32_t));
    if (!backtrack_seq) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("backtract_seq"));
        free(posteriors_float);
        phoneme_sequence_padded_aos_free(truth_phoneme_sequence_aos_padded, len_padding);
        free(truth_phoneme_sequence_aos_padded);
        free(alpha_hat_table);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    int32_t s = len_padding - 1;
    float *alpha_hat_table_frame = NULL;
    for (int32_t t = (int32_t) num_frames - 1; t >= 0; --t) {
        alpha_hat_table_frame = alpha_hat_table + t * len_padding;

        if (t == (int32_t) num_frames - 1) {
            backtrack_seq[t] = alpha_hat_table_frame[s] >= alpha_hat_table_frame[s - 1] ? s : s - 1;
        } else {
            if (s == 0) {
                backtrack_seq[t] = s;
            } else if (s == 1) {
                backtrack_seq[t] = alpha_hat_table_frame[s] >= alpha_hat_table_frame[s - 1] ? s : s - 1;
            } else if (strcmp(truth_phoneme_sequence_aos_padded[s], " ") == 0 ||
                       strcmp(truth_phoneme_sequence_aos_padded[s - 2], truth_phoneme_sequence_aos_padded[s]) == 0) {
                backtrack_seq[t] = alpha_hat_table_frame[s] >= alpha_hat_table_frame[s - 1] ? s : s - 1;
            } else {
                backtrack_seq[t] = alpha_hat_table_frame[s] >= alpha_hat_table_frame[s - 1] ? s : s - 1;
                backtrack_seq[t] = alpha_hat_table_frame[backtrack_seq[t]] >= alpha_hat_table_frame[s - 2] ? backtrack_seq[t] : s - 2;
            }
        }
        s = backtrack_seq[t];
    }

    int32_t num_ignored_frames = 0;
    float prob_average = 0.0f;
    for (int32_t t = 0; t < (int32_t) num_frames; ++t) {
        posteriors_float_frame = posteriors_float + t * posterior_size;
        if (strcmp(truth_phoneme_sequence_aos_padded[backtrack_seq[t]], " ") == 0) {
            prob = posteriors_float_frame[0];
        } else {
            status = pv_language_info_phoneme_index_from_string(object->language_info, truth_phoneme_sequence_aos_padded[backtrack_seq[t]], &phoneme_index);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_language_info_phoneme_index_from_string,
                        pv_status_to_string(status));
                free(posteriors_float);
                phoneme_sequence_padded_aos_free(truth_phoneme_sequence_aos_padded, len_padding);
                free(truth_phoneme_sequence_aos_padded);
                free(alpha_hat_table);
                free(backtrack_seq);
                return status;
            }
            prob = posteriors_float_frame[phoneme_index];
        }

        prob_average += prob;

        // Worst case threshold:
        if (prob < threshold_worst_case) {
            *passed = false;
            LOG_ERROR("Failed frame %d; threshold_worst_case: %f; prob: %f; phoneme: %s; index in target spoken phoneme sequence: %d", t, threshold_worst_case, prob, truth_phoneme_sequence_aos_padded[backtrack_seq[t]], ((backtrack_seq[t] % 2 == 0) ? -1 : (backtrack_seq[t] - 1) / 2));
            for (int32_t z = 0; z < posterior_size; ++z) {
                const char *tmp_phoneme_string = "_";
                if (z >= 0) {
                    pv_language_info_phoneme_index_to_string(
                            object->language_info,
                            z,
                            &tmp_phoneme_string);
                }

                LOG_ERROR("\t %s: %f, ", tmp_phoneme_string, posteriors_float_frame[z]);
            }
            LOG_ERROR_SIMPLE("\n");
        }
    }

    // Average case threshold:
    prob_average /= ((int32_t) num_frames - num_ignored_frames);
    if (prob_average < threshold_average_case) {
        *passed = false;
        LOG_ERROR_SIMPLE("Failed average case threshold test.");
        LOG_ERROR("Number of frames not ignored: %d; threshold_average_case: %f; prob_average: %f", (int32_t) num_frames - num_ignored_frames, threshold_average_case, prob_average);
    } else {
        LOG_INFO_SIMPLE("Passed average case threshold test.");
        LOG_INFO("Number of frames not ignored: %d; threshold_average_case: %f; prob_average: %f", (int32_t) num_frames - num_ignored_frames, threshold_average_case, prob_average);
    }

    free(posteriors_float);
    posteriors_float = NULL;
    phoneme_sequence_padded_aos_free(truth_phoneme_sequence_aos_padded, len_padding);
    free(truth_phoneme_sequence_aos_padded);
    truth_phoneme_sequence_aos_padded = NULL;
    free(alpha_hat_table);
    alpha_hat_table = NULL;
    free(backtrack_seq);
    backtrack_seq = NULL;

    return PV_STATUS_SUCCESS;
}

