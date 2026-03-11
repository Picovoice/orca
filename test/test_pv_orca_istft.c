#include <stdlib.h>
#include <string.h>

#include "test/pv_test.h"

#include "orca/pv_orca_fft.h"
#include "orca/pv_orca_istft.h"
#include "orca/pv_orca_pqmf.h"
#include "orca/pv_orca_synthesizer.h"
#include "test_data/test_pv_orca_fft_data.c"
#include "test_data/test_pv_orca_istft_data.c"
#include "test_data/test_pv_orca_mb_istft_data.c"
#include "test_data/test_pv_orca_pqmf_data.c"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

static pv_orca_istft_t *orca_istft_object = NULL;

static pv_buffer_t *buffer_spec_object = NULL;
static pv_buffer_t *buffer_spec_complex_object = NULL;

static pv_status_t test_pv_orca_istft_setup(void) {
    pv_status_t status = pv_orca_istft_init(
            PV_ORCA_VOCODER_WINDOW_LENGTH,
            PV_ORCA_VOCODER_WINDOW_SHIFT,
            PV_ORCA_VOCODER_NUM_FFT,
            PV_ORCA_VOCODER_NUM_SUBBANDS,
            1.0f,
            &orca_istft_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_orca_istft_teardown(void) {
    pv_buffer_delete(buffer_spec_object);
    pv_buffer_delete(buffer_spec_complex_object);
    pv_orca_istft_delete(orca_istft_object);
    orca_istft_object = NULL;
}

static void test_pv_orca_istft_preprocess(void) {
    const int32_t n = 3;
    const int32_t num_fft = 4;
    float spec[] = {
            /*   <--- real --->   |   <--- imag --->     */
            1.95f, -1.05f, -1.19f, -2.21f, 0.61f, 0.99f,
            1.01f, -1.62f, 1.68f, 0.29f, 1.73f, -0.33f,
            1.20f, -0.61f, -1.68f, -0.57f, 1.26f, -0.68f,
    };
    float y_target[] = {
            -1.0482524633f, 0.f, 0.1434129030f, 0.1002339795f, 0.0417307764f, 0.f,
            0.6577388644f, 0.f, -0.0156866405f, 0.0976980254f, 1.2690107822f, 0.f,
            0.6988024712f, 0.f, 0.0830829442f, 0.2586595416f, 0.0362298302f, 0.f,
    };

    float y[(num_fft + 2) * n];
    pv_orca_istft_preprocess_fft(num_fft, n, spec, y);
    pv_test_close_float_array(
            y,
            y_target,
            (num_fft + 2) * n,
            0.002f,
            0.001f,
            "y mismatch");
}

static void test_pv_orca_ifft(void) {
    pv_test_true(orca_istft_object != NULL, "failed to create orca_istft object");

    const float *input = TEST_ORCA_FFT_INPUT;
    const float *target = TEST_ORCA_FFT_TARGET;
    int32_t sequence_length = 1;
    int32_t num_fft = TEST_ORCA_FFT_SEQUENCE_LENGTH_1;

    float full_spec[num_fft + 2];
    pv_orca_istft_preprocess_fft(num_fft, sequence_length, input, full_spec);

    float *y = malloc(num_fft * sizeof(float));
    pv_orca_fft_inverse(full_spec, y);
    pv_test_close_float_array(
            y,
            target,
            num_fft,
            0.0002f,
            0.0001f,
            "y mismatch");
    free(y);
}

static void test_pv_orca_pqmf(void) {
    pv_test_true(orca_istft_object != NULL, "failed to create orca_istft object");

    const float *input = TEST_ORCA_PQMF_INPUT;
    const float *target = TEST_ORCA_PQMF_TARGET;

    const int32_t num_subbands = 4;
    const int32_t input_length = TEST_ORCA_PQMF_SEQUENCE_LENGTH;

    const int32_t output_length = num_subbands * input_length;
    float *y = malloc(output_length * sizeof(float));

    pv_orca_pqmf_synthesis(num_subbands, input_length, input, y);
    pv_test_close_float_array(
            y,
            target,
            output_length,
            0.005f,
            0.005f,
            "y mismatch");

    free(y);
}

static void test_pv_orca_istft_forward(void) {
    pv_test_true(orca_istft_object != NULL, "failed to create orca_istft object");

    float *buffer_spec_complex = malloc((PV_ORCA_VOCODER_NUM_FFT + 2) * TEST_ORCA_ISTFT_SEQUENCE_LENGTH * sizeof(float));
    pv_orca_istft_preprocess_fft(
            PV_ORCA_VOCODER_NUM_FFT,
            TEST_ORCA_ISTFT_SEQUENCE_LENGTH,
            TEST_ORCA_ISTFT_INPUT,
            buffer_spec_complex);

    float *pcm = malloc(PV_ORCA_VOCODER_WINDOW_SHIFT * TEST_ORCA_ISTFT_SEQUENCE_LENGTH * sizeof(float));
    pv_orca_istft_forward(orca_istft_object, TEST_ORCA_ISTFT_SEQUENCE_LENGTH, buffer_spec_complex, pcm);
    pv_test_close_float_array(
            pcm,
            TEST_ORCA_ISTFT_TARGET,
            PV_ORCA_VOCODER_WINDOW_SHIFT * TEST_ORCA_ISTFT_SEQUENCE_LENGTH,
            0.1f,
            0.05f,
            "pcm mismatch");

    free(buffer_spec_complex);
    free(pcm);
}

static void test_pv_orca_istft_multiband_forward(void) {
    pv_test_true(orca_istft_object != NULL, "failed to create orca_istft object");

    int32_t num_samples = PV_ORCA_VOCODER_WINDOW_LENGTH * TEST_ORCA_MB_ISTFT_SEQUENCE_LENGTH;
    int16_t *pcm = malloc(num_samples * sizeof(int16_t));
    pv_orca_istft_multiband_forward(
            orca_istft_object,
            TEST_ORCA_MB_ISTFT_SEQUENCE_LENGTH,
            TEST_ORCA_MB_ISTFT_INPUT,
            pcm);
    pv_test_close_int16_array(
            &pcm[PV_ORCA_ISTFT_LINEAR_FADE_IN_SAMPLES],
            &TEST_ORCA_MB_ISTFT_TARGET[PV_ORCA_ISTFT_LINEAR_FADE_IN_SAMPLES],
            num_samples - PV_ORCA_ISTFT_LINEAR_FADE_IN_SAMPLES - PV_ORCA_ISTFT_LINEAR_FADE_OUT_SAMPLES,
            0.4f,
            500,
            "pcm mismatch");

    free(pcm);
}

static const pv_test_case_t PV_ORCA_ISTFT_TEST_CASES[] = {
        {"preprocess",      test_pv_orca_istft_preprocess},
        {"ifft",            test_pv_orca_ifft},
        {"pqmf",            test_pv_orca_pqmf},
        {"istft",           test_pv_orca_istft_forward},
        {"multiband_istft", test_pv_orca_istft_multiband_forward},
};

const pv_test_suite_t PV_ORCA_ISTFT_TEST_SUITE = {
        .name = "orca_istft",
        .setup = test_pv_orca_istft_setup,
        .teardown = test_pv_orca_istft_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_ORCA_ISTFT_TEST_CASES),
        .test_cases = PV_ORCA_ISTFT_TEST_CASES,
};
