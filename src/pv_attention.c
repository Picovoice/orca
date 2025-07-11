#include <stdlib.h>
#include <string.h>

#include "math/pv_math.h"
#include "orca/pv_attention.h"
#include "orca/pv_buffer.h"
#include "orca/pv_orca_util.h"
#include "util/pv_check_status.h"
#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

static float ATTENTION_NEGATIVE_INFINITY_FLOAT = -1e6f;


#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_attention_param_serialize)(const pv_attention_param_t *param, FILE *file) {
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

    pv_status_t status = pv_embed_param_serialize(param->emb_rel_k_param, file);
    PV_CHECK_STATUS(status);

    status = pv_embed_param_serialize(param->emb_rel_v_param, file);
    PV_CHECK_STATUS(status);

    status = pv_cnn_param_serialize(param->q_param, file);
    PV_CHECK_STATUS(status);

    status = pv_cnn_param_serialize(param->k_param, file);
    PV_CHECK_STATUS(status);

    status = pv_cnn_param_serialize(param->v_param, file);
    PV_CHECK_STATUS(status);

    status = pv_cnn_param_serialize(param->o_param, file);
    PV_CHECK_STATUS(status);

    return PV_STATUS_SUCCESS;
}

#endif

pv_status_t PV_MOCKABLE(pv_attention_param_load)(FILE *f, pv_attention_param_t **param) {
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_attention_param_t *p = calloc(1, sizeof(pv_attention_param_t));
    PV_CHECK_ALLOC(p);

    size_t count = pv_fread(&(p->num_channels), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_attention_param_delete(p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->num_channels <= 0) {
        pv_attention_param_delete(p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    count = pv_fread(&(p->num_heads), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_attention_param_delete(p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->num_heads <= 0) {
        pv_attention_param_delete(p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    count = pv_fread(&(p->pos_embed_window_size), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_attention_param_delete(p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->pos_embed_window_size <= 0) {
        pv_attention_param_delete(p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    count = pv_fread(&(p->attention_window_future), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_attention_param_delete(p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->attention_window_future < 0) {
        pv_attention_param_delete(p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    pv_status_t status = pv_embed_param_load(f, (pv_embed_param_t **) &(p->emb_rel_k_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_attention_param_delete(p);
        return status;
    }

    status = pv_embed_param_load(f, (pv_embed_param_t **) &(p->emb_rel_v_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_attention_param_delete(p);
        return status;
    }

    status = pv_cnn_param_load(f, (pv_cnn_param_t **) &(p->q_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_attention_param_delete(p);
        return status;
    }

    status = pv_cnn_param_load(f, (pv_cnn_param_t **) &(p->k_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_attention_param_delete(p);
        return status;
    }

    status = pv_cnn_param_load(f, (pv_cnn_param_t **) &(p->v_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_attention_param_delete(p);
        return status;
    }

    status = pv_cnn_param_load(f, (pv_cnn_param_t **) &(p->o_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_attention_param_delete(p);
        return status;
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}


void PV_MOCKABLE(pv_attention_param_delete)(pv_attention_param_t *param) {
    if (param) {
        pv_cnn_param_delete((pv_cnn_param_t *) (param->o_param));

        pv_cnn_param_delete((pv_cnn_param_t *) (param->v_param));

        pv_cnn_param_delete((pv_cnn_param_t *) (param->k_param));

        pv_cnn_param_delete((pv_cnn_param_t *) (param->q_param));

        pv_embed_param_delete((pv_embed_param_t *) (param->emb_rel_v_param));

        pv_embed_param_delete((pv_embed_param_t *) (param->emb_rel_k_param));

        free(param);
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
    float *pos_enc_k;
    float *pos_enc_v;

    pv_cnn_t *q;
    pv_cnn_t *k;
    pv_cnn_t *v;
    pv_cnn_t *o;

    pv_buffer_t *buffer_text_encoder_transf_attn_1;
    pv_buffer_t *buffer_text_encoder_transf_attn_2;
    pv_buffer_t *buffer_text_encoder_transf_attn_score;
};

pv_status_t PV_MOCKABLE(pv_attention_encoding_init)(const pv_embed_t *embed_object, float **encoding) {
    PV_ASSERT(embed_object);
    PV_ASSERT(encoding);

    *encoding = NULL;

    int32_t num_embeddings = pv_embed_num_embeddings(embed_object);
    int32_t num_channels_head = pv_embed_dimension(embed_object);
    float *e = calloc(num_embeddings * num_channels_head, sizeof(float));
    if (!e) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    for (int32_t i = 0; i < num_embeddings; i++) {
        pv_embed_get(embed_object, i, e + (i * num_channels_head));
    }

    *encoding = e;

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_attention_init)(
        const pv_attention_param_t *param,
        pv_buffer_t *buffer_text_encoder_transf_attn_1,
        pv_buffer_t *buffer_text_encoder_transf_attn_2,
        pv_buffer_t *buffer_text_encoder_transf_attn_score,
        pv_attention_t **object) {
    PV_ASSERT(param);
    PV_ASSERT(object);

    *object = NULL;

    pv_attention_t *o = calloc(1, sizeof(pv_attention_t));
    if (!o) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->param = param;
    o->buffer_text_encoder_transf_attn_1 = buffer_text_encoder_transf_attn_1;
    o->buffer_text_encoder_transf_attn_2 = buffer_text_encoder_transf_attn_2;
    o->buffer_text_encoder_transf_attn_score = buffer_text_encoder_transf_attn_score;

    pv_status_t status = pv_embed_init(param->emb_rel_k_param, &(o->emb_rel_k));
    if (status != PV_STATUS_SUCCESS) {
        pv_attention_delete(o);
        return status;
    }

    status = pv_embed_init(param->emb_rel_v_param, &(o->emb_rel_v));
    if (status != PV_STATUS_SUCCESS) {
        pv_attention_delete(o);
        return status;
    }

    status = pv_attention_encoding_init(o->emb_rel_k, &(o->pos_enc_k));
    if (status != PV_STATUS_SUCCESS) {
        pv_attention_delete(o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    status = pv_attention_encoding_init(o->emb_rel_v, &(o->pos_enc_v));
    if (status != PV_STATUS_SUCCESS) {
        pv_attention_delete(o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    status = pv_cnn_init(param->q_param, &(o->q));
    if (status != PV_STATUS_SUCCESS) {
        pv_attention_delete(o);
        return status;
    }

    status = pv_cnn_init(param->k_param, &(o->k));
    if (status != PV_STATUS_SUCCESS) {
        pv_attention_delete(o);
        return status;
    }

    status = pv_cnn_init(param->v_param, &(o->v));
    if (status != PV_STATUS_SUCCESS) {
        pv_attention_delete(o);
        return status;
    }

    status = pv_cnn_init(param->o_param, &(o->o));
    if (status != PV_STATUS_SUCCESS) {
        pv_attention_delete(o);
        return status;
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_attention_delete)(pv_attention_t *object) {
    if (object) {
        pv_cnn_delete(object->o);
        pv_cnn_delete(object->v);
        pv_cnn_delete(object->k);
        pv_cnn_delete(object->q);

        free(object->pos_enc_k);
        free(object->pos_enc_v);
        pv_embed_delete(object->emb_rel_v);
        pv_embed_delete(object->emb_rel_k);

        free(object);
    }
}

static void pv_attention_scores(
        pv_attention_t *object,
        int32_t n,
        const float *q,
        const float *k,
        float *y) {
    PV_ASSERT(object);
    PV_ASSERT(n > 0);
    PV_ASSERT(q);
    PV_ASSERT(k);
    PV_ASSERT(y);

    int32_t num_heads = object->param->num_heads;
    int32_t num_channels = object->param->num_channels;
    int32_t num_channels_head = num_channels / num_heads;
    float norm = 1.0f / sqrtf((float) num_channels_head);

    int32_t window_size = object->param->pos_embed_window_size;
    for (int32_t iq = 0; iq < n; iq++) {
        for (int32_t ik = 0; ik < n; ik++) {
            int32_t index_embed = window_size + pv_orca_clip_int32(ik - iq, -window_size, window_size);
            float *pos_embed = &(object->pos_enc_k[index_embed * num_channels_head]);
            const int32_t is_in_window = abs(ik - iq) <= window_size;

            for (int32_t ih = 0; ih < num_heads; ih++) {
                const int32_t index_q_offset = (iq * num_channels) + (ih * num_channels_head);
                const int32_t index_k_offset = (ik * num_channels) + (ih * num_channels_head);

                float sum = 0.f;
                for (int32_t ch = 0; ch < num_channels_head; ch++) {

                    float key_plus_pos_embed;
                    if (is_in_window) {
                        key_plus_pos_embed = k[index_k_offset + ch] + pos_embed[ch];
                    } else {
                        // TODO (Ben): Use first (and last) index of positional embedding instead of 0?
                        key_plus_pos_embed = k[index_k_offset + ch];
                    }
                    sum += (q[index_q_offset + ch] * key_plus_pos_embed);
                }
                y[(ih * n * n) + (iq * n) + ik] = sum * norm;
            }
        }
    }
}

static void pv_attention_softmax(int32_t n, float *x, float *y) {
    PV_ASSERT(x);
    PV_ASSERT(n > 0);
    PV_ASSERT(y);

    int32_t max_index = 0;
    for (int32_t i = 1; i < n; i++) {
        if (x[i] > x[max_index]) {
            max_index = i;
        }
    }
    const float max = x[max_index];

    float sum = 0.f;
    for (int32_t i = 0; i < n; i++) {
        y[i] = expf(x[i] - max);
        sum += y[i];
    }

    const float scale = 1.f / sum;
    for (int32_t i = 0; i < n; i++) {
        y[i] = y[i] * scale;
    }
}

static void pv_attention_softmax_scores(pv_attention_t *object, int32_t n, float *scores) {
    PV_ASSERT(object);
    PV_ASSERT(scores);
    PV_ASSERT(n > 0);

    const int32_t num_heads = object->param->num_heads;

    const int32_t inner_step = n * n;
    for (int32_t i = 0; i < n; i++) {
        const int32_t offset = i * n;
        for (int32_t h = 0; h < num_heads; h++) {
            float *score_vector = scores + (h * inner_step) + offset;
            pv_attention_softmax(n, score_vector, score_vector);
        }
    }
}

static void pv_attention_scores_times_value(
        pv_attention_t *object,
        int32_t n,
        const float *scores,
        const float *value,
        float *y) {
    PV_ASSERT(object);
    PV_ASSERT(n > 0);
    PV_ASSERT(scores);
    PV_ASSERT(value);
    PV_ASSERT(y);

    int32_t num_heads = object->param->num_heads;
    int32_t num_channels = object->param->num_channels;
    int32_t num_channels_head = num_channels / num_heads;
    int32_t window_size = object->param->pos_embed_window_size;
    memset(y, 0, n * num_channels * sizeof(float));

    const int32_t inner_step = n * n;
    for (int32_t iq = 0; iq < n; iq++) {
        const int32_t query_offset = iq * n;
        for (int32_t ik = 0; ik < n; ik++) {

            int32_t index_embed = window_size + pv_orca_clip_int32(ik - iq, -window_size, window_size);
            float *pos_embed = &(object->pos_enc_v[index_embed * num_channels_head]);
            const int32_t is_in_window = abs(ik - iq) <= window_size;

            for (int32_t ih = 0; ih < num_heads; ih++) {
                const int32_t index_score = (ih * inner_step) + query_offset + ik;
                const int32_t index_value_offset = (ik * num_channels) + (ih * num_channels_head);
                const int32_t index_offset = (iq * num_channels) + (ih * num_channels_head);

                for (int32_t ch = 0; ch < num_channels_head; ch++) {
                    float pos_enc_value;

                    if (is_in_window) {
                        pos_enc_value = value[index_value_offset + ch] + pos_embed[ch];
                    } else {
                        pos_enc_value = value[index_value_offset + ch];
                    }

                    y[index_offset + ch] = y[index_offset + ch] + (scores[index_score] * pos_enc_value);

                }
            }
        }
    }
}

static void pv_attention_mask_future(
        int32_t attention_window_future,
        int32_t n,
        int32_t num_heads,
        float *scores) {
    PV_ASSERT(attention_window_future >= 0);
    PV_ASSERT(n > 0);
    PV_ASSERT(num_heads > 0);
    PV_ASSERT(scores);

    const int32_t inner_step = n * n;
    for (int32_t iq = 0; iq < n; iq++) {
        const int32_t query_offset = iq * n;

        for (int32_t ik = 0; ik < n; ik++) {

            if ((ik - iq) > attention_window_future) {

                for (int32_t ih = 0; ih < num_heads; ih++) {
                    const int32_t index_score = (ih * inner_step) + query_offset + ik;
                    scores[index_score] = ATTENTION_NEGATIVE_INFINITY_FLOAT;
                }

            }
        }
    }
}

pv_status_t PV_MOCKABLE(pv_attention_forward)(
        pv_attention_t *object,
        int32_t n,
        const float *query_input,
        const float *key_and_value_input,
        float *y) {
    PV_ASSERT(object);
    PV_ASSERT(n);
    PV_ASSERT(query_input);
    PV_ASSERT(key_and_value_input);
    PV_ASSERT(y);

    float *buffer_q = pv_buffer_get(object->buffer_text_encoder_transf_attn_1, n, false);
    if (!buffer_q) {
        return PV_STATUS_OUT_OF_MEMORY;
    }
    pv_status_t status = pv_cnn_forward(object->q, n, query_input, buffer_q);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    float *buffer_k = pv_buffer_get(object->buffer_text_encoder_transf_attn_2, n, false);
    if (!buffer_k) {
        return PV_STATUS_OUT_OF_MEMORY;
    }
    status = pv_cnn_forward(object->k, n, key_and_value_input, buffer_k);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    float *buffer_score = pv_buffer_get(object->buffer_text_encoder_transf_attn_score, n * n, false);
    if (!buffer_score) {
        return PV_STATUS_OUT_OF_MEMORY;
    }
    pv_attention_scores(object, n, buffer_q, buffer_k, buffer_score);

    if (object->param->attention_window_future >= 0) {
        pv_attention_mask_future(
                object->param->attention_window_future,
                n,
                object->param->num_heads,
                buffer_score);
    }

    pv_attention_softmax_scores(object, n, buffer_score);

    float *buffer_v = pv_buffer_get(object->buffer_text_encoder_transf_attn_1, n, false);
    if (!buffer_v) {
        return PV_STATUS_OUT_OF_MEMORY;
    }
    status = pv_cnn_forward(object->v, n, key_and_value_input, buffer_v);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    float *buffer_o = pv_buffer_get(object->buffer_text_encoder_transf_attn_2, n, true);
    if (!buffer_o) {
        return PV_STATUS_OUT_OF_MEMORY;
    }
    pv_attention_scores_times_value(object, n, buffer_score, buffer_v, buffer_o);

    status = pv_cnn_forward(object->o, n, buffer_o, y);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}
