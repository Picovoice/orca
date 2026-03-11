#include <stdlib.h>
#include <string.h>

#include "core/picovoice.h"
#include "core/pv_type.h"
#include "test/pv_test.h"

#include "test_data/test_pv_orca_lfm_film_generator_data.c"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

#ifdef __ORCA_FLOAT_MODE__

static const float TEST_TOLERANCE_PERCENT = 0.0005f;
static const float TEST_EPSILON = 0.005f;

#else

static const float TEST_TOLERANCE_PERCENT = 0.075f;
static const float TEST_EPSILON = 0.05f;

#endif

static pv_ypu_t *ypu = NULL;
static pv_orca_lfm_film_generator_t *orca_lfm_film_generator_object = NULL;

static pv_status_t test_pv_orca_lfm_film_generator_setup(void) {
    pv_status_t status = pv_ypu_init_cpu(1, &ypu);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_orca_lfm_film_generator_init(
            ypu,
            &LFM_FILM_GENERATOR_PARAM,
            &orca_lfm_film_generator_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_orca_lfm_film_generator_teardown(void) {
    pv_orca_lfm_film_generator_delete(ypu, orca_lfm_film_generator_object);
    pv_ypu_delete(ypu);
}

static pv_status_t test_pv_orca_lfm_film_generator_forward_inner(bool test_accuracy) {

    const int32_t N = TEST_ORCA_LFM_FILM_GENERATOR_SEQUENCE_LENGTH_0;
    const int32_t T = TEST_ORCA_LFM_FILM_GENERATOR_SEQUENCE_LENGTH_1;
    const int32_t dimension = LFM_FILM_GENERATOR_PARAM.dimension;

    pv_ypu_mem_t *m0 = pv_ypu_mem_alloc(
            ypu,
            sizeof(TEST_ORCA_LFM_FILM_GENERATOR_INPUT_0),
            PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m0 != NULL, "Failed to allocate m0");
    if (m0 == NULL) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_status_t status = pv_ypu_mem_copy_to(
            ypu,
            m0,
            TEST_ORCA_LFM_FILM_GENERATOR_INPUT_0,
            0,
            sizeof(TEST_ORCA_LFM_FILM_GENERATOR_INPUT_0));
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "pv_ypu_mem_copy_to failed with %s",
            pv_status_to_string(status));

    pv_ypu_mem_t *m1 = pv_ypu_mem_alloc(
            ypu,
            T * sizeof(int32_t),
            PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m1 != NULL, "Failed to allocate m1");
    if (m1 == NULL) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    float *buffer_bucket = pv_ypu_mem_get_host_view(ypu, m1, false);
    int32_t i_bucket = 0;
    for (int32_t i_d = 0; i_d < N; ++i_d) {
        int32_t duration = TEST_ORCA_LFM_FILM_GENERATOR_INPUT_1[i_d];
        for (int32_t j = 0; j < duration; ++j) {
            pv_test_true(i_bucket < T, "`i_bucket < T` failed.");
            buffer_bucket[i_bucket++] = ((float) i_d);
        }
    }
    pv_test_true(i_bucket == T, "`i_bucket == T` failed.");
    pv_ypu_mem_release_host_view(ypu, m1, true);

    pv_ypu_mem_t *m2 = pv_ypu_mem_alloc(
            ypu,
            T * dimension * sizeof(float),
            PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m2 != NULL, "Failed to allocate m2");
    if (m2 == NULL) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    status = pv_orca_lfm_film_generator_forward(
            ypu,
            orca_lfm_film_generator_object,
            T,
            m0,
            m1,
            m2);

    if (test_accuracy) {
        pv_test_true(status == PV_STATUS_SUCCESS, "pv_orca_lfm_film_generator_forward() returns failed status");

        float *buffer_output = pv_ypu_mem_get_host_view(ypu, m2, true);
        pv_test_close_float_array(
                buffer_output,
                TEST_ORCA_LFM_FILM_GENERATOR_TARGET,
                T * dimension,
                TEST_TOLERANCE_PERCENT,
                TEST_EPSILON,
                "failed to match target");
        pv_ypu_mem_release_host_view(ypu, m1, false);
    }

    pv_ypu_mem_free(ypu, m0);
    pv_ypu_mem_free(ypu, m1);
    pv_ypu_mem_free(ypu, m2);

    return status;
}

static void test_pv_orca_lfm_film_generator_forward(void) {
    test_pv_orca_lfm_film_generator_forward_inner(true);
}

static const pv_test_case_t PV_ORCA_LFM_FILM_GENERATOR_TEST_CASES[] = {
        {"orca_lfm_film_generator forward", test_pv_orca_lfm_film_generator_forward},
};

const pv_test_suite_t PV_ORCA_LFM_FILM_GENERATOR_TEST_SUITE = {
        .name = "orca_lfm_film_generator",
        .setup = test_pv_orca_lfm_film_generator_setup,
        .teardown = test_pv_orca_lfm_film_generator_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_ORCA_LFM_FILM_GENERATOR_TEST_CASES),
        .test_cases = PV_ORCA_LFM_FILM_GENERATOR_TEST_CASES,
};
