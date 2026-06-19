#include <stdlib.h>
#include <string.h>

#include "core/pv_error_messages.h"
#include "core/pv_type.h"
#include "orca/pv_rope_ffn.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

struct pv_rope_transformer_ffn {
    const pv_rope_transformer_ffn_param_t *param;

    pv_cnn_t *conv_1;
    pv_cnn_t *conv_2;
};

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_rope_transformer_ffn_param_serialize)(
        pv_ypu_t *ypu,
        const pv_rope_transformer_ffn_param_t *param,
        FILE *file) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(file);

    pv_status_t status = pv_cnn_param_serialize(
            ypu,
            param->conv_1_param,
            file);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_param_serialize,
                pv_status_to_string(status));
        return status;
    }

    status = pv_cnn_param_serialize(
            ypu,
            param->conv_2_param,
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

pv_status_t PV_MOCKABLE(pv_rope_transformer_ffn_param_load)(
        pv_ypu_t *ypu,
        FILE *f,
        pv_rope_transformer_ffn_param_t **param) {
    PV_ASSERT(ypu);
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_rope_transformer_ffn_param_t *p = pv_ypu_host_alloc(
            ypu,
            sizeof(pv_rope_transformer_ffn_param_t));
    if (!p) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_host_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(p, 0, sizeof(pv_rope_transformer_ffn_param_t));

    pv_status_t status = pv_cnn_param_load(
            ypu,
            f,
            (pv_cnn_param_t **) &(p->conv_1_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_param_load,
                pv_status_to_string(status));
        pv_rope_transformer_ffn_param_delete(ypu, p);
        return status;
    }

    status = pv_cnn_param_load(
            ypu,
            f,
            (pv_cnn_param_t **) &(p->conv_2_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_param_load,
                pv_status_to_string(status));
        pv_rope_transformer_ffn_param_delete(ypu, p);
        return status;
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_rope_transformer_ffn_param_delete)(
        pv_ypu_t *ypu,
        pv_rope_transformer_ffn_param_t *param) {
    PV_ASSERT(ypu);

    if (param) {
        pv_cnn_param_delete(ypu, (pv_cnn_param_t *) (param->conv_2_param));

        pv_cnn_param_delete(ypu, (pv_cnn_param_t *) (param->conv_1_param));

        pv_ypu_host_free(ypu, param);
    }
}

bool PV_MOCKABLE(pv_rope_transformer_ffn_param_is_equal)(
        const pv_rope_transformer_ffn_param_t *object,
        const pv_rope_transformer_ffn_param_t *other) {
    PV_ASSERT(object);
    PV_ASSERT(other);

    if (!pv_cnn_param_is_equal(object->conv_1_param, other->conv_1_param)) {
        return false;
    }

    if (!pv_cnn_param_is_equal(object->conv_2_param, other->conv_2_param)) {
        return false;
    }

    return true;
}

pv_status_t PV_MOCKABLE(pv_rope_transformer_ffn_init)(
        pv_ypu_t *ypu,
        const pv_rope_transformer_ffn_param_t *param,
        pv_rope_transformer_ffn_t **object) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(object);

    *object = NULL;

    pv_rope_transformer_ffn_t *o = pv_ypu_host_alloc(
            ypu,
            sizeof(pv_rope_transformer_ffn_t));
    if (!o) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_host_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(o, 0, sizeof(pv_rope_transformer_ffn_t));

    o->param = param;

    pv_status_t status = pv_cnn_init(
            ypu,
            param->conv_1_param,
            &(o->conv_1));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_init,
                pv_status_to_string(status));
        pv_rope_transformer_ffn_delete(ypu, o);
        return status;
    }

    status = pv_cnn_init(
            ypu,
            param->conv_2_param,
            &(o->conv_2));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_init,
                pv_status_to_string(status));
        pv_rope_transformer_ffn_delete(ypu, o);
        return status;
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_rope_transformer_ffn_delete)(
        pv_ypu_t *ypu,
        pv_rope_transformer_ffn_t *object) {
    PV_ASSERT(ypu);

    if (object) {
        pv_cnn_delete(ypu, object->conv_2);
        pv_cnn_delete(ypu, object->conv_1);

        pv_ypu_host_free(ypu, object);
    }
}

pv_status_t PV_MOCKABLE(pv_rope_transformer_ffn_forward)(
        pv_ypu_t *ypu,
        pv_rope_transformer_ffn_t *object,
        int32_t n,
        pv_ypu_mem_t *x_ypu,
        pv_ypu_mem_t *y_ypu) {
    PV_ASSERT(ypu);
    PV_ASSERT(object);
    PV_ASSERT(n);
    PV_ASSERT(x_ypu);
    PV_ASSERT(y_ypu);
    
    const int32_t intermediate_dimension = object->param->conv_1_param->output_channels;
    pv_ypu_mem_t *buffer_ypu = pv_ypu_buffer_get(
            ypu,
            n * intermediate_dimension * (int32_t) sizeof(float),
            false);
    if (!buffer_ypu) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("buffer_ypu"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_status_t status = pv_cnn_forward(
            ypu,
            object->conv_1,
            n,
            x_ypu,
            buffer_ypu,
            0,
            0);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_forward,
                pv_status_to_string(status));
        pv_ypu_buffer_release(ypu, buffer_ypu);
        return status;
    }

    pv_ypu_op_elementwise_args_t gelu_args = {
            .output = buffer_ypu,
            .input = buffer_ypu,
            .length = n * intermediate_dimension,
            .output_offset = 0,
            .input_offset = 0
    };
    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_GELU_APPROX,
            &gelu_args);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_GELU_APPROX),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        pv_ypu_buffer_release(ypu, buffer_ypu);
        return status;
    }

    status = pv_cnn_forward(
            ypu,
            object->conv_2,
            n,
            buffer_ypu,
            y_ypu,
            0,
            0);
    pv_ypu_buffer_release(ypu, buffer_ypu);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_forward,
                pv_status_to_string(status));
        return status;
    }

    return PV_STATUS_SUCCESS;
}
