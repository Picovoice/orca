#include <stdlib.h>

#include "core/picovoice.h"
#include "core/pv_type.h"
#include "test/pv_test.h"

#include "test_data/test_pv_convnext_film_data.c"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

static pv_ypu_t *ypu = NULL;
static pv_convnext_film_t *convnext_film_object = NULL;

static pv_status_t test_pv_convnext_film_setup(void) {
    pv_status_t status = pv_ypu_init_cpu(1, &ypu);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_convnext_film_init(
            ypu,
            &TEST_CONVNEXT_FILM_PARAM,
            &convnext_film_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_convnext_film_teardown(void) {
    pv_convnext_film_delete(ypu, convnext_film_object);
    pv_ypu_delete(ypu);
}

static void test_pv_convnext_film_forward(void) {
    int32_t num_channels = PV_ARRAY_LEN(TEST_CONVNEXT_FILM_TARGET) / TEST_CONVNEXT_FILM_SEQUENCE_LENGTH;

    pv_ypu_mem_t *m0 = pv_ypu_mem_alloc(
            ypu,
            sizeof(TEST_CONVNEXT_FILM_INPUT_0),
            PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m0 != NULL, "Failed to allocate m0");
    if (m0 == NULL) {
        return;
    }

    pv_status_t status = pv_ypu_mem_copy_to(
            ypu,
            m0,
            TEST_CONVNEXT_FILM_INPUT_0,
            0,
            sizeof(TEST_CONVNEXT_FILM_INPUT_0));
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "pv_ypu_mem_copy_to failed with %s",
            pv_status_to_string(status));

    pv_ypu_mem_t *m1 = pv_ypu_mem_alloc(
            ypu,
            sizeof(TEST_CONVNEXT_FILM_INPUT_1),
            PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m1 != NULL, "Failed to allocate m1");
    if (m1 == NULL) {
        return;
    }

    status = pv_ypu_mem_copy_to(
            ypu,
            m1,
            TEST_CONVNEXT_FILM_INPUT_1,
            0,
            sizeof(TEST_CONVNEXT_FILM_INPUT_1));
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "pv_ypu_mem_copy_to failed with %s",
            pv_status_to_string(status));

    pv_ypu_mem_t *m2 = pv_ypu_mem_alloc(
            ypu,
            sizeof(TEST_CONVNEXT_FILM_INPUT_2),
            PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m2 != NULL, "Failed to allocate m2");
    if (m2 == NULL) {
        return;
    }

    status = pv_ypu_mem_copy_to(
            ypu,
            m2,
            TEST_CONVNEXT_FILM_INPUT_2,
            0,
            sizeof(TEST_CONVNEXT_FILM_INPUT_2));
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "pv_ypu_mem_copy_to failed with %s",
            pv_status_to_string(status));

    pv_ypu_mem_t *m3 = pv_ypu_mem_alloc(
            ypu,
            sizeof(TEST_CONVNEXT_FILM_INPUT_3),
            PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m3 != NULL, "Failed to allocate m3");
    if (m3 == NULL) {
        return;
    }

    status = pv_ypu_mem_copy_to(
            ypu,
            m3,
            TEST_CONVNEXT_FILM_INPUT_3,
            0,
            sizeof(TEST_CONVNEXT_FILM_INPUT_3));
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "pv_ypu_mem_copy_to failed with %s",
            pv_status_to_string(status));

    pv_ypu_mem_t *buffer_ypu = pv_ypu_mem_alloc(
            ypu,
            TEST_CONVNEXT_FILM_SEQUENCE_LENGTH * num_channels * sizeof(float),
            PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(buffer_ypu != NULL, "Failed to allocate buffer");
    if (buffer_ypu == NULL) {
        return;
    }

    pv_convnext_film_forward(
            ypu,
            convnext_film_object,
            TEST_CONVNEXT_FILM_SEQUENCE_LENGTH,
            m0,
            m1,
            m2,
            m3,
            buffer_ypu,
            0,
            0,
            0,
            0,
            0);
    pv_ypu_mem_free(ypu, m0);
    pv_ypu_mem_free(ypu, m1);
    pv_ypu_mem_free(ypu, m2);
    pv_ypu_mem_free(ypu, m3);

    float *buffer = pv_ypu_mem_get_host_view(ypu, buffer_ypu, true);
    pv_test_close_float_array(
            buffer,
            TEST_CONVNEXT_FILM_TARGET,
            TEST_CONVNEXT_FILM_SEQUENCE_LENGTH * num_channels,
            0.08f,
            0.04f,
            "failed to forward convnext_film");
    pv_ypu_mem_release_host_view(ypu, buffer_ypu, false);
    pv_ypu_mem_free(ypu, buffer_ypu);
}

static const pv_test_case_t PV_CONVNEXT_FILM_TEST_CASES[] = {
        {"convnext_film forward", test_pv_convnext_film_forward},
};

const pv_test_suite_t PV_CONVNEXT_FILM_TEST_SUITE = {
        .name = "convnext_film",
        .setup = test_pv_convnext_film_setup,
        .teardown = test_pv_convnext_film_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_CONVNEXT_FILM_TEST_CASES),
        .test_cases = PV_CONVNEXT_FILM_TEST_CASES,
};
