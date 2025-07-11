#ifndef PV_CNN_MATMUL_H
#define PV_CNN_MATMUL_H

#include "core/pv_type.h"

void pv_cnn_kernel_1_q510(
        int32_t in_channels,
        int32_t out_channels,
        const q7_t *weight,
        int32_t n,
        const q510_t *x,
        int32_t *y);

void pv_cnn_kernel_3_q510(
        int32_t in_channels,
        int32_t out_channels,
        const q7_t *weight,
        int32_t n,
        const q510_t *x,
        int32_t *y);

void pv_cnn_kernel_5_q510(
        int32_t in_channels,
        int32_t out_channels,
        const q7_t *weight,
        int32_t n,
        const q510_t *x,
        int32_t *y);

void pv_cnn_kernel_7_q510(
        int32_t in_channels,
        int32_t out_channels,
        const q7_t *weight,
        int32_t n,
        const q510_t *x,
        int32_t *y);

#endif // PV_CNN_MATMUL_H