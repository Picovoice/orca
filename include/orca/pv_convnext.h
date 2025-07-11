#ifndef PV_CONVNEXT_H
#define PV_CONVNEXT_H

#include "core/pv_type.h"
#include "orca/pv_buffer.h"
#include "orca/pv_buffer_q510.h"
#include "orca/pv_layer_norm.h"
#include "orca/pv_cnn.h"

typedef struct {
    const float *scale_param;
    const pv_cnn_depthwise_param_t *conv_depthwise_param;
    const pv_layer_norm_param_t *layer_norm_param;
    const pv_cnn_param_t *conv_1_param;
    const pv_cnn_param_t *conv_2_param;
} pv_convnext_param_t;

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_convnext_param_serialize)(const pv_convnext_param_t *param, FILE *file);

#endif

void PV_MOCKABLE(pv_convnext_param_delete)(pv_convnext_param_t *param);

pv_status_t PV_MOCKABLE(pv_convnext_param_load)(FILE *f, pv_convnext_param_t **param);

bool PV_MOCKABLE(pv_convnext_param_is_equal)(const pv_convnext_param_t *object, const pv_convnext_param_t *other);

typedef struct pv_convnext pv_convnext_t;

pv_status_t PV_MOCKABLE(pv_convnext_init)(
        const pv_convnext_param_t *param,
        pv_buffer_t *buffer_vocos_backbone_convnext_1,
        pv_buffer_q510_t *buffer_vocos_backbone_convnext_2,
        pv_convnext_t **object);

void PV_MOCKABLE(pv_convnext_delete)(pv_convnext_t *object);

pv_status_t PV_MOCKABLE(pv_convnext_forward)(
        pv_convnext_t *object,
        int32_t n,
        const float *x,
        float *y);

#endif // PV_CONVNEXT_H
