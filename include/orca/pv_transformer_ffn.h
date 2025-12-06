#ifndef PV_TRANSFORMER_FFN_H
#define PV_TRANSFORMER_FFN_H

#include "orca/pv_cnn.h"
#include "ypu/pv_ypu.h"

typedef struct {
    const pv_cnn_param_t *conv_1_param;
    const pv_cnn_param_t *conv_2_param;
} pv_transformer_ffn_param_t;

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_transformer_ffn_param_serialize)(
        pv_ypu_t *ypu,
        const pv_transformer_ffn_param_t *param,
        FILE *file);

#endif

void PV_MOCKABLE(pv_transformer_ffn_param_delete)(pv_ypu_t *ypu, pv_transformer_ffn_param_t *param);

pv_status_t PV_MOCKABLE(pv_transformer_ffn_param_load)(pv_ypu_t *ypu, FILE *f, pv_transformer_ffn_param_t **param);

bool PV_MOCKABLE(pv_transformer_ffn_param_is_equal)(
        const pv_transformer_ffn_param_t *object,
        const pv_transformer_ffn_param_t *other);

int32_t PV_MOCKABLE(pv_transformer_ffn_param_receptive_field)(const pv_transformer_ffn_param_t *param);

typedef struct pv_transformer_ffn pv_transformer_ffn_t;

pv_status_t PV_MOCKABLE(pv_transformer_ffn_init)(
        pv_ypu_t *ypu,
        const pv_transformer_ffn_param_t *param,
        pv_transformer_ffn_t **object);

void PV_MOCKABLE(pv_transformer_ffn_delete)(pv_ypu_t *ypu, pv_transformer_ffn_t *object);

int32_t PV_MOCKABLE(pv_transformer_ffn_output_channels)(const pv_transformer_ffn_t *object);

pv_status_t PV_MOCKABLE(pv_transformer_ffn_forward)(
        pv_ypu_t *ypu,
        pv_transformer_ffn_t *object,
        int32_t n,
        pv_ypu_mem_t *x,
        pv_ypu_mem_t *y,
        int32_t x_offset,
        int32_t y_offset);

#endif // PV_TRANSFORMER_FFN_H
