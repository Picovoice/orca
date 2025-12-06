#include <stdlib.h>
#include <string.h>

#include "orca/pv_orca_util.h"
#include "orca/pv_residual_coupling.h"
#include "util/pv_check_status.h"
#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_residual_coupling_param_serialize)(
        pv_ypu_t *ypu,
        const pv_residual_coupling_param_t *param,
        FILE *file) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(file);

    pv_status_t status = pv_cnn_param_serialize(ypu, param->conv_pre_param, file);
    PV_CHECK_STATUS(status);

    status = pv_cnn_param_serialize(ypu, param->conv_post_param, file);
    PV_CHECK_STATUS(status);

    const size_t count = fwrite(&(param->num_wavenet_resblocks), sizeof(int32_t), 1, file);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

    for (int32_t i = 0; i < param->num_wavenet_resblocks; i++) {
        status = pv_wavenet_resblock_param_serialize(ypu, param->wavenet_resblocks_param[i], file);
        if (status != PV_STATUS_SUCCESS) {
            return status;
        }
    }

    return PV_STATUS_SUCCESS;
}

#endif

pv_status_t PV_MOCKABLE(pv_residual_coupling_param_load)(pv_ypu_t *ypu, FILE *f, pv_residual_coupling_param_t **param) {
    PV_ASSERT(ypu);
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_residual_coupling_param_t *p = pv_ypu_host_alloc(ypu, sizeof(pv_residual_coupling_param_t));
    PV_CHECK_ALLOC(p);

    memset(p, 0, sizeof(pv_residual_coupling_param_t));

    pv_status_t status = pv_cnn_param_load(ypu, f, (pv_cnn_param_t **) &(p->conv_pre_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_residual_coupling_param_delete(ypu, p);
        return status;
    }

    status = pv_cnn_param_load(ypu, f, (pv_cnn_param_t **) &(p->conv_post_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_residual_coupling_param_delete(ypu, p);
        return status;
    }

    size_t count = pv_fread(&(p->num_wavenet_resblocks), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_residual_coupling_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->num_wavenet_resblocks <= 0) {
        pv_residual_coupling_param_delete(ypu, p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    p->wavenet_resblocks_param = pv_ypu_host_alloc(
            ypu,
            p->num_wavenet_resblocks * (int32_t) sizeof(pv_wavenet_resblock_param_t *));
    if (!(p->wavenet_resblocks_param)) {
        pv_residual_coupling_param_delete(ypu, p);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(p->wavenet_resblocks_param, 0, p->num_wavenet_resblocks * (int32_t) sizeof(pv_wavenet_resblock_param_t *));

    for (int32_t i = 0; i < p->num_wavenet_resblocks; i++) {
        status = pv_wavenet_resblock_param_load(
                ypu,
                f,
                (pv_wavenet_resblock_param_t **) &(p->wavenet_resblocks_param[i]));
        if (status != PV_STATUS_SUCCESS) {
            pv_residual_coupling_param_delete(ypu, p);
            return status;
        }
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}


void PV_MOCKABLE(pv_residual_coupling_param_delete)(pv_ypu_t *ypu, pv_residual_coupling_param_t *param) {
    PV_ASSERT(ypu);

    if (param) {
        if (param->wavenet_resblocks_param) {
            for (int32_t i = param->num_wavenet_resblocks - 1; i >= 0; i--) {
                pv_wavenet_resblock_param_delete(
                        ypu,
                        (pv_wavenet_resblock_param_t *) (param->wavenet_resblocks_param[i]));
            }

            pv_ypu_host_free(ypu, param->wavenet_resblocks_param);
        }
        pv_cnn_param_delete(ypu, (pv_cnn_param_t *) (param->conv_post_param));

        pv_cnn_param_delete(ypu, (pv_cnn_param_t *) (param->conv_pre_param));

        pv_ypu_host_free(ypu, param);
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
};

pv_status_t PV_MOCKABLE(pv_residual_coupling_init)(
        pv_ypu_t *ypu,
        const pv_residual_coupling_param_t *param,
        pv_residual_coupling_t **object) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(object);

    *object = NULL;

    pv_residual_coupling_t *o = pv_ypu_host_alloc(ypu, sizeof(pv_residual_coupling_t));
    if (!o) {
        pv_residual_coupling_delete(ypu, o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(o, 0, sizeof(pv_residual_coupling_t));

    o->param = param;

    pv_status_t status = pv_cnn_init(ypu, param->conv_pre_param, &(o->conv_pre));
    if (status != PV_STATUS_SUCCESS) {
        pv_residual_coupling_delete(ypu, o);
        return status;
    }

    status = pv_cnn_init(ypu, param->conv_post_param, &(o->conv_post));
    if (status != PV_STATUS_SUCCESS) {
        pv_residual_coupling_delete(ypu, o);
        return status;
    }

    o->wavenet_resblocks = pv_ypu_host_alloc(
            ypu,
            param->num_wavenet_resblocks * (int32_t) sizeof(pv_wavenet_resblock_t *));
    if (!(o->wavenet_resblocks)) {
        pv_residual_coupling_delete(ypu, o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(o->wavenet_resblocks, 0, param->num_wavenet_resblocks * (int32_t) sizeof(pv_wavenet_resblock_t *));

    for (int32_t i = 0; i < param->num_wavenet_resblocks; i++) {
        status = pv_wavenet_resblock_init(
                ypu,
                param->wavenet_resblocks_param[i],
                &(o->wavenet_resblocks[i]));
        if (status != PV_STATUS_SUCCESS) {
            pv_residual_coupling_delete(ypu, o);
            return status;
        }
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_residual_coupling_delete)(pv_ypu_t *ypu, pv_residual_coupling_t *object) {
    PV_ASSERT(ypu);

    if (object) {
        if (object->wavenet_resblocks) {
            for (int32_t i = object->param->num_wavenet_resblocks - 1; i >= 0; i--) {
                pv_wavenet_resblock_delete(ypu, object->wavenet_resblocks[i]);
            }
            pv_ypu_host_free(ypu, object->wavenet_resblocks);
        }

        pv_cnn_delete(ypu, object->conv_post);
        pv_cnn_delete(ypu, object->conv_pre);

        pv_ypu_host_free(ypu, object);
    }
}

int32_t PV_MOCKABLE(pv_residual_coupling_output_channels)(const pv_residual_coupling_t *object) {
    PV_ASSERT(object);

    return pv_cnn_input_channels(object->conv_pre) * 2;
}

pv_status_t PV_MOCKABLE(pv_residual_coupling_forward)(
        pv_ypu_t *ypu,
        pv_residual_coupling_t *object,
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

    const int32_t half_channels = pv_cnn_input_channels(object->conv_pre);
    const int32_t input_channels = 2 * half_channels;

    pv_ypu_mem_t *buffer_x0 = pv_ypu_buffer_get(
            ypu,
            n * half_channels * (int32_t) sizeof(float),
            false);
    if (!buffer_x0) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_mem_t *buffer_x1 = pv_ypu_buffer_get(
            ypu,
            n * half_channels * (int32_t) sizeof(float),
            false);
    if (!buffer_x1) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_status_t status = pv_orca_util_split_channels(
            ypu,
            n,
            input_channels,
            x_ypu_mem,
            buffer_x0,
            buffer_x1,
            x_offset,
            0,
            0);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_mem_t *buffer_wavenet_in = pv_ypu_buffer_get(
            ypu,
            n * pv_cnn_output_channels(object->conv_pre) * (int32_t) sizeof(float),
            false);
    if (!buffer_wavenet_in) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    status = pv_cnn_forward(
            ypu,
            object->conv_pre,
            n,
            buffer_x0,
            buffer_wavenet_in,
            0,
            0);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_mem_t *buffer_wavenet_out = pv_ypu_buffer_get(
            ypu,
            n * pv_cnn_input_channels(object->conv_post) * (int32_t) sizeof(float),
            false);
    if (!buffer_wavenet_out) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_op_memset_args_t args0 = {
            .output = buffer_wavenet_out,
            .size_bytes = n * pv_cnn_input_channels(object->conv_post) * (int32_t) sizeof(float),
            .output_offset = 0};

    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_MEMSET,
            &args0);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    const int32_t num_wavenet_blocks = object->param->num_wavenet_resblocks;
    pv_wavenet_resblock_t **wavenet_resblocks = object->wavenet_resblocks;
    for (int32_t block_i = 0; block_i < num_wavenet_blocks; block_i++) {
        bool last_block = block_i == (num_wavenet_blocks - 1);
        status = pv_wavenet_resblock_forward(
                ypu,
                wavenet_resblocks[block_i],
                last_block,
                n,
                buffer_wavenet_in,
                buffer_wavenet_out,
                0,
                0);
        if (status != PV_STATUS_SUCCESS) {
            return status;
        }
    }

    pv_ypu_buffer_release(ypu, buffer_wavenet_in);

    pv_ypu_mem_t *buffer_mean = pv_ypu_buffer_get(
            ypu,
            n * pv_cnn_output_channels(object->conv_post) * (int32_t) sizeof(float),
            false);
    if (!buffer_mean) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    status = pv_cnn_forward(
            ypu,
            object->conv_post,
            n,
            buffer_wavenet_out,
            buffer_mean,
            0,
            0);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_buffer_release(ypu, buffer_wavenet_out);

    pv_ypu_op_pairwise_args_t args1 = {
            .output = buffer_x1,
            .lhs = buffer_x1,
            .rhs = buffer_mean,
            .length = n * half_channels,
            .output_offset = 0,
            .lhs_offset = 0,
            .rhs_offset = 0,
    };

    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_SUB,
            &args1);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_op_pairwise_broadcast_args_t args2 = {
            .output = y_ypu_mem,
            .lhs = buffer_x0,
            .rhs = buffer_x1,
            .m = n,
            .n = half_channels,
            .output_offset = y_offset,
            .lhs_offset = 0,
            .rhs_offset = 0,
    };

    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_CAT,
            &args2);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_buffer_release(ypu, buffer_mean);
    pv_ypu_buffer_release(ypu, buffer_x1);
    pv_ypu_buffer_release(ypu, buffer_x0);

    return PV_STATUS_SUCCESS;
}
