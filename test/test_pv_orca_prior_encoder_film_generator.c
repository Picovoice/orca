#include <stdlib.h>
#include <string.h>

#include "core/picovoice.h"
#include "core/pv_type.h"
#include "test/pv_test.h"

#include "test_data/test_pv_orca_prior_encoder_film_generator_data.c"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

#ifdef __ORCA_FLOAT_MODE__

static const float TEST_TOLERANCE_PERCENT = 0.0003f;
static const float TEST_EPSILON = 0.005f;

#else

static const float TEST_TOLERANCE_PERCENT = 0.05f;
static const float TEST_EPSILON = 0.075f;

#endif

static pv_ypu_t *ypu = NULL;
static pv_orca_prior_encoder_film_generator_t *orca_prior_encoder_film_generator_object = NULL;

static pv_status_t test_pv_orca_prior_encoder_film_generator_setup(void) {
    pv_status_t status = pv_ypu_init_cpu(1, &ypu);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_orca_prior_encoder_film_generator_init(
            ypu,
            &PRIOR_ENCODER_FILM_GENERATOR_PARAM,
            &orca_prior_encoder_film_generator_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_orca_prior_encoder_film_generator_teardown(void) {
    pv_orca_prior_encoder_film_generator_delete(ypu, orca_prior_encoder_film_generator_object);
    pv_ypu_delete(ypu);
}

static void test_pv_orca_prior_encoder_film_generator_forward(void) {
    const int32_t dimension = PRIOR_ENCODER_FILM_GENERATOR_PARAM.dimension;

    pv_ypu_mem_t *m0 = pv_ypu_mem_alloc(
            ypu,
            TEST_ORCA_PRIOR_ENCODER_FILM_GENERATOR_SEQUENCE_LENGTH * dimension * sizeof(float),
            PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m0 != NULL, "Failed to allocate m0");
    if (m0 == NULL) {
        return;
    }

    pv_status_t status = pv_orca_prior_encoder_film_generator_forward(
            ypu,
            orca_prior_encoder_film_generator_object,
            TEST_ORCA_PRIOR_ENCODER_FILM_GENERATOR_SEQUENCE_LENGTH,
            TEST_ORCA_PRIOR_ENCODER_FILM_GENERATOR_INPUT,
            m0);

    pv_test_true(status == PV_STATUS_SUCCESS, "pv_orca_prior_encoder_film_generator_forward() returns failed status");

    float *buffer_output = pv_ypu_mem_get_host_view(ypu, m0, true);
    pv_test_close_float_array(
            buffer_output,
            TEST_ORCA_PRIOR_ENCODER_FILM_GENERATOR_TARGET,
            TEST_ORCA_PRIOR_ENCODER_FILM_GENERATOR_SEQUENCE_LENGTH * dimension,
            TEST_TOLERANCE_PERCENT,
            TEST_EPSILON,
            "failed to match target");

    pv_ypu_mem_release_host_view(ypu, m0, false);
    pv_ypu_mem_free(ypu, m0);
}


static const pv_test_case_t PV_ORCA_PRIOR_ENCODER_FILM_GENERATOR_TEST_CASES[] = {
        {"orca_prior_encoder_film_generator forward", test_pv_orca_prior_encoder_film_generator_forward},
};


const pv_test_suite_t PV_ORCA_PRIOR_ENCODER_FILM_GENERATOR_TEST_SUITE = {
        .name = "orca_prior_encoder_film_generator",
        .setup = test_pv_orca_prior_encoder_film_generator_setup,
        .teardown = test_pv_orca_prior_encoder_film_generator_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_ORCA_PRIOR_ENCODER_FILM_GENERATOR_TEST_CASES),
        .test_cases = PV_ORCA_PRIOR_ENCODER_FILM_GENERATOR_TEST_CASES,
};
