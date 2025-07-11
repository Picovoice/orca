#include <stdlib.h>
#include <string.h>

#include "core/pv_error_messages.h"
#include "orca/pv_attention.h"
#include "orca/pv_buffer.h"
#include "orca/pv_transformer.h"
#include "orca/pv_transformer_ffn.h"
#include "orca/pv_profiler.h"
#include "util/pv_check_status.h"
#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_transformer_param_serialize)(const pv_transformer_param_t *param, FILE *file) {
    PV_ASSERT(param);
    PV_ASSERT(file);

    pv_status_t status = pv_layer_norm_param_serialize(param->layer_norm_attention_param, file);
    PV_CHECK_STATUS(status);

    status = pv_layer_norm_param_serialize(param->layer_norm_ffn_param, file);
    PV_CHECK_STATUS(status);

    status = pv_attention_param_serialize(param->attention_param, file);
    PV_CHECK_STATUS(status);

    status = pv_transformer_ffn_param_serialize(param->ffn_param, file);
    PV_CHECK_STATUS(status);

    return PV_STATUS_SUCCESS;
}

#endif

pv_status_t PV_MOCKABLE(pv_transformer_param_load)(FILE *f, pv_transformer_param_t **param) {
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_transformer_param_t *p = calloc(1, sizeof(pv_transformer_param_t));
    PV_CHECK_ALLOC(p);

    pv_status_t status = pv_layer_norm_param_load(f, (pv_layer_norm_param_t **) &(p->layer_norm_attention_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_transformer_param_delete(p);
        return status;
    }

    status = pv_layer_norm_param_load(f, (pv_layer_norm_param_t **) &(p->layer_norm_ffn_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_transformer_param_delete(p);
        return status;
    }

    status = pv_attention_param_load(f, (pv_attention_param_t **) &(p->attention_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_transformer_param_delete(p);
        return status;
    }

    status = pv_transformer_ffn_param_load(f, (pv_transformer_ffn_param_t **) &(p->ffn_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_transformer_param_delete(p);
        return status;
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}


void PV_MOCKABLE(pv_transformer_param_delete)(pv_transformer_param_t *param) {
    if (param) {
        pv_transformer_ffn_param_delete((pv_transformer_ffn_param_t *) (param->ffn_param));

        pv_attention_param_delete((pv_attention_param_t *) (param->attention_param));

        pv_layer_norm_param_delete((pv_layer_norm_param_t *) (param->layer_norm_ffn_param));

        pv_layer_norm_param_delete((pv_layer_norm_param_t *) (param->layer_norm_attention_param));

        free(param);
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

    pv_buffer_t *buffer_text_encoder_transf_attn_1;
    pv_buffer_t *buffer_text_encoder_transf_attn_2;
    pv_buffer_t *buffer_text_encoder_transf_attn__score;
    pv_buffer_t *buffer_text_encoder_transf_ffn;
};

pv_status_t PV_MOCKABLE(pv_transformer_init)(
        const pv_transformer_param_t *param,
        pv_buffer_t *buffer_text_encoder_transf_attn_1,
        pv_buffer_t *buffer_text_encoder_transf_attn_2,
        pv_buffer_t *buffer_text_encoder_transf_attn_score,
        pv_buffer_t *buffer_text_encoder_transf_ffn,
        pv_transformer_t **object) {
    PV_ASSERT(param);
    PV_ASSERT(object);

    *object = NULL;

    pv_transformer_t *o = calloc(1, sizeof(pv_transformer_t));
    if (!o) {
        pv_transformer_delete(o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->param = param;
    o->buffer_text_encoder_transf_attn_1 = buffer_text_encoder_transf_attn_1;
    o->buffer_text_encoder_transf_attn_2 = buffer_text_encoder_transf_attn_2;
    o->buffer_text_encoder_transf_attn__score = buffer_text_encoder_transf_attn_score;
    o->buffer_text_encoder_transf_ffn = buffer_text_encoder_transf_ffn;

    pv_status_t status = pv_layer_norm_init(param->layer_norm_attention_param, &(o->layer_norm_attention));
    if (status != PV_STATUS_SUCCESS) {
        pv_transformer_delete(o);
        return status;
    }

    status = pv_layer_norm_init(param->layer_norm_ffn_param, &(o->layer_norm_ffn));
    if (status != PV_STATUS_SUCCESS) {
        pv_transformer_delete(o);
        return status;
    }

    status = pv_attention_init(
            param->attention_param,
            o->buffer_text_encoder_transf_attn_1,
            o->buffer_text_encoder_transf_attn_2,
            o->buffer_text_encoder_transf_attn__score,
            &(o->attention));
    if (status != PV_STATUS_SUCCESS) {
        pv_transformer_delete(o);
        return status;
    }

    status = pv_transformer_ffn_init(param->ffn_param, o->buffer_text_encoder_transf_ffn, &(o->ffn));
    if (status != PV_STATUS_SUCCESS) {
        pv_transformer_delete(o);
        return status;
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_transformer_delete)(pv_transformer_t *object) {
    if (object) {
        pv_transformer_ffn_delete(object->ffn);
        pv_attention_delete(object->attention);
        pv_layer_norm_delete(object->layer_norm_ffn);
        pv_layer_norm_delete(object->layer_norm_attention);

        free(object);
    }
}

pv_status_t PV_MOCKABLE(pv_transformer_forward)(pv_transformer_t *object, int32_t n, const float *x, float *y) {
    PV_ASSERT(object);
    PV_ASSERT(n);
    PV_ASSERT(x);
    PV_ASSERT(y);
    PV_ORCA_PROFILER_START("transformer");

    float *buffer_1 = pv_buffer_get(object->buffer_text_encoder_transf_attn_1, n, false);
    if (!buffer_1) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_status_t status = pv_attention_forward(
            object->attention,
            n,
            x,
            x,
            buffer_1);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    const int32_t num_channels = object->param->layer_norm_attention_param->num_channels;
    for (int32_t i = 0; i < (num_channels * n); i++) {
        buffer_1[i] = x[i] + buffer_1[i];
    }

    status = pv_layer_norm_forward(object->layer_norm_attention, n, buffer_1, buffer_1);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    float *buffer_2 = pv_buffer_get(object->buffer_text_encoder_transf_attn_2, n, false);
    if (!buffer_2) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    status = pv_transformer_ffn_forward(object->ffn, n, buffer_1, buffer_2);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    for (int32_t i = 0; i < (num_channels * n); i++) {
        buffer_2[i] = buffer_1[i] + buffer_2[i];
    }

    status = pv_layer_norm_forward(object->layer_norm_ffn, n, buffer_2, buffer_2);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    memcpy(y, buffer_2, n * num_channels * sizeof(float));

    PV_ORCA_PROFILER_STOP("transformer");
    return PV_STATUS_SUCCESS;
}
