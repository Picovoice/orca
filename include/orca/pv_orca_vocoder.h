#ifndef PV_ORCA_VOCODER_H
#define PV_ORCA_VOCODER_H

#include "orca/pv_vocos_backbone.h"
#include "orca/pv_cnn.h"
#include "orca/pv_cnn_transposed.h"
#include "orca/pv_convnext_transposed.h"
#include "util/pv_file.h"

typedef struct {
    const pv_cnn_param_t *conv_pre_param;
    const pv_convnext_transposed_param_t *convnext_transposed_0_param;
    const pv_convnext_transposed_param_t *convnext_transposed_1_param;
    const pv_vocos_backbone_param_t *backbone_0_param;
    const pv_vocos_backbone_param_t *backbone_1_param;
    const pv_cnn_param_t *conv_proj_param;
    float pcm_normalization_factor;
} pv_orca_vocoder_param_t;

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_orca_vocoder_param_serialize)(const pv_orca_vocoder_param_t *param, FILE *file);

#endif

void PV_MOCKABLE(pv_orca_vocoder_param_delete)(pv_orca_vocoder_param_t *param);

pv_status_t PV_MOCKABLE(pv_orca_vocoder_param_load)(FILE *f, pv_orca_vocoder_param_t **param);

bool PV_MOCKABLE(pv_orca_vocoder_param_is_equal)(
        const pv_orca_vocoder_param_t *object,
        const pv_orca_vocoder_param_t *other);

int32_t PV_MOCKABLE(pv_orca_vocoder_param_receptive_field)(const pv_orca_vocoder_param_t *object);

typedef struct pv_orca_vocoder pv_orca_vocoder_t;

pv_status_t PV_MOCKABLE(pv_orca_vocoder_init)(
        const pv_orca_vocoder_param_t *param,
        pv_orca_vocoder_t **object);

void PV_MOCKABLE(pv_orca_vocoder_delete)(pv_orca_vocoder_t *object);

pv_status_t PV_MOCKABLE(pv_orca_vocoder_forward)(
        pv_orca_vocoder_t *object,
        int32_t n,
        const float *x,
        int16_t *pcm);

#endif // PV_ORCA_VOCODER_H
