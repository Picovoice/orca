#include <stdlib.h>

#include "core/picovoice.h"
#include "core/pv_type.h"
#include "test/pv_test.h"

#include "test_data/test_pv_residual_coupling_data.c"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

static pv_ypu_t *ypu = NULL;
static pv_residual_coupling_t *residual_coupling_object = NULL;

static pv_status_t test_pv_residual_coupling_setup(void) {
    pv_status_t status = pv_ypu_init_cpu(1, &ypu);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_residual_coupling_init(
            ypu,
            &TEST_RESIDUAL_COUPLING_PARAM,
            &residual_coupling_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_residual_coupling_teardown(void) {
    pv_residual_coupling_delete(ypu, residual_coupling_object);
    pv_ypu_delete(ypu);
}

static void test_pv_residual_coupling_forward(void) {
    pv_test_true(residual_coupling_object != NULL, "failed to create tmp file");

    int32_t num_channels = pv_residual_coupling_output_channels(residual_coupling_object);

    pv_ypu_mem_t *m0 = pv_ypu_mem_alloc(
        ypu,
        sizeof(TEST_RESIDUAL_COUPLING_INPUT),
        PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m0 != NULL, "Failed to allocate m0");
    if (m0 == NULL) {
        return;
    }

    pv_ypu_mem_t *m1 = pv_ypu_mem_alloc(
        ypu,
        TEST_RESIDUAL_COUPLING_SEQUENCE_LENGTH * num_channels * sizeof(float),
        PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m1 != NULL, "Failed to allocate m1");
    if (m1 == NULL) {
        return;
    }

    pv_status_t status = pv_ypu_mem_copy_to(
        ypu,
        m0,
        TEST_RESIDUAL_COUPLING_INPUT,
        0,
        sizeof(TEST_RESIDUAL_COUPLING_INPUT));
    pv_test_true(
        status == PV_STATUS_SUCCESS,
        "pv_ypu_mem_copy_to failed with %s",
        pv_status_to_string(status));


    status = pv_residual_coupling_forward(
        ypu,
        residual_coupling_object,
        TEST_RESIDUAL_COUPLING_SEQUENCE_LENGTH,
        m0,
        m1,
        0,
        0);
    pv_test_true(
        status == PV_STATUS_SUCCESS,
        "pv_residual_coupling_forward failed with %s",
        pv_status_to_string(status));

    float *buffer = pv_ypu_mem_get_host_view(ypu, m1, true);
    pv_test_close_float_array(
            buffer,
            TEST_RESIDUAL_COUPLING_TARGET,
            TEST_RESIDUAL_COUPLING_SEQUENCE_LENGTH * num_channels,
            0.05f,
            0.01f,
            "failed to forward residual_coupling");
    pv_ypu_mem_release_host_view(ypu, m1, true);

    pv_ypu_mem_free(ypu, m0);
    pv_ypu_mem_free(ypu, m1);
}

static const pv_test_case_t PV_RESIDUAL_COUPLING_TEST_CASES[] = {
        {"residual_coupling forward", test_pv_residual_coupling_forward},
};

const pv_test_suite_t PV_RESIDUAL_COUPLING_TEST_SUITE = {
        .name = "residual_coupling",
        .setup = test_pv_residual_coupling_setup,
        .teardown = test_pv_residual_coupling_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_RESIDUAL_COUPLING_TEST_CASES),
        .test_cases = PV_RESIDUAL_COUPLING_TEST_CASES,
};
