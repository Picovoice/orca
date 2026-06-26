#ifndef PV_CONVNEXT_H
#define PV_CONVNEXT_H

#include "core/pv_type.h"
#include "orca/pv_cnn.h"
#include "orca/pv_layer_norm.h"
#include "ypu/pv_ypu.h"

typedef struct {
    pv_ypu_config_mem_t *scale_param;
    const pv_cnn_depthwise_param_t *conv_depthwise_param;
    const pv_layer_norm_param_t *layer_norm_param;
    const pv_cnn_param_t *conv_1_param;
    const pv_cnn_param_t *conv_2_param;
} pv_convnext_param_t;

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_convnext_param_serialize)(
        pv_ypu_t *ypu,
        const pv_convnext_param_t *param,
        FILE *file);

#endif

void PV_MOCKABLE(pv_convnext_param_delete)(
        pv_ypu_t *ypu,
        pv_convnext_param_t *param);

pv_status_t PV_MOCKABLE(pv_convnext_param_load)(
        pv_ypu_t *ypu,
        FILE *f,
        pv_convnext_param_t **param);

bool PV_MOCKABLE(pv_convnext_param_is_equal)(
        const pv_convnext_param_t *object,
        const pv_convnext_param_t *other);

typedef struct pv_convnext pv_convnext_t;

pv_status_t PV_MOCKABLE(pv_convnext_init)(
        pv_ypu_t *ypu,
        const pv_convnext_param_t *param,
        pv_convnext_t **object);

void PV_MOCKABLE(pv_convnext_delete)(
        pv_ypu_t *ypu,
        pv_convnext_t *object);

pv_status_t PV_MOCKABLE(pv_convnext_reset_cache)(
        pv_ypu_t *ypu,
        pv_convnext_t *object);

pv_status_t PV_MOCKABLE(pv_convnext_forward)(
        pv_ypu_t *ypu,
        pv_convnext_t *object,
        int32_t n,
        pv_ypu_mem_t *x);

pv_status_t PV_MOCKABLE(pv_convnext_forward_with_cache)(
        pv_ypu_t *ypu,
        pv_convnext_t *object,
        int32_t n,
        pv_ypu_mem_t *x,
        bool is_flush,
        int32_t *n_out);

#endif // PV_CONVNEXT_H
