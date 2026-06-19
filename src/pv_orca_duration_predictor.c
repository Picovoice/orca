#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "core/pv_error_messages.h"
#include "math/pv_math.h"
#include "orca/pv_cnn.h"
#include "orca/pv_orca_duration_predictor.h"
#include "orca/pv_orca_util.h"
#include "orca/pv_rope_transformer.h"
#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

struct pv_orca_duration_predictor {
    const pv_orca_duration_predictor_param_t *param;

    pv_rope_transformer_t *transformer;
    pv_cnn_t *conv_proj;
};

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_orca_duration_predictor_param_serialize)(
        pv_ypu_t *ypu,
        const pv_orca_duration_predictor_param_t *param,
        FILE *file) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(file);

    pv_status_t status = pv_rope_transformer_param_serialize(
            ypu,
            param->transformer_param,
            file);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_rope_transformer_param_serialize,
                pv_status_to_string(status));
        return status;
    }

    status = pv_cnn_param_serialize(
            ypu,
            param->conv_proj_param,
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

pv_status_t PV_MOCKABLE(pv_orca_duration_predictor_param_load)(
        pv_ypu_t *ypu,
        FILE *f,
        pv_orca_duration_predictor_param_t **param) {
    PV_ASSERT(ypu);
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_orca_duration_predictor_param_t *p = pv_ypu_host_alloc(
            ypu,
            sizeof(pv_orca_duration_predictor_param_t));
    if (!p) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_host_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(p, 0, sizeof(pv_orca_duration_predictor_param_t));

    pv_status_t status = pv_rope_transformer_param_load(
            ypu,
            f,
            (pv_rope_transformer_param_t **) &(p->transformer_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_rope_transformer_param_load,
                pv_status_to_string(status));
        pv_orca_duration_predictor_param_delete(ypu, p);
        return status;
    }

    status = pv_cnn_param_load(
            ypu,
            f,
            (pv_cnn_param_t **) &(p->conv_proj_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_param_load,
                pv_status_to_string(status));
        pv_orca_duration_predictor_param_delete(ypu, p);
        return status;
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}


void PV_MOCKABLE(pv_orca_duration_predictor_param_delete)(
        pv_ypu_t *ypu,
        pv_orca_duration_predictor_param_t *param) {
    PV_ASSERT(ypu);

    if (param) {
        pv_cnn_param_delete(ypu, (pv_cnn_param_t *) (param->conv_proj_param));

        pv_rope_transformer_param_delete(ypu, (pv_rope_transformer_param_t *) (param->transformer_param));

        pv_ypu_host_free(ypu, param);
    }
}

bool PV_MOCKABLE(pv_orca_duration_predictor_param_is_equal)(
        const pv_orca_duration_predictor_param_t *object,
        const pv_orca_duration_predictor_param_t *other) {
    PV_ASSERT(object);
    PV_ASSERT(other);

    if (!pv_rope_transformer_param_is_equal(
                object->transformer_param,
                other->transformer_param)) {
            return false;
    }

    if (!pv_cnn_param_is_equal(
                object->conv_proj_param,
                other->conv_proj_param)) {
        return false;
    }

    return true;
}

pv_status_t PV_MOCKABLE(pv_orca_duration_predictor_init)(
        pv_ypu_t *ypu,
        const pv_orca_duration_predictor_param_t *param,
        pv_orca_duration_predictor_t **object) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(object);

    *object = NULL;

    pv_orca_duration_predictor_t *o = pv_ypu_host_alloc(
            ypu,
            sizeof(pv_orca_duration_predictor_t));
    if (!o) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_host_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(o, 0, sizeof(pv_orca_duration_predictor_t));

    o->param = param;

    pv_status_t status = pv_rope_transformer_init(
            ypu,
            param->transformer_param,
            &(o->transformer));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_rope_transformer_init,
                pv_status_to_string(status));
        pv_orca_duration_predictor_delete(ypu, o);
        return status;
    }

    status = pv_cnn_init(
            ypu,
            param->conv_proj_param,
            &(o->conv_proj));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_init,
                pv_status_to_string(status));
        pv_orca_duration_predictor_delete(ypu, o);
        return status;
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_orca_duration_predictor_delete)(
        pv_ypu_t *ypu,
        pv_orca_duration_predictor_t *object) {
    PV_ASSERT(ypu);

    if (object) {
        pv_rope_transformer_delete(ypu, object->transformer);
        pv_cnn_delete(ypu, object->conv_proj);

        pv_ypu_host_free(ypu, object);
    }
}

pv_status_t PV_MOCKABLE(pv_orca_duration_predictor_forward)(
        pv_ypu_t *ypu,
        pv_orca_duration_predictor_t *object,
        int32_t n,
        float speech_rate,
        pv_ypu_mem_t *x_ypu,
        int32_t *d,
        pv_ypu_mem_t *std) {
    PV_ASSERT(ypu);
    PV_ASSERT(object);
    PV_ASSERT(n > 0);
    PV_ASSERT(speech_rate > 0.0f);
    PV_ASSERT(x_ypu);
    PV_ASSERT(d);
    PV_ASSERT(std);

    const int32_t dimension = object->param->transformer_param->layer_norm_1_param->num_channels;

    pv_ypu_mem_t *buffer_hidden_ypu = pv_ypu_buffer_get(
            ypu,
            n * dimension * (int32_t) sizeof(float),
            false);
    if (!buffer_hidden_ypu) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("buffer_hidden"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_status_t status = pv_rope_transformer_forward(
            ypu,
            object->transformer,
            n,
            x_ypu,
            NULL,
            buffer_hidden_ypu);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_rope_transformer_forward,
                pv_status_to_string(status));
    	pv_ypu_buffer_release(ypu, buffer_hidden_ypu);
        return status;
    }

    pv_ypu_mem_t *buffer_d_log_std_ypu = pv_ypu_buffer_get(
            ypu,
            n * 2 * (int32_t) sizeof(float),
            false);
    if (!buffer_d_log_std_ypu) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("buffer_d_log_std"));
        pv_ypu_buffer_release(ypu, buffer_hidden_ypu);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_mem_t *buffer_d_log_std_transposed_ypu = pv_ypu_buffer_get(
            ypu,
            2 * n * (int32_t) sizeof(float),
            false);
    if (!buffer_d_log_std_transposed_ypu) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("buffer_d_log_std_transposed"));
        pv_ypu_buffer_release(ypu, buffer_d_log_std_ypu);
        pv_ypu_buffer_release(ypu, buffer_hidden_ypu);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    status = pv_cnn_forward(
            ypu,
            object->conv_proj,
            n,
            buffer_hidden_ypu,
            buffer_d_log_std_ypu,
            0,
            0);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_forward,
                pv_status_to_string(status));
        pv_ypu_buffer_release(ypu, buffer_d_log_std_transposed_ypu);
        pv_ypu_buffer_release(ypu, buffer_d_log_std_ypu);
    	pv_ypu_buffer_release(ypu, buffer_hidden_ypu);
        return status;
    }

    pv_ypu_op_transpose_args_t transpose_args = {
            .output = buffer_d_log_std_transposed_ypu,
            .input = buffer_d_log_std_ypu,
            .b = 1,
            .m = 2,
            .n = n,
            .k = (int32_t) sizeof(float),
            .output_offset = 0,
            .input_offset = 0
    };
    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_TRANSPOSE,
            &transpose_args);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_TRANSPOSE),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        pv_ypu_buffer_release(ypu, buffer_d_log_std_transposed_ypu);
        pv_ypu_buffer_release(ypu, buffer_d_log_std_ypu);
    	pv_ypu_buffer_release(ypu, buffer_hidden_ypu);
        return status;
    }

    pv_ypu_op_elementwise_args_t exp_args = {
            .output = std,
            .input = buffer_d_log_std_transposed_ypu,
            .length = n,
            .output_offset = 0,
            .input_offset = n * (int32_t) sizeof(float)
    };
    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_EXP,
            &exp_args);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_EXP),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        return status;
    }

    float *buffer_d_log_std_transposed = (float *) pv_ypu_mem_get_host_view(ypu, buffer_d_log_std_transposed_ypu, true);
    for (int32_t i = 0; i < n; i++) {
        d[i] = (int32_t) lrintf(fmaxf(0.0f, exp2f(buffer_d_log_std_transposed[i] + 3.0f) - 1.0f) / speech_rate);
    }
    pv_ypu_mem_release_host_view(ypu, buffer_d_log_std_transposed_ypu, false);

    pv_ypu_buffer_release(ypu, buffer_d_log_std_transposed_ypu);
    pv_ypu_buffer_release(ypu, buffer_d_log_std_ypu);
    pv_ypu_buffer_release(ypu, buffer_hidden_ypu);

    return PV_STATUS_SUCCESS;
}
