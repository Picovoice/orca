#ifndef PV_ORCA_UTIL_H
#define PV_ORCA_UTIL_H

#include <stdlib.h>

#include "core/picovoice.h"
#include "core/pv_type.h"
#include "ypu/pv_ypu.h"

pv_status_t PV_MOCKABLE(pv_orca_util_expand_tokens_to_frames)(
        pv_ypu_t *ypu,
        int32_t num_tokens,
        int32_t num_channels,
        const int32_t *durations,
        pv_ypu_mem_t *means_enc_ypu_mem,
        pv_ypu_mem_t *logs_enc_ypu_mem,
        pv_ypu_mem_t *means_ypu_mem,
        pv_ypu_mem_t *logs_ypu_mem,
        int32_t means_enc_offset,
        int32_t logs_enc_offset,
        int32_t means_offset,
        int32_t logs_offset);

pv_status_t PV_MOCKABLE(pv_orca_util_split_channels)(
        pv_ypu_t *ypu,
        int32_t n,
        int32_t num_channels,
        pv_ypu_mem_t *x,
        pv_ypu_mem_t *y0,
        pv_ypu_mem_t *y1,
        int32_t x_offset,
        int32_t y0_offset,
        int32_t y1_offset);

void PV_MOCKABLE(pv_orca_util_concatenate_channel_wise)(
        int32_t n,
        int32_t half_channels,
        const float *x0,
        const float *x1,
        float *y);

pv_status_t PV_MOCKABLE(pv_orca_util_fused_tanh_sigmoid_multiply)(
        pv_ypu_t *ypu,
        int32_t n,
        int32_t num_channels,
        pv_ypu_mem_t *x,
        pv_ypu_mem_t *y,
        int32_t x_offset,
        int32_t y_offset);

int32_t PV_MOCKABLE(pv_orca_clip_int32)(int32_t n, int32_t min, int32_t max);

typedef struct {
    float value;
    bool has_value;
} pv_orca_util_rand_normal_t;

pv_status_t PV_MOCKABLE(pv_orca_util_rand_normal_init)(pv_orca_util_rand_normal_t **object);

void PV_MOCKABLE(pv_orca_util_rand_normal_delete)(pv_orca_util_rand_normal_t *object);

float PV_MOCKABLE(pv_orca_util_rand_normal_sample)(pv_orca_util_rand_normal_t *object, uint64_t *state);

pv_status_t PV_MOCKABLE(pv_orca_util_sample_latents)(
        pv_ypu_t *ypu,
        int32_t n,
        int32_t num_channels,
        float noise_scale,
        int64_t random_state,
        pv_ypu_mem_t *means_ypu_mem,
        pv_ypu_mem_t *logs_ypu_mem,
        pv_ypu_mem_t *y_ypu_mem);

void PV_MOCKABLE(pv_orca_util_scale_and_quantize_activation)(
        int32_t num_elements,
        int32_t padding_numel,
        const float *x,
        q510_t *y,
        float *inverse_scale);

#endif
