#ifndef PV_ORCA_LFM_VF_ESTIMATOR_H
#define PV_ORCA_LFM_VF_ESTIMATOR_H

#include "orca/pv_cnn.h"
#include "orca/pv_convnext_film.h"
#include "orca/pv_layer_norm.h"
#include "orca/pv_orca_lfm_vf_estimator_param.h"
#include "orca/pv_orca_stream_state.h"
#include "util/pv_file.h"
#include "ypu/pv_ypu.h"

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_orca_lfm_vf_estimator_param_serialize)(
        pv_ypu_t *ypu,
        const pv_orca_lfm_vf_estimator_param_t *param, FILE *file);

#endif

void PV_MOCKABLE(pv_orca_lfm_vf_estimator_param_delete)(
        pv_ypu_t *ypu,
        pv_orca_lfm_vf_estimator_param_t *param);

pv_status_t PV_MOCKABLE(pv_orca_lfm_vf_estimator_param_load)(
        pv_ypu_t *ypu,
        FILE *f,
        pv_orca_lfm_vf_estimator_param_t **param);

bool PV_MOCKABLE(pv_orca_lfm_vf_estimator_param_is_equal)(
        const pv_orca_lfm_vf_estimator_param_t *object,
        const pv_orca_lfm_vf_estimator_param_t *other);

typedef struct pv_orca_lfm_vf_estimator pv_orca_lfm_vf_estimator_t;

pv_status_t PV_MOCKABLE(pv_orca_lfm_vf_estimator_init)(
        pv_ypu_t *ypu,
        const pv_orca_lfm_vf_estimator_param_t *param,
        pv_orca_lfm_vf_estimator_t **object);

void PV_MOCKABLE(pv_orca_lfm_vf_estimator_delete)(
        pv_ypu_t *ypu,
        pv_orca_lfm_vf_estimator_t *object);

pv_status_t PV_MOCKABLE(pv_orca_lfm_vf_estimator_forward)(
        pv_ypu_t *ypu,
        pv_orca_lfm_vf_estimator_t *object,
        pv_orca_stream_state_t *state,
        int32_t n,
        pv_ypu_mem_t *x,
        pv_ypu_mem_t *c,
        pv_ypu_mem_t *y,
        int32_t x_offset,
        int32_t c_offset,
        int32_t y_offset);

#endif // PV_ORCA_LFM_VF_ESTIMATOR_H
