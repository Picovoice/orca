#ifndef PV_CONVNEXT_TRANSPOSED_H
#define PV_CONVNEXT_TRANSPOSED_H

#include "core/pv_type.h"
#include "orca/pv_buffer.h"
#include "orca/pv_cnn.h"
#include "orca/pv_cnn_transposed.h"
#include "orca/pv_layer_norm.h"

typedef struct {
    const pv_cnn_transposed_depthwise_param_t *conv_transposed_depthwise_param;
    const pv_layer_norm_param_t *layer_norm_param;
    const pv_cnn_param_t *conv_1_param;
    const pv_cnn_param_t *conv_2_param;
} pv_convnext_transposed_param_t;

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_convnext_transposed_param_serialize)(const pv_convnext_transposed_param_t *param, FILE *file);

#endif

void PV_MOCKABLE(pv_convnext_transposed_param_delete)(pv_convnext_transposed_param_t *param);

pv_status_t PV_MOCKABLE(pv_convnext_transposed_param_load)(FILE *f, pv_convnext_transposed_param_t **param);

bool PV_MOCKABLE(pv_convnext_transposed_param_is_equal)(
        const pv_convnext_transposed_param_t *object,
        const pv_convnext_transposed_param_t *other);

typedef struct pv_convnext_transposed pv_convnext_transposed_t;

pv_status_t PV_MOCKABLE(pv_convnext_transposed_init)(
        const pv_convnext_transposed_param_t *param,
        pv_convnext_transposed_t **object);

void PV_MOCKABLE(pv_convnext_transposed_delete)(pv_convnext_transposed_t *object);

pv_status_t PV_MOCKABLE(pv_convnext_transposed_forward)(
        pv_convnext_transposed_t *object,
        int32_t n,
        const float *x,
        float *y);

int32_t PV_MOCKABLE(pv_convnext_transposed_num_output_frames)(const pv_convnext_transposed_t *object, int32_t n);

int32_t PV_MOCKABLE(pv_convnext_transposed_output_channels)(const pv_convnext_transposed_t *object);

#endif // PV_CONVNEXT_TRANSPOSED_H
