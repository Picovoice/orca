#ifndef PV_TRANSFORMER_FFN_H
#define PV_TRANSFORMER_FFN_H

#include "orca/pv_cnn.h"
#include "orca/pv_buffer.h"

typedef struct {
    const pv_cnn_param_t *conv_1_param;
    const pv_cnn_param_t *conv_2_param;
} pv_transformer_ffn_param_t;

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_transformer_ffn_param_serialize)(const pv_transformer_ffn_param_t *param, FILE *file);

#endif

void PV_MOCKABLE(pv_transformer_ffn_param_delete)(pv_transformer_ffn_param_t *param);

pv_status_t PV_MOCKABLE(pv_transformer_ffn_param_load)(FILE *f, pv_transformer_ffn_param_t **param);

bool PV_MOCKABLE(pv_transformer_ffn_param_is_equal)(
        const pv_transformer_ffn_param_t *object,
        const pv_transformer_ffn_param_t *other);

int32_t PV_MOCKABLE(pv_transformer_ffn_param_receptive_field)(const pv_transformer_ffn_param_t *param);

typedef struct pv_transformer_ffn pv_transformer_ffn_t;

pv_status_t PV_MOCKABLE(pv_transformer_ffn_init)(
        const pv_transformer_ffn_param_t *param,
        pv_buffer_t *buffer_text_encoder_transf_ffn,
        pv_transformer_ffn_t **object);

void PV_MOCKABLE(pv_transformer_ffn_delete)(pv_transformer_ffn_t *object);

int32_t PV_MOCKABLE(pv_transformer_ffn_output_channels)(const pv_transformer_ffn_t *object);

pv_status_t PV_MOCKABLE(pv_transformer_ffn_forward)(pv_transformer_ffn_t *object, int32_t n, const float *x, float *y);

#endif // PV_TRANSFORMER_FFN_H
