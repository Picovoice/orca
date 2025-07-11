#ifndef PV_TRANSFORMER_H
#define PV_TRANSFORMER_H

#include "orca/pv_attention.h"
#include "orca/pv_transformer_ffn.h"
#include "orca/pv_layer_norm.h"
#include "util/pv_file.h"

typedef struct {
    const pv_layer_norm_param_t *layer_norm_attention_param;
    const pv_layer_norm_param_t *layer_norm_ffn_param;
    const pv_attention_param_t *attention_param;
    const pv_transformer_ffn_param_t *ffn_param;
} pv_transformer_param_t;

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_transformer_param_serialize)(const pv_transformer_param_t *param, FILE *file);

#endif

void PV_MOCKABLE(pv_transformer_param_delete)(pv_transformer_param_t *param);

pv_status_t PV_MOCKABLE(pv_transformer_param_load)(FILE *f, pv_transformer_param_t **param);

bool PV_MOCKABLE(pv_transformer_param_is_equal)(
        const pv_transformer_param_t *object,
        const pv_transformer_param_t *other);

typedef struct pv_transformer pv_transformer_t;

pv_status_t PV_MOCKABLE(pv_transformer_init)(
        const pv_transformer_param_t *param,
        pv_buffer_t *buffer_text_encoder_transf_attn_1,
        pv_buffer_t *buffer_text_encoder_transf_attn_2,
        pv_buffer_t *buffer_text_encoder_transf_attn_score,
        pv_buffer_t *buffer_text_encoder_transf_ffn,
        pv_transformer_t **object);

void PV_MOCKABLE(pv_transformer_delete)(pv_transformer_t *object);

pv_status_t PV_MOCKABLE(pv_transformer_forward)(
        pv_transformer_t *object,
        int32_t n,
        const float *x,
        float *y);

#endif // PV_TRANSFORMER_H
