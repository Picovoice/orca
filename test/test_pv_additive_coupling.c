#include <stdlib.h>
#include <string.h>

#include "core/picovoice.h"
#include "core/pv_type.h"
#include "test/pv_test.h"

#include "test_data/test_pv_additive_coupling_data.c"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif


static const float TEST_TOLERANCE_PERCENT = 0.01f;
static const float TEST_EPSILON = 0.0052f;


static pv_additive_coupling_t *additive_coupling_object = NULL;
static pv_ypu_t *ypu = NULL;

static pv_status_t test_pv_additive_coupling_setup(void) {
    pv_status_t status = pv_ypu_init_cpu(1, &ypu);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_additive_coupling_init(
            ypu,
            &TEST_ADDITIVE_COUPLING_PARAM,
            &additive_coupling_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_additive_coupling_teardown(void) {
    pv_additive_coupling_delete(ypu, additive_coupling_object);
    additive_coupling_object = NULL;
}

static pv_status_t test_pv_additive_coupling_forward_inner(bool test_accuracy) {
    const int32_t N = TEST_ADDITIVE_COUPLING_SEQUENCE_LENGTH;
    const int32_t dimension = TEST_ADDITIVE_COUPLING_PARAM.transformer_param->attention_param->dimension;

    pv_ypu_mem_t *buffer_c_ypu = pv_ypu_mem_alloc(
            ypu,
            sizeof(TEST_ADDITIVE_COUPLING_INPUT_1),
            PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(buffer_c_ypu != NULL, "Failed to allocate buffer_c_ypu");
    if (buffer_c_ypu == NULL) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_mem_t *buffer_x_ypu = pv_ypu_mem_alloc(
            ypu,
            sizeof(TEST_ADDITIVE_COUPLING_INPUT_0),
            PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(buffer_x_ypu != NULL, "Failed to allocate buffer_x_ypu");
    if (buffer_x_ypu == NULL) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_mem_t *buffer_y_ypu = pv_ypu_mem_alloc(
            ypu,
            N * dimension * sizeof(float),
            PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(buffer_y_ypu != NULL, "Failed to allocate buffer_y_ypu");
    if (buffer_y_ypu == NULL) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_status_t status = pv_ypu_mem_copy_to(
            ypu,
            buffer_c_ypu,
            TEST_ADDITIVE_COUPLING_INPUT_1,
            0,
            sizeof(TEST_ADDITIVE_COUPLING_INPUT_1));
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "pv_ypu_mem_copy_to failed with %s",
            pv_status_to_string(status));

    status = pv_ypu_mem_copy_to(
            ypu,
            buffer_x_ypu,
            TEST_ADDITIVE_COUPLING_INPUT_0,
            0,
            sizeof(TEST_ADDITIVE_COUPLING_INPUT_0));
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "pv_ypu_mem_copy_to failed with %s",
            pv_status_to_string(status));

    status = pv_additive_coupling_forward(
            ypu,
            additive_coupling_object,
            N,
            buffer_c_ypu,
            buffer_x_ypu,
            buffer_y_ypu);

    pv_ypu_mem_free(ypu, buffer_x_ypu);
    pv_ypu_mem_free(ypu, buffer_c_ypu);

    if (test_accuracy) {
        float *buffer_y = pv_ypu_mem_get_host_view(ypu, buffer_y_ypu, true);

        pv_test_true(status == PV_STATUS_SUCCESS, "pv_additive_coupling_forward() returns failed status");

        pv_test_close_float_array(
                buffer_y,
                TEST_ADDITIVE_COUPLING_TARGET,
                N * dimension,
                TEST_TOLERANCE_PERCENT,
                TEST_EPSILON,
                "failed to match target");
    }

    pv_ypu_mem_free(ypu, buffer_y_ypu);

    return status;
}

static void test_pv_additive_coupling_forward(void) {
    test_pv_additive_coupling_forward_inner(true);
}

static const pv_test_case_t PV_ADDITIVE_COUPLING_TEST_CASES[] = {
        {"additive_coupling forward", test_pv_additive_coupling_forward},
};

const pv_test_suite_t PV_ADDITIVE_COUPLING_TEST_SUITE = {
        .name = "additive_coupling",
        .setup = test_pv_additive_coupling_setup,
        .teardown = test_pv_additive_coupling_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_ADDITIVE_COUPLING_TEST_CASES),
        .test_cases = PV_ADDITIVE_COUPLING_TEST_CASES,
};
