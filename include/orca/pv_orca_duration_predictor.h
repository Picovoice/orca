#ifndef PV_ORCA_DURATION_PREDICTOR_H
#define PV_ORCA_DURATION_PREDICTOR_H

#include "orca/pv_affine.h"
#include "orca/pv_cnn.h"
#include "orca/pv_embed.h"
#include "orca/pv_transformer.h"
#include "util/pv_file.h"
#include "ypu/pv_ypu.h"

typedef struct {
    const pv_cnn_param_t *conv_1_param;
    const pv_cnn_param_t *conv_2_param;
    const pv_layer_norm_param_t *layer_norm_1_param;
    const pv_layer_norm_param_t *layer_norm_2_param;
    const pv_cnn_param_t *conv_proj_param;
    const pv_affine_param_t *affine_pre_param;
    const pv_affine_param_t *affine_post_param;
} pv_orca_duration_predictor_param_t;

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_orca_duration_predictor_param_serialize)(
        pv_ypu_t *ypu,
        const pv_orca_duration_predictor_param_t *param,
        FILE *file);

#endif

void PV_MOCKABLE(pv_orca_duration_predictor_param_delete)(pv_ypu_t *ypu, pv_orca_duration_predictor_param_t *param);

pv_status_t PV_MOCKABLE(pv_orca_duration_predictor_param_load)(
        pv_ypu_t *ypu,
        FILE *f,
        pv_orca_duration_predictor_param_t **param);

bool PV_MOCKABLE(pv_orca_duration_predictor_param_is_equal)(
        const pv_orca_duration_predictor_param_t *object,
        const pv_orca_duration_predictor_param_t *other);

int32_t PV_MOCKABLE(pv_orca_duration_predictor_param_receptive_field)(
        const pv_orca_duration_predictor_param_t *object);

typedef struct pv_orca_duration_predictor pv_orca_duration_predictor_t;

pv_status_t PV_MOCKABLE(pv_orca_duration_predictor_init)(
        pv_ypu_t *ypu,
        const pv_orca_duration_predictor_param_t *param,
        pv_orca_duration_predictor_t **object);

void PV_MOCKABLE(pv_orca_duration_predictor_delete)(pv_ypu_t *ypu, pv_orca_duration_predictor_t *object);

pv_status_t PV_MOCKABLE(pv_orca_duration_predictor_forward)(
        pv_ypu_t *ypu,
        pv_orca_duration_predictor_t *object,
        float speech_rate,
        int32_t n,
        pv_ypu_mem_t *x,
        int32_t *durations_token,
        int32_t x_offset);

pv_status_t PV_MOCKABLE(pv_orca_duration_predictor_duration)(
        pv_ypu_t *ypu,
        int32_t num_channels,
        pv_ypu_mem_t *x,
        float speech_rate,
        int32_t x_offset,
        int32_t *duration_token);

#endif // PV_ORCA_DURATION_PREDICTOR_H
