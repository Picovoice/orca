#ifndef PV_LAYER_NORM_H
#define PV_LAYER_NORM_H

#include "util/pv_file.h"

typedef struct {
    int32_t num_channels;
    const q7_t *weight;
    const q7_t *bias;
    float eps;
} pv_layer_norm_param_t;

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_layer_norm_param_serialize_buffer)(const pv_layer_norm_param_t *param, size_t *length, void **buffer);

pv_status_t PV_MOCKABLE(pv_layer_norm_param_serialize)(const pv_layer_norm_param_t *param, FILE *file);

#endif

void PV_MOCKABLE(pv_layer_norm_param_delete)(pv_layer_norm_param_t *param);

pv_status_t PV_MOCKABLE(pv_layer_norm_param_load)(FILE *f, pv_layer_norm_param_t **param);

bool PV_MOCKABLE(pv_layer_norm_param_is_equal)(
        const pv_layer_norm_param_t *object,
        const pv_layer_norm_param_t *other);

typedef struct pv_layer_norm pv_layer_norm_t;

pv_status_t PV_MOCKABLE(pv_layer_norm_init)(
        const pv_layer_norm_param_t *param,
        pv_layer_norm_t **object);

void PV_MOCKABLE(pv_layer_norm_delete)(pv_layer_norm_t *object);

int32_t PV_MOCKABLE(pv_layer_norm_num_channels)(const pv_layer_norm_t *object);

pv_status_t PV_MOCKABLE(pv_layer_norm_forward)(pv_layer_norm_t *object, int32_t n, const float *x, float *y);

#endif // PV_LAYER_NORM_H
