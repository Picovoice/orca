#include <stdlib.h>

#include "core/picovoice.h"
#include "core/pv_type.h"
#include "test/pv_test.h"

#include "test_data/test_pv_orca_flow_data.c"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

static pv_orca_flow_t *orca_flow_object = NULL;

static pv_status_t test_pv_orca_flow_setup(void) {
    pv_status_t status = pv_orca_flow_init(&FLOW_PARAM, &orca_flow_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_orca_flow_teardown(void) {
    pv_orca_flow_delete(orca_flow_object);
    orca_flow_object = NULL;
}

static void test_pv_orca_flow_forward(void) {
    pv_test_true(orca_flow_object != NULL, "failed to create tmp file");

    int32_t num_channels = PV_ARRAY_LEN(TEST_ORCA_FLOW_TARGET) / TEST_ORCA_FLOW_SEQUENCE_LENGTH;
    float *buffer = calloc(TEST_ORCA_FLOW_SEQUENCE_LENGTH, num_channels * sizeof(float));

    pv_orca_flow_forward(
            orca_flow_object,
            TEST_ORCA_FLOW_SEQUENCE_LENGTH,
            TEST_ORCA_FLOW_INPUT,
            buffer);
    pv_test_close_float_array(
            buffer,
            TEST_ORCA_FLOW_TARGET,
            TEST_ORCA_FLOW_SEQUENCE_LENGTH * num_channels,
            0.05f,
            0.01f,
            "failed to forward orca_flow");

    free(buffer);
}

#ifdef __PV_MOCKS__

static void test_pv_orca_flow_init_failure_helper(pv_status_t expected) {
    pv_orca_flow_t *object = NULL;
    pv_status_t status = pv_orca_flow_init(&FLOW_PARAM, &object);
    pv_test_true(
            status == expected,
            "orca flow init error, got `%s` expected `%s`",
            pv_status_to_string(status),
            pv_status_to_string(expected));
    if (object) {
        pv_orca_flow_delete(object);
    }
}

static void test_pv_orca_flow_init_failure_1(void) {
    pv_status_t custom_rets[] = {
            PV_STATUS_INVALID_ARGUMENT,
    };
    PV_SET_MOCK_RETURN_SEQ(pv_buffer_init, custom_rets);

    test_pv_orca_flow_init_failure_helper(PV_STATUS_INVALID_ARGUMENT);
}

static void test_pv_orca_flow_init_failure_2(void) {
    pv_status_t custom_rets[] = {
            PV_STATUS_SUCCESS,
            PV_STATUS_INVALID_ARGUMENT,
    };
    PV_SET_MOCK_RETURN_SEQ(pv_buffer_init, custom_rets);

    test_pv_orca_flow_init_failure_helper(PV_STATUS_INVALID_ARGUMENT);
}

static void test_pv_orca_flow_init_failure_3(void) {
    pv_status_t custom_rets[] = {
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_INVALID_ARGUMENT,
    };
    PV_SET_MOCK_RETURN_SEQ(pv_buffer_init, custom_rets);

    test_pv_orca_flow_init_failure_helper(PV_STATUS_INVALID_ARGUMENT);
}

static void test_pv_orca_flow_init_failure_4(void) {
    pv_status_t custom_rets[] = {
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_INVALID_ARGUMENT,
    };
    PV_SET_MOCK_RETURN_SEQ(pv_buffer_init, custom_rets);

    test_pv_orca_flow_init_failure_helper(PV_STATUS_INVALID_ARGUMENT);
}

static void test_pv_orca_flow_init_failure_5(void) {
    pv_status_t custom_rets[] = {
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_INVALID_ARGUMENT,
    };
    PV_SET_MOCK_RETURN_SEQ(pv_buffer_init, custom_rets);

    test_pv_orca_flow_init_failure_helper(PV_STATUS_INVALID_ARGUMENT);
}

static void test_pv_orca_flow_init_failure_6(void) {
    pv_status_t custom_rets[] = {
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_INVALID_ARGUMENT,
    };
    PV_SET_MOCK_RETURN_SEQ(pv_buffer_init, custom_rets);

    test_pv_orca_flow_init_failure_helper(PV_STATUS_INVALID_ARGUMENT);
}

static void test_pv_orca_flow_init_failure_7(void) {
    pv_status_t custom_rets[] = {
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_INVALID_ARGUMENT,
    };
    PV_SET_MOCK_RETURN_SEQ(pv_buffer_init, custom_rets);

    test_pv_orca_flow_init_failure_helper(PV_STATUS_INVALID_ARGUMENT);
}

static void test_pv_orca_flow_init_failure_8(void) {
    pv_status_t custom_rets[] = {
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_INVALID_ARGUMENT,
    };
    PV_SET_MOCK_RETURN_SEQ(pv_buffer_init, custom_rets);

    test_pv_orca_flow_init_failure_helper(PV_STATUS_INVALID_ARGUMENT);
}

static void test_pv_orca_flow_init_failure_9(void) {
    pv_status_t custom_rets[] = {
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_INVALID_ARGUMENT,
    };
    PV_SET_MOCK_RETURN_SEQ(pv_buffer_init, custom_rets);

    test_pv_orca_flow_init_failure_helper(PV_STATUS_INVALID_ARGUMENT);
}

static void test_pv_orca_flow_init_failure_10(void) {
    pv_status_t custom_rets[] = {
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_INVALID_ARGUMENT,
    };
    PV_SET_MOCK_RETURN_SEQ(pv_buffer_init, custom_rets);

    test_pv_orca_flow_init_failure_helper(PV_STATUS_INVALID_ARGUMENT);
}

#endif

static const pv_test_case_t PV_ORCA_FLOW_TEST_CASES[] = {
        {"orca_flow forward", test_pv_orca_flow_forward},

#ifdef __PV_MOCKS__

        {"orca_flow init failure 1", test_pv_orca_flow_init_failure_1},
        {"orca_flow init failure 2", test_pv_orca_flow_init_failure_2},
        {"orca_flow init failure 3", test_pv_orca_flow_init_failure_3},
        {"orca_flow init failure 4", test_pv_orca_flow_init_failure_4},
        {"orca_flow init failure 5", test_pv_orca_flow_init_failure_5},
        {"orca_flow init failure 6", test_pv_orca_flow_init_failure_6},
        {"orca_flow init failure 7", test_pv_orca_flow_init_failure_7},
        {"orca_flow init failure 8", test_pv_orca_flow_init_failure_8},
        {"orca_flow init failure 9", test_pv_orca_flow_init_failure_9},
        {"orca_flow init failure 10", test_pv_orca_flow_init_failure_10}

#endif

};

const pv_test_suite_t PV_ORCA_FLOW_TEST_SUITE = {
        .name = "orca_flow",
        .setup = test_pv_orca_flow_setup,
        .teardown = test_pv_orca_flow_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_ORCA_FLOW_TEST_CASES),
        .test_cases = PV_ORCA_FLOW_TEST_CASES,
};
