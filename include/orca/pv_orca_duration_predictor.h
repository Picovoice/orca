#ifndef PV_ORCA_DURATION_PREDICTOR_H
#define PV_ORCA_DURATION_PREDICTOR_H

#include "orca/pv_cnn.h"
#include "orca/pv_rope_transformer.h"
#include "util/pv_file.h"
#include "ypu/pv_ypu.h"

typedef struct {
    const pv_rope_transformer_param_t *transformer_param;
    const pv_cnn_param_t *conv_proj_param;
} pv_orca_duration_predictor_param_t;

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_orca_duration_predictor_param_serialize)(
        pv_ypu_t *ypu,
        const pv_orca_duration_predictor_param_t *param,
        FILE *file);

#endif

void PV_MOCKABLE(pv_orca_duration_predictor_param_delete)(
        pv_ypu_t *ypu,
        pv_orca_duration_predictor_param_t *param);

pv_status_t PV_MOCKABLE(pv_orca_duration_predictor_param_load)(
        pv_ypu_t *ypu,
        FILE *f,
        pv_orca_duration_predictor_param_t **param);

bool PV_MOCKABLE(pv_orca_duration_predictor_param_is_equal)(
        const pv_orca_duration_predictor_param_t *object,
        const pv_orca_duration_predictor_param_t *other);

typedef struct pv_orca_duration_predictor pv_orca_duration_predictor_t;

pv_status_t PV_MOCKABLE(pv_orca_duration_predictor_init)(
        pv_ypu_t *ypu,
        const pv_orca_duration_predictor_param_t *param,
        pv_orca_duration_predictor_t **object);

void PV_MOCKABLE(pv_orca_duration_predictor_delete)(
        pv_ypu_t *ypu,
        pv_orca_duration_predictor_t *object);

pv_status_t PV_MOCKABLE(pv_orca_duration_predictor_forward)(
        pv_ypu_t *ypu,
        pv_orca_duration_predictor_t *object,
        int32_t n,
        float speech_rate,
        pv_ypu_mem_t *x,
        int32_t *d,
        pv_ypu_mem_t *std);

#endif // PV_ORCA_DURATION_PREDICTOR_H
