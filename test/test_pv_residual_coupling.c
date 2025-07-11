#include <stdlib.h>

#include "core/picovoice.h"
#include "core/pv_type.h"
#include "test/pv_test.h"

#include "test_data/test_pv_residual_coupling_data.c"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

static pv_residual_coupling_t *residual_coupling_object = NULL;

static pv_buffer_t *buffer_flow_x0_object = NULL;
static pv_buffer_t *buffer_flow_x1_object = NULL;
static pv_buffer_t *buffer_flow_mean_object = NULL;
static pv_buffer_t *buffer_flow_wavenet_in_object = NULL;
static pv_buffer_t *buffer_flow_wavenet_hidden_object = NULL;
static pv_buffer_t *buffer_flow_wavenet_inter_object = NULL;
static pv_buffer_t *buffer_flow_wavenet_inter_out_object = NULL;
static pv_buffer_t *buffer_flow_wavenet_out_object = NULL;

static pv_status_t test_pv_residual_coupling_setup(void) {
    const int32_t flow_channels = TEST_RESIDUAL_COUPLING_PARAM.conv_pre_param->input_channels;

    pv_status_t status = pv_buffer_init(flow_channels, &buffer_flow_x0_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_buffer_init(flow_channels, &buffer_flow_x1_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_buffer_init(flow_channels, &buffer_flow_mean_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    const int32_t wavenet_in_channels = TEST_RESIDUAL_COUPLING_PARAM.wavenet_resblocks_param[0]->conv_param->input_channels;
    const int32_t wavenet_hidden_channels = TEST_RESIDUAL_COUPLING_PARAM.wavenet_resblocks_param[0]->conv_param->output_channels;

    status = pv_buffer_init(wavenet_in_channels, &buffer_flow_wavenet_in_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_buffer_init(wavenet_hidden_channels, &buffer_flow_wavenet_hidden_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_buffer_init(wavenet_in_channels, &buffer_flow_wavenet_inter_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_buffer_init(wavenet_in_channels, &buffer_flow_wavenet_inter_out_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_buffer_init(wavenet_in_channels, &buffer_flow_wavenet_out_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_residual_coupling_init(
            &TEST_RESIDUAL_COUPLING_PARAM,
            buffer_flow_x0_object,
            buffer_flow_x1_object,
            buffer_flow_mean_object,
            buffer_flow_wavenet_in_object,
            buffer_flow_wavenet_hidden_object,
            buffer_flow_wavenet_inter_object,
            buffer_flow_wavenet_inter_out_object,
            buffer_flow_wavenet_out_object,
            &residual_coupling_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_residual_coupling_teardown(void) {
    pv_buffer_delete(buffer_flow_wavenet_out_object);
    pv_buffer_delete(buffer_flow_wavenet_inter_out_object);
    pv_buffer_delete(buffer_flow_wavenet_inter_object);
    pv_buffer_delete(buffer_flow_wavenet_hidden_object);
    pv_buffer_delete(buffer_flow_wavenet_in_object);
    pv_buffer_delete(buffer_flow_mean_object);
    pv_buffer_delete(buffer_flow_x1_object);
    pv_buffer_delete(buffer_flow_x0_object);

    pv_residual_coupling_delete(residual_coupling_object);
    residual_coupling_object = NULL;
}

static void test_pv_residual_coupling_forward(void) {
    pv_test_true(residual_coupling_object != NULL, "failed to create tmp file");

    int32_t num_channels = pv_residual_coupling_output_channels(residual_coupling_object);
    float *buffer = calloc(TEST_RESIDUAL_COUPLING_SEQUENCE_LENGTH, num_channels * sizeof(float));

    pv_residual_coupling_forward(
            residual_coupling_object,
            TEST_RESIDUAL_COUPLING_SEQUENCE_LENGTH,
            TEST_RESIDUAL_COUPLING_INPUT,
            buffer);
    pv_test_close_float_array(
            buffer,
            TEST_RESIDUAL_COUPLING_TARGET,
            TEST_RESIDUAL_COUPLING_SEQUENCE_LENGTH * num_channels,
            0.05f,
            0.01f,
            "failed to forward residual_coupling");

     free(buffer);
}

static const pv_test_case_t PV_RESIDUAL_COUPLING_TEST_CASES[] = {
        {"residual_coupling forward", test_pv_residual_coupling_forward},
};

const pv_test_suite_t PV_RESIDUAL_COUPLING_TEST_SUITE = {
        .name = "residual_coupling",
        .setup = test_pv_residual_coupling_setup,
        .teardown = test_pv_residual_coupling_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_RESIDUAL_COUPLING_TEST_CASES),
        .test_cases = PV_RESIDUAL_COUPLING_TEST_CASES,
};
