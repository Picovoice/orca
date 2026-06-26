#ifndef PV_ORCA_VOCODER_H
#define PV_ORCA_VOCODER_H

#include "orca/pv_cnn.h"
#include "orca/pv_cnn_transposed_depthwise.h"
#include "orca/pv_convnext_transposed.h"
#include "orca/pv_vocos_backbone.h"
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

pv_status_t PV_MOCKABLE(pv_orca_vocoder_param_serialize)(
        pv_ypu_t *ypu,
        const pv_orca_vocoder_param_t *param,
        FILE *file);

#endif

void PV_MOCKABLE(pv_orca_vocoder_param_delete)(
        pv_ypu_t *ypu,
        pv_orca_vocoder_param_t *param);

pv_status_t PV_MOCKABLE(pv_orca_vocoder_param_load)(
        pv_ypu_t *ypu,
        FILE *f,
        pv_orca_vocoder_param_t **param);

bool PV_MOCKABLE(pv_orca_vocoder_param_is_equal)(
        const pv_orca_vocoder_param_t *object,
        const pv_orca_vocoder_param_t *other);

typedef struct pv_orca_vocoder pv_orca_vocoder_t;

pv_status_t PV_MOCKABLE(pv_orca_vocoder_init)(
        pv_ypu_t *ypu,
        const pv_orca_vocoder_param_t *param,
        pv_orca_vocoder_t **object);

void PV_MOCKABLE(pv_orca_vocoder_delete)(
        pv_ypu_t *ypu,
        pv_orca_vocoder_t *object);

pv_status_t PV_MOCKABLE(pv_orca_vocoder_reset_cache)(
        pv_ypu_t *ypu,
        pv_orca_vocoder_t *object);

pv_status_t PV_MOCKABLE(pv_orca_vocoder_forward)(
        pv_ypu_t *ypu,
        pv_orca_vocoder_t *object,
        int32_t n,
        pv_ypu_mem_t *x,
        int16_t *pcm,
        int32_t x_offset);

pv_status_t PV_MOCKABLE(pv_orca_vocoder_forward_with_cache)(
        pv_ypu_t *ypu,
        pv_orca_vocoder_t *object,
        int32_t n,
        pv_ypu_mem_t *x,
        int16_t *pcm,
        int32_t x_offset,
        bool is_flush,
        int32_t *num_pcm_out);

#endif // PV_ORCA_VOCODER_H
