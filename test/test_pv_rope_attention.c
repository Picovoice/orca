#include <stdlib.h>
#include <string.h>

#include "core/picovoice.h"
#include "core/pv_type.h"
#include "test/pv_test.h"

#include "test_data/test_pv_rope_attention_data.c"

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
static pv_rope_attention_t *rope_attention_object = NULL;

static pv_status_t test_pv_rope_attention_setup(void) {
    pv_status_t status = pv_ypu_init_cpu(1, &ypu);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_rope_attention_init(
            ypu,
            &TEST_ROPE_ATTENTION_PARAM,
            &rope_attention_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_rope_attention_teardown(void) {
    pv_rope_attention_delete(ypu, rope_attention_object);
    pv_ypu_delete(ypu);
}

static void test_pv_rope_attention_helpers(void) {
    pv_test_true(
            rope_attention_object->param->dimension == TEST_ROPE_ATTENTION_PARAM.dimension,
            "failed to get dimension");

    pv_test_true(
            rope_attention_object->param->head_dimension == TEST_ROPE_ATTENTION_PARAM.head_dimension,
            "failed to get head_dimension");

    pv_test_true(
            rope_attention_object->param->num_heads == TEST_ROPE_ATTENTION_PARAM.num_heads,
            "failed to get num_heads");

    pv_test_true(
            rope_attention_object->param->ffn_intermediate_dimension == TEST_ROPE_ATTENTION_PARAM.ffn_intermediate_dimension,
            "failed to get ffn_intermediate_dimension");

    pv_test_true(
            rope_attention_object->param->num_lookaheads == TEST_ROPE_ATTENTION_PARAM.num_lookaheads,
            "failed to get num_lookaheads");

    pv_test_true(
            rope_attention_object->param->num_lookbacks == TEST_ROPE_ATTENTION_PARAM.num_lookbacks,
            "failed to get num_lookbacks");

    pv_test_true(
            rope_attention_object->param->rope_base == TEST_ROPE_ATTENTION_PARAM.rope_base,
            "failed to get rope_base");

    pv_test_true(
            rope_attention_object->param->sdpa_downsample_factor == TEST_ROPE_ATTENTION_PARAM.sdpa_downsample_factor,
            "failed to get sdpa_downsample_factor");
}

static pv_status_t test_pv_rope_attention_forward_inner(bool test_accuracy) {

    const int32_t N = TEST_ROPE_ATTENTION_SEQUENCE_LENGTH;
    const int32_t dimension = TEST_ROPE_ATTENTION_PARAM.dimension;

    pv_ypu_mem_t *m0 = pv_ypu_mem_alloc(
            ypu,
            sizeof(TEST_ROPE_ATTENTION_INPUT),
            PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m0 != NULL, "Failed to allocate m0");
    if (m0 == NULL) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_status_t status = pv_ypu_mem_copy_to(
            ypu,
            m0,
            TEST_ROPE_ATTENTION_INPUT,
            0,
            sizeof(TEST_ROPE_ATTENTION_INPUT));
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
        return PV_STATUS_OUT_OF_MEMORY;
    }

    status = pv_rope_attention_forward(
            ypu,
            rope_attention_object,
            N,
            m0,
            NULL,
            m1);
    pv_ypu_mem_free(ypu, m0);

    if (test_accuracy) {
        pv_test_true(status == PV_STATUS_SUCCESS, "pv_rope_attention_forward() returns failed status");
        float *buffer_output = pv_ypu_mem_get_host_view(ypu, m1, true);
        pv_test_close_float_array(
                buffer_output,
                TEST_ROPE_ATTENTION_TARGET,
                N * dimension,
                TEST_TOLERANCE_PERCENT,
                TEST_EPSILON,
                "failed to match target");
        pv_ypu_mem_release_host_view(ypu, m1, false);
    }

    pv_ypu_mem_free(ypu, m1);

    return status;
}

static void test_pv_rope_attention_forward(void) {
    test_pv_rope_attention_forward_inner(true);
}


static const pv_test_case_t PV_ROPE_ATTENTION_TEST_CASES[] = {
        {"rope attention helper functions", test_pv_rope_attention_helpers},
        {"rope attention forward", test_pv_rope_attention_forward},
};

const pv_test_suite_t PV_ROPE_ATTENTION_TEST_SUITE = {
        .name = "rope_attention",
        .setup = test_pv_rope_attention_setup,
        .teardown = test_pv_rope_attention_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_ROPE_ATTENTION_TEST_CASES),
        .test_cases = PV_ROPE_ATTENTION_TEST_CASES,
};
