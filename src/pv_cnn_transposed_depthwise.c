#include <stdlib.h>
#include <string.h>

#include "core/pv_error_messages.h"
#include "orca/pv_affine.h"
#include "orca/pv_cnn.h"
#include "orca/pv_cnn_transposed_depthwise.h"
#include "util/pv_check_status.h"
#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_cnn_transposed_depthwise_param_serialize)(
        pv_ypu_t *ypu,
        const pv_cnn_transposed_depthwise_param_t *param,
        FILE *file) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(file);

    return pv_cnn_depthwise_param_serialize(
            ypu,
            (pv_cnn_depthwise_param_t *) param,
            file);
}

#endif

pv_status_t PV_MOCKABLE(pv_cnn_transposed_depthwise_param_load)(
        pv_ypu_t *ypu,
        FILE *f,
        pv_cnn_transposed_depthwise_param_t **param) {
    PV_ASSERT(ypu);
    PV_ASSERT(f);
    PV_ASSERT(param);

    return pv_cnn_depthwise_param_load(
            ypu,
            f,
            (pv_cnn_depthwise_param_t **) param);
}

void PV_MOCKABLE(pv_cnn_transposed_depthwise_param_delete)(
        pv_ypu_t *ypu,
        pv_cnn_transposed_depthwise_param_t *param) {
    pv_cnn_depthwise_param_delete(ypu, (pv_cnn_depthwise_param_t *) param);
}

bool PV_MOCKABLE(pv_cnn_transposed_depthwise_param_is_equal)(
        const pv_cnn_transposed_depthwise_param_t *object,
        const pv_cnn_transposed_depthwise_param_t *other) {
    PV_ASSERT(object);
    PV_ASSERT(other);

    return pv_cnn_depthwise_param_is_equal(
            (const pv_cnn_depthwise_param_t *) object,
            (const pv_cnn_depthwise_param_t *) other);
}

struct pv_cnn_transposed_depthwise {
    const pv_cnn_transposed_depthwise_param_t *param;

    pv_ypu_mem_t *weight;
    pv_ypu_mem_t *bias;
    pv_ypu_mem_t *transposed_weight;
    pv_ypu_mem_t *int8_inverse_scale;

    int32_t cache_length;
    pv_ypu_mem_t *cache;
    int32_t lookahead;
    int32_t lookback;
};

pv_status_t PV_MOCKABLE(pv_cnn_transposed_depthwise_init)(
        pv_ypu_t *ypu,
        const pv_cnn_transposed_depthwise_param_t *param,
        pv_cnn_transposed_depthwise_t **object) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(object);

    *object = NULL;

    pv_cnn_transposed_depthwise_t *o = pv_ypu_host_alloc(ypu, sizeof(pv_cnn_transposed_depthwise_t));
    if (!o) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_host_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(o, 0, sizeof(pv_cnn_transposed_depthwise_t));

    o->param = param;

    o->weight = pv_ypu_mem_from_config(ypu, param->weight);
    if (!o->weight) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o->weight"));
        pv_cnn_transposed_depthwise_delete(ypu, o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->bias = pv_ypu_mem_from_config(ypu, param->bias);
    if (!o->bias) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o->bias"));
        pv_cnn_transposed_depthwise_delete(ypu, o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->int8_inverse_scale = pv_ypu_mem_from_config(ypu, param->int8_inverse_scale);
    if (!o->int8_inverse_scale) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o->int8_inverse_scale"));
        pv_cnn_transposed_depthwise_delete(ypu, o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    int32_t kernel_size = pv_cnn_transposed_depthwise_kernel_size(o);
    int32_t num_channels = pv_cnn_transposed_depthwise_num_channels(o);

    if (kernel_size > 1) {
        o->transposed_weight = pv_ypu_mem_alloc(
                ypu,
                num_channels * kernel_size * (int32_t) sizeof(q7_t),
                PV_YPU_DEVICE_MEM_FLAG_NONE);
        if (!o->transposed_weight) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("o->transposed_weight"));
            pv_cnn_transposed_depthwise_delete(ypu, o);
            return PV_STATUS_OUT_OF_MEMORY;
        }

        pv_ypu_op_transpose_args_t args = {
                .output = o->transposed_weight,
                .input = o->weight,
                .b = 1,
                .m = kernel_size,
                .n = num_channels,
                .k = sizeof(q7_t),
                .output_offset = 0,
                .input_offset = 0,
        };
        pv_status_t status = pv_ypu_operator_execute(
                ypu,
                PV_YPU_OPERATOR_TRANSPOSE,
                &args);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT(
                    &pv_error_msg_ypu_operator_fail,
                    PV_ERROR_ARGS_PUBLIC("execute"),
                    PV_ERROR_ARGS_PRIVATE(
                            "execute",
                            pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_TRANSPOSE),
                            pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                            pv_status_to_string(status)));
            pv_cnn_transposed_depthwise_delete(ypu, o);
            return status;
        }
    } else {
        o->transposed_weight = NULL;
    }

    o->lookahead = 2;
    o->lookback = 2;
    o->cache = pv_ypu_mem_alloc(
            ypu,
            (o->lookahead + o->lookback) * num_channels * (int32_t) sizeof(float),
            PV_YPU_DEVICE_MEM_FLAG_NONE);
    if (!o->cache) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o->cache"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->cache_length = 0;

    pv_ypu_op_memset_args_t args = {
            .output = o->cache,
            .size_bytes = (o->lookahead + o->lookback) * num_channels * (int32_t) sizeof(float),
            0,
    };
    pv_status_t status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_MEMSET,
            &args);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_MEMSET),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        return status;
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_cnn_transposed_depthwise_delete)(
        pv_ypu_t *ypu,
        pv_cnn_transposed_depthwise_t *object) {
    PV_ASSERT(ypu);

    if (object) {
        if (object->transposed_weight) {
            pv_ypu_mem_free(ypu, object->transposed_weight);
        }

        pv_ypu_mem_free(ypu, object->weight);
        pv_ypu_mem_free(ypu, object->bias);
        pv_ypu_mem_free(ypu, object->int8_inverse_scale);
        pv_ypu_mem_free(ypu, object->cache);

        pv_ypu_host_free(ypu, object);
    }
}

pv_status_t PV_MOCKABLE(pv_cnn_transposed_depthwise_reset_cache)(
        pv_ypu_t *ypu,
        pv_cnn_transposed_depthwise_t *object) {
    PV_ASSERT(ypu);
    PV_ASSERT(object);
    PV_ASSERT(object->cache);

    int32_t num_channels = pv_cnn_transposed_depthwise_num_channels(object);

    pv_ypu_op_memset_args_t args = {
            .output = object->cache,
            .size_bytes = (object->lookahead + object->lookback) * num_channels * (int32_t) sizeof(float),
            0,
    };
    pv_status_t status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_MEMSET,
            &args);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_MEMSET),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        return status;
    }

    object->cache_length = 0;

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_cnn_transposed_depthwise_forward)(
        pv_ypu_t *ypu,
        pv_cnn_transposed_depthwise_t *object,
        int32_t n,
        pv_ypu_mem_t *x_ypu,
        pv_ypu_mem_t *y_ypu,
        int32_t x_offset,
        int32_t y_offset) {
    PV_ASSERT(ypu);
    PV_ASSERT(object);
    PV_ASSERT(n);
    PV_ASSERT(x_ypu);
    PV_ASSERT(y_ypu);

    int32_t num_channels = pv_cnn_depthwise_num_channels((pv_cnn_depthwise_t *) object);
    int32_t kernel_size = object->param->kernel_size;
    int32_t padding = object->param->padding;
    const int32_t num_output_frames = pv_cnn_transposed_depthwise_num_output_frames(object, n);
    const int32_t num_output_elements_padded = (num_output_frames + 2 * padding) * num_channels;
    int32_t stride = object->param->stride;
    const int32_t padding_num_elements = padding * num_channels;

#ifdef __ORCA_FLOAT_MODE__

    float *x = (float *) (pv_ypu_mem_get_host_view(ypu, x_ypu, false) + x_offset);
    float *y = (float *) (pv_ypu_mem_get_host_view(ypu, y_ypu, false) + y_offset);
    float *int8_inverse_scale = (float *) pv_ypu_mem_get_host_view(ypu, object->int8_inverse_scale, false);
    float *bias = (float *) pv_ypu_mem_get_host_view(ypu, object->bias, false);
    q7_t *weight = (q7_t *) pv_ypu_mem_get_host_view(ypu, pv_cnn_depthwise_get_weight((pv_cnn_depthwise_t *) object), false);

    float *buffer = calloc(
            num_output_elements_padded,
            sizeof(float));
    if (!buffer) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    for (int32_t frame = 0; frame < n; frame++) {
        const int32_t frame_offset = frame * num_channels;
        const int32_t frame_offset_strided = frame * num_channels * stride;

        for (int32_t ke = 0; ke < kernel_size; ke++) {
            const int32_t kernel_offset = ke * num_channels;

            for (int32_t nc = 0; nc < num_channels; nc++) {
                buffer[frame_offset_strided + kernel_offset + nc] += x[frame_offset + nc] * (((float) weight[kernel_offset + nc]) / 128.0f);
            }
        }
    }

    pv_status_t status = pv_affine_execute_float(
            num_output_frames,
            num_channels,
            buffer + padding_num_elements,
            1.0f,
            0.0f,
            int8_inverse_scale,
            bias,
            y);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_affine_execute_float,
                pv_status_to_string(status));
        free(buffer);
        return status;
    }

    free(buffer);

#else

    const int32_t num_input_elements = n * num_channels;

    pv_ypu_mem_t *x_q510 = pv_ypu_buffer_get(
            ypu,
            num_input_elements * (int32_t) sizeof(q510_t),
            false);
    if (!x_q510) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("x_q510"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_mem_t *buffer = pv_ypu_buffer_get(
            ypu,
            num_output_elements_padded * (int32_t) sizeof(int32_t),
            false);
    if (!buffer) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("buffer"));
        pv_ypu_buffer_release(ypu, x_q510);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_mem_t *inverse_scale = pv_ypu_buffer_get(
            ypu,
            sizeof(float),
            false);
    if (!inverse_scale) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("inverse_scale"));
        pv_ypu_buffer_release(ypu, buffer);
        pv_ypu_buffer_release(ypu, x_q510);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_op_quantize_args_t args_quantize_q510 = {
            .output = x_q510,
            .scale = inverse_scale,
            .input = x_ypu,
            .m = 1,
            .n = num_input_elements,
            .output_offset = 0,
            .scale_offset = 0,
            .input_offset = x_offset,
    };
    pv_status_t status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_QUANTIZE_Q510,
            &args_quantize_q510);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_QUANTIZE_Q510),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        pv_ypu_buffer_release(ypu, inverse_scale);
        pv_ypu_buffer_release(ypu, buffer);
        pv_ypu_buffer_release(ypu, x_q510);
        return status;
    }

    pv_ypu_op_memset_args_t args_memset = {
            .output = buffer,
            .size_bytes = num_output_elements_padded * (int32_t) sizeof(int32_t),
            .output_offset = 0,
    };
    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_MEMSET,
            &args_memset);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_MEMSET),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        pv_ypu_buffer_release(ypu, inverse_scale);
        pv_ypu_buffer_release(ypu, buffer);
        pv_ypu_buffer_release(ypu, x_q510);
        return status;
    }

    pv_ypu_op_conv1d_transposed_depthwise_args_t args_conv1d_transposed = {
            .output = buffer,
            .lhs = pv_cnn_depthwise_get_weight((pv_cnn_depthwise_t *) object),
            .rhs = x_q510,
            .n = n,
            .num_channels = num_channels,
            .kernel_size = kernel_size,
            .stride = stride,
            .output_offset = 0,
            .lhs_offset = 0,
            .rhs_offset = 0,
    };
    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_CONV1D_TRANSPOSED_DEPTHWISE_Q1417_Q7_Q510,
            &args_conv1d_transposed);
    pv_ypu_buffer_release(ypu, x_q510);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_CONV1D_TRANSPOSED_DEPTHWISE_Q1417_Q7_Q510),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        pv_ypu_buffer_release(ypu, inverse_scale);
        pv_ypu_buffer_release(ypu, buffer);
        return status;
    }

    pv_ypu_op_elementwise_args_t args_convert_f32_i32 = {
            .output = y_ypu,
            .input = buffer,
            .length = num_output_frames * num_channels,
            .output_offset = y_offset,
            .input_offset = padding_num_elements * (int32_t) sizeof(int32_t),
    };
    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_CONVERT_F32_I32,
            &args_convert_f32_i32);
    pv_ypu_buffer_release(ypu, buffer);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_CONVERT_F32_I32),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        pv_ypu_buffer_release(ypu, inverse_scale);
        return status;
    }

    status = pv_affine_execute_from_q1417_to_float(
            ypu,
            num_output_frames,
            num_channels,
            y_ypu,
            inverse_scale,
            object->int8_inverse_scale,
            object->bias,
            y_ypu,
            y_offset,
            0,
            0,
            0,
            y_offset);
    pv_ypu_buffer_release(ypu, inverse_scale);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_affine_execute_from_q1417_to_float,
                pv_status_to_string(status));
        return status;
    }

#endif

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_cnn_transposed_depthwise_forward_with_cache)(
        pv_ypu_t *ypu,
        pv_cnn_transposed_depthwise_t *object,
        int32_t n,
        pv_ypu_mem_t *x_ypu,
        pv_ypu_mem_t *y_ypu,
        int32_t x_offset,
        int32_t y_offset,
        bool is_flush,
        int32_t *n_out) {
    PV_ASSERT(ypu);
    PV_ASSERT(object);
    PV_ASSERT(n);
    PV_ASSERT(x_ypu);
    PV_ASSERT(y_ypu);
    PV_ASSERT(n_out);

    int32_t num_channels = pv_cnn_depthwise_num_channels((pv_cnn_depthwise_t *) object);
    int32_t kernel_size = object->param->kernel_size;
    int32_t padding = object->param->padding;
    int32_t stride = object->param->stride;

    int32_t n_extended = n;
    n_extended += object->cache_length;

    bool is_first = (object->cache_length == 0);
    int32_t output_offset = 0;
    if (is_first & is_flush) {
        output_offset = padding * num_channels;
    } else if (is_first & !is_flush) {
        output_offset = padding * num_channels;
    } else if (!is_first & is_flush) {
        output_offset = (padding + stride * (object->cache_length - 2)) * num_channels;
    } else {
        output_offset = (padding + stride * (object->cache_length - 2)) * num_channels;
    }

    int32_t output_length = pv_cnn_transposed_depthwise_num_output_frames(object, n_extended) + 2 * padding;
    int32_t output_length_final;
    if (is_first & is_flush) {
        output_length_final = stride * n_extended;
    } else if (is_first & !is_flush) {
        output_length_final = stride * (n - object->lookahead);
    } else if (!is_first & is_flush) {
        output_length_final = stride * (n + object->lookahead);
    } else {
        output_length_final = stride * n;
    }

    int32_t output_size = output_length * num_channels;
    *n_out = output_length_final;

    int32_t input_cache_append_length = pv_min_int32(object->lookback + object->lookahead, n);
    int32_t old_cache_append_length = pv_min_int32((object->lookback + object->lookahead) - input_cache_append_length, object->cache_length);

#ifdef __ORCA_FLOAT_MODE__

    float *y = (float *) (pv_ypu_mem_get_host_view(ypu, y_ypu, false) + y_offset);
    float *int8_inverse_scale = (float *) pv_ypu_mem_get_host_view(ypu, object->int8_inverse_scale, false);
    float *bias = (float *) pv_ypu_mem_get_host_view(ypu, object->bias, false);
    q7_t *weight = (q7_t *) pv_ypu_mem_get_host_view(ypu, pv_cnn_depthwise_get_weight((pv_cnn_depthwise_t *) object), false);

    float *x_extended = calloc(
            n_extended * num_channels,
            sizeof(float));
    if (!x_extended) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    if (object->cache_length > 0) {
        pv_status_t status = pv_ypu_mem_copy_from(
                ypu,
                object->cache,
                x_extended,
                0,
                object->cache_length * num_channels * (int32_t) sizeof(float));
        if (status != PV_STATUS_SUCCESS) {
            return status;
        }
    }
    if (n > 0) {
        pv_status_t status = pv_ypu_mem_copy_from(
                ypu,
                x_ypu,
                x_extended + object->cache_length * num_channels,
                x_offset,
                n * num_channels * (int32_t) sizeof(float));
        if (status != PV_STATUS_SUCCESS) {
            return status;
        }
    }

    if (old_cache_append_length > 0) {
        pv_ypu_op_memmove_args_t args = {
                .output = object->cache,
                .batch_size = 1,
                .size_bytes = object->cache_length * num_channels * (int32_t) sizeof(float),
                .input_offset = (object->cache_length - old_cache_append_length) * num_channels * (int32_t) sizeof(float),
        };
        pv_status_t status = pv_ypu_operator_execute(
                ypu,
                PV_YPU_OPERATOR_MEMMOVE,
                &args);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT(
                    &pv_error_msg_ypu_operator_fail,
                    PV_ERROR_ARGS_PUBLIC("execute"),
                    PV_ERROR_ARGS_PRIVATE(
                            "execute",
                            pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_MEMMOVE),
                            pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                            pv_status_to_string(status)));
            return status;
        }
    }

    if (input_cache_append_length > 0) {
        pv_ypu_op_memcpy_args_t args = {
                .output = object->cache,
                .input = x_ypu,
                .size_bytes = input_cache_append_length * num_channels * (int32_t) sizeof(float),
                .output_offset = old_cache_append_length * num_channels * (int32_t) sizeof(float),
                .input_offset = x_offset + (n - input_cache_append_length) * num_channels * (int32_t) sizeof(float),
        };
        pv_status_t status = pv_ypu_operator_execute(
                ypu,
                PV_YPU_OPERATOR_MEMCPY,
                &args);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT(
                    &pv_error_msg_ypu_operator_fail,
                    PV_ERROR_ARGS_PUBLIC("execute"),
                    PV_ERROR_ARGS_PRIVATE(
                            "execute",
                            pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_MEMCPY),
                            pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                            pv_status_to_string(status)));
            return status;
        }
    }

    object->cache_length = input_cache_append_length + old_cache_append_length;

    float *buffer = calloc(
            output_size,
            sizeof(float));
    if (!buffer) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    for (int32_t frame = 0; frame < n_extended; frame++) {
        const int32_t frame_offset = frame * num_channels;
        const int32_t frame_offset_strided = frame * num_channels * stride;

        for (int32_t ke = 0; ke < kernel_size; ke++) {
            const int32_t kernel_offset = ke * num_channels;

            for (int32_t nc = 0; nc < num_channels; nc++) {
                buffer[frame_offset_strided + kernel_offset + nc] += x_extended[frame_offset + nc] * (((float) weight[kernel_offset + nc]) / 128.0f);
            }
        }
    }

    free(x_extended);
    x_extended = NULL;

    pv_status_t status = pv_affine_execute_float(
            output_length_final,
            num_channels,
            buffer + output_offset,
            1.0f,
            0.0f,
            int8_inverse_scale,
            bias,
            y);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_affine_execute_float,
                pv_status_to_string(status));
        free(buffer);
        return status;
    }

    free(buffer);

#else

    pv_ypu_mem_t *x_q510 = pv_ypu_buffer_get(
            ypu,
            n_extended * num_channels * (int32_t) sizeof(q510_t),
            false);
    if (!x_q510) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("x_q510"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_mem_t *inverse_scale = pv_ypu_buffer_get(
            ypu,
            sizeof(float),
            false);
    if (!inverse_scale) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("inverse_scale"));
        pv_ypu_buffer_release(ypu, x_q510);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_mem_t *x_extended = pv_ypu_buffer_get(
            ypu,
            (n_extended * num_channels) * (int32_t) sizeof(float),
            false);
    if (!x_extended) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("x_extended"));
        pv_ypu_buffer_release(ypu, inverse_scale);
        pv_ypu_buffer_release(ypu, x_q510);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_op_memset_args_t args_memset_0 = {
            .output = x_extended,
            .size_bytes = (n_extended * num_channels) * (int32_t) sizeof(float),
            .output_offset = 0,
    };
    pv_status_t status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_MEMSET,
            &args_memset_0);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_MEMSET),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        pv_ypu_buffer_release(ypu, x_extended);
        pv_ypu_buffer_release(ypu, inverse_scale);
        pv_ypu_buffer_release(ypu, x_q510);
        return status;
    }

    if (object->cache_length > 0) {
        pv_ypu_op_memcpy_args_t args = {
                .output = x_extended,
                .input = object->cache,
                .size_bytes = object->cache_length * num_channels * (int32_t) sizeof(float),
                .output_offset = 0,
                .input_offset = 0,
        };
        pv_status_t status = pv_ypu_operator_execute(
                ypu,
                PV_YPU_OPERATOR_MEMCPY,
                &args);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT(
                    &pv_error_msg_ypu_operator_fail,
                    PV_ERROR_ARGS_PUBLIC("execute"),
                    PV_ERROR_ARGS_PRIVATE(
                            "execute",
                            pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_MEMCPY),
                            pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                            pv_status_to_string(status)));
            pv_ypu_buffer_release(ypu, x_extended);
            pv_ypu_buffer_release(ypu, inverse_scale);
            pv_ypu_buffer_release(ypu, x_q510);
            return status;
        }
    }
    if (n > 0) {
        pv_ypu_op_memcpy_args_t args = {
                .output = x_extended,
                .input = x_ypu,
                .size_bytes = n * num_channels * (int32_t) sizeof(float),
                .output_offset = object->cache_length * num_channels * (int32_t) sizeof(float),
                .input_offset = x_offset,
        };
        pv_status_t status = pv_ypu_operator_execute(
                ypu,
                PV_YPU_OPERATOR_MEMCPY,
                &args);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT(
                    &pv_error_msg_ypu_operator_fail,
                    PV_ERROR_ARGS_PUBLIC("execute"),
                    PV_ERROR_ARGS_PRIVATE(
                            "execute",
                            pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_MEMCPY),
                            pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                            pv_status_to_string(status)));
            pv_ypu_buffer_release(ypu, x_extended);
            pv_ypu_buffer_release(ypu, inverse_scale);
            pv_ypu_buffer_release(ypu, x_q510);
            return status;
        }
    }

    if (old_cache_append_length > 0) {
        pv_ypu_op_memmove_args_t args = {
                .output = object->cache,
                .batch_size = 1,
                .size_bytes = object->cache_length * num_channels * (int32_t) sizeof(float),
                .input_offset = (object->cache_length - old_cache_append_length) * num_channels * (int32_t) sizeof(float),
        };
        pv_status_t status = pv_ypu_operator_execute(
                ypu,
                PV_YPU_OPERATOR_MEMMOVE,
                &args);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT(
                    &pv_error_msg_ypu_operator_fail,
                    PV_ERROR_ARGS_PUBLIC("execute"),
                    PV_ERROR_ARGS_PRIVATE(
                            "execute",
                            pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_MEMMOVE),
                            pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                            pv_status_to_string(status)));
            pv_ypu_buffer_release(ypu, x_extended);
            pv_ypu_buffer_release(ypu, inverse_scale);
            pv_ypu_buffer_release(ypu, x_q510);
            return status;
        }
    }

    if (input_cache_append_length > 0) {
        pv_ypu_op_memcpy_args_t args = {
                .output = object->cache,
                .input = x_ypu,
                .size_bytes = input_cache_append_length * num_channels * (int32_t) sizeof(float),
                .output_offset = old_cache_append_length * num_channels * (int32_t) sizeof(float),
                .input_offset = x_offset + (n - input_cache_append_length) * num_channels * (int32_t) sizeof(float),
        };
        pv_status_t status = pv_ypu_operator_execute(
                ypu,
                PV_YPU_OPERATOR_MEMCPY,
                &args);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT(
                    &pv_error_msg_ypu_operator_fail,
                    PV_ERROR_ARGS_PUBLIC("execute"),
                    PV_ERROR_ARGS_PRIVATE(
                            "execute",
                            pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_MEMCPY),
                            pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                            pv_status_to_string(status)));
            pv_ypu_buffer_release(ypu, x_extended);
            pv_ypu_buffer_release(ypu, inverse_scale);
            pv_ypu_buffer_release(ypu, x_q510);
            return status;
        }
    }

    object->cache_length = input_cache_append_length + old_cache_append_length;

    pv_ypu_op_quantize_args_t args_quantize_q510 = {
            .output = x_q510,
            .scale = inverse_scale,
            .input = x_extended,
            .m = 1,
            .n = n_extended * num_channels,
            .output_offset = 0,
            .scale_offset = 0,
            .input_offset = 0,
    };
    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_QUANTIZE_Q510,
            &args_quantize_q510);
    pv_ypu_buffer_release(ypu, x_extended);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_QUANTIZE_Q510),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        pv_ypu_buffer_release(ypu, inverse_scale);
        pv_ypu_buffer_release(ypu, x_q510);
        return status;
    }

    pv_ypu_mem_t *buffer = pv_ypu_buffer_get(
            ypu,
            output_size * (int32_t) sizeof(int32_t),
            false);
    if (!buffer) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("buffer"));
        pv_ypu_buffer_release(ypu, x_q510);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_op_memset_args_t args_memset_1 = {
            .output = buffer,
            .size_bytes = output_size * (int32_t) sizeof(int32_t),
            .output_offset = 0,
    };
    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_MEMSET,
            &args_memset_1);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_MEMSET),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        pv_ypu_buffer_release(ypu, inverse_scale);
        pv_ypu_buffer_release(ypu, buffer);
        pv_ypu_buffer_release(ypu, x_q510);
        return status;
    }

    pv_ypu_op_conv1d_transposed_depthwise_args_t args_conv1d_transposed = {
            .output = buffer,
            .lhs = pv_cnn_depthwise_get_weight((pv_cnn_depthwise_t *) object),
            .rhs = x_q510,
            .n = n_extended,
            .num_channels = num_channels,
            .kernel_size = kernel_size,
            .stride = stride,
            .output_offset = 0,
            .lhs_offset = 0,
            .rhs_offset = 0,
    };
    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_CONV1D_TRANSPOSED_DEPTHWISE_Q1417_Q7_Q510,
            &args_conv1d_transposed);
    pv_ypu_buffer_release(ypu, x_q510);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_CONV1D_TRANSPOSED_DEPTHWISE_Q1417_Q7_Q510),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        pv_ypu_buffer_release(ypu, inverse_scale);
        pv_ypu_buffer_release(ypu, buffer);
        return status;
    }

    pv_ypu_op_elementwise_args_t args_convert_f32_i32 = {
            .output = y_ypu,
            .input = buffer,
            .length = output_length_final * num_channels,
            .output_offset = y_offset,
            .input_offset = output_offset * (int32_t) sizeof(int32_t),
    };
    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_CONVERT_F32_I32,
            &args_convert_f32_i32);
    pv_ypu_buffer_release(ypu, buffer);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_CONVERT_F32_I32),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        pv_ypu_buffer_release(ypu, inverse_scale);
        return status;
    }

    status = pv_affine_execute_from_q1417_to_float(
            ypu,
            output_length_final,
            num_channels,
            y_ypu,
            inverse_scale,
            object->int8_inverse_scale,
            object->bias,
            y_ypu,
            y_offset,
            0,
            0,
            0,
            y_offset);
    pv_ypu_buffer_release(ypu, inverse_scale);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_affine_execute_from_q1417_to_float,
                pv_status_to_string(status));
        return status;
    }

#endif

    return PV_STATUS_SUCCESS;
}

int32_t PV_MOCKABLE(pv_cnn_transposed_depthwise_num_output_frames)(
        const pv_cnn_transposed_depthwise_t *object,
        int32_t n) {
    PV_ASSERT(object);
    PV_ASSERT(n > 0);

    return (((n - 1) * object->param->stride) + object->param->kernel_size) - (2 * object->param->padding);
}

int32_t PV_MOCKABLE(pv_cnn_transposed_depthwise_kernel_size)(const pv_cnn_transposed_depthwise_t *object) {
    PV_ASSERT(object);

    return object->param->kernel_size;
}

int32_t PV_MOCKABLE(pv_cnn_transposed_depthwise_num_channels)(const pv_cnn_transposed_depthwise_t *object) {
    PV_ASSERT(object);

    return object->param->num_channels;
}

int32_t PV_MOCKABLE(pv_cnn_transposed_depthwise_padding)(const pv_cnn_transposed_depthwise_t *object) {
    PV_ASSERT(object);

    return object->param->padding;
}

int32_t PV_MOCKABLE(pv_cnn_transposed_depthwise_cache_length)(const pv_cnn_transposed_depthwise_t *object) {
    PV_ASSERT(object);

    return object->cache_length;
}
