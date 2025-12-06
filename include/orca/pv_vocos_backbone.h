#ifndef PV_VOCOS_BACKBONE_H
#define PV_VOCOS_BACKBONE_H

#include "orca/pv_cnn.h"
#include "orca/pv_layer_norm.h"
#include "orca/pv_convnext.h"
#include "ypu/pv_ypu.h"

typedef struct {
    const pv_layer_norm_param_t *layer_norm_pre_param;
    const pv_layer_norm_param_t *layer_norm_post_param;
    int32_t num_convnext_layers;
    const pv_convnext_param_t **convnext_layers_param;
} pv_vocos_backbone_param_t;

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_vocos_backbone_param_serialize)(
        pv_ypu_t *ypu,
        const pv_vocos_backbone_param_t *param,
        FILE *file);

#endif

void PV_MOCKABLE(pv_vocos_backbone_param_delete)(pv_ypu_t *ypu, pv_vocos_backbone_param_t *param);

pv_status_t PV_MOCKABLE(pv_vocos_backbone_param_load)(pv_ypu_t *ypu, FILE *f, pv_vocos_backbone_param_t **param);

bool PV_MOCKABLE(pv_vocos_backbone_param_is_equal)(
        const pv_vocos_backbone_param_t *object,
        const pv_vocos_backbone_param_t *other);

int32_t PV_MOCKABLE(pv_vocos_backbone_param_receptive_field)(const pv_vocos_backbone_param_t *object);

typedef struct pv_vocos_backbone pv_vocos_backbone_t;

pv_status_t PV_MOCKABLE(pv_vocos_backbone_init)(
        pv_ypu_t *ypu,
        const pv_vocos_backbone_param_t *param,
        pv_vocos_backbone_t **object);

void PV_MOCKABLE(pv_vocos_backbone_delete)(pv_ypu_t *ypu, pv_vocos_backbone_t *object);

int32_t PV_MOCKABLE(pv_vocos_backbone_output_channels)(const pv_vocos_backbone_t *object);

pv_status_t PV_MOCKABLE(pv_vocos_backbone_forward)(
        pv_ypu_t *ypu,
        pv_vocos_backbone_t *object,
        int32_t n,
        pv_ypu_mem_t *x,
        pv_ypu_mem_t *y,
        int32_t x_offset,
        int32_t y_offset);

#endif // PV_VOCOS_BACKBONE_H
