#include <stdlib.h>
#include <string.h>

#include "core/picovoice.h"
#include "core/pv_type.h"
#include "test/pv_test.h"

#include "test_data/test_pv_orca_gaussian_upsampler_data.c"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

#ifdef __ORCA_FLOAT_MODE__

static const float TEST_TOLERANCE_PERCENT = 0.0005f;
static const float TEST_EPSILON = 0.005f;

#else

static const float TEST_TOLERANCE_PERCENT = 0.05f;
static const float TEST_EPSILON = 0.03f;

#endif

static pv_ypu_t *ypu = NULL;
static pv_orca_gaussian_upsampler_t *orca_gaussian_upsampler_object = NULL;

static pv_status_t test_pv_orca_gaussian_upsampler_setup(void) {
    pv_status_t status = pv_ypu_init_cpu(1, &ypu);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_orca_gaussian_upsampler_init(
            ypu,
            &GAUSSIAN_UPSAMPLER_PARAM,
            &orca_gaussian_upsampler_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_orca_gaussian_upsampler_teardown(void) {
    pv_orca_gaussian_upsampler_delete(ypu, orca_gaussian_upsampler_object);
    pv_ypu_delete(ypu);
}

static void test_pv_orca_gaussian_upsampler_forward(void) {
    const int32_t dimension = GAUSSIAN_UPSAMPLER_PARAM.dimension;

    pv_ypu_mem_t *m0 = pv_ypu_mem_alloc(
            ypu,
            sizeof(TEST_ORCA_GAUSSIAN_UPSAMPLER_INPUT_0),
            PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m0 != NULL, "Failed to allocate m0");
    if (m0 == NULL) {
        return;
    }

    pv_status_t status = pv_ypu_mem_copy_to(
            ypu,
            m0,
            TEST_ORCA_GAUSSIAN_UPSAMPLER_INPUT_0,
            0,
            sizeof(TEST_ORCA_GAUSSIAN_UPSAMPLER_INPUT_0));
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "pv_ypu_mem_copy_to failed with %s",
            pv_status_to_string(status));

    pv_ypu_mem_t *m1 = pv_ypu_mem_alloc(
            ypu,
            sizeof(TEST_ORCA_GAUSSIAN_UPSAMPLER_INPUT_2),
            PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m1 != NULL, "Failed to allocate m1");
    if (m1 == NULL) {
        return;
    }

    status = pv_ypu_mem_copy_to(
            ypu,
            m1,
            TEST_ORCA_GAUSSIAN_UPSAMPLER_INPUT_2,
            0,
            sizeof(TEST_ORCA_GAUSSIAN_UPSAMPLER_INPUT_2));
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "pv_ypu_mem_copy_to failed with %s",
            pv_status_to_string(status));

    pv_ypu_mem_t *m2 = pv_ypu_mem_alloc(
            ypu,
            TEST_ORCA_GAUSSIAN_UPSAMPLER_SEQUENCE_LENGTH_1 * dimension * sizeof(float),
            PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m2 != NULL, "Failed to allocate m2");
    if (m2 == NULL) {
        return;
    }

    status = pv_orca_gaussian_upsampler_forward(
            ypu,
            orca_gaussian_upsampler_object,
            TEST_ORCA_GAUSSIAN_UPSAMPLER_SEQUENCE_LENGTH_0,
            TEST_ORCA_GAUSSIAN_UPSAMPLER_SEQUENCE_LENGTH_1,
            m0,
            TEST_ORCA_GAUSSIAN_UPSAMPLER_INPUT_1,
            m1,
            m2);

    pv_test_true(status == PV_STATUS_SUCCESS, "pv_orca_gaussian_upsampler_forward() returns failed status");

    float *buffer_output = pv_ypu_mem_get_host_view(ypu, m2, true);
    pv_test_close_float_array(
            buffer_output,
            TEST_ORCA_GAUSSIAN_UPSAMPLER_TARGET,
            TEST_ORCA_GAUSSIAN_UPSAMPLER_SEQUENCE_LENGTH_1 * dimension,
            TEST_TOLERANCE_PERCENT,
            TEST_EPSILON,
            "failed to match target");
    pv_ypu_mem_release_host_view(ypu, m2, true);

    pv_ypu_mem_free(ypu, m0);
    pv_ypu_mem_free(ypu, m1);
    pv_ypu_mem_free(ypu, m2);
}

static const pv_test_case_t PV_ORCA_GAUSSIAN_UPSAMPLER_TEST_CASES[] = {
        {"orca_gaussian_upsampler forward", test_pv_orca_gaussian_upsampler_forward},
};

const pv_test_suite_t PV_ORCA_GAUSSIAN_UPSAMPLER_TEST_SUITE = {
        .name = "orca_gaussian_upsampler",
        .setup = test_pv_orca_gaussian_upsampler_setup,
        .teardown = test_pv_orca_gaussian_upsampler_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_ORCA_GAUSSIAN_UPSAMPLER_TEST_CASES),
        .test_cases = PV_ORCA_GAUSSIAN_UPSAMPLER_TEST_CASES,
};
