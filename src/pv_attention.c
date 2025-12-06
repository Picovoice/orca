#include <stdlib.h>
#include <string.h>

#include "math/pv_math.h"
#include "orca/pv_attention.h"
#include "orca/pv_orca_util.h"
#include "util/pv_check_status.h"
#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_attention_param_serialize)(
        pv_ypu_t *ypu,
        const pv_attention_param_t *param,
        FILE *file) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(file);

    size_t count = fwrite(&(param->num_channels), sizeof(int32_t), 1, file);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

    count = fwrite(&(param->num_heads), sizeof(int32_t), 1, file);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

    count = fwrite(&(param->pos_embed_window_size), sizeof(int32_t), 1, file);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

    count = fwrite(&(param->attention_window_future), sizeof(int32_t), 1, file);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

    pv_status_t status = pv_embed_param_serialize(ypu, param->emb_rel_k_param, file);
    PV_CHECK_STATUS(status);

    status = pv_embed_param_serialize(ypu, param->emb_rel_v_param, file);
    PV_CHECK_STATUS(status);

    status = pv_cnn_param_serialize(ypu, param->q_param, file);
    PV_CHECK_STATUS(status);

    status = pv_cnn_param_serialize(ypu, param->k_param, file);
    PV_CHECK_STATUS(status);

    status = pv_cnn_param_serialize(ypu, param->v_param, file);
    PV_CHECK_STATUS(status);

    status = pv_cnn_param_serialize(ypu, param->o_param, file);
    PV_CHECK_STATUS(status);

    return PV_STATUS_SUCCESS;
}

#endif

pv_status_t PV_MOCKABLE(pv_attention_param_load)(
        pv_ypu_t *ypu,
        FILE *f,
        pv_attention_param_t **param) {
    PV_ASSERT(ypu);
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_attention_param_t *p = pv_ypu_host_alloc(ypu, sizeof(pv_attention_param_t));
    PV_CHECK_ALLOC(p);

    memset(p, 0, sizeof(pv_attention_param_t));

    size_t count = pv_fread(&(p->num_channels), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_attention_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->num_channels <= 0) {
        pv_attention_param_delete(ypu, p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    count = pv_fread(&(p->num_heads), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_attention_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->num_heads <= 0) {
        pv_attention_param_delete(ypu, p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    count = pv_fread(&(p->pos_embed_window_size), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_attention_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->pos_embed_window_size <= 0) {
        pv_attention_param_delete(ypu, p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    count = pv_fread(&(p->attention_window_future), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_attention_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->attention_window_future < 0) {
        pv_attention_param_delete(ypu, p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    pv_status_t status = pv_embed_param_load(ypu, f, (pv_embed_param_t **) &(p->emb_rel_k_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_attention_param_delete(ypu, p);
        return status;
    }

    status = pv_embed_param_load(ypu, f, (pv_embed_param_t **) &(p->emb_rel_v_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_attention_param_delete(ypu, p);
        return status;
    }

    status = pv_cnn_param_load(ypu, f, (pv_cnn_param_t **) &(p->q_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_attention_param_delete(ypu, p);
        return status;
    }

    status = pv_cnn_param_load(ypu, f, (pv_cnn_param_t **) &(p->k_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_attention_param_delete(ypu, p);
        return status;
    }

    status = pv_cnn_param_load(ypu, f, (pv_cnn_param_t **) &(p->v_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_attention_param_delete(ypu, p);
        return status;
    }

    status = pv_cnn_param_load(ypu, f, (pv_cnn_param_t **) &(p->o_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_attention_param_delete(ypu, p);
        return status;
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}


void PV_MOCKABLE(pv_attention_param_delete)(
        pv_ypu_t *ypu,
        pv_attention_param_t *param) {
    PV_ASSERT(ypu);

    if (param) {
        pv_cnn_param_delete(ypu, (pv_cnn_param_t *) (param->o_param));

        pv_cnn_param_delete(ypu, (pv_cnn_param_t *) (param->v_param));

        pv_cnn_param_delete(ypu, (pv_cnn_param_t *) (param->k_param));

        pv_cnn_param_delete(ypu, (pv_cnn_param_t *) (param->q_param));

        pv_embed_param_delete(ypu, (pv_embed_param_t *) (param->emb_rel_v_param));

        pv_embed_param_delete(ypu, (pv_embed_param_t *) (param->emb_rel_k_param));

        pv_ypu_host_free(ypu, param);
    }
}

bool PV_MOCKABLE(pv_attention_param_is_equal)(
        const pv_attention_param_t *object,
        const pv_attention_param_t *other) {
    PV_ASSERT(object);
    PV_ASSERT(other);

    if (object->num_channels != other->num_channels) {
        return false;
    }

    if (object->num_heads != other->num_heads) {
        return false;
    }

    if (object->pos_embed_window_size != other->pos_embed_window_size) {
        return false;
    }

    if (object->attention_window_future != other->attention_window_future) {
        return false;
    }

    if (!pv_embed_param_is_equal(object->emb_rel_k_param, other->emb_rel_k_param)) {
        return false;
    }

    if (!pv_embed_param_is_equal(object->emb_rel_v_param, other->emb_rel_v_param)) {
        return false;
    }

    if (!pv_cnn_param_is_equal(object->q_param, other->q_param)) {
        return false;
    }

    if (!pv_cnn_param_is_equal(object->k_param, other->k_param)) {
        return false;
    }

    if (!pv_cnn_param_is_equal(object->v_param, other->v_param)) {
        return false;
    }

    if (!pv_cnn_param_is_equal(object->o_param, other->o_param)) {
        return false;
    }

    return true;
}

int32_t PV_MOCKABLE(pv_attention_param_receptive_field)(const pv_attention_param_t *param) {
    PV_ASSERT(param);

    return param->attention_window_future;
}

/**
 * The orca attention mechanism uses self-attention with relative position embeddings
 */
struct pv_attention {
    const pv_attention_param_t *param;

    pv_embed_t *emb_rel_k;
    pv_embed_t *emb_rel_v;
    pv_ypu_mem_t *pos_enc_k;
    pv_ypu_mem_t *pos_enc_v;

    pv_cnn_t *q;
    pv_cnn_t *k;
    pv_cnn_t *v;
    pv_cnn_t *o;
};

pv_status_t PV_MOCKABLE(pv_attention_encoding_init)(
        pv_ypu_t *ypu,
        const pv_embed_t *embed_object,
        pv_ypu_mem_t **encoding) {
    PV_ASSERT(ypu);
    PV_ASSERT(embed_object);
    PV_ASSERT(encoding);

    *encoding = NULL;

    int32_t num_embeddings = pv_embed_num_embeddings(embed_object);
    int32_t num_channels_head = pv_embed_dimension(embed_object);
    pv_ypu_mem_t *e_ypu_mem = pv_ypu_mem_alloc(
            ypu,
            num_embeddings * num_channels_head * (int32_t) sizeof(float),
            PV_YPU_DEVICE_MEM_FLAG_NONE);
    if (!e_ypu_mem) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    for (int32_t i = 0; i < num_embeddings; i++) {
        pv_embed_get(ypu, embed_object, i, e_ypu_mem, (i * num_channels_head) * (int32_t) sizeof(float));
    }

    *encoding = e_ypu_mem;

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_attention_init)(
        pv_ypu_t *ypu,
        const pv_attention_param_t *param,
        pv_attention_t **object) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(object);

    *object = NULL;

    pv_attention_t *o = pv_ypu_host_alloc(ypu, sizeof(pv_attention_t));
    if (!o) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(o, 0, sizeof(pv_attention_t));

    o->param = param;

    pv_status_t status = pv_embed_init(
            ypu,
            param->emb_rel_k_param,
            &(o->emb_rel_k));
    if (status != PV_STATUS_SUCCESS) {
        pv_attention_delete(ypu, o);
        return status;
    }

    status = pv_embed_init(
            ypu,
            param->emb_rel_v_param,
            &(o->emb_rel_v));
    if (status != PV_STATUS_SUCCESS) {
        pv_attention_delete(ypu, o);
        return status;
    }

    status = pv_attention_encoding_init(
            ypu,
            o->emb_rel_k,
            &(o->pos_enc_k));
    if (status != PV_STATUS_SUCCESS) {
        pv_attention_delete(ypu, o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    status = pv_attention_encoding_init(
            ypu,
            o->emb_rel_v,
            &(o->pos_enc_v));
    if (status != PV_STATUS_SUCCESS) {
        pv_attention_delete(ypu, o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    status = pv_cnn_init(ypu, param->q_param, &(o->q));
    if (status != PV_STATUS_SUCCESS) {
        pv_attention_delete(ypu, o);
        return status;
    }

    status = pv_cnn_init(ypu, param->k_param, &(o->k));
    if (status != PV_STATUS_SUCCESS) {
        pv_attention_delete(ypu, o);
        return status;
    }

    status = pv_cnn_init(ypu, param->v_param, &(o->v));
    if (status != PV_STATUS_SUCCESS) {
        pv_attention_delete(ypu, o);
        return status;
    }

    status = pv_cnn_init(ypu, param->o_param, &(o->o));
    if (status != PV_STATUS_SUCCESS) {
        pv_attention_delete(ypu, o);
        return status;
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_attention_delete)(
        pv_ypu_t *ypu,
        pv_attention_t *object) {
    PV_ASSERT(ypu);

    if (object) {
        pv_cnn_delete(ypu, object->o);
        pv_cnn_delete(ypu, object->v);
        pv_cnn_delete(ypu, object->k);
        pv_cnn_delete(ypu, object->q);

        pv_ypu_mem_free(ypu, object->pos_enc_k);
        pv_ypu_mem_free(ypu, object->pos_enc_v);
        pv_embed_delete(ypu, object->emb_rel_v);
        pv_embed_delete(ypu, object->emb_rel_k);

        pv_ypu_host_free(ypu, object);
    }
}

pv_status_t PV_MOCKABLE(pv_attention_forward)(
        pv_ypu_t *ypu,
        pv_attention_t *object,
        int32_t n,
        pv_ypu_mem_t *query_ypu_mem,
        pv_ypu_mem_t *key_and_value_ypu_mem,
        pv_ypu_mem_t *y,
        int32_t query_offset,
        int32_t key_and_value_offset,
        int32_t y_offset) {
    PV_ASSERT(ypu);
    PV_ASSERT(object);
    PV_ASSERT(n);
    PV_ASSERT(query_ypu_mem);
    PV_ASSERT(key_and_value_ypu_mem);
    PV_ASSERT(y);

    int32_t num_heads = object->param->num_heads;
    int32_t num_channels = object->param->num_channels;
    int32_t window_size = object->param->pos_embed_window_size;
    int32_t window_future = object->param->attention_window_future;

    pv_ypu_mem_t *buffer_q = pv_ypu_buffer_get(
            ypu,
            num_channels * n * (int32_t) sizeof(float),
            false);
    if (!buffer_q) {
        return PV_STATUS_OUT_OF_MEMORY;
    }
    pv_status_t status = pv_cnn_forward(ypu, object->q, n, query_ypu_mem, buffer_q, query_offset, 0);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_mem_t *buffer_k = pv_ypu_buffer_get(
            ypu,
            num_channels * n * (int32_t) sizeof(float),
            false);
    if (!buffer_k) {
        return PV_STATUS_OUT_OF_MEMORY;
    }
    status = pv_cnn_forward(ypu, object->k, n, key_and_value_ypu_mem, buffer_k, key_and_value_offset, 0);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_mem_t *buffer_v = pv_ypu_buffer_get(
            ypu,
            num_channels * n * (int32_t) sizeof(float),
            false);
    if (!buffer_v) {
        return PV_STATUS_OUT_OF_MEMORY;
    }
    status = pv_cnn_forward(ypu, object->v, n, key_and_value_ypu_mem, buffer_v, key_and_value_offset, 0);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_mem_t *buffer_o = pv_ypu_buffer_get(
            ypu,
            num_channels * n * (int32_t) sizeof(float),
            false);
    if (!buffer_o) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_mem_t *buffer_score = pv_ypu_buffer_get(
            ypu,
            num_heads * n * n * (int32_t) sizeof(float),
            false);
    if (!buffer_score) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_op_attention_args_t args = {
            .output = buffer_o,
            .query = buffer_q,
            .keys = buffer_k,
            .values = buffer_v,
            .pos_enc_k = object->pos_enc_k,
            .pos_enc_v = object->pos_enc_v,
            .scores = buffer_score,
            .n = n,
            .num_heads = num_heads,
            .num_channels = num_channels,
            .window_size = window_size,
            .window_future = window_future,
            .output_offset = 0,
            .query_offset = 0,
            .keys_offset = 0,
            .values_offset = 0,
            .pos_enc_k_offset = 0,
            .pos_enc_v_offset = 0,
            .scores_offset = 0,
    };

    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_ATTENTION,
            &args);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_buffer_release(ypu, buffer_score);

    status = pv_cnn_forward(ypu, object->o, n, buffer_o, y, 0, y_offset);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_buffer_release(ypu, buffer_o);
    pv_ypu_buffer_release(ypu, buffer_v);
    pv_ypu_buffer_release(ypu, buffer_k);
    pv_ypu_buffer_release(ypu, buffer_q);

    return PV_STATUS_SUCCESS;
}
