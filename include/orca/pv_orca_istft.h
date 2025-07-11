#ifndef PV_ORCA_ISTFT_H
#define PV_ORCA_ISTFT_H

#include "core/pv_type.h"
#include "orca/pv_orca_synthesizer.h"

#define PV_ORCA_ISTFT_LINEAR_FADE_IN_SAMPLES (256)
#define PV_ORCA_ISTFT_LINEAR_FADE_OUT_SAMPLES (256)

typedef struct pv_orca_istft pv_orca_istft_t;

pv_status_t PV_MOCKABLE(pv_orca_istft_init)(
        int32_t window_length,
        int32_t window_shift,
        int32_t num_fft,
        int32_t num_subbands,
        float pcm_normalization_factor,
        pv_orca_istft_t **object);

void PV_MOCKABLE(pv_orca_istft_delete)(pv_orca_istft_t *object);

/*
 * Preprocess from [n_fft + 2, num_frames] with real values in first half and complex values in second half
 * to [n_fft + 2, num_frames] with real and complex values interleaved
 */
void PV_MOCKABLE(pv_orca_istft_preprocess_fft)(
        int32_t num_fft,
        int32_t num_frames,
        const float *half_spec,
        float *full_spec);

pv_status_t PV_MOCKABLE(pv_orca_istft_forward)(
        const pv_orca_istft_t *object,
        int32_t num_frames,
        const float *vocoder_spec_complex,
        float *pcm_subband);

/*
 * Takes input [num_subbands * (n_fft + 2), num_frames], performs an istft for each subband, and combines the results
 * via PQMF into a single PCM signal
 */
pv_status_t PV_MOCKABLE(pv_orca_istft_multiband_forward)(
        const pv_orca_istft_t *object,
        int32_t num_frames,
        const float *spec_all_subbands,
        int16_t *pcm);

#endif
