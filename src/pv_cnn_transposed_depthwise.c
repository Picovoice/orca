#include <stdlib.h>

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

/* layout-compatible with pv_cnn_depthwise_t */
struct pv_cnn_transposed_depthwise {
    const pv_cnn_transposed_depthwise_param_t *param;

    pv_ypu_mem_t *weight;
    pv_ypu_mem_t *bias;
    pv_ypu_mem_t *transposed_weight;
    pv_ypu_mem_t *int8_inverse_scale;
};

pv_status_t PV_MOCKABLE(pv_cnn_transposed_depthwise_init)(
        pv_ypu_t *ypu,
        const pv_cnn_transposed_depthwise_param_t *param,
        pv_cnn_transposed_depthwise_t **object) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(object);

    return pv_cnn_depthwise_init(
            ypu,
            (const pv_cnn_depthwise_param_t *) param,
            (pv_cnn_depthwise_t **) object);
}

void PV_MOCKABLE(pv_cnn_transposed_depthwise_delete)(
        pv_ypu_t *ypu,
        pv_cnn_transposed_depthwise_t *object) {
    PV_ASSERT(ypu);

    return pv_cnn_depthwise_delete(
            ypu,
            (pv_cnn_depthwise_t *) object);
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
            .output_offset = 0
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

int32_t PV_MOCKABLE(pv_cnn_transposed_depthwise_num_output_frames)(
        const pv_cnn_transposed_depthwise_t *object,
        int32_t n) {
    PV_ASSERT(object);
    PV_ASSERT(n > 0);

    return (((n - 1) * object->param->stride) + object->param->kernel_size) - (2 * object->param->padding);
}
