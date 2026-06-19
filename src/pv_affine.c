#include <stdint.h>

#include "core/picovoice.h"
#include "core/pv_error_messages.h"
#include "orca/pv_affine.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

#if __ORCA_FLOAT_MODE__

pv_status_t PV_MOCKABLE(pv_affine_execute_float)(
        int32_t n,
        int32_t num_channels,
        const float *x,
        float scale,
        float shift,
        const float *weight,
        const float *bias,
        float *y) {
    PV_ASSERT(n);
    PV_ASSERT(num_channels);
    PV_ASSERT(x);
    PV_ASSERT(scale >= 0.0f);
    PV_ASSERT(weight);
    PV_ASSERT(bias);
    PV_ASSERT(y);

    for (int32_t i = 0; i < n; i++) {
        const int32_t i_offset = i * num_channels;
        for (int32_t j = 0; j < num_channels; j++) {
            y[i_offset + j] = (x[i_offset + j] * scale * weight[j]) + (x[i_offset + j] * shift) + bias[j];
        }
    }

    return PV_STATUS_SUCCESS;
}

#endif

pv_status_t PV_MOCKABLE(pv_affine_execute)(
        pv_ypu_t *ypu,
        int32_t n,
        int32_t num_channels,
        pv_ypu_mem_t *x_ypu,
        float scale,
        float shift,
        pv_ypu_mem_t *weight_ypu,
        pv_ypu_mem_t *bias_ypu,
        pv_ypu_mem_t *y_ypu,
        int32_t x_offset,
        int32_t weight_offset,
        int32_t bias_offset,
        int32_t y_offset) {
    PV_ASSERT(n);
    PV_ASSERT(num_channels);
    PV_ASSERT(x_ypu);
    PV_ASSERT(scale >= 0.0f);
    PV_ASSERT(weight_ypu);
    PV_ASSERT(bias_ypu);
    PV_ASSERT(y_ypu);

    pv_ypu_mem_t *buffer_0_ypu = pv_ypu_buffer_get(
            ypu,
            n * num_channels * (int32_t) sizeof(float),
            false);
    if (!buffer_0_ypu) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("buffer_0_ypu"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_op_elementwise_scalar_args_t args_mulsv0 = {
            .output = buffer_0_ypu,
            .input = x_ypu,
            .scalar.f32 = shift,
            .length = n * num_channels,
            .output_offset = 0,
            .input_offset = x_offset
    };
    pv_status_t status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_MULSV,
            &args_mulsv0);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_MULSV),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        pv_ypu_buffer_release(ypu, buffer_0_ypu);
        return status;
    }

    pv_ypu_mem_t *buffer_1_ypu = pv_ypu_buffer_get(
            ypu,
            n * num_channels * (int32_t) sizeof(float),
            false);
    if (!buffer_1_ypu) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("buffer_1_ypu"));
        pv_ypu_buffer_release(ypu, buffer_0_ypu);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_op_elementwise_scalar_args_t args_mulsv1 = {
            .output = buffer_1_ypu,
            .input = x_ypu,
            .scalar.f32 = scale,
            .length = n * num_channels,
            .output_offset = 0,
            .input_offset = x_offset
    };
    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_MULSV,
            &args_mulsv1);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_MULSV),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        pv_ypu_buffer_release(ypu, buffer_1_ypu);
        pv_ypu_buffer_release(ypu, buffer_0_ypu);
        return status;
    }

    pv_ypu_op_pairwise_broadcast_args_t args_mulmv_iadd = {
            .output = buffer_0_ypu,
            .lhs = buffer_1_ypu,
            .rhs = weight_ypu,
            .m = n,
            .n = num_channels,
            .output_offset = 0,
            .lhs_offset = 0,
            .rhs_offset = weight_offset
    };
    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_MULMV_IADD,
            &args_mulmv_iadd);
    pv_ypu_buffer_release(ypu, buffer_1_ypu);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_MULMV_IADD),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        pv_ypu_buffer_release(ypu, buffer_0_ypu);
        return status;
    }

    pv_ypu_op_pairwise_broadcast_args_t args_addmv = {
            .output = y_ypu,
            .lhs = buffer_0_ypu,
            .rhs = bias_ypu,
            .m = n,
            .n = num_channels,
            .output_offset = y_offset,
            .lhs_offset = 0,
            .rhs_offset = bias_offset,
    };
    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_ADDMV,
            &args_addmv);
    pv_ypu_buffer_release(ypu, buffer_0_ypu);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_ADDMV),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        return status;
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_affine_execute_from_q1417_to_float)(
        pv_ypu_t *ypu,
        int32_t n,
        int32_t num_channels,
        pv_ypu_mem_t *x,
        pv_ypu_mem_t *scale,
        pv_ypu_mem_t *weight,
        pv_ypu_mem_t *bias,
        pv_ypu_mem_t *y,
        int32_t x_offset,
        int32_t scale_offset,
        int32_t weight_offset,
        int32_t bias_offset,
        int32_t y_offset) {
    PV_ASSERT(n);
    PV_ASSERT(num_channels);
    PV_ASSERT(x);
    PV_ASSERT(scale);
    PV_ASSERT(weight);
    PV_ASSERT(bias);
    PV_ASSERT(y);

     pv_ypu_op_pairwise_broadcast_args_t args_mulmv0 = {
            .output = y,
            .lhs = x,
            .rhs = scale,
            .m = n * num_channels,
            .n = 1,
            .output_offset = y_offset,
            .lhs_offset = x_offset,
            .rhs_offset = scale_offset,
    };
    pv_status_t status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_MULMV,
            &args_mulmv0);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_MULMV),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        return status;
    }

    pv_ypu_op_pairwise_broadcast_args_t args_mulmv1 = {
            .output = y,
            .lhs = y,
            .rhs = weight,
            .m = n,
            .n = num_channels,
            .output_offset = y_offset,
            .lhs_offset = y_offset,
            .rhs_offset = weight_offset,
    };
    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_MULMV,
            &args_mulmv1);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_MULMV),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        return status;
    }

    pv_ypu_op_pairwise_broadcast_args_t args_addmv = {
            .output = y,
            .lhs = y,
            .rhs = bias,
            .m = n,
            .n = num_channels,
            .output_offset = y_offset,
            .lhs_offset = y_offset,
            .rhs_offset = bias_offset,
    };
    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_ADDMV,
            &args_addmv);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_ADDMV),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        return status;
    }

    return PV_STATUS_SUCCESS;
}