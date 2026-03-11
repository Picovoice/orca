#ifndef PV_ORCA_LFM_VF_ESTIMATOR_PARAM_H
#define PV_ORCA_LFM_VF_ESTIMATOR_PARAM_H

#include "orca/pv_cnn.h"
#include "orca/pv_convnext_film.h"
#include "orca/pv_layer_norm.h"

typedef struct pv_orca_lfm_vf_estimator_param {
    int32_t num_blocks;
    int32_t dimension;
    int32_t out_dimension;

    const pv_cnn_param_t *conv_pre_param;
    const pv_cnn_param_t *adanorm_linear_param;
    const pv_convnext_film_param_t **convnext_blocks_param;
    const pv_layer_norm_param_t *layer_norm_out_param;
    const pv_cnn_param_t *conv_out_param;
} pv_orca_lfm_vf_estimator_param_t;

#endif // PV_ORCA_LFM_VF_ESTIMATOR_PARAM_H
