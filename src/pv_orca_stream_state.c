#include <stdlib.h>
#include <string.h>

#include "core/pv_error_messages.h"
#include "core/pv_type.h"
#include "orca/pv_orca_internal.h"
#include "orca/pv_orca_stream_state.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

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

    o->N_domain_lookahead = synthesizer_param->N_domain_lookahead;
    o->N_domain_lookback = synthesizer_param->N_domain_lookback;
    o->lfm_film_generator_lookahead = synthesizer_param->lfm_film_generator_lookahead;
    o->lfm_film_generator_lookback = synthesizer_param->lfm_film_generator_lookback;
    o->lfm_vf_estimator_lookahead = synthesizer_param->lfm_vf_estimator_lookahead;
    o->lfm_vf_estimator_lookback = synthesizer_param->lfm_vf_estimator_lookback;
    o->vocoder_lookahead = synthesizer_param->vocoder_lookahead;
    o->vocoder_lookback = synthesizer_param->vocoder_lookback;
    o->T_domain_lookahead = o->lfm_film_generator_lookahead + o->lfm_vf_estimator_lookahead + o->vocoder_lookahead;
    o->T_domain_lookback = o->lfm_film_generator_lookback + o->lfm_vf_estimator_lookback + o->vocoder_lookback;

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
        pv_orca_stream_state_t **object) {
    PV_ASSERT(ypu);
    PV_ASSERT(synthesizer_param);
    PV_ASSERT(num_eos_punctuation_indices > 0);
    PV_ASSERT(eos_punctuation_indices);
    PV_ASSERT(num_fallback_cutoff_characters_indices > 0);
    PV_ASSERT(fallback_cutoff_characters_indices);
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

    const int32_t T_domain_input_dimension = synthesizer_param->gaussian_upsampler_param->dimension;
    const int32_t lfm_x_t_dimension = synthesizer_param->lfm_vf_estimator_param->out_dimension;

    status = pv_buffer_init(1, &(o->cached_N_domain));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_stream_state_delete(ypu, o);
        return status;
    }

    status = pv_buffer_init(1, &(o->buffer_N_domain_concat));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_stream_state_delete(ypu, o);
        return status;
    }

    status = pv_buffer_ypu_init(ypu, T_domain_input_dimension, &(o->cached_T_domain));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_stream_state_delete(ypu, o);
        return status;
    }

    status = pv_buffer_ypu_init(ypu, T_domain_input_dimension, &(o->buffer_T_domain_concat));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_stream_state_delete(ypu, o);
        return status;
    }

    status = pv_buffer_ypu_init(ypu, 1, &(o->cached_bucket));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_stream_state_delete(ypu, o);
        return status;
    }

    status = pv_buffer_ypu_init(ypu, 1, &(o->buffer_bucket_concat));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_stream_state_delete(ypu, o);
        return status;
    }

    status = pv_buffer_ypu_init(ypu, lfm_x_t_dimension, &(o->cached_lfm_x_t));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_stream_state_delete(ypu, o);
        return status;
    }

    status = pv_buffer_ypu_init(ypu, lfm_x_t_dimension, &(o->buffer_lfm_x_t_concat));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_stream_state_delete(ypu, o);
        return status;
    }

    o->synthesize_params = NULL;

    pv_orca_stream_state_reset(ypu, o);

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_orca_stream_state_reset)(
        pv_ypu_t *ypu,
        pv_orca_stream_state_t *object) {
    PV_ASSERT(ypu);
    PV_ASSERT(object);

    object->num_characters_to_report = 0;

    object->status = PV_ORCA_STREAM_STATUS_INACTIVE;

    object->is_first_chunk = true;
    object->is_flush = false;

    pv_buffer_free(object->buffer_N_domain_concat);
    pv_buffer_free(object->cached_N_domain);
    pv_buffer_ypu_free(ypu, object->buffer_T_domain_concat);
    pv_buffer_ypu_free(ypu, object->cached_T_domain);
    pv_buffer_ypu_free(ypu, object->buffer_bucket_concat);
    pv_buffer_ypu_free(ypu, object->cached_bucket);
    pv_buffer_ypu_free(ypu, object->buffer_lfm_x_t_concat);
    pv_buffer_ypu_free(ypu, object->cached_lfm_x_t);

    object->num_processed_chunks_N_domain = 0;
    object->num_processed_chunks_T_domain = 0;

    object->N_domain_garbage_lookback_offset = 0;
    object->N_domain_garbage_lookahead_offset = 0;
    object->T_domain_garbage_lookback_offset = 0;
    object->T_domain_garbage_lookahead_offset = 0;

    object->cached_prev_N_domain = false;

    object->bucket_offset = 0;
}

pv_status_t PV_MOCKABLE(pv_orca_stream_state_open)(
        pv_ypu_t *ypu,
        pv_orca_stream_state_t *object,
        const pv_orca_synthesize_params_t *synthesize_params) {
    PV_ASSERT(ypu);
    PV_ASSERT(object);
    PV_ASSERT(synthesize_params);

    pv_orca_stream_state_reset(ypu, object);

    pv_status_t status = pv_orca_stream_state_set_synthesize_params(object, synthesize_params);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_stream_state_set_synthesize_params,
                pv_status_to_string(status));
        return status;
    }

    object->status = PV_ORCA_STREAM_STATUS_ACTIVE;

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_orca_stream_state_refresh)(
        pv_ypu_t *ypu,
        pv_orca_stream_state_t *object) {
    PV_ASSERT(ypu);
    PV_ASSERT(object);

    pv_orca_stream_state_reset(ypu, object);
    object->status = PV_ORCA_STREAM_STATUS_ACTIVE;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_orca_stream_state_close)(
        pv_ypu_t *ypu,
        pv_orca_stream_state_t *object) {
    PV_ASSERT(ypu);

    if (object) {
        pv_orca_stream_state_reset(ypu, object);
        pv_orca_synthesize_params_delete(object->synthesize_params);
        object->synthesize_params = NULL;
    }
}

void PV_MOCKABLE(pv_orca_stream_state_delete)(
        pv_ypu_t *ypu,
        pv_orca_stream_state_t *object) {
    PV_ASSERT(ypu);

    if (object) {
        pv_buffer_ypu_delete(ypu, object->buffer_lfm_x_t_concat);
        pv_buffer_ypu_delete(ypu, object->cached_lfm_x_t);
        pv_buffer_ypu_delete(ypu, object->buffer_bucket_concat);
        pv_buffer_ypu_delete(ypu, object->cached_bucket);
        pv_buffer_ypu_delete(ypu, object->buffer_T_domain_concat);
        pv_buffer_ypu_delete(ypu, object->cached_T_domain);
        pv_buffer_delete(object->buffer_N_domain_concat);
        pv_buffer_delete(object->cached_N_domain);
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

void PV_MOCKABLE(pv_orca_stream_state_update_pcm_chunk)(
        pv_orca_stream_state_t *object,
        int32_t window_shift,
        int32_t num_samples,
        int32_t *num_samples_chunk,
        int32_t *pcm_offset) {
    const int32_t clamped_garbage_lookback = pv_min_int32(object->config->vocoder_lookback, object->T_domain_garbage_lookback_offset);
    const int32_t clamped_garbage_lookahead = pv_min_int32(object->config->vocoder_lookahead, object->T_domain_garbage_lookahead_offset);
    *num_samples_chunk = num_samples - (clamped_garbage_lookback + clamped_garbage_lookahead) * window_shift;
    *pcm_offset = clamped_garbage_lookback * window_shift;

    if (*num_samples_chunk < 0) {
        *num_samples_chunk = 0;
        *pcm_offset = 0;
    }
}

pv_status_t PV_MOCKABLE(pv_orca_stream_state_is_sufficient_context_n_domain)(
        pv_orca_stream_state_t *object,
        int32_t num_encoded_phonemes,
        const int32_t *encoded_phonemes,
        bool *is_sufficient) {
    PV_ASSERT(object);
    PV_ASSERT(num_encoded_phonemes >= 0);
    if (num_encoded_phonemes > 0) {
        PV_ASSERT(encoded_phonemes);
    } else {
        PV_ASSERT(encoded_phonemes == NULL);
    }
    PV_ASSERT(is_sufficient);

    *is_sufficient = false;

    pv_status_t status;
    if ((object->num_processed_chunks_N_domain == 0) || (object->cached_prev_N_domain == false)) {
        status = pv_buffer_concat(
                object->buffer_N_domain_concat,
                encoded_phonemes,
                num_encoded_phonemes * ((int32_t) sizeof(int32_t)));
        object->cached_prev_N_domain = false;
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_buffer_concat,
                    pv_status_to_string(status));
            return status;
        }
    } else {
        status = pv_buffer_replace(
                object->buffer_N_domain_concat,
                encoded_phonemes,
                num_encoded_phonemes * ((int32_t) sizeof(int32_t)));
        object->cached_prev_N_domain = false;
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_buffer_replace,
                    pv_status_to_string(status));
            return status;
        }
    }

    // In theory we could have generate one-by-one in streaming, but we are adhering to the old Orca scheme of
    // chunk generation as opposed to frame-by-frame generation, because the total inference speed is much faster.
    int32_t buffer_N_domain_length = (pv_buffer_length(object->buffer_N_domain_concat) / ((int32_t) sizeof(int32_t)));
    *is_sufficient = (buffer_N_domain_length >= object->config->N_domain_lookahead + 1);

    return PV_STATUS_SUCCESS;
}

static pv_status_t cache_n_domain(pv_orca_stream_state_t *object) {
    PV_ASSERT(object);

    if (object->is_flush) {
        pv_buffer_free(object->cached_N_domain);
        return PV_STATUS_SUCCESS;
    }

    const pv_orca_stream_config_t *config = object->config;

    const int32_t dimension = pv_buffer_dimension(object->buffer_N_domain_concat);
    PV_ASSERT(dimension == pv_buffer_dimension(object->cached_N_domain));
    PV_ASSERT(dimension == 1);

    const int32_t buffer_N_domain_concat_length = pv_buffer_length(object->buffer_N_domain_concat) / ((int32_t) sizeof(int32_t));
    PV_ASSERT(buffer_N_domain_concat_length > 0);

    int32_t length_cached_N_domain = (config->N_domain_lookback + config->N_domain_lookahead);
    int32_t buffer_N_domain_concat_offset = (buffer_N_domain_concat_length - length_cached_N_domain);
    if (buffer_N_domain_concat_offset < 0) {
        length_cached_N_domain += buffer_N_domain_concat_offset;
        buffer_N_domain_concat_offset = 0;
    }

    int32_t *buffer_N_domain_concat = pv_buffer_get(
            object->buffer_N_domain_concat,
            buffer_N_domain_concat_length * ((int32_t) sizeof(int32_t)),
            false);
    pv_buffer_replace(
            object->cached_N_domain,
            buffer_N_domain_concat + buffer_N_domain_concat_offset * dimension,
            length_cached_N_domain * ((int32_t) sizeof(int32_t)));


    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_orca_stream_state_update_n_domain)(
        pv_orca_stream_state_t *object,
        int32_t *num_to_N_domain) {
    PV_ASSERT(object);
    PV_ASSERT(num_to_N_domain);

    const int32_t cached_N_domain_length = pv_buffer_length(object->cached_N_domain) / ((int32_t) sizeof(int32_t));
    const int32_t buffer_N_domain_concat_length = pv_buffer_length(object->buffer_N_domain_concat) / ((int32_t) sizeof(int32_t));
    const int32_t dimension = pv_buffer_dimension(object->buffer_N_domain_concat);
    PV_ASSERT(dimension == pv_buffer_dimension(object->cached_N_domain));
    PV_ASSERT(dimension == 1);

    int32_t buffer_result_length = 0;
    buffer_result_length += cached_N_domain_length;
    buffer_result_length += buffer_N_domain_concat_length;

    PV_ASSERT(buffer_result_length >= 0);

    PV_ASSERT((cached_N_domain_length >= object->config->N_domain_lookahead) || (object->num_processed_chunks_N_domain == 0));
    object->N_domain_garbage_lookback_offset = pv_max_int32(cached_N_domain_length - object->config->N_domain_lookahead, 0);
    object->N_domain_garbage_lookahead_offset = (object->is_flush) ? 0 : object->config->N_domain_lookahead;

    int32_t *buffer_result = NULL;
    if (buffer_result_length > 0) {
        buffer_result = calloc(
                buffer_result_length * dimension,
                sizeof(int32_t));
        if (!buffer_result) {
            return PV_STATUS_OUT_OF_MEMORY;
        }

        if (cached_N_domain_length > 0) {
            pv_buffer_copy_to(
                    object->cached_N_domain,
                    buffer_result,
                    cached_N_domain_length * ((int32_t) sizeof(int32_t)));
        }

        if (buffer_N_domain_concat_length > 0) {
            pv_buffer_copy_to(
                    object->buffer_N_domain_concat,
                    buffer_result + cached_N_domain_length * dimension,
                    buffer_N_domain_concat_length * ((int32_t) sizeof(int32_t)));
        }
    }

    pv_status_t status = pv_buffer_replace(
            object->buffer_N_domain_concat,
            buffer_result,
            buffer_result_length * ((int32_t) sizeof(int32_t)));
    free(buffer_result);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_buffer_replace,
                pv_status_to_string(status));
        return status;
    }

    *num_to_N_domain = buffer_result_length;

    status = cache_n_domain(object);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                cache_n_domain,
                pv_status_to_string(status));
        return status;
    }

    object->cached_prev_N_domain = true;
    ++(object->num_processed_chunks_N_domain);

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_orca_stream_state_is_sufficient_context_t_domain)(
        pv_ypu_t *ypu,
        const pv_orca_stream_state_t *object,
        int32_t T,
        int32_t N_to_T_garbage_lookback_offset,
        int32_t N_to_T_garbage_lookahead_offset,
        pv_ypu_mem_t *buffer_gaussian_upsampled,
        pv_ypu_mem_t *buffer_bucket,
        pv_ypu_mem_t *buffer_lfm_x_t,
        bool *is_sufficient) {
    PV_ASSERT(ypu);
    PV_ASSERT(object);
    PV_ASSERT(T >= 0);
    PV_ASSERT(N_to_T_garbage_lookback_offset >= 0);
    PV_ASSERT(N_to_T_garbage_lookahead_offset >= 0);
    if (T > 0) {
        PV_ASSERT(buffer_gaussian_upsampled);
        PV_ASSERT(buffer_bucket);
        PV_ASSERT(buffer_lfm_x_t);
    }
    PV_ASSERT(is_sufficient);

    const int32_t T_cleaned = T - N_to_T_garbage_lookback_offset - N_to_T_garbage_lookahead_offset;
    PV_ASSERT(T_cleaned >= 0);

    const int32_t dimension_T_domain_concat = pv_buffer_ypu_dimension(object->buffer_T_domain_concat);
    const int32_t dimension_bucket_concat = pv_buffer_ypu_dimension(object->buffer_bucket_concat);
    PV_ASSERT(dimension_bucket_concat == 1);
    const int32_t dimension_lfm_x_t_concat = pv_buffer_ypu_dimension(object->buffer_lfm_x_t_concat);

    pv_status_t status;

    if (object->num_processed_chunks_T_domain == 0) {
        if (T_cleaned > 0) {
            status = pv_buffer_ypu_concat(
                    ypu,
                    object->buffer_T_domain_concat,
                    buffer_gaussian_upsampled,
                    N_to_T_garbage_lookback_offset * dimension_T_domain_concat,
                    T_cleaned);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_buffer_concat,
                        pv_status_to_string(status));
                return status;
            }

            status = pv_buffer_ypu_concat(
                    ypu,
                    object->buffer_bucket_concat,
                    buffer_bucket,
                    N_to_T_garbage_lookback_offset * dimension_bucket_concat,
                    T_cleaned);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_buffer_concat,
                        pv_status_to_string(status));
                return status;
            }

            status = pv_buffer_ypu_concat(
                    ypu,
                    object->buffer_lfm_x_t_concat,
                    buffer_lfm_x_t,
                    N_to_T_garbage_lookback_offset * dimension_lfm_x_t_concat,
                    T_cleaned);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_buffer_concat,
                        pv_status_to_string(status));
                return status;
            }
        }
        const int32_t buffer_T_domain_concat_length = pv_buffer_ypu_length(object->buffer_T_domain_concat);
        PV_ASSERT(pv_buffer_ypu_length(object->buffer_bucket_concat) == buffer_T_domain_concat_length);
        PV_ASSERT(pv_buffer_ypu_length(object->buffer_lfm_x_t_concat) == buffer_T_domain_concat_length);
        *is_sufficient = (buffer_T_domain_concat_length >= object->config->T_domain_lookahead + 13);
    } else {
        status = pv_buffer_ypu_replace(
                ypu,
                object->buffer_T_domain_concat,
                buffer_gaussian_upsampled,
                N_to_T_garbage_lookback_offset * dimension_T_domain_concat,
                T_cleaned);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_buffer_replace,
                    pv_status_to_string(status));
            return status;
        }

        status = pv_buffer_ypu_replace(
                ypu,
                object->buffer_bucket_concat,
                buffer_bucket,
                N_to_T_garbage_lookback_offset * dimension_bucket_concat,
                T_cleaned);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_buffer_replace,
                    pv_status_to_string(status));
            return status;
        }

        status = pv_buffer_ypu_replace(
                ypu,
                object->buffer_lfm_x_t_concat,
                buffer_lfm_x_t,
                N_to_T_garbage_lookback_offset * dimension_lfm_x_t_concat,
                T_cleaned);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_buffer_replace,
                    pv_status_to_string(status));
            return status;
        }

        *is_sufficient = (T_cleaned > 0);
    }

    return PV_STATUS_SUCCESS;
}

static pv_status_t cache_t_domain(
        pv_ypu_t *ypu,
        pv_orca_stream_state_t *object) {
    PV_ASSERT(object);

    if (object->is_flush) {
        pv_buffer_ypu_free(ypu, object->cached_T_domain);
        pv_buffer_ypu_free(ypu, object->cached_bucket);
        pv_buffer_ypu_free(ypu, object->cached_lfm_x_t);
        return PV_STATUS_SUCCESS;
    }

    const pv_orca_stream_config_t *config = object->config;

    const int32_t dimension_T_domain = pv_buffer_ypu_dimension(object->buffer_T_domain_concat);
    PV_ASSERT(dimension_T_domain == pv_buffer_ypu_dimension(object->cached_T_domain));
    const int32_t dimension_bucket = pv_buffer_ypu_dimension(object->buffer_bucket_concat);
    PV_ASSERT(dimension_bucket == pv_buffer_ypu_dimension(object->cached_bucket));
    PV_ASSERT(dimension_bucket == 1);
    const int32_t dimension_lfm_x_t = pv_buffer_ypu_dimension(object->buffer_lfm_x_t_concat);
    PV_ASSERT(dimension_lfm_x_t == pv_buffer_ypu_dimension(object->cached_lfm_x_t));

    const int32_t buffer_T_domain_concat_length = pv_buffer_ypu_length(object->buffer_T_domain_concat);
    PV_ASSERT(buffer_T_domain_concat_length > 0);
    const int32_t buffer_bucket_concat_length = pv_buffer_ypu_length(object->buffer_bucket_concat);
    PV_ASSERT(buffer_bucket_concat_length > 0);
    const int32_t buffer_lfm_x_t_concat_length = pv_buffer_ypu_length(object->buffer_lfm_x_t_concat);
    PV_ASSERT(buffer_lfm_x_t_concat_length > 0);
    PV_ASSERT(buffer_T_domain_concat_length == buffer_lfm_x_t_concat_length);
    PV_ASSERT(buffer_T_domain_concat_length == buffer_bucket_concat_length);

    int32_t length_cached_T_domain = (config->T_domain_lookback + config->T_domain_lookahead);
    int32_t buffer_T_domain_concat_offset = (buffer_T_domain_concat_length - length_cached_T_domain);
    if (buffer_T_domain_concat_offset < 0) {
        length_cached_T_domain += buffer_T_domain_concat_offset;
        buffer_T_domain_concat_offset = 0;
    }

    pv_status_t status = pv_buffer_ypu_replace(
            ypu,
            object->cached_T_domain,
            pv_buffer_ypu_get(ypu, object->buffer_T_domain_concat, buffer_T_domain_concat_length, false),
            buffer_T_domain_concat_offset * dimension_T_domain,
            length_cached_T_domain);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_buffer_replace,
                pv_status_to_string(status));
        return status;
    }

    status = pv_buffer_ypu_replace(
            ypu,
            object->cached_bucket,
            pv_buffer_ypu_get(ypu, object->buffer_bucket_concat, buffer_bucket_concat_length, false),
            buffer_T_domain_concat_offset * dimension_bucket,
            length_cached_T_domain);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_buffer_replace,
                pv_status_to_string(status));
        return status;
    }

    status = pv_buffer_ypu_replace(
            ypu,
            object->cached_lfm_x_t,
            pv_buffer_ypu_get(ypu, object->buffer_lfm_x_t_concat, buffer_lfm_x_t_concat_length, false),
            buffer_T_domain_concat_offset * dimension_lfm_x_t,
            length_cached_T_domain);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_buffer_replace,
                pv_status_to_string(status));
        return status;
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_orca_stream_state_update_t_domain)(
        pv_ypu_t *ypu,
        pv_orca_stream_state_t *object,
        int32_t *num_to_T_domain) {
    PV_ASSERT(ypu);
    PV_ASSERT(object);
    PV_ASSERT(num_to_T_domain);

    const int32_t cached_T_domain_length = pv_buffer_ypu_length(object->cached_T_domain);
    const int32_t cached_bucket_length = pv_buffer_ypu_length(object->cached_bucket);
    const int32_t cached_lfm_x_t_length = pv_buffer_ypu_length(object->cached_lfm_x_t);
    PV_ASSERT(cached_T_domain_length == cached_lfm_x_t_length);
    PV_ASSERT(cached_T_domain_length == cached_bucket_length);
    const int32_t buffer_T_domain_concat_length = pv_buffer_ypu_length(object->buffer_T_domain_concat);
    const int32_t buffer_bucket_concat_length = pv_buffer_ypu_length(object->buffer_bucket_concat);
    const int32_t buffer_lfm_x_t_concat_length = pv_buffer_ypu_length(object->buffer_lfm_x_t_concat);
    PV_ASSERT(buffer_T_domain_concat_length == buffer_lfm_x_t_concat_length);
    PV_ASSERT(buffer_T_domain_concat_length == buffer_bucket_concat_length);
    const int32_t dimension_T_domain_concat = pv_buffer_ypu_dimension(object->buffer_T_domain_concat);
    PV_ASSERT(dimension_T_domain_concat == pv_buffer_ypu_dimension(object->cached_T_domain));
    const int32_t dimension_bucket_concat = pv_buffer_ypu_dimension(object->buffer_bucket_concat);
    PV_ASSERT(dimension_bucket_concat == pv_buffer_ypu_dimension(object->cached_bucket));
    PV_ASSERT(dimension_bucket_concat == 1);
    const int32_t dimension_lfm_x_t_concat = pv_buffer_ypu_dimension(object->buffer_lfm_x_t_concat);
    PV_ASSERT(dimension_lfm_x_t_concat == pv_buffer_ypu_dimension(object->cached_lfm_x_t));

    int32_t buffer_result_length = 0;
    buffer_result_length += cached_T_domain_length;
    buffer_result_length += buffer_T_domain_concat_length;

    PV_ASSERT(buffer_result_length >= 0);

    PV_ASSERT((cached_T_domain_length >= object->config->T_domain_lookahead) || (object->num_processed_chunks_T_domain == 0));
    object->T_domain_garbage_lookback_offset = pv_max_int32(cached_T_domain_length - object->config->T_domain_lookahead, 0);
    object->T_domain_garbage_lookahead_offset = (object->is_flush) ? 0 : object->config->T_domain_lookahead;

    pv_ypu_mem_t *buffer_result_T_domain = NULL;
    pv_ypu_mem_t *buffer_result_bucket = NULL;
    pv_ypu_mem_t *buffer_result_lfm_x_t = NULL;
    if (buffer_result_length > 0) {
        buffer_result_T_domain = pv_ypu_mem_alloc(
                ypu,
                buffer_result_length * dimension_T_domain_concat * (int32_t) sizeof(float),
                PV_YPU_DEVICE_MEM_FLAG_NONE);
        if (!buffer_result_T_domain) {
            return PV_STATUS_OUT_OF_MEMORY;
        }

        buffer_result_bucket = pv_ypu_mem_alloc(
                ypu,
                buffer_result_length * dimension_bucket_concat * (int32_t) sizeof(float),
                PV_YPU_DEVICE_MEM_FLAG_NONE);
        if (!buffer_result_bucket) {
            pv_ypu_mem_free(ypu, buffer_result_T_domain);
            return PV_STATUS_OUT_OF_MEMORY;
        }

        buffer_result_lfm_x_t = pv_ypu_mem_alloc(
                ypu,
                buffer_result_length * dimension_lfm_x_t_concat * (int32_t) sizeof(float),
                PV_YPU_DEVICE_MEM_FLAG_NONE);
        if (!buffer_result_lfm_x_t) {
            pv_ypu_mem_free(ypu, buffer_result_bucket);
            pv_ypu_mem_free(ypu, buffer_result_T_domain);
            return PV_STATUS_OUT_OF_MEMORY;
        }

        if (cached_T_domain_length > 0) {
            pv_status_t status = pv_buffer_ypu_copy_to(
                    ypu,
                    object->cached_T_domain,
                    buffer_result_T_domain,
                    0,
                    cached_T_domain_length);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_buffer_ypu_copy_to,
                        pv_status_to_string(status));
                pv_ypu_mem_free(ypu, buffer_result_T_domain);
                pv_ypu_mem_free(ypu, buffer_result_bucket);
                pv_ypu_mem_free(ypu, buffer_result_lfm_x_t);
                return status;
            }
            status = pv_buffer_ypu_copy_to(
                    ypu,
                    object->cached_bucket,
                    buffer_result_bucket,
                    0,
                    cached_bucket_length);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_buffer_ypu_copy_to,
                        pv_status_to_string(status));
                pv_ypu_mem_free(ypu, buffer_result_T_domain);
                pv_ypu_mem_free(ypu, buffer_result_bucket);
                pv_ypu_mem_free(ypu, buffer_result_lfm_x_t);
                return status;
            }
            status = pv_buffer_ypu_copy_to(
                    ypu,
                    object->cached_lfm_x_t,
                    buffer_result_lfm_x_t,
                    0,
                    cached_lfm_x_t_length);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_buffer_ypu_copy_to,
                        pv_status_to_string(status));
                pv_ypu_mem_free(ypu, buffer_result_T_domain);
                pv_ypu_mem_free(ypu, buffer_result_bucket);
                pv_ypu_mem_free(ypu, buffer_result_lfm_x_t);
                return status;
            }
        }

        if (buffer_T_domain_concat_length > 0) {
            pv_status_t status = pv_buffer_ypu_copy_to(
                    ypu,
                    object->buffer_T_domain_concat,
                    buffer_result_T_domain,
                    cached_T_domain_length * dimension_T_domain_concat,
                    buffer_T_domain_concat_length);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_buffer_ypu_copy_to,
                        pv_status_to_string(status));
                pv_ypu_mem_free(ypu, buffer_result_T_domain);
                pv_ypu_mem_free(ypu, buffer_result_bucket);
                pv_ypu_mem_free(ypu, buffer_result_lfm_x_t);
                return status;
            }
            status = pv_buffer_ypu_copy_to(
                    ypu,
                    object->buffer_bucket_concat,
                    buffer_result_bucket,
                    cached_bucket_length * dimension_bucket_concat,
                    buffer_bucket_concat_length);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_buffer_ypu_copy_to,
                        pv_status_to_string(status));
                pv_ypu_mem_free(ypu, buffer_result_T_domain);
                pv_ypu_mem_free(ypu, buffer_result_bucket);
                pv_ypu_mem_free(ypu, buffer_result_lfm_x_t);
                return status;
            }
            status = pv_buffer_ypu_copy_to(
                    ypu,
                    object->buffer_lfm_x_t_concat,
                    buffer_result_lfm_x_t,
                    cached_lfm_x_t_length * dimension_lfm_x_t_concat,
                    buffer_lfm_x_t_concat_length);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_buffer_ypu_copy_to,
                        pv_status_to_string(status));
                pv_ypu_mem_free(ypu, buffer_result_T_domain);
                pv_ypu_mem_free(ypu, buffer_result_bucket);
                pv_ypu_mem_free(ypu, buffer_result_lfm_x_t);
                return status;
            }
        }
    }

    pv_status_t status = pv_buffer_ypu_replace(
            ypu,
            object->buffer_T_domain_concat,
            buffer_result_T_domain,
            0,
            buffer_result_length);
    pv_ypu_mem_free(ypu, buffer_result_T_domain);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_buffer_replace,
                pv_status_to_string(status));
        pv_ypu_mem_free(ypu, buffer_result_bucket);
        pv_ypu_mem_free(ypu, buffer_result_lfm_x_t);
        return status;
    }

    status = pv_buffer_ypu_replace(
            ypu,
            object->buffer_bucket_concat,
            buffer_result_bucket,
            0,
            buffer_result_length);
    pv_ypu_mem_free(ypu, buffer_result_bucket);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_buffer_replace,
                pv_status_to_string(status));
        pv_ypu_mem_free(ypu, buffer_result_lfm_x_t);
        return status;
    }

    status = pv_buffer_ypu_replace(
            ypu,
            object->buffer_lfm_x_t_concat,
            buffer_result_lfm_x_t,
            0,
            buffer_result_length);
    pv_ypu_mem_free(ypu, buffer_result_lfm_x_t);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_buffer_replace,
                pv_status_to_string(status));
        return status;
    }

    *num_to_T_domain = buffer_result_length;

    status = cache_t_domain(ypu, object);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                cache_t_domain,
                pv_status_to_string(status));
        return status;
    }

    ++(object->num_processed_chunks_T_domain);

    return PV_STATUS_SUCCESS;
}
