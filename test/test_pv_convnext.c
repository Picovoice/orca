#include <stdlib.h>

#include "core/picovoice.h"
#include "core/pv_type.h"
#include "orca/pv_buffer.h"
#include "orca/pv_buffer_q510.h"
#include "orca/pv_convnext_transposed.h"
#include "test/pv_test.h"

#include "test_data/test_pv_convnext_data.c"
#include "test_data/test_pv_convnext_transposed_data.c"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

static pv_convnext_t *convnext_object = NULL;

static pv_buffer_t *buffer_1_object = NULL;
static pv_buffer_q510_t *buffer_2_object = NULL;

static pv_status_t test_pv_convnext_setup(void) {
    pv_status_t status = pv_buffer_init(TEST_CONVNEXT_PARAM.conv_depthwise_param->num_channels, &buffer_1_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_buffer_q510_init(TEST_CONVNEXT_PARAM.conv_1_param->output_channels, &buffer_2_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_convnext_init(
            &TEST_CONVNEXT_PARAM,
            buffer_1_object,
            buffer_2_object,
            &convnext_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_convnext_teardown(void) {
    pv_buffer_q510_delete(buffer_2_object);
    pv_buffer_delete(buffer_1_object);
    pv_convnext_delete(convnext_object);
    convnext_object = NULL;
}

static void test_pv_convnext_forward(void) {
    pv_test_true(convnext_object != NULL, "failed to create tmp file");

    int32_t num_channels = PV_ARRAY_LEN(TEST_CONVNEXT_TARGET) / TEST_CONVNEXT_SEQUENCE_LENGTH;
    float *buffer = calloc(TEST_CONVNEXT_SEQUENCE_LENGTH, num_channels * sizeof(float));

    pv_convnext_forward(convnext_object, TEST_CONVNEXT_SEQUENCE_LENGTH, TEST_CONVNEXT_INPUT, buffer);
    pv_test_close_float_array(
            buffer,
            TEST_CONVNEXT_TARGET,
            TEST_CONVNEXT_SEQUENCE_LENGTH * num_channels,
            0.08f,
            0.04f,
            "failed to forward convnext");

    free(buffer);
}

#ifdef __PV_MOCKS__

static void test_pv_convnext_param_load_helper(pv_status_t expected) {
    FILE *dummy_file = malloc(sizeof(FILE *));
    pv_test_true(dummy_file != NULL, "failed create dummy file");
    if (!dummy_file) {
        return;
    }

    pv_convnext_param_t *param = NULL;
    pv_status_t status = pv_convnext_param_load(dummy_file, &param);
    pv_test_true(
            status == expected,
            "param load error, got `%s` expected `%s`",
            pv_status_to_string(status),
            pv_status_to_string(expected));
    free(dummy_file);
}

static void test_pv_convnext_param_load_failure_1(void) {
    PV_SET_MOCK_RETURN_VAL(pv_cnn_depthwise_param_load, PV_STATUS_INVALID_ARGUMENT)

    test_pv_convnext_param_load_helper(PV_STATUS_INVALID_ARGUMENT);
}

static void test_pv_convnext_param_load_failure_2(void) {
    PV_SET_MOCK_RETURN_VAL(pv_cnn_depthwise_param_load, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_layer_norm_param_load, PV_STATUS_INVALID_ARGUMENT)

    test_pv_convnext_param_load_helper(PV_STATUS_INVALID_ARGUMENT);
}

static void test_pv_convnext_param_load_failure_3(void) {
    PV_SET_MOCK_RETURN_VAL(pv_cnn_depthwise_param_load, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_layer_norm_param_load, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_cnn_param_load, PV_STATUS_INVALID_ARGUMENT)

    test_pv_convnext_param_load_helper(PV_STATUS_INVALID_ARGUMENT);
}

static void test_pv_convnext_param_load_failure_4(void) {
    PV_SET_MOCK_RETURN_VAL(pv_cnn_depthwise_param_load, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_layer_norm_param_load, PV_STATUS_SUCCESS)
    pv_status_t custom_rets[] = {
            PV_STATUS_SUCCESS,
            PV_STATUS_INVALID_ARGUMENT,
    };
    PV_SET_MOCK_RETURN_SEQ(pv_cnn_param_load, custom_rets);
    test_pv_convnext_param_load_helper(PV_STATUS_INVALID_ARGUMENT);
}

static void test_pv_convnext_transposed_param_load_helper(pv_status_t expected) {
    FILE *dummy_file = malloc(sizeof(FILE *));
    pv_test_true(dummy_file != NULL, "failed create dummy file");
    if (!dummy_file) {
        return;
    }

    pv_convnext_transposed_param_t *param = NULL;
    pv_status_t status = pv_convnext_transposed_param_load(dummy_file, &param);
    pv_test_true(
            status == expected,
            "param load error, got `%s` expected `%s`",
            pv_status_to_string(status),
            pv_status_to_string(expected));
    free(dummy_file);
}

static void test_pv_convnext_transposed_param_load_failure_1(void) {
    PV_SET_MOCK_RETURN_VAL(pv_cnn_depthwise_param_load, PV_STATUS_INVALID_ARGUMENT)

    test_pv_convnext_transposed_param_load_helper(PV_STATUS_INVALID_ARGUMENT);
}

static void test_pv_convnext_transposed_param_load_failure_2(void) {
    PV_SET_MOCK_RETURN_VAL(pv_cnn_depthwise_param_load, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_layer_norm_param_load, PV_STATUS_INVALID_ARGUMENT)

    test_pv_convnext_transposed_param_load_helper(PV_STATUS_INVALID_ARGUMENT);
}

static void test_pv_convnext_transposed_param_load_failure_3(void) {
    PV_SET_MOCK_RETURN_VAL(pv_cnn_depthwise_param_load, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_layer_norm_param_load, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_cnn_param_load, PV_STATUS_INVALID_ARGUMENT)

    test_pv_convnext_transposed_param_load_helper(PV_STATUS_INVALID_ARGUMENT);
}

static void test_pv_convnext_transposed_param_load_failure_4(void) {
    PV_SET_MOCK_RETURN_VAL(pv_cnn_depthwise_param_load, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_layer_norm_param_load, PV_STATUS_SUCCESS)
    pv_status_t custom_rets[] = {
            PV_STATUS_SUCCESS,
            PV_STATUS_INVALID_ARGUMENT,
    };
    PV_SET_MOCK_RETURN_SEQ(pv_cnn_param_load, custom_rets);
    test_pv_convnext_transposed_param_load_helper(PV_STATUS_INVALID_ARGUMENT);
}

#endif

static void test_pv_convnext_transposed_forward(void) {
    pv_convnext_transposed_t *convnext_transposed_object = NULL;
    pv_status_t status = pv_convnext_transposed_init(&TEST_CONVNEXT_TRANSPOSED_PARAM, &convnext_transposed_object);
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    pv_test_true(convnext_transposed_object != NULL, "failed to create tmp file");

    int32_t num_channels = pv_convnext_transposed_output_channels(convnext_transposed_object);

    int32_t num_output_frames =
            pv_convnext_transposed_num_output_frames(convnext_transposed_object, TEST_CONVNEXT_TRANSPOSED_SEQUENCE_LENGTH);
    float *buffer = calloc(num_output_frames, num_channels * sizeof(float));

    pv_convnext_transposed_forward(
            convnext_transposed_object,
            TEST_CONVNEXT_TRANSPOSED_SEQUENCE_LENGTH,
            TEST_CONVNEXT_TRANSPOSED_INPUT,
            buffer);
    pv_test_close_float_array(
            buffer,
            TEST_CONVNEXT_TRANSPOSED_TARGET,
            num_output_frames * num_channels,
            0.05f,
            0.02f,
            "failed to forward convnext transposed");

    free(buffer);
    pv_convnext_transposed_delete(convnext_transposed_object);
}

static const pv_test_case_t PV_CONVNEXT_TEST_CASES[] = {
        {"convnext forward",          test_pv_convnext_forward},
        {"convnext transposed forward", test_pv_convnext_transposed_forward},

#ifdef __PV_MOCKS__

        {"param load failure 1",      test_pv_convnext_param_load_failure_1},
        {"param load failure 2",      test_pv_convnext_param_load_failure_2},
        {"param load failure 3",      test_pv_convnext_param_load_failure_3},
        {"param load failure 4",      test_pv_convnext_param_load_failure_4},

        {"transposed param load failure 1", test_pv_convnext_transposed_param_load_failure_1},
        {"transposed param load failure 2", test_pv_convnext_transposed_param_load_failure_2},
        {"transposed param load failure 3", test_pv_convnext_transposed_param_load_failure_3},
        {"transposed param load failure 4", test_pv_convnext_transposed_param_load_failure_4},

#endif

};

const pv_test_suite_t PV_CONVNEXT_TEST_SUITE = {
        .name = "convnext",
        .setup = test_pv_convnext_setup,
        .teardown = test_pv_convnext_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_CONVNEXT_TEST_CASES),
        .test_cases = PV_CONVNEXT_TEST_CASES,
};
