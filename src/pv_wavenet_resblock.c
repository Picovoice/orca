#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "orca/pv_orca_util.h"
#include "orca/pv_residual_coupling.h"
#include "orca/pv_wavenet_resblock.h"
#include "util/pv_check_status.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_wavenet_resblock_param_serialize)(
        pv_ypu_t *ypu,
        const pv_wavenet_resblock_param_t *param,
        FILE *file) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(file);

    pv_status_t status = pv_cnn_param_serialize(ypu, param->conv_param, file);
    PV_CHECK_STATUS(status);

    status = pv_cnn_param_serialize(ypu, param->conv_skip_param, file);
    PV_CHECK_STATUS(status);

    return PV_STATUS_SUCCESS;
}

#endif

pv_status_t PV_MOCKABLE(pv_wavenet_resblock_param_load)(pv_ypu_t *ypu, FILE *f, pv_wavenet_resblock_param_t **param) {
    PV_ASSERT(ypu);
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_wavenet_resblock_param_t *p = pv_ypu_host_alloc(ypu, sizeof(pv_wavenet_resblock_param_t));
    PV_CHECK_ALLOC(p);

    memset(p, 0, sizeof(pv_wavenet_resblock_param_t));

    pv_status_t status = pv_cnn_param_load(ypu, f, (pv_cnn_param_t **) &(p->conv_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_wavenet_resblock_param_delete(ypu, p);
        return status;
    }

    status = pv_cnn_param_load(ypu, f, (pv_cnn_param_t **) &(p->conv_skip_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_wavenet_resblock_param_delete(ypu, p);
        return status;
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}


void PV_MOCKABLE(pv_wavenet_resblock_param_delete)(pv_ypu_t *ypu, pv_wavenet_resblock_param_t *param) {
    PV_ASSERT(ypu);

    if (param) {
        pv_cnn_param_delete(ypu, (pv_cnn_param_t *) (param->conv_skip_param));
        pv_cnn_param_delete(ypu, (pv_cnn_param_t *) (param->conv_param));

        pv_ypu_host_free(ypu, param);
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
};

pv_status_t PV_MOCKABLE(pv_wavenet_resblock_init)(
        pv_ypu_t *ypu,
        const pv_wavenet_resblock_param_t *param,
        pv_wavenet_resblock_t **object) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(object);

    *object = NULL;

    pv_wavenet_resblock_t *o = pv_ypu_host_alloc(ypu, sizeof(pv_wavenet_resblock_t));
    if (!o) {
        pv_wavenet_resblock_delete(ypu, o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(o, 0, sizeof(pv_wavenet_resblock_t));

    o->param = param;

    pv_status_t status = pv_cnn_init(ypu, param->conv_param, &(o->conv));
    if (status != PV_STATUS_SUCCESS) {
        pv_wavenet_resblock_delete(ypu, o);
        return status;
    }

    status = pv_cnn_init(ypu, param->conv_skip_param, &(o->conv_skip));
    if (status != PV_STATUS_SUCCESS) {
        pv_wavenet_resblock_delete(ypu, o);
        return status;
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_wavenet_resblock_delete)(pv_ypu_t *ypu, pv_wavenet_resblock_t *object) {
    PV_ASSERT(ypu);

    if (object) {
        pv_cnn_delete(ypu, object->conv_skip);
        pv_cnn_delete(ypu, object->conv);

        pv_ypu_host_free(ypu, object);
    }
}

int32_t PV_MOCKABLE(pv_wavenet_resblock_output_channels)(const pv_wavenet_resblock_t *object) {
    PV_ASSERT(object);

    return pv_cnn_input_channels(object->conv);
}

pv_status_t PV_MOCKABLE(pv_wavenet_resblock_forward)(
        pv_ypu_t *ypu,
        pv_wavenet_resblock_t *object,
        bool last_block,
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

    const int32_t num_channels = pv_cnn_input_channels(object->conv);
    const int32_t hidden_channels = pv_cnn_output_channels(object->conv);

    pv_ypu_mem_t *buffer_hidden_ypu_mem = pv_ypu_buffer_get(
            ypu,
            n * hidden_channels * (int32_t) sizeof(float),
            false);
    if (!buffer_hidden_ypu_mem) {
        return PV_STATUS_OUT_OF_MEMORY;
    }
    pv_status_t status = pv_cnn_forward(ypu, object->conv, n, x_ypu_mem, buffer_hidden_ypu_mem, x_offset, 0);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_mem_t *buffer_inter_ypu_mem = pv_ypu_buffer_get(
            ypu,
            n * hidden_channels * (int32_t) sizeof(float),
            false);
    if (!buffer_inter_ypu_mem) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_orca_util_fused_tanh_sigmoid_multiply(
            ypu,
            n,
            hidden_channels,
            buffer_hidden_ypu_mem,
            buffer_inter_ypu_mem,
            0,
            0);

    if (!last_block) {
        status = pv_cnn_forward(
                ypu,
                object->conv_skip,
                n,
                buffer_inter_ypu_mem,
                buffer_hidden_ypu_mem,
                0,
                0);
        if (status != PV_STATUS_SUCCESS) {
            return status;
        }

        for (int32_t i = 0; i < n; i++) {
            const int32_t channels_offset = i * num_channels;
            const int32_t channels_offset_two = i * hidden_channels;

            pv_ypu_op_pairwise_args_t args0 = {
                    .output = x_ypu_mem,
                    .lhs = x_ypu_mem,
                    .rhs = buffer_hidden_ypu_mem,
                    .length = num_channels,
                    .output_offset = x_offset + channels_offset * (int32_t) sizeof(float),
                    .lhs_offset = x_offset + channels_offset * (int32_t) sizeof(float),
                    .rhs_offset = channels_offset_two * (int32_t) sizeof(float),
            };

            status = pv_ypu_operator_execute(
                    ypu,
                    PV_YPU_OPERATOR_ADD,
                    &args0);
            if (status != PV_STATUS_SUCCESS) {
                return status;
            }

            pv_ypu_op_pairwise_args_t args1 = {
                    .output = y_ypu_mem,
                    .lhs = y_ypu_mem,
                    .rhs = buffer_hidden_ypu_mem,
                    .length = num_channels,
                    .output_offset = y_offset + channels_offset * (int32_t) sizeof(float),
                    .lhs_offset = y_offset + channels_offset * (int32_t) sizeof(float),
                    .rhs_offset = (channels_offset_two + num_channels) * (int32_t) sizeof(float),
            };

            status = pv_ypu_operator_execute(
                    ypu,
                    PV_YPU_OPERATOR_ADD,
                    &args1);
            if (status != PV_STATUS_SUCCESS) {
                return status;
            }
        }
    } else {
        pv_ypu_mem_t *buffer_inter2_ypu_mem = pv_ypu_buffer_get(
                ypu,
                n * num_channels * (int32_t) sizeof(float),
                false);
        if (!buffer_inter2_ypu_mem) {
            return PV_STATUS_OUT_OF_MEMORY;
        }

        status = pv_cnn_forward(
                ypu,
                object->conv_skip,
                n,
                buffer_inter_ypu_mem,
                buffer_inter2_ypu_mem,
                0,
                0);
        if (status != PV_STATUS_SUCCESS) {
            return status;
        }

        pv_ypu_op_pairwise_args_t args = {
                .output = y_ypu_mem,
                .lhs = y_ypu_mem,
                .rhs = buffer_inter2_ypu_mem,
                .length = n * num_channels,
                .output_offset = y_offset,
                .lhs_offset = y_offset,
                .rhs_offset = 0,
        };

        status = pv_ypu_operator_execute(
                ypu,
                PV_YPU_OPERATOR_ADD,
                &args);
        if (status != PV_STATUS_SUCCESS) {
            return status;
        }

        pv_ypu_buffer_release(ypu, buffer_inter2_ypu_mem);
    }

    pv_ypu_buffer_release(ypu, buffer_inter_ypu_mem);
    pv_ypu_buffer_release(ypu, buffer_hidden_ypu_mem);

    return PV_STATUS_SUCCESS;
}
