#include <stdlib.h>

#include "core/picovoice.h"
#include "core/pv_type.h"
#include "test/pv_test.h"

#include "test_data/test_pv_vocos_backbone_data.c"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

static const float TEST_TOLERANCE_PERCENT = 0.03f;
static const float TEST_EPSILON = 0.03f;

static pv_ypu_t *ypu = NULL;
static pv_vocos_backbone_t *vocos_backbone_object = NULL;

static pv_status_t test_pv_vocos_backbone_setup(void) {
    pv_status_t status = pv_ypu_init_cpu(1, &ypu);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_vocos_backbone_init(
            ypu,
            &DEC_BACKBONE_0_PARAM,
            &vocos_backbone_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_vocos_backbone_teardown(void) {
    pv_vocos_backbone_delete(ypu, vocos_backbone_object);
    pv_ypu_delete(ypu);
}

static void test_pv_vocos_backbone_forward(void) {
    int32_t num_channels = pv_vocos_backbone_output_channels(vocos_backbone_object);

    pv_ypu_mem_t *m0 = pv_ypu_mem_alloc(
            ypu,
            sizeof(TEST_VOCOS_BACKBONE_INPUT),
            PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m0 != NULL, "Failed to allocate m0");
    if (m0 == NULL) {
        return;
    }

    pv_status_t status = pv_ypu_mem_copy_to(
            ypu,
            m0,
            TEST_VOCOS_BACKBONE_INPUT,
            0,
            sizeof(TEST_VOCOS_BACKBONE_INPUT));
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "pv_ypu_mem_copy_to failed with %s",
            pv_status_to_string(status));

    pv_ypu_mem_t *m1 = pv_ypu_mem_alloc(
            ypu,
            TEST_VOCOS_BACKBONE_SEQUENCE_LENGTH * num_channels * sizeof(float),
            PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m1 != NULL, "Failed to allocate m1");
    if (m1 == NULL) {
        return;
    }

    status = pv_vocos_backbone_forward(
            ypu,
            vocos_backbone_object,
            TEST_VOCOS_BACKBONE_SEQUENCE_LENGTH,
            m0,
            m1);
    pv_test_true(status == PV_STATUS_SUCCESS, "pv_vocos_backbone_forward() returns failed status");
    pv_ypu_mem_free(ypu, m0);

    float *buffer_output = pv_ypu_mem_get_host_view(ypu, m1, true);
    pv_test_close_float_array(
            buffer_output,
            TEST_VOCOS_BACKBONE_TARGET,
            TEST_VOCOS_BACKBONE_SEQUENCE_LENGTH * num_channels,
            TEST_TOLERANCE_PERCENT,
            TEST_EPSILON,
            "failed to forward vocos_backbone");
    pv_ypu_mem_release_host_view(ypu, m1, false);

    pv_ypu_mem_free(ypu, m1);
}

static const pv_test_case_t PV_VOCOS_BACKBONE_TEST_CASES[] = {
        {"vocos_backbone forward", test_pv_vocos_backbone_forward},
};

const pv_test_suite_t PV_VOCOS_BACKBONE_TEST_SUITE = {
        .name = "vocos_backbone",
        .setup = test_pv_vocos_backbone_setup,
        .teardown = test_pv_vocos_backbone_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_VOCOS_BACKBONE_TEST_CASES),
        .test_cases = PV_VOCOS_BACKBONE_TEST_CASES,
};
