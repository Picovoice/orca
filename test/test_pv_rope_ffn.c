#include <stdlib.h>
#include <string.h>

#include "core/picovoice.h"
#include "core/pv_type.h"
#include "test/pv_test.h"

#include "test_data/test_pv_rope_ffn_data.c"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

#ifdef __ORCA_FLOAT_MODE__

static const float TEST_TOLERANCE_PERCENT = 0.001f;
static const float TEST_EPSILON = 0.005f;

#else

static const float TEST_TOLERANCE_PERCENT = 0.03f;
static const float TEST_EPSILON = 0.03f;

#endif

static pv_ypu_t *ypu = NULL;
static pv_rope_transformer_ffn_t *rope_ffn_object = NULL;

static pv_status_t test_pv_rope_ffn_setup(void) {
    pv_status_t status = pv_ypu_init_cpu(1, &ypu);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_rope_transformer_ffn_init(
            ypu,
            &TEST_ROPE_FFN_PARAM,
            &rope_ffn_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_rope_ffn_teardown(void) {
    pv_rope_transformer_ffn_delete(ypu, rope_ffn_object);
    pv_ypu_delete(ypu);
}

static void test_pv_rope_ffn_forward(void) {
    const int32_t N = TEST_ROPE_FFN_SEQUENCE_LENGTH;
    const int32_t dimension = TEST_ROPE_FFN_PARAM.conv_1_param->input_channels;

    pv_ypu_mem_t *m0 = pv_ypu_mem_alloc(
            ypu,
            sizeof(TEST_ROPE_FFN_INPUT),
            PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m0 != NULL, "Failed to allocate m0");
    if (m0 == NULL) {
        return;
    }

    pv_status_t status = pv_ypu_mem_copy_to(
            ypu,
            m0,
            TEST_ROPE_FFN_INPUT,
            0,
            sizeof(TEST_ROPE_FFN_INPUT));
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "pv_ypu_mem_copy_to failed with %s",
            pv_status_to_string(status));

    pv_ypu_mem_t *m1 = pv_ypu_mem_alloc(
            ypu,
            N * dimension * sizeof(float),
            PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m1 != NULL, "Failed to allocate m1");
    if (m1 == NULL) {
        return;
    }

    status = pv_rope_transformer_ffn_forward(
            ypu,
            rope_ffn_object,
            N,
            m0,
            m1);
    pv_ypu_mem_free(ypu, m0);
    pv_test_true(status == PV_STATUS_SUCCESS, "pv_rope_transformer_ffn_forward() returns failed status");

    float *buffer_output = pv_ypu_mem_get_host_view(ypu, m1, true);
    pv_test_close_float_array(
            buffer_output,
            TEST_ROPE_FFN_TARGET,
            N * dimension,
            TEST_TOLERANCE_PERCENT,
            TEST_EPSILON,
            "failed to match target");
    pv_ypu_mem_release_host_view(ypu, m1, false);

    pv_ypu_mem_free(ypu, m1);
}

static const pv_test_case_t PV_ROPE_FFN_TEST_CASES[] = {
        {"rope_ffn forward", test_pv_rope_ffn_forward},
};

const pv_test_suite_t PV_ROPE_FFN_TEST_SUITE = {
        .name = "rope_ffn",
        .setup = test_pv_rope_ffn_setup,
        .teardown = test_pv_rope_ffn_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_ROPE_FFN_TEST_CASES),
        .test_cases = PV_ROPE_FFN_TEST_CASES,
};
