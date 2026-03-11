#include <stdlib.h>

#include "core/picovoice.h"
#include "core/pv_type.h"
#include "orca/pv_convnext_transposed.h"
#include "test/pv_test.h"

#include "test_data/test_pv_convnext_data.c"
#include "test_data/test_pv_convnext_transposed_data.c"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

static pv_ypu_t *ypu = NULL;
static pv_convnext_t *convnext_object = NULL;

static pv_status_t test_pv_convnext_setup(void) {
    pv_status_t status = pv_ypu_init_cpu(1, &ypu);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_convnext_init(
            ypu,
            &TEST_CONVNEXT_PARAM,
            &convnext_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_convnext_teardown(void) {
    pv_convnext_delete(ypu, convnext_object);
    pv_ypu_delete(ypu);
}

static void test_pv_convnext_forward(void) {
    int32_t num_channels = PV_ARRAY_LEN(TEST_CONVNEXT_TARGET) / TEST_CONVNEXT_SEQUENCE_LENGTH;

    pv_ypu_mem_t *m0 = pv_ypu_mem_alloc(
            ypu,
            sizeof(TEST_CONVNEXT_INPUT),
            PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m0 != NULL, "Failed to allocate m0");
    if (m0 == NULL) {
        return;
    }

    pv_status_t status = pv_ypu_mem_copy_to(
            ypu,
            m0,
            TEST_CONVNEXT_INPUT,
            0,
            sizeof(TEST_CONVNEXT_INPUT));
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "pv_ypu_mem_copy_to failed with %s",
            pv_status_to_string(status));

    status = pv_convnext_forward(
            ypu,
            convnext_object,
            TEST_CONVNEXT_SEQUENCE_LENGTH,
            m0);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "pv_convnext_forward failed with %s",
            pv_status_to_string(status));

    float *buffer = pv_ypu_mem_get_host_view(ypu, m0, true);
    pv_test_close_float_array(
            buffer,
            TEST_CONVNEXT_TARGET,
            TEST_CONVNEXT_SEQUENCE_LENGTH * num_channels,
            0.08f,
            0.04f,
            "failed to forward convnext");
    pv_ypu_mem_release_host_view(ypu, m0, true);

    pv_ypu_mem_free(ypu, m0);
}

static void test_pv_convnext_transposed_forward(void) {
    pv_convnext_transposed_t *convnext_transposed_object = NULL;
    pv_status_t status = pv_convnext_transposed_init(
            ypu,
            &TEST_CONVNEXT_TRANSPOSED_PARAM,
            &convnext_transposed_object);
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    int32_t num_channels = pv_convnext_transposed_output_channels(convnext_transposed_object);

    int32_t num_output_frames = pv_convnext_transposed_num_output_frames(
            convnext_transposed_object,
            TEST_CONVNEXT_TRANSPOSED_SEQUENCE_LENGTH);

    pv_ypu_mem_t *m0 = pv_ypu_mem_alloc(
            ypu,
            sizeof(TEST_CONVNEXT_TRANSPOSED_INPUT),
            PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m0 != NULL, "Failed to allocate m0");
    if (m0 == NULL) {
        return;
    }

    pv_ypu_mem_t *m1 = pv_ypu_mem_alloc(
            ypu,
            num_output_frames * num_channels * sizeof(float),
            PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m1 != NULL, "Failed to allocate m1");
    if (m1 == NULL) {
        return;
    }

    status = pv_ypu_mem_copy_to(
            ypu,
            m0,
            TEST_CONVNEXT_TRANSPOSED_INPUT,
            0,
            sizeof(TEST_CONVNEXT_TRANSPOSED_INPUT));
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "pv_ypu_mem_copy_to failed with %s",
            pv_status_to_string(status));

    status = pv_convnext_transposed_forward(
            ypu,
            convnext_transposed_object,
            TEST_CONVNEXT_TRANSPOSED_SEQUENCE_LENGTH,
            m0,
            m1);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "pv_ypu_mem_copy_to failed with %s",
            pv_status_to_string(status));

    float *buffer = pv_ypu_mem_get_host_view(ypu, m1, true);
    pv_test_close_float_array(
            buffer,
            TEST_CONVNEXT_TRANSPOSED_TARGET,
            num_output_frames * num_channels,
            0.05f,
            0.02f,
            "failed to forward convnext transposed");
    pv_ypu_mem_release_host_view(ypu, m1, true);

    pv_ypu_mem_free(ypu, m0);
    pv_ypu_mem_free(ypu, m1);

    pv_convnext_transposed_delete(ypu, convnext_transposed_object);
}

static const pv_test_case_t PV_CONVNEXT_TEST_CASES[] = {
        {"convnext forward", test_pv_convnext_forward},
        {"convnext transposed forward", test_pv_convnext_transposed_forward},
};

const pv_test_suite_t PV_CONVNEXT_TEST_SUITE = {
        .name = "convnext",
        .setup = test_pv_convnext_setup,
        .teardown = test_pv_convnext_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_CONVNEXT_TEST_CASES),
        .test_cases = PV_CONVNEXT_TEST_CASES,
};
