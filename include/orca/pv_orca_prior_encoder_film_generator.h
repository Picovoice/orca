#ifndef PV_ORCA_PRIOR_ENCODER_FILM_GENERATOR_H
#define PV_ORCA_PRIOR_ENCODER_FILM_GENERATOR_H

#include "orca/pv_embed.h"
#include "orca/pv_rope_transformer.h"
#include "util/pv_file.h"

typedef struct {
    int32_t num_blocks;
    int32_t dimension;

    const pv_embed_param_t *embed_param;
    const pv_rope_transformer_param_t **transformers_param;
} pv_orca_prior_encoder_film_generator_param_t;

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_orca_prior_encoder_film_generator_param_serialize)(
        pv_ypu_t *ypu,
        const pv_orca_prior_encoder_film_generator_param_t *param,
        FILE *file);

#endif

void PV_MOCKABLE(pv_orca_prior_encoder_film_generator_param_delete)(
        pv_ypu_t *ypu,
        pv_orca_prior_encoder_film_generator_param_t *param);

pv_status_t PV_MOCKABLE(pv_orca_prior_encoder_film_generator_param_load)(
        pv_ypu_t *ypu,
        FILE *f,
        pv_orca_prior_encoder_film_generator_param_t **param);

bool PV_MOCKABLE(pv_orca_prior_encoder_film_generator_param_is_equal)(
        const pv_orca_prior_encoder_film_generator_param_t *object,
        const pv_orca_prior_encoder_film_generator_param_t *other);

typedef struct pv_orca_prior_encoder_film_generator pv_orca_prior_encoder_film_generator_t;

pv_status_t PV_MOCKABLE(pv_orca_prior_encoder_film_generator_init)(
        pv_ypu_t *ypu,
        const pv_orca_prior_encoder_film_generator_param_t *param,
        pv_orca_prior_encoder_film_generator_t **object);

void PV_MOCKABLE(pv_orca_prior_encoder_film_generator_delete)(
        pv_ypu_t *ypu,
        pv_orca_prior_encoder_film_generator_t *object);

pv_status_t PV_MOCKABLE(pv_orca_prior_encoder_film_generator_forward)(
        pv_ypu_t *ypu,
        pv_orca_prior_encoder_film_generator_t *object,
        int32_t n,
        const int32_t *x,
        pv_ypu_mem_t *y_ypu);

#endif // PV_ORCA_PRIOR_ENCODER_FILM_GENERATOR_H
