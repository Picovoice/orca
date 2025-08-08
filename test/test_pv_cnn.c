#include <stdlib.h>

#include "core/picovoice.h"
#include "core/pv_type.h"
#include "test/pv_test.h"

#include "test_data/test_pv_cnn_k1_data.c"
#include "test_data/test_pv_cnn_k3_data.c"
#include "test_data/test_pv_cnn_k5_data.c"
#include "test_data/test_pv_cnn_k7_data.c"
#include "test_data/test_pv_cnn_depthwise_data.c"
#include "test_data/test_pv_cnn_transposed_depthwise_data.c"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

static const float TEST_TOLERANCE_PERCENT = 0.02f;
static const float TEST_EPSILON = 0.005f;

static void test_pv_cnn_helpers(void) {
    pv_cnn_t *cnn = NULL;
    pv_status_t status = pv_cnn_init(&TEST_CNN_K3_PARAM, &cnn);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to create cnn object");
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    pv_test_true(
            pv_cnn_output_channels(cnn) == TEST_CNN_K3_PARAM.output_channels,
            "failed to get num output channels");

    pv_test_true(
            pv_cnn_input_channels(cnn) == TEST_CNN_K3_PARAM.input_channels,
            "failed to get num input channels");

    pv_test_true(
            pv_cnn_kernel_size(cnn) == TEST_CNN_K3_PARAM.kernel_size,
            "failed to get kernel size");

    pv_test_true(
            pv_cnn_padding(cnn) == TEST_CNN_K3_PARAM.padding,
            "failed to get kernel size");

    pv_test_true(
            pv_cnn_dilation(cnn) == TEST_CNN_K3_PARAM.dilation,
            "failed to get kernel size");

    pv_test_true(
            pv_cnn_stride(cnn) == TEST_CNN_K3_PARAM.stride,
            "failed to get kernel size");
    pv_cnn_delete(cnn);
}

static void test_pv_cnn_transpose_weights(void) {
    const int32_t input_channels = 4;
    const int32_t output_channels = 4;
    const q7_t bias[] = {38, 1, 48, 22, -39};
    const q7_t weight[] = {
            1, 5, 1, 0,
            4, 1, 0, 5,
            4, 0, 1, 1,
            1, 0, 1, 0};
    const q7_t transpose_weight[] = {
            1, 4, 4, 1,
            5, 1, 0, 0,
            1, 0, 1, 1,
            0, 5, 1, 0};
    const pv_cnn_param_t cnn_param = {
            .input_channels = 4,
            .output_channels = 4,
            .kernel_size = 1,
            .stride = 1,
            .padding = 0,
            .dilation = 1,
            .weight = weight,
            .bias = bias,
    };

    pv_cnn_t *cnn = NULL;
    pv_status_t status = pv_cnn_init(&cnn_param, &cnn);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to create cnn object");
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    q7_t *weight_test = (q7_t *) pv_cnn_get_weight(cnn);
    for (int32_t i = 0; i < input_channels * output_channels; i++) {
        pv_test_true(
                weight_test[i] == weight[i],
                "weights are not the same. got `%f`, expected `%f`",
                weight_test[i],
                weight[i]);
    }

    pv_cnn_transpose_weight(cnn, &weight_test);

    for (int32_t i = 0; i < input_channels * output_channels; i++) {
        pv_test_true(
                weight_test[i] == transpose_weight[i],
                "rearranged weights are not the same. got `%f`, expected `%f`",
                weight_test[i],
                transpose_weight[i]);
    }
    free(weight_test);
    pv_cnn_delete(cnn);
}

static void test_pv_cnn_kernel_1_forward(void) {
    pv_cnn_t *cnn = NULL;
    pv_status_t status = pv_cnn_init(&TEST_CNN_K1_PARAM, &cnn);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to create cnn object");
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    int32_t num_channels = pv_cnn_output_channels(cnn);
    float *buffer = calloc(TEST_CNN_K1_SEQUENCE_LENGTH, num_channels * sizeof(float));

    pv_cnn_forward(cnn, TEST_CNN_K1_SEQUENCE_LENGTH, TEST_CNN_K1_INPUT, buffer);
    pv_test_close_float_array(
            buffer,
            TEST_CNN_K1_TARGET,
            TEST_CNN_K1_SEQUENCE_LENGTH * num_channels,
            TEST_TOLERANCE_PERCENT,
            TEST_EPSILON,
            "failed to forward CNN with kernel size 1");

    free(buffer);
    pv_cnn_delete(cnn);
}

static void test_pv_cnn_kernel_1_forward_to_q510(void) {
    pv_cnn_t *cnn = NULL;
    pv_status_t status = pv_cnn_init(&TEST_CNN_K1_PARAM, &cnn);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to create cnn object");
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    int32_t num_channels = pv_cnn_output_channels(cnn);
    q510_t *buffer_q510 = calloc(TEST_CNN_K1_SEQUENCE_LENGTH, num_channels * sizeof(q510_t));
    float *buffer = calloc(TEST_CNN_K1_SEQUENCE_LENGTH, num_channels * sizeof(float));

    pv_cnn_forward_to_q510(cnn, TEST_CNN_K1_SEQUENCE_LENGTH, TEST_CNN_K1_INPUT, buffer_q510);
    for (int32_t i = 0; i < TEST_CNN_K1_SEQUENCE_LENGTH * num_channels; i++) {
        buffer[i] = pv_q510_to_float(buffer_q510[i]);
    }
    pv_test_close_float_array(
            buffer,
            TEST_CNN_K1_TARGET,
            TEST_CNN_K1_SEQUENCE_LENGTH * num_channels,
            0.04f,
            0.02f,
            "failed to forward CNN to q510 with kernel size 1");

    free(buffer_q510);
    free(buffer);
    pv_cnn_delete(cnn);
}

static void test_pv_cnn_kernel_1_forward_from_q510(void) {
    pv_cnn_t *cnn = NULL;
    pv_status_t status = pv_cnn_init(&TEST_CNN_K1_PARAM, &cnn);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to create cnn object");
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    int32_t num_channels = pv_cnn_output_channels(cnn);
    float *buffer = calloc(TEST_CNN_K1_SEQUENCE_LENGTH, num_channels * sizeof(float));
    q510_t *input = calloc(PV_ARRAY_LEN(TEST_CNN_K1_INPUT), sizeof(q510_t));
    for (int32_t i = 0; i < PV_ARRAY_LEN(TEST_CNN_K1_INPUT); i++) {
        input[i] = pv_float_to_q510(TEST_CNN_K1_INPUT[i]);
    }

    pv_cnn_forward_from_q510(cnn, TEST_CNN_K1_SEQUENCE_LENGTH, input, buffer);
    pv_test_close_float_array(
            buffer,
            TEST_CNN_K1_TARGET,
            TEST_CNN_K1_SEQUENCE_LENGTH * num_channels,
            0.04f,
            0.02f,
            "failed to forward CNN from q510 with kernel size 1");

    free(input);
    free(buffer);
    pv_cnn_delete(cnn);
}

static void test_pv_cnn_kernel_3_forward(void) {
    pv_cnn_t *cnn = NULL;
    pv_status_t status = pv_cnn_init(&TEST_CNN_K3_PARAM, &cnn);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to create cnn object");
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    int32_t num_channels = pv_cnn_output_channels(cnn);
    float *buffer = calloc(TEST_CNN_K3_SEQUENCE_LENGTH, num_channels * sizeof(float));

    pv_cnn_forward(cnn, TEST_CNN_K3_SEQUENCE_LENGTH, TEST_CNN_K3_INPUT, buffer);
    pv_test_close_float_array(
            buffer,
            TEST_CNN_K3_TARGET,
            TEST_CNN_K3_SEQUENCE_LENGTH * num_channels,
            TEST_TOLERANCE_PERCENT,
            TEST_EPSILON,
            "failed to forward CNN with kernel size 3");

    free(buffer);
    pv_cnn_delete(cnn);
}

static void test_pv_cnn_kernel_5_forward(void) {
    pv_cnn_t *cnn = NULL;
    pv_status_t status = pv_cnn_init(&TEST_CNN_K5_PARAM, &cnn);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to create cnn object");
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    int32_t num_channels = pv_cnn_output_channels(cnn);
    float *buffer = calloc(TEST_CNN_K5_SEQUENCE_LENGTH, num_channels * sizeof(float));

    pv_cnn_forward(cnn, TEST_CNN_K5_SEQUENCE_LENGTH, TEST_CNN_K5_INPUT, buffer);
    pv_test_close_float_array(
            buffer,
            TEST_CNN_K5_TARGET,
            TEST_CNN_K5_SEQUENCE_LENGTH * num_channels,
            TEST_TOLERANCE_PERCENT,
            TEST_EPSILON,
            "failed to forward CNN with kernel size 5");

    free(buffer);
    pv_cnn_delete(cnn);
}

static void test_pv_cnn_kernel_7_forward(void) {
    pv_cnn_t *cnn = NULL;
    pv_status_t status = pv_cnn_init(&TEST_CNN_K7_PARAM, &cnn);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to create cnn object");
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    int32_t num_channels = pv_cnn_output_channels(cnn);
    float *buffer = calloc(TEST_CNN_K7_SEQUENCE_LENGTH, num_channels * sizeof(float));

    pv_cnn_forward(cnn, TEST_CNN_K7_SEQUENCE_LENGTH, TEST_CNN_K7_INPUT, buffer);
    pv_test_close_float_array(
            buffer,
            TEST_CNN_K7_TARGET,
            TEST_CNN_K7_SEQUENCE_LENGTH * num_channels,
            TEST_TOLERANCE_PERCENT,
            TEST_EPSILON,
            "failed to forward CNN with kernel size 7");

    free(buffer);
    pv_cnn_delete(cnn);
}

static void test_pv_cnn_depthwise_forward(void) {
    pv_cnn_depthwise_t *cnn = NULL;
    pv_status_t status = pv_cnn_depthwise_init(&TEST_CNN_DEPTHWISE_PARAM, &cnn);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to create cnn object");
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    int32_t num_channels = pv_cnn_depthwise_num_channels(cnn);
    float *buffer = calloc(TEST_CNN_DEPTHWISE_SEQUENCE_LENGTH, num_channels * sizeof(float));

    pv_cnn_depthwise_forward(
            cnn,
            TEST_CNN_DEPTHWISE_SEQUENCE_LENGTH,
            TEST_CNN_DEPTHWISE_INPUT,
            buffer);
    pv_test_close_float_array(
            buffer,
            TEST_CNN_DEPTHWISE_TARGET,
            TEST_CNN_DEPTHWISE_SEQUENCE_LENGTH * num_channels,
            0.05f,
            0.03f,
            "failed to forward CNN depthwise");

    free(buffer);
    pv_cnn_depthwise_delete(cnn);
}

static void test_pv_cnn_transposed_depthwise_forward(void) {
    pv_cnn_transposed_depthwise_t *cnn = NULL;
    pv_status_t status = pv_cnn_transposed_depthwise_init(&TEST_CNN_TRANSPOSED_DEPTHWISE_PARAM, &cnn);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to create cnn object");
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    int32_t num_channels = pv_cnn_depthwise_num_channels((const pv_cnn_depthwise_t *) cnn);

    int32_t num_output_frames =
            pv_cnn_transposed_depthwise_num_output_frames(cnn, TEST_CNN_TRANSPOSED_DEPTHWISE_SEQUENCE_LENGTH);
    float *buffer = calloc(num_output_frames, num_channels * sizeof(float));

    pv_cnn_transposed_depthwise_forward(
            cnn,
            TEST_CNN_TRANSPOSED_DEPTHWISE_SEQUENCE_LENGTH,
            TEST_CNN_TRANSPOSED_DEPTHWISE_INPUT,
            buffer);
    pv_test_close_float_array(
            buffer,
            TEST_CNN_TRANSPOSED_DEPTHWISE_TARGET,
            num_output_frames * num_channels,
            0.05f,
            0.03f,
            "failed to forward CNN transposed depthwise");

    free(buffer);
    pv_cnn_transposed_depthwise_delete(cnn);
}

#ifdef __PV_MOCKS__

static void *calloc_return_null(size_t arg0, size_t arg1) {
    (void) arg0;
    (void) arg1;
    return NULL;
}

static void test_pv_cnn_init_1st_calloc_failure(void) {
    PV_SET_MOCK_CUSTOM_FUNC(calloc, calloc_return_null)

    pv_cnn_t *cnn = NULL;
    pv_status_t status = pv_cnn_init(&TEST_CNN_K3_PARAM, &cnn);
    pv_test_true(status == PV_STATUS_OUT_OF_MEMORY, "cnn init should fail with `PV_STATUS_OUT_OF_MEMORY`");
    
    reset_mocks();
    pv_test_error_message(
            "Failed to allocate, out of memory\\.",
            "Failed to allocate memory for `o`\\.",
            true,
            "cnn init 1st calloc failure error message mismatch");
}

static void test_pv_cnn_init_2nd_calloc_failure(void) {
    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_real,
            calloc_return_null
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs)

    pv_cnn_t *cnn = NULL;
    pv_status_t status = pv_cnn_init(&TEST_CNN_K3_PARAM, &cnn);
    pv_test_true(status == PV_STATUS_OUT_OF_MEMORY, "cnn init should fail with `PV_STATUS_OUT_OF_MEMORY`");
    
    reset_mocks();
    pv_test_error_message(
            pv_test_function_hash_regex(),
            "`pv_cnn_transpose_weight` failed with status `OUT_OF_MEMORY`\\.",
            true,
            "cnn init 2nd calloc failure error message mismatch");
}

static void test_pv_cnn_depthwise_init_1st_calloc_failure(void) {
    PV_SET_MOCK_CUSTOM_FUNC(calloc, calloc_return_null)

    pv_cnn_depthwise_t *cnn = NULL;
    pv_status_t status = pv_cnn_depthwise_init(&TEST_CNN_DEPTHWISE_PARAM, &cnn);
    pv_test_true(status == PV_STATUS_OUT_OF_MEMORY, "cnn init should fail with `PV_STATUS_OUT_OF_MEMORY`");
    
    reset_mocks();
    pv_test_error_message(
            "Failed to allocate, out of memory\\.",
            "Failed to allocate memory for `o`\\.",
            true,
            "cnn depthwise init 1st calloc failure error message mismatch");
}

static void test_pv_cnn_depthwise_init_2nd_calloc_failure(void) {
    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_real,
            calloc_return_null
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs)

    pv_cnn_depthwise_t *cnn = NULL;
    pv_status_t status = pv_cnn_depthwise_init(&TEST_CNN_DEPTHWISE_PARAM, &cnn);
    pv_test_true(status == PV_STATUS_OUT_OF_MEMORY, "cnn init should fail with `PV_STATUS_OUT_OF_MEMORY`");
    
    reset_mocks();
    pv_test_error_message(
            pv_test_function_hash_regex(),
            "`pv_cnn_depthwise_transpose_weight` failed with status `OUT_OF_MEMORY`\\.",
            true,
            "cnn depthwise init 2nd calloc failure error message mismatch");
}

#endif

static const pv_test_case_t PV_CNN_TEST_CASES[] = {
        {"cnn helper functions", test_pv_cnn_helpers},
        {"cnn transpose weights", test_pv_cnn_transpose_weights},
        {"cnn forward kernel 1", test_pv_cnn_kernel_1_forward},
        {"cnn forward kernel 1 to q510", test_pv_cnn_kernel_1_forward_to_q510},
        {"cnn forward kernel 1 from q510", test_pv_cnn_kernel_1_forward_from_q510},
        {"cnn forward kernel 3", test_pv_cnn_kernel_3_forward},
        {"cnn forward kernel 5", test_pv_cnn_kernel_5_forward},
        {"cnn forward kernel 7", test_pv_cnn_kernel_7_forward},
        {"cnn depthwise forward", test_pv_cnn_depthwise_forward},
        {"cnn transposed depthwise forward", test_pv_cnn_transposed_depthwise_forward},

#ifdef __PV_MOCKS__

        {"cnn init 1st calloc failure", test_pv_cnn_init_1st_calloc_failure},
        {"cnn init 2nd calloc failure", test_pv_cnn_init_2nd_calloc_failure},
        {"cnn depthwise init 1st calloc failure", test_pv_cnn_depthwise_init_1st_calloc_failure},
        {"cnn depthwise init 2nd calloc failure", test_pv_cnn_depthwise_init_2nd_calloc_failure},

#endif

};

const pv_test_suite_t PV_CNN_TEST_SUITE = {
        .name = "cnn",
        .setup = NULL,
        .teardown = NULL,
        .num_test_cases = PV_ARRAY_LEN(PV_CNN_TEST_CASES),
        .test_cases = PV_CNN_TEST_CASES,
};
