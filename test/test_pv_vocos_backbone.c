#include <stdlib.h>

#include "core/picovoice.h"
#include "core/pv_type.h"
#include "test/pv_test.h"

#include "test_data/test_pv_vocos_backbone_data.c"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

static pv_ypu_t *ypu = NULL;
static pv_vocos_backbone_t *vocos_backbone_object = NULL;

static pv_status_t test_pv_vocos_backbone_setup(void) {
    pv_status_t status = pv_ypu_init_cpu(1, &ypu);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_vocos_backbone_init(ypu, &DEC_BACKBONE_0_PARAM, &vocos_backbone_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_vocos_backbone_teardown(void) {
    pv_vocos_backbone_delete(ypu, vocos_backbone_object);
    pv_ypu_delete(ypu);
}

static void test_pv_vocos_backbone_forward(void) {
    int32_t num_channels = pv_vocos_backbone_output_channels(vocos_backbone_object);

    pv_ypu_mem_t *m0 = pv_ypu_mem_alloc(
        ypu,
        sizeof(TEST_VOCOS_BACKBONE_INPUT),
        PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m0 != NULL, "Failed to allocate m0");
    if (m0 == NULL) {
        return;
    }

    pv_ypu_mem_t *m1 = pv_ypu_mem_alloc(
        ypu,
        TEST_VOCOS_BACKBONE_SEQUENCE_LENGTH * num_channels * sizeof(float),
        PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m1 != NULL, "Failed to allocate m1");
    if (m1 == NULL) {
        return;
    }

    pv_status_t status = pv_ypu_mem_copy_to(
        ypu,
        m0,
        TEST_VOCOS_BACKBONE_INPUT,
        0,
        sizeof(TEST_VOCOS_BACKBONE_INPUT));
    pv_test_true(
        status == PV_STATUS_SUCCESS,
        "pv_ypu_mem_copy_to failed with %s",
        pv_status_to_string(status));

    status = pv_vocos_backbone_forward(
        ypu,
        vocos_backbone_object,
        TEST_VOCOS_BACKBONE_SEQUENCE_LENGTH,
        m0,
        m1,
        0,
        0);
    pv_test_true(
        status == PV_STATUS_SUCCESS,
        "pv_vocos_backbone_forward failed with %s",
        pv_status_to_string(status));

    float *buffer = pv_ypu_mem_get_host_view(ypu, m1, true);
    pv_test_close_float_array(
            buffer,
            TEST_VOCOS_BACKBONE_TARGET,
            TEST_VOCOS_BACKBONE_SEQUENCE_LENGTH * num_channels,
            0.02f,
            0.001f,
            "failed to forward vocos_backbone");
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

static void *pv_ypu_host_alloc_return_null(pv_ypu_t *ypu, int32_t size_bytes) {
    (void) ypu;
    (void) size_bytes;
    return NULL;
}

static void test_pv_vocos_backbone_param_load_failure_helper(pv_status_t expected) {
    FILE *dummy_file = malloc(sizeof(FILE *));
    pv_test_true(dummy_file != NULL, "failed create dummy file");
    if (!dummy_file) {
        return;
    }

    pv_vocos_backbone_param_t *param = NULL;
    pv_status_t status = pv_vocos_backbone_param_load(ypu, dummy_file, &param);
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
    void *(*custom_funcs[])(pv_ypu_t *, int32_t) = {
            pv_ypu_host_alloc_real,
            pv_ypu_host_alloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(pv_ypu_host_alloc, custom_funcs)

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
