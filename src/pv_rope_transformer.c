#include <stdlib.h>
#include <string.h>

#include "core/pv_error_messages.h"
#include "orca/pv_rope_attention.h"
#include "orca/pv_rope_ffn.h"
#include "orca/pv_rope_transformer.h"
#include "util/pv_check_status.h"
#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

struct pv_rope_transformer {
    const pv_rope_transformer_param_t *param;

    pv_layer_norm_t *layer_norm_1;
    pv_rope_attention_t *rope_attention;
    pv_layer_norm_t *layer_norm_2;
    pv_rope_transformer_ffn_t *ffn;
};

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_rope_transformer_param_serialize)(
        pv_ypu_t *ypu,
        const pv_rope_transformer_param_t *param,
        FILE *file) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(file);

    pv_status_t status = pv_layer_norm_param_serialize(
            ypu,
            param->layer_norm_1_param,
            true,
            file);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_layer_norm_param_serialize,
                pv_status_to_string(status));
        return status;
    }

    status = pv_rope_attention_param_serialize(
            ypu,
            param->attention_param,
            file);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_rope_attention_param_serialize,
                pv_status_to_string(status));
        return status;
    }

    status = pv_layer_norm_param_serialize(
            ypu,
            param->layer_norm_2_param,
            true,
            file);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_layer_norm_param_serialize,
                pv_status_to_string(status));
        return status;
    }

    status = pv_rope_transformer_ffn_param_serialize(
            ypu,
            param->ffn_param,
            file);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_rope_transformer_ffn_param_serialize,
                pv_status_to_string(status));
        return status;
    }

    return PV_STATUS_SUCCESS;
}

#endif

pv_status_t PV_MOCKABLE(pv_rope_transformer_param_load)(
        pv_ypu_t *ypu,
        FILE *f,
        pv_rope_transformer_param_t **param) {
    PV_ASSERT(ypu);
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_rope_transformer_param_t *p = pv_ypu_host_alloc(
            ypu,
            sizeof(pv_rope_transformer_param_t));
    if (!p) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_host_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(p, 0, sizeof(pv_rope_transformer_param_t));

    pv_status_t status = pv_layer_norm_param_load(
            ypu,
            f,
            true,
            (pv_layer_norm_param_t **) &(p->layer_norm_1_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_layer_norm_param_load,
                pv_status_to_string(status));
        pv_rope_transformer_param_delete(ypu, p);
        return status;
    }

    status = pv_rope_attention_param_load(
            ypu,
            f,
            (pv_rope_attention_param_t **) &(p->attention_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_rope_attention_param_load,
                pv_status_to_string(status));
        pv_rope_transformer_param_delete(ypu, p);
        return status;
    }

    status = pv_layer_norm_param_load(
            ypu,
            f,
            true,
            (pv_layer_norm_param_t **) &(p->layer_norm_2_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_layer_norm_param_load,
                pv_status_to_string(status));
        pv_rope_transformer_param_delete(ypu, p);
        return status;
    }

    status = pv_rope_transformer_ffn_param_load(
            ypu,
            f,
            (pv_rope_transformer_ffn_param_t **) &(p->ffn_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_rope_transformer_ffn_param_load,
                pv_status_to_string(status));
        pv_rope_transformer_param_delete(ypu, p);
        return status;
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_rope_transformer_param_delete)(
        pv_ypu_t *ypu,
        pv_rope_transformer_param_t *param) {
    PV_ASSERT(ypu);

    if (param) {
        pv_rope_transformer_ffn_param_delete(ypu, (pv_rope_transformer_ffn_param_t *) (param->ffn_param));

        pv_layer_norm_param_delete(ypu, (pv_layer_norm_param_t *) (param->layer_norm_2_param));

        pv_rope_attention_param_delete(ypu, (pv_rope_attention_param_t *) (param->attention_param));

        pv_layer_norm_param_delete(ypu, (pv_layer_norm_param_t *) (param->layer_norm_1_param));

        pv_ypu_host_free(ypu, param);
    }
}

bool PV_MOCKABLE(pv_rope_transformer_param_is_equal)(
        const pv_rope_transformer_param_t *object,
        const pv_rope_transformer_param_t *other) {
    PV_ASSERT(object);
    PV_ASSERT(other);

    if (!pv_layer_norm_param_is_equal(object->layer_norm_1_param, other->layer_norm_1_param)) {
        return false;
    }

    if (!pv_rope_attention_param_is_equal(object->attention_param, other->attention_param)) {
        return false;
    }

    if (!pv_layer_norm_param_is_equal(object->layer_norm_2_param, other->layer_norm_2_param)) {
        return false;
    }

    if (!pv_rope_transformer_ffn_param_is_equal(object->ffn_param, other->ffn_param)) {
        return false;
    }

    return true;
}

pv_status_t PV_MOCKABLE(pv_rope_transformer_init)(
        pv_ypu_t *ypu,
        const pv_rope_transformer_param_t *param,
        pv_rope_transformer_t **object) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(object);

    *object = NULL;

    pv_rope_transformer_t *o = pv_ypu_host_alloc(
            ypu,
            sizeof(pv_rope_transformer_t));
    if (!o) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_host_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(o, 0, sizeof(pv_rope_transformer_t));

    o->param = param;

    pv_status_t status = pv_layer_norm_init(
            ypu,
            param->layer_norm_1_param,
            &(o->layer_norm_1));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_layer_norm_init,
                pv_status_to_string(status));
        pv_rope_transformer_delete(ypu, o);
        return status;
    }

    status = pv_rope_attention_init(
            ypu,
            param->attention_param,
            &(o->rope_attention));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_rope_attention_init,
                pv_status_to_string(status));
        pv_rope_transformer_delete(ypu, o);
        return status;
    }

    status = pv_layer_norm_init(
            ypu,
            param->layer_norm_2_param,
            &(o->layer_norm_2));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_layer_norm_init,
                pv_status_to_string(status));
        pv_rope_transformer_delete(ypu, o);
        return status;
    }

    status = pv_rope_transformer_ffn_init(
            ypu,
            param->ffn_param,
            &(o->ffn));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_rope_transformer_ffn_init,
                pv_status_to_string(status));
        pv_rope_transformer_delete(ypu, o);
        return status;
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_rope_transformer_delete)(
        pv_ypu_t *ypu,
        pv_rope_transformer_t *object) {
    PV_ASSERT(ypu);

    if (object) {
        pv_rope_transformer_ffn_delete(ypu, object->ffn);
        pv_layer_norm_delete(ypu, object->layer_norm_2);
        pv_rope_attention_delete(ypu, object->rope_attention);
        pv_layer_norm_delete(ypu, object->layer_norm_1);

        pv_ypu_host_free(ypu, object);
    }
}

pv_status_t PV_MOCKABLE(pv_rope_transformer_forward)(
        pv_ypu_t *ypu,
        pv_rope_transformer_t *object,
        int32_t n,
        pv_ypu_mem_t *x_ypu,
        pv_ypu_mem_t *bucket_ypu,
        pv_ypu_mem_t *y_ypu) {
    PV_ASSERT(ypu);
    PV_ASSERT(object);
    PV_ASSERT(n > 0);
    PV_ASSERT(x_ypu);
    PV_ASSERT(y_ypu);

    const int32_t num_channels = object->param->layer_norm_1_param->num_channels;

    pv_ypu_mem_t *buffer_ypu = pv_ypu_buffer_get(
            ypu,
            n * num_channels * (int32_t) sizeof(float),
            false);
    if (!buffer_ypu) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("buffer"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_status_t status = pv_layer_norm_forward(
            ypu,
            object->layer_norm_1,
            n,
            x_ypu,
            buffer_ypu);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_layer_norm_forward,
                pv_status_to_string(status));
        pv_ypu_buffer_release(ypu, buffer_ypu);
        return status;
    }

    status = pv_rope_attention_forward(
            ypu,
            object->rope_attention,
            n,
            buffer_ypu,
            bucket_ypu,
            buffer_ypu);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_rope_attention_forward,
                pv_status_to_string(status));
        pv_ypu_buffer_release(ypu, buffer_ypu);
        return status;
    }

    pv_ypu_op_pairwise_args_t args_add0 = {
            .output = buffer_ypu,
            .lhs = x_ypu,
            .rhs = buffer_ypu,
            .length = num_channels * n,
            .output_offset = 0,
            .lhs_offset = 0,
            .rhs_offset = 0,
    };
    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_ADD,
            &args_add0);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_ADD),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        pv_ypu_buffer_release(ypu, buffer_ypu);
        return status;
    }

    status = pv_layer_norm_forward(
            ypu,
            object->layer_norm_2,
            n,
            buffer_ypu,
            y_ypu);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_layer_norm_forward,
                pv_status_to_string(status));
        pv_ypu_buffer_release(ypu, buffer_ypu);
        return status;
    }

    status = pv_rope_transformer_ffn_forward(
            ypu,
            object->ffn,
            n,
            y_ypu,
            y_ypu);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_rope_transformer_ffn_forward,
                pv_status_to_string(status));
        pv_ypu_buffer_release(ypu, buffer_ypu);
        return status;
    }

    pv_ypu_op_pairwise_args_t args_add1 = {
            .output = y_ypu,
            .lhs = buffer_ypu,
            .rhs = y_ypu,
            .length = num_channels * n,
            .output_offset = 0,
            .lhs_offset = 0,
            .rhs_offset = 0,
    };
    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_ADD,
            &args_add1);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_ADD),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        pv_ypu_buffer_release(ypu, buffer_ypu);
        return status;
    }

    pv_ypu_buffer_release(ypu, buffer_ypu);

    return PV_STATUS_SUCCESS;
}
