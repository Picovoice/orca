#include <stdlib.h>

#include "core/picovoice.h"
#include "core/pv_type.h"
#include "test/pv_test.h"

#include "test_data/test_pv_vocos_backbone_data.c"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

static pv_vocos_backbone_t *vocos_backbone_object = NULL;

static pv_status_t test_pv_vocos_backbone_setup(void) {
    pv_status_t status = pv_vocos_backbone_init(&DEC_BACKBONE_0_PARAM, &vocos_backbone_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_vocos_backbone_teardown(void) {
    pv_vocos_backbone_delete(vocos_backbone_object);
    vocos_backbone_object = NULL;
}

static void test_pv_vocos_backbone_forward(void) {
    pv_test_true(vocos_backbone_object != NULL, "failed to create tmp file");

    int32_t num_channels = pv_vocos_backbone_output_channels(vocos_backbone_object);
    float *buffer = calloc(TEST_VOCOS_BACKBONE_SEQUENCE_LENGTH, num_channels * sizeof(float));

    pv_vocos_backbone_forward(
            vocos_backbone_object,
            TEST_VOCOS_BACKBONE_SEQUENCE_LENGTH,
            TEST_VOCOS_BACKBONE_INPUT,
            buffer);
    pv_test_close_float_array(
            buffer,
            TEST_VOCOS_BACKBONE_TARGET,
            TEST_VOCOS_BACKBONE_SEQUENCE_LENGTH * num_channels,
            0.02f,
            0.001f,
            "failed to forward vocos_backbone");

    free(buffer);
}

#ifdef __PV_MOCKS__

static size_t pv_fread_valid(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    (void) stream;
    (void) nmemb;

    int32_t value = 1;
    memcpy(ptr, &value, size);

    return 1;
}

static size_t pv_fread_invalid_set_value(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    (void) stream;
    (void) nmemb;

    int32_t value = -1;
    memcpy(ptr, &value, size);

    return 1;
}

static size_t pv_fread_invalid_ret_value(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    (void) stream;
    (void) size;
    (void) nmemb;
    (void) ptr;

    return 99;
}

static void *calloc_return_null(size_t arg0, size_t arg1) {
    (void) arg0;
    (void) arg1;
    return NULL;
}

static void test_pv_vocos_backbone_param_load_failure_helper(pv_status_t expected) {
    FILE *dummy_file = malloc(sizeof(FILE *));
    pv_test_true(dummy_file != NULL, "failed create dummy file");
    if (!dummy_file) {
        return;
    }

    pv_vocos_backbone_param_t *param = NULL;
    pv_status_t status = pv_vocos_backbone_param_load(dummy_file, &param);
    pv_test_true(
            status == expected,
            "param load error, got `%s` expected `%s`",
            pv_status_to_string(status),
            pv_status_to_string(expected));
    free(dummy_file);
}

static void test_pv_vocos_backbone_param_load_failure_1(void) {
    PV_SET_MOCK_RETURN_VAL(pv_layer_norm_param_load, PV_STATUS_INVALID_ARGUMENT)

    test_pv_vocos_backbone_param_load_failure_helper(PV_STATUS_INVALID_ARGUMENT);
}

static void test_pv_vocos_backbone_param_load_failure_2(void) {
    pv_status_t custom_rets[] = {
            PV_STATUS_SUCCESS,
            PV_STATUS_INVALID_ARGUMENT,
    };
    PV_SET_MOCK_RETURN_SEQ(pv_layer_norm_param_load, custom_rets);

    test_pv_vocos_backbone_param_load_failure_helper(PV_STATUS_INVALID_ARGUMENT);
}

static void test_pv_vocos_backbone_param_load_failure_3(void) {
    PV_SET_MOCK_CUSTOM_FUNC(pv_fread, pv_fread_invalid_ret_value)

    test_pv_vocos_backbone_param_load_failure_helper(PV_STATUS_IO_ERROR);
}

static void test_pv_vocos_backbone_param_load_failure_4(void) {
    PV_SET_MOCK_CUSTOM_FUNC(pv_fread, pv_fread_invalid_set_value)

    test_pv_vocos_backbone_param_load_failure_helper(PV_STATUS_INVALID_ARGUMENT);
}

static void test_pv_vocos_backbone_param_load_failure_5(void) {
    PV_SET_MOCK_CUSTOM_FUNC(pv_fread, pv_fread_valid)
    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_real,
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs)

    test_pv_vocos_backbone_param_load_failure_helper(PV_STATUS_OUT_OF_MEMORY);
}

static void test_pv_vocos_backbone_param_load_failure_6(void) {
    PV_SET_MOCK_CUSTOM_FUNC(pv_fread, pv_fread_valid)
    PV_SET_MOCK_RETURN_VAL(pv_convnext_param_load, PV_STATUS_INVALID_ARGUMENT)

    test_pv_vocos_backbone_param_load_failure_helper(PV_STATUS_INVALID_ARGUMENT);
}

#endif

static const pv_test_case_t PV_VOCOS_BACKBONE_TEST_CASES[] = {
        {"vocos_backbone forward", test_pv_vocos_backbone_forward},

#ifdef __PV_MOCKS__

        {"vocos_backbone param load failure 1", test_pv_vocos_backbone_param_load_failure_1},
        {"vocos_backbone param load failure 2", test_pv_vocos_backbone_param_load_failure_2},
        {"vocos_backbone param load failure 3", test_pv_vocos_backbone_param_load_failure_3},
        {"vocos_backbone param load failure 4", test_pv_vocos_backbone_param_load_failure_4},
        {"vocos_backbone param load failure 5", test_pv_vocos_backbone_param_load_failure_5},
        {"vocos_backbone param load failure 6", test_pv_vocos_backbone_param_load_failure_6},

#endif

};

const pv_test_suite_t PV_VOCOS_BACKBONE_TEST_SUITE = {
        .name = "vocos_backbone",
        .setup = test_pv_vocos_backbone_setup,
        .teardown = test_pv_vocos_backbone_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_VOCOS_BACKBONE_TEST_CASES),
        .test_cases = PV_VOCOS_BACKBONE_TEST_CASES,
};
