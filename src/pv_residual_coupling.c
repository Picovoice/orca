#include <stdlib.h>
#include <string.h>

#include "orca/pv_buffer.h"
#include "orca/pv_orca_util.h"
#include "orca/pv_residual_coupling.h"
#include "util/pv_check_status.h"
#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_residual_coupling_param_serialize)(const pv_residual_coupling_param_t *param, FILE *file) {
    PV_ASSERT(param);
    PV_ASSERT(file);

    pv_status_t status = pv_cnn_param_serialize(param->conv_pre_param, file);
    PV_CHECK_STATUS(status);

    status = pv_cnn_param_serialize(param->conv_post_param, file);
    PV_CHECK_STATUS(status);

    const size_t count = fwrite(&(param->num_wavenet_resblocks), sizeof(int32_t), 1, file);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

    for (int32_t i = 0; i < param->num_wavenet_resblocks; i++) {
        status = pv_wavenet_resblock_param_serialize(param->wavenet_resblocks_param[i], file);
        if (status != PV_STATUS_SUCCESS) {
            return status;
        }
    }

    return PV_STATUS_SUCCESS;
}

#endif

pv_status_t PV_MOCKABLE(pv_residual_coupling_param_load)(FILE *f, pv_residual_coupling_param_t **param) {
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_residual_coupling_param_t *p = calloc(1, sizeof(pv_residual_coupling_param_t));
    PV_CHECK_ALLOC(p);

    pv_status_t status = pv_cnn_param_load(f, (pv_cnn_param_t **) &(p->conv_pre_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_residual_coupling_param_delete(p);
        return status;
    }

    status = pv_cnn_param_load(f, (pv_cnn_param_t **) &(p->conv_post_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_residual_coupling_param_delete(p);
        return status;
    }

    size_t count = pv_fread(&(p->num_wavenet_resblocks), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_residual_coupling_param_delete(p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->num_wavenet_resblocks <= 0) {
        pv_residual_coupling_param_delete(p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    p->wavenet_resblocks_param = calloc(p->num_wavenet_resblocks, sizeof(pv_wavenet_resblock_param_t *));
    if (!(p->wavenet_resblocks_param)) {
        pv_residual_coupling_param_delete(p);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    for (int32_t i = 0; i < p->num_wavenet_resblocks; i++) {
        status = pv_wavenet_resblock_param_load(f, (pv_wavenet_resblock_param_t **) &(p->wavenet_resblocks_param[i]));
        if (status != PV_STATUS_SUCCESS) {
            pv_residual_coupling_param_delete(p);
            return status;
        }
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}


void PV_MOCKABLE(pv_residual_coupling_param_delete)(pv_residual_coupling_param_t *param) {
    if (param) {
        if (param->wavenet_resblocks_param) {
            for (int32_t i = param->num_wavenet_resblocks - 1; i >= 0; i--) {
                pv_wavenet_resblock_param_delete((pv_wavenet_resblock_param_t *) (param->wavenet_resblocks_param[i]));
            }

            free(param->wavenet_resblocks_param);
        }
        pv_cnn_param_delete((pv_cnn_param_t *) (param->conv_post_param));

        pv_cnn_param_delete((pv_cnn_param_t *) (param->conv_pre_param));

        free(param);
    }
}

bool PV_MOCKABLE(pv_residual_coupling_param_is_equal)(
        const pv_residual_coupling_param_t *object,
        const pv_residual_coupling_param_t *other) {
    PV_ASSERT(object);
    PV_ASSERT(other);

    if (!pv_cnn_param_is_equal(object->conv_pre_param, other->conv_pre_param)) {
        return false;
    }

    if (!pv_cnn_param_is_equal(object->conv_post_param, other->conv_post_param)) {
        return false;
    }

    if (object->num_wavenet_resblocks != other->num_wavenet_resblocks) {
        return false;
    }

    for (int32_t i = 0; i < object->num_wavenet_resblocks; i++) {
        if (!pv_wavenet_resblock_param_is_equal(object->wavenet_resblocks_param[i],
                                                other->wavenet_resblocks_param[i])) {
            return false;
        }
    }

    return true;
}

struct pv_residual_coupling {
    const pv_residual_coupling_param_t *param;

    pv_cnn_t *conv_pre;
    pv_cnn_t *conv_post;
    pv_wavenet_resblock_t **wavenet_resblocks;

    pv_buffer_t *buffer_flow_residual_coupling_x0;
    pv_buffer_t *buffer_flow_residual_coupling_x1;
    pv_buffer_t *buffer_flow_residual_coupling_mean;
    pv_buffer_t *buffer_flow_wavenet_in;
    pv_buffer_t *buffer_flow_wavenet_hidden;
    pv_buffer_t *buffer_flow_wavenet_inter;
    pv_buffer_t *buffer_flow_wavenet_inter_out;
    pv_buffer_t *buffer_flow_wavenet_out;
};

pv_status_t PV_MOCKABLE(pv_residual_coupling_init)(
        const pv_residual_coupling_param_t *param,
        pv_buffer_t *buffer_flow_residual_coupling_x0,
        pv_buffer_t *buffer_flow_residual_coupling_x1,
        pv_buffer_t *buffer_flow_residual_coupling_mean,
        pv_buffer_t *buffer_flow_wavenet_in,
        pv_buffer_t *buffer_flow_wavenet_hidden,
        pv_buffer_t *buffer_flow_wavenet_inter,
        pv_buffer_t *buffer_flow_wavenet_inter_out,
        pv_buffer_t *buffer_flow_wavenet_out,
        pv_residual_coupling_t **object) {
    PV_ASSERT(param);
    PV_ASSERT(object);

    *object = NULL;

    pv_residual_coupling_t *o = calloc(1, sizeof(pv_residual_coupling_t));
    if (!o) {
        pv_residual_coupling_delete(o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->param = param;
    o->buffer_flow_residual_coupling_x0 = buffer_flow_residual_coupling_x0;
    o->buffer_flow_residual_coupling_x1 = buffer_flow_residual_coupling_x1;
    o->buffer_flow_residual_coupling_mean = buffer_flow_residual_coupling_mean;
    o->buffer_flow_wavenet_in = buffer_flow_wavenet_in;
    o->buffer_flow_wavenet_hidden = buffer_flow_wavenet_hidden;
    o->buffer_flow_wavenet_inter = buffer_flow_wavenet_inter;
    o->buffer_flow_wavenet_inter_out = buffer_flow_wavenet_inter_out;
    o->buffer_flow_wavenet_out = buffer_flow_wavenet_out;

    pv_status_t status = pv_cnn_init(param->conv_pre_param, &(o->conv_pre));
    if (status != PV_STATUS_SUCCESS) {
        pv_residual_coupling_delete(o);
        return status;
    }

    status = pv_cnn_init(param->conv_post_param, &(o->conv_post));
    if (status != PV_STATUS_SUCCESS) {
        pv_residual_coupling_delete(o);
        return status;
    }

    o->wavenet_resblocks = calloc(param->num_wavenet_resblocks, sizeof(pv_wavenet_resblock_t *));
    if (!(o->wavenet_resblocks)) {
        pv_residual_coupling_delete(o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    for (int32_t i = 0; i < param->num_wavenet_resblocks; i++) {
        status = pv_wavenet_resblock_init(
                param->wavenet_resblocks_param[i],
                o->buffer_flow_wavenet_hidden,
                o->buffer_flow_wavenet_inter,
                o->buffer_flow_wavenet_inter_out,
                &(o->wavenet_resblocks[i]));
        if (status != PV_STATUS_SUCCESS) {
            pv_residual_coupling_delete(o);
            return status;
        }
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_residual_coupling_delete)(pv_residual_coupling_t *object) {
    if (object) {
        if (object->wavenet_resblocks) {
            for (int32_t i = object->param->num_wavenet_resblocks - 1; i >= 0; i--) {
                pv_wavenet_resblock_delete(object->wavenet_resblocks[i]);
            }
            free(object->wavenet_resblocks);
        }

        pv_cnn_delete(object->conv_post);
        pv_cnn_delete(object->conv_pre);

        free(object);
    }
}

int32_t PV_MOCKABLE(pv_residual_coupling_output_channels)(const pv_residual_coupling_t *object) {
    PV_ASSERT(object);

    return pv_cnn_input_channels(object->conv_pre) * 2;
}

pv_status_t PV_MOCKABLE(pv_residual_coupling_forward)(
        pv_residual_coupling_t *object,
        int32_t n,
        const float *x,
        float *y) {
    PV_ASSERT(object);
    PV_ASSERT(n);
    PV_ASSERT(x);
    PV_ASSERT(y);

    const int32_t half_channels = pv_cnn_input_channels(object->conv_pre);
    const int32_t input_channels = 2 * half_channels;

    float *buffer_x0 = pv_buffer_get(object->buffer_flow_residual_coupling_x0, n, false);
    if (!buffer_x0) {
        return PV_STATUS_OUT_OF_MEMORY;
    }
    float *buffer_x1 = pv_buffer_get(object->buffer_flow_residual_coupling_x1, n, false);
    if (!buffer_x1) {
        return PV_STATUS_OUT_OF_MEMORY;
    }
    pv_orca_util_split_channels(n, input_channels, x, buffer_x0, buffer_x1);

    float *buffer_wavenet_in = pv_buffer_get(object->buffer_flow_wavenet_in, n, false);
    if (!buffer_wavenet_in) {
        return PV_STATUS_OUT_OF_MEMORY;
    }
    pv_status_t status = pv_cnn_forward(object->conv_pre, n, buffer_x0, buffer_wavenet_in);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    float *buffer_wavenet_out = pv_buffer_get(object->buffer_flow_wavenet_out, n, true);
    if (!buffer_wavenet_out) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    const int32_t num_wavenet_blocks = object->param->num_wavenet_resblocks;
    pv_wavenet_resblock_t **wavenet_resblocks = object->wavenet_resblocks;
    for (int32_t block_i = 0; block_i < num_wavenet_blocks; block_i++) {
        bool last_block = block_i == (num_wavenet_blocks - 1);
        status = pv_wavenet_resblock_forward(
                wavenet_resblocks[block_i],
                last_block,
                n,
                buffer_wavenet_in,
                buffer_wavenet_out);
        if (status != PV_STATUS_SUCCESS) {
            return status;
        }
    }

    float *buffer_mean = pv_buffer_get(object->buffer_flow_residual_coupling_mean, n, false);
    if (!buffer_mean) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    status = pv_cnn_forward(object->conv_post, n, buffer_wavenet_out, buffer_mean);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    for (int32_t i = 0; i < n * half_channels; i++) {
        buffer_x1[i] = buffer_x1[i] - buffer_mean[i];
    }

    pv_orca_util_concatenate_channel_wise(n, half_channels, buffer_x0, buffer_x1, y);

    return PV_STATUS_SUCCESS;
}
