#ifndef PV_NORMALIZER_VERBALIZER_IT_H
#define PV_NORMALIZER_VERBALIZER_IT_H

#include "lm/pv_noun_gender_dict.h"
#include "orca/normalizer/pv_normalizer_token.h"
#include "orca/normalizer/pv_normalizer_use_cases.h"

typedef struct pv_normalizer_verbalizer_it pv_normalizer_verbalizer_it_t;

pv_status_t PV_MOCKABLE(pv_normalizer_verbalizer_it_init)(
        int32_t num_use_cases,
        const pv_normalizer_use_cases_it_t *use_cases,
        pv_noun_gender_dict_t *noun_gender_dict,
        pv_normalizer_verbalizer_it_t **object);

void PV_MOCKABLE(pv_normalizer_verbalizer_it_delete)(pv_normalizer_verbalizer_it_t *object);

pv_status_t PV_MOCKABLE(pv_normalizer_verbalizer_it_verbalize)(
        pv_normalizer_verbalizer_it_t *object,
        pv_normalizer_token_list_t *token_list,
        int32_t num_tokens_skip);

#endif // PV_NORMALIZER_VERBALIZER_IT_H
