#include "core/picovoice.h"
#include "orca/pv_orca_fft.h"
#include "orca/pv_orca_istft.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

/**
 * This file is the adapted pv_fft.c class for Orca. We just need to adapt the fft size (512 -> 16)
 * and the num stages (8 -> 4). See pv_fft.c for details on the FFT algorithm
 */

#ifndef M_PI

#define M_PI (3.141592653589793)

#endif

#define PV_ORCA_NUM_FFT_COMPLEX (PV_ORCA_VOCODER_NUM_FFT >> 1)

#define PV_ORCA_FFT_NUM_STAGES (4)

#define ORCA_COS_OFFSET (PV_ORCA_VOCODER_NUM_FFT >> 2)

static const float ORCA_SIN_TABLE[3 * PV_ORCA_VOCODER_NUM_FFT >> 2] = {
        0.0000000f, 0.3826834f, 0.7071068f, 0.9238795f, 1.0000000f, 0.9238795f, 0.7071068f, 0.3826834f,
        0.0000000f, -0.3826834f, -0.7071068f, -0.9238795f};

static float pv_orca_fft_sin(int32_t i) {
    return ORCA_SIN_TABLE[i];
}

static float pv_orca_fft_cos(int32_t i) {
    return ORCA_SIN_TABLE[ORCA_COS_OFFSET + i];
}

static const int16_t ORCA_BIT_REVERSAL_TABLE[PV_ORCA_NUM_FFT_COMPLEX] = {
        0, 8, 4, 12, 2, 10, 6, 14};

static void pv_orca_fft_complex_DIT_RN(float *y, float sign) {
    uint32_t num_parts = PV_ORCA_NUM_FFT_COMPLEX >> 1;
    uint32_t part_size = 2;
    uint32_t half_part_size = 1;
    uint32_t trig_inc = PV_ORCA_NUM_FFT_COMPLEX;

    for (uint32_t i = 0; i < PV_ORCA_FFT_NUM_STAGES; i++) {
        float *p1 = y;
        float *p2 = y + part_size;

        for (uint32_t j = 0; j < num_parts; j++) {
            const float *p_sin = ORCA_SIN_TABLE;
            const float *p_cos = ORCA_SIN_TABLE + ORCA_COS_OFFSET;

            for (uint32_t k = 0; k < half_part_size; k++) {
                const float r1 = p1[0];
                const float i1 = p1[1];
                const float r2 = p2[0];
                const float i2 = p2[1];
                const float v_sin = *p_sin;
                const float v_cos = *p_cos;
                const float tr = (r2 * v_cos) - (sign * i2 * v_sin);
                const float ti = (i2 * v_cos) + (sign * r2 * v_sin);
                *p1++ += tr;
                *p1++ += ti;
                *p2++ = r1 - tr;
                *p2++ = i1 - ti;
                p_sin += trig_inc;
                p_cos += trig_inc;
            }
            p1 += part_size;
            p2 += part_size;
        }

        num_parts >>= 1U;
        part_size <<= 1U;
        half_part_size <<= 1U;
        trig_inc >>= 1U;
    }
}

void pv_orca_fft_inverse_preprocess(const float *x, float *y) {
    PV_ASSERT(x);
    PV_ASSERT(y);

    y[0] = x[0] + x[PV_ORCA_VOCODER_NUM_FFT];
    y[1] = x[0] - x[PV_ORCA_VOCODER_NUM_FFT];

    for (int32_t i = 1; i < PV_ORCA_NUM_FFT_COMPLEX; i++) {
        const int32_t r = ORCA_BIT_REVERSAL_TABLE[i];

        const int32_t j = 2 * i;
        const int32_t k = j + 1;
        const int32_t l = PV_ORCA_VOCODER_NUM_FFT - j;
        const int32_t m = l + 1;

        const float sin = pv_orca_fft_sin(i);
        const float cos = pv_orca_fft_cos(i);

        const float w = x[k] + x[m];
        const float z = x[j] - x[l];

        y[r] = 0.5f * (x[j] + x[l] - cos * w - sin * z);
        y[r + 1] = 0.5f * (x[k] - x[m] + cos * z - sin * w);
    }
}

void PV_MOCKABLE(pv_orca_fft_inverse)(const float *x, float *y) {
    PV_ASSERT(x);
    PV_ASSERT(y);

    pv_orca_fft_inverse_preprocess(x, y);
    pv_orca_fft_complex_DIT_RN(y, 1.f);
}
