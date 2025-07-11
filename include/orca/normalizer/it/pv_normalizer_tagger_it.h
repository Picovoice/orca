#ifndef PV_NORMALIZER_TAGGER_IT_H
#define PV_NORMALIZER_TAGGER_IT_H

#include "core/pv_language.h"
#include "lm/pv_noun_gender_dict.h"
#include "orca/normalizer/pv_normalizer_token.h"
#include "orca/normalizer/pv_normalizer_tokenizer.h"
#include "orca/normalizer/pv_normalizer_use_cases.h"

typedef struct pv_normalizer_tagger_it pv_normalizer_tagger_it_t;

pv_status_t PV_MOCKABLE(pv_normalizer_tagger_it_init)(
        const pv_language_info_t *language_info,
        int32_t num_use_cases,
        const pv_normalizer_use_cases_it_t *use_cases,
        pv_normalizer_tokenizer_t *tokenizer,
        pv_noun_gender_dict_t *noun_gender_dict,
        pv_normalizer_tagger_it_t **object);

void PV_MOCKABLE(pv_normalizer_tagger_it_delete)(pv_normalizer_tagger_it_t *object);

pv_status_t PV_MOCKABLE(pv_normalizer_tagger_it_tag)(
        pv_normalizer_tagger_it_t *object,
        pv_normalizer_token_list_t *token_list,
        int32_t num_tokens_skip,
        bool split_untagged);

pv_status_t PV_MOCKABLE(pv_normalizer_tagger_it_split_untagged_token)(
        const pv_normalizer_tagger_it_t *object,
        pv_normalizer_token_t **token,
        pv_normalizer_token_list_t **token_list,
        bool *did_split);

#endif // PV_NORMALIZER_TAGGER_IT_H
