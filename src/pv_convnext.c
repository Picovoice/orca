#include <stdlib.h>
#include <string.h>

#include "io/pv_dump.h"
#include "core/pv_type.h"
#include "nn/pv_activation.h"
#include "orca/pv_buffer.h"
#include "orca/pv_buffer_q510.h"
#include "orca/pv_cnn.h"
#include "orca/pv_convnext.h"
#include "orca/pv_profiler.h"
#include "util/pv_check_status.h"
#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_convnext_param_serialize)(const pv_convnext_param_t *param, FILE *file) {
    PV_ASSERT(param);
    PV_ASSERT(file);

    pv_status_t status = pv_cnn_depthwise_param_serialize(param->conv_depthwise_param, file);
    PV_CHECK_STATUS(status);

    status = pv_layer_norm_param_serialize(param->layer_norm_param, file);
    PV_CHECK_STATUS(status);

    status = pv_cnn_param_serialize(param->conv_1_param, file);
    PV_CHECK_STATUS(status);

    status = pv_cnn_param_serialize(param->conv_2_param, file);
    PV_CHECK_STATUS(status);

    const size_t length = sizeof(float) * param->conv_2_param->output_channels;
    const size_t count = fwrite(param->scale_param, 1, length, file);
    if (count != length) {
        return PV_STATUS_IO_ERROR;
    }

    return PV_STATUS_SUCCESS;
}

#endif

pv_status_t PV_MOCKABLE(pv_convnext_param_load)(FILE *f, pv_convnext_param_t **param) {
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_convnext_param_t *p = calloc(1, sizeof(pv_convnext_param_t));
    PV_CHECK_ALLOC(p);

    pv_status_t status = pv_cnn_depthwise_param_load(f, (pv_cnn_depthwise_param_t **) &(p->conv_depthwise_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_convnext_param_delete(p);
        return status;
    }

    status = pv_layer_norm_param_load(f, (pv_layer_norm_param_t **) &(p->layer_norm_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_convnext_param_delete(p);
        return status;
    }

    status = pv_cnn_param_load(f, (pv_cnn_param_t **) &(p->conv_1_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_convnext_param_delete(p);
        return status;
    }

    status = pv_cnn_param_load(f, (pv_cnn_param_t **) &(p->conv_2_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_convnext_param_delete(p);
        return status;
    }

    const size_t length = p->conv_2_param->output_channels;
    p->scale_param = malloc(sizeof(float) * length);
    if (!p->scale_param) {
        pv_convnext_param_delete(p);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    const size_t count = pv_fread((q7_t *) (p->scale_param), sizeof(float), length, f);
    if (count != length) {
        pv_convnext_param_delete(p);
        return PV_STATUS_IO_ERROR;
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}


void PV_MOCKABLE(pv_convnext_param_delete)(pv_convnext_param_t *param) {
    if (param) {
        free((float *) param->scale_param);

        pv_cnn_param_delete((pv_cnn_param_t *) (param->conv_2_param));

        pv_cnn_param_delete((pv_cnn_param_t *) (param->conv_1_param));

        pv_layer_norm_param_delete((pv_layer_norm_param_t *) (param->layer_norm_param));

        pv_cnn_depthwise_param_delete((pv_cnn_depthwise_param_t *) (param->conv_depthwise_param));

        free(param);
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

    return true;
}

struct pv_convnext {
    const pv_convnext_param_t *param;

    pv_cnn_depthwise_t *conv_depthwise;
    pv_layer_norm_t *layer_norm;
    pv_cnn_t *conv_1;
    pv_cnn_t *conv_2;

    pv_buffer_t *buffer_vocos_backbone_convnext_1;
    pv_buffer_q510_t *buffer_vocos_backbone_convnext_2;
};

pv_status_t PV_MOCKABLE(pv_convnext_init)(
        const pv_convnext_param_t *param,
        pv_buffer_t *buffer_vocos_backbone_convnext_1,
        pv_buffer_q510_t *buffer_vocos_backbone_convnext_2,
        pv_convnext_t **object) {
    PV_ASSERT(param);
    PV_ASSERT(buffer_vocos_backbone_convnext_1);
    PV_ASSERT(buffer_vocos_backbone_convnext_2);
    PV_ASSERT(object);

    *object = NULL;

    pv_convnext_t *o = calloc(1, sizeof(pv_convnext_t));
    if (!o) {
        pv_convnext_delete(o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->param = param;
    o->buffer_vocos_backbone_convnext_1 = buffer_vocos_backbone_convnext_1;
    o->buffer_vocos_backbone_convnext_2 = buffer_vocos_backbone_convnext_2;

    pv_status_t status = pv_cnn_depthwise_init(param->conv_depthwise_param, &(o->conv_depthwise));
    if (status != PV_STATUS_SUCCESS) {
        pv_convnext_delete(o);
        return status;
    }

    status = pv_layer_norm_init(param->layer_norm_param, &(o->layer_norm));
    if (status != PV_STATUS_SUCCESS) {
        pv_convnext_delete(o);
        return status;
    }

    status = pv_cnn_init(param->conv_1_param, &(o->conv_1));
    if (status != PV_STATUS_SUCCESS) {
        pv_convnext_delete(o);
        return status;
    }

    status = pv_cnn_init(param->conv_2_param, &(o->conv_2));
    if (status != PV_STATUS_SUCCESS) {
        pv_convnext_delete(o);
        return status;
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_convnext_delete)(pv_convnext_t *object) {
    if (object) {
        pv_cnn_delete(object->conv_2);
        pv_cnn_delete(object->conv_1);

        pv_layer_norm_delete(object->layer_norm);

        pv_cnn_depthwise_delete(object->conv_depthwise);

        free(object);
    }
}

pv_status_t PV_MOCKABLE(pv_convnext_forward)(
        pv_convnext_t *object,
        int32_t n,
        const float *x,
        float *y) {
    PV_ASSERT(object);
    PV_ASSERT(n);
    PV_ASSERT(x);
    PV_ASSERT(y);
    PV_ORCA_PROFILER_START("convnext");

    const float *residual = x;

    float *buffer_1 = pv_buffer_get(object->buffer_vocos_backbone_convnext_1, n, false);
    if (!buffer_1) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_status_t status = pv_cnn_depthwise_forward(object->conv_depthwise, n, x, buffer_1);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_layer_norm_forward(object->layer_norm, n, buffer_1, buffer_1);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    q510_t *buffer_2 = pv_buffer_q510_get(object->buffer_vocos_backbone_convnext_2, n, false);
    if (!buffer_2) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    PV_ORCA_PROFILER_START("convnext_kernel_1");
    status = pv_cnn_forward_to_q510(object->conv_1, n, buffer_1, buffer_2);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }
    PV_ORCA_PROFILER_STOP("convnext_kernel_1");

    PV_ORCA_PROFILER_START("convnext_gelu");

#ifndef __ORCA_FLOAT_MODE__

    // NOTE: the following gelu implementation is much faster (factor ~5)
    // but might lead to slight differences in the output because of overflowing q510_t values
    // Need to wait for trained model preventing this
    pv_activation_gelu_q510_approx(n * pv_cnn_output_channels(object->conv_1), buffer_2);

#else

    pv_activation_gelu_float(n * pv_cnn_output_channels(object->conv_1), buffer_hidden);

#endif

    PV_ORCA_PROFILER_STOP("convnext_gelu");

    PV_ORCA_PROFILER_START("convnext_kernel_1");
    status = pv_cnn_forward_from_q510(object->conv_2, n, buffer_2, buffer_1);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }
    PV_ORCA_PROFILER_STOP("convnext_kernel_1");

    const float *scale = object->param->scale_param;
    const int32_t num_channels = pv_cnn_output_channels(object->conv_2);
    for (int32_t t = 0; t < n; t++) {
        const int32_t index_offset = t * num_channels;

        for (int32_t c = 0; c < num_channels; c++) {
            const int32_t index = index_offset + c;
            y[index] = residual[index] + (buffer_1[index] * scale[c]);
        }
    }

    PV_ORCA_PROFILER_STOP("convnext");
    return PV_STATUS_SUCCESS;
}
