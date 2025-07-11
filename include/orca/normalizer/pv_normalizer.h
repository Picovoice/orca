#ifndef PV_NORMALIZER_H
#define PV_NORMALIZER_H

#include "core/pv_language.h"
#include "lm/pv_noun_gender_dict.h"
#include "orca/normalizer/pv_normalizer_tagger.h"
#include "orca/normalizer/pv_normalizer_token.h"
#include "orca/normalizer/pv_normalizer_tokenizer.h"
#include "orca/normalizer/pv_normalizer_use_cases.h"
#include "orca/normalizer/pv_normalizer_util.h"
#include "orca/normalizer/pv_normalizer_verbalizer.h"

typedef struct pv_normalizer pv_normalizer_t;

pv_status_t PV_MOCKABLE(pv_normalizer_init)(
        pv_language_info_t *language_info,
        pv_noun_gender_dict_t *noun_gender_dict,
        const void **tokenizer_data,
        pv_normalizer_t **object);

void PV_MOCKABLE(pv_normalizer_delete)(pv_normalizer_t *object);

pv_status_t PV_MOCKABLE(pv_normalizer_get_characters)(
        const pv_normalizer_t *object,
        int32_t *num_characters,
        const char *const **characters);

pv_normalizer_language_t PV_MOCKABLE(pv_normalizer_get_language)(const pv_normalizer_t *object);

pv_language_info_t *PV_MOCKABLE(pv_normalizer_get_language_info)(const pv_normalizer_t *object);

pv_normalizer_tokenizer_t *PV_MOCKABLE(pv_normalizer_get_tokenizer)(const pv_normalizer_t *object);

pv_normalizer_tagger_t *PV_MOCKABLE(pv_normalizer_get_tagger)(const pv_normalizer_t *object);

pv_normalizer_verbalizer_t *PV_MOCKABLE(pv_normalizer_get_verbalizer)(const pv_normalizer_t *object);

void PV_MOCKABLE(pv_normalizer_remove_hyphen_only_tokens)(pv_normalizer_token_list_t *token_list);

void PV_MOCKABLE(pv_normalizer_remove_invalid_custom_pron_markers)(pv_normalizer_token_list_t *token_list);

pv_status_t PV_MOCKABLE(pv_normalizer_normalize)(
        pv_normalizer_t *object,
        const char *text,
        bool preserve_word_boundary,
        bool remove_unknown_characters,
        char **normalized,
        pv_normalizer_token_list_t **token_list);

#endif // PV_NORMALIZER_H
