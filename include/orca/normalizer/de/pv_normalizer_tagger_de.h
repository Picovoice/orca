#ifndef PV_NORMALIZER_TAGGER_DE_H
#define PV_NORMALIZER_TAGGER_DE_H

#include "core/pv_language.h"
#include "lm/pv_noun_gender_dict.h"
#include "orca/normalizer/pv_normalizer_token.h"
#include "orca/normalizer/pv_normalizer_tokenizer.h"
#include "orca/normalizer/pv_normalizer_use_cases.h"

typedef struct pv_normalizer_tagger_de pv_normalizer_tagger_de_t;

pv_status_t PV_MOCKABLE(pv_normalizer_tagger_de_init)(
        const pv_language_info_t *language_info,
        int32_t num_use_cases,
        const pv_normalizer_use_cases_de_t *use_cases,
        pv_normalizer_tokenizer_t *tokenizer,
        pv_noun_gender_dict_t *noun_gender_dict,
        pv_normalizer_tagger_de_t **object);

void PV_MOCKABLE(pv_normalizer_tagger_de_delete)(pv_normalizer_tagger_de_t *object);

pv_status_t PV_MOCKABLE(pv_normalizer_tagger_de_tag)(
        pv_normalizer_tagger_de_t *object,
        pv_normalizer_token_list_t *token_list,
        int32_t num_tokens_skip,
        bool split_untagged);

pv_status_t PV_MOCKABLE(pv_normalizer_tagger_de_tag_token)(
        const pv_normalizer_tagger_de_t *object,
        pv_normalizer_token_t *token,
        pv_normalizer_token_list_t *token_list);

pv_status_t PV_MOCKABLE(pv_normalizer_tagger_de_split_untagged_token)(
        const pv_normalizer_tagger_de_t *object,
        pv_normalizer_token_t **token,
        pv_normalizer_token_list_t **token_list,
        bool *did_split);

pv_status_t PV_MOCKABLE(pv_normalizer_tagger_de_tag_time)(
        pv_normalizer_token_t *token,
        pv_normalizer_token_list_t *token_list);

#endif // PV_NORMALIZER_TAGGER_DE_H
