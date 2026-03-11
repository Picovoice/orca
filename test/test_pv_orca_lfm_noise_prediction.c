#include <stdlib.h>
#include <string.h>

#include "core/picovoice.h"
#include "core/pv_type.h"
#include "orca/pv_orca_stream_state.h"
#include "test/pv_test.h"

#include "test_data/test_pv_orca_lfm_noise_prediction_data.c"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

#ifdef __ORCA_FLOAT_MODE__

static const float TEST_TOLERANCE_PERCENT = 0.0008f;
static const float TEST_EPSILON = 0.005f;

#else

static const float TEST_TOLERANCE_PERCENT = 0.12f;
static const float TEST_EPSILON = 0.07f;

#endif

static const float *TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_LIST[40] = {
        TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_0,
        TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_1,
        TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_2,
        TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_3,
        TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_4,
        TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_5,
        TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_6,
        TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_7,
        TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_8,
        TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_9,
        TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_10,
        TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_11,
        TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_12,
        TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_13,
        TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_14,
        TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_15,
        TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_16,
        TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_17,
        TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_18,
        TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_19,
        TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_20,
        TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_21,
        TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_22,
        TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_23,
        TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_24,
        TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_25,
        TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_26,
        TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_27,
        TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_28,
        TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_29,
        TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_30,
        TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_31,
        TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_32,
        TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_33,
        TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_34,
        TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_35,
        TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_36,
        TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_37,
        TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_38,
        TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_39,
};

static const int32_t TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_SIZE_LIST[40] = {
        sizeof(TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_0),
        sizeof(TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_1),
        sizeof(TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_2),
        sizeof(TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_3),
        sizeof(TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_4),
        sizeof(TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_5),
        sizeof(TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_6),
        sizeof(TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_7),
        sizeof(TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_8),
        sizeof(TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_9),
        sizeof(TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_10),
        sizeof(TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_11),
        sizeof(TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_12),
        sizeof(TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_13),
        sizeof(TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_14),
        sizeof(TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_15),
        sizeof(TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_16),
        sizeof(TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_17),
        sizeof(TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_18),
        sizeof(TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_19),
        sizeof(TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_20),
        sizeof(TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_21),
        sizeof(TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_22),
        sizeof(TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_23),
        sizeof(TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_24),
        sizeof(TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_25),
        sizeof(TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_26),
        sizeof(TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_27),
        sizeof(TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_28),
        sizeof(TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_29),
        sizeof(TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_30),
        sizeof(TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_31),
        sizeof(TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_32),
        sizeof(TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_33),
        sizeof(TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_34),
        sizeof(TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_35),
        sizeof(TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_36),
        sizeof(TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_37),
        sizeof(TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_38),
        sizeof(TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_39),
};


static const float *TEST_ORCA_LFM_NOISE_PREDICTION_TARGET_LIST[30] = {
        TEST_ORCA_LFM_NOISE_PREDICTION_TARGET_0,
        TEST_ORCA_LFM_NOISE_PREDICTION_TARGET_1,
        TEST_ORCA_LFM_NOISE_PREDICTION_TARGET_2,
        TEST_ORCA_LFM_NOISE_PREDICTION_TARGET_3,
        TEST_ORCA_LFM_NOISE_PREDICTION_TARGET_4,
        TEST_ORCA_LFM_NOISE_PREDICTION_TARGET_5,
        TEST_ORCA_LFM_NOISE_PREDICTION_TARGET_6,
        TEST_ORCA_LFM_NOISE_PREDICTION_TARGET_7,
        TEST_ORCA_LFM_NOISE_PREDICTION_TARGET_8,
        TEST_ORCA_LFM_NOISE_PREDICTION_TARGET_9,
        TEST_ORCA_LFM_NOISE_PREDICTION_TARGET_10,
        TEST_ORCA_LFM_NOISE_PREDICTION_TARGET_11,
        TEST_ORCA_LFM_NOISE_PREDICTION_TARGET_12,
        TEST_ORCA_LFM_NOISE_PREDICTION_TARGET_13,
        TEST_ORCA_LFM_NOISE_PREDICTION_TARGET_14,
        TEST_ORCA_LFM_NOISE_PREDICTION_TARGET_15,
        TEST_ORCA_LFM_NOISE_PREDICTION_TARGET_16,
        TEST_ORCA_LFM_NOISE_PREDICTION_TARGET_17,
        TEST_ORCA_LFM_NOISE_PREDICTION_TARGET_18,
        TEST_ORCA_LFM_NOISE_PREDICTION_TARGET_19,
        TEST_ORCA_LFM_NOISE_PREDICTION_TARGET_20,
        TEST_ORCA_LFM_NOISE_PREDICTION_TARGET_21,
        TEST_ORCA_LFM_NOISE_PREDICTION_TARGET_22,
        TEST_ORCA_LFM_NOISE_PREDICTION_TARGET_23,
        TEST_ORCA_LFM_NOISE_PREDICTION_TARGET_24,
        TEST_ORCA_LFM_NOISE_PREDICTION_TARGET_25,
        TEST_ORCA_LFM_NOISE_PREDICTION_TARGET_26,
        TEST_ORCA_LFM_NOISE_PREDICTION_TARGET_27,
        TEST_ORCA_LFM_NOISE_PREDICTION_TARGET_28,
        TEST_ORCA_LFM_NOISE_PREDICTION_TARGET_29,
};

static pv_ypu_t *ypu = NULL;
static pv_orca_lfm_condition_fuser_t *orca_lfm_condition_fuser_object = NULL;
static pv_orca_lfm_vf_estimator_t *orca_lfm_vf_estimator_object = NULL;

static pv_status_t test_pv_orca_lfm_noise_prediction_setup(void) {
    pv_status_t status = pv_ypu_init_cpu(1, &ypu);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_orca_lfm_condition_fuser_init(
            ypu,
            &LFM_CONDITION_FUSER_PARAM,
            &orca_lfm_condition_fuser_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_orca_lfm_vf_estimator_init(
            ypu,
            &LFM_VF_ESTIMATOR_PARAM,
            &orca_lfm_vf_estimator_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_orca_lfm_noise_prediction_teardown(void) {
    pv_orca_lfm_condition_fuser_delete(ypu, orca_lfm_condition_fuser_object);
    pv_orca_lfm_vf_estimator_delete(ypu, orca_lfm_vf_estimator_object);
    pv_ypu_delete(ypu);
}

static pv_status_t test_pv_orca_lfm_noise_prediction_forward_inner(
        bool test_accuracy,
        bool mock_lfm_condition_fuser) {
    const int32_t T = TEST_ORCA_LFM_NOISE_PREDICTION_SEQUENCE_LENGTH;
    const int32_t out_dimension = LFM_VF_ESTIMATOR_PARAM.out_dimension;
    pv_test_true(out_dimension == 16, "`out_dimension != 16`");
    const int32_t dimension = LFM_VF_ESTIMATOR_PARAM.dimension;
    pv_test_true(dimension == 100, "`dimension != 100`");
    float *buffer_lfm_x_t = (float *) calloc(
            T * out_dimension, // [T, O = 16].
            sizeof(float));
    memcpy(
            buffer_lfm_x_t,
            TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_LIST[3],
            T * out_dimension * sizeof(float));

    pv_orca_stream_state_t *state = calloc(1, sizeof(pv_orca_stream_state_t));
    state->status = PV_ORCA_STREAM_STATUS_INACTIVE;

    const int32_t nfe = 10;
    const float dt = 0.1f;

    for (int32_t i = 0; i < nfe; ++i) {
        const int32_t i_input_offset = i * 4;
        const int32_t i_output_offset = i * 3;

        pv_ypu_mem_t *m0 = pv_ypu_mem_alloc(
                ypu,
                TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_SIZE_LIST[i_input_offset],
                PV_YPU_DEVICE_MEM_FLAG_NONE);
        pv_test_true(m0 != NULL, "Failed to allocate m0");
        if (m0 == NULL) {
            return PV_STATUS_OUT_OF_MEMORY;
        }
        pv_status_t status = pv_ypu_mem_copy_to(
                ypu,
                m0,
                TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_LIST[i_input_offset],
                0,
                TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_SIZE_LIST[i_input_offset]);
        pv_test_true(
                status == PV_STATUS_SUCCESS,
                "pv_ypu_mem_copy_to failed with %s",
                pv_status_to_string(status));

        pv_ypu_mem_t *m1 = pv_ypu_mem_alloc(
                ypu,
                TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_SIZE_LIST[i_input_offset + 1],
                PV_YPU_DEVICE_MEM_FLAG_NONE);
        pv_test_true(m1 != NULL, "Failed to allocate m1");
        if (m1 == NULL) {
            return PV_STATUS_OUT_OF_MEMORY;
        }
        status = pv_ypu_mem_copy_to(
                ypu,
                m1,
                TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_LIST[i_input_offset + 1],
                0,
                TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_SIZE_LIST[i_input_offset + 1]);
        pv_test_true(
                status == PV_STATUS_SUCCESS,
                "pv_ypu_mem_copy_to failed with %s",
                pv_status_to_string(status));


        pv_ypu_mem_t *m2 = pv_ypu_mem_alloc(
                ypu,
                T * dimension * sizeof(float),
                PV_YPU_DEVICE_MEM_FLAG_NONE);
        pv_test_true(m2 != NULL, "Failed to allocate m2");
        if (m2 == NULL) {
            return PV_STATUS_OUT_OF_MEMORY;
        }

        status = pv_orca_lfm_condition_fuser_forward(
                ypu,
                orca_lfm_condition_fuser_object,
                T,
                m0,
                m1,
                m2,
                0,
                0,
                0);
        pv_ypu_mem_free(ypu, m0);
        pv_ypu_mem_free(ypu, m1);
        if (test_accuracy) {
            pv_test_true(status == PV_STATUS_SUCCESS, "pv_orca_lfm_condition_fuser_forward() returns failed status");

            float *buffer_lfm_condition = pv_ypu_mem_get_host_view(ypu, m2, true);
            pv_test_close_float_array(
                    buffer_lfm_condition,
                    TEST_ORCA_LFM_NOISE_PREDICTION_TARGET_LIST[i_output_offset + 0],
                    T * dimension,
                    TEST_TOLERANCE_PERCENT,
                    TEST_EPSILON,
                    "NFE %d; failed to match target for `buffer_lfm_condition`",
                    i);
            pv_ypu_mem_release_host_view(ypu, m2, false);
            pv_ypu_mem_free(ypu, m2);
        } else if (!mock_lfm_condition_fuser) {
            pv_ypu_mem_free(ypu, m2);
            free(state);
            free(buffer_lfm_x_t);

            return status;
        }

        pv_ypu_mem_t *m3 = pv_ypu_mem_alloc(
                ypu,
                TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_SIZE_LIST[i_input_offset + 3],
                PV_YPU_DEVICE_MEM_FLAG_NONE);
        pv_test_true(m3 != NULL, "Failed to allocate m3");
        if (m3 == NULL) {
            return PV_STATUS_OUT_OF_MEMORY;
        }
        status = pv_ypu_mem_copy_to(
                ypu,
                m3,
                TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_LIST[i_input_offset + 3],
                0,
                TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_SIZE_LIST[i_input_offset + 3]);
        pv_test_true(
                status == PV_STATUS_SUCCESS,
                "pv_ypu_mem_copy_to failed with %s",
                pv_status_to_string(status));

        pv_ypu_mem_t *m4 = pv_ypu_mem_alloc(
                ypu,
                TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_SIZE_LIST[i_input_offset + 2],
                PV_YPU_DEVICE_MEM_FLAG_NONE);
        pv_test_true(m4 != NULL, "Failed to allocate m4");
        if (m4 == NULL) {
            return PV_STATUS_OUT_OF_MEMORY;
        }
        status = pv_ypu_mem_copy_to(
                ypu,
                m4,
                TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_LIST[i_input_offset + 2],
                0,
                TEST_ORCA_LFM_NOISE_PREDICTION_INPUT_SIZE_LIST[i_input_offset + 2]);
        pv_test_true(
                status == PV_STATUS_SUCCESS,
                "pv_ypu_mem_copy_to failed with %s",
                pv_status_to_string(status));

        pv_ypu_mem_t *m5 = pv_ypu_mem_alloc(
                ypu,
                T * dimension * sizeof(float),
                PV_YPU_DEVICE_MEM_FLAG_NONE);
        pv_test_true(m5 != NULL, "Failed to allocate m5");
        if (m5 == NULL) {
            return PV_STATUS_OUT_OF_MEMORY;
        }

        status = pv_orca_lfm_vf_estimator_forward(
                ypu,
                orca_lfm_vf_estimator_object,
                state,
                T,
                m3,
                m4,
                m5,
                0,
                0,
                0);
        pv_ypu_mem_free(ypu, m3);
        pv_ypu_mem_free(ypu, m4);

        float *buffer_lfm_velocity_pred = pv_ypu_mem_get_host_view(ypu, m5, true);
        if (test_accuracy) {
            pv_test_true(status == PV_STATUS_SUCCESS, "pv_orca_lfm_vf_estimator_forward() returns failed status");

            pv_test_close_float_array(
                    buffer_lfm_velocity_pred,
                    TEST_ORCA_LFM_NOISE_PREDICTION_TARGET_LIST[i_output_offset + 1],
                    T * out_dimension,
                    TEST_TOLERANCE_PERCENT,
                    TEST_EPSILON,
                    "NFE %d; failed to match target for `buffer_lfm_velocity_pred`",
                    i);
        } else {
            pv_ypu_mem_release_host_view(ypu, m5, false);
            pv_ypu_mem_free(ypu, m5);
            free(state);
            free(buffer_lfm_x_t);

            return status;
        }

        for (int32_t frame = 0; frame < T; ++frame) {
            const int32_t frame_offset = frame * out_dimension;

            for (int32_t i_c = 0; i_c < out_dimension; ++i_c) {
                buffer_lfm_x_t[frame_offset + i_c] += dt * buffer_lfm_velocity_pred[frame_offset + i_c];
            }
        }

        pv_test_close_float_array(
                buffer_lfm_x_t,
                TEST_ORCA_LFM_NOISE_PREDICTION_TARGET_LIST[i_output_offset + 2],
                T * out_dimension,
                TEST_TOLERANCE_PERCENT,
                TEST_EPSILON,
                "NFE %d; failed to match target for `buffer_lfm_x_t`",
                i);

        pv_ypu_mem_release_host_view(ypu, m5, false);
        pv_ypu_mem_free(ypu, m5);
    }

    free(state);
    free(buffer_lfm_x_t);

    return PV_STATUS_SUCCESS;
}

static void test_pv_orca_lfm_noise_prediction_forward(void) {
    test_pv_orca_lfm_noise_prediction_forward_inner(
            true,
            false);
}

static const pv_test_case_t PV_ORCA_LFM_NOISE_PREDICTION_TEST_CASES[] = {
        {"orca_lfm_noise_prediction forward", test_pv_orca_lfm_noise_prediction_forward},
};

const pv_test_suite_t PV_ORCA_LFM_NOISE_PREDICTION_TEST_SUITE = {
        .name = "orca_lfm_noise_prediction",
        .setup = test_pv_orca_lfm_noise_prediction_setup,
        .teardown = test_pv_orca_lfm_noise_prediction_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_ORCA_LFM_NOISE_PREDICTION_TEST_CASES),
        .test_cases = PV_ORCA_LFM_NOISE_PREDICTION_TEST_CASES,
};
