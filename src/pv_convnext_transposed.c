#include <stdlib.h>
#include <string.h>

#include "core/pv_type.h"
#include "nn/pv_activation.h"
#include "orca/pv_buffer.h"
#include "orca/pv_cnn.h"
#include "orca/pv_cnn_transposed.h"
#include "orca/pv_convnext_transposed.h"
#include "orca/pv_profiler.h"
#include "util/pv_check_status.h"
#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_convnext_transposed_param_serialize)(const pv_convnext_transposed_param_t *param, FILE *file) {
    PV_ASSERT(param);
    PV_ASSERT(file);

    pv_status_t status = pv_cnn_transposed_depthwise_param_serialize(param->conv_transposed_depthwise_param, file);
    PV_CHECK_STATUS(status);

    status = pv_layer_norm_param_serialize(param->layer_norm_param, file);
    PV_CHECK_STATUS(status);

    status = pv_cnn_param_serialize(param->conv_1_param, file);
    PV_CHECK_STATUS(status);

    status = pv_cnn_param_serialize(param->conv_2_param, file);
    PV_CHECK_STATUS(status);

    return PV_STATUS_SUCCESS;
}

#endif

pv_status_t PV_MOCKABLE(pv_convnext_transposed_param_load)(FILE *f, pv_convnext_transposed_param_t **param) {
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_convnext_transposed_param_t *p = calloc(1, sizeof(pv_convnext_transposed_param_t));
    PV_CHECK_ALLOC(p);

    pv_status_t status =
            pv_cnn_depthwise_param_load(f, (pv_cnn_depthwise_param_t **) &(p->conv_transposed_depthwise_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_convnext_transposed_param_delete(p);
        return status;
    }

    status = pv_layer_norm_param_load(f, (pv_layer_norm_param_t **) &(p->layer_norm_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_convnext_transposed_param_delete(p);
        return status;
    }

    status = pv_cnn_param_load(f, (pv_cnn_param_t **) &(p->conv_1_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_convnext_transposed_param_delete(p);
        return status;
    }

    status = pv_cnn_param_load(f, (pv_cnn_param_t **) &(p->conv_2_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_convnext_transposed_param_delete(p);
        return status;
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_convnext_transposed_param_delete)(pv_convnext_transposed_param_t *param) {
    if (param) {
        pv_cnn_param_delete((pv_cnn_param_t *) (param->conv_2_param));

        pv_cnn_param_delete((pv_cnn_param_t *) (param->conv_1_param));

        pv_layer_norm_param_delete((pv_layer_norm_param_t *) (param->layer_norm_param));

        pv_cnn_transposed_depthwise_param_delete(
                (pv_cnn_transposed_depthwise_param_t *) (param->conv_transposed_depthwise_param));

        free(param);
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

    pv_buffer_t *buffer_1;
    pv_buffer_t *buffer_2;
};

pv_status_t PV_MOCKABLE(pv_convnext_transposed_init)(
        const pv_convnext_transposed_param_t *param,
        pv_convnext_transposed_t **object) {
    PV_ASSERT(param);
    PV_ASSERT(object);

    *object = NULL;

    pv_convnext_transposed_t *o = calloc(1, sizeof(pv_convnext_transposed_t));
    if (!o) {
        pv_convnext_transposed_delete(o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->param = param;

    pv_status_t status = pv_cnn_transposed_depthwise_init(
            param->conv_transposed_depthwise_param, &(o->conv_transposed_depthwise));
    if (status != PV_STATUS_SUCCESS) {
        pv_convnext_transposed_delete(o);
        return status;
    }

    status = pv_layer_norm_init(param->layer_norm_param, &(o->layer_norm));
    if (status != PV_STATUS_SUCCESS) {
        pv_convnext_transposed_delete(o);
        return status;
    }

    status = pv_cnn_init(param->conv_1_param, &(o->conv_1));
    if (status != PV_STATUS_SUCCESS) {
        pv_convnext_transposed_delete(o);
        return status;
    }

    status = pv_cnn_init(param->conv_2_param, &(o->conv_2));
    if (status != PV_STATUS_SUCCESS) {
        pv_convnext_transposed_delete(o);
        return status;
    }

    int32_t num_channels_depthwise = o->param->conv_transposed_depthwise_param->num_channels;
    status = pv_buffer_init(num_channels_depthwise, &(o->buffer_1));
    if (status != PV_STATUS_SUCCESS) {
        pv_convnext_transposed_delete(o);
        return status;
    }

    int32_t num_intermediate_channels = o->param->conv_1_param->output_channels;
    status = pv_buffer_init(num_intermediate_channels, &(o->buffer_2));
    if (status != PV_STATUS_SUCCESS) {
        pv_convnext_transposed_delete(o);
        return status;
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_convnext_transposed_delete)(pv_convnext_transposed_t *object) {
    if (object) {
        pv_buffer_delete(object->buffer_2);
        pv_buffer_delete(object->buffer_1);

        pv_cnn_delete(object->conv_2);
        pv_cnn_delete(object->conv_1);

        pv_layer_norm_delete(object->layer_norm);

        pv_cnn_transposed_depthwise_delete(object->conv_transposed_depthwise);

        free(object);
    }
}

int32_t PV_MOCKABLE(pv_convnext_transposed_output_channels)(const pv_convnext_transposed_t *object) {
    PV_ASSERT(object);

    return pv_cnn_output_channels(object->conv_2);
}

pv_status_t PV_MOCKABLE(pv_convnext_transposed_forward)(
        pv_convnext_transposed_t *object,
        int32_t n,
        const float *x,
        float *y) {
    PV_ASSERT(object);
    PV_ASSERT(n);
    PV_ASSERT(x);
    PV_ASSERT(y);
    PV_ORCA_PROFILER_START("convnext_transposed");

    int32_t n_upsampled = pv_cnn_transposed_depthwise_num_output_frames(object->conv_transposed_depthwise, n);

    float *buffer_1 = pv_buffer_get(object->buffer_1, n_upsampled, true);
    if (!buffer_1) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_status_t status = pv_cnn_transposed_depthwise_forward(object->conv_transposed_depthwise, n, x, buffer_1);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_layer_norm_forward(object->layer_norm, n_upsampled, buffer_1, buffer_1);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    float *buffer_2 = pv_buffer_get(object->buffer_2, n_upsampled, false);
    if (!buffer_2) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    PV_ORCA_PROFILER_START("convnext_transposed_kernel_1");
    status = pv_cnn_forward(object->conv_1, n_upsampled, buffer_1, buffer_2);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    PV_ORCA_PROFILER_STOP("convnext_transposed_kernel_1");

    PV_ORCA_PROFILER_START("convnext_transposed_gelu");

#ifndef __ORCA_FLOAT_MODE__

    // NOTE: the following gelu implementation is much faster (factor ~5)
    // but might lead to slight differences in the output
    pv_activation_gelu_float_approx(n_upsampled * pv_cnn_output_channels(object->conv_1), buffer_2);

#else

    pv_activation_gelu_float(n_upsampled * pv_cnn_output_channels(object->conv_1), buffer_2);

#endif

    PV_ORCA_PROFILER_STOP("convnext_transposed_gelu");

    PV_ORCA_PROFILER_START("convnext_transposed_kernel_1");
    status = pv_cnn_forward(object->conv_2, n_upsampled, buffer_2, y);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }
    PV_ORCA_PROFILER_STOP("convnext_transposed_kernel_1");

    pv_buffer_free(object->buffer_1);
    pv_buffer_free(object->buffer_2);

    PV_ORCA_PROFILER_STOP("convnext_transposed");
    return PV_STATUS_SUCCESS;
}

int32_t PV_MOCKABLE(pv_convnext_transposed_num_output_frames)(const pv_convnext_transposed_t *object, int32_t n) {
    PV_ASSERT(object);
    PV_ASSERT(n > 0);

    return pv_cnn_transposed_depthwise_num_output_frames(object->conv_transposed_depthwise, n);
}
