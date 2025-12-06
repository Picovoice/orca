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

pv_status_t PV_MOCKABLE(pv_transformer_param_serialize)(pv_ypu_t *ypu, const pv_transformer_param_t *param, FILE *file);

#endif

void PV_MOCKABLE(pv_transformer_param_delete)(pv_ypu_t *ypu, pv_transformer_param_t *param);

pv_status_t PV_MOCKABLE(pv_transformer_param_load)(pv_ypu_t *ypu, FILE *f, pv_transformer_param_t **param);

bool PV_MOCKABLE(pv_transformer_param_is_equal)(
        const pv_transformer_param_t *object,
        const pv_transformer_param_t *other);

typedef struct pv_transformer pv_transformer_t;

pv_status_t PV_MOCKABLE(pv_transformer_init)(
        pv_ypu_t *ypu,
        const pv_transformer_param_t *param,
        pv_transformer_t **object);

void PV_MOCKABLE(pv_transformer_delete)(pv_ypu_t *ypu, pv_transformer_t *object);

pv_status_t PV_MOCKABLE(pv_transformer_forward)(
        pv_ypu_t *ypu,
        pv_transformer_t *object,
        int32_t n,
        pv_ypu_mem_t *x,
        pv_ypu_mem_t *y,
        int32_t x_offset,
        int32_t y_offset);

#endif // PV_TRANSFORMER_H
