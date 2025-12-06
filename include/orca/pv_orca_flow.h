#ifndef PV_ORCA_FLOW_H
#define PV_ORCA_FLOW_H

#include "orca/pv_residual_coupling.h"
#include "util/pv_file.h"
#include "ypu/pv_ypu.h"

/* Set manual receptive field size for Orca flow. Since the latents are sampled randomly, the edges
 * don't need to be exact for subsequent next audio chunks. */
static const int32_t PV_ORCA_FLOW_RECEPTIVE_FIELD = 10;

typedef struct {
    int32_t num_flows;
    const pv_residual_coupling_param_t **flows_param;
} pv_orca_flow_param_t;

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_orca_flow_param_serialize)(pv_ypu_t *ypu, const pv_orca_flow_param_t *param, FILE *file);

#endif

void PV_MOCKABLE(pv_orca_flow_param_delete)(pv_ypu_t *ypu, pv_orca_flow_param_t *param);

pv_status_t PV_MOCKABLE(pv_orca_flow_param_load)(pv_ypu_t *ypu, FILE *f, pv_orca_flow_param_t **param);

bool PV_MOCKABLE(pv_orca_flow_param_is_equal)(
        const pv_orca_flow_param_t *object,
        const pv_orca_flow_param_t *other);

typedef struct pv_orca_flow pv_orca_flow_t;

pv_status_t PV_MOCKABLE(pv_orca_flow_init)(
        pv_ypu_t *ypu,
        const pv_orca_flow_param_t *param,
        pv_orca_flow_t **object);

void PV_MOCKABLE(pv_orca_flow_delete)(pv_ypu_t *ypu, pv_orca_flow_t *object);

pv_status_t PV_MOCKABLE(pv_orca_flow_forward)(
        pv_ypu_t *ypu,
        pv_orca_flow_t *object,
        int32_t n,
        pv_ypu_mem_t *x,
        pv_ypu_mem_t *y,
        int32_t x_offset,
        int32_t y_offset);

#endif // PV_ORCA_FLOW_H
