#ifndef PV_ORCA_GAUSSIAN_UPSAMPLER_H
#define PV_ORCA_GAUSSIAN_UPSAMPLER_H

#include "orca/pv_rope_transformer.h"
#include "util/pv_file.h"
#include "ypu/pv_ypu.h"

typedef struct {
    int32_t dimension;
    int32_t num_lookaheads_gaussian_upsampling;
    int32_t num_lookbacks_gaussian_upsampling;
    const pv_rope_transformer_param_t *transformer_param;
} pv_orca_gaussian_upsampler_param_t;

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_orca_gaussian_upsampler_param_serialize)(
        pv_ypu_t *ypu,
        const pv_orca_gaussian_upsampler_param_t *param,
        FILE *file);

#endif

void PV_MOCKABLE(pv_orca_gaussian_upsampler_param_delete)(
        pv_ypu_t *ypu,
        pv_orca_gaussian_upsampler_param_t *param);

pv_status_t PV_MOCKABLE(pv_orca_gaussian_upsampler_param_load)(
        pv_ypu_t *ypu,
        FILE *f,
        pv_orca_gaussian_upsampler_param_t **param);

bool PV_MOCKABLE(pv_orca_gaussian_upsampler_param_is_equal)(
        const pv_orca_gaussian_upsampler_param_t *object,
        const pv_orca_gaussian_upsampler_param_t *other);

typedef struct pv_orca_gaussian_upsampler {
    const pv_orca_gaussian_upsampler_param_t *param;

    pv_rope_transformer_t *transformer;
} pv_orca_gaussian_upsampler_t;

pv_status_t PV_MOCKABLE(pv_orca_gaussian_upsampler_init)(
        pv_ypu_t *ypu,
        const pv_orca_gaussian_upsampler_param_t *param,
        pv_orca_gaussian_upsampler_t **object);

void PV_MOCKABLE(pv_orca_gaussian_upsampler_delete)(
        pv_ypu_t *ypu,
        pv_orca_gaussian_upsampler_t *object);

pv_status_t PV_MOCKABLE(pv_orca_gaussian_upsampler_attention)(
        pv_ypu_t *ypu,
        pv_orca_gaussian_upsampler_t *object,
        int32_t n,
        int32_t T,
        const int32_t *d,
        pv_ypu_mem_t *buffer_gaussian_center,
        pv_ypu_mem_t *std,
        pv_ypu_mem_t *v,
        pv_ypu_mem_t *y);

pv_status_t PV_MOCKABLE(pv_orca_gaussian_upsampler_forward)(
        pv_ypu_t *ypu,
        pv_orca_gaussian_upsampler_t *object,
        int32_t n,
        int32_t T,
        pv_ypu_mem_t *x,
        int32_t *d,
        pv_ypu_mem_t *std,
        pv_ypu_mem_t *y);

#endif // PV_ORCA_GAUSSIAN_UPSAMPLER_H
