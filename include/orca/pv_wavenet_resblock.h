#ifndef PV_WAVENET_RESBLOCK_H
#define PV_WAVENET_RESBLOCK_H

#include "orca/pv_cnn.h"
#include "ypu/pv_ypu.h"

typedef struct {
    const pv_cnn_param_t *conv_param;
    const pv_cnn_param_t *conv_skip_param;
} pv_wavenet_resblock_param_t;

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_wavenet_resblock_param_serialize)(
        pv_ypu_t *ypu,
        const pv_wavenet_resblock_param_t *param,
        FILE *file);

#endif

void PV_MOCKABLE(pv_wavenet_resblock_param_delete)(pv_ypu_t *ypu, pv_wavenet_resblock_param_t *param);

pv_status_t PV_MOCKABLE(pv_wavenet_resblock_param_load)(pv_ypu_t *ypu, FILE *f, pv_wavenet_resblock_param_t **param);

bool PV_MOCKABLE(pv_wavenet_resblock_param_is_equal)(
        const pv_wavenet_resblock_param_t *object,
        const pv_wavenet_resblock_param_t *other);

typedef struct pv_wavenet_resblock pv_wavenet_resblock_t;

pv_status_t PV_MOCKABLE(pv_wavenet_resblock_init)(
        pv_ypu_t *ypu,
        const pv_wavenet_resblock_param_t *param,
        pv_wavenet_resblock_t **object);

void PV_MOCKABLE(pv_wavenet_resblock_delete)(pv_ypu_t *ypu, pv_wavenet_resblock_t *object);

int32_t PV_MOCKABLE(pv_wavenet_resblock_output_channels)(const pv_wavenet_resblock_t *object);

pv_status_t PV_MOCKABLE(pv_wavenet_resblock_forward)(
        pv_ypu_t *ypu,
        pv_wavenet_resblock_t *object,
        bool last_block,
        int32_t n,
        pv_ypu_mem_t *x,
        pv_ypu_mem_t *y,
        int32_t x_offset,
        int32_t y_offset);

#endif // PV_WAVENET_RESBLOCK_H
