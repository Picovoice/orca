#include <stdlib.h>
#include <string.h>

#include "core/pv_type.h"
#include "model/pv_activation.h"
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

pv_status_t PV_MOCKABLE(pv_convnext_transposed_param_serialize)(
        pv_ypu_t *ypu,
        const pv_convnext_transposed_param_t *param,
        FILE *file) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(file);

    pv_status_t status = pv_cnn_transposed_depthwise_param_serialize(ypu, param->conv_transposed_depthwise_param, file);
    PV_CHECK_STATUS(status);

    status = pv_layer_norm_param_serialize(ypu, param->layer_norm_param, file);
    PV_CHECK_STATUS(status);

    status = pv_cnn_param_serialize(ypu, param->conv_1_param, file);
    PV_CHECK_STATUS(status);

    status = pv_cnn_param_serialize(ypu, param->conv_2_param, file);
    PV_CHECK_STATUS(status);

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

    pv_convnext_transposed_param_t *p = pv_ypu_host_alloc(ypu, sizeof(pv_convnext_transposed_param_t));
    PV_CHECK_ALLOC(p);

    memset(p, 0, sizeof(pv_convnext_transposed_param_t));

    pv_status_t status = pv_cnn_depthwise_param_load(
            ypu,
            f,
            (pv_cnn_depthwise_param_t **) &(p->conv_transposed_depthwise_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_convnext_transposed_param_delete(ypu, p);
        return status;
    }

    status = pv_layer_norm_param_load(
            ypu,
            f,
            (pv_layer_norm_param_t **) &(p->layer_norm_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_convnext_transposed_param_delete(ypu, p);
        return status;
    }

    status = pv_cnn_param_load(ypu, f, (pv_cnn_param_t **) &(p->conv_1_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_convnext_transposed_param_delete(ypu, p);
        return status;
    }

    status = pv_cnn_param_load(ypu, f, (pv_cnn_param_t **) &(p->conv_2_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_convnext_transposed_param_delete(ypu, p);
        return status;
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_convnext_transposed_param_delete)(pv_ypu_t *ypu, pv_convnext_transposed_param_t *param) {
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
                object->conv_transposed_depthwise_param,
                other->conv_transposed_depthwise_param)) {
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

    pv_convnext_transposed_t *o = pv_ypu_host_alloc(ypu, sizeof(pv_convnext_transposed_t));
    if (!o) {
        pv_convnext_transposed_delete(ypu, o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(o, 0, sizeof(pv_convnext_transposed_t));

    o->param = param;

    pv_status_t status = pv_cnn_transposed_depthwise_init(
            ypu, param->conv_transposed_depthwise_param, &(o->conv_transposed_depthwise));
    if (status != PV_STATUS_SUCCESS) {
        pv_convnext_transposed_delete(ypu, o);
        return status;
    }

    status = pv_layer_norm_init(ypu, param->layer_norm_param, &(o->layer_norm));
    if (status != PV_STATUS_SUCCESS) {
        pv_convnext_transposed_delete(ypu, o);
        return status;
    }

    status = pv_cnn_init(ypu, param->conv_1_param, &(o->conv_1));
    if (status != PV_STATUS_SUCCESS) {
        pv_convnext_transposed_delete(ypu, o);
        return status;
    }

    status = pv_cnn_init(ypu, param->conv_2_param, &(o->conv_2));
    if (status != PV_STATUS_SUCCESS) {
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

int32_t PV_MOCKABLE(pv_convnext_transposed_output_channels)(const pv_convnext_transposed_t *object) {
    PV_ASSERT(object);

    return pv_cnn_output_channels(object->conv_2);
}

pv_status_t PV_MOCKABLE(pv_convnext_transposed_forward)(
        pv_ypu_t *ypu,
        pv_convnext_transposed_t *object,
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
    PV_ORCA_PROFILER_START("convnext_transposed");

    int32_t num_channels_depthwise = object->param->conv_transposed_depthwise_param->num_channels;
    int32_t num_intermediate_channels = object->param->conv_1_param->output_channels;
    int32_t n_upsampled = pv_cnn_transposed_depthwise_num_output_frames(object->conv_transposed_depthwise, n);

    pv_ypu_mem_t *buffer_1 = pv_ypu_buffer_get(
            ypu,
            num_channels_depthwise * n_upsampled * (int32_t) sizeof(float),
            false);
    if (!buffer_1) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_status_t status = pv_cnn_transposed_depthwise_forward(
            ypu,
            object->conv_transposed_depthwise,
            n,
            x_ypu_mem,
            buffer_1,
            x_offset,
            0);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_layer_norm_forward(
            ypu,
            object->layer_norm,
            n_upsampled,
            buffer_1,
            buffer_1,
            0,
            0);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_mem_t *buffer_2 = pv_ypu_buffer_get(
            ypu,
            num_intermediate_channels * n_upsampled * (int32_t) sizeof(float),
            false);
    if (!buffer_2) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    PV_ORCA_PROFILER_START("convnext_transposed_kernel_1");
    status = pv_cnn_forward(
            ypu,
            object->conv_1,
            n_upsampled,
            buffer_1,
            buffer_2,
            0,
            0);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    PV_ORCA_PROFILER_STOP("convnext_transposed_kernel_1");

    PV_ORCA_PROFILER_START("convnext_transposed_gelu");

    pv_activation_gelu_float_approx(
            ypu,
            n_upsampled * pv_cnn_output_channels(object->conv_1),
            buffer_2,
            0);

    PV_ORCA_PROFILER_STOP("convnext_transposed_gelu");

    PV_ORCA_PROFILER_START("convnext_transposed_kernel_1");
    status = pv_cnn_forward(
            ypu,
            object->conv_2,
            n_upsampled,
            buffer_2,
            y_ypu_mem,
            0,
            y_offset);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }
    PV_ORCA_PROFILER_STOP("convnext_transposed_kernel_1");

    pv_ypu_buffer_release(ypu, buffer_1);
    pv_ypu_buffer_release(ypu, buffer_2);

    PV_ORCA_PROFILER_STOP("convnext_transposed");

    return PV_STATUS_SUCCESS;
}

int32_t PV_MOCKABLE(pv_convnext_transposed_num_output_frames)(const pv_convnext_transposed_t *object, int32_t n) {
    PV_ASSERT(object);
    PV_ASSERT(n > 0);

    return pv_cnn_transposed_depthwise_num_output_frames(object->conv_transposed_depthwise, n);
}
