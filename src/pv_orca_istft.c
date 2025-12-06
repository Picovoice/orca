#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "core/picovoice.h"
#include "core/pv_error_messages.h"
#include "core/pv_type.h"
#include "io/pv_dump.h"
#include "orca/pv_buffer.h"
#include "orca/pv_orca_fft.h"
#include "orca/pv_orca_istft.h"
#include "orca/pv_orca_pqmf.h"
#include "orca/pv_profiler.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

#ifndef M_PI

#define M_PI (3.141592653589793)

#endif


static pv_status_t generate_hann_window(int32_t length, float **window) {
    PV_ASSERT(length > 1);
    PV_ASSERT(window);

    *window = NULL;

    float *w = calloc(length, sizeof(float));
    if (!w) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    for (int32_t i = 0; i < length; ++i) {
        // non-symmetric hann window mimicking torch.hann_window()
        w[i] = 0.5f * (float) (1 - cos((2.0 * M_PI * i) / length));
    }

    *window = w;

    return PV_STATUS_SUCCESS;
}

struct pv_orca_istft {
    float *window;
    int32_t padding;
    int32_t window_length;
    int32_t window_shift;
    int32_t num_fft;
    int32_t num_subbands;
    float pcm_normalization_factor;

    pv_buffer_t *buffer_reconstructed_frames;
    pv_buffer_t *buffer_reconstructed_frame;

    pv_buffer_t *buffer_window_norms;
    pv_buffer_t *buffer_spec_subband;
    pv_buffer_t *buffer_spec_subband_preprocessed;
    pv_buffer_t *buffer_pcm_all_subbands;
};

pv_status_t PV_MOCKABLE(pv_orca_istft_init)(
        int32_t window_length,
        int32_t window_shift,
        int32_t num_fft,
        int32_t num_subbands,
        float pcm_normalization_factor,
        pv_orca_istft_t **object) {
    PV_ASSERT((window_length / 2) % 2 == 0);
    PV_ASSERT(window_shift > 0);
    PV_ASSERT(num_fft > 0);
    PV_ASSERT(num_subbands > 0);
    PV_ASSERT(object);

    *object = NULL;

    pv_orca_istft_t *o = calloc(1, sizeof(pv_orca_istft_t));
    if (!o) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    int32_t padding = (window_length - window_shift) / 2;
    o->padding = padding;
    o->num_fft = num_fft;
    o->window_length = window_length;
    o->window_shift = window_shift;
    o->num_subbands = num_subbands;
    o->pcm_normalization_factor = pcm_normalization_factor;

    pv_status_t status = generate_hann_window(window_length, &(o->window));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_istft_delete(o);
        return status;
    }

    status = pv_buffer_init(o->num_fft, &(o->buffer_reconstructed_frames));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_istft_delete(o);
        return status;
    }

    status = pv_buffer_init(o->num_fft, &(o->buffer_reconstructed_frame));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_istft_delete(o);
        return status;
    }

    status = pv_buffer_init(o->num_fft, &(o->buffer_window_norms));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_istft_delete(o);
        return status;
    }

    status = pv_buffer_init(o->num_fft + 2, &(o->buffer_spec_subband));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_istft_delete(o);
        return status;
    }

    status = pv_buffer_init(o->num_fft + 2, &(o->buffer_spec_subband_preprocessed));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_istft_delete(o);
        return status;
    }

    status = pv_buffer_init(o->num_subbands, &(o->buffer_pcm_all_subbands));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_istft_delete(o);
        return status;
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_orca_istft_delete)(pv_orca_istft_t *object) {
    if (object) {
        pv_buffer_delete(object->buffer_pcm_all_subbands);
        pv_buffer_delete(object->buffer_spec_subband_preprocessed);
        pv_buffer_delete(object->buffer_spec_subband);
        pv_buffer_delete(object->buffer_window_norms);
        pv_buffer_delete(object->buffer_reconstructed_frame);
        pv_buffer_delete(object->buffer_reconstructed_frames);

        free(object->window);
        free(object);
    }
}

void PV_MOCKABLE(pv_orca_istft_preprocess_fft)(
        int32_t num_fft,
        int32_t num_frames,
        const float *spec_half_real_half_imag,
        float *spec_complex) {
    PV_ASSERT(num_fft);
    PV_ASSERT(num_frames);
    PV_ASSERT(spec_half_real_half_imag);
    PV_ASSERT(spec_complex);

    for (int32_t frame = 0; frame < num_frames; frame++) {
        int32_t frame_offset = frame * (num_fft + 2);

        const float *magnitude = spec_half_real_half_imag + frame_offset;
        const float *phase = spec_half_real_half_imag + frame_offset + (num_fft / 2) + 1;

        for (int32_t i = 0; i < (num_fft / 2) + 1; i++) {
            float mag = fminf(expf(magnitude[i]), 100.f);
            float multiplication_factor =
                    ((i > 0) && (i < (num_fft / 2))) ? (2.f / (float) num_fft) : (1.f / (float) num_fft);

            spec_complex[frame_offset + (2 * i)] = mag * cosf(phase[i]) * multiplication_factor;
            spec_complex[frame_offset + (2 * i) + 1] = mag * sinf(phase[i]) * multiplication_factor;
        }

        spec_complex[frame_offset + 1] = 0.f; // DC imaginary part is always 0 for real output signals
        spec_complex[frame_offset + num_fft + 1] = 0.f; // Nyquist frequency's imaginary part is 0 for real output
    }
}

pv_status_t PV_MOCKABLE(pv_orca_istft_forward)(
        const pv_orca_istft_t *object,
        int32_t num_frames,
        const float *vocoder_spec_complex,
        float *pcm_subband) {
    PV_ASSERT(object);
    PV_ASSERT(num_frames);
    PV_ASSERT(vocoder_spec_complex);
    PV_ASSERT(pcm_subband);
    PV_ORCA_PROFILER_START("istft");

    float *buffer_reconstructed_frame = pv_buffer_get(object->buffer_reconstructed_frame, 1, false);
    if (!buffer_reconstructed_frame) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    float *buffer_reconstructed_frames = pv_buffer_get(object->buffer_reconstructed_frames, num_frames, true);
    if (!buffer_reconstructed_frames) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    int32_t num_fft_plus_two = object->num_fft + 2;
    for (int32_t frame = 0; frame < num_frames; frame++) {
        const int32_t frame_offset = frame * object->window_shift;
        pv_orca_fft_inverse(vocoder_spec_complex + (frame * (num_fft_plus_two)), buffer_reconstructed_frame);

        for (int32_t sample = 0; sample < object->num_fft; sample++) {
            buffer_reconstructed_frames[frame_offset + sample] +=
                    buffer_reconstructed_frame[sample] * object->window[sample];
        }
    }

    pv_buffer_free(object->buffer_reconstructed_frame);

    float *buffer_window_norms = pv_buffer_get(object->buffer_window_norms, num_frames, true);
    if (!buffer_window_norms) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    for (int32_t frame = 0; frame < num_frames; frame++) {
        const int32_t frame_offset = frame * object->window_shift;

        for (int32_t sample = 0; sample < object->num_fft; sample++) {
            buffer_window_norms[frame_offset + sample] += object->window[sample] * object->window[sample];
        }
    }

    int32_t num_samples = num_frames * object->window_shift;
    for (int32_t i = object->padding; i < num_samples + object->padding; i++) {
        pcm_subband[i - object->padding] = (buffer_reconstructed_frames[i] / buffer_window_norms[i]);
    }

    PV_ORCA_PROFILER_STOP("istft");
    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_orca_istft_multiband_forward)(
        const pv_orca_istft_t *object,
        int32_t num_frames,
        const float *spec_all_subbands,
        int16_t *pcm) {
    PV_ASSERT(object);
    PV_ASSERT(num_frames);
    PV_ASSERT(spec_all_subbands);
    PV_ASSERT(pcm);
    PV_ORCA_PROFILER_START("multiband_istft");

    const int32_t num_samples = num_frames * object->window_length;
    const int32_t num_samples_subband = num_frames * (object->window_length / object->num_subbands);
    const int32_t num_channels_all_subbands = object->num_subbands * (object->num_fft + 2);

    float *buffer_spec_subband = pv_buffer_get(object->buffer_spec_subband, num_frames, false);
    if (!buffer_spec_subband) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    float *buffer_spec_subband_preprocessed = pv_buffer_get(
            object->buffer_spec_subband_preprocessed,
            num_frames,
            false);
    if (!buffer_spec_subband_preprocessed) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    // [PV_ORCA_VOCODER_NUM_SUBBANDS, num_samples]
    float *buffer_pcm_all_subbands = pv_buffer_get(
            object->buffer_pcm_all_subbands,
            num_samples_subband,
            false);
    if (!buffer_pcm_all_subbands) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    for (int32_t i = 0; i < object->num_subbands; i++) {
        const int32_t channel_offset = i * (object->num_fft + 2);
        for (int32_t j = 0; j < num_frames; j++) {
            const int32_t frame_offset_combined = j * num_channels_all_subbands;
            const int32_t frame_offset_single = j * (object->num_fft + 2);
            memcpy(
                    buffer_spec_subband + frame_offset_single,
                    spec_all_subbands + frame_offset_combined + channel_offset,
                    (object->num_fft + 2) * sizeof(float));
        }

        const int32_t subband_offset_samples = i * num_samples_subband;

        pv_orca_istft_preprocess_fft(
                object->num_fft,
                num_frames,
                buffer_spec_subband,
                buffer_spec_subband_preprocessed);

        pv_status_t status = pv_orca_istft_forward(
                object,
                num_frames,
                buffer_spec_subband_preprocessed,
                buffer_pcm_all_subbands + subband_offset_samples);
        if (status != PV_STATUS_SUCCESS) {
            return status;
        }
    }
    pv_buffer_free(object->buffer_spec_subband);
    pv_buffer_free(object->buffer_spec_subband_preprocessed);
    pv_buffer_free(object->buffer_window_norms);
    pv_buffer_free(object->buffer_reconstructed_frames);

    PV_DUMP_ARRAY_FLOAT_SIMPLE(buffer_pcm_all_subbands, num_samples_subband * object->num_subbands, "pcm_all_subbands")

    pv_status_t status = pv_orca_pqmf_synthesis(
            object->num_subbands,
            num_samples_subband,
            buffer_pcm_all_subbands,
            buffer_pcm_all_subbands); // reuse buffer_pcm_all_subbands. becomes [num_samples]
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    for (int32_t i = 0; i < num_samples; i++) {
        float scale = object->pcm_normalization_factor;
        if (i < PV_ORCA_ISTFT_LINEAR_FADE_IN_SAMPLES) {
            scale *= (float) i / (float) (PV_ORCA_ISTFT_LINEAR_FADE_IN_SAMPLES);
        } else if (i >= (num_samples - PV_ORCA_ISTFT_LINEAR_FADE_OUT_SAMPLES)) {
            scale *= (float) (num_samples - 1 - i) / (float) PV_ORCA_ISTFT_LINEAR_FADE_OUT_SAMPLES;
        }
        pcm[i] = pv_float_to_int16(buffer_pcm_all_subbands[i] * 32767.f * scale);
    }

    pv_buffer_free(object->buffer_pcm_all_subbands);

    PV_ORCA_PROFILER_STOP("multiband_istft");
    return PV_STATUS_SUCCESS;
}
