#ifndef PV_CNN_TRANSPOSED_H
#define PV_CNN_TRANSPOSED_H

#include "util/pv_file.h"
#include "core/pv_type.h"

/* layout-compatible with pv_cnn_depthwise_param_t */
typedef struct {
    int32_t num_channels;
    int32_t kernel_size;
    int32_t stride;
    int32_t padding;
    int32_t dilation;
    const q7_t *weight;
    const q7_t *bias;
} pv_cnn_transposed_depthwise_param_t;

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_cnn_transposed_depthwise_param_serialize)(
        const pv_cnn_transposed_depthwise_param_t *param,
        FILE *file);

#endif

void PV_MOCKABLE(pv_cnn_transposed_depthwise_param_delete)(pv_cnn_transposed_depthwise_param_t *param);

pv_status_t PV_MOCKABLE(pv_cnn_transposed_depthwise_param_load)(FILE *f, pv_cnn_transposed_depthwise_param_t **param);

bool PV_MOCKABLE(pv_cnn_transposed_depthwise_param_is_equal)(
        const pv_cnn_transposed_depthwise_param_t *object,
        const pv_cnn_transposed_depthwise_param_t *other);

int32_t PV_MOCKABLE(pv_cnn_transposed_depthwise_param_receptive_field)(
        const pv_cnn_transposed_depthwise_param_t *object);

typedef struct pv_cnn_transposed_depthwise pv_cnn_transposed_depthwise_t;

pv_status_t PV_MOCKABLE(pv_cnn_transposed_depthwise_init)(
        const pv_cnn_transposed_depthwise_param_t *param,
        pv_cnn_transposed_depthwise_t **object);

void PV_MOCKABLE(pv_cnn_transposed_depthwise_delete)(pv_cnn_transposed_depthwise_t *object);

pv_status_t PV_MOCKABLE(pv_cnn_transposed_depthwise_forward)(
        pv_cnn_transposed_depthwise_t *object,
        int32_t n,
        const float *x,
        float *y);

int32_t PV_MOCKABLE(pv_cnn_transposed_depthwise_num_output_frames)(
        const pv_cnn_transposed_depthwise_t *object,
        int32_t n);

#endif // PV_CNN_TRANSPOSED_H
