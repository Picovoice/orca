#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "orca/pv_buffer.h"
#include "orca/pv_orca_util.h"
#include "orca/pv_residual_coupling.h"
#include "orca/pv_wavenet_resblock.h"
#include "util/pv_check_status.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_wavenet_resblock_param_serialize)(const pv_wavenet_resblock_param_t *param, FILE *file) {
    PV_ASSERT(param);
    PV_ASSERT(file);

    pv_status_t status = pv_cnn_param_serialize(param->conv_param, file);
    PV_CHECK_STATUS(status);

    status = pv_cnn_param_serialize(param->conv_skip_param, file);
    PV_CHECK_STATUS(status);

    return PV_STATUS_SUCCESS;
}

#endif

pv_status_t PV_MOCKABLE(pv_wavenet_resblock_param_load)(FILE *f, pv_wavenet_resblock_param_t **param) {
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_wavenet_resblock_param_t *p = calloc(1, sizeof(pv_wavenet_resblock_param_t));
    PV_CHECK_ALLOC(p);

    pv_status_t status = pv_cnn_param_load(f, (pv_cnn_param_t **) &(p->conv_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_wavenet_resblock_param_delete(p);
        return status;
    }

    status = pv_cnn_param_load(f, (pv_cnn_param_t **) &(p->conv_skip_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_wavenet_resblock_param_delete(p);
        return status;
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}


void PV_MOCKABLE(pv_wavenet_resblock_param_delete)(pv_wavenet_resblock_param_t *param) {
    if (param) {
        pv_cnn_param_delete((pv_cnn_param_t *) (param->conv_skip_param));
        pv_cnn_param_delete((pv_cnn_param_t *) (param->conv_param));

        free(param);
    }
}

bool PV_MOCKABLE(pv_wavenet_resblock_param_is_equal)(
        const pv_wavenet_resblock_param_t *object,
        const pv_wavenet_resblock_param_t *other) {
    PV_ASSERT(object);
    PV_ASSERT(other);

    if (!pv_cnn_param_is_equal(object->conv_param, other->conv_param)) {
        return false;
    }

    if (!pv_cnn_param_is_equal(object->conv_skip_param, other->conv_skip_param)) {
        return false;
    }

    return true;
}

struct pv_wavenet_resblock {
    const pv_wavenet_resblock_param_t *param;

    pv_cnn_t *conv;
    pv_cnn_t *conv_skip;

    pv_buffer_t *buffer_flow_wavenet_hidden;
    pv_buffer_t *buffer_flow_wavenet_inter;
    pv_buffer_t *buffer_flow_wavenet_inter_out;
};

pv_status_t PV_MOCKABLE(pv_wavenet_resblock_init)(
        const pv_wavenet_resblock_param_t *param,
        pv_buffer_t *buffer_flow_wavenet_hidden,
        pv_buffer_t *buffer_flow_wavenet_inter,
        pv_buffer_t *buffer_flow_wavenet_inter_out,
        pv_wavenet_resblock_t **object) {
    PV_ASSERT(param);
    PV_ASSERT(object);

    *object = NULL;

    pv_wavenet_resblock_t *o = calloc(1, sizeof(pv_wavenet_resblock_t));
    if (!o) {
        pv_wavenet_resblock_delete(o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->param = param;
    o->buffer_flow_wavenet_hidden = buffer_flow_wavenet_hidden;
    o->buffer_flow_wavenet_inter = buffer_flow_wavenet_inter;
    o->buffer_flow_wavenet_inter_out = buffer_flow_wavenet_inter_out;

    pv_status_t status = pv_cnn_init(param->conv_param, &(o->conv));
    if (status != PV_STATUS_SUCCESS) {
        pv_wavenet_resblock_delete(o);
        return status;
    }

    status = pv_cnn_init(param->conv_skip_param, &(o->conv_skip));
    if (status != PV_STATUS_SUCCESS) {
        pv_wavenet_resblock_delete(o);
        return status;
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_wavenet_resblock_delete)(pv_wavenet_resblock_t *object) {
    if (object) {
        pv_cnn_delete(object->conv_skip);
        pv_cnn_delete(object->conv);

        free(object);
    }
}

int32_t PV_MOCKABLE(pv_wavenet_resblock_output_channels)(const pv_wavenet_resblock_t *object) {
    PV_ASSERT(object);

    return pv_cnn_input_channels(object->conv);
}

pv_status_t PV_MOCKABLE(pv_wavenet_resblock_forward)(
        pv_wavenet_resblock_t *object,
        bool last_block,
        int32_t n,
        float *x,
        float *y) {
    PV_ASSERT(object);
    PV_ASSERT(n);
    PV_ASSERT(x);
    PV_ASSERT(y);

    const int32_t num_channels = pv_cnn_input_channels(object->conv);
    const int32_t hidden_channels = pv_cnn_output_channels(object->conv);

    float *buffer_hidden = pv_buffer_get(object->buffer_flow_wavenet_hidden, n, false);
    if (!buffer_hidden) {
        return PV_STATUS_OUT_OF_MEMORY;
    }
    pv_status_t status = pv_cnn_forward(object->conv, n, x, buffer_hidden);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    float *buffer_inter = pv_buffer_get(object->buffer_flow_wavenet_inter, n, false);
    if (!buffer_inter) {
        return PV_STATUS_OUT_OF_MEMORY;
    }
    pv_orca_util_fused_tanh_sigmoid_multiply(n, hidden_channels, buffer_hidden, buffer_inter);

    if (!last_block) {
        status = pv_cnn_forward(object->conv_skip, n, buffer_inter, buffer_hidden);
        if (status != PV_STATUS_SUCCESS) {
            return status;
        }

        for (int32_t i = 0; i < n; i++) {
            const int32_t channels_offset = i * num_channels;
            const int32_t channels_offset_two = i * hidden_channels;

            for (int32_t j = 0; j < num_channels; j++) {
                x[channels_offset + j] += buffer_hidden[channels_offset_two + j];
                y[channels_offset + j] += buffer_hidden[channels_offset_two + j + num_channels];
            }
        }
    } else {
        float *buffer_inter2 = pv_buffer_get(object->buffer_flow_wavenet_inter_out, n, false);
        if (!buffer_inter2) {
            return PV_STATUS_OUT_OF_MEMORY;
        }
        status = pv_cnn_forward(object->conv_skip, n, buffer_inter, buffer_inter2);
        if (status != PV_STATUS_SUCCESS) {
            return status;
        }

        for (int32_t i = 0; i < n * num_channels; i++) {
            y[i] = y[i] + buffer_inter2[i];
        }
    }

    return PV_STATUS_SUCCESS;
}
