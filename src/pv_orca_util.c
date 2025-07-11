#include <string.h>

#include "core/pv_error_messages.h"
#include "orca/pv_orca_util.h"
#include "orca/pv_profiler.h"
#include "math/pv_math.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

#define EULER_NUMBER_F (2.71828182846)

void PV_MOCKABLE(pv_orca_util_expand_tokens_to_frames)(
        int32_t num_tokens,
        int32_t num_channels,
        const int32_t *durations,
        const float *means_enc,
        const float *logs_enc,
        float *means,
        float *logs) {
    PV_ASSERT(num_tokens);
    PV_ASSERT(num_channels);
    PV_ASSERT(durations);
    PV_ASSERT(means_enc);
    PV_ASSERT(logs_enc);
    PV_ASSERT(means);
    PV_ASSERT(logs);

    int32_t cumulative_duration = 0;

    for (int32_t i = 0; i < num_tokens; i++) {
        const float *means_enc_i = means_enc + (i * num_channels);
        const float *logs_enc_i = logs_enc + (i * num_channels);
        float *m = means + (cumulative_duration * num_channels);
        float *s = logs + (cumulative_duration * num_channels);

        for (int32_t j = 0; j < durations[i]; j++) {
            float *mm = m + (j * num_channels);
            float *ss = s + (j * num_channels);

            for (int32_t k = 0; k < num_channels; k++) {
                mm[k] = means_enc_i[k];
                ss[k] = logs_enc_i[k];
            }
        }
        cumulative_duration += durations[i];
    }
}

void PV_MOCKABLE(pv_orca_util_split_channels)(
        int32_t n,
        int32_t input_channels,
        const float *x,
        float *y0,
        float *y1) {
    PV_ASSERT(n);
    PV_ASSERT(x);
    PV_ASSERT(input_channels > 0);
    PV_ASSERT(input_channels % 2 == 0);
    PV_ASSERT(y0);
    PV_ASSERT(y1);

    int32_t half_channels = input_channels / 2;
    for (int32_t i = 0; i < n; i++) {
        const float *xx = x + (i * (input_channels));
        float *yy0 = y0 + (i * half_channels);
        float *yy1 = y1 + (i * half_channels);

        memcpy(yy0, xx, half_channels * sizeof(float));
        memcpy(yy1, xx + half_channels, half_channels * sizeof(float));
    }
}

void PV_MOCKABLE(pv_orca_util_concatenate_channel_wise)(
        int32_t n,
        int32_t half_channels,
        const float *x0,
        const float *x1,
        float *y) {
    PV_ASSERT(n);
    PV_ASSERT(half_channels);
    PV_ASSERT(x0);
    PV_ASSERT(x1);
    PV_ASSERT(y);

    for (int32_t frame = 0; frame < n; frame++) {
        const int32_t half_frame_offset = frame * half_channels;
        const int32_t full_frame_offset = 2 * frame * half_channels;
        for (int32_t c = 0; c < half_channels; c++) {
            y[full_frame_offset + c] = x0[half_frame_offset + c];
            y[full_frame_offset + half_channels + c] = x1[half_frame_offset + c];
        }
    }
}


float sigmoid_float(float x) {
    return (1 / (1 + powf((float) EULER_NUMBER_F, -x)));
}

void PV_MOCKABLE(pv_orca_util_fused_tanh_sigmoid_multiply)(
        int32_t n,
        int32_t num_channels,
        const float *x,
        float *y) {
    PV_ASSERT(n);
    PV_ASSERT(num_channels % 2 == 0);
    PV_ASSERT(x);
    PV_ASSERT(y);
    PV_ORCA_PROFILER_START("fused_tanh_sigmoid_multiply");

    int32_t out_channels = num_channels / 2;

#ifndef __ORCA_FLOAT_MODE__

    q15_t activation_tanh = 0;
    q15_t activation_sigmoid = 0;

#endif

    for (int32_t i = 0; i < n; i++) {
        const float *xx = x + (i * num_channels);

        const float *input_tanh = xx;
        const float *input_sigmoid = xx + out_channels;

        float *yy = y + (i * out_channels);

        for (int32_t j = 0; j < out_channels; j++) {

#ifdef __ORCA_FLOAT_MODE__

            yy[j] = tanhf(input_tanh[j]) * sigmoid_float(input_sigmoid[j]);

#else

            pv_tanh_scalar(pv_float_to_q510(input_tanh[j]), &activation_tanh);
            pv_sigmoid_scalar(pv_float_to_q510(input_sigmoid[j]), &activation_sigmoid);
            yy[j] = pv_q15_to_float(activation_tanh) * pv_q15_to_float(activation_sigmoid);

#endif

        }
    }
    PV_ORCA_PROFILER_STOP("fused_tanh_sigmoid_multiply");
}

pv_status_t PV_MOCKABLE(pv_orca_util_sample_latents)(
        int32_t n,
        int32_t num_channels,
        float noise_scale,
        int64_t random_state,
        const float *means,
        const float *logs,
        float *y) {
    PV_ASSERT(n);
    PV_ASSERT(num_channels);
    PV_ASSERT(noise_scale > 0);
    PV_ASSERT(means);
    PV_ASSERT(logs);
    PV_ASSERT(y);

    uint64_t *state = (uint64_t *) &random_state;

    pv_orca_util_rand_normal_t *rand_normal = NULL;
    pv_status_t status = pv_orca_util_rand_normal_init(&(rand_normal));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_module_function_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("pv_orca_util_rand_normal_init"));
        return status;
    }

    for (int32_t i = 0; i < n; i++) {
        const int32_t frame_offset = i * num_channels;

        for (int32_t j = 0; j < num_channels; j++) {
            float r = pv_orca_util_rand_normal_sample(rand_normal, state);
            float random_factor = ((r * expf(logs[frame_offset + j])) * noise_scale);
            y[frame_offset + j] = means[frame_offset + j] + random_factor;
        }
    }

    pv_orca_util_rand_normal_delete(rand_normal);

    return PV_STATUS_SUCCESS;
}

int32_t PV_MOCKABLE(pv_orca_clip_int32)(int32_t n, int32_t min, int32_t max) {
    return (n < min) ? min : (n > max) ? max : n;
}

pv_status_t PV_MOCKABLE(pv_orca_util_rand_normal_init)(pv_orca_util_rand_normal_t **object) {
    PV_ASSERT(object);

    *object = NULL;

    pv_orca_util_rand_normal_t *o = calloc(1, sizeof(pv_orca_util_rand_normal_t));
    if (!o) {
        return PV_STATUS_OUT_OF_MEMORY;
    }
    o->has_value = false;

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_orca_util_rand_normal_delete)(pv_orca_util_rand_normal_t *object) {
    if (object) {
        free(object);
    }
}

float PV_MOCKABLE(pv_orca_util_rand_normal_sample)(pv_orca_util_rand_normal_t *object, uint64_t *state) {
    PV_ASSERT(object);
    PV_ASSERT(state);

    if (object->has_value) {
        object->has_value = false;
        return object->value;
    }

    float ru1 = 0.0f;
    float ru2 = 0.0f;
    float rr = 0.0f;
    while (rr >= 1.0 || rr == 0.0) {
        ru1 =  (2.0f * (float) pv_rand_uniform(state)) - 1.0f;
        ru2 =  (2.0f * (float) pv_rand_uniform(state)) - 1.0f;
        rr = ru1 * ru1 + ru2 * ru2;
    }

    float f = sqrtf(-2.0f * (logf(rr) / rr));
    float rand_normal1 = ru1 * f;
    float rand_normal2 = ru2 * f;

    object->value = rand_normal2;
    object->has_value = true;

    return rand_normal1;
}

void PV_MOCKABLE(pv_orca_util_scale_and_quantize_activation)(
        int32_t num_elements,
        int32_t padding_numel,
        const float *x,
        q510_t *y,
        float *inverse_scale) {
    PV_ASSERT(num_elements);
    PV_ASSERT(padding_numel >= 0);
    PV_ASSERT(x);
    PV_ASSERT(y);
    PV_ASSERT(inverse_scale);

    float max = 0.f;
    for (int32_t i = 0; i < num_elements; i++) {
        if (fabsf(x[i]) > max) {
            max = fabsf(x[i]);
        }
    }
    const float scale = 31.f / max;

    for (int32_t i = padding_numel; i < num_elements + padding_numel; i++) {
        y[i] = pv_float_to_q510(x[i - padding_numel] * scale);
    }

    // divided by 1024 * 128 = 131072 to account for q510 input (scaled by 1024) and q7 weight (scaled by 128)
    *inverse_scale = 1.f / (131072.f * scale);
}
