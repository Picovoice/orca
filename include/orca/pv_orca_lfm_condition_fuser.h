#ifndef PV_ORCA_LFM_CONDITION_FUSER_H
#define PV_ORCA_LFM_CONDITION_FUSER_H

#include "orca/pv_cnn.h"
#include "util/pv_file.h"
#include "ypu/pv_ypu.h"

typedef struct {
    const pv_cnn_param_t *conv_1_param;
    const pv_cnn_param_t *conv_2_param;
} pv_orca_lfm_condition_fuser_param_t;

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_orca_lfm_condition_fuser_param_serialize)(
        pv_ypu_t *ypu,
        const pv_orca_lfm_condition_fuser_param_t *param,
        FILE *file);

#endif

void PV_MOCKABLE(pv_orca_lfm_condition_fuser_param_delete)(
        pv_ypu_t *ypu,
        pv_orca_lfm_condition_fuser_param_t *param);

pv_status_t PV_MOCKABLE(pv_orca_lfm_condition_fuser_param_load)(
        pv_ypu_t *ypu,
        FILE *f,
        pv_orca_lfm_condition_fuser_param_t **param);

bool PV_MOCKABLE(pv_orca_lfm_condition_fuser_param_is_equal)(
        const pv_orca_lfm_condition_fuser_param_t *object,
        const pv_orca_lfm_condition_fuser_param_t *other);

typedef struct pv_orca_lfm_condition_fuser pv_orca_lfm_condition_fuser_t;

pv_status_t PV_MOCKABLE(pv_orca_lfm_condition_fuser_init)(
        pv_ypu_t *ypu,
        const pv_orca_lfm_condition_fuser_param_t *param,
        pv_orca_lfm_condition_fuser_t **object);

void PV_MOCKABLE(pv_orca_lfm_condition_fuser_delete)(
        pv_ypu_t *ypu,
        pv_orca_lfm_condition_fuser_t *object);

pv_status_t PV_MOCKABLE(pv_orca_lfm_condition_fuser_forward)(
        pv_ypu_t *ypu,
        pv_orca_lfm_condition_fuser_t *object,
        int32_t n,
        pv_ypu_mem_t *content_condition,
        pv_ypu_mem_t *time_condition,
        pv_ypu_mem_t *fused_condition,
        int32_t content_condition_offset,
        int32_t time_condition_offset,
        int32_t fused_condition_offset);

#endif // PV_ORCA_LFM_CONDITION_FUSER_H
