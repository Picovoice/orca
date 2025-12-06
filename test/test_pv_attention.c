#include <stdlib.h>

#include "core/picovoice.h"
#include "core/pv_type.h"
#include "test/pv_test.h"

#include "test_data/test_pv_attention_data.c"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

static pv_ypu_t *ypu = NULL;
static pv_attention_t *attention_object = NULL;

static pv_status_t test_pv_attention_setup(void) {
    pv_status_t status = pv_ypu_init_cpu(1, &ypu);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_attention_init(
            ypu,
            &TEST_ATTENTION_PARAM,
            &attention_object);
    if (status != PV_STATUS_SUCCESS) {
        pv_ypu_delete(ypu);
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_attention_teardown(void) {
    pv_attention_delete(ypu, attention_object);
    pv_ypu_delete(ypu);
}

static void test_pv_attention_forward(void) {
    int32_t num_channels = PV_ARRAY_LEN(TEST_ATTENTION_TARGET) / TEST_ATTENTION_SEQUENCE_LENGTH;

    pv_ypu_mem_t *m0 = pv_ypu_mem_alloc(
        ypu,
        sizeof(TEST_ATTENTION_INPUT),
        PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m0 != NULL, "Failed to allocate m0");
    if (m0 == NULL) {
        return;
    }

    pv_ypu_mem_t *m1 = pv_ypu_mem_alloc(
        ypu,
        TEST_ATTENTION_SEQUENCE_LENGTH * num_channels * sizeof(float),
        PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m1 != NULL, "Failed to allocate m1");
    if (m1 == NULL) {
        return;
    }

    pv_status_t status = pv_ypu_mem_copy_to(
        ypu,
        m0,
        TEST_ATTENTION_INPUT,
        0,
        sizeof(TEST_ATTENTION_INPUT));
    pv_test_true(
        status == PV_STATUS_SUCCESS,
        "pv_ypu_mem_copy_to failed with %s",
        pv_status_to_string(status));

    status = pv_attention_forward(
            ypu,
            attention_object,
            TEST_ATTENTION_SEQUENCE_LENGTH,
            m0,
            m0,
            m1,
            0,
            0,
            0);
    pv_test_true(
        status == PV_STATUS_SUCCESS,
        "pv_attention_forward failed with %s",
        pv_status_to_string(status));

    float *buffer = pv_ypu_mem_get_host_view(ypu, m1, true);
    pv_test_close_float_array(
            buffer,
            TEST_ATTENTION_TARGET,
            TEST_ATTENTION_SEQUENCE_LENGTH * num_channels,
            0.05f,
            0.03f,
            "failed to forward attention");
    pv_ypu_mem_release_host_view(ypu, m1, true);

    pv_ypu_mem_free(ypu, m0);
    pv_ypu_mem_free(ypu, m1);
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
    pv_status_t status = pv_attention_param_load(ypu, dummy_file, &param);
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
