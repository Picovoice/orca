#ifndef PV_ORCA_SYNTHESIZER_H
#define PV_ORCA_SYNTHESIZER_H

#include "core/pv_language.h"
#include "orca/pv_orca.h"
#include "orca/pv_orca_stream_state.h"
#include "orca/pv_orca_text_encoder.h"
#include "orca/pv_orca_duration_predictor.h"
#include "orca/pv_orca_flow.h"
#include "orca/pv_orca_vocoder.h"

#define PV_ORCA_WINDOW_SHIFT (256)

#define PV_ORCA_VOCODER_WINDOW_LENGTH (16)
#define PV_ORCA_VOCODER_WINDOW_SHIFT (4)
#define PV_ORCA_VOCODER_NUM_FFT (16)
#define PV_ORCA_VOCODER_NUM_SUBBANDS (4)

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_orca_synthesizer_param_serialize)(const pv_orca_synthesizer_param_t *param, FILE *f);

#endif

void PV_MOCKABLE(pv_orca_synthesizer_param_delete)(pv_orca_synthesizer_param_t *param);

pv_status_t PV_MOCKABLE(pv_orca_synthesizer_param_load)(
        FILE *f,
        const char *version,
        pv_orca_synthesizer_param_t **param);

bool PV_MOCKABLE(pv_orca_synthesizer_param_is_equal)(
        const pv_orca_synthesizer_param_t *object,
        const pv_orca_synthesizer_param_t *other);

typedef struct pv_orca_synthesizer pv_orca_synthesizer_t;

pv_status_t PV_MOCKABLE(pv_orca_synthesizer_init)(
        const pv_orca_synthesizer_param_t *param,
        pv_orca_stream_state_t *stream_state,
        pv_orca_synthesizer_t **object);

void PV_MOCKABLE(pv_orca_synthesizer_delete)(pv_orca_synthesizer_t *object);

int32_t PV_MOCKABLE(pv_orca_synthesizer_sample_rate)(const pv_orca_synthesizer_t *object);

pv_status_t PV_MOCKABLE(pv_orca_synthesizer_forward)(
        pv_orca_synthesizer_t *object,
        const pv_orca_synthesize_params_t *synthesize_params,
        bool no_random_latents,
        int32_t num_encoded_phonemes,
        const int32_t *encoded_phonemes,
        int32_t **encoded_phonemes_durations,
        int32_t *num_samples,
        int16_t **pcm);

#endif // PV_ORCA_SYNTHESIZER_H
