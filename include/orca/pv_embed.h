#ifndef PV_EMBED_H
#define PV_EMBED_H

#include "util/pv_file.h"

typedef struct {
    int32_t num_embeddings;
    int32_t output_channels;
    const float *weight;
} pv_embed_param_t;

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_embed_param_serialize_buffer)(const pv_embed_param_t *param, size_t *length, void **buffer);

pv_status_t PV_MOCKABLE(pv_embed_param_serialize)(const pv_embed_param_t *param, FILE *file);

#endif

void PV_MOCKABLE(pv_embed_param_delete)(pv_embed_param_t *param);

pv_status_t PV_MOCKABLE(pv_embed_param_load)(FILE *f, pv_embed_param_t **param);

bool PV_MOCKABLE(pv_embed_param_is_equal)(
        const pv_embed_param_t *object,
        const pv_embed_param_t *other);

typedef struct pv_embed pv_embed_t;

pv_status_t PV_MOCKABLE(pv_embed_init)(const pv_embed_param_t *param, pv_embed_t **object);

void PV_MOCKABLE(pv_embed_delete)(pv_embed_t *object);

int32_t PV_MOCKABLE(pv_embed_dimension)(const pv_embed_t *object);

int32_t PV_MOCKABLE(pv_embed_num_embeddings)(const pv_embed_t *object);

void PV_MOCKABLE(pv_embed_get)(const pv_embed_t *object, int32_t index, float *y);

pv_status_t PV_MOCKABLE(pv_embed_forward)(const pv_embed_t *object, int32_t n, const int32_t *x, float *y);

#endif // PV_EMBED_H
