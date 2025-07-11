#include <stdlib.h>

#include "orca/pv_transformer_ffn.h"
#include "util/pv_check_status.h"
#include "core/pv_type.h"
#include "orca/pv_profiler.h"
#include "orca/pv_buffer.h"
#include "nn/pv_activation.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_transformer_ffn_param_serialize)(const pv_transformer_ffn_param_t *param, FILE *file) {
    PV_ASSERT(param);
    PV_ASSERT(file);

    pv_status_t status = pv_cnn_param_serialize(param->conv_1_param, file);
    PV_CHECK_STATUS(status);

    status = pv_cnn_param_serialize(param->conv_2_param, file);
    PV_CHECK_STATUS(status);

    return PV_STATUS_SUCCESS;
}

#endif

pv_status_t PV_MOCKABLE(pv_transformer_ffn_param_load)(FILE *f, pv_transformer_ffn_param_t **param) {
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_transformer_ffn_param_t *p = calloc(1, sizeof(pv_transformer_ffn_param_t));
    PV_CHECK_ALLOC(p);

    pv_status_t status = pv_cnn_param_load(f, (pv_cnn_param_t **) &(p->conv_1_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_transformer_ffn_param_delete(p);
        return status;
    }

    status = pv_cnn_param_load(f, (pv_cnn_param_t **) &(p->conv_2_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_transformer_ffn_param_delete(p);
        return status;
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}


void PV_MOCKABLE(pv_transformer_ffn_param_delete)(pv_transformer_ffn_param_t *param) {
    if (param) {
        pv_cnn_param_delete((pv_cnn_param_t *) (param->conv_2_param));

        pv_cnn_param_delete((pv_cnn_param_t *) (param->conv_1_param));

        free(param);
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

    pv_buffer_t *buffer_text_encoder_transf_ffn;
};

pv_status_t PV_MOCKABLE(pv_transformer_ffn_init)(
        const pv_transformer_ffn_param_t *param,
        pv_buffer_t *buffer_text_encoder_transf_ffn,
        pv_transformer_ffn_t **object) {
    PV_ASSERT(param);
    PV_ASSERT(object);

    *object = NULL;

    pv_transformer_ffn_t *o = calloc(1, sizeof(pv_transformer_ffn_t));
    if (!o) {
        pv_transformer_ffn_delete(o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->param = param;
    o->buffer_text_encoder_transf_ffn = buffer_text_encoder_transf_ffn;

    pv_status_t status = pv_cnn_init(param->conv_1_param, &(o->conv_1));
    if (status != PV_STATUS_SUCCESS) {
        pv_transformer_ffn_delete(o);
        return status;
    }

    status = pv_cnn_init(param->conv_2_param, &(o->conv_2));
    if (status != PV_STATUS_SUCCESS) {
        pv_transformer_ffn_delete(o);
        return status;
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_transformer_ffn_delete)(pv_transformer_ffn_t *object) {
    if (object) {
        pv_cnn_delete(object->conv_2);
        pv_cnn_delete(object->conv_1);

        free(object);
    }
}

int32_t PV_MOCKABLE(pv_transformer_ffn_output_channels)(const pv_transformer_ffn_t *object) {
    PV_ASSERT(object);

    return pv_cnn_output_channels(object->conv_2);
}

pv_status_t
PV_MOCKABLE(pv_transformer_ffn_forward)(pv_transformer_ffn_t *object, int32_t n, const float *x, float *y) {
    PV_ASSERT(object);
    PV_ASSERT(n);
    PV_ASSERT(x);
    PV_ASSERT(y);
    PV_ORCA_PROFILER_START("transformer_ffn");

    float *buffer = pv_buffer_get(object->buffer_text_encoder_transf_ffn, n, false);
    if (!buffer) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_status_t status = pv_cnn_forward(object->conv_1, n, x, buffer);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_activation_relu_float(n * pv_buffer_dimension(object->buffer_text_encoder_transf_ffn), buffer);

    status = pv_cnn_forward(object->conv_2, n, buffer, y);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    PV_ORCA_PROFILER_STOP("transformer_ffn");
    return PV_STATUS_SUCCESS;
}
