#ifndef PV_RESIDUAL_COUPLING_H
#define PV_RESIDUAL_COUPLING_H

#include "orca/pv_cnn.h"
#include "orca/pv_wavenet_resblock.h"
#include "util/pv_file.h"
#include "ypu/pv_ypu.h"

typedef struct {
    const pv_cnn_param_t *conv_pre_param;
    const pv_cnn_param_t *conv_post_param;
    int32_t num_wavenet_resblocks;
    const pv_wavenet_resblock_param_t **wavenet_resblocks_param;
} pv_residual_coupling_param_t;

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_residual_coupling_param_serialize)(
        pv_ypu_t *ypu,
        const pv_residual_coupling_param_t *param,
        FILE *file);

#endif

void PV_MOCKABLE(pv_residual_coupling_param_delete)(pv_ypu_t *ypu, pv_residual_coupling_param_t *param);

pv_status_t PV_MOCKABLE(pv_residual_coupling_param_load)(pv_ypu_t *ypu, FILE *f, pv_residual_coupling_param_t **param);

bool PV_MOCKABLE(pv_residual_coupling_param_is_equal)(
        const pv_residual_coupling_param_t *object,
        const pv_residual_coupling_param_t *other);

typedef struct pv_residual_coupling pv_residual_coupling_t;

pv_status_t PV_MOCKABLE(pv_residual_coupling_init)(
        pv_ypu_t *ypu,
        const pv_residual_coupling_param_t *param,
        pv_residual_coupling_t **object);

void PV_MOCKABLE(pv_residual_coupling_delete)(pv_ypu_t *ypu, pv_residual_coupling_t *object);

int32_t PV_MOCKABLE(pv_residual_coupling_output_channels)(const pv_residual_coupling_t *object);

pv_status_t PV_MOCKABLE(pv_residual_coupling_forward)(
        pv_ypu_t *ypu,
        pv_residual_coupling_t *object,
        int32_t n,
        pv_ypu_mem_t *x,
        pv_ypu_mem_t *y,
        int32_t x_offset,
        int32_t y_offset);

#endif // PV_RESIDUAL_COUPLING_H
