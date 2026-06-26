#include <stdlib.h>
#include <string.h>

#include "core/pv_error_messages.h"
#include "core/pv_type.h"
#include "orca/pv_cnn.h"
#include "orca/pv_convnext.h"
#include "util/pv_check_status.h"
#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_convnext_param_serialize)(
        pv_ypu_t *ypu,
        const pv_convnext_param_t *param,
        FILE *file) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(file);

    pv_status_t status = pv_cnn_depthwise_param_serialize(
            ypu,
            param->conv_depthwise_param,
            file);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_depthwise_param_serialize,
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

    const size_t length = sizeof(float) * param->conv_2_param->output_channels;
    const size_t count = fwrite(param->scale_param->data, 1, length, file);
    if (count != length) {
        return PV_STATUS_IO_ERROR;
    }

    return PV_STATUS_SUCCESS;
}

#endif

pv_status_t PV_MOCKABLE(pv_convnext_param_load)(
        pv_ypu_t *ypu,
        FILE *f,
        pv_convnext_param_t **param) {
    PV_ASSERT(ypu);
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_convnext_param_t *p = pv_ypu_host_alloc(
            ypu,
            sizeof(pv_convnext_param_t));
    if (!p) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_host_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(p, 0, sizeof(pv_convnext_param_t));

    pv_status_t status = pv_cnn_depthwise_param_load(
            ypu,
            f,
            (pv_cnn_depthwise_param_t **) &(p->conv_depthwise_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_depthwise_param_load,
                pv_status_to_string(status));
        pv_convnext_param_delete(ypu, p);
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
        pv_convnext_param_delete(ypu, p);
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
        pv_convnext_param_delete(ypu, p);
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
        pv_convnext_param_delete(ypu, p);
        return status;
    }

    const size_t length = p->conv_2_param->output_channels;
    p->scale_param = pv_ypu_config_mem_alloc(
            ypu,
            (int32_t) (sizeof(float) * length),
            PV_YPU_DEVICE_MEM_FLAG_STATIC);
    if (!p->scale_param) {
        pv_convnext_param_delete(ypu, p);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    const size_t count = pv_fread((q7_t *) (p->scale_param->data), sizeof(float), length, f);
    if (count != length) {
        pv_convnext_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}


void PV_MOCKABLE(pv_convnext_param_delete)(
        pv_ypu_t *ypu,
        pv_convnext_param_t *param) {
    PV_ASSERT(ypu);

    if (param) {
        pv_ypu_config_mem_free(ypu, param->scale_param);

        pv_cnn_param_delete(ypu, (pv_cnn_param_t *) (param->conv_2_param));

        pv_cnn_param_delete(ypu, (pv_cnn_param_t *) (param->conv_1_param));

        pv_layer_norm_param_delete(ypu, (pv_layer_norm_param_t *) (param->layer_norm_param));

        pv_cnn_depthwise_param_delete(ypu, (pv_cnn_depthwise_param_t *) (param->conv_depthwise_param));

        pv_ypu_host_free(ypu, param);
    }
}

bool PV_MOCKABLE(pv_convnext_param_is_equal)(const pv_convnext_param_t *object, const pv_convnext_param_t *other) {
    PV_ASSERT(object);
    PV_ASSERT(other);

    if (!pv_cnn_depthwise_param_is_equal(object->conv_depthwise_param, other->conv_depthwise_param)) {
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

    if (!pv_ypu_config_mem_is_equal(object->scale_param, other->scale_param)) {
        return false;
    }

    return true;
}

struct pv_convnext {
    const pv_convnext_param_t *param;

    pv_cnn_depthwise_t *conv_depthwise;
    pv_layer_norm_t *layer_norm;
    pv_cnn_t *conv_1;
    pv_cnn_t *conv_2;

    pv_ypu_mem_t *scale;

    int32_t cache_length;
    pv_ypu_mem_t *cache;
};

pv_status_t PV_MOCKABLE(pv_convnext_init)(
        pv_ypu_t *ypu,
        const pv_convnext_param_t *param,
        pv_convnext_t **object) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(object);

    *object = NULL;

    pv_convnext_t *o = pv_ypu_host_alloc(
            ypu,
            sizeof(pv_convnext_t));
    if (!o) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_host_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(o, 0, sizeof(pv_convnext_t));

    o->param = param;

    pv_status_t status = pv_cnn_depthwise_init(
            ypu,
            param->conv_depthwise_param,
            &(o->conv_depthwise),
            true);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_depthwise_init,
                pv_status_to_string(status));
        pv_convnext_delete(ypu, o);
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
        pv_convnext_delete(ypu, o);
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
        pv_convnext_delete(ypu, o);
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
        pv_convnext_delete(ypu, o);
        return status;
    }

    o->scale = pv_ypu_mem_from_config(
            ypu,
            param->scale_param);
    if (!o->scale) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o->scale"));
        pv_convnext_delete(ypu, o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    int32_t padding = pv_cnn_depthwise_padding(o->conv_depthwise);
    int32_t num_channels = pv_cnn_depthwise_num_channels(o->conv_depthwise);

    o->cache = pv_ypu_mem_alloc(
            ypu,
            padding * num_channels * (int32_t) sizeof(float),
            PV_YPU_DEVICE_MEM_FLAG_NONE);
    if (!o->cache) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->cache_length = 0;

    pv_ypu_op_memset_args_t args = {
            .output = o->cache,
            .size_bytes = padding * num_channels * (int32_t) sizeof(float),
            0,
    };
    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_MEMSET,
            &args);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_MEMSET),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        return status;
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_convnext_delete)(
        pv_ypu_t *ypu,
        pv_convnext_t *object) {
    PV_ASSERT(ypu);

    if (object) {
        pv_cnn_delete(ypu, object->conv_2);
        pv_cnn_delete(ypu, object->conv_1);

        pv_layer_norm_delete(ypu, object->layer_norm);

        pv_cnn_depthwise_delete(ypu, object->conv_depthwise);

        pv_ypu_mem_free(ypu, object->scale);

        pv_ypu_mem_free(ypu, object->cache);

        pv_ypu_host_free(ypu, object);
    }
}

pv_status_t PV_MOCKABLE(pv_convnext_reset_cache)(
        pv_ypu_t *ypu,
        pv_convnext_t *object) {
    PV_ASSERT(ypu);
    PV_ASSERT(object);

    int32_t padding = pv_cnn_depthwise_padding(object->conv_depthwise);
    int32_t num_channels = pv_cnn_depthwise_num_channels(object->conv_depthwise);

    pv_status_t status = pv_cnn_depthwise_reset_cache(ypu, object->conv_depthwise);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_depthwise_reset_cache,
                pv_status_to_string(status));
        return status;
    }

    pv_ypu_op_memset_args_t args = {
            .output = object->cache,
            .size_bytes = padding * num_channels * (int32_t) sizeof(float),
            0,
    };
    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_MEMSET,
            &args);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_MEMSET),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        return status;
    }

    object->cache_length = 0;

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_convnext_forward)(
        pv_ypu_t *ypu,
        pv_convnext_t *object,
        int32_t n,
        pv_ypu_mem_t *x_ypu) {
    PV_ASSERT(ypu);
    PV_ASSERT(object);
    PV_ASSERT(n);
    PV_ASSERT(x_ypu);

    pv_ypu_mem_t *buffer_1_ypu = pv_ypu_buffer_get(
            ypu,
            object->param->conv_depthwise_param->num_channels * n * ((int32_t) sizeof(float)),
            false);
    if (!buffer_1_ypu) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("buffer_1"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_status_t status = pv_cnn_depthwise_forward(
            ypu,
            object->conv_depthwise,
            n,
            x_ypu,
            buffer_1_ypu,
            0,
            0);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_depthwise_forward,
                pv_status_to_string(status));
        pv_ypu_buffer_release(ypu, buffer_1_ypu);
        return status;
    }

    status = pv_layer_norm_forward(
            ypu,
            object->layer_norm,
            n,
            buffer_1_ypu,
            buffer_1_ypu);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_layer_norm_forward,
                pv_status_to_string(status));
        pv_ypu_buffer_release(ypu, buffer_1_ypu);
        return status;
    }

    pv_ypu_mem_t *buffer_2_ypu = pv_ypu_buffer_get(
            ypu,
            object->param->conv_1_param->output_channels * n * ((int32_t) sizeof(float)),
            false);
    if (!buffer_2_ypu) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("buffer_2"));
        pv_ypu_buffer_release(ypu, buffer_1_ypu);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    status = pv_cnn_forward(
            ypu,
            object->conv_1,
            n,
            buffer_1_ypu,
            buffer_2_ypu,
            0,
            0);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_forward,
                pv_status_to_string(status));
        pv_ypu_buffer_release(ypu, buffer_2_ypu);
        pv_ypu_buffer_release(ypu, buffer_1_ypu);
        return status;
    }

    pv_ypu_op_elementwise_args_t gelu_args = {
            .output = buffer_2_ypu,
            .input = buffer_2_ypu,
            .length = n * pv_cnn_output_channels(object->conv_1),
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
        pv_ypu_buffer_release(ypu, buffer_1_ypu);
        return status;
    }

    status = pv_cnn_forward(
            ypu,
            object->conv_2,
            n,
            buffer_2_ypu,
            buffer_1_ypu,
            0,
            0);
    pv_ypu_buffer_release(ypu, buffer_2_ypu);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_forward,
                pv_status_to_string(status));
        pv_ypu_buffer_release(ypu, buffer_1_ypu);
        return status;
    }

    const int32_t num_channels = pv_cnn_output_channels(object->conv_2);

    pv_ypu_op_pairwise_broadcast_args_t mulmv_iadd_args = {
            .output = x_ypu,
            .lhs = buffer_1_ypu,
            .rhs = object->scale,
            .m = n,
            .n = num_channels,
            .output_offset = 0,
            .lhs_offset = 0,
            .rhs_offset = 0,
    };
    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_MULMV_IADD,
            &mulmv_iadd_args);
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
        return status;
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_convnext_forward_with_cache)(
        pv_ypu_t *ypu,
        pv_convnext_t *object,
        int32_t n,
        pv_ypu_mem_t *x_ypu,
        bool is_flush,
        int32_t *n_out) {
    PV_ASSERT(ypu);
    PV_ASSERT(object);
    PV_ASSERT(n);
    PV_ASSERT(x_ypu);
    PV_ASSERT(n_out);
    PV_ASSERT(object->cache);

    int32_t cache_length = pv_cnn_depthwise_cache_length(object->conv_depthwise);
    int32_t padding = pv_cnn_depthwise_padding(object->conv_depthwise);
    int32_t n_max = n + cache_length + padding - 2 * padding;
    pv_ypu_mem_t *buffer_1_ypu = pv_ypu_buffer_get(
            ypu,
            object->param->conv_depthwise_param->num_channels * n_max * ((int32_t) sizeof(float)),
            false);
    if (!buffer_1_ypu) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("buffer_1"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    int32_t n_conv_depthwise_out = 0;
    pv_status_t status = pv_cnn_depthwise_forward_with_cache(
            ypu,
            object->conv_depthwise,
            n,
            x_ypu,
            buffer_1_ypu,
            0,
            0,
            is_flush,
            &n_conv_depthwise_out);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_depthwise_forward_with_cache,
                pv_status_to_string(status));
        pv_ypu_buffer_release(ypu, buffer_1_ypu);
        return status;
    }
    *n_out = n_conv_depthwise_out;
    assert(n_conv_depthwise_out <= n_max);

    status = pv_layer_norm_forward(
            ypu,
            object->layer_norm,
            n_conv_depthwise_out,
            buffer_1_ypu,
            buffer_1_ypu);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_layer_norm_forward,
                pv_status_to_string(status));
        pv_ypu_buffer_release(ypu, buffer_1_ypu);
        return status;
    }

    pv_ypu_mem_t *buffer_2_ypu = pv_ypu_buffer_get(
            ypu,
            object->param->conv_1_param->output_channels * n_conv_depthwise_out * ((int32_t) sizeof(float)),
            false);
    if (!buffer_2_ypu) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("buffer_2"));
        pv_ypu_buffer_release(ypu, buffer_1_ypu);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    status = pv_cnn_forward(
            ypu,
            object->conv_1,
            n_conv_depthwise_out,
            buffer_1_ypu,
            buffer_2_ypu,
            0,
            0);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_forward,
                pv_status_to_string(status));
        pv_ypu_buffer_release(ypu, buffer_2_ypu);
        pv_ypu_buffer_release(ypu, buffer_1_ypu);
        return status;
    }

    pv_ypu_op_elementwise_args_t gelu_args = {
            .output = buffer_2_ypu,
            .input = buffer_2_ypu,
            .length = n_conv_depthwise_out * pv_cnn_output_channels(object->conv_1),
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
        pv_ypu_buffer_release(ypu, buffer_1_ypu);
        return status;
    }

    status = pv_cnn_forward(
            ypu,
            object->conv_2,
            n_conv_depthwise_out,
            buffer_2_ypu,
            buffer_1_ypu,
            0,
            0);
    pv_ypu_buffer_release(ypu, buffer_2_ypu);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_forward,
                pv_status_to_string(status));
        pv_ypu_buffer_release(ypu, buffer_1_ypu);
        return status;
    }

    const int32_t num_channels = pv_cnn_output_channels(object->conv_2);

    float *buffer_residual = calloc(
            n_conv_depthwise_out * num_channels,
            sizeof(float));
    if (!buffer_residual) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    int32_t residual_cache_load_length = pv_min_int32(object->cache_length, n_conv_depthwise_out);
    if (residual_cache_load_length > 0) {
        pv_ypu_mem_copy_from(
                ypu,
                object->cache,
                buffer_residual,
                0,
                residual_cache_load_length * num_channels * (int32_t) sizeof(float));
    }

    if (residual_cache_load_length < n_conv_depthwise_out) {
        pv_ypu_mem_copy_from(
                ypu,
                x_ypu,
                buffer_residual + residual_cache_load_length * num_channels,
                0,
                (n_conv_depthwise_out - residual_cache_load_length) * num_channels * (int32_t) sizeof(float));
    }

    if (object->cache_length - residual_cache_load_length > 0) {
        pv_ypu_op_memmove_args_t args = {
                .output = object->cache,
                .batch_size = 1,
                .size_bytes = object->cache_length * num_channels * (int32_t) sizeof(float),
                .input_offset = residual_cache_load_length * (int32_t) sizeof(float),
        };
        pv_status_t status = pv_ypu_operator_execute(
                ypu,
                PV_YPU_OPERATOR_MEMMOVE,
                &args);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT(
                    &pv_error_msg_ypu_operator_fail,
                    PV_ERROR_ARGS_PUBLIC("execute"),
                    PV_ERROR_ARGS_PRIVATE(
                            "execute",
                            pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_MEMMOVE),
                            pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                            pv_status_to_string(status)));
            return status;
        }
    }

    if (n > 0) {
        pv_ypu_op_memcpy_args_t args = {
                .output = object->cache,
                .input = x_ypu,
                .size_bytes = (padding - (object->cache_length - residual_cache_load_length)) * num_channels * (int32_t) sizeof(float),
                .output_offset = (object->cache_length - residual_cache_load_length) * num_channels * (int32_t) sizeof(float),
                .input_offset = (n - (padding - (object->cache_length - residual_cache_load_length))) * num_channels * (int32_t) sizeof(float),
        };
        pv_status_t status = pv_ypu_operator_execute(
                ypu,
                PV_YPU_OPERATOR_MEMCPY,
                &args);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT(
                    &pv_error_msg_ypu_operator_fail,
                    PV_ERROR_ARGS_PUBLIC("execute"),
                    PV_ERROR_ARGS_PRIVATE(
                            "execute",
                            pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_MEMCPY),
                            pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                            pv_status_to_string(status)));
            return status;
        }
    }

    object->cache_length = padding;

    pv_ypu_mem_copy_to(
            ypu,
            x_ypu,
            buffer_residual,
            0,
            n_conv_depthwise_out * num_channels * (int32_t) sizeof(float));
    free(buffer_residual);
    buffer_residual = NULL;

    pv_ypu_op_pairwise_broadcast_args_t mulmv_iadd_args = {
            .output = x_ypu,
            .lhs = buffer_1_ypu,
            .rhs = object->scale,
            .m = n_conv_depthwise_out,
            .n = num_channels,
            .output_offset = 0,
            .lhs_offset = 0,
            .rhs_offset = 0,
    };
    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_MULMV_IADD,
            &mulmv_iadd_args);
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
        return status;
    }

    return PV_STATUS_SUCCESS;
}
