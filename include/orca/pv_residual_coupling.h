#ifndef PV_RESIDUAL_COUPLING_H
#define PV_RESIDUAL_COUPLING_H

#include "orca/pv_buffer.h"
#include "orca/pv_cnn.h"
#include "orca/pv_wavenet_resblock.h"
#include "util/pv_file.h"

typedef struct {
    const pv_cnn_param_t *conv_pre_param;
    const pv_cnn_param_t *conv_post_param;
    int32_t num_wavenet_resblocks;
    const pv_wavenet_resblock_param_t **wavenet_resblocks_param;
} pv_residual_coupling_param_t;

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_residual_coupling_param_serialize)(const pv_residual_coupling_param_t *param, FILE *file);

#endif

void PV_MOCKABLE(pv_residual_coupling_param_delete)(pv_residual_coupling_param_t *param);

pv_status_t PV_MOCKABLE(pv_residual_coupling_param_load)(FILE *f, pv_residual_coupling_param_t **param);

bool PV_MOCKABLE(pv_residual_coupling_param_is_equal)(
        const pv_residual_coupling_param_t *object,
        const pv_residual_coupling_param_t *other);

typedef struct pv_residual_coupling pv_residual_coupling_t;

pv_status_t PV_MOCKABLE(pv_residual_coupling_init)(
        const pv_residual_coupling_param_t *param,
        pv_buffer_t *buffer_flow_residual_coupling_x0,
        pv_buffer_t *buffer_flow_residual_coupling_x1,
        pv_buffer_t *buffer_flow_residual_coupling_mean,
        pv_buffer_t *buffer_flow_wavenet_in,
        pv_buffer_t *buffer_flow_wavenet_hidden,
        pv_buffer_t *buffer_flow_wavenet_inter,
        pv_buffer_t *buffer_flow_wavenet_inter_out,
        pv_buffer_t *buffer_flow_wavenet_out,
        pv_residual_coupling_t **object);

void PV_MOCKABLE(pv_residual_coupling_delete)(pv_residual_coupling_t *object);

int32_t PV_MOCKABLE(pv_residual_coupling_output_channels)(const pv_residual_coupling_t *object);

pv_status_t PV_MOCKABLE(pv_residual_coupling_forward)(
        pv_residual_coupling_t *object,
        int32_t n,
        const float *x,
        float *y);

#endif // PV_RESIDUAL_COUPLING_H
