#include <stdlib.h>
#include <string.h>

#include "core/pv_error_messages.h"
#include "core/pv_type.h"
#include "orca/pv_cnn.h"
#include "orca/pv_cnn_transposed_depthwise.h"
#include "orca/pv_convnext_transposed.h"
#include "util/pv_check_status.h"
#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_convnext_transposed_param_serialize)(
        pv_ypu_t *ypu,
        const pv_convnext_transposed_param_t *param,
        FILE *file) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(file);

    pv_status_t status = pv_cnn_transposed_depthwise_param_serialize(
            ypu,
            param->conv_transposed_depthwise_param,
            file);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_transposed_depthwise_param_serialize,
                pv_status_to_string(status));
        return status;
    }

    status = pv_layer_norm_param_serialize(
            ypu,
            param->layer_norm_param,
            true,
            file);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_layer_norm_param_serialize,
                pv_status_to_string(status));
        return status;
    }

    status = pv_cnn_param_serialize(
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

pv_status_t PV_MOCKABLE(pv_convnext_transposed_param_load)(
        pv_ypu_t *ypu,
        FILE *f,
        pv_convnext_transposed_param_t **param) {
    PV_ASSERT(ypu);
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_convnext_transposed_param_t *p = pv_ypu_host_alloc(
            ypu,
            sizeof(pv_convnext_transposed_param_t));
    if (!p) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_host_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(p, 0, sizeof(pv_convnext_transposed_param_t));

    pv_status_t status = pv_cnn_transposed_depthwise_param_load(
            ypu,
            f,
            (pv_cnn_transposed_depthwise_param_t **) &(p->conv_transposed_depthwise_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_transposed_depthwise_param_load,
                pv_status_to_string(status));
        pv_convnext_transposed_param_delete(ypu, p);
        return status;
    }

    status = pv_layer_norm_param_load(
            ypu,
            f,
            true,
            (pv_layer_norm_param_t **) &(p->layer_norm_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_layer_norm_param_load,
                pv_status_to_string(status));
        pv_convnext_transposed_param_delete(ypu, p);
        return status;
    }

    status = pv_cnn_param_load(
            ypu,
            f,
            (pv_cnn_param_t **) &(p->conv_1_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_param_load,
                pv_status_to_string(status));
        pv_convnext_transposed_param_delete(ypu, p);
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
        pv_convnext_transposed_param_delete(ypu, p);
        return status;
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_convnext_transposed_param_delete)(
        pv_ypu_t *ypu,
        pv_convnext_transposed_param_t *param) {
    PV_ASSERT(ypu);

    if (param) {
        pv_cnn_param_delete(ypu, (pv_cnn_param_t *) (param->conv_2_param));

        pv_cnn_param_delete(ypu, (pv_cnn_param_t *) (param->conv_1_param));

        pv_layer_norm_param_delete(ypu, (pv_layer_norm_param_t *) (param->layer_norm_param));

        pv_cnn_transposed_depthwise_param_delete(
                ypu,
                (pv_cnn_transposed_depthwise_param_t *) (param->conv_transposed_depthwise_param));

        pv_ypu_host_free(ypu, param);
    }
}

bool PV_MOCKABLE(pv_convnext_transposed_param_is_equal)(
        const pv_convnext_transposed_param_t *object,
        const pv_convnext_transposed_param_t *other) {
    PV_ASSERT(object);
    PV_ASSERT(other);

    if (!pv_cnn_transposed_depthwise_param_is_equal(
                object->conv_transposed_depthwise_param, other->conv_transposed_depthwise_param)) {
        return false;
    }

    if (!pv_layer_norm_param_is_equal(object->layer_norm_param, other->layer_norm_param)) {
        return false;
    }

    if (!pv_cnn_param_is_equal(object->conv_1_param, other->conv_1_param)) {
        return false;
    }

    if (!pv_cnn_param_is_equal(object->conv_2_param, other->conv_2_param)) {
        return false;
    }

    return true;
}

struct pv_convnext_transposed {
    const pv_convnext_transposed_param_t *param;

    pv_cnn_transposed_depthwise_t *conv_transposed_depthwise;
    pv_layer_norm_t *layer_norm;
    pv_cnn_t *conv_1;
    pv_cnn_t *conv_2;
};

pv_status_t PV_MOCKABLE(pv_convnext_transposed_init)(
        pv_ypu_t *ypu,
        const pv_convnext_transposed_param_t *param,
        pv_convnext_transposed_t **object) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(object);

    *object = NULL;

    pv_convnext_transposed_t *o = pv_ypu_host_alloc(
            ypu,
            sizeof(pv_convnext_transposed_t));
    if (!o) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_host_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(o, 0, sizeof(pv_convnext_transposed_t));

    o->param = param;

    pv_status_t status = pv_cnn_transposed_depthwise_init(
            ypu,
            param->conv_transposed_depthwise_param,
            &(o->conv_transposed_depthwise));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_transposed_depthwise_init,
                pv_status_to_string(status));
        pv_convnext_transposed_delete(ypu, o);
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
        pv_convnext_transposed_delete(ypu, o);
        return status;
    }

    status = pv_cnn_init(
            ypu,
            param->conv_1_param,
            &(o->conv_1),
            false);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_init,
                pv_status_to_string(status));
        pv_convnext_transposed_delete(ypu, o);
        return status;
    }

    status = pv_cnn_init(
            ypu,
            param->conv_2_param,
            &(o->conv_2),
            false);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_init,
                pv_status_to_string(status));
        pv_convnext_transposed_delete(ypu, o);
        return status;
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_convnext_transposed_delete)(
        pv_ypu_t *ypu,
        pv_convnext_transposed_t *object) {
    PV_ASSERT(ypu);

    if (object) {
        pv_cnn_delete(ypu, object->conv_2);
        pv_cnn_delete(ypu, object->conv_1);

        pv_layer_norm_delete(ypu, object->layer_norm);

        pv_cnn_transposed_depthwise_delete(ypu, object->conv_transposed_depthwise);

        pv_ypu_host_free(ypu, object);
    }
}

pv_status_t PV_MOCKABLE(pv_convnext_transposed_reset_cache)(
        pv_ypu_t *ypu,
        pv_convnext_transposed_t *object) {
    PV_ASSERT(ypu);
    PV_ASSERT(object);

    return pv_cnn_transposed_depthwise_reset_cache(ypu, object->conv_transposed_depthwise);
}

int32_t PV_MOCKABLE(pv_convnext_transposed_output_channels)(const pv_convnext_transposed_t *object) {
    PV_ASSERT(object);

    return pv_cnn_output_channels(object->conv_2);
}

pv_status_t PV_MOCKABLE(pv_convnext_transposed_forward)(
        pv_ypu_t *ypu,
        pv_convnext_transposed_t *object,
        int32_t n,
        pv_ypu_mem_t *x_ypu,
        pv_ypu_mem_t *y_ypu) {
    PV_ASSERT(object);
    PV_ASSERT(n);
    PV_ASSERT(x_ypu);
    PV_ASSERT(y_ypu);

    const int32_t num_channels_depthwise = object->param->conv_transposed_depthwise_param->num_channels;
    const int32_t n_upsampled = pv_cnn_transposed_depthwise_num_output_frames(object->conv_transposed_depthwise, n);

    pv_ypu_mem_t *buffer_1_ypu = pv_ypu_buffer_get(
            ypu,
            num_channels_depthwise * n_upsampled * (int32_t) sizeof(float),
            false);
    if (!buffer_1_ypu) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("buffer_1_ypu"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_status_t status = pv_cnn_transposed_depthwise_forward(
            ypu,
            object->conv_transposed_depthwise,
            n,
            x_ypu,
            buffer_1_ypu,
            0,
            0);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_transposed_depthwise_forward,
                pv_status_to_string(status));
        pv_ypu_buffer_release(ypu, buffer_1_ypu);
        return status;
    }

    status = pv_layer_norm_forward(
            ypu,
            object->layer_norm,
            n_upsampled,
            buffer_1_ypu,
            buffer_1_ypu);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_layer_norm_forward,
                pv_status_to_string(status));
        pv_ypu_buffer_release(ypu, buffer_1_ypu);
        return status;
    }

    const int32_t num_intermediate_channels = object->param->conv_1_param->output_channels;
    pv_ypu_mem_t *buffer_2_ypu = pv_ypu_buffer_get(
            ypu,
            num_intermediate_channels * n_upsampled * (int32_t) sizeof(float),
            false);
    if (!buffer_2_ypu) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("buffer_2_ypu"));
        pv_ypu_buffer_release(ypu, buffer_1_ypu);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    status = pv_cnn_forward(
            ypu,
            object->conv_1,
            n_upsampled,
            buffer_1_ypu,
            buffer_2_ypu,
            0,
            0);
    pv_ypu_buffer_release(ypu, buffer_1_ypu);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_forward,
                pv_status_to_string(status));
        pv_ypu_buffer_release(ypu, buffer_2_ypu);
        return status;
    }

    pv_ypu_op_elementwise_args_t gelu_args = {
            .output = buffer_2_ypu,
            .input = buffer_2_ypu,
            .length = n_upsampled * pv_cnn_output_channels(object->conv_1),
            .output_offset = 0,
            .input_offset = 0,
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
        pv_ypu_buffer_release(ypu, buffer_2_ypu);
        return status;
    }

    status = pv_cnn_forward(
            ypu,
            object->conv_2,
            n_upsampled,
            buffer_2_ypu,
            y_ypu,
            0,
            0);
    pv_ypu_buffer_release(ypu, buffer_2_ypu);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_forward,
                pv_status_to_string(status));
        return status;
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_convnext_transposed_forward_with_cache)(
        pv_ypu_t *ypu,
        pv_convnext_transposed_t *object,
        int32_t n,
        pv_ypu_mem_t *x_ypu,
        pv_ypu_mem_t *y_ypu,
        bool is_flush,
        int32_t *n_out) {
    PV_ASSERT(object);
    PV_ASSERT(n);
    PV_ASSERT(x_ypu);
    PV_ASSERT(y_ypu);

    const int32_t num_channels_depthwise = object->param->conv_transposed_depthwise_param->num_channels;
    int32_t cache_length = pv_cnn_transposed_depthwise_cache_length(object->conv_transposed_depthwise);
    const int32_t n_upsampled_max = (n + cache_length) * 4;

    pv_ypu_mem_t *buffer_1_ypu = pv_ypu_buffer_get(
            ypu,
            num_channels_depthwise * n_upsampled_max * (int32_t) sizeof(float),
            false);
    if (!buffer_1_ypu) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("buffer_1_ypu"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    int32_t n_upsampled_out = 0;
    pv_status_t status = pv_cnn_transposed_depthwise_forward_with_cache(
            ypu,
            object->conv_transposed_depthwise,
            n,
            x_ypu,
            buffer_1_ypu,
            0,
            0,
            is_flush,
            &n_upsampled_out);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_transposed_depthwise_forward_with_cache,
                pv_status_to_string(status));
        pv_ypu_buffer_release(ypu, buffer_1_ypu);
        return status;
    }
    assert(n_upsampled_out <= n_upsampled_max);

    status = pv_layer_norm_forward(
            ypu,
            object->layer_norm,
            n_upsampled_out,
            buffer_1_ypu,
            buffer_1_ypu);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_layer_norm_forward,
                pv_status_to_string(status));
        pv_ypu_buffer_release(ypu, buffer_1_ypu);
        return status;
    }

    const int32_t num_intermediate_channels = object->param->conv_1_param->output_channels;
    pv_ypu_mem_t *buffer_2_ypu = pv_ypu_buffer_get(
            ypu,
            num_intermediate_channels * n_upsampled_out * (int32_t) sizeof(float),
            false);
    if (!buffer_2_ypu) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("buffer_2_ypu"));
        pv_ypu_buffer_release(ypu, buffer_1_ypu);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    status = pv_cnn_forward(
            ypu,
            object->conv_1,
            n_upsampled_out,
            buffer_1_ypu,
            buffer_2_ypu,
            0,
            0);
    pv_ypu_buffer_release(ypu, buffer_1_ypu);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_forward,
                pv_status_to_string(status));
        pv_ypu_buffer_release(ypu, buffer_2_ypu);
        return status;
    }

    pv_ypu_op_elementwise_args_t gelu_args = {
            .output = buffer_2_ypu,
            .input = buffer_2_ypu,
            .length = n_upsampled_out * pv_cnn_output_channels(object->conv_1),
            .output_offset = 0,
            .input_offset = 0,
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
        pv_ypu_buffer_release(ypu, buffer_2_ypu);
        return status;
    }

    status = pv_cnn_forward(
            ypu,
            object->conv_2,
            n_upsampled_out,
            buffer_2_ypu,
            y_ypu,
            0,
            0);
    pv_ypu_buffer_release(ypu, buffer_2_ypu);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_forward,
                pv_status_to_string(status));
        return status;
    }

    *n_out = n_upsampled_out;

    return PV_STATUS_SUCCESS;
}


int32_t PV_MOCKABLE(pv_convnext_transposed_num_output_frames)(
        const pv_convnext_transposed_t *object,
        int32_t n) {
    PV_ASSERT(object);
    PV_ASSERT(n > 0);

    return pv_cnn_transposed_depthwise_num_output_frames(object->conv_transposed_depthwise, n);
}
