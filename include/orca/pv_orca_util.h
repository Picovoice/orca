#ifndef PV_ORCA_UTIL_H
#define PV_ORCA_UTIL_H

#include <stdlib.h>

#include "core/picovoice.h"
#include "core/pv_type.h"

void PV_MOCKABLE(pv_orca_util_expand_tokens_to_frames)(
        int32_t num_tokens,
        int32_t num_channels,
        const int32_t *durations,
        const float *means_enc,
        const float *logs_enc,
        float *means,
        float *logs);

void PV_MOCKABLE(pv_orca_util_split_channels)(
        int32_t n,
        int32_t num_channels,
        const float *x,
        float *y0,
        float *y1);

void PV_MOCKABLE(pv_orca_util_concatenate_channel_wise)(
        int32_t n,
        int32_t half_channels,
        const float *x0,
        const float *x1,
        float *y);

void PV_MOCKABLE(pv_orca_util_fused_tanh_sigmoid_multiply)(
        int32_t n,
        int32_t num_channels,
        const float *x,
        float *y);

int32_t PV_MOCKABLE(pv_orca_clip_int32)(int32_t n, int32_t min, int32_t max);

typedef struct {
    float value;
    bool has_value;
} pv_orca_util_rand_normal_t;

pv_status_t PV_MOCKABLE(pv_orca_util_rand_normal_init)(pv_orca_util_rand_normal_t **object);

void PV_MOCKABLE(pv_orca_util_rand_normal_delete)(pv_orca_util_rand_normal_t *object);

float PV_MOCKABLE(pv_orca_util_rand_normal_sample)(pv_orca_util_rand_normal_t *object, uint64_t *state);

pv_status_t PV_MOCKABLE(pv_orca_util_sample_latents)(
        int32_t n,
        int32_t num_channels,
        float noise_scale,
        int64_t random_state,
        const float *means,
        const float *logs,
        float *y);

void PV_MOCKABLE(pv_orca_util_scale_and_quantize_activation)(
        int32_t num_elements,
        int32_t padding_numel,
        const float *x,
        q510_t *y,
        float *inverse_scale);

#endif
