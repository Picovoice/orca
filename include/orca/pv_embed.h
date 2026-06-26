#ifndef PV_EMBED_H
#define PV_EMBED_H

#include "util/pv_file.h"
#include "ypu/pv_ypu.h"

typedef struct {
    int32_t num_embeddings;
    int32_t output_channels;
    const float *weight;
} pv_embed_param_t;

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_embed_param_serialize_buffer)(
        pv_ypu_t *ypu,
        const pv_embed_param_t *param,
        size_t *length,
        void **buffer);

pv_status_t PV_MOCKABLE(pv_embed_param_serialize)(pv_ypu_t *ypu, const pv_embed_param_t *param, FILE *file);

#endif

void PV_MOCKABLE(pv_embed_param_delete)(pv_ypu_t *ypu, pv_embed_param_t *param);

pv_status_t PV_MOCKABLE(pv_embed_param_load)(pv_ypu_t *ypu, FILE *f, pv_embed_param_t **param);

bool PV_MOCKABLE(pv_embed_param_is_equal)(
        const pv_embed_param_t *object,
        const pv_embed_param_t *other);

typedef struct pv_embed pv_embed_t;

pv_status_t PV_MOCKABLE(pv_embed_init)(pv_ypu_t *ypu, const pv_embed_param_t *param, pv_embed_t **object);

void PV_MOCKABLE(pv_embed_delete)(pv_ypu_t *ypu, pv_embed_t *object);

int32_t PV_MOCKABLE(pv_embed_dimension)(const pv_embed_t *object);

int32_t PV_MOCKABLE(pv_embed_num_embeddings)(const pv_embed_t *object);

pv_status_t PV_MOCKABLE(pv_embed_get)(
        pv_ypu_t *ypu,
        const pv_embed_t *object,
        int32_t index,
        pv_ypu_mem_t *y,
        int32_t y_offset);

pv_status_t PV_MOCKABLE(pv_embed_forward)(
        pv_ypu_t *ypu,
        const pv_embed_t *object,
        int32_t n,
        const int32_t *x,
        pv_ypu_mem_t *y,
        int32_t y_offset);

#endif // PV_EMBED_H
