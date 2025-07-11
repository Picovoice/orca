#include <stdlib.h>
#include <string.h>

#include "core/picovoice.h"
#include "core/pv_type.h"
#include "test/pv_test.h"

#include "test_data/test_pv_orca_text_encoder_data.c"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

static pv_orca_text_encoder_t *orca_text_encoder_object = NULL;

static pv_status_t test_pv_orca_text_encoder_setup(void) {
    pv_status_t status = pv_orca_text_encoder_init(&ENC_P_PARAM, &orca_text_encoder_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_orca_text_encoder_teardown(void) {
    pv_orca_text_encoder_delete(orca_text_encoder_object);
    orca_text_encoder_object = NULL;
}

static void test_pv_orca_text_encoder_forward(void) {
    pv_test_true(orca_text_encoder_object != NULL, "failed to create tmp file");

    int32_t num_channels = pv_orca_text_encoder_output_channels(orca_text_encoder_object);
    float *buffer_token = calloc(TEST_ORCA_TEXT_ENCODER_SEQUENCE_LENGTH * num_channels, sizeof(float));
    float *buffer_means = calloc(TEST_ORCA_TEXT_ENCODER_SEQUENCE_LENGTH * num_channels, sizeof(float));
    float *buffer_logs = calloc(TEST_ORCA_TEXT_ENCODER_SEQUENCE_LENGTH * num_channels, sizeof(float));

    pv_orca_text_encoder_forward(
            orca_text_encoder_object,
            TEST_ORCA_TEXT_ENCODER_SEQUENCE_LENGTH,
            TEST_ORCA_TEXT_ENCODER_INPUT,
            buffer_token,
            buffer_means,
            buffer_logs);

    pv_test_close_float_array(
            buffer_token,
            TEST_ORCA_TEXT_ENCODER_TARGET,
            TEST_ORCA_TEXT_ENCODER_SEQUENCE_LENGTH * num_channels,

#ifndef __ORCA_FLOAT_MODE__

            0.025f,
            0.002f,

#else

            0.001f,
            0.001f,

#endif

            "failed to forward orca_text_encoder");
    pv_test_close_float_array(
            buffer_means,
            TEST_ORCA_TEXT_ENCODER_TARGET_MEANS,
            TEST_ORCA_TEXT_ENCODER_SEQUENCE_LENGTH * num_channels,

#ifndef __ORCA_FLOAT_MODE__

            0.025f,
            0.002f,

#else

            0.001f,
            0.001f,

#endif

            "failed to forward orca_text_encoder means");
    pv_test_close_float_array(
            buffer_logs,
            TEST_ORCA_TEXT_ENCODER_TARGET_LOGS,
            TEST_ORCA_TEXT_ENCODER_SEQUENCE_LENGTH * num_channels,

#ifndef __ORCA_FLOAT_MODE__

            0.025f,
            0.002f,

#else

            0.001f,
            0.001f,

#endif

            "failed to forward orca_text_encoder logs");
    free(buffer_token);
    free(buffer_means);
    free(buffer_logs);
}

#ifdef __PV_MOCKS__

static void *calloc_return_null(size_t arg0, size_t arg1) {
    (void) arg0;
    (void) arg1;
    return NULL;
}

static void test_pv_orca_text_encoder_init_failure_helper(pv_status_t expected) {
    pv_orca_text_encoder_t *object = NULL;
    pv_status_t status = pv_orca_text_encoder_init(&ENC_P_PARAM, &object);
    pv_test_true(
            status == expected,
            "init error, got `%s` expected `%s`",
            pv_status_to_string(status),
            pv_status_to_string(expected));
}

static void test_pv_orca_text_encoder_init_failure_1(void) {
    PV_SET_MOCK_CUSTOM_FUNC(calloc, calloc_return_null)

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

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_real,
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs)

    test_pv_orca_text_encoder_init_failure_helper(PV_STATUS_OUT_OF_MEMORY);
}

static void test_pv_orca_text_encoder_init_failure_5(void) {
    PV_SET_MOCK_RETURN_VAL(pv_embed_init, PV_STATUS_SUCCESS)

    pv_status_t custom_rets[] = {
            PV_STATUS_INVALID_ARGUMENT,
    };
    PV_SET_MOCK_RETURN_SEQ(pv_buffer_init, custom_rets);

    test_pv_orca_text_encoder_init_failure_helper(PV_STATUS_INVALID_ARGUMENT);
}

static void test_pv_orca_text_encoder_init_failure_6(void) {
    PV_SET_MOCK_RETURN_VAL(pv_embed_init, PV_STATUS_SUCCESS)

    pv_status_t custom_rets[] = {
            PV_STATUS_SUCCESS,
            PV_STATUS_INVALID_ARGUMENT,
    };
    PV_SET_MOCK_RETURN_SEQ(pv_buffer_init, custom_rets);

    test_pv_orca_text_encoder_init_failure_helper(PV_STATUS_INVALID_ARGUMENT);
}

static void test_pv_orca_text_encoder_init_failure_7(void) {
    PV_SET_MOCK_RETURN_VAL(pv_embed_init, PV_STATUS_SUCCESS)

    pv_status_t custom_rets[] = {
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_INVALID_ARGUMENT,
    };
    PV_SET_MOCK_RETURN_SEQ(pv_buffer_init, custom_rets);

    test_pv_orca_text_encoder_init_failure_helper(PV_STATUS_INVALID_ARGUMENT);
}

static void test_pv_orca_text_encoder_init_failure_8(void) {
    PV_SET_MOCK_RETURN_VAL(pv_embed_init, PV_STATUS_SUCCESS)

    pv_status_t custom_rets[] = {
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_INVALID_ARGUMENT,
    };
    PV_SET_MOCK_RETURN_SEQ(pv_buffer_init, custom_rets);

    test_pv_orca_text_encoder_init_failure_helper(PV_STATUS_INVALID_ARGUMENT);
}

static void test_pv_orca_text_encoder_init_failure_9(void) {
    PV_SET_MOCK_RETURN_VAL(pv_embed_init, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_buffer_init, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_transformer_init, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_text_encoder_init_failure_helper(PV_STATUS_OUT_OF_MEMORY);
}

static void test_pv_orca_text_encoder_init_failure_10(void) {
    pv_status_t custom_rets[] = {
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_INVALID_ARGUMENT,
    };
    PV_SET_MOCK_RETURN_SEQ(pv_buffer_init, custom_rets);

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
        {"orca_text_encoder init failure 6", test_pv_orca_text_encoder_init_failure_6},
        {"orca_text_encoder init failure 7", test_pv_orca_text_encoder_init_failure_7},
        {"orca_text_encoder init failure 8", test_pv_orca_text_encoder_init_failure_8},
        {"orca_text_encoder init failure 9", test_pv_orca_text_encoder_init_failure_9},
        {"orca_text_encoder init failure 10", test_pv_orca_text_encoder_init_failure_10},

#endif

};

const pv_test_suite_t PV_ORCA_TEXT_ENCODER_TEST_SUITE = {
        .name = "orca_text_encoder",
        .setup = test_pv_orca_text_encoder_setup,
        .teardown = test_pv_orca_text_encoder_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_ORCA_TEXT_ENCODER_TEST_CASES),
        .test_cases = PV_ORCA_TEXT_ENCODER_TEST_CASES,
};
