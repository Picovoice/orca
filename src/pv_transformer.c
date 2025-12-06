#include <stdlib.h>
#include <string.h>

#include "core/pv_error_messages.h"
#include "orca/pv_attention.h"
#include "orca/pv_profiler.h"
#include "orca/pv_transformer.h"
#include "orca/pv_transformer_ffn.h"
#include "util/pv_check_status.h"
#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_transformer_param_serialize)(
        pv_ypu_t *ypu,
        const pv_transformer_param_t *param,
        FILE *file) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(file);

    pv_status_t status = pv_layer_norm_param_serialize(ypu, param->layer_norm_attention_param, file);
    PV_CHECK_STATUS(status);

    status = pv_layer_norm_param_serialize(ypu, param->layer_norm_ffn_param, file);
    PV_CHECK_STATUS(status);

    status = pv_attention_param_serialize(ypu, param->attention_param, file);
    PV_CHECK_STATUS(status);

    status = pv_transformer_ffn_param_serialize(ypu, param->ffn_param, file);
    PV_CHECK_STATUS(status);

    return PV_STATUS_SUCCESS;
}

#endif

pv_status_t PV_MOCKABLE(pv_transformer_param_load)(pv_ypu_t *ypu, FILE *f, pv_transformer_param_t **param) {
    PV_ASSERT(ypu);
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_transformer_param_t *p = pv_ypu_host_alloc(ypu, sizeof(pv_transformer_param_t));
    PV_CHECK_ALLOC(p);

    memset(p, 0, sizeof(pv_transformer_param_t));

    pv_status_t status = pv_layer_norm_param_load(ypu, f, (pv_layer_norm_param_t **) &(p->layer_norm_attention_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_transformer_param_delete(ypu, p);
        return status;
    }

    status = pv_layer_norm_param_load(ypu, f, (pv_layer_norm_param_t **) &(p->layer_norm_ffn_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_transformer_param_delete(ypu, p);
        return status;
    }

    status = pv_attention_param_load(ypu, f, (pv_attention_param_t **) &(p->attention_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_transformer_param_delete(ypu, p);
        return status;
    }

    status = pv_transformer_ffn_param_load(ypu, f, (pv_transformer_ffn_param_t **) &(p->ffn_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_transformer_param_delete(ypu, p);
        return status;
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}


void PV_MOCKABLE(pv_transformer_param_delete)(pv_ypu_t *ypu, pv_transformer_param_t *param) {
    PV_ASSERT(ypu);

    if (param) {
        pv_transformer_ffn_param_delete(ypu, (pv_transformer_ffn_param_t *) (param->ffn_param));

        pv_attention_param_delete(ypu, (pv_attention_param_t *) (param->attention_param));

        pv_layer_norm_param_delete(ypu, (pv_layer_norm_param_t *) (param->layer_norm_ffn_param));

        pv_layer_norm_param_delete(ypu, (pv_layer_norm_param_t *) (param->layer_norm_attention_param));

        pv_ypu_host_free(ypu, param);
    }
}

bool PV_MOCKABLE(pv_transformer_param_is_equal)(
        const pv_transformer_param_t *object,
        const pv_transformer_param_t *other) {
    PV_ASSERT(object);
    PV_ASSERT(other);

    if (!pv_layer_norm_param_is_equal(object->layer_norm_attention_param, other->layer_norm_attention_param)) {
        return false;
    }

    if (!pv_layer_norm_param_is_equal(object->layer_norm_ffn_param, other->layer_norm_ffn_param)) {
        return false;
    }

    if (!pv_attention_param_is_equal(object->attention_param, other->attention_param)) {
        return false;
    }

    if (!pv_transformer_ffn_param_is_equal(object->ffn_param, other->ffn_param)) {
        return false;
    }

    return true;
}

struct pv_transformer {
    const pv_transformer_param_t *param;

    pv_layer_norm_t *layer_norm_attention;
    pv_layer_norm_t *layer_norm_ffn;
    pv_attention_t *attention;
    pv_transformer_ffn_t *ffn;
};

pv_status_t PV_MOCKABLE(pv_transformer_init)(
        pv_ypu_t *ypu,
        const pv_transformer_param_t *param,
        pv_transformer_t **object) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(object);

    *object = NULL;

    pv_transformer_t *o = pv_ypu_host_alloc(ypu, sizeof(pv_transformer_t));
    if (!o) {
        pv_transformer_delete(ypu, o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(o, 0, sizeof(pv_transformer_t));

    o->param = param;

    pv_status_t status = pv_layer_norm_init(ypu, param->layer_norm_attention_param, &(o->layer_norm_attention));
    if (status != PV_STATUS_SUCCESS) {
        pv_transformer_delete(ypu, o);
        return status;
    }

    status = pv_layer_norm_init(ypu, param->layer_norm_ffn_param, &(o->layer_norm_ffn));
    if (status != PV_STATUS_SUCCESS) {
        pv_transformer_delete(ypu, o);
        return status;
    }

    status = pv_attention_init(
            ypu,
            param->attention_param,
            &(o->attention));
    if (status != PV_STATUS_SUCCESS) {
        pv_transformer_delete(ypu, o);
        return status;
    }

    status = pv_transformer_ffn_init(ypu, param->ffn_param, &(o->ffn));
    if (status != PV_STATUS_SUCCESS) {
        pv_transformer_delete(ypu, o);
        return status;
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_transformer_delete)(pv_ypu_t *ypu, pv_transformer_t *object) {
    PV_ASSERT(ypu);

    if (object) {
        pv_transformer_ffn_delete(ypu, object->ffn);
        pv_attention_delete(ypu, object->attention);
        pv_layer_norm_delete(ypu, object->layer_norm_ffn);
        pv_layer_norm_delete(ypu, object->layer_norm_attention);

        pv_ypu_host_free(ypu, object);
    }
}

pv_status_t PV_MOCKABLE(pv_transformer_forward)(
        pv_ypu_t *ypu,
        pv_transformer_t *object,
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
    PV_ORCA_PROFILER_START("transformer");

    const int32_t num_channels = object->param->layer_norm_attention_param->num_channels;

    pv_ypu_mem_t *buffer_1 = pv_ypu_buffer_get(
            ypu,
            n * num_channels * (int32_t) sizeof(float),
            false);
    if (!buffer_1) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_status_t status = pv_attention_forward(
            ypu,
            object->attention,
            n,
            x_ypu_mem,
            x_ypu_mem,
            buffer_1,
            x_offset,
            x_offset,
            0);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_op_pairwise_args_t args0 = {
            .output = buffer_1,
            .lhs = x_ypu_mem,
            .rhs = buffer_1,
            .length = num_channels * n,
            .output_offset = 0,
            .lhs_offset = x_offset,
            .rhs_offset = 0,
    };

    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_ADD,
            &args0);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_layer_norm_forward(
            ypu,
            object->layer_norm_attention,
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
            n * num_channels * (int32_t) sizeof(float),
            false);
    if (!buffer_2) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    status = pv_transformer_ffn_forward(
            ypu,
            object->ffn,
            n,
            buffer_1,
            buffer_2,
            0,
            0);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_op_pairwise_args_t args1 = {
            .output = buffer_2,
            .lhs = buffer_1,
            .rhs = buffer_2,
            .length = num_channels * n,
            .output_offset = 0,
            .lhs_offset = 0,
            .rhs_offset = 0,
    };

    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_ADD,
            &args1);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_layer_norm_forward(
            ypu,
            object->layer_norm_ffn,
            n,
            buffer_2,
            buffer_2,
            0,
            0);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_op_memcpy_args_t args2 = {
            .output = y_ypu_mem,
            .input = buffer_2,
            .size_bytes = n * num_channels * (int32_t) sizeof(float),
            .output_offset = y_offset,
            .input_offset = 0,
    };

    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_MEMCPY,
            &args2);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_buffer_release(ypu, buffer_2);
    pv_ypu_buffer_release(ypu, buffer_1);

    PV_ORCA_PROFILER_STOP("transformer");

    return PV_STATUS_SUCCESS;
}
