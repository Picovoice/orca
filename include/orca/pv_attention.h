#ifndef PV_ATTENTION_H
#define PV_ATTENTION_H

#include "orca/pv_buffer.h"
#include "orca/pv_cnn.h"
#include "orca/pv_embed.h"

typedef struct {
    int32_t num_channels;
    int32_t num_heads;
    int32_t pos_embed_window_size;
    int32_t attention_window_future;
    const pv_embed_param_t *emb_rel_k_param;
    const pv_embed_param_t *emb_rel_v_param;
    const pv_cnn_param_t *q_param;
    const pv_cnn_param_t *k_param;
    const pv_cnn_param_t *v_param;
    const pv_cnn_param_t *o_param;
} pv_attention_param_t;

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_attention_param_serialize)(const pv_attention_param_t *param, FILE *file);

#endif

void PV_MOCKABLE(pv_attention_param_delete)(pv_attention_param_t *param);

pv_status_t PV_MOCKABLE(pv_attention_param_load)(FILE *f, pv_attention_param_t **param);

bool PV_MOCKABLE(pv_attention_param_is_equal)(
        const pv_attention_param_t *object,
        const pv_attention_param_t *other);

int32_t PV_MOCKABLE(pv_attention_param_receptive_field)(const pv_attention_param_t *param);

typedef struct pv_attention pv_attention_t;

pv_status_t PV_MOCKABLE(pv_attention_encoding_init)(const pv_embed_t *embed_object, float **encoding);

pv_status_t PV_MOCKABLE(pv_attention_init)(
        const pv_attention_param_t *param,
        pv_buffer_t *buffer_text_encoder_transf_attn_1,
        pv_buffer_t *buffer_text_encoder_transf_attn_2,
        pv_buffer_t *buffer_text_encoder_transf_attn_score,
        pv_attention_t **object);

void PV_MOCKABLE(pv_attention_delete)(pv_attention_t *object);

pv_status_t PV_MOCKABLE(pv_attention_forward)(
        pv_attention_t *object,
        int32_t n,
        const float *query,
        const float *key_and_value,
        float *y);

#endif // PV_ATTENTION_H
