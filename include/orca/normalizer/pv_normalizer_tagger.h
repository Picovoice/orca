#ifndef PV_NORMALIZER_TAGGER_H
#define PV_NORMALIZER_TAGGER_H

#include "core/pv_language.h"
#include "lm/pv_noun_gender_dict.h"
#include "orca/normalizer/pv_normalizer_token.h"
#include "orca/normalizer/pv_normalizer_tokenizer.h"
#include "orca/normalizer/pv_normalizer_use_cases.h"

typedef struct pv_normalizer_tagger pv_normalizer_tagger_t;

pv_status_t PV_MOCKABLE(pv_normalizer_tagger_init)(
        const pv_language_info_t *language_info,
        int32_t num_use_cases,
        const int32_t *use_cases,
        pv_normalizer_tokenizer_t *tokenizer,
        pv_noun_gender_dict_t *noun_gender_dict,
        pv_normalizer_tagger_t **object);

void PV_MOCKABLE(pv_normalizer_tagger_delete)(pv_normalizer_tagger_t *object);

pv_status_t PV_MOCKABLE(pv_normalizer_tagger_tag)(
        pv_normalizer_tagger_t *object,
        pv_normalizer_token_list_t *token_list,
        int32_t num_tokens_skip,
        bool split_untagged);

void PV_MOCKABLE(pv_normalizer_tagger_tag_from_language_agnostic_common)(
        pv_normalizer_token_t *token,
        int32_t tag_none,
        int32_t tag_space,
        int32_t tag_punctuation,
        int32_t tag_custom_pronunciation);

pv_status_t PV_MOCKABLE(pv_normalizer_tagger_tag_word_common)(
        const pv_language_info_t *language_info,
        pv_normalizer_token_t *token,
        int32_t tag);

pv_status_t PV_MOCKABLE(pv_normalizer_tagger_tag_punctuation_common)(
        const pv_normalizer_language_t language,
        pv_normalizer_token_t *token,
        int32_t tag);

#endif // PV_NORMALIZER_TAGGER_H
