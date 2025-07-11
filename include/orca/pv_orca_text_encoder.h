#ifndef PV_ORCA_TEXT_ENCODER_H
#define PV_ORCA_TEXT_ENCODER_H

#include <stdint.h>

#include "orca/pv_cnn.h"
#include "orca/pv_embed.h"
#include "orca/pv_transformer.h"
#include "util/pv_file.h"

typedef struct {
    const pv_embed_param_t *embed_param;
    const pv_cnn_param_t *conv_post_param;
    int32_t num_transformers;
    int32_t attention_window_future;
    const pv_transformer_param_t **transformers_param;
} pv_orca_text_encoder_param_t;

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_orca_text_encoder_param_serialize)(const pv_orca_text_encoder_param_t *param, FILE *file);

#endif

void PV_MOCKABLE(pv_orca_text_encoder_param_delete)(pv_orca_text_encoder_param_t *param);

pv_status_t PV_MOCKABLE(pv_orca_text_encoder_param_load)(FILE *f, pv_orca_text_encoder_param_t **param);

bool PV_MOCKABLE(pv_orca_text_encoder_param_is_equal)(
        const pv_orca_text_encoder_param_t *object,
        const pv_orca_text_encoder_param_t *other);

int32_t PV_MOCKABLE(pv_orca_text_encoder_param_receptive_field)(const pv_orca_text_encoder_param_t *object);

typedef struct pv_orca_text_encoder pv_orca_text_encoder_t;

pv_status_t PV_MOCKABLE(pv_orca_text_encoder_init)(
        const pv_orca_text_encoder_param_t *param,
        pv_orca_text_encoder_t **object);

void PV_MOCKABLE(pv_orca_text_encoder_delete)(pv_orca_text_encoder_t *object);

int32_t PV_MOCKABLE(pv_orca_text_encoder_output_channels)(const pv_orca_text_encoder_t *object);

pv_status_t PV_MOCKABLE(pv_orca_text_encoder_forward)(
        pv_orca_text_encoder_t *object,
        int32_t num_tokens,
        const int32_t *tokens,
        float *encoded_tokens,
        float *means,
        float *logs);


#endif // PV_ORCA_TEXT_ENCODER_H
