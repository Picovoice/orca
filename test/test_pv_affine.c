#include <stdlib.h>

#include "core/picovoice.h"
#include "core/pv_type.h"
#include "test/pv_test.h"

#include "test_data/test_pv_affine_data.c"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

static pv_affine_t *affine_object = NULL;

static pv_status_t test_pv_affine_setup(void) {
    pv_status_t status = pv_affine_init(&TEST_AFFINE_PARAM, &affine_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_affine_teardown(void) {
    pv_affine_delete(affine_object);
    affine_object = NULL;
}

static void test_pv_affine_forward(void) {
    pv_test_true(affine_object != NULL, "failed to create tmp file");

    int32_t num_channels = pv_affine_num_channels(affine_object);
    float *buffer = calloc(TEST_AFFINE_SEQUENCE_LENGTH, num_channels * sizeof(float));

    pv_affine_forward(affine_object, TEST_AFFINE_SEQUENCE_LENGTH, TEST_AFFINE_INPUT, buffer);
    pv_test_close_float_array(
            buffer,
            TEST_AFFINE_TARGET,
            TEST_AFFINE_SEQUENCE_LENGTH * num_channels,
            0.00001f,
            0.00002f,
            "failed to forward affine");

    free(buffer);
}

static const pv_test_case_t PV_AFFINE_TEST_CASES[] = {
        {"affine forward", test_pv_affine_forward},
};

const pv_test_suite_t PV_AFFINE_TEST_SUITE = {
        .name = "affine",
        .setup = test_pv_affine_setup,
        .teardown = test_pv_affine_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_AFFINE_TEST_CASES),
        .test_cases = PV_AFFINE_TEST_CASES,
};
