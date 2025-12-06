#include <stdlib.h>
#include <string.h>

#include "core/pv_error_messages.h"
#include "core/pv_type.h"
#include "orca/pv_orca_internal.h"
#include "orca/pv_orca_stream_state.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

static const int32_t PV_ORCA_STREAM_PHONEMES_CHUNK_SIZE = 30;
static const int32_t PV_ORCA_STREAM_MIN_NUM_PHONEMES_KEPT = 200;
static const int32_t PV_ORCA_STREAM_MAX_NUM_PHONEMES_KEPT = 400;
static const int32_t PV_ORCA_STREAM_NUM_PHONEMES_REMOVE_FALLBACK = 200;

static pv_status_t cache_z_prior(
        pv_ypu_t *ypu,
        pv_orca_stream_state_t *object,
        int32_t num_frames,
        pv_ypu_mem_t *buffer_z_prior) {
    PV_ASSERT(ypu);
    PV_ASSERT(object);
    PV_ASSERT(num_frames > 0);
    PV_ASSERT(buffer_z_prior);

    if (object->is_flush) {
        return PV_STATUS_SUCCESS;
    }

    const pv_orca_stream_config_t *config = object->config;
    const int32_t num_channels = object->num_channels;

    int32_t buffer_z_prior_offset = ((num_frames - (2 * config->receptive_field_flow)) * num_channels);
    int32_t length_cached_z_prior = 2 * config->receptive_field_flow * num_channels;
    int32_t cached_z_prior_offset = 0;
    if (buffer_z_prior_offset < 0) {
        cached_z_prior_offset += -buffer_z_prior_offset;
        length_cached_z_prior -= -buffer_z_prior_offset;
        buffer_z_prior_offset = 0;
    }

    pv_ypu_op_memcpy_args_t args = {
            .output = object->cached_z_prior,
            .input = buffer_z_prior,
            .size_bytes = length_cached_z_prior * (int32_t) sizeof(float),
            .output_offset = cached_z_prior_offset * (int32_t) sizeof(float),
            .input_offset = buffer_z_prior_offset * (int32_t) sizeof(float),
    };

    pv_status_t status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_MEMCPY,
            &args);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static pv_status_t cache_z(
        pv_ypu_t *ypu,
        pv_orca_stream_state_t *object,
        int32_t num_frames,
        pv_ypu_mem_t *buffer_z) {
    PV_ASSERT(ypu);
    PV_ASSERT(object);
    PV_ASSERT(num_frames > 0);
    PV_ASSERT(buffer_z);

    if (object->is_flush) {
        return PV_STATUS_SUCCESS;
    }

    const pv_orca_stream_config_t *config = object->config;
    const int32_t num_channels = object->num_channels;

    int32_t buffer_z_offset =
            ((num_frames - config->receptive_field_flow - (2 * config->receptive_field_vocoder)) *
             num_channels);
    int32_t length_cached_z_prior = 2 * config->receptive_field_vocoder * num_channels;
    int32_t cached_z_offset = 0;
    if (buffer_z_offset < 0) {
        cached_z_offset += -buffer_z_offset;
        length_cached_z_prior -= -buffer_z_offset;
        buffer_z_offset = 0;
    }

    pv_ypu_op_memcpy_args_t args = {
            .output = object->cached_z,
            .input = buffer_z,
            .size_bytes = length_cached_z_prior * (int32_t) sizeof(float),
            .output_offset = cached_z_offset * (int32_t) sizeof(float),
            .input_offset = buffer_z_offset * (int32_t) sizeof(float),
    };

    pv_status_t status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_MEMCPY,
            &args);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static pv_status_t pv_orca_stream_config_init(
        pv_ypu_t *ypu,
        const pv_orca_synthesizer_param_t *synthesizer_param,
        const int32_t num_eos_punctuation_indices,
        int32_t *eos_punctuation_indices,
        const int32_t num_fallback_cutoff_characters_indices,
        int32_t *fallback_cutoff_characters_indices,
        int32_t word_boundary_index,
        const pv_orca_stream_config_t **object) {
    PV_ASSERT(ypu);
    PV_ASSERT(synthesizer_param);
    PV_ASSERT(num_eos_punctuation_indices > 0);
    PV_ASSERT(eos_punctuation_indices);
    PV_ASSERT(num_fallback_cutoff_characters_indices > 0);
    PV_ASSERT(fallback_cutoff_characters_indices);
    PV_ASSERT(object);

    *object = NULL;

    pv_orca_stream_config_t *o = pv_ypu_host_alloc(ypu, sizeof(pv_orca_stream_config_t));
    if (!o) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(o, 0, sizeof(pv_orca_stream_config_t));

    o->num_eos_punctuation_indices = num_eos_punctuation_indices;
    o->eos_punctuation_indices = eos_punctuation_indices;

    o->num_fallback_cutoff_characters_indices = num_fallback_cutoff_characters_indices;
    o->fallback_cutoff_characters_indices = fallback_cutoff_characters_indices;

    o->word_boundary_index = word_boundary_index;

    o->receptive_field_text_encoder = pv_orca_text_encoder_param_receptive_field(synthesizer_param->text_encoder_param);

    o->receptive_field_duration_predictor =
            pv_orca_duration_predictor_param_receptive_field(synthesizer_param->duration_predictor_param);

    o->receptive_field_flow = PV_ORCA_FLOW_RECEPTIVE_FIELD;

    o->receptive_field_vocoder = pv_orca_vocoder_param_receptive_field(synthesizer_param->vocoder_param);

    o->size_chunk_phoneme_tokens = PV_ORCA_STREAM_PHONEMES_CHUNK_SIZE;

    o->min_num_phonemes_kept = PV_ORCA_STREAM_MIN_NUM_PHONEMES_KEPT;
    o->max_num_phonemes_kept = PV_ORCA_STREAM_MAX_NUM_PHONEMES_KEPT;
    o->num_phonemes_remove_fallback = PV_ORCA_STREAM_NUM_PHONEMES_REMOVE_FALLBACK;
    PV_ASSERT((o->max_num_phonemes_kept - o->num_phonemes_remove_fallback) >= o->min_num_phonemes_kept);

    *object = o;

    return PV_STATUS_SUCCESS;
}

static void pv_orca_stream_config_delete(pv_ypu_t *ypu, pv_orca_stream_config_t *object) {
    PV_ASSERT(ypu);

    if (object) {
        free(object->eos_punctuation_indices);
        free(object->fallback_cutoff_characters_indices);
        pv_ypu_host_free(ypu, object);
    }
}

static pv_status_t pv_orca_stream_state_prepare_buffers(pv_orca_stream_state_t *object) {
    PV_ASSERT(object);

    pv_status_t status = pv_buffer_int32_alloc(object->buffer_encoded_phonemes, 1, true);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_buffer_int32_alloc,
                pv_status_to_string(status));
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static pv_status_t pv_orca_stream_state_set_synthesize_params(
        pv_orca_stream_state_t *object,
        const pv_orca_synthesize_params_t *synthesize_params) {
    PV_ASSERT(object);
    PV_ASSERT(synthesize_params);

    if (!object->synthesize_params) {
        pv_status_t status = pv_orca_synthesize_params_init(&(object->synthesize_params));
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_orca_synthesize_params_init,
                    pv_status_to_string(status));
            return status;
        }
    }

    pv_status_t status = pv_orca_synthesize_params_copy(synthesize_params, object->synthesize_params);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_synthesize_params_copy,
                pv_status_to_string(status));
        return status;
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_orca_stream_state_init)(
        pv_ypu_t *ypu,
        const pv_orca_synthesizer_param_t *synthesizer_param,
        const int32_t num_eos_punctuation_indices,
        int32_t *eos_punctuation_indices,
        const int32_t num_fallback_cutoff_characters_indices,
        int32_t *fallback_cutoff_characters_indices,
        const int32_t word_boundary_index,
        const int32_t text_buffer_size,
        pv_orca_stream_state_t **object) {
    PV_ASSERT(ypu);
    PV_ASSERT(synthesizer_param);
    PV_ASSERT(num_eos_punctuation_indices > 0);
    PV_ASSERT(eos_punctuation_indices);
    PV_ASSERT(num_fallback_cutoff_characters_indices > 0);
    PV_ASSERT(fallback_cutoff_characters_indices);
    PV_ASSERT(text_buffer_size > 0);
    PV_ASSERT(object);

    *object = NULL;

    pv_orca_stream_state_t *o = pv_ypu_host_alloc(ypu, sizeof(pv_orca_stream_state_t));
    if (!o) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(o, 0, sizeof(pv_orca_stream_state_t));

    pv_status_t status = pv_orca_stream_config_init(
            ypu,
            synthesizer_param,
            num_eos_punctuation_indices,
            eos_punctuation_indices,
            num_fallback_cutoff_characters_indices,
            fallback_cutoff_characters_indices,
            word_boundary_index,
            &(o->config));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_stream_state_delete(ypu, o);
        return status;
    }

    const int32_t num_channels = synthesizer_param->text_encoder_param->conv_post_param->output_channels / 2;

    o->cached_z_prior = pv_ypu_mem_alloc(
            ypu,
            2 * o->config->receptive_field_flow * num_channels * (int32_t) sizeof(float),
            PV_YPU_DEVICE_MEM_FLAG_NONE);
    if (o->cached_z_prior == NULL) {
        pv_orca_stream_state_delete(ypu, o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->cached_z = pv_ypu_mem_alloc(
            ypu,
            2 * o->config->receptive_field_vocoder * num_channels * (int32_t) sizeof(float),
            PV_YPU_DEVICE_MEM_FLAG_NONE);
    if (o->cached_z_prior == NULL) {
        pv_orca_stream_state_delete(ypu, o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    status = pv_buffer_int32_init(text_buffer_size, &(o->buffer_encoded_phonemes));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_stream_state_delete(ypu, o);
        return status;
    }

    o->synthesize_params = NULL;

    o->num_channels = num_channels;

    pv_orca_stream_state_reset(o);

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_orca_stream_state_reset)(pv_orca_stream_state_t *object) {
    PV_ASSERT(object);

    object->num_characters_to_report = 0;

    pv_buffer_int32_free(object->buffer_encoded_phonemes);

    object->status = PV_ORCA_STREAM_STATUS_INACTIVE;

    object->is_first_chunk = true;
    object->is_flush = false;

    object->num_processed_chunks = 0;
    object->num_non_synthesized_encoded_phonemes = 0;

    object->start_index_dp = 0;
    object->end_index_dp = 0;
    object->durations_offset = 0;

    object->start_index_flow = 0;
    object->end_index_flow = 0;
}

pv_status_t PV_MOCKABLE(pv_orca_stream_state_open)(
        pv_orca_stream_state_t *object,
        const pv_orca_synthesize_params_t *synthesize_params) {
    PV_ASSERT(object);
    PV_ASSERT(synthesize_params);

    pv_orca_stream_state_reset(object);

    pv_status_t status = pv_orca_stream_state_prepare_buffers(object);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_stream_state_prepare_buffers,
                pv_status_to_string(status));
        return status;
    }

    status = pv_orca_stream_state_set_synthesize_params(object, synthesize_params);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_stream_state_set_synthesize_params,
                pv_status_to_string(status));
        return status;
    }

    object->status = PV_ORCA_STREAM_STATUS_ACTIVE;

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_orca_stream_state_refresh)(pv_orca_stream_state_t *object) {
    PV_ASSERT(object);

    pv_orca_stream_state_reset(object);
    object->status = PV_ORCA_STREAM_STATUS_ACTIVE;

    return pv_orca_stream_state_prepare_buffers(object);
}

void PV_MOCKABLE(pv_orca_stream_state_close)(pv_orca_stream_state_t *object) {
    if (object) {
        pv_orca_stream_state_reset(object);
        pv_orca_synthesize_params_delete(object->synthesize_params);
        object->synthesize_params = NULL;
    }
}

void PV_MOCKABLE(pv_orca_stream_state_delete)(pv_ypu_t *ypu, pv_orca_stream_state_t *object) {
    PV_ASSERT(ypu);

    if (object) {
        pv_ypu_mem_free(ypu, object->cached_z);
        pv_ypu_mem_free(ypu, object->cached_z_prior);
        pv_buffer_int32_delete(object->buffer_encoded_phonemes);
        pv_orca_stream_config_delete(ypu, (pv_orca_stream_config_t *) object->config);
        pv_ypu_host_free(ypu, object);
    }
}

void PV_MOCKABLE(pv_orca_stream_state_count_num_characters)(pv_orca_stream_state_t *object, const char *text) {
    PV_ASSERT(object);
    PV_ASSERT(text);

    object->num_characters_to_report += (int32_t) strlen(text);
}

int32_t PV_MOCKABLE(pv_orca_stream_state_flush_num_characters)(pv_orca_stream_state_t *object) {
    PV_ASSERT(object);

    int32_t num_characters_to_report = object->num_characters_to_report;
    object->num_characters_to_report = 0;

    return num_characters_to_report;
}

int32_t PV_MOCKABLE(pv_orca_stream_state_index_past_cutoff)(const pv_orca_stream_state_t *object) {
    PV_ASSERT(object);

    const pv_orca_stream_config_t *config = object->config;

    int32_t index_last_cutoff = -1;
    int32_t *encoded_phonemes = pv_buffer_int32_get(object->buffer_encoded_phonemes);
    int32_t num_encoded_phonemes = pv_buffer_int32_length(object->buffer_encoded_phonemes);

    if (num_encoded_phonemes > config->min_num_phonemes_kept) {
        for (int32_t i = num_encoded_phonemes - config->min_num_phonemes_kept; i >= 0; i--) {
            for (int32_t j = 0; j < config->num_eos_punctuation_indices; j++) {
                if (encoded_phonemes[i] == config->eos_punctuation_indices[j]) {
                    index_last_cutoff = i + 1;
                    if (config->word_boundary_index > 0) {
                        index_last_cutoff++;
                    }
                    break;
                }
            }
        }
    }

    if ((num_encoded_phonemes - index_last_cutoff) > config->max_num_phonemes_kept) {

        int32_t start_index =
                num_encoded_phonemes - (config->max_num_phonemes_kept - config->num_phonemes_remove_fallback);
        for (int32_t j = 0; j < config->num_fallback_cutoff_characters_indices; j++) {

            if (index_last_cutoff != -1) {
                break;
            }

            for (int32_t i = start_index; i >= 0; i--) {
                if (encoded_phonemes[i] == config->fallback_cutoff_characters_indices[j]) {
                    index_last_cutoff = i + 1;
                    if (config->word_boundary_index > 0) {
                        index_last_cutoff++;
                    }
                    break;
                }
            }
        }

        if ((num_encoded_phonemes - index_last_cutoff) > config->max_num_phonemes_kept) {
            if (config->word_boundary_index > 0) {
                for (int32_t i = start_index; i >= 0; i--) {
                    if (encoded_phonemes[i] == config->word_boundary_index) {
                        index_last_cutoff = i + 1;
                        break;
                    }
                }
            } else {
                index_last_cutoff = config->max_num_phonemes_kept - config->num_phonemes_remove_fallback;
            }
        }
        PV_ASSERT(index_last_cutoff != -1);
    }

    return index_last_cutoff;
}

pv_status_t PV_MOCKABLE(pv_orca_stream_state_append_encoded_phonemes)(
        pv_orca_stream_state_t *object,
        int32_t new_num_encoded_phonemes,
        const int32_t *new_encoded_phonemes) {
    PV_ASSERT(object);
    PV_ASSERT(new_num_encoded_phonemes > 0);
    PV_ASSERT(new_encoded_phonemes);

    int32_t index_cutoff = pv_orca_stream_state_index_past_cutoff(object);
    if (index_cutoff > 0) {
        ORCA_LOG_VERBOSE(
                "[STREAM STATE] Pruning encoded phonemes. keep indices from %d to %d",
                index_cutoff,
                pv_buffer_int32_length(object->buffer_encoded_phonemes));
        pv_status_t status = pv_buffer_int32_remove_from_start(
                object->buffer_encoded_phonemes,
                index_cutoff);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_buffer_int32_remove_from_start,
                    pv_status_to_string(status));
            return status;
        }

        object->end_index_dp -= index_cutoff;
        PV_ASSERT(object->end_index_dp >= 0);
    }

    pv_status_t status = pv_buffer_int32_append(
            object->buffer_encoded_phonemes,
            new_num_encoded_phonemes,
            new_encoded_phonemes);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_buffer_int32_append,
                pv_status_to_string(status));
        return status;
    }

    object->num_non_synthesized_encoded_phonemes += new_num_encoded_phonemes;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_orca_stream_state_update_encoder_state)(
        pv_orca_stream_state_t *object,
        int32_t num_encoded_phonemes,
        int32_t *num_encoded_phonemes_to_dp,
        int32_t *num_encoded_phonemes_to_flow) {
    PV_ASSERT(object);
    PV_ASSERT(num_encoded_phonemes > 0);
    PV_ASSERT(num_encoded_phonemes_to_dp);
    PV_ASSERT(num_encoded_phonemes_to_flow);

    const pv_orca_stream_config_t *config = object->config;

    object->start_index_dp = object->end_index_dp;

    if (!object->is_flush) {
        object->end_index_dp = num_encoded_phonemes - config->receptive_field_text_encoder;
        if (object->end_index_dp > num_encoded_phonemes) {
            object->end_index_dp = num_encoded_phonemes;
        }

        object->end_index_flow = object->end_index_dp - config->receptive_field_duration_predictor;
    } else {
        object->end_index_dp = num_encoded_phonemes;
        object->end_index_flow = num_encoded_phonemes;
    }

    if (object->is_first_chunk) {
        object->start_index_dp = 0;
        object->start_index_flow = 0;
    } else {
        object->start_index_dp = object->start_index_dp - (2 * config->receptive_field_duration_predictor);
        object->start_index_flow = object->start_index_dp + config->receptive_field_duration_predictor;
    }

    object->durations_offset = 0;
    if (object->status == PV_ORCA_STREAM_STATUS_ACTIVE && !object->is_first_chunk) {
        object->durations_offset = config->receptive_field_duration_predictor;
    }

    *num_encoded_phonemes_to_dp = object->end_index_dp - object->start_index_dp;
    *num_encoded_phonemes_to_flow = object->end_index_flow - object->start_index_flow;

    PV_ASSERT(*num_encoded_phonemes_to_dp >= 0);
    PV_ASSERT(*num_encoded_phonemes_to_flow >= 0);
}

void PV_MOCKABLE(pv_orca_stream_state_update_chunk_state)(pv_orca_stream_state_t *object) {
    PV_ASSERT(object);

    object->num_non_synthesized_encoded_phonemes = 0;
    object->num_processed_chunks += 1;
}

bool PV_MOCKABLE(pv_orca_stream_state_is_sufficient_context)(const pv_orca_stream_state_t *object) {
    PV_ASSERT(object);

    if (object->num_processed_chunks == 0) {
        return object->num_non_synthesized_encoded_phonemes >= object->config->size_chunk_phoneme_tokens;
    } else {
        return object->num_non_synthesized_encoded_phonemes >
               object->config->receptive_field_text_encoder + object->config->receptive_field_duration_predictor;
    }
}

pv_status_t PV_MOCKABLE(pv_orca_stream_state_update_z_prior)(
        pv_ypu_t *ypu,
        pv_orca_stream_state_t *object,
        pv_ypu_mem_t **buffer_z_prior,
        int32_t *num_frames_to_flow) {
    PV_ASSERT(ypu);
    PV_ASSERT(object);
    PV_ASSERT(buffer_z_prior);
    PV_ASSERT(*buffer_z_prior);
    PV_ASSERT(num_frames_to_flow);
    PV_ASSERT(*num_frames_to_flow > 0);

    int32_t num_frames = *num_frames_to_flow;

    if (object->is_first_chunk) {
        pv_status_t status = cache_z_prior(ypu, object, num_frames, *buffer_z_prior);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    cache_z_prior,
                    pv_status_to_string(status));
            return status;
        }

        return PV_STATUS_SUCCESS;
    }

    const int32_t cached_z_prior_length = 2 * object->config->receptive_field_flow;
    const int32_t num_channels = object->num_channels;

    *num_frames_to_flow += cached_z_prior_length;

    pv_ypu_mem_t *buffer_z_prior_concat = pv_ypu_buffer_get(
            ypu,
            *num_frames_to_flow * num_channels * (int32_t) sizeof(float),
            false);
    if (!buffer_z_prior_concat) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("pv_buffer_get"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_op_memcpy_args_t args0 = {
            .output = buffer_z_prior_concat,
            .input = object->cached_z_prior,
            .size_bytes = cached_z_prior_length * num_channels * (int32_t) sizeof(float),
            .output_offset = 0,
            .input_offset = 0,
    };

    pv_status_t status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_MEMCPY,
            &args0);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_op_memcpy_args_t args1 = {
            .output = buffer_z_prior_concat,
            .input = *buffer_z_prior,
            .size_bytes = num_frames * num_channels * (int32_t) sizeof(float),
            .output_offset = cached_z_prior_length * num_channels * (int32_t) sizeof(float),
            .input_offset = 0,
    };

    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_MEMCPY,
            &args1);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = cache_z_prior(ypu, object, num_frames, *buffer_z_prior);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                cache_z_prior,
                pv_status_to_string(status));
        return status;
    }

    pv_ypu_buffer_release(ypu, *buffer_z_prior);
    *buffer_z_prior = buffer_z_prior_concat;

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_orca_stream_state_update_z)(
        pv_ypu_t *ypu,
        pv_orca_stream_state_t *object,
        int32_t num_frames,
        pv_ypu_mem_t **buffer_z,
        int32_t *num_frames_to_voc) {
    PV_ASSERT(ypu);
    PV_ASSERT(object);
    PV_ASSERT(num_frames > 0);
    PV_ASSERT(buffer_z);
    PV_ASSERT(*buffer_z);
    PV_ASSERT(num_frames_to_voc);

    const pv_orca_stream_config_t *config = object->config;

    if (object->is_first_chunk) {
        pv_status_t status = cache_z(ypu, object, num_frames, *buffer_z);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    cache_z,
                    pv_status_to_string(status));
            return status;
        }

        if (object->is_flush) {
            *num_frames_to_voc = num_frames;
        } else {
            *num_frames_to_voc = num_frames - config->receptive_field_flow;
        }

        return PV_STATUS_SUCCESS;
    }

    const int32_t cached_z_length = 2 * object->config->receptive_field_vocoder;
    const int32_t num_channels = object->num_channels;

    if (object->is_flush) {
        *num_frames_to_voc = num_frames - config->receptive_field_flow + cached_z_length;
    } else {
        *num_frames_to_voc = num_frames - (2 * config->receptive_field_flow) + cached_z_length;
    }

    pv_ypu_mem_t *buffer_z_concat = pv_ypu_buffer_get(
            ypu,
            *num_frames_to_voc * num_channels * (int32_t) sizeof(float),
            false);
    if (!buffer_z_concat) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("pv_buffer_get"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_op_memcpy_args_t args0 = {
            .output = buffer_z_concat,
            .input = object->cached_z,
            .size_bytes = cached_z_length * num_channels * (int32_t) sizeof(float),
            .output_offset = 0,
            .input_offset = 0,
    };

    pv_status_t status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_MEMCPY,
            &args0);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    if (object->is_flush) {
        pv_ypu_op_memcpy_args_t args1 = {
                .output = buffer_z_concat,
                .input = *buffer_z,
                .size_bytes = (num_frames - config->receptive_field_flow) * num_channels * (int32_t) sizeof(float),
                .output_offset = cached_z_length * num_channels * (int32_t) sizeof(float),
                .input_offset = config->receptive_field_flow * num_channels * (int32_t) sizeof(float),
        };

        status = pv_ypu_operator_execute(
                ypu,
                PV_YPU_OPERATOR_MEMCPY,
                &args1);
        if (status != PV_STATUS_SUCCESS) {
            return status;
        }
    } else {
        pv_ypu_op_memcpy_args_t args1 = {
                .output = buffer_z_concat,
                .input = *buffer_z,
                .size_bytes = (num_frames - (2 * config->receptive_field_flow)) * num_channels * (int32_t) sizeof(float),
                .output_offset = cached_z_length * num_channels * (int32_t) sizeof(float),
                .input_offset = config->receptive_field_flow * num_channels * (int32_t) sizeof(float),
        };

        status = pv_ypu_operator_execute(
                ypu,
                PV_YPU_OPERATOR_MEMCPY,
                &args1);
        if (status != PV_STATUS_SUCCESS) {
            return status;
        }
    }

    status = cache_z(ypu, object, num_frames, *buffer_z);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                cache_z,
                pv_status_to_string(status));
        return status;
    }

    pv_ypu_buffer_release(ypu, *buffer_z);
    *buffer_z = buffer_z_concat;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_orca_stream_state_update_pcm_chunk)(
        pv_orca_stream_state_t *object,
        int32_t window_shift,
        int32_t num_samples,
        int32_t *num_samples_chunk,
        int32_t *pcm_offset) {

    const pv_orca_stream_config_t *config = object->config;

    *num_samples_chunk = num_samples;

    if (object->is_first_chunk) {
        if (!object->is_flush) {
            *num_samples_chunk -= config->receptive_field_vocoder * window_shift;
        }
    } else {
        *pcm_offset = config->receptive_field_vocoder * window_shift;

        if (object->is_flush) {
            *num_samples_chunk -= (config->receptive_field_vocoder * window_shift);
        } else {
            *num_samples_chunk -= (2 * config->receptive_field_vocoder * window_shift);
        }
    }

    if (*num_samples_chunk < 0) {
        *num_samples_chunk = 0;
        *pcm_offset = 0;
    }
}
