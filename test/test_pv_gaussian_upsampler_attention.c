#include <stdlib.h>

#include "core/picovoice.h"
#include "core/pv_type.h"
#include "test/pv_test.h"

#include "test_data/test_pv_gaussian_upsampler_attention_data.c"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

static const float TEST_TOLERANCE_PERCENT = 0.00002f;
static const float TEST_EPSILON = 0.005f;

static pv_ypu_t *ypu = NULL;

static pv_status_t test_pv_gaussian_upsampler_setup(void) {
    pv_status_t status = pv_ypu_init_cpu(1, &ypu);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_gaussian_upsampler_teardown(void) {
    pv_ypu_delete(ypu);
}


static void test_pv_gaussian_upsampler_attention_forward(void) {
    pv_orca_gaussian_upsampler_t *object = calloc(
            1,
            sizeof(pv_orca_gaussian_upsampler_t));

    pv_orca_gaussian_upsampler_param_t *param = calloc(
            1,
            sizeof(pv_orca_gaussian_upsampler_param_t));

    param->dimension = 96;
    param->num_lookaheads_gaussian_upsampling = 1;
    param->num_lookbacks_gaussian_upsampling = 5;

    object->param = param;

    const int32_t dimension = object->param->dimension;

    const int32_t N = TEST_GAUSSIAN_UPSAMPLER_ATTENTION_SEQUENCE_LENGTH_0;
    const int32_t T = TEST_GAUSSIAN_UPSAMPLER_ATTENTION_SEQUENCE_LENGTH_1;

    pv_ypu_mem_t *m1 = pv_ypu_mem_alloc(
            ypu,
            sizeof(TEST_GAUSSIAN_UPSAMPLER_ATTENTION_INPUT_1),
            PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m1 != NULL, "Failed to allocate m1");
    if (m1 == NULL) {
        return;
    }

    pv_status_t status = pv_ypu_mem_copy_to(
            ypu,
            m1,
            TEST_GAUSSIAN_UPSAMPLER_ATTENTION_INPUT_1,
            0,
            sizeof(TEST_GAUSSIAN_UPSAMPLER_ATTENTION_INPUT_1));
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "pv_ypu_mem_copy_to failed with %s",
            pv_status_to_string(status));

    pv_ypu_mem_t *m2 = pv_ypu_mem_alloc(
            ypu,
            sizeof(TEST_GAUSSIAN_UPSAMPLER_ATTENTION_INPUT_2),
            PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m2 != NULL, "Failed to allocate m2");
    if (m2 == NULL) {
        return;
    }

    status = pv_ypu_mem_copy_to(
            ypu,
            m2,
            TEST_GAUSSIAN_UPSAMPLER_ATTENTION_INPUT_2,
            0,
            sizeof(TEST_GAUSSIAN_UPSAMPLER_ATTENTION_INPUT_2));
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "pv_ypu_mem_copy_to failed with %s",
            pv_status_to_string(status));

    pv_ypu_mem_t *m3 = pv_ypu_mem_alloc(
            ypu,
            sizeof(TEST_GAUSSIAN_UPSAMPLER_ATTENTION_INPUT_3),
            PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m3 != NULL, "Failed to allocate m3");
    if (m3 == NULL) {
        return;
    }

    status = pv_ypu_mem_copy_to(
            ypu,
            m3,
            TEST_GAUSSIAN_UPSAMPLER_ATTENTION_INPUT_3,
            0,
            sizeof(TEST_GAUSSIAN_UPSAMPLER_ATTENTION_INPUT_3));
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "pv_ypu_mem_copy_to failed with %s",
            pv_status_to_string(status));

    pv_ypu_mem_t *buffer_ypu = pv_ypu_mem_alloc(
            ypu,
            T * dimension * sizeof(float),
            PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(buffer_ypu != NULL, "Failed to allocate buffer");
    if (buffer_ypu == NULL) {
        return;
    }

    status = pv_orca_gaussian_upsampler_attention(
            ypu,
            object,
            N,
            T,
            TEST_GAUSSIAN_UPSAMPLER_ATTENTION_INPUT_0,
            m1,
            m2,
            m3,
            buffer_ypu);
    pv_ypu_mem_free(ypu, m1);
    pv_ypu_mem_free(ypu, m2);
    pv_ypu_mem_free(ypu, m3);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            " pv_orca_gaussian_upsampler_attention return failed status");
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    float *buffer = pv_ypu_mem_get_host_view(ypu, buffer_ypu, true);
    pv_test_close_float_array(
            buffer,
            TEST_GAUSSIAN_UPSAMPLER_ATTENTION_TARGET,
            T * dimension,
            TEST_TOLERANCE_PERCENT,
            TEST_EPSILON,
            "failed to forward gaussian upsampler attention");
    pv_ypu_mem_release_host_view(ypu, buffer_ypu, false);
    pv_ypu_mem_free(ypu, buffer_ypu);
    free(object->param);
    free(object);
}


static const pv_test_case_t PV_GAUSSIAN_UPSAMPLER_ATTENTION_TEST_CASES[] = {
        {"gaussian upsampler attention forward", test_pv_gaussian_upsampler_attention_forward},
};


const pv_test_suite_t PV_GAUSSIAN_UPSAMPLER_TEST_SUITE = {
        .name = "gaussian_upsampler_attention",
        .setup = test_pv_gaussian_upsampler_setup,
        .teardown = test_pv_gaussian_upsampler_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_GAUSSIAN_UPSAMPLER_ATTENTION_TEST_CASES),
        .test_cases = PV_GAUSSIAN_UPSAMPLER_ATTENTION_TEST_CASES,
};
