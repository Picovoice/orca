#ifndef PV_ADANORM_H
#define PV_ADANORM_H

#include "orca/pv_cnn.h"
#include "util/pv_file.h"
#include "ypu/pv_ypu.h"

typedef struct {
    float eps;

    const pv_cnn_param_t *linear_param;
} pv_adanorm_param_t;

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_adanorm_param_serialize)(
        pv_ypu_t *ypu,
        const pv_adanorm_param_t *param,
        FILE *file);

#endif

void PV_MOCKABLE(pv_adanorm_param_delete)(
        pv_ypu_t *ypu,
        pv_adanorm_param_t *param);

pv_status_t PV_MOCKABLE(pv_adanorm_param_load)(
        pv_ypu_t *ypu,
        FILE *f,
        pv_adanorm_param_t **param);

bool PV_MOCKABLE(pv_adanorm_param_is_equal)(
        const pv_adanorm_param_t *object,
        const pv_adanorm_param_t *other);

typedef struct pv_adanorm pv_adanorm_t;

pv_status_t PV_MOCKABLE(pv_adanorm_init)(
        pv_ypu_t *ypu,
        const pv_adanorm_param_t *param,
        pv_adanorm_t **object);

void PV_MOCKABLE(pv_adanorm_delete)(
        pv_ypu_t *ypu,
        pv_adanorm_t *object);

pv_status_t PV_MOCKABLE(pv_adanorm_rope_transformer_forward)(
        pv_ypu_t *ypu,
        pv_adanorm_t *object,
        int32_t n,
        pv_ypu_mem_t *x,
        pv_ypu_mem_t *c,
        pv_ypu_mem_t *gates_list,
        pv_ypu_mem_t *y);

#endif // PV_ADANORM_H
