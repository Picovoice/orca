#include <stdlib.h>
#include <string.h>

#include "core/picovoice.h"
#include "core/pv_type.h"
#include "test/pv_test.h"

#include "test_data/test_pv_orca_duration_predictor_data.c"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif


static const float TEST_TOLERANCE_PERCENT_DURATION = 0.0f;
static const int32_t TEST_EPSILON_DURATION = 0;

#ifdef __ORCA_FLOAT_MODE__

static const float TEST_TOLERANCE_PERCENT_STD = 0.000002f;
static const float TEST_EPSILON_STD = 0.005f;

#else

static const float TEST_TOLERANCE_PERCENT_STD = 0.001f;
static const float TEST_EPSILON_STD = 0.005f;

#endif

static pv_ypu_t *ypu = NULL;
static pv_orca_duration_predictor_t *orca_duration_predictor_object = NULL;

static pv_status_t test_pv_orca_duration_predictor_setup(void) {
    pv_status_t status = pv_ypu_init_cpu(1, &ypu);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_orca_duration_predictor_init(
            ypu,
            &DURATION_PREDICTOR_PARAM,
            &orca_duration_predictor_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_orca_duration_predictor_teardown(void) {
    pv_orca_duration_predictor_delete(ypu, orca_duration_predictor_object);
    pv_ypu_delete(ypu);
}

static void test_pv_orca_duration_predictor_forward(void) {
    pv_ypu_mem_t *m0 = pv_ypu_mem_alloc(
            ypu,
            sizeof(TEST_ORCA_DURATION_PREDICTOR_INPUT),
            PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m0 != NULL, "Failed to allocate m0");
    if (m0 == NULL) {
        return;
    }

    pv_status_t status = pv_ypu_mem_copy_to(
            ypu,
            m0,
            TEST_ORCA_DURATION_PREDICTOR_INPUT,
            0,
            sizeof(TEST_ORCA_DURATION_PREDICTOR_INPUT));
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "pv_ypu_mem_copy_to failed with %s",
            pv_status_to_string(status));

    int32_t *buffer_duration = (int32_t *) calloc(
            TEST_ORCA_DURATION_PREDICTOR_SEQUENCE_LENGTH * 1,
            sizeof(int32_t));

    pv_ypu_mem_t *m1 = pv_ypu_mem_alloc(
            ypu,
            TEST_ORCA_DURATION_PREDICTOR_SEQUENCE_LENGTH * sizeof(int32_t),
            PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m1 != NULL, "Failed to allocate m1");
    if (m1 == NULL) {
        return;
    }

    status = pv_orca_duration_predictor_forward(
            ypu,
            orca_duration_predictor_object,
            TEST_ORCA_DURATION_PREDICTOR_SEQUENCE_LENGTH,
            1.0f,
            m0,
            buffer_duration,
            m1);

    pv_test_true(status == PV_STATUS_SUCCESS, "pv_orca_duration_predictor_forward() returns failed status");

    pv_test_close_int32_array(
            buffer_duration,
            TEST_ORCA_DURATION_PREDICTOR_TARGET_0,
            TEST_ORCA_DURATION_PREDICTOR_SEQUENCE_LENGTH,
            TEST_TOLERANCE_PERCENT_DURATION,
            TEST_EPSILON_DURATION,
            "failed to match target duration");

    free(buffer_duration);

    float *buffer_std = pv_ypu_mem_get_host_view(ypu, m1, true);
    pv_test_close_float_array(
            buffer_std,
            TEST_ORCA_DURATION_PREDICTOR_TARGET_1,
            TEST_ORCA_DURATION_PREDICTOR_SEQUENCE_LENGTH,
            TEST_TOLERANCE_PERCENT_STD,
            TEST_EPSILON_STD,
            "failed to match target std");
    pv_ypu_mem_release_host_view(ypu, m1, true);

    pv_ypu_mem_free(ypu, m0);
    pv_ypu_mem_free(ypu, m1);
}

static const pv_test_case_t PV_ORCA_DURATION_PREDICTOR_TEST_CASES[] = {
        {"orca_duration_predictor forward", test_pv_orca_duration_predictor_forward},
};

const pv_test_suite_t PV_ORCA_DURATION_PREDICTOR_TEST_SUITE = {
        .name = "orca_duration_predictor",
        .setup = test_pv_orca_duration_predictor_setup,
        .teardown = test_pv_orca_duration_predictor_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_ORCA_DURATION_PREDICTOR_TEST_CASES),
        .test_cases = PV_ORCA_DURATION_PREDICTOR_TEST_CASES,
};
