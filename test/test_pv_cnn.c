#include <stdlib.h>

#include "core/picovoice.h"
#include "core/pv_type.h"
#include "test/pv_test.h"

#include "test_data/test_pv_cnn_k1_data.c"
#include "test_data/test_pv_cnn_k3_data.c"
#include "test_data/test_pv_cnn_k5_data.c"
#include "test_data/test_pv_cnn_k7_data.c"
#include "test_data/test_pv_cnn_depthwise_data.c"
#include "test_data/test_pv_cnn_transposed_depthwise_data.c"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"
#include "ypu/mock/pv_ypu_mock.h"

#endif

static const float TEST_TOLERANCE_PERCENT = 0.02f;
static const float TEST_EPSILON = 0.005f;

static pv_ypu_t *ypu = NULL;

static pv_status_t test_pv_cnn_setup(void) {
    pv_status_t status = pv_ypu_init_cpu(1, &ypu);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_cnn_teardown(void) {
    pv_ypu_delete(ypu);
}

static void test_pv_cnn_helpers(void) {
    pv_cnn_t *cnn = NULL;
    pv_status_t status = pv_cnn_init(ypu, &TEST_CNN_K3_PARAM, &cnn);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to create cnn object");
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    pv_test_true(
            pv_cnn_output_channels(cnn) == TEST_CNN_K3_PARAM.output_channels,
            "failed to get num output channels");

    pv_test_true(
            pv_cnn_input_channels(cnn) == TEST_CNN_K3_PARAM.input_channels,
            "failed to get num input channels");

    pv_test_true(
            pv_cnn_kernel_size(cnn) == TEST_CNN_K3_PARAM.kernel_size,
            "failed to get kernel size");

    pv_test_true(
            pv_cnn_padding(cnn) == TEST_CNN_K3_PARAM.padding,
            "failed to get kernel size");

    pv_test_true(
            pv_cnn_dilation(cnn) == TEST_CNN_K3_PARAM.dilation,
            "failed to get kernel size");

    pv_test_true(
            pv_cnn_stride(cnn) == TEST_CNN_K3_PARAM.stride,
            "failed to get kernel size");
    pv_cnn_delete(ypu, cnn);
}

static void test_pv_cnn_kernel_1_forward(void) {
    pv_cnn_t *cnn = NULL;
    pv_status_t status = pv_cnn_init(ypu, &TEST_CNN_K1_PARAM, &cnn);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to create cnn object");
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    int32_t num_channels = pv_cnn_output_channels(cnn);

    pv_ypu_mem_t *m0 = pv_ypu_mem_alloc(
        ypu,
        sizeof(TEST_CNN_K1_INPUT),
        PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m0 != NULL, "Failed to allocate m0");
    if (m0 == NULL) {
        return;
    }

    pv_ypu_mem_t *m1 = pv_ypu_mem_alloc(
        ypu,
        TEST_CNN_K1_SEQUENCE_LENGTH * num_channels * sizeof(float),
        PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m1 != NULL, "Failed to allocate m1");
    if (m1 == NULL) {
        return;
    }

    status = pv_ypu_mem_copy_to(
        ypu,
        m0,
        TEST_CNN_K1_INPUT,
        0,
        sizeof(TEST_CNN_K1_INPUT));
    pv_test_true(
        status == PV_STATUS_SUCCESS,
        "pv_ypu_mem_copy_to failed with %s",
        pv_status_to_string(status));

    status = pv_cnn_forward(
        ypu,
        cnn,
        TEST_CNN_K1_SEQUENCE_LENGTH,
        m0,
        m1,
        0,
        0);
    pv_test_true(
        status == PV_STATUS_SUCCESS,
        "pv_cnn_forward failed with %s",
        pv_status_to_string(status));

    float *buffer = pv_ypu_mem_get_host_view(ypu, m1, true);
    pv_test_close_float_array(
            buffer,
            TEST_CNN_K1_TARGET,
            TEST_CNN_K1_SEQUENCE_LENGTH * num_channels,
            TEST_TOLERANCE_PERCENT,
            TEST_EPSILON,
            "failed to forward CNN with kernel size 1");

    pv_ypu_mem_release_host_view(ypu, m1, true);

    pv_ypu_mem_free(ypu, m0);
    pv_ypu_mem_free(ypu, m1);
    pv_cnn_delete(ypu, cnn);
}

static void test_pv_cnn_kernel_1_forward_to_q510(void) {
    pv_cnn_t *cnn = NULL;
    pv_status_t status = pv_cnn_init(ypu, &TEST_CNN_K1_PARAM, &cnn);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to create cnn object");
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    int32_t num_channels = pv_cnn_output_channels(cnn);

    pv_ypu_mem_t *m0 = pv_ypu_mem_alloc(
        ypu,
        sizeof(TEST_CNN_K1_INPUT),
        PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m0 != NULL, "Failed to allocate m0");
    if (m0 == NULL) {
        return;
    }

    pv_ypu_mem_t *m1 = pv_ypu_mem_alloc(
        ypu,
        TEST_CNN_K1_SEQUENCE_LENGTH * num_channels * sizeof(float),
        PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m1 != NULL, "Failed to allocate m1");
    if (m1 == NULL) {
        return;
    }

    pv_ypu_mem_t *m2 = pv_ypu_mem_alloc(
        ypu,
        TEST_CNN_K1_SEQUENCE_LENGTH * num_channels * sizeof(q510_t),
        PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m2 != NULL, "Failed to allocate m2");
    if (m2 == NULL) {
        return;
    }

    status = pv_ypu_mem_copy_to(
        ypu,
        m0,
        TEST_CNN_K1_INPUT,
        0,
        sizeof(TEST_CNN_K1_INPUT));
    pv_test_true(
        status == PV_STATUS_SUCCESS,
        "pv_ypu_mem_copy_to failed with %s",
        pv_status_to_string(status));

    status = pv_cnn_forward_to_q510(
        ypu,
        cnn,
        TEST_CNN_K1_SEQUENCE_LENGTH,
        m0,
        m2,
        0,
        0);
    pv_test_true(
        status == PV_STATUS_SUCCESS,
        "pv_cnn_forward_to_q510 failed with %s",
        pv_status_to_string(status));

    float *buffer = pv_ypu_mem_get_host_view(ypu, m1, true);
    q510_t *buffer_q510 = pv_ypu_mem_get_host_view(ypu, m2, true);

    for (int32_t i = 0; i < TEST_CNN_K1_SEQUENCE_LENGTH * num_channels; i++) {
        buffer[i] = pv_q510_to_float(buffer_q510[i]);
    }

    pv_test_close_float_array(
            buffer,
            TEST_CNN_K1_TARGET,
            TEST_CNN_K1_SEQUENCE_LENGTH * num_channels,
            0.04f,
            0.02f,
            "failed to forward CNN to q510 with kernel size 1");

    pv_ypu_mem_release_host_view(ypu, m1, true);
    pv_ypu_mem_release_host_view(ypu, m2, true);

    pv_ypu_mem_free(ypu, m0);
    pv_ypu_mem_free(ypu, m1);
    pv_ypu_mem_free(ypu, m2);
    pv_cnn_delete(ypu, cnn);
}

static void test_pv_cnn_kernel_1_forward_from_q510(void) {
    pv_cnn_t *cnn = NULL;
    pv_status_t status = pv_cnn_init(ypu, &TEST_CNN_K1_PARAM, &cnn);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to create cnn object");
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    int32_t num_channels = pv_cnn_output_channels(cnn);

    pv_ypu_mem_t *m1 = pv_ypu_mem_alloc(
        ypu,
        TEST_CNN_K1_SEQUENCE_LENGTH * num_channels * sizeof(float),
        PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m1 != NULL, "Failed to allocate m1");
    if (m1 == NULL) {
        return;
    }

    pv_ypu_mem_t *m2 = pv_ypu_mem_alloc(
        ypu,
        sizeof(TEST_CNN_K1_INPUT),
        PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m2 != NULL, "Failed to allocate m2");
    if (m2 == NULL) {
        return;
    }

    q510_t *input = pv_ypu_mem_get_host_view(ypu, m2, true);
    for (int32_t i = 0; i < PV_ARRAY_LEN(TEST_CNN_K1_INPUT); i++) {
        input[i] = pv_float_to_q510(TEST_CNN_K1_INPUT[i]);
    }
    pv_ypu_mem_release_host_view(ypu, m2, true);

    status = pv_cnn_forward_from_q510(
        ypu,
        cnn,
        TEST_CNN_K1_SEQUENCE_LENGTH,
        m2,
        m1,
        0,
        0);
    pv_test_true(
        status == PV_STATUS_SUCCESS,
        "pv_cnn_forward_from_q510 failed with %s",
        pv_status_to_string(status));

    float *buffer = pv_ypu_mem_get_host_view(ypu, m1, true);
    pv_test_close_float_array(
            buffer,
            TEST_CNN_K1_TARGET,
            TEST_CNN_K1_SEQUENCE_LENGTH * num_channels,
            0.04f,
            0.02f,
            "failed to forward CNN from q510 with kernel size 1");
    pv_ypu_mem_release_host_view(ypu, m1, true);

    pv_ypu_mem_free(ypu, m1);
    pv_ypu_mem_free(ypu, m2);
    pv_cnn_delete(ypu, cnn);
}

static void test_pv_cnn_kernel_3_forward(void) {
    pv_cnn_t *cnn = NULL;
    pv_status_t status = pv_cnn_init(ypu, &TEST_CNN_K3_PARAM, &cnn);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to create cnn object");
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    int32_t num_channels = pv_cnn_output_channels(cnn);

    pv_ypu_mem_t *m0 = pv_ypu_mem_alloc(
        ypu,
        sizeof(TEST_CNN_K3_INPUT),
        PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m0 != NULL, "Failed to allocate m0");
    if (m0 == NULL) {
        return;
    }

    pv_ypu_mem_t *m1 = pv_ypu_mem_alloc(
        ypu,
        TEST_CNN_K3_SEQUENCE_LENGTH * num_channels * sizeof(float),
        PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m1 != NULL, "Failed to allocate m1");
    if (m1 == NULL) {
        return;
    }

    status = pv_ypu_mem_copy_to(
        ypu,
        m0,
        TEST_CNN_K3_INPUT,
        0,
        sizeof(TEST_CNN_K3_INPUT));
    pv_test_true(
        status == PV_STATUS_SUCCESS,
        "pv_ypu_mem_copy_to failed with %s",
        pv_status_to_string(status));

    status = pv_cnn_forward(
        ypu,
        cnn,
        TEST_CNN_K3_SEQUENCE_LENGTH,
        m0,
        m1,
        0,
        0);
    pv_test_true(
        status == PV_STATUS_SUCCESS,
        "pv_cnn_forward failed with %s",
        pv_status_to_string(status));

    float *buffer = pv_ypu_mem_get_host_view(ypu, m1, true);
    pv_test_close_float_array(
            buffer,
            TEST_CNN_K3_TARGET,
            TEST_CNN_K3_SEQUENCE_LENGTH * num_channels,
            TEST_TOLERANCE_PERCENT,
            TEST_EPSILON,
            "failed to forward CNN with kernel size 3");
    pv_ypu_mem_release_host_view(ypu, m1, true);

    pv_ypu_mem_free(ypu, m0);
    pv_ypu_mem_free(ypu, m1);
    pv_cnn_delete(ypu, cnn);
}

static void test_pv_cnn_kernel_5_forward(void) {
    pv_cnn_t *cnn = NULL;
    pv_status_t status = pv_cnn_init(ypu, &TEST_CNN_K5_PARAM, &cnn);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to create cnn object");
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    int32_t num_channels = pv_cnn_output_channels(cnn);

    pv_ypu_mem_t *m0 = pv_ypu_mem_alloc(
        ypu,
        sizeof(TEST_CNN_K5_INPUT),
        PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m0 != NULL, "Failed to allocate m0");
    if (m0 == NULL) {
        return;
    }

    pv_ypu_mem_t *m1 = pv_ypu_mem_alloc(
        ypu,
        TEST_CNN_K5_SEQUENCE_LENGTH * num_channels * sizeof(float),
        PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m1 != NULL, "Failed to allocate m1");
    if (m1 == NULL) {
        return;
    }

    status = pv_ypu_mem_copy_to(
        ypu,
        m0,
        TEST_CNN_K5_INPUT,
        0,
        sizeof(TEST_CNN_K5_INPUT));
    pv_test_true(
        status == PV_STATUS_SUCCESS,
        "pv_ypu_mem_copy_to failed with %s",
        pv_status_to_string(status));

    status = pv_cnn_forward(
        ypu,
        cnn,
        TEST_CNN_K5_SEQUENCE_LENGTH,
        m0,
        m1,
        0,
        0);
    pv_test_true(
        status == PV_STATUS_SUCCESS,
        "pv_cnn_forward failed with %s",
        pv_status_to_string(status));

    float *buffer = pv_ypu_mem_get_host_view(ypu, m1, true);
    pv_test_close_float_array(
            buffer,
            TEST_CNN_K5_TARGET,
            TEST_CNN_K5_SEQUENCE_LENGTH * num_channels,
            TEST_TOLERANCE_PERCENT,
            TEST_EPSILON,
            "failed to forward CNN with kernel size 5");
    pv_ypu_mem_release_host_view(ypu, m1, true);

    pv_ypu_mem_free(ypu, m0);
    pv_ypu_mem_free(ypu, m1);
    pv_cnn_delete(ypu, cnn);
}

static void test_pv_cnn_kernel_7_forward(void) {
    pv_cnn_t *cnn = NULL;
    pv_status_t status = pv_cnn_init(ypu, &TEST_CNN_K7_PARAM, &cnn);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to create cnn object");
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    int32_t num_channels = pv_cnn_output_channels(cnn);

    pv_ypu_mem_t *m0 = pv_ypu_mem_alloc(
        ypu,
        sizeof(TEST_CNN_K7_INPUT),
        PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m0 != NULL, "Failed to allocate m0");
    if (m0 == NULL) {
        return;
    }

    pv_ypu_mem_t *m1 = pv_ypu_mem_alloc(
        ypu,
        TEST_CNN_K7_SEQUENCE_LENGTH * num_channels * sizeof(float),
        PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m1 != NULL, "Failed to allocate m1");
    if (m1 == NULL) {
        return;
    }

    status = pv_ypu_mem_copy_to(
        ypu,
        m0,
        TEST_CNN_K7_INPUT,
        0,
        sizeof(TEST_CNN_K7_INPUT));
    pv_test_true(
        status == PV_STATUS_SUCCESS,
        "pv_ypu_mem_copy_to failed with %s",
        pv_status_to_string(status));

    status = pv_cnn_forward(
        ypu,
        cnn,
        TEST_CNN_K7_SEQUENCE_LENGTH,
        m0,
        m1,
        0,
        0);
    pv_test_true(
        status == PV_STATUS_SUCCESS,
        "pv_cnn_forward failed with %s",
        pv_status_to_string(status));

    float *buffer = pv_ypu_mem_get_host_view(ypu, m1, true);
    pv_test_close_float_array(
            buffer,
            TEST_CNN_K7_TARGET,
            TEST_CNN_K7_SEQUENCE_LENGTH * num_channels,
            TEST_TOLERANCE_PERCENT,
            TEST_EPSILON,
            "failed to forward CNN with kernel size 7");
    pv_ypu_mem_release_host_view(ypu, m1, true);

    pv_ypu_mem_free(ypu, m0);
    pv_ypu_mem_free(ypu, m1);
    pv_cnn_delete(ypu, cnn);
}

static void test_pv_cnn_depthwise_forward(void) {
    pv_cnn_depthwise_t *cnn = NULL;
    pv_status_t status = pv_cnn_depthwise_init(ypu, &TEST_CNN_DEPTHWISE_PARAM, &cnn);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to create cnn object");
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    int32_t num_channels = pv_cnn_depthwise_num_channels(cnn);

    pv_ypu_mem_t *m0 = pv_ypu_mem_alloc(
        ypu,
        sizeof(TEST_CNN_DEPTHWISE_INPUT),
        PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m0 != NULL, "Failed to allocate m0");
    if (m0 == NULL) {
        return;
    }

    pv_ypu_mem_t *m1 = pv_ypu_mem_alloc(
        ypu,
        TEST_CNN_DEPTHWISE_SEQUENCE_LENGTH * num_channels * sizeof(float),
        PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m1 != NULL, "Failed to allocate m1");
    if (m1 == NULL) {
        return;
    }

    status = pv_ypu_mem_copy_to(
        ypu,
        m0,
        TEST_CNN_DEPTHWISE_INPUT,
        0,
        sizeof(TEST_CNN_DEPTHWISE_INPUT));
    pv_test_true(
        status == PV_STATUS_SUCCESS,
        "pv_ypu_mem_copy_to failed with %s",
        pv_status_to_string(status));


    pv_cnn_depthwise_forward(
            ypu,
            cnn,
            TEST_CNN_DEPTHWISE_SEQUENCE_LENGTH,
            m0,
            m1,
            0,
            0);
    pv_test_true(
        status == PV_STATUS_SUCCESS,
        "pv_cnn_depthwise_forward failed with %s",
        pv_status_to_string(status));

    float *buffer = pv_ypu_mem_get_host_view(ypu, m1, true);
    pv_test_close_float_array(
            buffer,
            TEST_CNN_DEPTHWISE_TARGET,
            TEST_CNN_DEPTHWISE_SEQUENCE_LENGTH * num_channels,
            0.05f,
            0.03f,
            "failed to forward CNN depthwise");
    pv_ypu_mem_release_host_view(ypu, m1, true);

    pv_ypu_mem_free(ypu, m0);
    pv_ypu_mem_free(ypu, m1);
    pv_cnn_depthwise_delete(ypu, cnn);
}

static void test_pv_cnn_transposed_depthwise_forward(void) {
    pv_cnn_transposed_depthwise_t *cnn = NULL;
    pv_status_t status = pv_cnn_transposed_depthwise_init(
        ypu,
        &TEST_CNN_TRANSPOSED_DEPTHWISE_PARAM,
        &cnn);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to create cnn object");
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    int32_t num_channels = pv_cnn_depthwise_num_channels((const pv_cnn_depthwise_t *) cnn);
    int32_t num_output_frames = pv_cnn_transposed_depthwise_num_output_frames(
            cnn,
            TEST_CNN_TRANSPOSED_DEPTHWISE_SEQUENCE_LENGTH);

    pv_ypu_mem_t *m0 = pv_ypu_mem_alloc(
        ypu,
        sizeof(TEST_CNN_TRANSPOSED_DEPTHWISE_INPUT),
        PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m0 != NULL, "Failed to allocate m0");
    if (m0 == NULL) {
        return;
    }

    pv_ypu_mem_t *m1 = pv_ypu_mem_alloc(
        ypu,
        num_output_frames * num_channels * sizeof(float),
        PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m1 != NULL, "Failed to allocate m1");
    if (m1 == NULL) {
        return;
    }

    status = pv_ypu_mem_copy_to(
        ypu,
        m0,
        TEST_CNN_TRANSPOSED_DEPTHWISE_INPUT,
        0,
        sizeof(TEST_CNN_TRANSPOSED_DEPTHWISE_INPUT));
    pv_test_true(
        status == PV_STATUS_SUCCESS,
        "pv_ypu_mem_copy_to failed with %s",
        pv_status_to_string(status));


    status = pv_cnn_transposed_depthwise_forward(
        ypu,
        cnn,
        TEST_CNN_TRANSPOSED_DEPTHWISE_SEQUENCE_LENGTH,
        m0,
        m1,
        0, 0);
    pv_test_true(
        status == PV_STATUS_SUCCESS,
        "pv_cnn_transposed_depthwise_forward failed with %s",
        pv_status_to_string(status));

    float *buffer = pv_ypu_mem_get_host_view(ypu, m1, true);
    pv_test_close_float_array(
            buffer,
            TEST_CNN_TRANSPOSED_DEPTHWISE_TARGET,
            num_output_frames * num_channels,
            0.05f,
            0.03f,
            "failed to forward CNN transposed depthwise");
    pv_ypu_mem_release_host_view(ypu, m1, true);

    pv_ypu_mem_free(ypu, m0);
    pv_ypu_mem_free(ypu, m1);
    pv_cnn_transposed_depthwise_delete(ypu, cnn);
}

#ifdef __PV_MOCKS__

static void test_pv_cnn_init_1st_alloc_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_ypu_host_alloc, NULL)

    pv_cnn_t *cnn = NULL;
    pv_status_t status = pv_cnn_init(ypu, &TEST_CNN_K3_PARAM, &cnn);
    pv_test_true(status == PV_STATUS_OUT_OF_MEMORY, "cnn init should fail with `PV_STATUS_OUT_OF_MEMORY`");
    
    reset_mocks();
    pv_test_error_message(
            "Failed to allocate, out of memory\\.",
            "Failed to allocate memory for `o`\\.",
            true,
            "cnn init 1st alloc failure error message mismatch");
}

static void test_pv_cnn_depthwise_init_1st_alloc_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_ypu_host_alloc, NULL)

    pv_cnn_depthwise_t *cnn = NULL;
    pv_status_t status = pv_cnn_depthwise_init(ypu, &TEST_CNN_DEPTHWISE_PARAM, &cnn);
    pv_test_true(status == PV_STATUS_OUT_OF_MEMORY, "cnn init should fail with `PV_STATUS_OUT_OF_MEMORY`");
    
    reset_mocks();
    pv_test_error_message(
            "Failed to allocate, out of memory\\.",
            "Failed to allocate memory for `o`\\.",
            true,
            "cnn depthwise init 1st alloc failure error message mismatch");
}

#endif

static const pv_test_case_t PV_CNN_TEST_CASES[] = {
        {"cnn helper functions", test_pv_cnn_helpers},
        {"cnn forward kernel 1", test_pv_cnn_kernel_1_forward},
        {"cnn forward kernel 1 to q510", test_pv_cnn_kernel_1_forward_to_q510},
        {"cnn forward kernel 1 from q510", test_pv_cnn_kernel_1_forward_from_q510},
        {"cnn forward kernel 3", test_pv_cnn_kernel_3_forward},
        {"cnn forward kernel 5", test_pv_cnn_kernel_5_forward},
        {"cnn forward kernel 7", test_pv_cnn_kernel_7_forward},
        {"cnn depthwise forward", test_pv_cnn_depthwise_forward},
        {"cnn transposed depthwise forward", test_pv_cnn_transposed_depthwise_forward},

#ifdef __PV_MOCKS__

        {"cnn init 1st alloc failure", test_pv_cnn_init_1st_alloc_failure},
        {"cnn depthwise init 1st alloc failure", test_pv_cnn_depthwise_init_1st_alloc_failure},

#endif

};

const pv_test_suite_t PV_CNN_TEST_SUITE = {
        .name = "cnn",
        .setup = test_pv_cnn_setup,
        .teardown = test_pv_cnn_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_CNN_TEST_CASES),
        .test_cases = PV_CNN_TEST_CASES,
};
