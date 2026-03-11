#include <string.h>

#include "core/pv_error_messages.h"
#include "orca/pv_additive_coupling.h"
#include "orca/pv_cnn.h"
#include "orca/pv_profiler.h"
#include "orca/pv_rope_transformer_film_conditioned.h"
#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

#define ADDITIVE_COUPLING_NUM_SPLITS (2)

struct pv_additive_coupling {
    const pv_additive_coupling_param_t *param;

    pv_cnn_t *conv_pre;
    pv_rope_transformer_film_conditioned_t *transformer;
    pv_cnn_t *conv_post;
};

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_additive_coupling_param_serialize)(
        pv_ypu_t *ypu,
        const pv_additive_coupling_param_t *param,
        FILE *file) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(file);

    pv_status_t status = pv_cnn_param_serialize(
            ypu,
            param->conv_pre_param,
            file);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_param_serialize,
                pv_status_to_string(status));
        return status;
    }

    status = pv_rope_transformer_film_conditioned_param_serialize(
            ypu,
            param->transformer_param,
            file);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_rope_transformer_film_conditioned_param_serialize,
                pv_status_to_string(status));
        return status;
    }

    status = pv_cnn_param_serialize(
            ypu,
            param->conv_post_param,
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

pv_status_t PV_MOCKABLE(pv_additive_coupling_param_load)(
        pv_ypu_t *ypu,
        FILE *f,
        pv_additive_coupling_param_t **param) {
    PV_ASSERT(ypu);
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_additive_coupling_param_t *p = pv_ypu_host_alloc(
            ypu,
            sizeof(pv_additive_coupling_param_t));
    if (!p) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_host_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(p, 0, sizeof(pv_additive_coupling_param_t));

    pv_status_t status = pv_cnn_param_load(
            ypu,
            f,
            (pv_cnn_param_t **) &(p->conv_pre_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_param_load,
                pv_status_to_string(status));
        pv_additive_coupling_param_delete(ypu, p);
        return status;
    }

    status = pv_rope_transformer_film_conditioned_param_load(
            ypu,
            f,
            (pv_rope_transformer_film_conditioned_param_t **) &(p->transformer_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_rope_transformer_film_conditioned_param_load,
                pv_status_to_string(status));
        pv_additive_coupling_param_delete(ypu, p);
        return status;
    }

    status = pv_cnn_param_load(
            ypu,
            f,
            (pv_cnn_param_t **) &(p->conv_post_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_param_load,
                pv_status_to_string(status));
        pv_additive_coupling_param_delete(ypu, p);
        return status;
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_additive_coupling_param_delete)(
        pv_ypu_t *ypu,
        pv_additive_coupling_param_t *param) {
    PV_ASSERT(ypu);

    if (param) {

        pv_cnn_param_delete(ypu, (pv_cnn_param_t *) (param->conv_post_param));

        pv_rope_transformer_film_conditioned_param_delete(ypu, (pv_rope_transformer_film_conditioned_param_t *) (param->transformer_param));

        pv_cnn_param_delete(ypu, (pv_cnn_param_t *) (param->conv_pre_param));

        pv_ypu_host_free(ypu, param);
    }
}

bool PV_MOCKABLE(pv_additive_coupling_param_is_equal)(
        const pv_additive_coupling_param_t *object,
        const pv_additive_coupling_param_t *other) {
    PV_ASSERT(object);
    PV_ASSERT(other);

    if (!pv_cnn_param_is_equal(object->conv_pre_param, other->conv_pre_param)) {
        return false;
    }

    if (!pv_rope_transformer_film_conditioned_param_is_equal(object->transformer_param, other->transformer_param)) {
        return false;
    }

    if (!pv_cnn_param_is_equal(object->conv_post_param, other->conv_post_param)) {
        return false;
    }

    return true;
}

pv_status_t PV_MOCKABLE(pv_additive_coupling_init)(
        pv_ypu_t *ypu,
        const pv_additive_coupling_param_t *param,
        pv_additive_coupling_t **object) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(object);

    *object = NULL;

    pv_additive_coupling_t *o = pv_ypu_host_alloc(
            ypu,
            sizeof(pv_additive_coupling_t));
    if (!o) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_host_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(o, 0, sizeof(pv_additive_coupling_t));

    o->param = param;
    
    pv_status_t status = pv_cnn_init(
            ypu,
            param->conv_pre_param,
            &(o->conv_pre));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_init,
                pv_status_to_string(status));
        pv_additive_coupling_delete(ypu, o);
        return status;
    }

    status = pv_rope_transformer_film_conditioned_init(
            ypu,
            param->transformer_param,
            &(o->transformer));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_rope_transformer_film_conditioned_init,
                pv_status_to_string(status));
        pv_additive_coupling_delete(ypu, o);
        return status;
    }

    status = pv_cnn_init(
            ypu,
            param->conv_post_param,
            &(o->conv_post));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_init,
                pv_status_to_string(status));
        pv_additive_coupling_delete(ypu, o);
        return status;
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_additive_coupling_delete)(
        pv_ypu_t *ypu,
        pv_additive_coupling_t *object) {
    PV_ASSERT(ypu);

    if (object) {

        pv_cnn_delete(ypu, object->conv_post);

        pv_rope_transformer_film_conditioned_delete(ypu, object->transformer);

        pv_cnn_delete(ypu, object->conv_pre);

        pv_ypu_host_free(ypu, object);
    }
}

pv_status_t PV_MOCKABLE(pv_additive_coupling_forward)(
        pv_ypu_t *ypu,
        pv_additive_coupling_t *object,
        int32_t n,
        pv_ypu_mem_t *c_ypu,
        pv_ypu_mem_t *x_ypu,
        pv_ypu_mem_t *y_ypu) {
    PV_ASSERT(ypu);
    PV_ASSERT(object);
    PV_ASSERT(n > 0);
    PV_ASSERT(c_ypu);
    PV_ASSERT(x_ypu);
    PV_ASSERT(y_ypu);

    const int32_t half_channels = object->param->conv_pre_param->input_channels;
    const int32_t num_channels = object->param->conv_pre_param->output_channels;
    PV_ASSERT(half_channels * ADDITIVE_COUPLING_NUM_SPLITS == num_channels);

    pv_ypu_mem_t *buffer_x_ypu = pv_ypu_buffer_get(
            ypu,
            n * ADDITIVE_COUPLING_NUM_SPLITS * half_channels * ((int32_t) sizeof(float)),
            false);
    if (!buffer_x_ypu) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("buffer_x"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_op_transpose_args_t transpose_args0 = {
            .output = buffer_x_ypu,
            .input = x_ypu,
            .b = 1,
            .m = ADDITIVE_COUPLING_NUM_SPLITS,
            .n = n,
            .k = half_channels * (int32_t) sizeof(float),
            .output_offset = 0,
            .input_offset = 0
    };
    pv_status_t status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_TRANSPOSE,
            &transpose_args0);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_TRANSPOSE),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        pv_ypu_buffer_release(ypu, buffer_x_ypu);
        return status;
    }

    pv_ypu_mem_t *buffer_transformer_input_ypu = pv_ypu_buffer_get(
            ypu,
            n * num_channels * ((int32_t) sizeof(float)),
            false);
    if (!buffer_transformer_input_ypu) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("buffer_transformer_input_ypu"));
        pv_ypu_buffer_release(ypu, buffer_x_ypu);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    status = pv_cnn_forward(
            ypu,
            object->conv_pre,
            n,
            buffer_x_ypu,
            buffer_transformer_input_ypu,
            0,
            0);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_forward,
                pv_status_to_string(status));
        pv_ypu_buffer_release(ypu, buffer_transformer_input_ypu);
        pv_ypu_buffer_release(ypu, buffer_x_ypu);
        return status;
    }

    pv_ypu_mem_t *buffer_transformer_output_ypu = pv_ypu_buffer_get(
            ypu,
            n * num_channels * ((int32_t) sizeof(float)),
            false);
    if (!buffer_transformer_output_ypu) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("buffer_transformer_output_ypu"));
        pv_ypu_buffer_release(ypu, buffer_transformer_input_ypu);
        pv_ypu_buffer_release(ypu, buffer_x_ypu);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    status = pv_rope_transformer_film_conditioned_forward(
            ypu,
            object->transformer,
            n,
            buffer_transformer_input_ypu,
            c_ypu,
            buffer_transformer_output_ypu);
    pv_ypu_buffer_release(ypu, buffer_transformer_input_ypu);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_rope_transformer_film_conditioned_forward,
                pv_status_to_string(status));
        pv_ypu_buffer_release(ypu, buffer_transformer_output_ypu);
        pv_ypu_buffer_release(ypu, buffer_x_ypu);
        return status;
    }

    pv_ypu_mem_t *buffer_mean_ypu = pv_ypu_buffer_get(
            ypu,
            n * half_channels * ((int32_t) sizeof(float)),
            false);
    if (!buffer_mean_ypu) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("buffer_mean_ypu"));
        pv_ypu_buffer_release(ypu, buffer_transformer_output_ypu);
        pv_ypu_buffer_release(ypu, buffer_x_ypu);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    status = pv_cnn_forward(
            ypu,
            object->conv_post,
            n,
            buffer_transformer_output_ypu,
            buffer_mean_ypu,
            0,
            0);
    pv_ypu_buffer_release(ypu, buffer_transformer_output_ypu);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_forward,
                pv_status_to_string(status));
        pv_ypu_buffer_release(ypu, buffer_mean_ypu);
        pv_ypu_buffer_release(ypu, buffer_x_ypu);
        return status;
    }

    pv_ypu_op_pairwise_args_t args_add = {
            .output = buffer_x_ypu,
            .lhs = buffer_mean_ypu,
            .rhs = buffer_x_ypu,
            .length = n * half_channels,
            .output_offset = n * half_channels * ((int32_t) sizeof(float)),
            .lhs_offset = 0,
            .rhs_offset = n * half_channels * ((int32_t) sizeof(float)),
    };
    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_ADD,
            &args_add);
    pv_ypu_buffer_release(ypu, buffer_mean_ypu);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_ADD),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        pv_ypu_buffer_release(ypu, buffer_x_ypu);
        return status;
    }

    pv_ypu_op_transpose_args_t transpose_args1 = {
            .output = y_ypu,
            .input = buffer_x_ypu,
            .b = 1,
            .m = n,
            .n = ADDITIVE_COUPLING_NUM_SPLITS,
            .k = half_channels * (int32_t) sizeof(float),
            .output_offset = 0,
            .input_offset = 0
    };
    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_TRANSPOSE,
            &transpose_args1);
    pv_ypu_buffer_release(ypu, buffer_x_ypu);
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

    return PV_STATUS_SUCCESS;
}
