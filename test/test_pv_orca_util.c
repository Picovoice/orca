#include <string.h>
#include <math.h>

#include "test/pv_test.h"

#include "orca/pv_orca_util.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif


static void test_pv_orca_util_rand_normal(void) {
    pv_orca_util_rand_normal_t *object;
    uint64_t state = 0;
    pv_status_t status = pv_orca_util_rand_normal_init(&object);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to initialize pv_orca_util_rand_normal_t");

    const int32_t num_samples = 10000;
    float random_numbers[num_samples];

    float sum = 0.0f;
    for (int32_t i = 0; i < num_samples; i++) {
        float x = pv_orca_util_rand_normal_sample(object, &state);
        random_numbers[i] = x;
        sum += x;
    }
    float mean = sum / (float) num_samples;

    float sum_2 = 0.0f;
    for (int i = 0; i < num_samples; i++) {
        sum_2 += (random_numbers[i] - mean) * (random_numbers[i] - mean);
    }
    float std = sqrtf(sum_2 / (float) num_samples);

    pv_test_close_float((1.f + mean), 1.00f, 0.03f, "incorrect mean");
    pv_test_close_float(std, 1.f, 0.03f, "incorrect standard deviation");

    pv_orca_util_rand_normal_delete(object);
}

static const pv_test_case_t PV_ORCA_UTIL_TEST_CASES[] = {
        {"rand normal",                 test_pv_orca_util_rand_normal},
};

const pv_test_suite_t PV_ORCA_UTIL_TEST_SUITE = {
        .name = "orca_util",
        .setup = NULL,
        .teardown = NULL,
        .num_test_cases = PV_ARRAY_LEN(PV_ORCA_UTIL_TEST_CASES),
        .test_cases = PV_ORCA_UTIL_TEST_CASES,
};
