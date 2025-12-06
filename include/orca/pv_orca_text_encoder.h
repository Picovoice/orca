#ifndef PV_ORCA_TEXT_ENCODER_H
#define PV_ORCA_TEXT_ENCODER_H

#include <stdint.h>

#include "orca/pv_cnn.h"
#include "orca/pv_embed.h"
#include "orca/pv_transformer.h"
#include "util/pv_file.h"
#include "ypu/pv_ypu.h"

typedef struct {
    const pv_embed_param_t *embed_param;
    const pv_cnn_param_t *conv_post_param;
    int32_t num_transformers;
    int32_t attention_window_future;
    const pv_transformer_param_t **transformers_param;
} pv_orca_text_encoder_param_t;

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_orca_text_encoder_param_serialize)(
        pv_ypu_t *ypu,
        const pv_orca_text_encoder_param_t *param,
        FILE *file);

#endif

void PV_MOCKABLE(pv_orca_text_encoder_param_delete)(pv_ypu_t *ypu, pv_orca_text_encoder_param_t *param);

pv_status_t PV_MOCKABLE(pv_orca_text_encoder_param_load)(pv_ypu_t *ypu, FILE *f, pv_orca_text_encoder_param_t **param);

bool PV_MOCKABLE(pv_orca_text_encoder_param_is_equal)(
        const pv_orca_text_encoder_param_t *object,
        const pv_orca_text_encoder_param_t *other);

int32_t PV_MOCKABLE(pv_orca_text_encoder_param_receptive_field)(const pv_orca_text_encoder_param_t *object);

typedef struct pv_orca_text_encoder pv_orca_text_encoder_t;

pv_status_t PV_MOCKABLE(pv_orca_text_encoder_init)(
        pv_ypu_t *ypu,
        const pv_orca_text_encoder_param_t *param,
        pv_orca_text_encoder_t **object);

void PV_MOCKABLE(pv_orca_text_encoder_delete)(pv_ypu_t *ypu, pv_orca_text_encoder_t *object);

int32_t PV_MOCKABLE(pv_orca_text_encoder_output_channels)(const pv_orca_text_encoder_t *object);

pv_status_t PV_MOCKABLE(pv_orca_text_encoder_forward)(
        pv_ypu_t *ypu,
        pv_orca_text_encoder_t *object,
        int32_t num_tokens,
        const int32_t *tokens,
        pv_ypu_mem_t *encoded_tokens,
        pv_ypu_mem_t *means,
        pv_ypu_mem_t *logs,
        int32_t encoded_tokens_offset,
        int32_t means_offset,
        int32_t logs_offset);


#endif // PV_ORCA_TEXT_ENCODER_H
