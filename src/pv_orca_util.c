#include <math.h>
#include <string.h>

#include "core/pv_error_messages.h"
#include "math/pv_math.h"
#include "orca/pv_orca_util.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

#define EULER_NUMBER_F (2.71828182846)

void PV_MOCKABLE(pv_orca_util_generate_bucket)(
        pv_ypu_t *ypu,
        int32_t n,
        int32_t bucket_offset,
        const int32_t *durations,
        pv_ypu_mem_t *bucket_ypu) {
    PV_ASSERT(ypu);
    PV_ASSERT(n > 0);
    PV_ASSERT(bucket_offset >= 0);
    PV_ASSERT(durations);
    PV_ASSERT(bucket_ypu);

    int32_t *bucket = pv_ypu_mem_get_host_view(ypu, bucket_ypu, false);

    int32_t i_bucket = 0;
    for (int32_t i = 0; i < n; i++) {
        for (int32_t j = 0; j < durations[i]; j++) {
            bucket[i_bucket++] = i + bucket_offset;
        }
    }

    pv_ypu_mem_release_host_view(ypu, bucket_ypu, true);
}

pv_status_t PV_MOCKABLE(pv_orca_util_sample_standard_gaussian_with_temperature)(
        pv_ypu_t *ypu,
        int32_t n,
        int32_t num_channels,
        float temperature,
        int64_t random_state,
        pv_ypu_mem_t *y_ypu_mem) {
    PV_ASSERT(n);
    PV_ASSERT(num_channels);
    PV_ASSERT(temperature >= 0);
    PV_ASSERT(y_ypu_mem);

    if (temperature == 0) {
        pv_ypu_op_memset_args_t memset_args = {
            .output = y_ypu_mem,
            .size_bytes = n * num_channels * ((int32_t) sizeof(float)),
            .output_offset = 0,
        };
        return pv_ypu_operator_execute(
                ypu,
                PV_YPU_OPERATOR_MEMSET,
                &memset_args);
    }

    uint64_t *state = (uint64_t *) &random_state;

    pv_orca_util_rand_normal_t *rand_normal = NULL;
    pv_status_t status = pv_orca_util_rand_normal_init(&(rand_normal));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_module_function_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("pv_orca_util_rand_normal_init"));
        return status;
    }

    float *y = pv_ypu_mem_get_host_view(ypu, y_ypu_mem, false);

    for (int32_t i = 0; i < n; i++) {
        const int32_t frame_offset = i * num_channels;

        for (int32_t j = 0; j < num_channels; j++) {
            y[frame_offset + j] = pv_orca_util_rand_normal_sample(rand_normal, state) * temperature;
        }
    }

    pv_ypu_mem_release_host_view(ypu, y_ypu_mem, true);

    pv_orca_util_rand_normal_delete(rand_normal);

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_orca_util_rand_normal_init)(pv_orca_util_rand_normal_t **object) {
    PV_ASSERT(object);

    *object = NULL;

    pv_orca_util_rand_normal_t *o = calloc(1, sizeof(pv_orca_util_rand_normal_t));
    if (!o) {
        return PV_STATUS_OUT_OF_MEMORY;
    }
    o->has_value = false;

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_orca_util_rand_normal_delete)(pv_orca_util_rand_normal_t *object) {
    if (object) {
        free(object);
    }
}

float PV_MOCKABLE(pv_orca_util_rand_normal_sample)(pv_orca_util_rand_normal_t *object, uint64_t *state) {
    PV_ASSERT(object);
    PV_ASSERT(state);

    if (object->has_value) {
        object->has_value = false;
        return object->value;
    }

    float ru1 = 0.0f;
    float ru2 = 0.0f;
    float rr = 0.0f;
    while (rr >= 1.0 || rr == 0.0) {
        ru1 =  (2.0f * (float) pv_rand_uniform(state)) - 1.0f;
        ru2 =  (2.0f * (float) pv_rand_uniform(state)) - 1.0f;
        rr = ru1 * ru1 + ru2 * ru2;
    }

    float f = sqrtf(-2.0f * (logf(rr) / rr));
    float rand_normal1 = ru1 * f;
    float rand_normal2 = ru2 * f;

    object->value = rand_normal2;
    object->has_value = true;

    return rand_normal1;
}
