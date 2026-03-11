#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "core/picovoice.h"
#include "core/pv_type.h"
#include "orca/pv_orca_istft.h"
#include "orca/pv_orca_synthesizer.h"
#include "test/pv_test.h"

#include "test_data/test_pv_orca_vocoder_data.c"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

static pv_ypu_t *ypu = NULL;
static pv_orca_vocoder_t *orca_vocoder_object = NULL;

static pv_status_t test_pv_orca_vocoder_setup(void) {
    pv_status_t status = pv_ypu_init_cpu(1, &ypu);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_orca_vocoder_init(
            ypu,
            &DEC_PARAM,
            &orca_vocoder_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_orca_vocoder_teardown(void) {
    pv_orca_vocoder_delete(ypu, orca_vocoder_object);
    pv_ypu_delete(ypu);
}

static void test_pv_orca_vocoder_forward(void) {
    pv_ypu_mem_t *m0 = pv_ypu_mem_alloc(
            ypu,
            sizeof(TEST_ORCA_VOCODER_INPUT),
            PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m0 != NULL, "Failed to allocate m0");
    if (m0 == NULL) {
        return;
    }

    pv_status_t status = pv_ypu_mem_copy_to(
            ypu,
            m0,
            TEST_ORCA_VOCODER_INPUT,
            0,
            sizeof(TEST_ORCA_VOCODER_INPUT));
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "pv_ypu_mem_copy_to failed with %s",
            pv_status_to_string(status));

    int16_t *buffer_output = calloc(
            TEST_ORCA_VOCODER_SEQUENCE_LENGTH * PV_ORCA_WINDOW_SHIFT,
            sizeof(int16_t));

    status = pv_orca_vocoder_forward(
            ypu,
            orca_vocoder_object,
            TEST_ORCA_VOCODER_SEQUENCE_LENGTH,
            m0,
            buffer_output,
            0);
    pv_ypu_mem_free(ypu, m0);
    pv_test_true(status == PV_STATUS_SUCCESS, "pv_orca_vocoder_forward() returns failed status");

#ifdef __ORCA_FLOAT_MODE__

    float max_difference = 7;

#else

    float max_difference = 1030;

#endif

    int32_t start = PV_ORCA_ISTFT_LINEAR_FADE_IN_SAMPLES;
    int32_t end = TEST_ORCA_VOCODER_SEQUENCE_LENGTH * PV_ORCA_WINDOW_SHIFT - PV_ORCA_ISTFT_LINEAR_FADE_OUT_SAMPLES;
    for (int32_t i = start; i < end; i++) {
        float target = (float) TEST_ORCA_VOCODER_TARGET[i] * DEC_PARAM.pcm_normalization_factor;
        float sample_difference = fabsf((float) buffer_output[i] - target);
        pv_test_true(
                sample_difference <= max_difference,
                "pcm mismatch at index %d (difference > %.0f), got %d, expected %d",
                i,
                max_difference,
                buffer_output[i],
                TEST_ORCA_VOCODER_TARGET[i]);
    }

    free(buffer_output);
}

static const pv_test_case_t PV_ORCA_VOCODER_TEST_CASES[] = {
        {"orca_vocoder forward", test_pv_orca_vocoder_forward},
};

const pv_test_suite_t PV_ORCA_VOCODER_TEST_SUITE = {
        .name = "orca_vocoder",
        .setup = test_pv_orca_vocoder_setup,
        .teardown = test_pv_orca_vocoder_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_ORCA_VOCODER_TEST_CASES),
        .test_cases = PV_ORCA_VOCODER_TEST_CASES,
};
