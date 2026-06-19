#include <stdlib.h>
#include <string.h>

#include "core/pv_error_messages.h"
#include "orca/pv_adanorm.h"
#include "orca/pv_affine.h"
#include "orca/pv_cnn.h"
#include "orca/pv_layer_norm.h"
#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

#define ADANORM_LINEAR_NUM_SPLITS (6)

struct pv_adanorm {
    const pv_adanorm_param_t *param;

    pv_cnn_t *linear;
};

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_adanorm_param_serialize)(
        pv_ypu_t *ypu,
        const pv_adanorm_param_t *param,
        FILE *file) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(file);

    size_t count = fwrite(&(param->eps), sizeof(float), 1, file);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

    pv_status_t status = pv_cnn_param_serialize(
            ypu,
            param->linear_param,
            file);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_param_serialize,
                pv_status_to_string(status));
        return status;
    }

    return PV_STATUS_SUCCESS;
}

#endif

pv_status_t PV_MOCKABLE(pv_adanorm_param_load)(
        pv_ypu_t *ypu,
        FILE *f,
        pv_adanorm_param_t **param) {
    PV_ASSERT(ypu);
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_adanorm_param_t *p = pv_ypu_host_alloc(
            ypu,
            sizeof(pv_adanorm_param_t));
    if (!p) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_host_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(p, 0, sizeof(pv_adanorm_param_t));

    size_t count = pv_fread(&(p->eps), sizeof(float), 1, f);
    if (count != 1) {
        PV_ERROR_REPORT(
                &pv_error_msg_fread_param_failure,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("count", f));
        pv_adanorm_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->eps <= 0.0f) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p->eps"));
        pv_adanorm_param_delete(ypu, p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    pv_status_t status = pv_cnn_param_load(
            ypu,
            f,
            (pv_cnn_param_t **) &(p->linear_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_param_load,
                pv_status_to_string(status));
        pv_adanorm_param_delete(ypu, p);
        return status;
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_adanorm_param_delete)(
        pv_ypu_t *ypu,
        pv_adanorm_param_t *param) {
    PV_ASSERT(ypu);

    if (param) {
        pv_cnn_param_delete(
                ypu,
                (pv_cnn_param_t *) (param->linear_param));

        pv_ypu_host_free(ypu, param);
    }
}

bool PV_MOCKABLE(pv_adanorm_param_is_equal)(
        const pv_adanorm_param_t *object,
        const pv_adanorm_param_t *other) {
    PV_ASSERT(object);
    PV_ASSERT(other);

    if (object->eps != other->eps) {
        return false;
    }

    if (!pv_cnn_param_is_equal(object->linear_param, other->linear_param)) {
        return false;
    }

    return true;
}

pv_status_t PV_MOCKABLE(pv_adanorm_init)(
        pv_ypu_t *ypu,
        const pv_adanorm_param_t *param,
        pv_adanorm_t **object) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(object);

    *object = NULL;

    pv_adanorm_t *o = pv_ypu_host_alloc(
            ypu,
            sizeof(pv_adanorm_t));
    if (!o) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_host_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(o, 0, sizeof(pv_adanorm_t));

    o->param = param;

    pv_status_t status = pv_cnn_init(
            ypu,
            param->linear_param,
            &(o->linear));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_init,
                pv_status_to_string(status));
        pv_adanorm_delete(ypu, o);
        return status;
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_adanorm_delete)(
        pv_ypu_t *ypu,
        pv_adanorm_t *object) {
    PV_ASSERT(ypu);

    if (object) {
        pv_cnn_delete(ypu, object->linear);

        pv_ypu_host_free(ypu, object);
    }
}

pv_status_t PV_MOCKABLE(pv_adanorm_rope_transformer_forward)(
        pv_ypu_t *ypu,
        pv_adanorm_t *object,
        int32_t n,
        pv_ypu_mem_t *x_ypu,
        pv_ypu_mem_t *c_ypu,
        pv_ypu_mem_t *gates_list_ypu,
        pv_ypu_mem_t *y_ypu) {
    PV_ASSERT(ypu);
    PV_ASSERT(object);
    PV_ASSERT(n);
    PV_ASSERT(x_ypu);
    PV_ASSERT(c_ypu);
    PV_ASSERT(gates_list_ypu);
    PV_ASSERT(y_ypu);

    const int32_t input_channels = object->param->linear_param->input_channels;
    pv_ypu_mem_t *buffer_c_ypu = pv_ypu_buffer_get(
            ypu,
            n * input_channels * (int32_t) sizeof(float),
            false);
    if (!buffer_c_ypu) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("buffer_c_ypu"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_op_elementwise_args_t silu_args = {
            .output = buffer_c_ypu,
            .input = c_ypu,
            .length = n * input_channels,
            .output_offset = 0,
            .input_offset = 0
    };
    pv_status_t status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_SILU,
            &silu_args);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_SILU),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        pv_ypu_buffer_release(ypu, buffer_c_ypu);
        return status;
    }

    const int32_t output_channels = object->param->linear_param->output_channels;
    pv_ypu_mem_t *buffer_scale_shift_and_more_ypu = pv_ypu_buffer_get(
            ypu,
            n * output_channels * (int32_t) sizeof(float),
            false);
    if (!buffer_scale_shift_and_more_ypu) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("buffer_scale_shift_and_more_ypu"));
        pv_ypu_buffer_release(ypu, buffer_c_ypu);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    status = pv_cnn_forward(
            ypu,
            object->linear,
            n,
            buffer_c_ypu,
            buffer_scale_shift_and_more_ypu,
            0,
            0);
    pv_ypu_buffer_release(ypu, buffer_c_ypu);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_forward,
                pv_status_to_string(status));
        pv_ypu_buffer_release(ypu, buffer_scale_shift_and_more_ypu);
        return status;
    }

    const int32_t num_channels = output_channels / ADANORM_LINEAR_NUM_SPLITS;

    pv_ypu_op_transpose_args_t transpose_args = {
            .output = gates_list_ypu,
            .input = buffer_scale_shift_and_more_ypu,
            .b = 1,
            .m = ADANORM_LINEAR_NUM_SPLITS,
            .n = n,
            .k = num_channels * (int32_t) sizeof(float),
            .output_offset = 0,
            .input_offset = 0
    };
    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_TRANSPOSE,
            &transpose_args);
    pv_ypu_buffer_release(ypu, buffer_scale_shift_and_more_ypu);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_TRANSPOSE),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        return status;
    }

    pv_ypu_op_layer_norm_non_affine_args_t layer_norm_args = {
            .output = y_ypu,
            .input = x_ypu,
            .m = n,
            .n = num_channels,
            .eps = object->param->eps,
            .output_offset = 0,
            .input_offset = 0
    };
    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_LAYER_NORM_NON_AFFINE,
            &layer_norm_args);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_LAYER_NORM_NON_AFFINE),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        return status;
    }

    status = pv_affine_execute(
            ypu,
            1,
            n * num_channels,
            y_ypu,
            1.0f,
            1.0f,
            gates_list_ypu,
            gates_list_ypu,
            y_ypu,
            0,
            0,
            1 * n * num_channels * (int32_t) sizeof(float),
            0);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_affine_execute,
                pv_status_to_string(status));
        return status;
    }

    return PV_STATUS_SUCCESS;
}
