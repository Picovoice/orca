#include <stdlib.h>
#include <string.h>

#include "core/pv_type.h"
#include "model/pv_activation.h"
#include "orca/pv_profiler.h"
#include "orca/pv_transformer_ffn.h"
#include "util/pv_check_status.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_transformer_ffn_param_serialize)(
        pv_ypu_t *ypu,
        const pv_transformer_ffn_param_t *param,
        FILE *file) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(file);

    pv_status_t status = pv_cnn_param_serialize(ypu, param->conv_1_param, file);
    PV_CHECK_STATUS(status);

    status = pv_cnn_param_serialize(ypu, param->conv_2_param, file);
    PV_CHECK_STATUS(status);

    return PV_STATUS_SUCCESS;
}

#endif

pv_status_t PV_MOCKABLE(pv_transformer_ffn_param_load)(pv_ypu_t *ypu, FILE *f, pv_transformer_ffn_param_t **param) {
    PV_ASSERT(ypu);
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_transformer_ffn_param_t *p = pv_ypu_host_alloc(ypu, sizeof(pv_transformer_ffn_param_t));
    PV_CHECK_ALLOC(p);

    memset(p, 0, sizeof(pv_transformer_ffn_param_t));

    pv_status_t status = pv_cnn_param_load(ypu, f, (pv_cnn_param_t **) &(p->conv_1_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_transformer_ffn_param_delete(ypu, p);
        return status;
    }

    status = pv_cnn_param_load(ypu, f, (pv_cnn_param_t **) &(p->conv_2_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_transformer_ffn_param_delete(ypu, p);
        return status;
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}


void PV_MOCKABLE(pv_transformer_ffn_param_delete)(pv_ypu_t *ypu, pv_transformer_ffn_param_t *param) {
    PV_ASSERT(ypu);

    if (param) {
        pv_cnn_param_delete(ypu, (pv_cnn_param_t *) (param->conv_2_param));

        pv_cnn_param_delete(ypu, (pv_cnn_param_t *) (param->conv_1_param));

        pv_ypu_host_free(ypu, param);
    }
}

bool PV_MOCKABLE(pv_transformer_ffn_param_is_equal)(
        const pv_transformer_ffn_param_t *object,
        const pv_transformer_ffn_param_t *other) {
    PV_ASSERT(object);
    PV_ASSERT(other);

    if (!pv_cnn_param_is_equal(object->conv_1_param, other->conv_1_param)) {
        return false;
    }

    if (!pv_cnn_param_is_equal(object->conv_2_param, other->conv_2_param)) {
        return false;
    }

    return true;
}

int32_t PV_MOCKABLE(pv_transformer_ffn_param_receptive_field)(const pv_transformer_ffn_param_t *param) {
    PV_ASSERT(param);

    int32_t receptive_field = 0;
    receptive_field += pv_cnn_param_receptive_field(param->conv_1_param);
    receptive_field += pv_cnn_param_receptive_field(param->conv_2_param);
    return receptive_field;
}

struct pv_transformer_ffn {
    const pv_transformer_ffn_param_t *param;

    pv_cnn_t *conv_1;
    pv_cnn_t *conv_2;
};

pv_status_t PV_MOCKABLE(pv_transformer_ffn_init)(
        pv_ypu_t *ypu,
        const pv_transformer_ffn_param_t *param,
        pv_transformer_ffn_t **object) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(object);

    *object = NULL;

    pv_transformer_ffn_t *o = pv_ypu_host_alloc(ypu, sizeof(pv_transformer_ffn_t));
    if (!o) {
        pv_transformer_ffn_delete(ypu, o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(o, 0, sizeof(pv_transformer_ffn_t));

    o->param = param;

    pv_status_t status = pv_cnn_init(ypu, param->conv_1_param, &(o->conv_1));
    if (status != PV_STATUS_SUCCESS) {
        pv_transformer_ffn_delete(ypu, o);
        return status;
    }

    status = pv_cnn_init(ypu, param->conv_2_param, &(o->conv_2));
    if (status != PV_STATUS_SUCCESS) {
        pv_transformer_ffn_delete(ypu, o);
        return status;
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_transformer_ffn_delete)(pv_ypu_t *ypu, pv_transformer_ffn_t *object) {
    PV_ASSERT(ypu);

    if (object) {
        pv_cnn_delete(ypu, object->conv_2);
        pv_cnn_delete(ypu, object->conv_1);

        pv_ypu_host_free(ypu, object);
    }
}

int32_t PV_MOCKABLE(pv_transformer_ffn_output_channels)(const pv_transformer_ffn_t *object) {
    PV_ASSERT(object);

    return pv_cnn_output_channels(object->conv_2);
}

pv_status_t
PV_MOCKABLE(pv_transformer_ffn_forward)(
        pv_ypu_t *ypu,
        pv_transformer_ffn_t *object,
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
    PV_ORCA_PROFILER_START("transformer_ffn");

    int32_t num_hidden_channels = object->param->conv_1_param->output_channels;

    pv_ypu_mem_t *buffer = pv_ypu_buffer_get(
            ypu,
            n * num_hidden_channels * (int32_t) sizeof(float),
            false);
    if (!buffer) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_status_t status = pv_cnn_forward(
            ypu,
            object->conv_1,
            n,
            x_ypu_mem,
            buffer,
            x_offset,
            0);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_activation_relu_float(
            ypu,
            n * num_hidden_channels,
            buffer,
            0);

    status = pv_cnn_forward(
            ypu,
            object->conv_2,
            n,
            buffer,
            y_ypu_mem,
            0,
            y_offset);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_buffer_release(ypu, buffer);

    PV_ORCA_PROFILER_STOP("transformer_ffn");
    return PV_STATUS_SUCCESS;
}
