#include <stdlib.h>
#include <string.h>

#include "core/picovoice.h"
#include "core/pv_type.h"
#include "test/pv_test.h"

#include "test_data/test_pv_orca_text_encoder_data.c"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

static pv_ypu_t *ypu = NULL;
static pv_orca_text_encoder_t *orca_text_encoder_object = NULL;

static pv_status_t test_pv_orca_text_encoder_setup(void) {
    pv_status_t status = pv_ypu_init_cpu(1, &ypu);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_orca_text_encoder_init(ypu, &ENC_P_PARAM, &orca_text_encoder_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_orca_text_encoder_teardown(void) {
    pv_orca_text_encoder_delete(ypu, orca_text_encoder_object);
    pv_ypu_delete(ypu);
}

static void test_pv_orca_text_encoder_forward(void) {
    pv_test_true(orca_text_encoder_object != NULL, "failed to create tmp file");

    int32_t num_channels = pv_orca_text_encoder_output_channels(orca_text_encoder_object);

    pv_ypu_mem_t *m0 = pv_ypu_mem_alloc(
        ypu,
        TEST_ORCA_TEXT_ENCODER_SEQUENCE_LENGTH * num_channels * sizeof(float),
        PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m0 != NULL, "Failed to allocate m0");
    if (m0 == NULL) {
        return;
    }

    pv_ypu_mem_t *m1 = pv_ypu_mem_alloc(
        ypu,
        TEST_ORCA_TEXT_ENCODER_SEQUENCE_LENGTH * num_channels * sizeof(float),
        PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m1 != NULL, "Failed to allocate m1");
    if (m1 == NULL) {
        return;
    }

    pv_ypu_mem_t *m2 = pv_ypu_mem_alloc(
        ypu,
        TEST_ORCA_TEXT_ENCODER_SEQUENCE_LENGTH * num_channels * sizeof(float),
        PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m2 != NULL, "Failed to allocate m2");
    if (m2 == NULL) {
        return;
    }

    pv_status_t status = pv_orca_text_encoder_forward(
        ypu,
        orca_text_encoder_object,
        TEST_ORCA_TEXT_ENCODER_SEQUENCE_LENGTH,
        TEST_ORCA_TEXT_ENCODER_INPUT,
        m0,
        m1,
        m2,
        0,
        0,
        0);
    pv_test_true(
        status == PV_STATUS_SUCCESS,
        "pv_orca_text_encoder_forward failed with %s",
        pv_status_to_string(status));

    float *buffer_token = pv_ypu_mem_get_host_view(ypu, m0, true);
    float *buffer_means = pv_ypu_mem_get_host_view(ypu, m1, true);
    float *buffer_logs = pv_ypu_mem_get_host_view(ypu, m2, true);

    pv_test_close_float_array(
            buffer_token,
            TEST_ORCA_TEXT_ENCODER_TARGET,
            TEST_ORCA_TEXT_ENCODER_SEQUENCE_LENGTH * num_channels,
            0.025f,
            0.002f,
            "failed to forward orca_text_encoder");
    pv_test_close_float_array(
            buffer_means,
            TEST_ORCA_TEXT_ENCODER_TARGET_MEANS,
            TEST_ORCA_TEXT_ENCODER_SEQUENCE_LENGTH * num_channels,
            0.025f,
            0.002f,
            "failed to forward orca_text_encoder means");
    pv_test_close_float_array(
            buffer_logs,
            TEST_ORCA_TEXT_ENCODER_TARGET_LOGS,
            TEST_ORCA_TEXT_ENCODER_SEQUENCE_LENGTH * num_channels,
            0.025f,
            0.002f,
            "failed to forward orca_text_encoder logs");

    pv_ypu_mem_release_host_view(ypu, m0, true);
    pv_ypu_mem_release_host_view(ypu, m1, true);
    pv_ypu_mem_release_host_view(ypu, m2, true);
    pv_ypu_mem_free(ypu, m0);
    pv_ypu_mem_free(ypu, m1);
    pv_ypu_mem_free(ypu, m2);
}

#ifdef __PV_MOCKS__

static void *pv_ypu_host_alloc_return_null(pv_ypu_t *ypu, int32_t size_bytes) {
    (void) ypu;
    (void) size_bytes;
    return NULL;
}

static void test_pv_orca_text_encoder_init_failure_helper(pv_status_t expected) {
    pv_orca_text_encoder_t *object = NULL;
    pv_status_t status = pv_orca_text_encoder_init(ypu, &ENC_P_PARAM, &object);
    pv_test_true(
            status == expected,
            "init error, got `%s` expected `%s`",
            pv_status_to_string(status),
            pv_status_to_string(expected));
}

static void test_pv_orca_text_encoder_init_failure_1(void) {
    PV_SET_MOCK_CUSTOM_FUNC(pv_ypu_host_alloc, pv_ypu_host_alloc_return_null)

    test_pv_orca_text_encoder_init_failure_helper(PV_STATUS_OUT_OF_MEMORY);
}

static void test_pv_orca_text_encoder_init_failure_2(void) {
    PV_SET_MOCK_RETURN_VAL(pv_embed_init, PV_STATUS_INVALID_ARGUMENT)

    test_pv_orca_text_encoder_init_failure_helper(PV_STATUS_INVALID_ARGUMENT);
}

static void test_pv_orca_text_encoder_init_failure_3(void) {
    PV_SET_MOCK_RETURN_VAL(pv_embed_init, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_cnn_init, PV_STATUS_INVALID_ARGUMENT)

    test_pv_orca_text_encoder_init_failure_helper(PV_STATUS_INVALID_ARGUMENT);
}

static void test_pv_orca_text_encoder_init_failure_4(void) {
    PV_SET_MOCK_RETURN_VAL(pv_embed_init, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_cnn_init, PV_STATUS_SUCCESS)

    void *(*custom_funcs[])(pv_ypu_t *, int32_t) = {
            pv_ypu_host_alloc_real,
            pv_ypu_host_alloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(pv_ypu_host_alloc, custom_funcs)

    test_pv_orca_text_encoder_init_failure_helper(PV_STATUS_OUT_OF_MEMORY);
}

static void test_pv_orca_text_encoder_init_failure_5(void) {
    PV_SET_MOCK_RETURN_VAL(pv_embed_init, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_cnn_init, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_transformer_init, PV_STATUS_INVALID_ARGUMENT)

    test_pv_orca_text_encoder_init_failure_helper(PV_STATUS_INVALID_ARGUMENT);
}

#endif

static const pv_test_case_t PV_ORCA_TEXT_ENCODER_TEST_CASES[] = {
        {"orca_text_encoder forward", test_pv_orca_text_encoder_forward},

#ifdef __PV_MOCKS__

        {"orca_text_encoder init failure 1", test_pv_orca_text_encoder_init_failure_1},
        {"orca_text_encoder init failure 2", test_pv_orca_text_encoder_init_failure_2},
        {"orca_text_encoder init failure 3", test_pv_orca_text_encoder_init_failure_3},
        {"orca_text_encoder init failure 4", test_pv_orca_text_encoder_init_failure_4},
        {"orca_text_encoder init failure 5", test_pv_orca_text_encoder_init_failure_5},

#endif

};

const pv_test_suite_t PV_ORCA_TEXT_ENCODER_TEST_SUITE = {
        .name = "orca_text_encoder",
        .setup = test_pv_orca_text_encoder_setup,
        .teardown = test_pv_orca_text_encoder_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_ORCA_TEXT_ENCODER_TEST_CASES),
        .test_cases = PV_ORCA_TEXT_ENCODER_TEST_CASES,
};
