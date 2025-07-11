#include <string.h>
#include <math.h>

#include "test/pv_test.h"

#include "orca/pv_orca_util.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

static void test_pv_orca_util_expand_tokens_to_frames(void) {
    int32_t num_tokens = 3;
    int32_t hidden_channels = 2;
    const int32_t durations[] = {2, 3, 1};
    const int32_t num_frames = 6;
    float mean_enc[] = {0.1f, 0.2f, 0.3f, 0.05f, 0.1f, 0.1f};
    float logs_enc[] = {0.3f, 0.4f, 0.25f, 0.1f, 0.2f, 0.2f};
    float target_means[] = {0.1f, 0.2f, 0.1f, 0.2f, 0.3f, 0.05f, 0.3f, 0.05f, 0.3f, 0.05f, 0.1f, 0.1f};
    float target_logs[] = {0.3f, 0.4f, 0.3f, 0.4f, 0.25f, 0.1f, 0.25f, 0.1f, 0.25f, 0.1f, 0.2f, 0.2f};
    float *means = calloc(num_frames * hidden_channels, sizeof(float));
    pv_test_true(means, "Failed to allocate `means`");
    if (!means) {
        return;
    }
    float *logs = calloc(num_frames * hidden_channels, sizeof(float));
    pv_test_true(logs, "Failed to allocate `logs`");
    if (!logs) {
        return;
    }
    pv_orca_util_expand_tokens_to_frames(
            num_tokens,
            hidden_channels,
            durations,
            mean_enc,
            logs_enc,
            means,
            logs);
    for (int32_t n = 0; n < num_frames; n++) {
        pv_test_true(means[n] == target_means[n], "mean mismatch, got %f, expected %f", means[n], target_means[n]);
        pv_test_true(logs[n] == target_logs[n], "logs mismatch, got %f, expected %f", logs[n], target_logs[n]);
    }
    free(means);
    free(logs);
}

static void test_pv_orca_util_split_channels(void) {
    int32_t n = 3;
    const int32_t num_channels = 4;
    const int32_t half_channels = num_channels / 2;
    float x[] = {
            0.1f, 0.2f, 0.5f, 0.7f,
            0.1f, 0.4f, 0.5f, 0.9f,
            0.2f, 0.1f, 0.6f, 0.7f,
    };
    float y0_target[] = {
            0.1f, 0.2f,
            0.1f, 0.4f,
            0.2f, 0.1f,
    };
    float y1_target[] = {
            0.5f, 0.7f,
            0.5f, 0.9f,
            0.6f, 0.7f,
    };

    float y0[n * half_channels];
    float y1[n * half_channels];
    pv_orca_util_split_channels(n, num_channels, x, y0, y1);

    for (int32_t i = 0; i < n; i++) {
        for (int32_t j = 0; j < half_channels; j++) {
            pv_test_true(
                    y0[(i * half_channels) + j] == y0_target[(i * half_channels) + j],
                    "y0 mismatch: got `%f`, expected `%f`",
                    y0[(i * half_channels) + j],
                    y0_target[(i * half_channels) + j]);
            pv_test_true(
                    y1[(i * half_channels) + j] == y1_target[(i * half_channels) + j],
                    "y1 mismatch: got `%f`, expected `%f`",
                    y1[(i * half_channels) + j],
                    y1_target[(i * half_channels) + j]);
        }
    }
}

static void test_pv_orca_util_concatenate_channel_wise(void) {
    int32_t n = 3;
    const int32_t num_channels = 2;
    float x0[] = {
            0.1f, 0.2f,
            0.1f, 0.4f,
            0.2f, 0.1f,
    };
    float x1[] = {
            0.5f, 0.7f,
            0.5f, 0.9f,
            0.6f, 0.7f,
    };
    float y_target[] = {
            0.1f, 0.2f, 0.5f, 0.7f,
            0.1f, 0.4f, 0.5f, 0.9f,
            0.2f, 0.1f, 0.6f, 0.7f,
    };
    float y[2 * n * num_channels];
    pv_orca_util_concatenate_channel_wise(n, num_channels, x0, x1, y);

    for (int32_t i = 0; i < 2 * n * num_channels; i++) {
        pv_test_true(y[i] == y_target[i], "y mismatch: got `%f`, expected `%f`", y[i], y_target[i]);
    }
}

static void test_pv_orca_util_fused_tanh_sigmoid_multiply(void) {
    const int32_t n = 2;
    const int32_t num_channels = 2;
    float x[] = {
            0.1f, 0.2f,
            0.3f, 0.4f,
    };
    float y[n * num_channels];

    float y_target[] = {0.0548f, 0.1744f};

    pv_orca_util_fused_tanh_sigmoid_multiply(n, num_channels, x, y);

    pv_test_close_float_array(y, y_target, n * (num_channels / 2), 0.01f, 0.01f, "y mismatch");
}

static void test_pv_orca_clip_int32(void) {
    const int32_t X[] = {1, 5, 2, 7, 6, 9};
    int32_t min = 3;
    int32_t max = 6;
    const int32_t Y[] = {3, 5, 3, 6, 6, 6};

    const int32_t length = PV_ARRAY_LEN(X);

    for (int32_t i = 0; i < length; i++) {
        pv_test_true(pv_orca_clip_int32(X[i], min, max) == Y[i], "incorrect clipping");
    }
}

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
        {"expand_tokens_to_frames",     test_pv_orca_util_expand_tokens_to_frames},
        {"split_channels",              test_pv_orca_util_split_channels},
        {"concatenate_channel_wise",    test_pv_orca_util_concatenate_channel_wise},
        {"fused_tanh_sigmoid_multiply", test_pv_orca_util_fused_tanh_sigmoid_multiply},
        {"clip_int32",                  test_pv_orca_clip_int32},
        {"rand normal",                 test_pv_orca_util_rand_normal},
};

const pv_test_suite_t PV_ORCA_UTIL_TEST_SUITE = {
        .name = "orca_util",
        .setup = NULL,
        .teardown = NULL,
        .num_test_cases = PV_ARRAY_LEN(PV_ORCA_UTIL_TEST_CASES),
        .test_cases = PV_ORCA_UTIL_TEST_CASES,
};
