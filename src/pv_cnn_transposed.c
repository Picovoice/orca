#include <stdlib.h>

#include "core/pv_error_messages.h"
#include "orca/pv_cnn.h"
#include "orca/pv_cnn_transposed.h"
#include "orca/pv_orca_util.h"
#include "orca/pv_profiler.h"
#include "util/pv_check_status.h"
#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"
#include "ypu/mock/pv_ypu_mock.h"

#endif

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_cnn_transposed_depthwise_param_serialize)(
        pv_ypu_t *ypu,
        const pv_cnn_transposed_depthwise_param_t *param,
        FILE *file) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(file);

    return pv_cnn_depthwise_param_serialize(ypu, (pv_cnn_depthwise_param_t *) param, file);
}

#endif

pv_status_t PV_MOCKABLE(pv_cnn_transposed_depthwise_param_load)(
        pv_ypu_t *ypu,
        FILE *f,
        pv_cnn_transposed_depthwise_param_t **param) {
    PV_ASSERT(ypu);
    PV_ASSERT(f);
    PV_ASSERT(param);

    return pv_cnn_depthwise_param_load(ypu, f, (pv_cnn_depthwise_param_t **) param);
}

void PV_MOCKABLE(pv_cnn_transposed_depthwise_param_delete)(
        pv_ypu_t *ypu,
        pv_cnn_transposed_depthwise_param_t *param) {
    PV_ASSERT(ypu);

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

int32_t PV_MOCKABLE(pv_cnn_transposed_depthwise_param_receptive_field)(
        const pv_cnn_transposed_depthwise_param_t *object) {
    PV_ASSERT(object);

    PV_ASSERT((object->kernel_size - object->stride) % 2 == 0);

    return (object->kernel_size - object->stride) / 2;
}

/* layout-compatible with pv_cnn_depthwise_t */
struct pv_cnn_transposed_depthwise {
    const pv_cnn_transposed_depthwise_param_t *param;

    pv_ypu_mem_t *weight;
    pv_ypu_mem_t *bias;
    pv_ypu_mem_t *transposed_weight;
    pv_ypu_mem_t *bias_float;
};

static pv_status_t pv_cnn_transposed_depthwise_forward_float_to_float(
        pv_ypu_t *ypu,
        pv_cnn_transposed_depthwise_t *object,
        int32_t n,
        pv_ypu_mem_t *x_ypu_mem,
        pv_ypu_mem_t *y_ypu_mem,
        int32_t x_offset,
        int32_t y_offset) {
    PV_ASSERT(ypu);
    PV_ASSERT(object);
    PV_ASSERT(n);
    PV_ASSERT(x_ypu_mem);
    PV_ASSERT(y_ypu_mem);
    PV_ORCA_PROFILER_START("cnn_transposed_depthwise");

    int32_t num_channels = pv_cnn_depthwise_num_channels((pv_cnn_depthwise_t *) object);
    int32_t kernel_size = object->param->kernel_size;
    int32_t padding = object->param->padding;
    const int32_t num_input_elements = n * num_channels;
    const int32_t num_output_frames = pv_cnn_transposed_depthwise_num_output_frames(object, n);
    const int32_t num_output_elements_padded = (num_output_frames + 2 * padding) * num_channels;
    int32_t stride = object->param->stride;
    const int32_t padding_numel = padding * num_channels;

    pv_ypu_mem_t *x_q510_ypu_mem = pv_ypu_buffer_get(
            ypu,
            num_input_elements * (int32_t) sizeof(q510_t),
            false);
    if (!x_q510_ypu_mem) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_mem_t *buffer_ypu_mem = pv_ypu_buffer_get(
            ypu,
            num_output_elements_padded * (int32_t) sizeof(int32_t),
            false);
    if (!buffer_ypu_mem) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_mem_t *inverse_scale = pv_ypu_buffer_get(
            ypu,
            sizeof(float),
            false);
    if (!inverse_scale) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_op_quantize_args_t args0 = {
            .output = x_q510_ypu_mem,
            .scale = inverse_scale,
            .input = x_ypu_mem,
            .m = 1,
            .n = num_input_elements,
            .output_offset = 0,
            .scale_offset = 0,
            .input_offset = x_offset,
    };

    pv_status_t status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_QUANTIZE_Q510,
            &args0);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_op_memset_args_t args1 = {
            .output = buffer_ypu_mem,
            .size_bytes = num_output_elements_padded * (int32_t) sizeof(int32_t),
            .output_offset = 0};

    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_MEMSET,
            &args1);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_op_conv1d_transposed_depthwise_args_t args2 = {
            .output = buffer_ypu_mem,
            .lhs = pv_cnn_depthwise_get_weight((pv_cnn_depthwise_t *) object),
            .rhs = x_q510_ypu_mem,
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
            &args2);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_op_elementwise_args_t args3 = {
            .output = y_ypu_mem,
            .input = buffer_ypu_mem,
            .length = num_output_frames * num_channels,
            .output_offset = y_offset,
            .input_offset = padding_numel * (int32_t) sizeof(int32_t),
    };

    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_CONVERT_F32_I32,
            &args3);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_op_pairwise_broadcast_args_t args4 = {
            .output = y_ypu_mem,
            .lhs = y_ypu_mem,
            .rhs = inverse_scale,
            .m = num_output_frames * num_channels,
            .n = 1,
            .output_offset = y_offset,
            .lhs_offset = y_offset,
            .rhs_offset = 0,
    };

    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_MULMV,
            &args4);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_op_pairwise_broadcast_args_t args5 = {
            .output = y_ypu_mem,
            .lhs = y_ypu_mem,
            .rhs = object->bias_float,
            .m = num_output_frames,
            .n = num_channels,
            .output_offset = y_offset,
            .lhs_offset = y_offset,
            .rhs_offset = 0,
    };

    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_ADDMV,
            &args5);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_buffer_release(ypu, inverse_scale);
    pv_ypu_buffer_release(ypu, x_q510_ypu_mem);
    pv_ypu_buffer_release(ypu, buffer_ypu_mem);

    return PV_STATUS_SUCCESS;
}

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

void PV_MOCKABLE(pv_cnn_transposed_depthwise_delete)(pv_ypu_t *ypu, pv_cnn_transposed_depthwise_t *object) {
    PV_ASSERT(ypu);

    return pv_cnn_depthwise_delete(ypu, (pv_cnn_depthwise_t *) object);
}

pv_status_t PV_MOCKABLE(pv_cnn_transposed_depthwise_forward)(
        pv_ypu_t *ypu,
        pv_cnn_transposed_depthwise_t *object,
        int32_t n,
        pv_ypu_mem_t *x_ypu_mem,
        pv_ypu_mem_t *y_ypu_mem,
        int32_t x_offset,
        int32_t y_offset) {
    PV_ASSERT(ypu);
    PV_ASSERT(object);
    PV_ASSERT(n);
    PV_ASSERT(x_ypu_mem);
    PV_ASSERT(y_ypu_mem);

    return pv_cnn_transposed_depthwise_forward_float_to_float(
            ypu,
            object,
            n,
            x_ypu_mem,
            y_ypu_mem,
            x_offset,
            y_offset);
}

int32_t PV_MOCKABLE(pv_cnn_transposed_depthwise_num_output_frames)(
        const pv_cnn_transposed_depthwise_t *object,
        int32_t n) {
    PV_ASSERT(object);
    PV_ASSERT(n > 0);

    return (((n - 1) * object->param->stride) + object->param->kernel_size) - (2 * object->param->padding);
}
