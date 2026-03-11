#ifndef PV_ADDITIVE_COUPLING_H
#define PV_ADDITIVE_COUPLING_H

#include "orca/pv_cnn.h"
#include "orca/pv_rope_transformer_film_conditioned.h"
#include "util/pv_file.h"
#include "ypu/pv_ypu.h"

typedef struct {
    const pv_cnn_param_t *conv_pre_param;
    const pv_rope_transformer_film_conditioned_param_t *transformer_param;
    const pv_cnn_param_t *conv_post_param;
} pv_additive_coupling_param_t;

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_additive_coupling_param_serialize)(
        pv_ypu_t *ypu,
        const pv_additive_coupling_param_t *param,
        FILE *file);

#endif

void PV_MOCKABLE(pv_additive_coupling_param_delete)(
        pv_ypu_t *ypu,
        pv_additive_coupling_param_t *param);

pv_status_t PV_MOCKABLE(pv_additive_coupling_param_load)(
        pv_ypu_t *ypu,
        FILE *f,
        pv_additive_coupling_param_t **param);

bool PV_MOCKABLE(pv_additive_coupling_param_is_equal)(
        const pv_additive_coupling_param_t *object,
        const pv_additive_coupling_param_t *other);

typedef struct pv_additive_coupling pv_additive_coupling_t;

pv_status_t PV_MOCKABLE(pv_additive_coupling_init)(
        pv_ypu_t *ypu,
        const pv_additive_coupling_param_t *param,
        pv_additive_coupling_t **object);

void PV_MOCKABLE(pv_additive_coupling_delete)(
        pv_ypu_t *ypu,
        pv_additive_coupling_t *object);

pv_status_t PV_MOCKABLE(pv_additive_coupling_forward)(
        pv_ypu_t *ypu,
        pv_additive_coupling_t *object,
        int32_t n,
        pv_ypu_mem_t *c,
        pv_ypu_mem_t *x,
        pv_ypu_mem_t *y);


#endif // PV_ADDITIVE_COUPLING_H
