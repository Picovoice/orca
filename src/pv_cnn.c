#include <stdlib.h>
#include <string.h>

#include "core/pv_error_messages.h"
#include "math/pv_math.h"
#include "orca/pv_cnn.h"
#include "orca/pv_cnn_matmul.h"
#include "orca/pv_orca_util.h"
#include "orca/pv_profiler.h"
#include "util/pv_check_status.h"
#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

static pv_status_t q7_to_float(const int32_t n, const q7_t *x, float **x_float) {
    PV_ASSERT(n > 0);
    PV_ASSERT(x);
    PV_ASSERT(x_float);

    *x_float = NULL;

    float *x_float_internal = malloc(n * sizeof(float));
    if (!x_float_internal) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    for (int32_t i = 0; i < n; i++) {
        x_float_internal[i] = (float) x[i] / 128.f;
    }

    *x_float = x_float_internal;

    return PV_STATUS_SUCCESS;
}

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_cnn_param_serialize_buffer)(const pv_cnn_param_t *param, size_t *length, void **buffer) {
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

    void *b = malloc(*length);
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

    memcpy(b, param->bias, sizeof(q7_t) * num_bias_params);
    b += sizeof(q7_t) * num_bias_params;

    memcpy(b, param->weight, sizeof(q7_t) * num_weight_params);

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_cnn_param_serialize)(const pv_cnn_param_t *param, FILE *file) {
    PV_ASSERT(param);
    PV_ASSERT(file);

    size_t length = 0;
    void *buffer = NULL;
    const pv_status_t status = pv_cnn_param_serialize_buffer(param, &length, &buffer);
    PV_CHECK_STATUS(status);

    const size_t count = fwrite(buffer, 1, length, file);
    free(buffer);

    return (count == length) ? PV_STATUS_SUCCESS : PV_STATUS_IO_ERROR;
}

#endif

pv_status_t PV_MOCKABLE(pv_cnn_param_load)(FILE *f, pv_cnn_param_t **param) {
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_cnn_param_t *p = calloc(1, sizeof(pv_cnn_param_t));
    PV_CHECK_ALLOC(p);

    size_t count = pv_fread(&(p->input_channels), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_cnn_param_delete(p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->input_channels <= 0) {
        pv_cnn_param_delete(p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    count = pv_fread(&(p->output_channels), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_cnn_param_delete(p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->output_channels <= 0) {
        pv_cnn_param_delete(p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    count = pv_fread(&(p->kernel_size), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_cnn_param_delete(p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->kernel_size <= 0) {
        pv_cnn_param_delete(p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    count = pv_fread(&(p->stride), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_cnn_param_delete(p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->stride < 0) {
        pv_cnn_param_delete(p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    count = pv_fread(&(p->padding), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_cnn_param_delete(p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->padding < 0) {
        pv_cnn_param_delete(p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    count = pv_fread(&(p->dilation), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_cnn_param_delete(p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->dilation < 0) {
        pv_cnn_param_delete(p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    const int32_t num_bias_params = p->output_channels;
    const int32_t num_weight_params = p->input_channels * p->output_channels * p->kernel_size;

    p->bias = malloc(sizeof(q7_t) * num_bias_params);
    if (!(p->bias)) {
        pv_cnn_param_delete(p);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    count = pv_fread((q7_t *) (p->bias), sizeof(q7_t), num_bias_params, f);
    if (count != (size_t) num_bias_params) {
        pv_cnn_param_delete(p);
        return PV_STATUS_IO_ERROR;
    }

    p->weight = malloc(sizeof(q7_t) * num_weight_params);
    if (!(p->weight)) {
        pv_cnn_param_delete(p);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    count = pv_fread((q7_t *) (p->weight), sizeof(q7_t), num_weight_params, f);
    if (count != (size_t) num_weight_params) {
        pv_cnn_param_delete(p);
        return PV_STATUS_IO_ERROR;
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}


void PV_MOCKABLE(pv_cnn_param_delete)(pv_cnn_param_t *param) {
    if (param) {
        free((q7_t *) (param->weight));
        free((q7_t *) (param->bias));

        free(param);
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

    const int32_t num_bias_params = object->output_channels;
    const int32_t num_weight_params = object->input_channels * object->output_channels * object->kernel_size;

    for (int32_t i = 0; i < num_bias_params; i++) {
        if (object->bias[i] != other->bias[i]) {
            return false;
        }
    }

    for (int32_t i = 0; i < num_weight_params; i++) {
        if (object->weight[i] != other->weight[i]) {
            return false;
        }
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
        const pv_cnn_depthwise_param_t *param,
        size_t *length,
        void **buffer) {
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

    void *b = malloc(*length);
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

    memcpy(b, param->bias, sizeof(q7_t) * num_bias_params);
    b += sizeof(q7_t) * num_bias_params;

    memcpy(b, param->weight, sizeof(q7_t) * num_weight_params);

    return PV_STATUS_SUCCESS;
}


pv_status_t PV_MOCKABLE(pv_cnn_depthwise_param_serialize)(const pv_cnn_depthwise_param_t *param, FILE *file) {
    PV_ASSERT(param);
    PV_ASSERT(file);

    size_t length = 0;
    void *buffer = NULL;
    const pv_status_t status = pv_cnn_depthwise_param_serialize_buffer(param, &length, &buffer);
    PV_CHECK_STATUS(status);

    const size_t count = fwrite(buffer, 1, length, file);
    free(buffer);

    return (count == length) ? PV_STATUS_SUCCESS : PV_STATUS_IO_ERROR;
}

#endif

pv_status_t PV_MOCKABLE(pv_cnn_depthwise_param_load)(FILE *f, pv_cnn_depthwise_param_t **param) {
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_cnn_depthwise_param_t *p = calloc(1, sizeof(pv_cnn_depthwise_param_t));
    PV_CHECK_ALLOC(p);

    size_t count = pv_fread(&(p->num_channels), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_cnn_depthwise_param_delete(p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->num_channels <= 0) {
        pv_cnn_depthwise_param_delete(p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    count = pv_fread(&(p->kernel_size), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_cnn_depthwise_param_delete(p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->kernel_size <= 0) {
        pv_cnn_depthwise_param_delete(p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    count = pv_fread(&(p->stride), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_cnn_depthwise_param_delete(p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->stride < 0) {
        pv_cnn_depthwise_param_delete(p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    count = pv_fread(&(p->padding), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_cnn_depthwise_param_delete(p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->padding < 0) {
        pv_cnn_depthwise_param_delete(p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    count = pv_fread(&(p->dilation), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_cnn_depthwise_param_delete(p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->dilation < 0) {
        pv_cnn_depthwise_param_delete(p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    const int32_t num_bias_params = p->num_channels;
    const int32_t num_weight_params = p->num_channels * p->kernel_size;

    p->bias = malloc(sizeof(q7_t) * num_bias_params);
    if (!p->bias) {
        pv_cnn_depthwise_param_delete(p);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    count = pv_fread((q7_t *) (p->bias), sizeof(q7_t), num_bias_params, f);
    if (count != (size_t) num_bias_params) {
        pv_cnn_depthwise_param_delete(p);
        return PV_STATUS_IO_ERROR;
    }

    p->weight = malloc(sizeof(q7_t) * num_weight_params);
    if (!p->weight) {
        pv_cnn_depthwise_param_delete(p);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    count = pv_fread((q7_t *) (p->weight), sizeof(q7_t), num_weight_params, f);
    if (count != (size_t) num_weight_params) {
        pv_cnn_depthwise_param_delete(p);
        return PV_STATUS_IO_ERROR;
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}


void PV_MOCKABLE(pv_cnn_depthwise_param_delete)(pv_cnn_depthwise_param_t *param) {
    if (param) {
        free((q7_t *) param->weight);
        free((q7_t *) param->bias);

        free(param);
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

    const int32_t num_bias_params = object->num_channels;
    const int32_t num_weight_params = object->num_channels * object->kernel_size;

    for (int32_t i = 0; i < num_bias_params; i++) {
        if (object->bias[i] != other->bias[i]) {
            return false;
        }
    }

    for (int32_t i = 0; i < num_weight_params; i++) {
        if (object->weight[i] != other->weight[i]) {
            return false;
        }
    }

    return true;
}

struct pv_cnn {
    const pv_cnn_param_t *param;

    q7_t *transposed_weight;
    float *bias_float;
};

static pv_status_t pv_cnn_forward_float_to_float(pv_cnn_t *object, int32_t n, const float *x, float *y) {
    PV_ASSERT(object);
    PV_ASSERT(n);
    PV_ASSERT(x);
    PV_ASSERT(y);

    const int32_t in_channels = pv_cnn_input_channels(object);
    const int32_t out_channels = pv_cnn_output_channels(object);
    const int32_t kernel_size = pv_cnn_kernel_size(object);
    const int32_t padding = pv_cnn_padding(object);
    const int32_t padding_numel = padding * in_channels;

    const float *bias_float = object->bias_float;
    const q7_t *weight = pv_cnn_get_weight(object);

    PV_ORCA_PROFILER_START("cnn_preprocess");

    float inverse_scale = 0.f;
    q510_t *x_q510 = calloc((n * in_channels) + (2 * padding_numel), sizeof(q510_t));
    if (!x_q510) {
        return PV_STATUS_OUT_OF_MEMORY;
    }
    pv_orca_util_scale_and_quantize_activation(n * in_channels, padding_numel, x, x_q510, &inverse_scale);

    int32_t *buffer = calloc(n * out_channels, sizeof(int32_t));
    if (!buffer) {
        free(x_q510);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    PV_ORCA_PROFILER_STOP("cnn_preprocess");

    switch (kernel_size) {
        case 1:
            pv_cnn_kernel_1_q510(in_channels, out_channels, weight, n, x_q510, buffer);
            break;
        case 3:
            pv_cnn_kernel_3_q510(in_channels, out_channels, weight, n, x_q510, buffer);
            break;
        case 5:
            pv_cnn_kernel_5_q510(in_channels, out_channels, weight, n, x_q510, buffer);
            break;
        case 7:
            pv_cnn_kernel_7_q510(in_channels, out_channels, weight, n, x_q510, buffer);
            break;
        default:
            PV_ERROR_REPORT(
                    &pv_error_msg_invalid_argument_internal,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("kernel_size"));
            free(buffer);
            free(x_q510);
            return PV_STATUS_RUNTIME_ERROR;
    }

    PV_ORCA_PROFILER_START("cnn_bias_add");
    for (int32_t frame = 0; frame < n; frame++) {
        const int32_t frame_offset = frame * out_channels;
        for (int32_t oc = 0; oc < out_channels; oc++) {
            y[frame_offset + oc] = ((float) buffer[frame_offset + oc] * inverse_scale) + bias_float[oc];
        }
    }
    PV_ORCA_PROFILER_STOP("cnn_bias_add");

    free(buffer);
    free(x_q510);
    return PV_STATUS_SUCCESS;
}

static pv_status_t pv_cnn_forward_float_to_q510(pv_cnn_t *object, int32_t n, const float *x, q510_t *y) {
    PV_ASSERT(object);
    PV_ASSERT(n);
    PV_ASSERT(x);
    PV_ASSERT(y);

    const int32_t in_channels = pv_cnn_input_channels(object);
    const int32_t out_channels = pv_cnn_output_channels(object);
    const int32_t kernel_size = pv_cnn_kernel_size(object);
    const int32_t padding = pv_cnn_padding(object);
    const int32_t padding_numel = padding * in_channels;

    const q7_t *bias = object->param->bias;
    const q7_t *weight = pv_cnn_get_weight(object);

    PV_ORCA_PROFILER_START("cnn_preprocess");

    q510_t *x_q510 = calloc((n * in_channels) + (2 * padding_numel), sizeof(q510_t));
    if (!x_q510) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    for (int32_t i = padding_numel; i < (n * in_channels) + padding_numel; i++) {
        x_q510[i] = pv_float_to_q510(x[i - padding_numel]);
    }

    int32_t *buffer = calloc(n * out_channels, sizeof(int32_t));
    if (!buffer) {
        free(x_q510);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    PV_ORCA_PROFILER_STOP("cnn_preprocess");

    switch (kernel_size) {
        case 1:
            pv_cnn_kernel_1_q510(in_channels, out_channels, weight, n, x_q510, buffer);
            break;
        case 3:
            pv_cnn_kernel_3_q510(in_channels, out_channels, weight, n, x_q510, buffer);
            break;
        case 5:
            pv_cnn_kernel_5_q510(in_channels, out_channels, weight, n, x_q510, buffer);
            break;
        case 7:
            pv_cnn_kernel_7_q510(in_channels, out_channels, weight, n, x_q510, buffer);
            break;
        default:
            PV_ERROR_REPORT(
                    &pv_error_msg_invalid_argument_internal,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("kernel_size"));
            free(buffer);
            free(x_q510);
            return PV_STATUS_RUNTIME_ERROR;
    }

    PV_ORCA_PROFILER_START("cnn_bias_add");
    for (int32_t frame = 0; frame < n; frame++) {
        const int32_t frame_offset = frame * out_channels;
        for (int32_t oc = 0; oc < out_channels; oc++) {
            buffer[frame_offset + oc] += ((int32_t) bias[oc]) << 10;
            y[frame_offset + oc] = pv_int32_to_int16(((buffer[frame_offset + oc] + (1 << 6)) >> 7));
        }
    }
    PV_ORCA_PROFILER_STOP("cnn_bias_add");

    free(buffer);
    free(x_q510);
    return PV_STATUS_SUCCESS;
}

static pv_status_t pv_cnn_forward_q510_to_float(pv_cnn_t *object, int32_t n, const q510_t *x, float *y) {
    PV_ASSERT(object);
    PV_ASSERT(n);
    PV_ASSERT(x);
    PV_ASSERT(y);

    const int32_t in_channels = pv_cnn_input_channels(object);
    const int32_t out_channels = pv_cnn_output_channels(object);
    const int32_t kernel_size = pv_cnn_kernel_size(object);

    const float *bias_float = object->bias_float;
    const q7_t *weight = pv_cnn_get_weight(object);

    PV_ORCA_PROFILER_START("cnn_preprocess");

    int32_t *buffer = calloc(n * out_channels, sizeof(int32_t));
    if (!buffer) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    PV_ORCA_PROFILER_STOP("cnn_preprocess");

    switch (kernel_size) {
        case 1:
            pv_cnn_kernel_1_q510(in_channels, out_channels, weight, n, x, buffer);
            break;
        case 3:
            pv_cnn_kernel_3_q510(in_channels, out_channels, weight, n, x, buffer);
            break;
        case 5:
            pv_cnn_kernel_5_q510(in_channels, out_channels, weight, n, x, buffer);
            break;
        case 7:
            pv_cnn_kernel_7_q510(in_channels, out_channels, weight, n, x, buffer);
            break;
        default:
            PV_ERROR_REPORT(
                    &pv_error_msg_invalid_argument_internal,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("kernel_size"));
            free(buffer);
            return PV_STATUS_RUNTIME_ERROR;
    }

    PV_ORCA_PROFILER_START("cnn_bias_add");
    for (int32_t frame = 0; frame < n; frame++) {
        const int32_t frame_offset = frame * out_channels;
        for (int32_t oc = 0; oc < out_channels; oc++) {
            y[frame_offset + oc] = ((float) buffer[frame_offset + oc] / 131072.f) + bias_float[oc];
        }
    }
    PV_ORCA_PROFILER_STOP("cnn_bias_add");

    free(buffer);
    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_cnn_transpose_weight)(pv_cnn_t *object, q7_t **weight) {
    PV_ASSERT(object);
    PV_ASSERT(weight);

    int32_t in_channels = pv_cnn_input_channels(object);
    int32_t out_channels = pv_cnn_output_channels(object);
    int32_t kernel_size = pv_cnn_kernel_size(object);
    const q7_t *orig_weight = object->param->weight;

    q7_t *transposed = calloc((out_channels * in_channels * kernel_size), sizeof(q7_t));
    if (!transposed) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    if (kernel_size > 1) {
        for (int32_t oc = 0; oc < out_channels; oc++) {
            for (int32_t ke = 0; ke < kernel_size; ke++) {
                for (int32_t ic = 0; ic < in_channels; ic++) {
                    transposed[(oc * kernel_size * in_channels) + (ke * in_channels) + ic] =
                            orig_weight[(oc * in_channels * kernel_size) + (ic * kernel_size) + ke];
                }
            }
        }
    } else {
        for (int32_t oc = 0; oc < out_channels; oc++) {
            for (int32_t ic = 0; ic < in_channels; ic++) {
                transposed[(ic * out_channels) + oc] = orig_weight[(oc * in_channels) + ic];
            }
        }
    }

    *weight = transposed;

    return PV_STATUS_SUCCESS;
}

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

const q7_t *PV_MOCKABLE(pv_cnn_get_weight)(const pv_cnn_t *object) {
    PV_ASSERT(object);

    if (object->transposed_weight) {
        return (const q7_t *) object->transposed_weight;
    } else {
        return object->param->weight;
    }
}

pv_status_t PV_MOCKABLE(pv_cnn_init)(
        const pv_cnn_param_t *param,
        pv_cnn_t **object) {
    PV_ASSERT(param);
    PV_ASSERT(object);

    *object = NULL;

    pv_cnn_t *o = calloc(1, sizeof(pv_cnn_t));
    if (!o) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->param = param;

    if ((param->kernel_size > 1) && ((o->param->output_channels % 4) != 0)) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_min,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE(
                        "output_channels",
                        o->param->output_channels,
                        4));
        pv_cnn_delete(o);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    if (param->kernel_size > 1) {
        pv_status_t status = pv_cnn_transpose_weight(o, &(o->transposed_weight));
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_cnn_transpose_weight,
                    pv_status_to_string(status));
            pv_cnn_delete(o);
            return status;
        }
    } else {
        o->transposed_weight = NULL;
    }

    pv_status_t status = q7_to_float(param->output_channels, param->bias, &(o->bias_float));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                q7_to_float,
                pv_status_to_string(status));
        pv_cnn_delete(o);
        return status;
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_cnn_delete)(pv_cnn_t *object) {
    if (object) {
        if (object->transposed_weight) {
            free(object->transposed_weight);
        }

        free(object->bias_float);

        free(object);
    }
}

pv_status_t PV_MOCKABLE(pv_cnn_forward)(pv_cnn_t *object, int32_t n, const float *x, float *y) {
    PV_ASSERT(object);
    PV_ASSERT(n);
    PV_ASSERT(x);
    PV_ASSERT(y);

    return pv_cnn_forward_float_to_float(object, n, x, y);
}

pv_status_t PV_MOCKABLE(pv_cnn_forward_to_q510)(pv_cnn_t *object, int32_t n, const float *x, q510_t *y) {
    PV_ASSERT(object);
    PV_ASSERT(n);
    PV_ASSERT(x);
    PV_ASSERT(y);

    return pv_cnn_forward_float_to_q510(object, n, x, y);
}

pv_status_t PV_MOCKABLE(pv_cnn_forward_from_q510)(pv_cnn_t *object, int32_t n, const q510_t *x, float *y) {
    PV_ASSERT(object);
    PV_ASSERT(n);
    PV_ASSERT(x);
    PV_ASSERT(y);

    return pv_cnn_forward_q510_to_float(object, n, x, y);
}

struct pv_cnn_depthwise {
    const pv_cnn_depthwise_param_t *param;

    q7_t *transposed_weight;
    float *bias_float;
};

static pv_status_t pv_cnn_depthwise_transpose_weight(pv_cnn_depthwise_t *object, q7_t **weight) {
    PV_ASSERT(object);
    PV_ASSERT(weight);

    *weight = NULL;

    int32_t kernel_size = pv_cnn_depthwise_kernel_size(object);
    if (kernel_size == 1) {  // weight_T = weight
        return PV_STATUS_SUCCESS;
    }

    int32_t num_channels = pv_cnn_depthwise_num_channels(object);
    const q7_t *orig_weight = object->param->weight;

    q7_t *transposed = calloc((num_channels * kernel_size), sizeof(q7_t));
    if (!transposed) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    for (int32_t ke = 0; ke < kernel_size; ke++) {
        for (int32_t nc = 0; nc < num_channels; nc++) {
            transposed[(ke * num_channels) + nc] = orig_weight[(nc * kernel_size) + ke];
        }
    }

    *weight = transposed;

    return PV_STATUS_SUCCESS;
}

static pv_status_t pv_cnn_depthwise_forward_q510(pv_cnn_depthwise_t *object, int32_t n, const float *x, float *y) {
    PV_ASSERT(object);
    PV_ASSERT(n);
    PV_ASSERT(x);
    PV_ASSERT(y);
    PV_ORCA_PROFILER_START("cnn_depthwise");

    int32_t num_channels = pv_cnn_depthwise_num_channels(object);
    int32_t kernel_size = object->param->kernel_size;

    const q7_t *weight = pv_cnn_depthwise_get_weight(object);
    const float *bias_float = object->bias_float;

    int32_t padding = object->param->padding;
    const int32_t padding_numel = padding * num_channels;

    float inverse_scale = 0.f;
    q510_t *x_q510 = calloc((n * num_channels) + (2 * padding_numel), sizeof(q510_t));
    if (!x_q510) {
        return PV_STATUS_OUT_OF_MEMORY;
    }
    pv_orca_util_scale_and_quantize_activation(n * num_channels, padding_numel, x, x_q510, &inverse_scale);

    int32_t *buffer = calloc(n * num_channels, sizeof(int32_t));
    if (!buffer) {
        free(x_q510);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    for (int32_t frame = 0; frame < n; frame++) {
        const int32_t frame_offset = frame * num_channels;
        const q7_t *w = weight;

        for (int32_t ke = 0; ke < kernel_size; ke++) {
            const int32_t channel_offset = ke * num_channels;

            for (int32_t nc = 0; nc < num_channels; nc++) {
                buffer[frame_offset + nc] +=
                        (int32_t) x_q510[frame_offset + channel_offset + nc] * (int32_t) *w++;
            }
        }

        for (int32_t nc = 0; nc < num_channels; nc++) {
            y[frame_offset + nc] = ((float) buffer[frame_offset + nc] * inverse_scale) + bias_float[nc];
        }
    }

    free(x_q510);
    free(buffer);

    PV_ORCA_PROFILER_STOP("cnn_depthwise");
    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_cnn_depthwise_init)(
        const pv_cnn_depthwise_param_t *param,
        pv_cnn_depthwise_t **object) {
    PV_ASSERT(param);
    PV_ASSERT(object);

    *object = NULL;

    pv_cnn_depthwise_t *o = calloc(1, sizeof(pv_cnn_depthwise_t));
    if (!o) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->param = param;

    pv_status_t status = pv_cnn_depthwise_transpose_weight(o, &(o->transposed_weight));  // [kernel_size, num_channels]
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_depthwise_transpose_weight,
                pv_status_to_string(status));
        pv_cnn_depthwise_delete(o);
        return status;
    }

    status = q7_to_float(param->num_channels, param->bias, &(o->bias_float));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                q7_to_float,
                pv_status_to_string(status));
        pv_cnn_depthwise_delete(o);
        return status;
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_cnn_depthwise_delete)(pv_cnn_depthwise_t *object) {
    if (object) {
        if (object->transposed_weight) {
            free(object->transposed_weight);
        }

        free(object->bias_float);

        free(object);
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

const q7_t *PV_MOCKABLE(pv_cnn_depthwise_get_weight)(const pv_cnn_depthwise_t *object) {
    PV_ASSERT(object);

    if (object->transposed_weight) {
        return (const q7_t *) object->transposed_weight;
    } else {
        return object->param->weight;
    }
}

pv_status_t PV_MOCKABLE(pv_cnn_depthwise_forward)(pv_cnn_depthwise_t *object, int32_t n, const float *x, float *y) {
    PV_ASSERT(object);
    PV_ASSERT(n);
    PV_ASSERT(x);
    PV_ASSERT(y);

    return pv_cnn_depthwise_forward_q510(object, n, x, y);
}
