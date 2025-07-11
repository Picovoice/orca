#ifndef PV_NORMALIZER_VERBALIZER_H
#define PV_NORMALIZER_VERBALIZER_H

#include "lm/pv_noun_gender_dict.h"
#include "orca/normalizer/pv_normalizer_token.h"
#include "orca/normalizer/pv_normalizer_use_cases.h"
#include "orca/normalizer/pv_normalizer_util.h"

typedef struct pv_normalizer_verbalizer pv_normalizer_verbalizer_t;

pv_status_t PV_MOCKABLE(pv_normalizer_verbalizer_init)(
        pv_normalizer_language_t language,
        int32_t num_use_cases,
        const int32_t *use_cases,
        pv_noun_gender_dict_t *noun_gender_dict,
        pv_normalizer_verbalizer_t **object);

void PV_MOCKABLE(pv_normalizer_verbalizer_delete)(pv_normalizer_verbalizer_t *object);

pv_status_t PV_MOCKABLE(pv_normalizer_verbalizer_verbalize)(
        pv_normalizer_verbalizer_t *object,
        pv_normalizer_token_list_t *token_list,
        int32_t num_tokens_skip);

pv_status_t PV_MOCKABLE(pv_normalizer_verbalizer_verbalize_word_common)(pv_normalizer_token_t *token);

#endif // PV_NORMALIZER_VERBALIZER_H
