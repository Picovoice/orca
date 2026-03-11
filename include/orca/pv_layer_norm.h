#ifndef PV_LAYER_NORM_H
#define PV_LAYER_NORM_H

#include "util/pv_file.h"
#include "ypu/pv_ypu.h"

typedef struct {
    int32_t num_channels;
    bool elementwise_affine;
    float eps;
    pv_ypu_config_mem_t *weight;
    pv_ypu_config_mem_t *bias;
} pv_layer_norm_param_t;

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_layer_norm_param_serialize_buffer)(
        pv_ypu_t *ypu,
        const pv_layer_norm_param_t *param,
        size_t *length,
        bool elementwise_affine,
        void **buffer);

pv_status_t PV_MOCKABLE(pv_layer_norm_param_serialize)(
        pv_ypu_t *ypu,
        const pv_layer_norm_param_t *param,
        bool elementwise_affine,
        FILE *file);

#endif

void PV_MOCKABLE(pv_layer_norm_param_delete)(
        pv_ypu_t *ypu,
        pv_layer_norm_param_t *param);

pv_status_t PV_MOCKABLE(pv_layer_norm_param_load)(
        pv_ypu_t *ypu,
        FILE *f,
        bool elementwise_affine,
        pv_layer_norm_param_t **param);

bool PV_MOCKABLE(pv_layer_norm_param_is_equal)(
        const pv_layer_norm_param_t *object,
        const pv_layer_norm_param_t *other);

typedef struct pv_layer_norm pv_layer_norm_t;

pv_status_t PV_MOCKABLE(pv_layer_norm_init)(
        pv_ypu_t *ypu,
        const pv_layer_norm_param_t *param,
        pv_layer_norm_t **object);

void PV_MOCKABLE(pv_layer_norm_delete)(
        pv_ypu_t *ypu,
        pv_layer_norm_t *object);

int32_t PV_MOCKABLE(pv_layer_norm_num_channels)(const pv_layer_norm_t *object);

pv_status_t PV_MOCKABLE(pv_layer_norm_forward)(
        pv_ypu_t *ypu,
        pv_layer_norm_t *object,
        int32_t n,
        pv_ypu_mem_t *x,
        pv_ypu_mem_t *y);

#endif // PV_LAYER_NORM_H
