#include <math.h>
#include <stdlib.h>

#include "core/picovoice.h"
#include "core/pv_type.h"
#include "test/pv_test.h"
#include "orca/pv_orca_istft.h"

#include "test_data/test_pv_orca_vocoder_data.c"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

static pv_orca_vocoder_t *orca_vocoder_object = NULL;

static pv_status_t test_pv_orca_vocoder_setup(void) {
    pv_status_t status = pv_orca_vocoder_init(&DEC_PARAM, &orca_vocoder_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_orca_vocoder_teardown(void) {
    pv_orca_vocoder_delete(orca_vocoder_object);
    orca_vocoder_object = NULL;
}

static void test_pv_orca_vocoder_forward(void) {
    pv_test_true(orca_vocoder_object != NULL, "failed to create tmp file");

    const int32_t num_samples = TEST_ORCA_VOCODER_SEQUENCE_LENGTH * PV_ORCA_WINDOW_SHIFT;

    int16_t *pcm = calloc(TEST_ORCA_VOCODER_SEQUENCE_LENGTH * PV_ORCA_WINDOW_SHIFT, sizeof(int16_t));
    if (!pcm) {
        return;
    }

    pv_orca_vocoder_forward(orca_vocoder_object, TEST_ORCA_VOCODER_SEQUENCE_LENGTH, TEST_ORCA_VOCODER_INPUT, pcm);

    float max_difference = 100;
    int32_t start = PV_ORCA_ISTFT_LINEAR_FADE_IN_SAMPLES;
    int32_t end = num_samples - PV_ORCA_ISTFT_LINEAR_FADE_OUT_SAMPLES;
    for (int32_t i = start; i < end; i++) {
        float target = (float) TEST_ORCA_VOCODER_TARGET[i] * DEC_PARAM.pcm_normalization_factor;
        float sample_difference = fabsf((float) pcm[i] - target);
        pv_test_true(
                sample_difference < max_difference,
                "pcm mismatch at index %d (difference > %.0f), got %d, expected %d",
                i,
                max_difference,
                pcm[i],
                TEST_ORCA_VOCODER_TARGET[i]);
    }

    free(pcm);
}

#ifdef __PV_MOCKS__

static size_t pv_fread_ret_one_read_zero(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    (void) stream;

    memset(ptr, 0, size * nmemb);
    return 1;
}

static size_t pv_fread_ret_zero(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    (void) ptr;
    (void) size;
    (void) nmemb;
    (void) stream;

    return 0;
}

static void test_pv_orca_vocoder_param_load_failure_helper(pv_status_t expected) {
    FILE *dummy_file = malloc(sizeof(FILE *));
    pv_test_true(dummy_file != NULL, "failed to create dummy file");
    if (!dummy_file) {
        return;
    }

    pv_orca_vocoder_param_t *param = NULL;
    pv_status_t status = pv_orca_vocoder_param_load(dummy_file, &param);
    pv_test_true(
            status == expected,
            "param load error, got `%s` expected `%s`",
            pv_status_to_string(status),
            pv_status_to_string(expected));
    free(dummy_file);
    if (param) {
        pv_orca_vocoder_param_delete(param);
    }
}

static void test_pv_orca_vocoder_param_load_failure_1(void) {
    PV_SET_MOCK_RETURN_VAL(pv_cnn_param_load, PV_STATUS_INVALID_ARGUMENT)

    test_pv_orca_vocoder_param_load_failure_helper(PV_STATUS_INVALID_ARGUMENT);
}

static void test_pv_orca_vocoder_param_load_failure_2(void) {
    PV_SET_MOCK_RETURN_VAL(pv_cnn_param_load, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_convnext_transposed_param_load, PV_STATUS_INVALID_ARGUMENT)

    test_pv_orca_vocoder_param_load_failure_helper(PV_STATUS_INVALID_ARGUMENT);
}

static void test_pv_orca_vocoder_param_load_failure_3(void) {
    PV_SET_MOCK_RETURN_VAL(pv_cnn_param_load, PV_STATUS_SUCCESS)
    pv_status_t custom_rets[] = {
            PV_STATUS_SUCCESS,
            PV_STATUS_INVALID_ARGUMENT,
    };
    PV_SET_MOCK_RETURN_SEQ(pv_convnext_transposed_param_load, custom_rets);

    test_pv_orca_vocoder_param_load_failure_helper(PV_STATUS_INVALID_ARGUMENT);
}

static void test_pv_orca_vocoder_param_load_failure_4(void) {
    PV_SET_MOCK_RETURN_VAL(pv_cnn_param_load, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_convnext_transposed_param_load, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_vocos_backbone_param_load, PV_STATUS_INVALID_ARGUMENT)

    test_pv_orca_vocoder_param_load_failure_helper(PV_STATUS_INVALID_ARGUMENT);
}

static void test_pv_orca_vocoder_param_load_failure_5(void) {
    PV_SET_MOCK_RETURN_VAL(pv_cnn_param_load, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_convnext_transposed_param_load, PV_STATUS_SUCCESS)
    pv_status_t custom_rets[] = {
            PV_STATUS_SUCCESS,
            PV_STATUS_INVALID_ARGUMENT,
    };
    PV_SET_MOCK_RETURN_SEQ(pv_vocos_backbone_param_load, custom_rets);

    test_pv_orca_vocoder_param_load_failure_helper(PV_STATUS_INVALID_ARGUMENT);
}

static void test_pv_orca_vocoder_param_load_failure_6(void) {
    PV_SET_MOCK_RETURN_VAL(pv_cnn_param_load, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_convnext_transposed_param_load, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_vocos_backbone_param_load, PV_STATUS_SUCCESS)
    pv_status_t custom_rets[] = {
            PV_STATUS_SUCCESS,
            PV_STATUS_INVALID_ARGUMENT,
    };
    PV_SET_MOCK_RETURN_SEQ(pv_cnn_param_load, custom_rets);

    test_pv_orca_vocoder_param_load_failure_helper(PV_STATUS_INVALID_ARGUMENT);
}

static void test_pv_orca_vocoder_param_load_failure_7(void) {
    PV_SET_MOCK_RETURN_VAL(pv_cnn_param_load, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_convnext_transposed_param_load, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_vocos_backbone_param_load, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_cnn_param_load, PV_STATUS_SUCCESS)
    PV_SET_MOCK_CUSTOM_FUNC(pv_fread, pv_fread_ret_zero)

    test_pv_orca_vocoder_param_load_failure_helper(PV_STATUS_IO_ERROR);
}

static void test_pv_orca_vocoder_param_load_failure_8(void) {
    PV_SET_MOCK_RETURN_VAL(pv_cnn_param_load, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_convnext_transposed_param_load, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_vocos_backbone_param_load, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_cnn_param_load, PV_STATUS_SUCCESS)
    PV_SET_MOCK_CUSTOM_FUNC(pv_fread, pv_fread_ret_one_read_zero)

    test_pv_orca_vocoder_param_load_failure_helper(PV_STATUS_INVALID_ARGUMENT);
}

#endif

static const pv_test_case_t PV_ORCA_VOCODER_TEST_CASES[] = {
        {"orca_vocoder forward", test_pv_orca_vocoder_forward},

#ifdef __PV_MOCKS__

        {"param load failure 1", test_pv_orca_vocoder_param_load_failure_1},
        {"param load failure 2", test_pv_orca_vocoder_param_load_failure_2},
        {"param load failure 3", test_pv_orca_vocoder_param_load_failure_3},
        {"param load failure 4", test_pv_orca_vocoder_param_load_failure_4},
        {"param load failure 5", test_pv_orca_vocoder_param_load_failure_5},
        {"param load failure 6", test_pv_orca_vocoder_param_load_failure_6},
        {"param load failure 7", test_pv_orca_vocoder_param_load_failure_7},
        {"param load failure 8", test_pv_orca_vocoder_param_load_failure_8},

#endif

};

const pv_test_suite_t PV_ORCA_VOCODER_TEST_SUITE = {
        .name = "orca_vocoder",
        .setup = test_pv_orca_vocoder_setup,
        .teardown = test_pv_orca_vocoder_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_ORCA_VOCODER_TEST_CASES),
        .test_cases = PV_ORCA_VOCODER_TEST_CASES,
};
