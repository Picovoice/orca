#include <stdlib.h>

#include "core/picovoice.h"
#include "core/pv_type.h"
#include "test/pv_test.h"

#include "test_data/test_pv_transformer_ffn_data.c"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

static pv_transformer_ffn_t *transformer_ffn_object = NULL;
static pv_buffer_t *buffer_object = NULL;

static pv_status_t test_pv_transformer_ffn_setup(void) {
    int32_t num_hidden_channels = TEST_TRANSFORMER_FFN_CONV_1_PARAM.output_channels;
    pv_status_t status = pv_buffer_init(num_hidden_channels, &buffer_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_transformer_ffn_init(&TEST_TRANSFORMER_FFN_PARAM, buffer_object, &transformer_ffn_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_transformer_ffn_teardown(void) {
    pv_buffer_delete(buffer_object);
    pv_transformer_ffn_delete(transformer_ffn_object);
    transformer_ffn_object = NULL;
}

static void test_pv_transformer_ffn_forward(void) {
    pv_test_true(transformer_ffn_object != NULL, "failed to create tmp file");

    int32_t num_channels = pv_transformer_ffn_output_channels(transformer_ffn_object);
    float *buffer = calloc(TEST_TRANSFORMER_FFN_SEQUENCE_LENGTH, num_channels * sizeof(float));

    pv_transformer_ffn_forward(
            transformer_ffn_object,
            TEST_TRANSFORMER_FFN_SEQUENCE_LENGTH,
            TEST_TRANSFORMER_FFN_INPUT,
            buffer);
    pv_test_close_float_array(
            buffer,
            TEST_TRANSFORMER_FFN_TARGET,
            TEST_TRANSFORMER_FFN_SEQUENCE_LENGTH * num_channels,
            0.005f,
            0.002f,
            "failed to forward transformer_ffn");

    free(buffer);
}

static const pv_test_case_t PV_TRANSFORMER_FFN_TEST_CASES[] = {
        {"transformer_ffn forward", test_pv_transformer_ffn_forward},
};

const pv_test_suite_t PV_TRANSFORMER_FFN_TEST_SUITE = {
        .name = "transformer_ffn",
        .setup = test_pv_transformer_ffn_setup,
        .teardown = test_pv_transformer_ffn_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_TRANSFORMER_FFN_TEST_CASES),
        .test_cases = PV_TRANSFORMER_FFN_TEST_CASES,
};
