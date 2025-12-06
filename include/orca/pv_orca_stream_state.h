#ifndef PV_ORCA_STREAM_STATE_H
#define PV_ORCA_STREAM_STATE_H

#include <stdlib.h>

#include "core/pv_language.h"
#include "orca/pv_buffer_int32.h"
#include "orca/pv_orca.h"
#include "orca/pv_orca_phonemizer.h"
#include "orca/pv_orca_text_encoder.h"
#include "orca/pv_orca_duration_predictor.h"
#include "orca/pv_orca_flow.h"
#include "orca/pv_orca_vocoder.h"
#include "ypu/pv_ypu.h"

typedef struct {
    const int32_t sample_rate;

    const pv_orca_text_encoder_param_t *text_encoder_param;
    const pv_orca_duration_predictor_param_t *duration_predictor_param;
    const pv_orca_flow_param_t *flow_param;
    const pv_orca_vocoder_param_t *vocoder_param;

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

    int32_t num_channels;

    int32_t num_characters_to_report;

    pv_buffer_int32_t *buffer_encoded_phonemes;

    pv_orca_stream_status_t status;

    bool is_first_chunk;
    bool is_flush;

    int32_t num_processed_chunks;
    int32_t num_non_synthesized_encoded_phonemes;

    int32_t start_index_dp;
    int32_t end_index_dp;
    int32_t durations_offset;

    int32_t start_index_flow;
    int32_t end_index_flow;

    pv_ypu_mem_t *cached_z_prior;
    pv_ypu_mem_t *cached_z;
} pv_orca_stream_state_t;

pv_status_t PV_MOCKABLE(pv_orca_stream_state_init)(
        pv_ypu_t *ypu,
        const pv_orca_synthesizer_param_t *synthesizer_param,
        const int32_t num_eos_punctuation_indices,
        int32_t *eos_punctuation_indices,
        const int32_t num_fallback_cutoff_characters_indices,
        int32_t *fallback_cutoff_characters_indices,
        const int32_t word_boundary_index,
        const int32_t text_buffer_size,
        pv_orca_stream_state_t **object);

pv_status_t PV_MOCKABLE(pv_orca_stream_state_refresh)(pv_orca_stream_state_t *object);

void PV_MOCKABLE(pv_orca_stream_state_reset)(pv_orca_stream_state_t *object);

pv_status_t PV_MOCKABLE(pv_orca_stream_state_open)(
        pv_orca_stream_state_t *object,
        const pv_orca_synthesize_params_t *synthesize_params);

void PV_MOCKABLE(pv_orca_stream_state_close)(pv_orca_stream_state_t *object);

void PV_MOCKABLE(pv_orca_stream_state_delete)(pv_ypu_t *ypu, pv_orca_stream_state_t *object);

void PV_MOCKABLE(pv_orca_stream_state_count_num_characters)(pv_orca_stream_state_t *object, const char *text);

int32_t PV_MOCKABLE(pv_orca_stream_state_flush_num_characters)(pv_orca_stream_state_t *object);

/* Looks for semantic markers to remove past context when the buffered text becomes too long, in order to
 * reduce the length of the text that is processed by the neural net.
 * Semantic markers are searched for in the following order:
 * 1. End-of-sentence punctuations.
 * 2. Mid-sentence punctuations.
 * 3. Word boundaries.
 */
int32_t PV_MOCKABLE(pv_orca_stream_state_index_past_cutoff)(const pv_orca_stream_state_t *object);

pv_status_t PV_MOCKABLE(pv_orca_stream_state_append_encoded_phonemes)(
        pv_orca_stream_state_t *object,
        int32_t new_num_encoded_phonemes,
        const int32_t *new_encoded_phonemes);

void PV_MOCKABLE(pv_orca_stream_state_update_encoder_state)(
        pv_orca_stream_state_t *object,
        int32_t num_encoded_phonemes,
        int32_t *num_encoded_phonemes_to_dp,
        int32_t *num_encoded_phonemes_to_flow);

void PV_MOCKABLE(pv_orca_stream_state_update_chunk_state)(pv_orca_stream_state_t *object);

bool PV_MOCKABLE(pv_orca_stream_state_is_sufficient_context)(const pv_orca_stream_state_t *object);

pv_status_t PV_MOCKABLE(pv_orca_stream_state_update_z_prior)(
        pv_ypu_t *ypu,
        pv_orca_stream_state_t *object,
        pv_ypu_mem_t **buffer_z_prior,
        int32_t *num_frames_to_flow);

pv_status_t PV_MOCKABLE(pv_orca_stream_state_update_z)(
        pv_ypu_t *ypu,
        pv_orca_stream_state_t *object,
        int32_t num_frames,
        pv_ypu_mem_t **buffer_z,
        int32_t *num_frames_to_voc);

void PV_MOCKABLE(pv_orca_stream_state_update_pcm_chunk)(
        pv_orca_stream_state_t *object,
        int32_t window_shift,
        int32_t num_samples,
        int32_t *num_samples_chunk,
        int32_t *pcm_offset);

#endif // PV_ORCA_STREAM_STATE_H
