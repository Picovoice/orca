#ifndef PV_AFFINE_H
#define PV_AFFINE_H

#include "util/pv_file.h"

typedef struct {
    int32_t num_channels;
    int32_t scale_offset;
    const float *scale;
    const float *bias;
} pv_affine_param_t;

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_affine_param_serialize_buffer)(const pv_affine_param_t *param, size_t *length, void **buffer);

pv_status_t PV_MOCKABLE(pv_affine_param_serialize)(const pv_affine_param_t *param, FILE *file);

#endif

void PV_MOCKABLE(pv_affine_param_delete)(pv_affine_param_t *param);

pv_status_t PV_MOCKABLE(pv_affine_param_load)(FILE *f, pv_affine_param_t **param);

bool PV_MOCKABLE(pv_affine_param_is_equal)(const pv_affine_param_t *object, const pv_affine_param_t *other);

typedef struct pv_affine pv_affine_t;

pv_status_t PV_MOCKABLE(pv_affine_init)(const pv_affine_param_t *param, pv_affine_t **object);

void PV_MOCKABLE(pv_affine_delete)(pv_affine_t *object);

int32_t PV_MOCKABLE(pv_affine_num_channels)(const pv_affine_t *object);

pv_status_t PV_MOCKABLE(pv_affine_forward)(pv_affine_t *object, int32_t n, const float *x, float *y);

#endif // PV_AFFINE_H
