#ifndef PV_NORMALIZER_VERBALIZER_FR_H
#define PV_NORMALIZER_VERBALIZER_FR_H

#include "orca/normalizer/pv_normalizer_token.h"
#include "orca/normalizer/pv_normalizer_use_cases.h"

typedef struct pv_normalizer_verbalizer_fr pv_normalizer_verbalizer_fr_t;

pv_status_t PV_MOCKABLE(pv_normalizer_verbalizer_fr_init)(
        int32_t num_use_cases,
        const pv_normalizer_use_cases_fr_t *use_cases,
        pv_normalizer_verbalizer_fr_t **object);

void PV_MOCKABLE(pv_normalizer_verbalizer_fr_delete)(pv_normalizer_verbalizer_fr_t *object);

pv_status_t PV_MOCKABLE(pv_normalizer_verbalizer_fr_verbalize)(
        pv_normalizer_verbalizer_fr_t *object,
        pv_normalizer_token_list_t *token_list,
        int32_t num_tokens_skip);

#endif // PV_NORMALIZER_VERBALIZER_FR_H
