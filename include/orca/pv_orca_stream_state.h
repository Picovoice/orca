#ifndef PV_ORCA_STREAM_STATE_H
#define PV_ORCA_STREAM_STATE_H

#include <stdlib.h>

#include "core/pv_language.h"
#include "orca/pv_buffer_ypu.h"
#include "orca/pv_orca.h"
#include "orca/pv_orca_duration_predictor.h"
#include "orca/pv_orca_gaussian_upsampler.h"
#include "orca/pv_orca_lfm_condition_fuser.h"
#include "orca/pv_orca_lfm_film_generator.h"
#include "orca/pv_orca_lfm_vf_estimator_param.h"
#include "orca/pv_orca_phonemizer.h"
#include "orca/pv_orca_prior_encoder_film_generator.h"
#include "orca/pv_orca_prior_encoder_flow.h"
#include "orca/pv_orca_vocoder.h"

typedef struct {
    const int32_t sample_rate;
    const int32_t lfm_time_embedding_dim;
    const int32_t N_domain_lookahead;
    const int32_t N_domain_lookback;
    const int32_t lfm_film_generator_lookahead;
    const int32_t lfm_film_generator_lookback;
    const int32_t lfm_vf_estimator_lookahead;
    const int32_t lfm_vf_estimator_lookback;
    const int32_t vocoder_lookahead;
    const int32_t vocoder_lookback;

    const pv_orca_prior_encoder_film_generator_param_t *prior_encoder_film_generator_param;
    const pv_orca_prior_encoder_flow_param_t *prior_encoder_flow_param;
    const pv_orca_duration_predictor_param_t *duration_predictor_param;
    const pv_orca_gaussian_upsampler_param_t *gaussian_upsampler_param;
    const pv_orca_lfm_film_generator_param_t *lfm_film_generator_param;
    const pv_orca_lfm_condition_fuser_param_t *lfm_condition_fuser_param;
    const pv_orca_lfm_vf_estimator_param_t *lfm_vf_estimator_param;
    const pv_orca_vocoder_param_t *vocoder_param;

    pv_ypu_config_mem_t *lfm_time_embedding_param;

    const char language_code[PV_LANGUAGE_CODE_LENGTH];
    const char *version;
} pv_orca_synthesizer_param_t;

typedef struct pv_orca_stream_config {
    int32_t num_eos_punctuation_indices;
    int32_t *eos_punctuation_indices;

    int32_t num_fallback_cutoff_characters_indices;
    int32_t *fallback_cutoff_characters_indices;
    int32_t word_boundary_index;

    int32_t receptive_field_text_encoder;
    int32_t receptive_field_duration_predictor;
    int32_t receptive_field_flow;
    int32_t receptive_field_vocoder;

    int32_t N_domain_lookahead;
    int32_t N_domain_lookback;
    int32_t lfm_film_generator_lookahead;
    int32_t lfm_film_generator_lookback;
    int32_t lfm_vf_estimator_lookahead;
    int32_t lfm_vf_estimator_lookback;
    int32_t vocoder_lookahead;
    int32_t vocoder_lookback;
    int32_t T_domain_lookahead;
    int32_t T_domain_lookback;

    int32_t size_chunk_phoneme_tokens;

    int32_t min_num_phonemes_kept;
    int32_t max_num_phonemes_kept;
    int32_t num_phonemes_remove_fallback;
} pv_orca_stream_config_t;

typedef enum {
    PV_ORCA_STREAM_STATUS_ACTIVE = 0,
    PV_ORCA_STREAM_STATUS_INACTIVE,
} pv_orca_stream_status_t;

typedef struct pv_orca_stream_state {
    const pv_orca_stream_config_t *config;

    pv_orca_synthesize_params_t *synthesize_params;

    int32_t num_characters_to_report;

    pv_orca_stream_status_t status;

    bool is_first_chunk;
    bool is_flush;

    pv_buffer_t *cached_N_domain;
    pv_buffer_t *buffer_N_domain_concat;

    pv_buffer_ypu_t *cached_T_domain;
    pv_buffer_ypu_t *buffer_T_domain_concat;

    pv_buffer_ypu_t *cached_bucket;
    pv_buffer_ypu_t *buffer_bucket_concat;

    pv_buffer_ypu_t *cached_lfm_x_t;
    pv_buffer_ypu_t *buffer_lfm_x_t_concat;

    int32_t num_processed_chunks_N_domain;
    int32_t num_processed_chunks_T_domain;

    int32_t N_domain_garbage_lookback_offset;
    int32_t N_domain_garbage_lookahead_offset;
    int32_t T_domain_garbage_lookback_offset;
    int32_t T_domain_garbage_lookahead_offset;

    bool cached_prev_N_domain;

    int32_t bucket_offset;
} pv_orca_stream_state_t;

pv_status_t PV_MOCKABLE(pv_orca_stream_state_init)(
        pv_ypu_t *ypu,
        const pv_orca_synthesizer_param_t *synthesizer_param,
        const int32_t num_eos_punctuation_indices,
        int32_t *eos_punctuation_indices,
        const int32_t num_fallback_cutoff_characters_indices,
        int32_t *fallback_cutoff_characters_indices,
        const int32_t word_boundary_index,
        pv_orca_stream_state_t **object);

pv_status_t PV_MOCKABLE(pv_orca_stream_state_refresh)(
        pv_ypu_t *ypu,
        pv_orca_stream_state_t *object);

void PV_MOCKABLE(pv_orca_stream_state_reset)(
        pv_ypu_t *ypu,
        pv_orca_stream_state_t *object);

pv_status_t PV_MOCKABLE(pv_orca_stream_state_open)(
        pv_ypu_t *ypu,
        pv_orca_stream_state_t *object,
        const pv_orca_synthesize_params_t *synthesize_params);

void PV_MOCKABLE(pv_orca_stream_state_close)(
        pv_ypu_t *ypu,
        pv_orca_stream_state_t *object);

void PV_MOCKABLE(pv_orca_stream_state_delete)(
        pv_ypu_t *ypu,
        pv_orca_stream_state_t *object);

void PV_MOCKABLE(pv_orca_stream_state_count_num_characters)(pv_orca_stream_state_t *object, const char *text);

int32_t PV_MOCKABLE(pv_orca_stream_state_flush_num_characters)(pv_orca_stream_state_t *object);

void PV_MOCKABLE(pv_orca_stream_state_update_pcm_chunk)(
        pv_orca_stream_state_t *object,
        int32_t window_shift,
        int32_t num_samples,
        int32_t *num_samples_chunk,
        int32_t *pcm_offset);

pv_status_t PV_MOCKABLE(pv_orca_stream_state_is_sufficient_context_n_domain)(
        pv_orca_stream_state_t *object,
        int32_t num_encoded_phonemes,
        const int32_t *encoded_phonemes,
        bool *is_sufficient);

pv_status_t PV_MOCKABLE(pv_orca_stream_state_update_n_domain)(
        pv_orca_stream_state_t *object,
        int32_t *num_to_N_domain);

pv_status_t PV_MOCKABLE(pv_orca_stream_state_is_sufficient_context_t_domain)(
        pv_ypu_t *ypu,
        const pv_orca_stream_state_t *object,
        int32_t T,
        int32_t N_to_T_garbage_lookback_offset,
        int32_t N_to_T_garbage_lookahead_offset,
        pv_ypu_mem_t *buffer_gaussian_upsampled,
        pv_ypu_mem_t *buffer_bucket,
        pv_ypu_mem_t *buffer_lfm_x_t,
        bool *is_sufficient);

pv_status_t PV_MOCKABLE(pv_orca_stream_state_update_t_domain)(
        pv_ypu_t *ypu,
        pv_orca_stream_state_t *object,
        int32_t *num_to_T_domain);

#endif // PV_ORCA_STREAM_STATE_H
