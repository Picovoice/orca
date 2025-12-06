#include <string.h>
#include <math.h>

#include "test/pv_test.h"

#include "orca/pv_orca_util.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

static pv_ypu_t *ypu = NULL;

static pv_status_t test_pv_util_setup(void) {
    pv_status_t status = pv_ypu_init_cpu(1, &ypu);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_util_teardown(void) {
    pv_ypu_delete(ypu);
}

static void test_pv_orca_util_expand_tokens_to_frames(void) {
    int32_t num_tokens = 3;
    int32_t hidden_channels = 2;
    const int32_t durations[] = {2, 3, 1};
    const int32_t num_frames = 6;
    float mean_enc[] = {0.1f, 0.2f, 0.3f, 0.05f, 0.1f, 0.1f};
    float logs_enc[] = {0.3f, 0.4f, 0.25f, 0.1f, 0.2f, 0.2f};
    float target_means[] = {0.1f, 0.2f, 0.1f, 0.2f, 0.3f, 0.05f, 0.3f, 0.05f, 0.3f, 0.05f, 0.1f, 0.1f};
    float target_logs[] = {0.3f, 0.4f, 0.3f, 0.4f, 0.25f, 0.1f, 0.25f, 0.1f, 0.25f, 0.1f, 0.2f, 0.2f};

    pv_ypu_mem_t *m0 = pv_ypu_mem_alloc(
        ypu,
        sizeof(mean_enc),
        PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m0 != NULL, "Failed to allocate m0");
    if (m0 == NULL) {
        return;
    }

    pv_ypu_mem_t *m1 = pv_ypu_mem_alloc(
        ypu,
        sizeof(logs_enc),
        PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m1 != NULL, "Failed to allocate m1");
    if (m1 == NULL) {
        return;
    }

    pv_ypu_mem_t *means = pv_ypu_mem_alloc(
        ypu,
        num_frames * hidden_channels * sizeof(float),
        PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(means != NULL, "Failed to allocate means");
    if (means == NULL) {
        return;
    }

    pv_ypu_mem_t *logs = pv_ypu_mem_alloc(
        ypu,
        num_frames * hidden_channels * sizeof(float),
        PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(logs != NULL, "Failed to allocate logs");
    if (logs == NULL) {
        return;
    }

    pv_status_t status = pv_ypu_mem_copy_to(
        ypu,
        m0,
        mean_enc,
        0,
        sizeof(mean_enc));
    pv_test_true(
        status == PV_STATUS_SUCCESS,
        "pv_ypu_mem_copy_to failed with %s",
        pv_status_to_string(status));

    status = pv_ypu_mem_copy_to(
        ypu,
        m1,
        logs_enc,
        0,
        sizeof(logs_enc));
    pv_test_true(
        status == PV_STATUS_SUCCESS,
        "pv_ypu_mem_copy_to failed with %s",
        pv_status_to_string(status));

    status = pv_orca_util_expand_tokens_to_frames(
            ypu,
            num_tokens,
            hidden_channels,
            durations,
            m0,
            m1,
            means,
            logs,
            0,
            0,
            0,
            0);
    pv_test_true(
        status == PV_STATUS_SUCCESS,
        "pv_orca_util_expand_tokens_to_frames failed with %s",
        pv_status_to_string(status));

    float *h0 = pv_ypu_mem_get_host_view(ypu, means, true);
    float *h1 = pv_ypu_mem_get_host_view(ypu, logs, true);
    for (int32_t n = 0; n < num_frames; n++) {
        pv_test_true(h0[n] == target_means[n], "mean mismatch, got %f, expected %f", h0[n], target_means[n]);
        pv_test_true(h1[n] == target_logs[n], "logs mismatch, got %f, expected %f", h1[n], target_logs[n]);
    }
    pv_ypu_mem_release_host_view(ypu, means, false);
    pv_ypu_mem_release_host_view(ypu, logs, false);

    pv_ypu_mem_free(ypu, m0);
    pv_ypu_mem_free(ypu, m1);
    pv_ypu_mem_free(ypu, means);
    pv_ypu_mem_free(ypu, logs);
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

    pv_ypu_mem_t *m0 = pv_ypu_mem_alloc(
        ypu,
        sizeof(x),
        PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m0 != NULL, "Failed to allocate m0");
    if (m0 == NULL) {
        return;
    }

    pv_ypu_mem_t *m1 = pv_ypu_mem_alloc(
        ypu,
        sizeof(y0),
        PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m1 != NULL, "Failed to allocate m1");
    if (m1 == NULL) {
        return;
    }

    pv_ypu_mem_t *m2 = pv_ypu_mem_alloc(
        ypu,
        sizeof(y1),
        PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m2 != NULL, "Failed to allocate m2");
    if (m2 == NULL) {
        return;
    }

    pv_status_t status = pv_ypu_mem_copy_to(
        ypu,
        m0,
        x,
        0,
        sizeof(x));
    pv_test_true(
        status == PV_STATUS_SUCCESS,
        "pv_ypu_mem_copy_to failed with %s",
        pv_status_to_string(status));

    status = pv_orca_util_split_channels(
        ypu,
        n,
        num_channels,
        m0,
        m1,
        m2,
        0,
        0,
        0);
    pv_test_true(
        status == PV_STATUS_SUCCESS,
        "pv_orca_util_split_channels failed with %s",
        pv_status_to_string(status));

    status = pv_ypu_mem_copy_from(
        ypu,
        m1,
        y0,
        0,
        sizeof(y0));
    pv_test_true(
        status == PV_STATUS_SUCCESS,
        "pv_ypu_mem_copy_from failed with %s",
        pv_status_to_string(status));

    pv_ypu_mem_copy_from(
        ypu,
        m2,
        y1,
        0,
        sizeof(y1));
    pv_test_true(
        status == PV_STATUS_SUCCESS,
        "pv_ypu_mem_copy_from failed with %s",
        pv_status_to_string(status));

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

    pv_ypu_mem_free(ypu, m0);
    pv_ypu_mem_free(ypu, m1);
    pv_ypu_mem_free(ypu, m2);
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

    pv_ypu_mem_t *m0 = pv_ypu_mem_alloc(
        ypu,
        sizeof(x),
        PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m0 != NULL, "Failed to allocate m0");
    if (m0 == NULL) {
        return;
    }

    pv_ypu_mem_t *m1 = pv_ypu_mem_alloc(
        ypu,
        sizeof(y),
        PV_YPU_DEVICE_MEM_FLAG_NONE);
    pv_test_true(m1 != NULL, "Failed to allocate m1");
    if (m1 == NULL) {
        return;
    }

    pv_status_t status = pv_ypu_mem_copy_to(
        ypu,
        m0,
        x,
        0,
        sizeof(x));
    pv_test_true(
        status == PV_STATUS_SUCCESS,
        "pv_ypu_mem_copy_to failed with %s",
        pv_status_to_string(status));

    status = pv_ypu_mem_copy_to(
        ypu,
        m1,
        y,
        0,
        sizeof(y));
    pv_test_true(
        status == PV_STATUS_SUCCESS,
        "pv_ypu_mem_copy_to failed with %s",
        pv_status_to_string(status));

    status = pv_orca_util_fused_tanh_sigmoid_multiply(
        ypu,
        n,
        num_channels,
        m0,
        m1,
        0,
        0);
    pv_test_true(
        status == PV_STATUS_SUCCESS,
        "pv_orca_util_fused_tanh_sigmoid_multiply failed with %s",
        pv_status_to_string(status));

    status = pv_ypu_mem_copy_from(
        ypu,
        m0,
        x,
        0,
        sizeof(x));
    pv_test_true(
        status == PV_STATUS_SUCCESS,
        "pv_ypu_mem_copy_from failed with %s",
        pv_status_to_string(status));

    status = pv_ypu_mem_copy_from(
        ypu,
        m1,
        y,
        0,
        sizeof(y));
    pv_test_true(
        status == PV_STATUS_SUCCESS,
        "pv_ypu_mem_copy_from failed with %s",
        pv_status_to_string(status));

    pv_test_close_float_array(y, y_target, n * (num_channels / 2), 0.01f, 0.01f, "y mismatch");

    pv_ypu_mem_free(ypu, m0);
    pv_ypu_mem_free(ypu, m1);
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
        .setup = test_pv_util_setup,
        .teardown = test_pv_util_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_ORCA_UTIL_TEST_CASES),
        .test_cases = PV_ORCA_UTIL_TEST_CASES,
};
