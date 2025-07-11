#ifndef PV_ORCA_PQMF_H
#define PV_ORCA_PQMF_H

#include "core/picovoice.h"

pv_status_t PV_MOCKABLE(pv_orca_pqmf_synthesis)(
        int32_t num_subbands,
        int32_t n,
        const float *x,
        float *y);

#endif // PV_ORCA_PQMF_H
