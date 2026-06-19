#include <stdlib.h>
#include <string.h>

#include "core/pv_error_messages.h"
#include "math/pv_math.h"
#include "orca/pv_affine.h"
#include "orca/pv_cnn.h"
#include "orca/pv_orca_util.h"
#include "util/pv_check_status.h"
#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_cnn_param_serialize_buffer)(
        pv_ypu_t *ypu,
        const pv_cnn_param_t *param,
        size_t *length,
        void **buffer) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);

    int32_t num_int8_inverse_scale_params = param->output_channels;
    int32_t num_bias_params = param->output_channels;
    int32_t num_weight_params = param->input_channels * param->output_channels * param->kernel_size;

    *length =
            sizeof(int32_t) +
            sizeof(int32_t) +
            sizeof(int32_t) +
            sizeof(int32_t) +
            sizeof(int32_t) +
            sizeof(int32_t) +
            (sizeof(float) * num_int8_inverse_scale_params) +
            (sizeof(float) * num_bias_params) +
            (sizeof(q7_t) * num_weight_params);
    *buffer = NULL;

    void *b = pv_ypu_host_alloc(ypu, (int32_t) (*length));
    if (!b) {
        return PV_STATUS_OUT_OF_MEMORY;
    }
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

    memcpy(b, param->int8_inverse_scale->data, sizeof(float) * num_int8_inverse_scale_params);
    b += sizeof(float) * num_int8_inverse_scale_params;

    memcpy(b, param->bias->data, sizeof(float) * num_bias_params);
    b += sizeof(float) * num_bias_params;

    memcpy(b, param->weight->data, sizeof(q7_t) * num_weight_params);

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_cnn_param_serialize)(
        pv_ypu_t *ypu,
        const pv_cnn_param_t *param,
        FILE *file) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(file);

    size_t length = 0;
    void *buffer = NULL;
    const pv_status_t status = pv_cnn_param_serialize_buffer(ypu, param, &length, &buffer);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    const size_t count = fwrite(buffer, 1, length, file);
    pv_ypu_host_free(ypu, buffer);

    return (count == length) ? PV_STATUS_SUCCESS : PV_STATUS_IO_ERROR;
}

#endif

pv_status_t PV_MOCKABLE(pv_cnn_param_load)(
        pv_ypu_t *ypu,
        FILE *f,
        pv_cnn_param_t **param) {
    PV_ASSERT(ypu);
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_cnn_param_t *p = pv_ypu_host_alloc(ypu, sizeof(pv_cnn_param_t));
    if (!p) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

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

    const int32_t num_int8_inverse_scale_params = p->output_channels;
    const int32_t num_bias_params = p->output_channels;
    const int32_t num_weight_params = p->input_channels * p->output_channels * p->kernel_size;

    p->int8_inverse_scale = pv_ypu_config_mem_alloc(
            ypu,
            (int32_t) sizeof(float) * num_int8_inverse_scale_params,
            PV_YPU_DEVICE_MEM_FLAG_STATIC);
    if (!(p->int8_inverse_scale)) {
        pv_cnn_param_delete(ypu, p);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    count = pv_fread(
            p->int8_inverse_scale->data,
            sizeof(float),
            num_int8_inverse_scale_params,
            f);
    if (count != (size_t) num_int8_inverse_scale_params) {
        pv_cnn_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }

    p->bias = pv_ypu_config_mem_alloc(
            ypu,
            (int32_t) sizeof(float) * num_bias_params,
            PV_YPU_DEVICE_MEM_FLAG_STATIC);
    if (!(p->bias)) {
        pv_cnn_param_delete(ypu, p);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    count = pv_fread(
            p->bias->data,
            sizeof(float),
            num_bias_params,
            f);
    if (count != (size_t) num_bias_params) {
        pv_cnn_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }

    p->weight = pv_ypu_config_mem_alloc(
            ypu,
            (int32_t) sizeof(q7_t) * num_weight_params,
            PV_YPU_DEVICE_MEM_FLAG_STATIC);
    if (!(p->weight)) {
        pv_cnn_param_delete(ypu, p);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    count = pv_fread(
            p->weight->data,
            sizeof(q7_t),
            num_weight_params,
            f);
    if (count != (size_t) num_weight_params) {
        pv_cnn_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}


void PV_MOCKABLE(pv_cnn_param_delete)(
        pv_ypu_t *ypu,
        pv_cnn_param_t *param) {
    PV_ASSERT(ypu);

    if (param) {
        pv_ypu_config_mem_free(ypu, param->weight);
        pv_ypu_config_mem_free(ypu, param->bias);
        pv_ypu_config_mem_free(ypu, param->int8_inverse_scale);

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

    if (!pv_ypu_config_mem_is_equal(object->int8_inverse_scale, other->int8_inverse_scale)) {
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

    const int32_t num_int8_inverse_scale_params = param->num_channels;
    const int32_t num_bias_params = param->num_channels;
    const int32_t num_weight_params = param->num_channels * param->kernel_size;

    *length =
            sizeof(int32_t) +
            sizeof(int32_t) +
            sizeof(int32_t) +
            sizeof(int32_t) +
            sizeof(int32_t) +
            (sizeof(float) * num_int8_inverse_scale_params) +
            (sizeof(float) * num_bias_params) +
            (sizeof(q7_t) * num_weight_params);
    *buffer = NULL;

    void *b = pv_ypu_host_alloc(ypu, (int32_t) (*length));
    if (!b) {
        return PV_STATUS_OUT_OF_MEMORY;
    }
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

    memcpy(b, param->int8_inverse_scale->data, sizeof(float) * num_int8_inverse_scale_params);
    b += sizeof(float) * num_int8_inverse_scale_params;

    memcpy(b, param->bias->data, sizeof(float) * num_bias_params);
    b += sizeof(float) * num_bias_params;

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
    const pv_status_t status = pv_cnn_depthwise_param_serialize_buffer(
            ypu,
            param,
            &length,
            &buffer);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    const size_t count = fwrite(buffer, 1, length, file);
    pv_ypu_host_free(ypu, buffer);

    return (count == length) ? PV_STATUS_SUCCESS : PV_STATUS_IO_ERROR;
}

#endif

pv_status_t PV_MOCKABLE(pv_cnn_depthwise_param_load)(
        pv_ypu_t *ypu,
        FILE *f,
        pv_cnn_depthwise_param_t **param) {
    PV_ASSERT(ypu);
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_cnn_depthwise_param_t *p = pv_ypu_host_alloc(ypu, sizeof(pv_cnn_depthwise_param_t));
    if (!p) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

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

    const int32_t num_int8_inverse_scale_params = p->num_channels;
    const int32_t num_bias_params = p->num_channels;
    const int32_t num_weight_params = p->num_channels * p->kernel_size;

    p->int8_inverse_scale = pv_ypu_config_mem_alloc(
            ypu,
            (int32_t) sizeof(float) * num_int8_inverse_scale_params,
            PV_YPU_DEVICE_MEM_FLAG_STATIC);
    if (!p->int8_inverse_scale) {
        pv_cnn_depthwise_param_delete(ypu, p);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    count = pv_fread(
            p->int8_inverse_scale->data,
            sizeof(float),
            num_int8_inverse_scale_params,
            f);
    if (count != (size_t) num_int8_inverse_scale_params) {
        pv_cnn_depthwise_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }

    p->bias = pv_ypu_config_mem_alloc(
            ypu,
            (int32_t) sizeof(float) * num_bias_params,
            PV_YPU_DEVICE_MEM_FLAG_STATIC);
    if (!p->bias) {
        pv_cnn_depthwise_param_delete(ypu, p);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    count = pv_fread(
            p->bias->data,
            sizeof(float),
            num_bias_params,
            f);
    if (count != (size_t) num_bias_params) {
        pv_cnn_depthwise_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }

    p->weight = pv_ypu_config_mem_alloc(
            ypu,
            (int32_t) sizeof(q7_t) * num_weight_params,
            PV_YPU_DEVICE_MEM_FLAG_STATIC);
    if (!p->weight) {
        pv_cnn_depthwise_param_delete(ypu, p);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    count = pv_fread(
            p->weight->data,
            sizeof(q7_t),
            num_weight_params,
            f);
    if (count != (size_t) num_weight_params) {
        pv_cnn_depthwise_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_cnn_depthwise_param_delete)(
        pv_ypu_t *ypu,
        pv_cnn_depthwise_param_t *param) {
    if (param) {
        pv_ypu_config_mem_free(ypu, param->weight);
        pv_ypu_config_mem_free(ypu, param->bias);
        pv_ypu_config_mem_free(ypu, param->int8_inverse_scale);

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

    if (!pv_ypu_config_mem_is_equal(object->int8_inverse_scale, other->int8_inverse_scale)) {
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
    pv_ypu_mem_t *int8_inverse_scale;
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
                &pv_error_msg_ypu_host_alloc,
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

    o->weight = pv_ypu_mem_from_config(ypu, param->weight);
    if (!o->weight) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o->weight"));
        pv_cnn_delete(ypu, o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->bias = pv_ypu_mem_from_config(ypu, param->bias);
    if (!o->bias) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o->bias"));
        pv_cnn_delete(ypu, o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->int8_inverse_scale = pv_ypu_mem_from_config(ypu, param->int8_inverse_scale);
    if (!o->int8_inverse_scale) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o->int8_inverse_scale"));
        pv_cnn_delete(ypu, o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    int32_t in_channels = param->input_channels;
    int32_t out_channels = param->output_channels;
    int32_t kernel_size = param->kernel_size;

    if (param->kernel_size > 1) {
        o->transposed_weight = pv_ypu_mem_alloc(
                ypu,
                out_channels * in_channels * kernel_size * (int32_t) sizeof(q7_t),
                PV_YPU_DEVICE_MEM_FLAG_NONE);
        if (!o->transposed_weight) {
            PV_ERROR_REPORT(
                    &pv_error_msg_ypu_device_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("o->transposed_weight"));
            pv_cnn_delete(ypu, o);
            return PV_STATUS_OUT_OF_MEMORY;
        }

        pv_ypu_op_transpose_args_t args = {
                .output = o->transposed_weight,
                .input = o->weight,
                .b = out_channels,
                .m = kernel_size,
                .n = in_channels,
                .k = sizeof(q7_t),
                .output_offset = 0,
                .input_offset = 0,
        };

        pv_status_t status = pv_ypu_operator_execute(
                ypu,
                PV_YPU_OPERATOR_TRANSPOSE,
                &args);
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
    } else {
        o->transposed_weight = NULL;
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_cnn_delete)(
        pv_ypu_t *ypu,
        pv_cnn_t *object) {
    PV_ASSERT(ypu);

    if (object) {
        if (object->transposed_weight) {
            pv_ypu_mem_free(ypu, object->transposed_weight);
        }

        pv_ypu_mem_free(ypu, object->weight);
        pv_ypu_mem_free(ypu, object->bias);
        pv_ypu_mem_free(ypu, object->int8_inverse_scale);

        pv_ypu_host_free(ypu, object);
    }
}

pv_status_t PV_MOCKABLE(pv_cnn_forward)(
        pv_ypu_t *ypu,
        pv_cnn_t *object,
        int32_t n,
        pv_ypu_mem_t *x_ypu,
        pv_ypu_mem_t *y_ypu,
        int32_t x_offset,
        int32_t y_offset) {
    PV_ASSERT(ypu);
    PV_ASSERT(object);
    PV_ASSERT(n);
    PV_ASSERT(x_ypu);
    PV_ASSERT(y_ypu);

    const int32_t in_channels = pv_cnn_input_channels(object);
    const int32_t out_channels = pv_cnn_output_channels(object);
    const int32_t kernel_size = pv_cnn_kernel_size(object);
    const int32_t padding = pv_cnn_padding(object);
    const int32_t padding_num_elements = padding * in_channels;

#ifdef __ORCA_FLOAT_MODE__

    float *x = (float *) (pv_ypu_mem_get_host_view(ypu, x_ypu, false) + x_offset);
    float *y = (float *) (pv_ypu_mem_get_host_view(ypu, y_ypu, false) + y_offset);
    float *int8_inverse_scale = (float *) pv_ypu_mem_get_host_view(ypu, object->int8_inverse_scale, false);
    float *bias = (float *) pv_ypu_mem_get_host_view(ypu, object->bias, false);
    q7_t *weight = (q7_t *) pv_ypu_mem_get_host_view(ypu, pv_cnn_get_weight(object), false);

    float *x_padded = calloc(
            (n * in_channels) + (2 * padding_num_elements),
            sizeof(float));
    if (!x_padded) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    for (int32_t i = padding_num_elements; i < n * in_channels + padding_num_elements; i++) {
        x_padded[i] = x[i - padding_num_elements];
    }

    float *buffer = calloc(
            n * out_channels,
            sizeof(float));
    if (!buffer) {
        free(x_padded);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    for (int32_t frame = 0; frame < n; frame++) {
        const q7_t *w = weight;
        const int32_t frame_offset_out = frame * out_channels;
        const int32_t frame_offset_in = frame * in_channels;

        for (int32_t oc = 0; oc < out_channels; oc++) {

            float sum = 0;
            for (int32_t ke = 0; ke < kernel_size; ke++) {
                const int32_t kernel_offset = ke * in_channels;

                for (int32_t ic = 0; ic < in_channels; ic++) {
                    sum += x_padded[frame_offset_in + kernel_offset + ic] * (((float) *(w++)) / 128.0f);
                }

            }

            buffer[frame_offset_out + oc] = sum;
        }
    }

    pv_status_t status = pv_affine_execute_float(
            n,
            out_channels,
            buffer,
            1.0f,
            0.0f,
            int8_inverse_scale,
            bias,
            y);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_affine_execute_float,
                pv_status_to_string(status));
        free(buffer);
        free(x_padded);
        return status;
    }
    
    free(buffer);
    free(x_padded);

#else

    pv_ypu_mem_t *x_q510 = pv_ypu_buffer_get(
            ypu,
            ((n * in_channels) + (2 * padding_num_elements)) * (int32_t) sizeof(q510_t),
            false);
    if (!x_q510) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("x_q510"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_mem_t *inverse_scale = pv_ypu_buffer_get(
            ypu,
            sizeof(float),
            false);
    if (!inverse_scale) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("inverse_scale"));
        pv_ypu_buffer_release(ypu, x_q510);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_op_memset_args_t args_memset = {
            .output = x_q510,
            .size_bytes = ((n * in_channels) + (2 * padding_num_elements)) * (int32_t) sizeof(q510_t),
            .output_offset = 0,
    };
    pv_status_t status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_MEMSET,
            &args_memset);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_MEMSET),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        pv_ypu_buffer_release(ypu, inverse_scale);
        pv_ypu_buffer_release(ypu, x_q510);
        return status;
    }

    pv_ypu_op_quantize_args_t args_quantize_q510 = {
            .output = x_q510,
            .scale = inverse_scale,
            .input = x_ypu,
            .m = 1,
            .n = n * in_channels,
            .output_offset = padding_num_elements * (int32_t) sizeof(q510_t),
            .scale_offset = 0,
            .input_offset = x_offset,
    };
    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_QUANTIZE_Q510,
            &args_quantize_q510);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_QUANTIZE_Q510),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        pv_ypu_buffer_release(ypu, inverse_scale);
        pv_ypu_buffer_release(ypu, x_q510);
        return status;
    }

    pv_ypu_op_conv1d_args_t args_conv1d = {
            .output = y_ypu,
            .lhs = pv_cnn_get_weight(object),
            .rhs = x_q510,
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
            &args_conv1d);
    pv_ypu_buffer_release(ypu, x_q510);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_CONV1D_Q1417_Q7_Q510),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        pv_ypu_buffer_release(ypu, inverse_scale);
        return status;
    }

    pv_ypu_op_elementwise_args_t args_convert_f32_i32 = {
            .output = y_ypu,
            .input = y_ypu,
            .length = n * out_channels,
            .output_offset = y_offset,
            .input_offset = y_offset,
    };
    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_CONVERT_F32_I32,
            &args_convert_f32_i32);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_CONVERT_F32_I32),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        pv_ypu_buffer_release(ypu, inverse_scale);
        return status;
    }

    status = pv_affine_execute_from_q1417_to_float(
            ypu,
            n,
            out_channels,
            y_ypu,
            inverse_scale,
            object->int8_inverse_scale,
            object->bias,
            y_ypu,
            y_offset,
            0,
            0,
            0,
            y_offset);
    pv_ypu_buffer_release(ypu, inverse_scale);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_affine_execute_from_q1417_to_float,
                pv_status_to_string(status));
        return status;
    }

#endif

    return PV_STATUS_SUCCESS;
}

struct pv_cnn_depthwise {
    const pv_cnn_depthwise_param_t *param;

    pv_ypu_mem_t *weight;
    pv_ypu_mem_t *bias;
    pv_ypu_mem_t *transposed_weight;
    pv_ypu_mem_t *int8_inverse_scale;
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
                &pv_error_msg_ypu_host_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(o, 0, sizeof(pv_cnn_depthwise_t));

    o->param = param;

    o->weight = pv_ypu_mem_from_config(ypu, param->weight);
    if (!o->weight) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o->weight"));
        pv_cnn_depthwise_delete(ypu, o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->bias = pv_ypu_mem_from_config(ypu, param->bias);
    if (!o->bias) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o->bias"));
        pv_cnn_depthwise_delete(ypu, o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->int8_inverse_scale = pv_ypu_mem_from_config(ypu, param->int8_inverse_scale);
    if (!o->int8_inverse_scale) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o->int8_inverse_scale"));
        pv_cnn_depthwise_delete(ypu, o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    int32_t kernel_size = pv_cnn_depthwise_kernel_size(o);
    int32_t num_channels = pv_cnn_depthwise_num_channels(o);

    if (kernel_size > 1) {
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
                .b = 1,
                .m = kernel_size,
                .n = num_channels,
                .k = sizeof(q7_t),
                .output_offset = 0,
                .input_offset = 0,
        };
        pv_status_t status = pv_ypu_operator_execute(
                ypu,
                PV_YPU_OPERATOR_TRANSPOSE,
                &args);
        if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_TRANSPOSE),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
            pv_cnn_depthwise_delete(ypu, o);
            return status;
        }
    } else {
        o->transposed_weight = NULL;
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_cnn_depthwise_delete)(
        pv_ypu_t *ypu,
        pv_cnn_depthwise_t *object) {
    PV_ASSERT(ypu);

    if (object) {
        if (object->transposed_weight) {
            pv_ypu_mem_free(ypu, object->transposed_weight);
        }

        pv_ypu_mem_free(ypu, object->weight);
        pv_ypu_mem_free(ypu, object->bias);
        pv_ypu_mem_free(ypu, object->int8_inverse_scale);

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
        pv_ypu_mem_t *x_ypu,
        pv_ypu_mem_t *y_ypu,
        int32_t x_offset,
        int32_t y_offset) {
    PV_ASSERT(ypu);
    PV_ASSERT(object);
    PV_ASSERT(n > 0);
    PV_ASSERT(x_ypu);
    PV_ASSERT(y_ypu);

    int32_t num_channels = pv_cnn_depthwise_num_channels(object);
    int32_t kernel_size = object->param->kernel_size;

    int32_t padding = object->param->padding;
    const int32_t padding_num_elements = padding * num_channels;

#ifdef __ORCA_FLOAT_MODE__

    float *x = (float *) (pv_ypu_mem_get_host_view(ypu, x_ypu, false) + x_offset);
    float *y = (float *) (pv_ypu_mem_get_host_view(ypu, y_ypu, false) + y_offset);
    float *int8_inverse_scale = (float *) pv_ypu_mem_get_host_view(ypu, object->int8_inverse_scale, false);
    float *bias = (float *) pv_ypu_mem_get_host_view(ypu, object->bias, false);
    q7_t *weight = (q7_t *) pv_ypu_mem_get_host_view(ypu, pv_cnn_depthwise_get_weight(object), false);

    float *x_padded = calloc(
            (n * num_channels) + (2 * padding_num_elements),
            sizeof(float));
    if (!x_padded) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    for (int32_t i = padding_num_elements; i < n * num_channels + padding_num_elements; i++) {
        x_padded[i] = x[i - padding_num_elements];
    }

    float *buffer = calloc(
            n * num_channels,
            sizeof(float));
    if (!buffer) {
        free(x_padded);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    for (int32_t frame = 0; frame < n; frame++) {
        const int32_t frame_offset = frame * num_channels;
        const q7_t *w = weight;

        for (int32_t ke = 0; ke < kernel_size; ke++) {
            const int32_t channel_offset = ke * num_channels;

            for (int32_t nc = 0; nc < num_channels; nc++) {
                buffer[frame_offset + nc] += x_padded[frame_offset + channel_offset + nc] * (((float) *(w++)) / 128.0f);
            }
        }
    }

    pv_status_t status = pv_affine_execute_float(
            n,
            num_channels,
            buffer,
            1.0f,
            0.0f,
            int8_inverse_scale,
            bias,
            y);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_affine_execute_float,
                pv_status_to_string(status));
        free(x_padded);
        free(buffer);
        return status;
    }

    free(x_padded);
    free(buffer);

#else

    const int32_t x_q510_size_bytes = ((n * num_channels) + (2 * padding_num_elements)) * (int32_t) sizeof(q510_t);
    pv_ypu_mem_t *x_q510 = pv_ypu_buffer_get(
            ypu,
            x_q510_size_bytes,
            false);
    if (!x_q510) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("x_q510"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_mem_t *inverse_scale = pv_ypu_buffer_get(
            ypu,
            sizeof(float),
            false);
    if (!inverse_scale) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("inverse_scale"));
        pv_ypu_buffer_release(ypu, x_q510);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_op_memset_args_t args_memset0 = {
            .output = x_q510,
            .size_bytes = x_q510_size_bytes,
            .output_offset = 0,
    };
    pv_status_t status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_MEMSET,
            &args_memset0);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_MEMSET),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        pv_ypu_buffer_release(ypu, inverse_scale);
        pv_ypu_buffer_release(ypu, x_q510);
        return status;
    }

    pv_ypu_op_memset_args_t args_memset1 = {
            .output = y_ypu,
            .size_bytes = n * num_channels * (int32_t) sizeof(float),
            .output_offset = y_offset,
    };
    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_MEMSET,
            &args_memset1);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_MEMSET),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        pv_ypu_buffer_release(ypu, inverse_scale);
        pv_ypu_buffer_release(ypu, x_q510);
        return status;
    }

    pv_ypu_op_quantize_args_t args_quantize_q510 = {
            .output = x_q510,
            .scale = inverse_scale,
            .input = x_ypu,
            .m = 1,
            .n = n * num_channels,
            .output_offset = padding_num_elements * (int32_t) sizeof(q510_t),
            .scale_offset = 0,
            .input_offset = x_offset,
    };
    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_QUANTIZE_Q510,
            &args_quantize_q510);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_QUANTIZE_Q510),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        pv_ypu_buffer_release(ypu, inverse_scale);
        pv_ypu_buffer_release(ypu, x_q510);
        return status;
    }

    pv_ypu_op_conv1d_depthwise_args_t args_conv1d = {
            .output = y_ypu,
            .lhs = pv_cnn_depthwise_get_weight(object),
            .rhs = x_q510,
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
            &args_conv1d);
    pv_ypu_buffer_release(ypu, x_q510);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_CONV1D_DEPTHWISE_Q1417_Q7_Q510),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        pv_ypu_buffer_release(ypu, inverse_scale);
        return status;
    }

    pv_ypu_op_elementwise_args_t args_convert_f32_i32 = {
            .output = y_ypu,
            .input = y_ypu,
            .length = n * num_channels,
            .output_offset = y_offset,
            .input_offset = y_offset,
    };
    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_CONVERT_F32_I32,
            &args_convert_f32_i32);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_CONVERT_F32_I32),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        pv_ypu_buffer_release(ypu, inverse_scale);
        return status;
    }

    status = pv_affine_execute_from_q1417_to_float(
            ypu,
            n,
            num_channels,
            y_ypu,
            inverse_scale,
            object->int8_inverse_scale,
            object->bias,
            y_ypu,
            y_offset,
            0,
            0,
            0,
            y_offset);
    pv_ypu_buffer_release(ypu, inverse_scale);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_affine_execute_from_q1417_to_float,
                pv_status_to_string(status));
        return status;
    }

#endif

    return PV_STATUS_SUCCESS;
}
