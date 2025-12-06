#include <string.h>

#include "core/pv_error_messages.h"
#include "math/pv_math.h"
#include "orca/pv_orca_util.h"
#include "orca/pv_profiler.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

#define EULER_NUMBER_F (2.71828182846)

pv_status_t PV_MOCKABLE(pv_orca_util_expand_tokens_to_frames)(
        pv_ypu_t *ypu,
        int32_t num_tokens,
        int32_t num_channels,
        const int32_t *durations,
        pv_ypu_mem_t *means_enc_ypu_mem,
        pv_ypu_mem_t *logs_enc_ypu_mem,
        pv_ypu_mem_t *means_ypu_mem,
        pv_ypu_mem_t *logs_ypu_mem,
        int32_t means_enc_offset,
        int32_t logs_enc_offset,
        int32_t means_offset,
        int32_t logs_offset) {
    PV_ASSERT(ypu);
    PV_ASSERT(num_tokens);
    PV_ASSERT(num_channels);
    PV_ASSERT(durations);
    PV_ASSERT(means_enc_ypu_mem);
    PV_ASSERT(logs_enc_ypu_mem);
    PV_ASSERT(means_ypu_mem);
    PV_ASSERT(logs_ypu_mem);

    float *means_enc = pv_ypu_mem_get_host_view(ypu, means_enc_ypu_mem, true) + means_enc_offset;
    float *logs_enc = pv_ypu_mem_get_host_view(ypu, logs_enc_ypu_mem, true) + logs_enc_offset;
    float *means = pv_ypu_mem_get_host_view(ypu, means_ypu_mem, true) + means_offset;
    float *logs = pv_ypu_mem_get_host_view(ypu, logs_ypu_mem, true) + logs_offset;

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

    pv_ypu_mem_release_host_view(ypu, means_enc_ypu_mem, true);
    pv_ypu_mem_release_host_view(ypu, logs_enc_ypu_mem, true);
    pv_ypu_mem_release_host_view(ypu, means_ypu_mem, true);
    pv_ypu_mem_release_host_view(ypu, logs_ypu_mem, true);

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_orca_util_split_channels)(
        pv_ypu_t *ypu,
        int32_t n,
        int32_t input_channels,
        pv_ypu_mem_t *x_ypu_mem,
        pv_ypu_mem_t *y0_ypu_mem,
        pv_ypu_mem_t *y1_ypu_mem,
        int32_t x_offset,
        int32_t y0_offset,
        int32_t y1_offset) {
    PV_ASSERT(ypu);
    PV_ASSERT(n);
    PV_ASSERT(x_ypu_mem);
    PV_ASSERT(input_channels > 0);
    PV_ASSERT(input_channels % 2 == 0);
    PV_ASSERT(y0_ypu_mem);
    PV_ASSERT(y1_ypu_mem);

    int32_t half_channels = input_channels / 2;
    for (int32_t i = 0; i < n; i++) {
        pv_ypu_op_memcpy_args_t args0 = {
                .output = y0_ypu_mem,
                .input = x_ypu_mem,
                .size_bytes = half_channels * (int32_t) sizeof(float),
                .output_offset = y0_offset + i * half_channels * (int32_t) sizeof(float),
                .input_offset = x_offset + i * input_channels * (int32_t) sizeof(float),
        };

        pv_status_t status = pv_ypu_operator_execute(
                ypu,
                PV_YPU_OPERATOR_MEMCPY,
                &args0);
        if (status != PV_STATUS_SUCCESS) {
            return status;
        }

        pv_ypu_op_memcpy_args_t args1 = {
                .output = y1_ypu_mem,
                .input = x_ypu_mem,
                .size_bytes = half_channels * (int32_t) sizeof(float),
                .output_offset = y1_offset + i * half_channels * (int32_t) sizeof(float),
                .input_offset = x_offset + (i * input_channels + half_channels) * (int32_t) sizeof(float),
        };

        status = pv_ypu_operator_execute(
                ypu,
                PV_YPU_OPERATOR_MEMCPY,
                &args1);
        if (status != PV_STATUS_SUCCESS) {
            return status;
        }
    }

    return PV_STATUS_SUCCESS;
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

pv_status_t PV_MOCKABLE(pv_orca_util_fused_tanh_sigmoid_multiply)(
        pv_ypu_t *ypu,
        int32_t n,
        int32_t num_channels,
        pv_ypu_mem_t *x_ypu_mem,
        pv_ypu_mem_t *y_ypu_mem,
        int32_t x_offset,
        int32_t y_offset) {
    PV_ASSERT(ypu);
    PV_ASSERT(n);
    PV_ASSERT(num_channels % 2 == 0);
    PV_ASSERT(x_ypu_mem);
    PV_ASSERT(y_ypu_mem);
    PV_ORCA_PROFILER_START("fused_tanh_sigmoid_multiply");

    pv_ypu_mem_t *buffer = pv_ypu_buffer_get(
            ypu,
            num_channels * (int32_t) sizeof(float),
            false);
    if (buffer == NULL) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    int32_t out_channels = num_channels / 2;

    for (int32_t i = 0; i < n; i++) {
        pv_ypu_op_elementwise_args_t args0 = {
                .output = buffer,
                .input = x_ypu_mem,
                .length = out_channels,
                .output_offset = 0,
                .input_offset = x_offset + i * num_channels * (int32_t) sizeof(float),
        };

        pv_status_t status = pv_ypu_operator_execute(
                ypu,
                PV_YPU_OPERATOR_TANH,
                &args0);
        if (status != PV_STATUS_SUCCESS) {
            return status;
        }

        pv_ypu_op_elementwise_args_t args1 = {
                .output = buffer,
                .input = x_ypu_mem,
                .length = out_channels,
                .output_offset = out_channels * (int32_t) sizeof(float),
                .input_offset = x_offset + (i * num_channels + out_channels) * (int32_t) sizeof(float),
        };

        status = pv_ypu_operator_execute(ypu, PV_YPU_OPERATOR_SIGMOID, &args1);
        if (status != PV_STATUS_SUCCESS) {
            return status;
        }

        pv_ypu_op_pairwise_args_t args2 = {
                .output = y_ypu_mem,
                .lhs = buffer,
                .rhs = buffer,
                .length = out_channels,
                .output_offset = y_offset + (i * out_channels) * (int32_t) sizeof(float),
                .lhs_offset = 0,
                .rhs_offset = out_channels * (int32_t) sizeof(float),
        };

        status = pv_ypu_operator_execute(
                ypu,
                PV_YPU_OPERATOR_MUL,
                &args2);
        if (status != PV_STATUS_SUCCESS) {
            return status;
        }
    }
    PV_ORCA_PROFILER_STOP("fused_tanh_sigmoid_multiply");

    pv_ypu_buffer_release(ypu, buffer);

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_orca_util_sample_latents)(
        pv_ypu_t *ypu,
        int32_t n,
        int32_t num_channels,
        float noise_scale,
        int64_t random_state,
        pv_ypu_mem_t *means_ypu_mem,
        pv_ypu_mem_t *logs_ypu_mem,
        pv_ypu_mem_t *y_ypu_mem) {
    PV_ASSERT(n);
    PV_ASSERT(num_channels);
    PV_ASSERT(noise_scale > 0);
    PV_ASSERT(means_ypu_mem);
    PV_ASSERT(logs_ypu_mem);
    PV_ASSERT(y_ypu_mem);

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

    float *y = pv_ypu_mem_get_host_view(ypu, y_ypu_mem, true);
    float *means = pv_ypu_mem_get_host_view(ypu, means_ypu_mem, true);
    float *logs = pv_ypu_mem_get_host_view(ypu, logs_ypu_mem, true);

    for (int32_t i = 0; i < n; i++) {
        const int32_t frame_offset = i * num_channels;

        for (int32_t j = 0; j < num_channels; j++) {
            float r = pv_orca_util_rand_normal_sample(rand_normal, state);
            float random_factor = ((r * expf(logs[frame_offset + j])) * noise_scale);
            y[frame_offset + j] = means[frame_offset + j] + random_factor;
        }
    }

    pv_ypu_mem_release_host_view(ypu, logs_ypu_mem, true);
    pv_ypu_mem_release_host_view(ypu, means_ypu_mem, true);
    pv_ypu_mem_release_host_view(ypu, y_ypu_mem, true);

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
        ru1 = (2.0f * (float) pv_rand_uniform(state)) - 1.0f;
        ru2 = (2.0f * (float) pv_rand_uniform(state)) - 1.0f;
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
