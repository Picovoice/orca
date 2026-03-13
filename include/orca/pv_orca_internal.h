#ifndef PV_ORCA_INTERNAL_H
#define PV_ORCA_INTERNAL_H

#include "core/pv_type.h"
#include "gatekeeper/pv_https_client.h"
#include "hippo/pv_hippo.h"
#include "io/pv_log.h"
#include "io/pv_serialized.h"
#include "normalizer/pv_normalizer.h"
#include "orca/pv_orca.h"
#include "orca/pv_orca_synthesizer.h"

#define PV_ORCA_MAGIC_TOKEN "orca"

#define PV_ORCA_VERSION "3.0.X"

#ifdef __ORCA_LOG_LEVEL_VERBOSE__

#define ORCA_LOG_VERBOSE(m, ...) _LOG("[VERBOSE] " m, __VA_ARGS__)
#define ORCA_LOG_VERBOSE_SIMPLE(m) _LOG_SIMPLE("[VERBOSE] " m)
#define ORCA_LOG_VERBOSE_INLINE(m, ...) _LOG_INLINE(m, __VA_ARGS__)
#define ORCA_LOG_VERBOSE_INLINE_SIMPLE(m) _LOG_INLINE_SIMPLE(m)

#else

#define ORCA_LOG_VERBOSE(fmt, ...)
#define ORCA_LOG_VERBOSE_SIMPLE(fmt)
#define ORCA_LOG_VERBOSE_INLINE(fmt, ...)
#define ORCA_LOG_VERBOSE_INLINE_SIMPLE(fmt)

#endif

pv_status_t PV_MOCKABLE(pv_orca_internal_init)(
        const char *access_key,
        pv_https_client_factory_t *https_client_factory,
        const char *model_path,
        pv_ypu_t *ypu,
        pv_orca_t **object);

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_orca_internal_param_serialize)(
        pv_ypu_t *ypu,
        const pv_orca_phonemizer_param_t *phonemizer_param,
        const pv_orca_synthesizer_param_t *synthesizer_param,
        const char *path);

#endif

pv_normalizer_t *PV_MOCKABLE(pv_orca_get_normalizer)(const pv_orca_t *object);

pv_status_t PV_MOCKABLE(pv_orca_internal_param_load)(
        pv_ypu_t *ypu,
        FILE *f,
        pv_orca_phonemizer_param_t **phonemizer_param,
        pv_orca_synthesizer_param_t **synthesizer_param);

typedef struct {
    pv_orca_phonemizer_param_t *phonemizer_param;
    pv_orca_synthesizer_param_t *synthesizer_param;
} pv_orca_internal_param_t;

const pv_serialized_vtable_t *PV_MOCKABLE(pv_orca_internal_param_serialized_vtable)(void);

pv_status_t PV_MOCKABLE(pv_orca_phoneme_alignment_init)(
        const char *phoneme,
        float start_sec,
        float end_sec,
        pv_orca_phoneme_alignment_t **object);

pv_status_t PV_MOCKABLE(pv_orca_phoneme_alignment_copy)(
        pv_orca_phoneme_alignment_t *source,
        pv_orca_phoneme_alignment_t **destination);

pv_status_t PV_MOCKABLE(pv_orca_phoneme_alignment_delete)(pv_orca_phoneme_alignment_t *object);

pv_status_t PV_MOCKABLE(pv_orca_word_alignment_init)(
        const char *word,
        float start_sec,
        float end_sec,
        int32_t num_phonemes,
        pv_orca_phoneme_alignment_t **phonemes,
        pv_orca_word_alignment_t **object);

pv_status_t PV_MOCKABLE(pv_orca_word_alignment_delete)(pv_orca_word_alignment_t *object);

pv_status_t PV_MOCKABLE(pv_orca_phonemize_text)(
        const pv_orca_t *object,
        const char *text,
        bool is_flush,
        bool no_text_additions,
        int32_t *num_text_tokens,
        pv_normalizer_token_t ***text_tokens,
        int32_t *num_encoded_phonemes,
        int32_t **encoded_phonemes,
        int32_t **text_tokens_num_encoded_phonemes);

pv_status_t PV_MOCKABLE(pv_orca_synthesize_internal)(
        const pv_orca_t *object,
        const char *text,
        const pv_orca_synthesize_params_t *synthesize_params,
        bool no_random_latents,
        bool no_text_additions,
        int32_t *num_samples,
        int16_t **pcm,
        int32_t *num_alignments,
        pv_orca_word_alignment_t ***alignments);

pv_status_t PV_MOCKABLE(pv_orca_synthesize_params_copy)(
        const pv_orca_synthesize_params_t *source,
        pv_orca_synthesize_params_t *destination);

pv_status_t PV_MOCKABLE(pv_orca_synthesize_params_get_random_state_valid)(
        const pv_orca_synthesize_params_t *object,
        int64_t *random_state);

#if !defined(__PV_TARGET_NO_FILE_INTERFACE__) && !defined(__PV_TARGET_NO_FILE_SYSTEM__)

pv_status_t PV_MOCKABLE(pv_orca_synthesize_to_file_internal)(
        const pv_orca_t *object,
        const char *text,
        const pv_orca_synthesize_params_t *synthesize_params,
        const char *output_path,
        int32_t *num_alignments,
        pv_orca_word_alignment_t ***alignments);

#endif

pv_status_t PV_MOCKABLE(pv_orca_merge_word_alignments)(
        int32_t num_alignments,
        pv_orca_word_alignment_t **alignments_orig,
        const int32_t *indices_merged,
        int32_t *num_alignments_merged,
        pv_orca_word_alignment_t ***alignments_merged);

pv_status_t PV_MOCKABLE(pv_orca_create_word_alignments)(
        const pv_orca_t *object,
        int32_t num_text_tokens,
        pv_normalizer_token_t **text_tokens,
        const int32_t *text_tokens_num_encoded_phonemes,
        const int32_t *encoded_phonemes,
        const int32_t *encoded_phonemes_durations,
        int32_t *num_alignments,
        pv_orca_word_alignment_t ***alignments);

pv_orca_stream_state_t *PV_MOCKABLE(pv_orca_stream_state_get)(const pv_orca_t *object);

pv_status_t PV_MOCKABLE(pv_orca_stream_synthesize_internal)(
        pv_orca_stream_t *object,
        bool no_random_latents,
        bool no_synthesis,
        const char *text,
        bool no_text_additions,
        int32_t *num_samples,
        int16_t **pcm);

pv_status_t PV_MOCKABLE(pv_orca_stream_flush_internal)(
        pv_orca_stream_t *object,
        bool no_random_latents,
        bool no_synthesis,
        bool no_text_additions,
        int32_t *num_samples,
        int16_t **pcm);

pv_status_t PV_MOCKABLE(pv_orca_synthesize_params_set_default_random_state)(
        pv_orca_synthesize_params_t *object);

#endif // PV_ORCA_INTERNAL_H
