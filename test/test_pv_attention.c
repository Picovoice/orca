#include <stdlib.h>

#include "core/picovoice.h"
#include "core/pv_type.h"
#include "test/pv_test.h"

#include "test_data/test_pv_attention_data.c"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

static pv_attention_t *attention_object = NULL;
static pv_buffer_t *buffer_text_encoder_1_object = NULL;
static pv_buffer_t *buffer_text_encoder_2_object = NULL;
static pv_buffer_t *buffer_attention_score_object = NULL;

static pv_status_t test_pv_attention_setup(void) {
    int32_t num_hidden_channels = TEST_ATTENTION_CONV_K_PARAM.output_channels;
    pv_status_t status = pv_buffer_init(num_hidden_channels, &(buffer_text_encoder_1_object));
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_buffer_init(num_hidden_channels, &(buffer_text_encoder_2_object));
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    int32_t num_heads = TEST_ATTENTION_PARAM.num_heads;
    status = pv_buffer_init(num_heads, &(buffer_attention_score_object));
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_attention_init(
            &TEST_ATTENTION_PARAM,
            buffer_text_encoder_1_object,
            buffer_text_encoder_2_object,
            buffer_attention_score_object,
            &attention_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_attention_teardown(void) {
    pv_buffer_delete(buffer_attention_score_object);
    pv_buffer_delete(buffer_text_encoder_2_object);
    pv_buffer_delete(buffer_text_encoder_1_object);
    pv_attention_delete(attention_object);
    attention_object = NULL;
}

static void test_pv_attention_forward(void) {
    pv_test_true(attention_object != NULL, "failed to create tmp file");

    int32_t num_channels = PV_ARRAY_LEN(TEST_ATTENTION_TARGET) / TEST_ATTENTION_SEQUENCE_LENGTH;
    float *buffer = calloc(TEST_ATTENTION_SEQUENCE_LENGTH, num_channels * sizeof(float));

    pv_attention_forward(
            attention_object,
            TEST_ATTENTION_SEQUENCE_LENGTH,
            TEST_ATTENTION_INPUT,
            TEST_ATTENTION_INPUT,
            buffer);
    pv_test_close_float_array(
            buffer,
            TEST_ATTENTION_TARGET,
            TEST_ATTENTION_SEQUENCE_LENGTH * num_channels,
            0.05f,
            0.03f,
            "failed to forward attention");

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

static void test_pv_attention_param_load_failure_helper(pv_status_t expected) {
    FILE *dummy_file = malloc(sizeof(FILE *));
    pv_test_true(dummy_file != NULL, "failed create dummy file");
    if (!dummy_file) {
        return;
    }

    pv_attention_param_t *param = NULL;
    pv_status_t status = pv_attention_param_load(dummy_file, &param);
    pv_test_true(
            status == expected,
            "param load error, got `%s` expected `%s`",
            pv_status_to_string(status),
            pv_status_to_string(expected));
    free(dummy_file);
}

static void test_pv_attention_param_load_failure_1(void) {
    PV_SET_MOCK_CUSTOM_FUNC(pv_fread, pv_fread_invalid_ret_value)

    test_pv_attention_param_load_failure_helper(PV_STATUS_IO_ERROR);
}

static void test_pv_attention_param_load_failure_1_1(void) {
    PV_SET_MOCK_CUSTOM_FUNC(pv_fread, pv_fread_invalid_set_value)

    test_pv_attention_param_load_failure_helper(PV_STATUS_INVALID_ARGUMENT);
}

static void test_pv_attention_param_load_failure_2(void) {
    size_t (*custom_funcs[])(void *ptr, size_t size, size_t nmemb, FILE *stream) = {
        pv_fread_valid,
        pv_fread_invalid_ret_value,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(pv_fread, custom_funcs)

    test_pv_attention_param_load_failure_helper(PV_STATUS_IO_ERROR);
}

static void test_pv_attention_param_load_failure_2_2(void) {
    size_t (*custom_funcs[])(void *ptr, size_t size, size_t nmemb, FILE *stream) = {
        pv_fread_valid,
        pv_fread_invalid_set_value
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(pv_fread, custom_funcs)

    test_pv_attention_param_load_failure_helper(PV_STATUS_INVALID_ARGUMENT);
}

static void test_pv_attention_param_load_failure_3(void) {
    size_t (*custom_funcs[])(void *ptr, size_t size, size_t nmemb, FILE *stream) = {
        pv_fread_valid,
        pv_fread_valid,
        pv_fread_invalid_ret_value,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(pv_fread, custom_funcs)

    test_pv_attention_param_load_failure_helper(PV_STATUS_IO_ERROR);
}

static void test_pv_attention_param_load_failure_3_3(void) {
    size_t (*custom_funcs[])(void *ptr, size_t size, size_t nmemb, FILE *stream) = {
        pv_fread_valid,
        pv_fread_valid,
        pv_fread_invalid_set_value,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(pv_fread, custom_funcs)

    test_pv_attention_param_load_failure_helper(PV_STATUS_INVALID_ARGUMENT);
}

static void test_pv_attention_param_load_failure_4(void) {
    PV_SET_MOCK_CUSTOM_FUNC(pv_fread, pv_fread_valid)
    PV_SET_MOCK_RETURN_VAL(pv_embed_param_load, PV_STATUS_INVALID_ARGUMENT)

    test_pv_attention_param_load_failure_helper(PV_STATUS_INVALID_ARGUMENT);
}

static void test_pv_attention_param_load_failure_5(void) {
    PV_SET_MOCK_CUSTOM_FUNC(pv_fread, pv_fread_valid)
    pv_status_t custom_rets[] = {PV_STATUS_SUCCESS, PV_STATUS_INVALID_ARGUMENT};
    PV_SET_MOCK_RETURN_SEQ(pv_embed_param_load, custom_rets)

    test_pv_attention_param_load_failure_helper(PV_STATUS_INVALID_ARGUMENT);
}

static void test_pv_attention_param_load_failure_6(void) {
    PV_SET_MOCK_CUSTOM_FUNC(pv_fread, pv_fread_valid)
    PV_SET_MOCK_RETURN_VAL(pv_cnn_param_load, PV_STATUS_INVALID_ARGUMENT)

    test_pv_attention_param_load_failure_helper(PV_STATUS_INVALID_ARGUMENT);
}

static void test_pv_attention_param_load_failure_7(void) {
    PV_SET_MOCK_CUSTOM_FUNC(pv_fread, pv_fread_valid)
    pv_status_t custom_rets[] = {
            PV_STATUS_SUCCESS,
            PV_STATUS_INVALID_ARGUMENT,
    };
    PV_SET_MOCK_RETURN_SEQ(pv_cnn_param_load, custom_rets)

    test_pv_attention_param_load_failure_helper(PV_STATUS_INVALID_ARGUMENT);
}

static void test_pv_attention_param_load_failure_8(void) {
    PV_SET_MOCK_CUSTOM_FUNC(pv_fread, pv_fread_valid)
    pv_status_t custom_rets[] = {
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_INVALID_ARGUMENT,
    };
    PV_SET_MOCK_RETURN_SEQ(pv_cnn_param_load, custom_rets)

    test_pv_attention_param_load_failure_helper(PV_STATUS_INVALID_ARGUMENT);
}

static void test_pv_attention_param_load_failure_9(void) {
    PV_SET_MOCK_CUSTOM_FUNC(pv_fread, pv_fread_valid)
    pv_status_t custom_rets[] = {
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_INVALID_ARGUMENT,
    };
    PV_SET_MOCK_RETURN_SEQ(pv_cnn_param_load, custom_rets)

    test_pv_attention_param_load_failure_helper(PV_STATUS_INVALID_ARGUMENT);
}

#endif

static const pv_test_case_t PV_ATTENTION_TEST_CASES[] = {
        {"attention forward",          test_pv_attention_forward},

#ifdef __PV_MOCKS__

        {"attention param load failure 1", test_pv_attention_param_load_failure_1},
        {"attention param load failure 1 1", test_pv_attention_param_load_failure_1_1},
        {"attention param load failure 2", test_pv_attention_param_load_failure_2},
        {"attention param load failure 2 2", test_pv_attention_param_load_failure_2_2},
        {"attention param load failure 3", test_pv_attention_param_load_failure_3},
        {"attention param load failure 3 3", test_pv_attention_param_load_failure_3_3},
        {"attention param load failure 4", test_pv_attention_param_load_failure_4},
        {"attention param load failure 5", test_pv_attention_param_load_failure_5},
        {"attention param load failure 6", test_pv_attention_param_load_failure_6},
        {"attention param load failure 7", test_pv_attention_param_load_failure_7},
        {"attention param load failure 8", test_pv_attention_param_load_failure_8},
        {"attention param load failure 9", test_pv_attention_param_load_failure_9},

#endif

};

const pv_test_suite_t PV_ATTENTION_TEST_SUITE = {
        .name = "attention",
        .setup = test_pv_attention_setup,
        .teardown = test_pv_attention_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_ATTENTION_TEST_CASES),
        .test_cases = PV_ATTENTION_TEST_CASES,
};
