#include <stdlib.h>

#include "core/picovoice.h"
#include "core/pv_type.h"
#include "test/pv_test.h"

#include "test_data/test_pv_orca_flow_data.c"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

static pv_ypu_t *ypu = NULL;
static pv_orca_flow_t *orca_flow_object = NULL;

static pv_status_t test_pv_orca_flow_setup(void) {
    pv_status_t status = pv_ypu_init_cpu(1, &ypu);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_orca_flow_init(ypu, &FLOW_PARAM, &orca_flow_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_orca_flow_teardown(void) {
    pv_orca_flow_delete(ypu, orca_flow_object);
    pv_ypu_delete(ypu);
}

static void test_pv_orca_flow_forward(void) {
    pv_test_true(orca_flow_object != NULL, "failed to create tmp file");

    int32_t num_channels = PV_ARRAY_LEN(TEST_ORCA_FLOW_TARGET) / TEST_ORCA_FLOW_SEQUENCE_LENGTH;

    pv_ypu_mem_t *m0 = pv_ypu_mem_alloc(
        ypu,
        sizeof(TEST_ORCA_FLOW_INPUT),
        PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m0 != NULL, "Failed to allocate m0");
    if (m0 == NULL) {
        return;
    }

    pv_ypu_mem_t *m1 = pv_ypu_mem_alloc(
        ypu,
        TEST_ORCA_FLOW_SEQUENCE_LENGTH * num_channels * sizeof(float),
        PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m1 != NULL, "Failed to allocate m1");
    if (m1 == NULL) {
        return;
    }

    pv_status_t status = pv_ypu_mem_copy_to(
        ypu,
        m0,
        TEST_ORCA_FLOW_INPUT,
        0,
        sizeof(TEST_ORCA_FLOW_INPUT));
    pv_test_true(
        status == PV_STATUS_SUCCESS,
        "pv_ypu_mem_copy_to failed with %s",
        pv_status_to_string(status));

    status = pv_orca_flow_forward(
            ypu,
            orca_flow_object,
            TEST_ORCA_FLOW_SEQUENCE_LENGTH,
            m0, m1, 0, 0);
    pv_test_true(
        status == PV_STATUS_SUCCESS,
        "pv_orca_flow_forward failed with %s",
        pv_status_to_string(status));

    float *buffer = pv_ypu_mem_get_host_view(ypu, m1, true);
    pv_test_close_float_array(
            buffer,
            TEST_ORCA_FLOW_TARGET,
            TEST_ORCA_FLOW_SEQUENCE_LENGTH * num_channels,
            0.05f,
            0.01f,
            "failed to forward orca_flow");
    pv_ypu_mem_release_host_view(ypu, m1, true);

    pv_ypu_mem_free(ypu, m0);
    pv_ypu_mem_free(ypu, m1);
}

#ifdef __PV_MOCKS__

static void *pv_ypu_host_alloc_return_null(pv_ypu_t *ypu, int32_t size_bytes) {
    (void) ypu;
    (void) size_bytes;
    return NULL;
}

static void test_pv_orca_flow_init_failure_helper(pv_status_t expected) {
    pv_orca_flow_t *object = NULL;
    pv_status_t status = pv_orca_flow_init(ypu, &FLOW_PARAM, &object);
    pv_test_true(
            status == expected,
            "orca flow init error, got `%s` expected `%s`",
            pv_status_to_string(status),
            pv_status_to_string(expected));
    if (object) {
        pv_orca_flow_delete(ypu, object);
    }
}

static void test_pv_orca_flow_init_failure_1(void) {
    PV_SET_MOCK_RETURN_VAL(pv_ypu_host_alloc, NULL)

    test_pv_orca_flow_init_failure_helper(PV_STATUS_OUT_OF_MEMORY);
}

static void test_pv_orca_flow_init_failure_2(void) {
    void *(*custom_funcs[])(pv_ypu_t *, int32_t) = {
        pv_ypu_host_alloc_real,
        pv_ypu_host_alloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(pv_ypu_host_alloc, custom_funcs)

    test_pv_orca_flow_init_failure_helper(PV_STATUS_OUT_OF_MEMORY);
}

static void test_pv_orca_flow_init_failure_3(void) {
    PV_SET_MOCK_RETURN_VAL(pv_residual_coupling_init, PV_STATUS_INVALID_ARGUMENT)

    test_pv_orca_flow_init_failure_helper(PV_STATUS_INVALID_ARGUMENT);
}

#endif

static const pv_test_case_t PV_ORCA_FLOW_TEST_CASES[] = {
        {"orca_flow forward", test_pv_orca_flow_forward},

#ifdef __PV_MOCKS__

        {"orca_flow init failure 1", test_pv_orca_flow_init_failure_1},
        {"orca_flow init failure 2", test_pv_orca_flow_init_failure_2},
        {"orca_flow init failure 3", test_pv_orca_flow_init_failure_3},

#endif

};

const pv_test_suite_t PV_ORCA_FLOW_TEST_SUITE = {
        .name = "orca_flow",
        .setup = test_pv_orca_flow_setup,
        .teardown = test_pv_orca_flow_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_ORCA_FLOW_TEST_CASES),
        .test_cases = PV_ORCA_FLOW_TEST_CASES,
};
