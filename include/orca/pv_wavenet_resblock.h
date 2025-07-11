#ifndef PV_WAVENET_RESBLOCK_H
#define PV_WAVENET_RESBLOCK_H

#include "orca/pv_cnn.h"

typedef struct {
    const pv_cnn_param_t *conv_param;
    const pv_cnn_param_t *conv_skip_param;
} pv_wavenet_resblock_param_t;

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_wavenet_resblock_param_serialize)(const pv_wavenet_resblock_param_t *param, FILE *file);

#endif

void PV_MOCKABLE(pv_wavenet_resblock_param_delete)(pv_wavenet_resblock_param_t *param);

pv_status_t PV_MOCKABLE(pv_wavenet_resblock_param_load)(FILE *f, pv_wavenet_resblock_param_t **param);

bool PV_MOCKABLE(pv_wavenet_resblock_param_is_equal)(
        const pv_wavenet_resblock_param_t *object,
        const pv_wavenet_resblock_param_t *other);

typedef struct pv_wavenet_resblock pv_wavenet_resblock_t;

pv_status_t PV_MOCKABLE(pv_wavenet_resblock_init)(
        const pv_wavenet_resblock_param_t *param,
        pv_buffer_t *buffer_flow_wavenet_hidden,
        pv_buffer_t *buffer_flow_wavenet_inter,
        pv_buffer_t *buffer_flow_wavenet_inter_out,
        pv_wavenet_resblock_t **object);

void PV_MOCKABLE(pv_wavenet_resblock_delete)(pv_wavenet_resblock_t *object);

int32_t PV_MOCKABLE(pv_wavenet_resblock_output_channels)(const pv_wavenet_resblock_t *object);

pv_status_t PV_MOCKABLE(pv_wavenet_resblock_forward)(
        pv_wavenet_resblock_t *object,
        bool last_block,
        int32_t n,
        float *x,
        float *y);

#endif // PV_WAVENET_RESBLOCK_H
