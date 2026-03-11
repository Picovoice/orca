#ifndef PV_ROPE_ATTENTION_H
#define PV_ROPE_ATTENTION_H

#include "orca/pv_cnn.h"
#include "util/pv_file.h"
#include "ypu/pv_ypu.h"

typedef struct {
    int32_t dimension;
    int32_t head_dimension;
    int32_t num_heads;
    int32_t ffn_intermediate_dimension;
    int32_t num_lookaheads;
    int32_t num_lookbacks;
    float rope_base;
    int32_t sdpa_downsample_factor;

    const pv_cnn_param_t *conv_q_param;
    const pv_cnn_param_t *conv_k_param;
    const pv_cnn_param_t *conv_v_param;
    const pv_cnn_param_t *conv_o_param;
} pv_rope_attention_param_t;

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_rope_attention_param_serialize)(
        pv_ypu_t *ypu,
        const pv_rope_attention_param_t *param,
        FILE *file);

#endif

void PV_MOCKABLE(pv_rope_attention_param_delete)(
        pv_ypu_t *ypu,
        pv_rope_attention_param_t *param);

pv_status_t PV_MOCKABLE(pv_rope_attention_param_load)(
        pv_ypu_t *ypu,
        FILE *f,
        pv_rope_attention_param_t **param);

bool PV_MOCKABLE(pv_rope_attention_param_is_equal)(
        const pv_rope_attention_param_t *object,
        const pv_rope_attention_param_t *other);

typedef struct pv_rope_attention {
    const pv_rope_attention_param_t *param;

    pv_cnn_t *conv_q;
    pv_cnn_t *conv_k;
    pv_cnn_t *conv_v;
    pv_cnn_t *conv_o;
} pv_rope_attention_t;

pv_status_t PV_MOCKABLE(pv_rope_attention_init)(
        pv_ypu_t *ypu,
        const pv_rope_attention_param_t *param,
        pv_rope_attention_t **object);

void PV_MOCKABLE(pv_rope_attention_delete)(
        pv_ypu_t *ypu,
        pv_rope_attention_t *object);

pv_status_t PV_MOCKABLE(pv_rope)(
        pv_ypu_t *ypu,
        pv_rope_attention_t *object,
        int32_t n,
        pv_ypu_mem_t *x,
        pv_ypu_mem_t *y);

pv_status_t PV_MOCKABLE(pv_rope_attention_forward)(
        pv_ypu_t *ypu,
        pv_rope_attention_t *object,
        int32_t n,
        pv_ypu_mem_t *x,
        pv_ypu_mem_t *bucket,
        pv_ypu_mem_t *y);

#endif // PV_ROPE_ATTENTION_H
