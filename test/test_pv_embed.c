#include <stdlib.h>

#include "core/picovoice.h"
#include "core/pv_type.h"
#include "test/pv_test.h"

#include "test_data/test_pv_embed_data.c"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

static pv_embed_t *embed_object = NULL;
static pv_ypu_t *ypu = NULL;

static pv_status_t test_pv_embed_setup(void) {
    pv_status_t status = pv_ypu_init_cpu(1, &ypu);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_embed_init(ypu, &TEST_EMBED_PARAM, &embed_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_embed_teardown(void) {
    pv_embed_delete(ypu, embed_object);
    pv_ypu_delete(ypu);
}

static void test_pv_embed_forward(void) {
    int32_t dimension = pv_embed_dimension(embed_object);

    pv_ypu_mem_t *m1 = pv_ypu_mem_alloc(
        ypu,
        TEST_EMBED_SEQUENCE_LENGTH * dimension * sizeof(int32_t),
        PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m1 != NULL, "Failed to allocate m1");
    if (m1 == NULL) {
        return;
    }

    pv_status_t status = status = pv_embed_forward(
        ypu,
        embed_object,
        TEST_EMBED_SEQUENCE_LENGTH,
        TEST_EMBED_INPUT,
        m1,
        0);
    pv_test_true(
        status == PV_STATUS_SUCCESS,
        "pv_embed_forward failed with %s",
        pv_status_to_string(status));

    float *buffer = pv_ypu_mem_get_host_view(ypu, m1, true);
    pv_test_close_float_array(
            buffer,
            TEST_EMBED_TARGET,
            TEST_EMBED_SEQUENCE_LENGTH * dimension,
            0.0001f,
            0.0f,
            "failed to forward embed");
    pv_ypu_mem_release_host_view(ypu, m1, true);

    pv_ypu_mem_free(ypu, m1);
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
