#include <stdlib.h>
#include <string.h>

#include "core/pv_error_messages.h"
#include "orca/pv_adanorm.h"
#include "orca/pv_affine.h"
#include "orca/pv_layer_norm.h"
#include "orca/pv_rope_attention.h"
#include "orca/pv_rope_ffn.h"
#include "orca/pv_rope_transformer_film_conditioned.h"
#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

struct pv_rope_transformer_film_conditioned {
    const pv_rope_transformer_film_conditioned_param_t *param;

    pv_adanorm_t *adanorm;
    pv_rope_attention_t *attention;
    pv_layer_norm_t *layer_norm;
    pv_rope_transformer_ffn_t *ffn;
};


#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_rope_transformer_film_conditioned_param_serialize)(
        pv_ypu_t *ypu,
        const pv_rope_transformer_film_conditioned_param_t *param,
        FILE *file) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(file);

    pv_status_t status = pv_adanorm_param_serialize(
            ypu,
            param->adanorm_param,
            file);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_adanorm_param_serialize,
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
            param->layer_norm_param,
            false,
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

pv_status_t PV_MOCKABLE(pv_rope_transformer_film_conditioned_param_load)(
        pv_ypu_t *ypu,
        FILE *f,
        pv_rope_transformer_film_conditioned_param_t **param) {
    PV_ASSERT(ypu);
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_rope_transformer_film_conditioned_param_t *p = pv_ypu_host_alloc(
            ypu,
            sizeof(pv_rope_transformer_film_conditioned_param_t));
    if (!p) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_host_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(p, 0, sizeof(pv_rope_transformer_film_conditioned_param_t));

    pv_status_t status = pv_adanorm_param_load(
            ypu,
            f,
            (pv_adanorm_param_t **) &(p->adanorm_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_adanorm_param_load,
                pv_status_to_string(status));
        pv_rope_transformer_film_conditioned_param_delete(ypu, p);
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
        pv_rope_transformer_film_conditioned_param_delete(ypu, p);
        return status;
    }

    status = pv_layer_norm_param_load(
            ypu,
            f,
            false,
            (pv_layer_norm_param_t **) &(p->layer_norm_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_layer_norm_param_load,
                pv_status_to_string(status));
        pv_rope_transformer_film_conditioned_param_delete(ypu, p);
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
        pv_rope_transformer_film_conditioned_param_delete(ypu, p);
        return status;
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_rope_transformer_film_conditioned_param_delete)(
        pv_ypu_t *ypu,
        pv_rope_transformer_film_conditioned_param_t *param) {
    PV_ASSERT(ypu);

    if (param) {
        pv_rope_transformer_ffn_param_delete(ypu, (pv_rope_transformer_ffn_param_t *) (param->ffn_param));

        pv_layer_norm_param_delete(ypu, (pv_layer_norm_param_t *) (param->layer_norm_param));

        pv_rope_attention_param_delete(ypu, (pv_rope_attention_param_t *) (param->attention_param));

        pv_adanorm_param_delete(ypu, (pv_adanorm_param_t *) (param->adanorm_param));

        pv_ypu_host_free(ypu, param);
    }
}

bool PV_MOCKABLE(pv_rope_transformer_film_conditioned_param_is_equal)(
        const pv_rope_transformer_film_conditioned_param_t *object,
        const pv_rope_transformer_film_conditioned_param_t *other) {
    PV_ASSERT(object);
    PV_ASSERT(other);

    if (!pv_adanorm_param_is_equal(object->adanorm_param, other->adanorm_param)) {
        return false;
    }

    if (!pv_rope_attention_param_is_equal(object->attention_param, other->attention_param)) {
        return false;
    }

    if (!pv_layer_norm_param_is_equal(object->layer_norm_param, other->layer_norm_param)) {
        return false;
    }

    if (!pv_rope_transformer_ffn_param_is_equal(object->ffn_param, other->ffn_param)) {
        return false;
    }

    return true;
}

pv_status_t PV_MOCKABLE(pv_rope_transformer_film_conditioned_init)(
        pv_ypu_t *ypu,
        const pv_rope_transformer_film_conditioned_param_t *param,
        pv_rope_transformer_film_conditioned_t **object) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(object);

    *object = NULL;

    pv_rope_transformer_film_conditioned_t *o = pv_ypu_host_alloc(
            ypu,
            sizeof(pv_rope_transformer_film_conditioned_t));
    if (!o) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_host_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(o, 0, sizeof(pv_rope_transformer_film_conditioned_t));

    o->param = param;

    pv_status_t status = pv_adanorm_init(
            ypu,
            param->adanorm_param,
            &(o->adanorm));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_adanorm_init,
                pv_status_to_string(status));
        pv_rope_transformer_film_conditioned_delete(ypu, o);
        return status;
    }

    status = pv_rope_attention_init(
            ypu,
            param->attention_param,
            &(o->attention));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_rope_attention_init,
                pv_status_to_string(status));
        pv_rope_transformer_film_conditioned_delete(ypu, o);
        return status;
    }

    status = pv_layer_norm_init(
            ypu,
            param->layer_norm_param,
            &(o->layer_norm));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_layer_norm_init,
                pv_status_to_string(status));
        pv_rope_transformer_film_conditioned_delete(ypu, o);
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
        pv_rope_transformer_film_conditioned_delete(ypu, o);
        return status;
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_rope_transformer_film_conditioned_delete)(
        pv_ypu_t *ypu,
        pv_rope_transformer_film_conditioned_t *object) {
    PV_ASSERT(ypu);

    if (object) {
        pv_rope_transformer_ffn_delete(ypu, object->ffn);
        pv_layer_norm_delete(ypu, object->layer_norm);
        pv_rope_attention_delete(ypu, object->attention);
        pv_adanorm_delete(ypu, object->adanorm);

        pv_ypu_host_free(ypu, object);
    }
}

pv_status_t PV_MOCKABLE(pv_rope_transformer_film_conditioned_forward)(
        pv_ypu_t *ypu,
        pv_rope_transformer_film_conditioned_t *object,
        int32_t n,
        pv_ypu_mem_t *x_ypu,
        pv_ypu_mem_t *c_ypu,
        pv_ypu_mem_t *y_ypu) {
    PV_ASSERT(ypu);
    PV_ASSERT(object);
    PV_ASSERT(n > 0);
    PV_ASSERT(x_ypu);
    PV_ASSERT(c_ypu);
    PV_ASSERT(y_ypu);

    const int32_t num_channels = object->param->adanorm_param->linear_param->input_channels;

    pv_ypu_mem_t *buffer_ypu = pv_ypu_buffer_get(
            ypu,
            n * num_channels * (int32_t) sizeof(float),
            false);
    if (!buffer_ypu) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("buffer_ypu"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_mem_t *buffer_gates_list_ypu = pv_ypu_buffer_get(
            ypu,
            n * 6 * num_channels * (int32_t) sizeof(float),
            false);
    if (!buffer_gates_list_ypu) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("buffer_gates_list_ypu"));
        pv_ypu_buffer_release(ypu, buffer_ypu);
        return PV_STATUS_OUT_OF_MEMORY;
       
    }

    pv_status_t status = pv_adanorm_rope_transformer_forward(
            ypu,
            object->adanorm,
            n,
            x_ypu,
            c_ypu,
            buffer_gates_list_ypu,
            buffer_ypu);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_adanorm_rope_transformer_forward,
                pv_status_to_string(status));
        pv_ypu_buffer_release(ypu, buffer_gates_list_ypu);
        pv_ypu_buffer_release(ypu, buffer_ypu);
        return status;
    }

    status = pv_rope_attention_forward(
            ypu,
            object->attention,
            n,
            buffer_ypu,
            NULL,
            buffer_ypu);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_rope_attention_forward,
                pv_status_to_string(status));
        pv_ypu_buffer_release(ypu, buffer_gates_list_ypu);
        pv_ypu_buffer_release(ypu, buffer_ypu);
        return status;
    }

    status = pv_affine_execute(
            ypu,
            1,
            n * num_channels,
            buffer_ypu,
            1.0f,
            0.0f,
            buffer_gates_list_ypu,
            x_ypu,
            buffer_ypu,
            0,
            2 * n * num_channels * (int32_t) sizeof(float),
            0,
            0);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_affine_execute,
                pv_status_to_string(status));
        pv_ypu_buffer_release(ypu, buffer_gates_list_ypu);
        pv_ypu_buffer_release(ypu, buffer_ypu);
        return status;
    }

    pv_ypu_op_layer_norm_non_affine_args_t layer_norm_args = {
            .output = y_ypu,
            .input = buffer_ypu,
            .m = n,
            .n = object->param->layer_norm_param->num_channels,
            .eps = object->param->layer_norm_param->eps,
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
        pv_ypu_buffer_release(ypu, buffer_gates_list_ypu);
        pv_ypu_buffer_release(ypu, buffer_ypu);
        return status;
    }

    status = pv_affine_execute(
            ypu,
            1,
            n * object->param->layer_norm_param->num_channels,
            y_ypu,
            1.0f,
            1.0f,
            buffer_gates_list_ypu,
            buffer_gates_list_ypu,
            y_ypu,
            0,
            3 * n * num_channels * (int32_t) sizeof(float),
            4 * n * num_channels * (int32_t) sizeof(float),
            0);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_affine_execute,
                pv_status_to_string(status));
        pv_ypu_buffer_release(ypu, buffer_gates_list_ypu);
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
        pv_ypu_buffer_release(ypu, buffer_gates_list_ypu);
        pv_ypu_buffer_release(ypu, buffer_ypu);
        return status;
    }

    status = pv_affine_execute(
            ypu,
            1,
            n * num_channels,
            y_ypu,
            1.0f,
            0.0f,
            buffer_gates_list_ypu,
            buffer_ypu,
            y_ypu,
            0,
            5 * n * num_channels * (int32_t) sizeof(float),
            0,
            0);
    pv_ypu_buffer_release(ypu, buffer_gates_list_ypu);
    pv_ypu_buffer_release(ypu, buffer_ypu);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_affine_execute,
                pv_status_to_string(status));
        return status;
    }
 
    return PV_STATUS_SUCCESS;
}
