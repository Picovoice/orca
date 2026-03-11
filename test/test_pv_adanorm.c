#include <stdlib.h>

#include "core/picovoice.h"
#include "core/pv_type.h"
#include "test/pv_test.h"

#include "test_data/test_pv_adanorm_data.c"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

static const float TEST_TOLERANCE_PERCENT = 0.02f;
static const float TEST_EPSILON = 0.0065f;

static pv_adanorm_t *adanorm_object = NULL;
static pv_ypu_t *ypu = NULL;

static pv_status_t test_pv_adanorm_setup(void) {
    pv_status_t status = pv_ypu_init_cpu(1, &ypu);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_adanorm_init(ypu, &(TEST_ADANORM_PARAM), &adanorm_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_adanorm_teardown(void) {
    pv_adanorm_delete(ypu, adanorm_object);
    pv_ypu_delete(ypu);
}

static pv_status_t test_pv_adanorm_forward_inner(bool test_accuracy) {
    int32_t num_channels = TEST_ADANORM_PARAM.linear_param->input_channels;

    pv_ypu_mem_t *buffer_x_ypu = pv_ypu_mem_alloc(
            ypu,
            sizeof(TEST_ADANORM_INPUT_0),
            PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(buffer_x_ypu != NULL, "Failed to allocate buffer_x_ypu");
    if (buffer_x_ypu == NULL) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_mem_t *buffer_c_ypu = pv_ypu_mem_alloc(
            ypu,
            sizeof(TEST_ADANORM_INPUT_1),
            PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(buffer_c_ypu != NULL, "Failed to allocate buffer_c_ypu");
    if (buffer_c_ypu == NULL) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_mem_t *buffer_gates_list_ypu = pv_ypu_mem_alloc(
            ypu,
            TEST_ADANORM_SEQUENCE_LENGTH * 6 * num_channels * sizeof(float),
            PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(buffer_gates_list_ypu != NULL, "Failed to allocate buffer_gates_list");
    if (buffer_gates_list_ypu == NULL) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_mem_t *buffer_y_ypu = pv_ypu_mem_alloc(
            ypu,
            TEST_ADANORM_SEQUENCE_LENGTH * num_channels * sizeof(float),
            PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(buffer_y_ypu != NULL, "Failed to allocate buffer_y");
    if (buffer_y_ypu == NULL) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_status_t status = pv_ypu_mem_copy_to(
            ypu,
            buffer_x_ypu,
            TEST_ADANORM_INPUT_0,
            0,
            sizeof(TEST_ADANORM_INPUT_0));
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "pv_ypu_mem_copy_to failed with %s",
            pv_status_to_string(status));

    status = pv_ypu_mem_copy_to(
            ypu,
            buffer_c_ypu,
            TEST_ADANORM_INPUT_1,
            0,
            sizeof(TEST_ADANORM_INPUT_1));
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "pv_ypu_mem_copy_to failed with %s",
            pv_status_to_string(status));

    status = pv_adanorm_rope_transformer_forward(
            ypu,
            adanorm_object,
            TEST_ADANORM_SEQUENCE_LENGTH,
            buffer_x_ypu,
            buffer_c_ypu,
            buffer_gates_list_ypu,
            buffer_y_ypu);

    pv_ypu_mem_free(ypu, buffer_x_ypu);
    pv_ypu_mem_free(ypu, buffer_c_ypu);

    if (test_accuracy) {
        float *buffer_y = pv_ypu_mem_get_host_view(ypu, buffer_y_ypu, true);
        float *buffer_gates_list = pv_ypu_mem_get_host_view(ypu, buffer_gates_list_ypu, true);
        pv_test_close_float_array(
                buffer_y,
                TEST_ADANORM_TARGET_0,
                TEST_ADANORM_SEQUENCE_LENGTH * num_channels,
                TEST_TOLERANCE_PERCENT,
                TEST_EPSILON,
                "failed to forward adanorm");
        pv_test_close_float_array(
                buffer_gates_list + 2 * TEST_ADANORM_SEQUENCE_LENGTH * num_channels,
                TEST_ADANORM_TARGET_1,
                TEST_ADANORM_SEQUENCE_LENGTH * num_channels,
                TEST_TOLERANCE_PERCENT,
                TEST_EPSILON,
                "failed to forward adanorm");
        pv_test_close_float_array(
                buffer_gates_list + 3 * TEST_ADANORM_SEQUENCE_LENGTH * num_channels,
                TEST_ADANORM_TARGET_2,
                TEST_ADANORM_SEQUENCE_LENGTH * num_channels,
                TEST_TOLERANCE_PERCENT,
                TEST_EPSILON,
                "failed to forward adanorm");
        pv_test_close_float_array(
                buffer_gates_list + 4 * TEST_ADANORM_SEQUENCE_LENGTH * num_channels,
                TEST_ADANORM_TARGET_3,
                TEST_ADANORM_SEQUENCE_LENGTH * num_channels,
                TEST_TOLERANCE_PERCENT,
                TEST_EPSILON,
                "failed to forward adanorm");
        pv_test_close_float_array(
                buffer_gates_list + 5 * TEST_ADANORM_SEQUENCE_LENGTH * num_channels,
                TEST_ADANORM_TARGET_4,
                TEST_ADANORM_SEQUENCE_LENGTH * num_channels,
                TEST_TOLERANCE_PERCENT,
                TEST_EPSILON,
                "failed to forward adanorm");
        pv_ypu_mem_release_host_view(ypu, buffer_gates_list_ypu, true);
        pv_ypu_mem_release_host_view(ypu, buffer_y_ypu, true);
    }

    pv_ypu_mem_free(ypu, buffer_gates_list_ypu);
    pv_ypu_mem_free(ypu, buffer_y_ypu);

    return status;
}

static void test_pv_adanorm_forward(void) {
    test_pv_adanorm_forward_inner(true);
}

static const pv_test_case_t PV_ADANORM_TEST_CASES[] = {
        {"adanorm forward", test_pv_adanorm_forward},
};

const pv_test_suite_t PV_ADANORM_TEST_SUITE = {
        .name = "adanorm",
        .setup = test_pv_adanorm_setup,
        .teardown = test_pv_adanorm_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_ADANORM_TEST_CASES),
        .test_cases = PV_ADANORM_TEST_CASES,
};
