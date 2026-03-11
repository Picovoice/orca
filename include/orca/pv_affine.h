#ifndef PV_AFFINE_H
#define PV_AFFINE_H

#include "ypu/pv_ypu.h"

#if __ORCA_FLOAT_MODE__

pv_status_t PV_MOCKABLE(pv_affine_execute_float)(
        int32_t n,
        int32_t num_channels,
        const float *x,
        float scale,
        float shift,
        const float *weight,
        const float *bias,
        float *y);

#endif

pv_status_t PV_MOCKABLE(pv_affine_execute)(
        pv_ypu_t *ypu,
        int32_t n,
        int32_t num_channels,
        pv_ypu_mem_t *x,
        float scale,
        float shift,
        pv_ypu_mem_t *weight,
        pv_ypu_mem_t *bias,
        pv_ypu_mem_t *y,
        int32_t x_offset,
        int32_t weight_offset,
        int32_t bias_offset,
        int32_t y_offset);

pv_status_t PV_MOCKABLE(pv_affine_execute_from_q1417_to_float)(
        pv_ypu_t *ypu,
        int32_t n,
        int32_t num_channels,
        pv_ypu_mem_t *x,
        pv_ypu_mem_t *scale,
        pv_ypu_mem_t *weight,
        pv_ypu_mem_t *bias,
        pv_ypu_mem_t *y,
        int32_t x_offset,
        int32_t scale_offset,
        int32_t weight_offset,
        int32_t bias_offset,
        int32_t y_offset);

#endif // PV_AFFINE_H
