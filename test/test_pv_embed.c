#include <stdlib.h>

#include "core/picovoice.h"
#include "core/pv_type.h"
#include "test/pv_test.h"

#include "test_data/test_pv_embed_data.c"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

static pv_embed_t *embed_object = NULL;

static pv_status_t test_pv_embed_setup(void) {
    pv_status_t status = pv_embed_init(&TEST_EMBED_PARAM, &embed_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_embed_teardown(void) {
    pv_embed_delete(embed_object);
    embed_object = NULL;
}

static void test_pv_embed_forward(void) {
    pv_test_true(embed_object != NULL, "failed to create tmp file");

    int32_t dimension = pv_embed_dimension(embed_object);
    float *buffer = calloc(TEST_EMBED_SEQUENCE_LENGTH, dimension * sizeof(float));

    pv_embed_forward(embed_object, TEST_EMBED_SEQUENCE_LENGTH, TEST_EMBED_INPUT, buffer);
    pv_test_close_float_array(
            buffer,
            TEST_EMBED_TARGET,
            TEST_EMBED_SEQUENCE_LENGTH * dimension,
            0.0001f,
            0.0f,
            "failed to forward embed");

    free(buffer);
}

static const pv_test_case_t PV_EMBED_TEST_CASES[] = {
        {"embed forward",          test_pv_embed_forward},
};

const pv_test_suite_t PV_EMBED_TEST_SUITE = {
        .name = "embed",
        .setup = test_pv_embed_setup,
        .teardown = test_pv_embed_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_EMBED_TEST_CASES),
        .test_cases = PV_EMBED_TEST_CASES,
};
