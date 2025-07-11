#ifndef PV_ORCA_DURATION_PREDICTOR_H
#define PV_ORCA_DURATION_PREDICTOR_H

#include "orca/pv_affine.h"
#include "orca/pv_cnn.h"
#include "orca/pv_embed.h"
#include "orca/pv_transformer.h"
#include "util/pv_file.h"

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
        const pv_orca_duration_predictor_param_t *param,
        FILE *file);

#endif

void PV_MOCKABLE(pv_orca_duration_predictor_param_delete)(pv_orca_duration_predictor_param_t *param);

pv_status_t PV_MOCKABLE(pv_orca_duration_predictor_param_load)(FILE *f, pv_orca_duration_predictor_param_t **param);

bool PV_MOCKABLE(pv_orca_duration_predictor_param_is_equal)(
        const pv_orca_duration_predictor_param_t *object,
        const pv_orca_duration_predictor_param_t *other);

int32_t PV_MOCKABLE(pv_orca_duration_predictor_param_receptive_field)(
        const pv_orca_duration_predictor_param_t *object);

typedef struct pv_orca_duration_predictor pv_orca_duration_predictor_t;

pv_status_t PV_MOCKABLE(pv_orca_duration_predictor_init)(
        const pv_orca_duration_predictor_param_t *param,
        pv_orca_duration_predictor_t **object);

void PV_MOCKABLE(pv_orca_duration_predictor_delete)(pv_orca_duration_predictor_t *object);

pv_status_t PV_MOCKABLE(pv_orca_duration_predictor_forward)(
        pv_orca_duration_predictor_t *object,
        float speech_rate,
        int32_t n,
        const float *x,
        int32_t *durations_token);

int32_t PV_MOCKABLE(pv_orca_duration_predictor_duration)(int32_t num_channels, float *x, float speech_rate);

#endif // PV_ORCA_DURATION_PREDICTOR_H
