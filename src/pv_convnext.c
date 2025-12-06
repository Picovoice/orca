#include <stdlib.h>
#include <string.h>

#include "core/pv_type.h"
#include "io/pv_dump.h"
#include "model/pv_activation.h"
#include "orca/pv_cnn.h"
#include "orca/pv_convnext.h"
#include "orca/pv_profiler.h"
#include "util/pv_check_status.h"
#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"
#include "ypu/mock/pv_ypu_mock.h"

#endif

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_convnext_param_serialize)(pv_ypu_t *ypu, const pv_convnext_param_t *param, FILE *file) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(file);

    pv_status_t status = pv_cnn_depthwise_param_serialize(ypu, param->conv_depthwise_param, file);
    PV_CHECK_STATUS(status);

    status = pv_layer_norm_param_serialize(ypu, param->layer_norm_param, file);
    PV_CHECK_STATUS(status);

    status = pv_cnn_param_serialize(ypu, param->conv_1_param, file);
    PV_CHECK_STATUS(status);

    status = pv_cnn_param_serialize(ypu, param->conv_2_param, file);
    PV_CHECK_STATUS(status);

    const size_t length = sizeof(float) * param->conv_2_param->output_channels;
    const size_t count = fwrite(param->scale_param->data, 1, length, file);
    if (count != length) {
        return PV_STATUS_IO_ERROR;
    }

    return PV_STATUS_SUCCESS;
}

#endif

pv_status_t PV_MOCKABLE(pv_convnext_param_load)(pv_ypu_t *ypu, FILE *f, pv_convnext_param_t **param) {
    PV_ASSERT(ypu);
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_convnext_param_t *p = pv_ypu_host_alloc(ypu, sizeof(pv_convnext_param_t));
    PV_CHECK_ALLOC(p);

    memset(p, 0, sizeof(pv_convnext_param_t));

    pv_status_t status = pv_cnn_depthwise_param_load(ypu, f, (pv_cnn_depthwise_param_t **) &(p->conv_depthwise_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_convnext_param_delete(ypu, p);
        return status;
    }

    status = pv_layer_norm_param_load(ypu, f, (pv_layer_norm_param_t **) &(p->layer_norm_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_convnext_param_delete(ypu, p);
        return status;
    }

    status = pv_cnn_param_load(ypu, f, (pv_cnn_param_t **) &(p->conv_1_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_convnext_param_delete(ypu, p);
        return status;
    }

    status = pv_cnn_param_load(ypu, f, (pv_cnn_param_t **) &(p->conv_2_param));
    if (status != PV_STATUS_SUCCESS) {
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
    const size_t count = pv_fread(p->scale_param->data, sizeof(float), length, f);
    if (count != length) {
        pv_convnext_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}


void PV_MOCKABLE(pv_convnext_param_delete)(pv_ypu_t *ypu, pv_convnext_param_t *param) {
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
};

pv_status_t PV_MOCKABLE(pv_convnext_init)(
        pv_ypu_t *ypu,
        const pv_convnext_param_t *param,
        pv_convnext_t **object) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(object);

    *object = NULL;

    pv_convnext_t *o = pv_ypu_host_alloc(ypu, sizeof(pv_convnext_t));
    if (!o) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(o, 0, sizeof(pv_convnext_t));

    o->param = param;

    pv_status_t status = pv_cnn_depthwise_init(
            ypu,
            param->conv_depthwise_param,
            &(o->conv_depthwise));
    if (status != PV_STATUS_SUCCESS) {
        pv_convnext_delete(ypu, o);
        return status;
    }

    status = pv_layer_norm_init(
            ypu,
            param->layer_norm_param,
            &(o->layer_norm));
    if (status != PV_STATUS_SUCCESS) {
        pv_convnext_delete(ypu, o);
        return status;
    }

    status = pv_cnn_init(
            ypu,
            param->conv_1_param,
            &(o->conv_1));
    if (status != PV_STATUS_SUCCESS) {
        pv_convnext_delete(ypu, o);
        return status;
    }

    status = pv_cnn_init(
            ypu,
            param->conv_2_param,
            &(o->conv_2));
    if (status != PV_STATUS_SUCCESS) {
        pv_convnext_delete(ypu, o);
        return status;
    }

    o->scale = pv_ypu_mem_from_config(
            ypu,
            param->scale_param);
    if (o->scale == NULL) {
        pv_convnext_delete(ypu, o);
        return status;
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_convnext_delete)(pv_ypu_t *ypu, pv_convnext_t *object) {
    PV_ASSERT(ypu);

    if (object) {
        pv_cnn_delete(ypu, object->conv_2);
        pv_cnn_delete(ypu, object->conv_1);
        pv_layer_norm_delete(ypu, object->layer_norm);
        pv_cnn_depthwise_delete(ypu, object->conv_depthwise);
        pv_ypu_mem_free(ypu, object->scale);
        pv_ypu_host_free(ypu, object);
    }
}

pv_status_t PV_MOCKABLE(pv_convnext_forward)(
        pv_ypu_t *ypu,
        pv_convnext_t *object,
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
    PV_ORCA_PROFILER_START("convnext");

    pv_ypu_mem_t *buffer_1 = pv_ypu_buffer_get(
            ypu,
            object->param->conv_depthwise_param->num_channels * n * (int32_t) sizeof(float),
            false);
    if (!buffer_1) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_status_t status = pv_cnn_depthwise_forward(
            ypu,
            object->conv_depthwise,
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
            n,
            buffer_1,
            buffer_1,
            0,
            0);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_mem_t *buffer_2 = pv_ypu_buffer_get(
            ypu,
            object->param->conv_1_param->output_channels * n * (int32_t) sizeof(q510_t),
            false);
    if (!buffer_2) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    PV_ORCA_PROFILER_START("convnext_kernel_1");
    status = pv_cnn_forward_to_q510(
            ypu,
            object->conv_1,
            n,
            buffer_1,
            buffer_2,
            0,
            0);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }
    PV_ORCA_PROFILER_STOP("convnext_kernel_1");

    PV_ORCA_PROFILER_START("convnext_gelu");

    pv_activation_gelu_q510_approx(
            ypu,
            n * pv_cnn_output_channels(object->conv_1),
            buffer_2,
            0);

    PV_ORCA_PROFILER_STOP("convnext_gelu");

    PV_ORCA_PROFILER_START("convnext_kernel_1");
    status = pv_cnn_forward_from_q510(
            ypu,
            object->conv_2,
            n,
            buffer_2,
            buffer_1,
            0,
            0);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }
    PV_ORCA_PROFILER_STOP("convnext_kernel_1");

    const int32_t num_channels = pv_cnn_output_channels(object->conv_2);

    pv_ypu_op_pairwise_broadcast_args_t args0 = {
            .output = buffer_1,
            .lhs = buffer_1,
            .rhs = object->scale,
            .m = n,
            .n = num_channels,
            .output_offset = 0,
            .lhs_offset = 0,
            .rhs_offset = 0,
    };

    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_MULMV,
            &args0);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_op_pairwise_args_t args1 = {
            .output = y_ypu_mem,
            .lhs = x_ypu_mem,
            .rhs = buffer_1,
            .length = n * num_channels,
            .output_offset = y_offset,
            .lhs_offset = x_offset,
            .rhs_offset = 0,
    };

    status = pv_ypu_operator_execute(ypu, PV_YPU_OPERATOR_ADD, &args1);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_buffer_release(ypu, buffer_2);
    pv_ypu_buffer_release(ypu, buffer_1);

    PV_ORCA_PROFILER_STOP("convnext");
    return PV_STATUS_SUCCESS;
}
