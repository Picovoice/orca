#include <stdlib.h>
#include <string.h>

#include "orca/pv_buffer.h"
#include "orca/pv_orca_text_encoder.h"
#include "orca/pv_orca_util.h"
#include "orca/pv_profiler.h"
#include "util/pv_check_status.h"
#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif


#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_orca_text_encoder_param_serialize)(const pv_orca_text_encoder_param_t *param, FILE *file) {
    PV_ASSERT(param);
    PV_ASSERT(file);

    pv_status_t status = pv_embed_param_serialize(param->embed_param, file);
    PV_CHECK_STATUS(status);

    status = pv_cnn_param_serialize(param->conv_post_param, file);
    PV_CHECK_STATUS(status);

    size_t count = fwrite(&(param->num_transformers), sizeof(int32_t), 1, file);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

    for (int32_t i = 0; i < param->num_transformers; i++) {
        status = pv_transformer_param_serialize(param->transformers_param[i], file);
        if (status != PV_STATUS_SUCCESS) {
            return status;
        }
    }

    return PV_STATUS_SUCCESS;
}

#endif

pv_status_t PV_MOCKABLE(pv_orca_text_encoder_param_load)(FILE *f, pv_orca_text_encoder_param_t **param) {
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_orca_text_encoder_param_t *p = calloc(1, sizeof(pv_orca_text_encoder_param_t));
    PV_CHECK_ALLOC(p);

    pv_status_t status = pv_embed_param_load(f, (pv_embed_param_t **) &(p->embed_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_text_encoder_param_delete(p);
        return status;
    }

    status = pv_cnn_param_load(f, (pv_cnn_param_t **) &(p->conv_post_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_text_encoder_param_delete(p);
        return status;
    }

    size_t count = pv_fread(&(p->num_transformers), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_orca_text_encoder_param_delete(p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->num_transformers <= 0) {
        pv_orca_text_encoder_param_delete(p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    p->transformers_param = calloc(p->num_transformers, sizeof(pv_transformer_param_t *));
    if (!(p->transformers_param)) {
        pv_orca_text_encoder_param_delete(p);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    for (int32_t i = 0; i < p->num_transformers; i++) {
        status = pv_transformer_param_load(f, (pv_transformer_param_t **) &(p->transformers_param[i]));
        if (status != PV_STATUS_SUCCESS) {
            pv_orca_text_encoder_param_delete(p);
            return status;
        }
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}


void PV_MOCKABLE(pv_orca_text_encoder_param_delete)(pv_orca_text_encoder_param_t *param) {
    if (param) {
        if (param->transformers_param) {
            for (int32_t i = param->num_transformers - 1; i >= 0; i--) {
                pv_transformer_param_delete((pv_transformer_param_t *) (param->transformers_param[i]));
            }

            free((pv_transformer_param_t **) (param->transformers_param));
        }

        pv_cnn_param_delete((pv_cnn_param_t *) (param->conv_post_param));
        pv_embed_param_delete((pv_embed_param_t *) (param->embed_param));

        free(param);
    }
}

bool PV_MOCKABLE(pv_orca_text_encoder_param_is_equal)(
        const pv_orca_text_encoder_param_t *object,
        const pv_orca_text_encoder_param_t *other) {
    PV_ASSERT(object);
    PV_ASSERT(other);

    if (!pv_embed_param_is_equal(object->embed_param, other->embed_param)) {
        return false;
    }

    if (!pv_cnn_param_is_equal(object->conv_post_param, other->conv_post_param)) {
        return false;
    }

    if (object->num_transformers != other->num_transformers) {
        return false;
    }

    for (int32_t i = 0; i < object->num_transformers; i++) {
        if (!pv_transformer_param_is_equal(object->transformers_param[i], other->transformers_param[i])) {
            return false;
        }
    }

    return true;
}

int32_t PV_MOCKABLE(pv_orca_text_encoder_param_receptive_field)(const pv_orca_text_encoder_param_t *object) {
    PV_ASSERT(object);

    int32_t num_layers = object->num_transformers;

    int32_t attention_contribution = 0;
    for (int32_t i = 0; i < num_layers; i++) {
        attention_contribution += pv_attention_param_receptive_field(object->transformers_param[i]->attention_param);
    }

    int32_t feed_forward_contribution = 0;
    for (int32_t i = 0; i < num_layers; i++) {
        feed_forward_contribution += pv_transformer_ffn_param_receptive_field(object->transformers_param[i]->ffn_param);
    }

    return attention_contribution + feed_forward_contribution;
}

struct pv_orca_text_encoder {
    const pv_orca_text_encoder_param_t *param; // maybe not necessary?

    pv_embed_t *embed;
    pv_cnn_t *conv_post;
    pv_transformer_t **transformers;

    int32_t num_hidden_channels;

    pv_buffer_t *buffer_text_encoder_transf_attn_1;
    pv_buffer_t *buffer_text_encoder_transf_attn_2;
    pv_buffer_t *buffer_text_encoder_transf_attn_score;
    pv_buffer_t *buffer_text_encoder_transf_ffn;
    pv_buffer_t *buffer_stats;
};

pv_status_t PV_MOCKABLE(pv_orca_text_encoder_init)(
        const pv_orca_text_encoder_param_t *param,
        pv_orca_text_encoder_t **object) {
    PV_ASSERT(param);
    PV_ASSERT(object);

    *object = NULL;

    pv_orca_text_encoder_t *o = calloc(1, sizeof(pv_orca_text_encoder_t));
    if (!o) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->param = param;

    pv_status_t status = pv_embed_init(param->embed_param, &(o->embed));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_text_encoder_delete(o);
        return status;
    }

    status = pv_cnn_init(param->conv_post_param, &(o->conv_post));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_text_encoder_delete(o);
        return status;
    }

    o->transformers = calloc(param->num_transformers, sizeof(pv_transformer_t *));
    if (!(o->transformers)) {
        pv_orca_text_encoder_delete(o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->num_hidden_channels = o->param->embed_param->output_channels;

    PV_ASSERT(o->num_hidden_channels == pv_cnn_input_channels(o->conv_post));
    PV_ASSERT(o->num_hidden_channels == param->transformers_param[0]->attention_param->num_channels);

    status = pv_buffer_init(o->num_hidden_channels, &(o->buffer_text_encoder_transf_attn_1));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_text_encoder_delete(o);
        return status;
    }

    status = pv_buffer_init(o->num_hidden_channels, &(o->buffer_text_encoder_transf_attn_2));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_text_encoder_delete(o);
        return status;
    }

    int32_t num_heads = param->transformers_param[0]->attention_param->num_heads;
    status = pv_buffer_init(num_heads, &(o->buffer_text_encoder_transf_attn_score));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_text_encoder_delete(o);
        return status;
    }


    int32_t num_ffn_channels = param->transformers_param[0]->ffn_param->conv_1_param->output_channels;
    status = pv_buffer_init(num_ffn_channels, &(o->buffer_text_encoder_transf_ffn));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_text_encoder_delete(o);
        return status;
    }

    for (int32_t i = 0; i < param->num_transformers; i++) {
        PV_ASSERT(num_heads == param->transformers_param[i]->attention_param->num_heads);
        PV_ASSERT(o->num_hidden_channels == param->transformers_param[i]->attention_param->num_channels);
        status = pv_transformer_init(
                param->transformers_param[i],
                o->buffer_text_encoder_transf_attn_1,
                o->buffer_text_encoder_transf_attn_2,
                o->buffer_text_encoder_transf_attn_score,
                o->buffer_text_encoder_transf_ffn,
                &(o->transformers[i]));
        if (status != PV_STATUS_SUCCESS) {
            pv_orca_text_encoder_delete(o);
            return status;
        }
    }

    int32_t output_channels = pv_cnn_output_channels(o->conv_post);
    PV_ASSERT(output_channels % 2 == 0);
    status = pv_buffer_init(output_channels, &(o->buffer_stats));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_text_encoder_delete(o);
        return status;
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_orca_text_encoder_delete)(pv_orca_text_encoder_t *object) {
    if (object) {
        pv_buffer_delete(object->buffer_stats);
        pv_buffer_delete(object->buffer_text_encoder_transf_ffn);
        pv_buffer_delete(object->buffer_text_encoder_transf_attn_score);
        pv_buffer_delete(object->buffer_text_encoder_transf_attn_2);
        pv_buffer_delete(object->buffer_text_encoder_transf_attn_1);

        if (object->transformers) {
            for (int32_t i = object->param->num_transformers - 1; i >= 0; i--) {
                pv_transformer_delete(object->transformers[i]);
            }
            free(object->transformers);
        }

        pv_cnn_delete(object->conv_post);
        pv_embed_delete(object->embed);

        free(object);
    }
}

int32_t PV_MOCKABLE(pv_orca_text_encoder_output_channels)(const pv_orca_text_encoder_t *object) {
    PV_ASSERT(object);

    return pv_cnn_output_channels(object->conv_post) / 2;
}

pv_status_t PV_MOCKABLE(pv_orca_text_encoder_forward)(
        pv_orca_text_encoder_t *object,
        int32_t num_tokens,
        const int32_t *tokens,
        float *encoded_tokens,
        float *means,
        float *logs) {
    PV_ASSERT(object);
    PV_ASSERT(num_tokens > 0);
    PV_ASSERT(tokens);
    PV_ASSERT(encoded_tokens);
    PV_ASSERT(means);
    PV_ASSERT(logs);
    PV_ORCA_PROFILER_START("text_encoder");

    pv_status_t status = pv_embed_forward(object->embed, num_tokens, tokens, encoded_tokens);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    const int32_t num_transformers = object->param->num_transformers;
    for (int32_t i = 0; i < num_transformers; i++) {
        status = pv_transformer_forward(object->transformers[i], num_tokens, encoded_tokens, encoded_tokens);
        if (status != PV_STATUS_SUCCESS) {
            return status;
        }
    }
    pv_buffer_free(object->buffer_text_encoder_transf_attn_1);
    pv_buffer_free(object->buffer_text_encoder_transf_attn_2);
    pv_buffer_free(object->buffer_text_encoder_transf_attn_score);
    pv_buffer_free(object->buffer_text_encoder_transf_ffn);

    float *buffer_stats = pv_buffer_get(object->buffer_stats, num_tokens, false);
    if (!buffer_stats) {
        return PV_STATUS_OUT_OF_MEMORY;
    }
    status = pv_cnn_forward(object->conv_post, num_tokens, encoded_tokens, buffer_stats);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    int32_t output_channels = pv_cnn_output_channels(object->conv_post);
    pv_orca_util_split_channels(num_tokens, output_channels, buffer_stats, means, logs);

    pv_buffer_free(object->buffer_stats);

    PV_ORCA_PROFILER_STOP("text_encoder");

    return PV_STATUS_SUCCESS;
}
