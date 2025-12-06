#include <stdlib.h>
#include <string.h>

#include "core/pv_error_messages.h"
#include "math/pv_math.h"
#include "orca/pv_cnn.h"
#include "orca/pv_orca_util.h"
#include "orca/pv_profiler.h"
#include "util/pv_check_status.h"
#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"
#include "ypu/mock/pv_ypu_mock.h"

#endif

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_cnn_param_serialize_buffer)(
        pv_ypu_t *ypu,
        const pv_cnn_param_t *param,
        size_t *length,
        void **buffer) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);

    int32_t num_bias_params = param->output_channels;
    int32_t num_weight_params = param->input_channels * param->output_channels * param->kernel_size;

    *length =
            sizeof(int32_t) +
            sizeof(int32_t) +
            sizeof(int32_t) +
            sizeof(int32_t) +
            sizeof(int32_t) +
            sizeof(int32_t) +
            (sizeof(q7_t) * num_bias_params) +
            (sizeof(q7_t) * num_weight_params);
    *buffer = NULL;

    void *b = pv_ypu_host_alloc(ypu, *length);
    PV_CHECK_ALLOC(b);
    *buffer = b;

    memcpy(b, &(param->input_channels), sizeof(int32_t));
    b += sizeof(int32_t);

    memcpy(b, &(param->output_channels), sizeof(int32_t));
    b += sizeof(int32_t);

    memcpy(b, &(param->kernel_size), sizeof(int32_t));
    b += sizeof(int32_t);

    memcpy(b, &(param->stride), sizeof(int32_t));
    b += sizeof(int32_t);

    memcpy(b, &(param->padding), sizeof(int32_t));
    b += sizeof(int32_t);

    memcpy(b, &(param->dilation), sizeof(int32_t));
    b += sizeof(int32_t);

    memcpy(b, param->bias->data, sizeof(q7_t) * num_bias_params);
    b += sizeof(q7_t) * num_bias_params;

    memcpy(b, param->weight->data, sizeof(q7_t) * num_weight_params);

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_cnn_param_serialize)(pv_ypu_t *ypu, const pv_cnn_param_t *param, FILE *file) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(file);

    size_t length = 0;
    void *buffer = NULL;
    const pv_status_t status = pv_cnn_param_serialize_buffer(ypu, param, &length, &buffer);
    PV_CHECK_STATUS(status);

    const size_t count = fwrite(buffer, 1, length, file);
    pv_ypu_host_free(ypu, buffer);

    return (count == length) ? PV_STATUS_SUCCESS : PV_STATUS_IO_ERROR;
}

#endif

pv_status_t PV_MOCKABLE(pv_cnn_param_load)(pv_ypu_t *ypu, FILE *f, pv_cnn_param_t **param) {
    PV_ASSERT(ypu);
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_cnn_param_t *p = pv_ypu_host_alloc(ypu, sizeof(pv_cnn_param_t));
    PV_CHECK_ALLOC(p);

    memset(p, 0, sizeof(pv_cnn_param_t));

    size_t count = pv_fread(&(p->input_channels), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_cnn_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->input_channels <= 0) {
        pv_cnn_param_delete(ypu, p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    count = pv_fread(&(p->output_channels), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_cnn_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->output_channels <= 0) {
        pv_cnn_param_delete(ypu, p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    count = pv_fread(&(p->kernel_size), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_cnn_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->kernel_size <= 0) {
        pv_cnn_param_delete(ypu, p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    count = pv_fread(&(p->stride), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_cnn_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->stride < 0) {
        pv_cnn_param_delete(ypu, p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    count = pv_fread(&(p->padding), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_cnn_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->padding < 0) {
        pv_cnn_param_delete(ypu, p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    count = pv_fread(&(p->dilation), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_cnn_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->dilation < 0) {
        pv_cnn_param_delete(ypu, p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    const int32_t num_bias_params = p->output_channels;
    const int32_t num_weight_params = p->input_channels * p->output_channels * p->kernel_size;

    p->bias = pv_ypu_config_mem_alloc(
            ypu,
            sizeof(q7_t) * num_bias_params,
            PV_YPU_DEVICE_MEM_FLAG_STATIC);
    if (!(p->bias)) {
        pv_cnn_param_delete(ypu, p);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    count = pv_fread(p->bias->data, sizeof(q7_t), num_bias_params, f);
    if (count != (size_t) num_bias_params) {
        pv_cnn_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }

    p->weight = pv_ypu_config_mem_alloc(
            ypu,
            sizeof(q7_t) * num_weight_params,
            PV_YPU_DEVICE_MEM_FLAG_STATIC);
    if (!(p->weight)) {
        pv_cnn_param_delete(ypu, p);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    count = pv_fread(p->weight->data, sizeof(q7_t), num_weight_params, f);
    if (count != (size_t) num_weight_params) {
        pv_cnn_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}


void PV_MOCKABLE(pv_cnn_param_delete)(pv_ypu_t *ypu, pv_cnn_param_t *param) {
    PV_ASSERT(ypu);

    if (param) {
        pv_ypu_config_mem_free(ypu, param->weight);
        pv_ypu_config_mem_free(ypu, param->bias);
        pv_ypu_host_free(ypu, param);
    }
}

bool PV_MOCKABLE(pv_cnn_param_is_equal)(
        const pv_cnn_param_t *object,
        const pv_cnn_param_t *other) {
    PV_ASSERT(object);
    PV_ASSERT(other);

    if (object->input_channels != other->input_channels) {
        return false;
    }

    if (object->output_channels != other->output_channels) {
        return false;
    }

    if (object->kernel_size != other->kernel_size) {
        return false;
    }

    if (!pv_ypu_config_mem_is_equal(object->bias, other->bias)) {
        return false;
    }

    if (!pv_ypu_config_mem_is_equal(object->weight, other->weight)) {
        return false;
    }

    return true;
}

int32_t PV_MOCKABLE(pv_cnn_param_receptive_field)(const pv_cnn_param_t *param) {
    PV_ASSERT(param);

    PV_ASSERT(param->dilation == 1);
    PV_ASSERT(param->stride == 1);

    PV_ASSERT((param->kernel_size % 2) == 1);
    return (param->kernel_size - 1) / 2;
}

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_cnn_depthwise_param_serialize_buffer)(
        pv_ypu_t *ypu,
        const pv_cnn_depthwise_param_t *param,
        size_t *length,
        void **buffer) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(length);
    PV_ASSERT(buffer);

    const int32_t num_bias_params = param->num_channels;
    const int32_t num_weight_params = param->num_channels * param->kernel_size;

    *length =
            sizeof(int32_t) +
            sizeof(int32_t) +
            sizeof(int32_t) +
            sizeof(int32_t) +
            sizeof(int32_t) +
            (sizeof(q7_t) * num_bias_params) + // bias
            (sizeof(q7_t) * num_weight_params); // weight
    *buffer = NULL;

    void *b = pv_ypu_host_alloc(ypu, *length);
    PV_CHECK_ALLOC(b);
    *buffer = b;

    memcpy(b, &(param->num_channels), sizeof(int32_t));
    b += sizeof(int32_t);

    memcpy(b, &(param->kernel_size), sizeof(int32_t));
    b += sizeof(int32_t);

    memcpy(b, &(param->stride), sizeof(int32_t));
    b += sizeof(int32_t);

    memcpy(b, &(param->padding), sizeof(int32_t));
    b += sizeof(int32_t);

    memcpy(b, &(param->dilation), sizeof(int32_t));
    b += sizeof(int32_t);

    memcpy(b, param->bias->data, sizeof(q7_t) * num_bias_params);
    b += sizeof(q7_t) * num_bias_params;

    memcpy(b, param->weight->data, sizeof(q7_t) * num_weight_params);

    return PV_STATUS_SUCCESS;
}


pv_status_t PV_MOCKABLE(pv_cnn_depthwise_param_serialize)(
        pv_ypu_t *ypu,
        const pv_cnn_depthwise_param_t *param,
        FILE *file) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(file);

    size_t length = 0;
    void *buffer = NULL;
    const pv_status_t status = pv_cnn_depthwise_param_serialize_buffer(ypu, param, &length, &buffer);
    PV_CHECK_STATUS(status);

    const size_t count = fwrite(buffer, 1, length, file);
    pv_ypu_host_free(ypu, buffer);

    return (count == length) ? PV_STATUS_SUCCESS : PV_STATUS_IO_ERROR;
}

#endif

pv_status_t PV_MOCKABLE(pv_cnn_depthwise_param_load)(pv_ypu_t *ypu, FILE *f, pv_cnn_depthwise_param_t **param) {
    PV_ASSERT(ypu);
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_cnn_depthwise_param_t *p = pv_ypu_host_alloc(ypu, sizeof(pv_cnn_depthwise_param_t));
    PV_CHECK_ALLOC(p);

    memset(p, 0, sizeof(pv_cnn_depthwise_param_t));

    size_t count = pv_fread(&(p->num_channels), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_cnn_depthwise_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->num_channels <= 0) {
        pv_cnn_depthwise_param_delete(ypu, p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    count = pv_fread(&(p->kernel_size), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_cnn_depthwise_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->kernel_size <= 0) {
        pv_cnn_depthwise_param_delete(ypu, p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    count = pv_fread(&(p->stride), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_cnn_depthwise_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->stride < 0) {
        pv_cnn_depthwise_param_delete(ypu, p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    count = pv_fread(&(p->padding), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_cnn_depthwise_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->padding < 0) {
        pv_cnn_depthwise_param_delete(ypu, p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    count = pv_fread(&(p->dilation), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_cnn_depthwise_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->dilation < 0) {
        pv_cnn_depthwise_param_delete(ypu, p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    const int32_t num_bias_params = p->num_channels;
    const int32_t num_weight_params = p->num_channels * p->kernel_size;

    p->bias = pv_ypu_config_mem_alloc(
            ypu,
            sizeof(q7_t) * num_bias_params,
            PV_YPU_DEVICE_MEM_FLAG_STATIC);
    if (!p->bias) {
        pv_cnn_depthwise_param_delete(ypu, p);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    count = pv_fread(p->bias->data, sizeof(q7_t), num_bias_params, f);
    if (count != (size_t) num_bias_params) {
        pv_cnn_depthwise_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }

    p->weight = pv_ypu_config_mem_alloc(
            ypu,
            sizeof(q7_t) * num_weight_params,
            PV_YPU_DEVICE_MEM_FLAG_STATIC);
    if (!p->weight) {
        pv_cnn_depthwise_param_delete(ypu, p);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    count = pv_fread(p->weight->data, sizeof(q7_t), num_weight_params, f);
    if (count != (size_t) num_weight_params) {
        pv_cnn_depthwise_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}


void PV_MOCKABLE(pv_cnn_depthwise_param_delete)(pv_ypu_t *ypu, pv_cnn_depthwise_param_t *param) {
    PV_ASSERT(ypu);

    if (param) {
        pv_ypu_config_mem_free(ypu, param->weight);
        pv_ypu_config_mem_free(ypu, param->bias);
        pv_ypu_host_free(ypu, param);
    }
}

bool PV_MOCKABLE(pv_cnn_depthwise_param_is_equal)(
        const pv_cnn_depthwise_param_t *object,
        const pv_cnn_depthwise_param_t *other) {
    PV_ASSERT(object);
    PV_ASSERT(other);

    if (object->num_channels != other->num_channels) {
        return false;
    }

    if (object->kernel_size != other->kernel_size) {
        return false;
    }

    if (!pv_ypu_config_mem_is_equal(object->bias, other->bias)) {
        return false;
    }

    if (!pv_ypu_config_mem_is_equal(object->weight, other->weight)) {
        return false;
    }

    return true;
}

struct pv_cnn {
    const pv_cnn_param_t *param;

    pv_ypu_mem_t *weight;
    pv_ypu_mem_t *bias;
    pv_ypu_mem_t *transposed_weight;
    pv_ypu_mem_t *bias_float;
};

int32_t PV_MOCKABLE(pv_cnn_output_channels)(const pv_cnn_t *object) {
    PV_ASSERT(object);

    return object->param->output_channels;
}

int32_t PV_MOCKABLE(pv_cnn_input_channels)(const pv_cnn_t *object) {
    PV_ASSERT(object);

    return object->param->input_channels;
}

int32_t PV_MOCKABLE(pv_cnn_kernel_size)(const pv_cnn_t *object) {
    PV_ASSERT(object);

    return object->param->kernel_size;
}

int32_t PV_MOCKABLE(pv_cnn_padding)(const pv_cnn_t *object) {
    PV_ASSERT(object);

    return object->param->padding;
}

int32_t PV_MOCKABLE(pv_cnn_dilation)(const pv_cnn_t *object) {
    PV_ASSERT(object);

    return object->param->dilation;
}

int32_t PV_MOCKABLE(pv_cnn_stride)(const pv_cnn_t *object) {
    PV_ASSERT(object);

    return object->param->stride;
}

pv_ypu_mem_t *PV_MOCKABLE(pv_cnn_get_weight)(const pv_cnn_t *object) {
    PV_ASSERT(object);

    if (object->transposed_weight) {
        return object->transposed_weight;
    } else {
        return object->weight;
    }
}

pv_status_t PV_MOCKABLE(pv_cnn_init)(
        pv_ypu_t *ypu,
        const pv_cnn_param_t *param,
        pv_cnn_t **object) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);

    *object = NULL;

    pv_cnn_t *o = pv_ypu_host_alloc(ypu, sizeof(pv_cnn_t));
    if (!o) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(o, 0, sizeof(pv_cnn_t));

    o->param = param;

    if ((param->kernel_size > 1) && ((o->param->output_channels % 4) != 0)) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_min,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE(
                        "output_channels",
                        o->param->output_channels,
                        4));
        pv_cnn_delete(ypu, o);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    int32_t in_channels = param->input_channels;
    int32_t out_channels = param->output_channels;
    int32_t kernel_size = param->kernel_size;

    o->weight = pv_ypu_mem_from_config(ypu, param->weight);
    if (!o->weight) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o->weight"));
        pv_cnn_delete(ypu, o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->bias = pv_ypu_mem_from_config(ypu, param->bias);
    if (!o->bias) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o->bias"));
        pv_cnn_delete(ypu, o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->bias_float = pv_ypu_mem_alloc(
            ypu,
            out_channels * (int32_t) sizeof(float),
            PV_YPU_DEVICE_MEM_FLAG_NONE);
    if (!o->bias_float) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o->bias_float"));
        pv_cnn_delete(ypu, o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_op_elementwise_args_t args = {
            .output = o->bias_float,
            .input = o->bias,
            .length = out_channels,
            .output_offset = 0,
            .input_offset = 0,
    };

    pv_status_t status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_CONVERT_F32_Q7,
            &args);
    if (status != PV_STATUS_SUCCESS) {
        pv_cnn_delete(ypu, o);
        return status;
    }

    if (param->kernel_size > 1) {
        o->transposed_weight = pv_ypu_mem_alloc(
                ypu,
                out_channels * in_channels * kernel_size * (int32_t) sizeof(q7_t),
                PV_YPU_DEVICE_MEM_FLAG_NONE);
        if (!o->transposed_weight) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("o->transposed_weight"));
            pv_cnn_delete(ypu, o);
            return PV_STATUS_OUT_OF_MEMORY;
        }

        pv_ypu_op_transpose_args_t args = {
                .output = o->transposed_weight,
                .input = o->weight,
                .m = out_channels,
                .n = in_channels,
                .k = kernel_size,
                .output_offset = 0,
                .input_offset = 0,
        };

        pv_status_t status = pv_ypu_operator_execute(
                ypu,
                PV_YPU_OPERATOR_TRANSPOSE_Q7,
                &args);
        if (status != PV_STATUS_SUCCESS) {
            return status;
        }
    } else {
        o->transposed_weight = NULL;
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_cnn_delete)(pv_ypu_t *ypu, pv_cnn_t *object) {
    PV_ASSERT(ypu);

    if (object) {
        if (object->transposed_weight) {
            pv_ypu_mem_free(ypu, object->transposed_weight);
        }

        pv_ypu_mem_free(ypu, object->weight);
        pv_ypu_mem_free(ypu, object->bias);
        pv_ypu_mem_free(ypu, object->bias_float);

        pv_ypu_host_free(ypu, object);
    }
}

pv_status_t PV_MOCKABLE(pv_cnn_forward)(
        pv_ypu_t *ypu,
        pv_cnn_t *object,
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

    const int32_t in_channels = pv_cnn_input_channels(object);
    const int32_t out_channels = pv_cnn_output_channels(object);
    const int32_t kernel_size = pv_cnn_kernel_size(object);
    const int32_t padding = pv_cnn_padding(object);
    const int32_t padding_numel = padding * in_channels;

    pv_ypu_mem_t *x_q510_ypu_mem = pv_ypu_buffer_get(
            ypu,
            ((n * in_channels) + (2 * padding_numel)) * (int32_t) sizeof(q510_t),
            false);
    if (!x_q510_ypu_mem) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_mem_t *inverse_scale = pv_ypu_buffer_get(
            ypu,
            sizeof(float),
            false);
    if (!inverse_scale) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_op_memset_args_t args0 = {
            .output = x_q510_ypu_mem,
            .size_bytes = ((n * in_channels) + (2 * padding_numel)) * (int32_t) sizeof(q510_t),
            .output_offset = 0,
    };

    pv_status_t status = pv_ypu_operator_execute(
            ypu, PV_YPU_OPERATOR_MEMSET, &args0);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_op_quantize_args_t args1 = {
            .output = x_q510_ypu_mem,
            .scale = inverse_scale,
            .input = x_ypu_mem,
            .m = 1,
            .n = n * in_channels,
            .output_offset = padding_numel * (int32_t) sizeof(q510_t),
            .scale_offset = 0,
            .input_offset = x_offset,
    };

    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_QUANTIZE_Q510,
            &args1);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_op_conv1d_args_t args2 = {
            .output = y_ypu_mem,
            .lhs = pv_cnn_get_weight(object),
            .rhs = x_q510_ypu_mem,
            .n = n,
            .out_channels = out_channels,
            .in_channels = in_channels,
            .kernel_size = kernel_size,
            .output_offset = y_offset,
            .lhs_offset = 0,
            .rhs_offset = 0,
    };

    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_CONV1D_Q1417_Q7_Q510,
            &args2);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_op_elementwise_args_t args3 = {
            .output = y_ypu_mem,
            .input = y_ypu_mem,
            .length = n * out_channels,
            .output_offset = y_offset,
            .input_offset = y_offset,
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
            .m = n * out_channels,
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
            .m = n,
            .n = out_channels,
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

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_cnn_forward_to_q510)(
        pv_ypu_t *ypu,
        pv_cnn_t *object,
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

    const int32_t in_channels = pv_cnn_input_channels(object);
    const int32_t out_channels = pv_cnn_output_channels(object);
    const int32_t kernel_size = pv_cnn_kernel_size(object);
    const int32_t padding = pv_cnn_padding(object);
    const int32_t padding_numel = padding * in_channels;

    pv_ypu_mem_t *x_q510_ypu_mem = pv_ypu_buffer_get(
            ypu,
            ((n * in_channels) + (2 * padding_numel)) * (int32_t) sizeof(q510_t),
            false);
    if (!x_q510_ypu_mem) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_mem_t *buffer_ypu_mem = pv_ypu_buffer_get(
            ypu,
            n * out_channels * (int32_t) sizeof(int32_t),
            false);
    if (!buffer_ypu_mem) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_op_memset_args_t args0 = {
            .output = x_q510_ypu_mem,
            .size_bytes = ((n * in_channels) + (2 * padding_numel)) * (int32_t) sizeof(q510_t),
            .output_offset = 0};

    pv_status_t status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_MEMSET,
            &args0);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_op_elementwise_args_t args1 = {
            .output = x_q510_ypu_mem,
            .input = x_ypu_mem,
            .length = n * in_channels,
            .output_offset = padding_numel * (int32_t) sizeof(q510_t),
            .input_offset = x_offset};

    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_CONVERT_Q510_F32,
            &args1);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_op_conv1d_args_t args2 = {
            .output = buffer_ypu_mem,
            .lhs = pv_cnn_get_weight(object),
            .rhs = x_q510_ypu_mem,
            .n = n,
            .out_channels = out_channels,
            .in_channels = in_channels,
            .kernel_size = kernel_size,
            .output_offset = 0,
            .lhs_offset = 0,
            .rhs_offset = 0,
    };

    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_CONV1D_Q1417_Q7_Q510,
            &args2);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_op_pairwise_broadcast_args_t args3 = {
            .output = buffer_ypu_mem,
            .lhs = buffer_ypu_mem,
            .rhs = object->bias,
            .m = n,
            .n = out_channels,
            .output_offset = 0,
            .lhs_offset = 0,
            .rhs_offset = 0,
    };

    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_ADDMV_Q1417_Q1417_Q7,
            &args3);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_op_elementwise_args_t args4 = {
            .output = y_ypu_mem,
            .input = buffer_ypu_mem,
            .length = n * out_channels,
            .output_offset = y_offset,
            .input_offset = 0,
    };

    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_CONVERT_Q510_Q1417,
            &args4);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_buffer_release(ypu, buffer_ypu_mem);
    pv_ypu_buffer_release(ypu, x_q510_ypu_mem);

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_cnn_forward_from_q510)(
        pv_ypu_t *ypu,
        pv_cnn_t *object,
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

    const int32_t in_channels = pv_cnn_input_channels(object);
    const int32_t out_channels = pv_cnn_output_channels(object);
    const int32_t kernel_size = pv_cnn_kernel_size(object);

    const float inverse_scale = 1.0f / (1024.0f * 128.0f);

    pv_ypu_op_conv1d_args_t args0 = {
            .output = y_ypu_mem,
            .lhs = pv_cnn_get_weight(object),
            .rhs = x_ypu_mem,
            .n = n,
            .out_channels = out_channels,
            .in_channels = in_channels,
            .kernel_size = kernel_size,
            .output_offset = y_offset,
            .lhs_offset = x_offset,
            .rhs_offset = 0,
    };

    pv_status_t status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_CONV1D_Q1417_Q7_Q510,
            &args0);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_op_elementwise_args_t args1 = {
            .output = y_ypu_mem,
            .input = y_ypu_mem,
            .length = n * out_channels,
            .output_offset = y_offset,
            .input_offset = y_offset,
    };

    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_CONVERT_F32_I32,
            &args1);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_op_elementwise_scalar_args_t args2 = {
            .output = y_ypu_mem,
            .input = y_ypu_mem,
            .scalar.f32 = inverse_scale,
            .length = n * out_channels,
            .output_offset = y_offset,
            .input_offset = y_offset,
    };

    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_MULSV,
            &args2);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_op_pairwise_broadcast_args_t args3 = {
            .output = y_ypu_mem,
            .lhs = y_ypu_mem,
            .rhs = object->bias_float,
            .m = n,
            .n = out_channels,
            .output_offset = y_offset,
            .lhs_offset = y_offset,
            .rhs_offset = 0,
    };

    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_ADDMV,
            &args3);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

struct pv_cnn_depthwise {
    const pv_cnn_depthwise_param_t *param;

    pv_ypu_mem_t *weight;
    pv_ypu_mem_t *bias;
    pv_ypu_mem_t *transposed_weight;
    pv_ypu_mem_t *bias_float;
};

pv_status_t PV_MOCKABLE(pv_cnn_depthwise_init)(
        pv_ypu_t *ypu,
        const pv_cnn_depthwise_param_t *param,
        pv_cnn_depthwise_t **object) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(object);

    *object = NULL;

    pv_cnn_depthwise_t *o = pv_ypu_host_alloc(ypu, sizeof(pv_cnn_depthwise_t));
    if (!o) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(o, 0, sizeof(pv_cnn_depthwise_t));

    o->param = param;

    int32_t kernel_size = pv_cnn_depthwise_kernel_size(o);
    int32_t num_channels = pv_cnn_depthwise_num_channels(o);

    o->weight = pv_ypu_mem_from_config(ypu, param->weight);
    if (!o->weight) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o->weight"));
        pv_cnn_depthwise_delete(ypu, o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->bias = pv_ypu_mem_from_config(ypu, param->bias);
    if (!o->bias) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o->bias"));
        pv_cnn_depthwise_delete(ypu, o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->bias_float = pv_ypu_mem_alloc(
            ypu,
            num_channels * (int32_t) sizeof(float),
            PV_YPU_DEVICE_MEM_FLAG_NONE);
    if (!o->bias_float) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o->bias_float"));
        pv_cnn_depthwise_delete(ypu, o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_op_elementwise_args_t args = {
            .output = o->bias_float,
            .input = o->bias,
            .length = num_channels,
            .output_offset = 0,
            .input_offset = 0,
    };

    pv_status_t status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_CONVERT_F32_Q7,
            &args);
    if (status != PV_STATUS_SUCCESS) {
        pv_cnn_depthwise_delete(ypu, o);
        return status;
    }

    if (param->kernel_size > 1) {
        o->transposed_weight = pv_ypu_mem_alloc(
                ypu,
                num_channels * kernel_size * (int32_t) sizeof(q7_t),
                PV_YPU_DEVICE_MEM_FLAG_NONE);
        if (!o->transposed_weight) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("o->transposed_weight"));
            pv_cnn_depthwise_delete(ypu, o);
            return PV_STATUS_OUT_OF_MEMORY;
        }

        pv_ypu_op_transpose_args_t args = {
                .output = o->transposed_weight,
                .input = o->weight,
                .m = 1,
                .n = num_channels,
                .k = kernel_size,
                .output_offset = 0,
                .input_offset = 0,
        };

        status = pv_ypu_operator_execute(
                ypu,
                PV_YPU_OPERATOR_TRANSPOSE_Q7,
                &args);
        if (status != PV_STATUS_SUCCESS) {
            pv_cnn_depthwise_delete(ypu, o);
            return status;
        }
    } else {
        o->transposed_weight = NULL;
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_cnn_depthwise_delete)(pv_ypu_t *ypu, pv_cnn_depthwise_t *object) {
    PV_ASSERT(ypu);

    if (object) {
        if (object->transposed_weight) {
            pv_ypu_mem_free(ypu, object->transposed_weight);
        }

        pv_ypu_mem_free(ypu, object->weight);
        pv_ypu_mem_free(ypu, object->bias);
        pv_ypu_mem_free(ypu, object->bias_float);

        pv_ypu_host_free(ypu, object);
    }
}

int32_t PV_MOCKABLE(pv_cnn_depthwise_num_channels)(const pv_cnn_depthwise_t *object) {
    PV_ASSERT(object);

    return object->param->num_channels;
}

int32_t PV_MOCKABLE(pv_cnn_depthwise_kernel_size)(const pv_cnn_depthwise_t *object) {
    PV_ASSERT(object);

    return object->param->kernel_size;
}

int32_t PV_MOCKABLE(pv_cnn_depthwise_stride)(const pv_cnn_depthwise_t *object) {
    PV_ASSERT(object);

    return object->param->stride;
}

pv_ypu_mem_t *PV_MOCKABLE(pv_cnn_depthwise_get_weight)(const pv_cnn_depthwise_t *object) {
    PV_ASSERT(object);

    if (object->transposed_weight) {
        return object->transposed_weight;
    } else {
        return object->weight;
    }
}

pv_status_t PV_MOCKABLE(pv_cnn_depthwise_forward)(
        pv_ypu_t *ypu,
        pv_cnn_depthwise_t *object,
        int32_t n,
        pv_ypu_mem_t *x_ypu_mem,
        pv_ypu_mem_t *y_ypu_mem,
        int32_t x_offset,
        int32_t y_offset) {
    PV_ASSERT(ypu);
    PV_ASSERT(object);
    PV_ASSERT(n > 0);
    PV_ASSERT(x_ypu_mem);
    PV_ASSERT(y_ypu_mem);
    PV_ASSERT(x_offset >= 0);
    PV_ASSERT(y_offset >= 0);

    int32_t num_channels = pv_cnn_depthwise_num_channels(object);
    int32_t kernel_size = object->param->kernel_size;

    int32_t padding = object->param->padding;
    const int32_t padding_numel = padding * num_channels;

    const int32_t x_q510_size_bytes = ((n * num_channels) + (2 * padding_numel)) * (int32_t) sizeof(q510_t);
    pv_ypu_mem_t *x_q510_ypu_mem = pv_ypu_buffer_get(
            ypu,
            x_q510_size_bytes,
            false);
    if (!x_q510_ypu_mem) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_mem_t *inverse_scale = pv_ypu_buffer_get(
            ypu,
            sizeof(float),
            false);
    if (!inverse_scale) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_op_memset_args_t args0 = {
            .output = x_q510_ypu_mem,
            .size_bytes = x_q510_size_bytes,
            .output_offset = 0,
    };

    pv_status_t status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_MEMSET,
            &args0);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_op_memset_args_t args1 = {
            .output = y_ypu_mem,
            .size_bytes = n * num_channels * (int32_t) sizeof(float),
            .output_offset = y_offset,
    };

    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_MEMSET,
            &args1);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_op_quantize_args_t args2 = {
            .output = x_q510_ypu_mem,
            .scale = inverse_scale,
            .input = x_ypu_mem,
            .m = 1,
            .n = n * num_channels,
            .output_offset = padding_numel * (int32_t) sizeof(q510_t),
            .scale_offset = 0,
            .input_offset = x_offset,
    };

    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_QUANTIZE_Q510,
            &args2);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_op_conv1d_depthwise_args_t args3 = {
            .output = y_ypu_mem,
            .lhs = pv_cnn_depthwise_get_weight(object),
            .rhs = x_q510_ypu_mem,
            .n = n,
            .num_channels = num_channels,
            .kernel_size = kernel_size,
            .output_offset = y_offset,
            .lhs_offset = 0,
            .rhs_offset = 0,
    };

    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_CONV1D_DEPTHWISE_Q1417_Q7_Q510,
            &args3);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_op_elementwise_args_t args4 = {
            .output = y_ypu_mem,
            .input = y_ypu_mem,
            .length = n * num_channels,
            .output_offset = y_offset,
            .input_offset = y_offset,
    };

    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_CONVERT_F32_I32,
            &args4);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_op_pairwise_broadcast_args_t args5 = {
            .output = y_ypu_mem,
            .lhs = y_ypu_mem,
            .rhs = inverse_scale,
            .m = n * num_channels,
            .n = 1,
            .output_offset = y_offset,
            .lhs_offset = y_offset,
            .rhs_offset = 0,
    };

    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_MULMV,
            &args5);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_op_pairwise_broadcast_args_t args6 = {
            .output = y_ypu_mem,
            .lhs = y_ypu_mem,
            .rhs = object->bias_float,
            .m = n,
            .n = num_channels,
            .output_offset = y_offset,
            .lhs_offset = y_offset,
            .rhs_offset = 0,
    };

    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_ADDMV,
            &args6);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_buffer_release(ypu, inverse_scale);
    pv_ypu_buffer_release(ypu, x_q510_ypu_mem);

    return PV_STATUS_SUCCESS;
}
