#ifndef PV_ORCA_UTIL_H
#define PV_ORCA_UTIL_H

#include <stdlib.h>

#include "core/picovoice.h"
#include "core/pv_type.h"
#include "ypu/pv_ypu.h"

typedef struct {
    float value;
    bool has_value;
} pv_orca_util_rand_normal_t;

pv_status_t PV_MOCKABLE(pv_orca_util_rand_normal_init)(pv_orca_util_rand_normal_t **object);

void PV_MOCKABLE(pv_orca_util_rand_normal_delete)(pv_orca_util_rand_normal_t *object);

float PV_MOCKABLE(pv_orca_util_rand_normal_sample)(pv_orca_util_rand_normal_t *object, uint64_t *state);

void PV_MOCKABLE(pv_orca_util_generate_bucket)(
        pv_ypu_t *ypu,
        int32_t n,
        int32_t bucket_offset,
        const int32_t *durations,
        pv_ypu_mem_t *bucket);

pv_status_t PV_MOCKABLE(pv_orca_util_sample_standard_gaussian_with_temperature)(
        pv_ypu_t *ypu,
        int32_t n,
        int32_t num_channels,
        float temperature,
        int64_t random_state,
        pv_ypu_mem_t *y_ypu_mem);

#endif
