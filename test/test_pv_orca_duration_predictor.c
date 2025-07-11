#include <stdlib.h>

#include "core/picovoice.h"
#include "core/pv_type.h"
#include "test/pv_test.h"

#include "test_data/test_pv_orca_duration_predictor_data.c"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

static pv_orca_duration_predictor_t *orca_duration_predictor_object = NULL;

static pv_status_t test_pv_orca_duration_predictor_setup(void) {
    pv_status_t status = pv_orca_duration_predictor_init(&DP_PARAM, &orca_duration_predictor_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_orca_duration_predictor_teardown(void) {
    pv_orca_duration_predictor_delete(orca_duration_predictor_object);
    orca_duration_predictor_object = NULL;
}


static void test_pv_orca_duration_predictor_forward(void) {
    pv_test_true(orca_duration_predictor_object != NULL, "failed to create tmp file");

    int32_t *buffer = calloc(TEST_ORCA_DURATION_PREDICTOR_SEQUENCE_LENGTH, sizeof(int32_t));

    pv_orca_duration_predictor_forward(
            orca_duration_predictor_object,
            1.0f,
            TEST_ORCA_DURATION_PREDICTOR_SEQUENCE_LENGTH,
            TEST_ORCA_DURATION_PREDICTOR_INPUT,
            buffer);
    pv_test_close_int32_array(
            buffer,
            TEST_ORCA_DURATION_PREDICTOR_TARGET,
            TEST_ORCA_DURATION_PREDICTOR_SEQUENCE_LENGTH,
            0.02f,
            0,
            "failed to forward orca_duration_predictor");
    free(buffer);
}

#ifdef __PV_MOCKS__

static void test_pv_duration_predictor_param_load_failure_helper(pv_status_t expected) {
    FILE *dummy_file = malloc(sizeof(FILE *));
    pv_test_true(dummy_file != NULL, "failed create dummy file");
    if (!dummy_file) {
        return;
    }

    pv_orca_duration_predictor_param_t *param = NULL;
    pv_status_t status = pv_orca_duration_predictor_param_load(dummy_file, &param);
    pv_test_true(
            status == expected,
            "param load error, got `%s` expected `%s`",
            pv_status_to_string(status),
            pv_status_to_string(expected));
    free(dummy_file);
}

static void test_pv_duration_predictor_param_load_failure_1(void) {
    PV_SET_MOCK_RETURN_VAL(pv_cnn_param_load, PV_STATUS_INVALID_ARGUMENT)

    test_pv_duration_predictor_param_load_failure_helper(PV_STATUS_INVALID_ARGUMENT);
}

static void test_pv_duration_predictor_param_load_failure_2(void) {
    pv_status_t custom_rets[] = {
            PV_STATUS_SUCCESS,
            PV_STATUS_INVALID_ARGUMENT,
    };
    PV_SET_MOCK_RETURN_SEQ(pv_cnn_param_load, custom_rets);

    test_pv_duration_predictor_param_load_failure_helper(PV_STATUS_INVALID_ARGUMENT);
}

static void test_pv_duration_predictor_param_load_failure_3(void) {
    PV_SET_MOCK_RETURN_VAL(pv_cnn_param_load, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_layer_norm_param_load, PV_STATUS_OUT_OF_MEMORY)

    test_pv_duration_predictor_param_load_failure_helper(PV_STATUS_OUT_OF_MEMORY);
}

static void test_pv_duration_predictor_param_load_failure_4(void) {
    PV_SET_MOCK_RETURN_VAL(pv_cnn_param_load, PV_STATUS_SUCCESS)
    pv_status_t custom_rets[] = {
            PV_STATUS_SUCCESS,
            PV_STATUS_INVALID_ARGUMENT,
    };
    PV_SET_MOCK_RETURN_SEQ(pv_layer_norm_param_load, custom_rets);

    test_pv_duration_predictor_param_load_failure_helper(PV_STATUS_INVALID_ARGUMENT);
}

static void test_pv_duration_predictor_param_load_failure_5(void) {
    PV_SET_MOCK_RETURN_VAL(pv_layer_norm_param_load, PV_STATUS_SUCCESS)    
    pv_status_t custom_rets[] = {
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_INVALID_ARGUMENT,
    };
    PV_SET_MOCK_RETURN_SEQ(pv_cnn_param_load, custom_rets);

    test_pv_duration_predictor_param_load_failure_helper(PV_STATUS_INVALID_ARGUMENT);
}

static void test_pv_duration_predictor_param_load_failure_6(void) {
    PV_SET_MOCK_RETURN_VAL(pv_cnn_param_load, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_layer_norm_param_load, PV_STATUS_SUCCESS)    
    pv_status_t custom_rets[] = {
            PV_STATUS_INVALID_ARGUMENT,
    };
    PV_SET_MOCK_RETURN_SEQ(pv_affine_param_load, custom_rets);

    test_pv_duration_predictor_param_load_failure_helper(PV_STATUS_INVALID_ARGUMENT);
}

static void test_pv_duration_predictor_param_load_failure_7(void) {
    PV_SET_MOCK_RETURN_VAL(pv_cnn_param_load, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_layer_norm_param_load, PV_STATUS_SUCCESS)    
    pv_status_t custom_rets[] = {
            PV_STATUS_SUCCESS,
            PV_STATUS_INVALID_ARGUMENT,
    };
    PV_SET_MOCK_RETURN_SEQ(pv_affine_param_load, custom_rets);

    test_pv_duration_predictor_param_load_failure_helper(PV_STATUS_INVALID_ARGUMENT);
}

#endif

static const pv_test_case_t PV_ORCA_DURATION_PREDICTOR_TEST_CASES[] = {
        {"orca_duration_predictor forward", test_pv_orca_duration_predictor_forward},

#ifdef __PV_MOCKS__

    {"param_load_failure_1", test_pv_duration_predictor_param_load_failure_1},
    {"param_load_failure_2", test_pv_duration_predictor_param_load_failure_2},
    {"param_load_failure_3", test_pv_duration_predictor_param_load_failure_3},
    {"param_load_failure_4", test_pv_duration_predictor_param_load_failure_4},
    {"param_load_failure_5", test_pv_duration_predictor_param_load_failure_5},
    {"param_load_failure_6", test_pv_duration_predictor_param_load_failure_6},
    {"param_load_failure_7", test_pv_duration_predictor_param_load_failure_7},

#endif

};

const pv_test_suite_t PV_ORCA_DURATION_PREDICTOR_TEST_SUITE = {
        .name = "orca_duration_predictor",
        .setup = test_pv_orca_duration_predictor_setup,
        .teardown = test_pv_orca_duration_predictor_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_ORCA_DURATION_PREDICTOR_TEST_CASES),
        .test_cases = PV_ORCA_DURATION_PREDICTOR_TEST_CASES,
};
