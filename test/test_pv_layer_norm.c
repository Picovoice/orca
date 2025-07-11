#include <stdlib.h>

#include "core/picovoice.h"
#include "core/pv_type.h"
#include "test/pv_test.h"

#include "test_data/test_pv_layer_norm_data.c"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

static pv_layer_norm_t *layer_norm_object = NULL;

static pv_status_t test_pv_layer_norm_setup(void) {
    pv_status_t status = pv_layer_norm_init(&TEST_LAYER_NORM_PARAM, &layer_norm_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_layer_norm_teardown(void) {
    pv_layer_norm_delete(layer_norm_object);
    layer_norm_object = NULL;
}

static void test_pv_layer_norm_forward(void) {
    pv_test_true(layer_norm_object != NULL, "failed to create tmp file");

    int32_t num_channels = pv_layer_norm_num_channels(layer_norm_object);
    float *buffer = calloc(TEST_LAYER_NORM_SEQUENCE_LENGTH, num_channels * sizeof(float));

    pv_layer_norm_forward(layer_norm_object, TEST_LAYER_NORM_SEQUENCE_LENGTH, TEST_LAYER_NORM_INPUT, buffer);
    pv_test_close_float_array(
            buffer,
            TEST_LAYER_NORM_TARGET,
            TEST_LAYER_NORM_SEQUENCE_LENGTH * num_channels,
            0.001f,
            0.002f,
            "failed to forward layer_norm");

    free(buffer);
}

static const pv_test_case_t PV_LAYER_NORM_TEST_CASES[] = {
        {"layer_norm forward", test_pv_layer_norm_forward},
};

const pv_test_suite_t PV_LAYER_NORM_TEST_SUITE = {
        .name = "layer_norm",
        .setup = test_pv_layer_norm_setup,
        .teardown = test_pv_layer_norm_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_LAYER_NORM_TEST_CASES),
        .test_cases = PV_LAYER_NORM_TEST_CASES,
};
