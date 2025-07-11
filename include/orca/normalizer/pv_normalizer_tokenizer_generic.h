#ifndef PV_NORMALIZER_TOKENIZER_GENERIC_H
#define PV_NORMALIZER_TOKENIZER_GENERIC_H

#include "core/pv_language.h"
#include "orca/normalizer/pv_normalizer_token.h"
#include "orca/normalizer/pv_normalizer_util.h"

typedef struct pv_normalizer_tokenizer_generic pv_normalizer_tokenizer_generic_t;

pv_status_t PV_MOCKABLE(pv_normalizer_tokenizer_generic_init)(
        pv_normalizer_language_t language,
        pv_language_info_t *language_info,
        pv_normalizer_tokenizer_generic_t **object);

void PV_MOCKABLE(pv_normalizer_tokenizer_generic_delete)(pv_normalizer_tokenizer_generic_t *object);

pv_status_t PV_MOCKABLE(pv_normalizer_tokenizer_generic_token_list_split_verbalized)(
        const pv_normalizer_tokenizer_generic_t *tokenizer,
        pv_normalizer_token_list_t *token_list,
        pv_normalizer_token_list_t **split_token_list);

pv_status_t PV_MOCKABLE(pv_normalizer_tokenizer_generic_tokenize)(
        const pv_normalizer_tokenizer_generic_t *object,
        const char *text,
        char word_boundary_character,
        bool preserve_word_boundary,
        bool remove_unknown_characters,
        bool split_on_special_characters,
        bool split_on_apostrophe,
        pv_normalizer_token_list_t **token_list);

char PV_MOCKABLE(pv_normalizer_tokenizer_generic_default_word_boundary_character)(void);

typedef struct pv_normalizer_tokenizer_generic_stream pv_normalizer_tokenizer_generic_stream_t;

pv_status_t PV_MOCKABLE(pv_normalizer_tokenizer_generic_stream_open)(
        pv_normalizer_tokenizer_generic_t *object,
        pv_normalizer_tokenizer_generic_stream_t **stream);

void PV_MOCKABLE(pv_normalizer_tokenizer_generic_stream_close)(pv_normalizer_tokenizer_generic_stream_t *object);

pv_status_t PV_MOCKABLE(pv_normalizer_tokenizer_generic_stream_tokenize_initial)(
        pv_normalizer_tokenizer_generic_stream_t *object,
        const char *text,
        char word_boundary_character,
        bool preserve_word_boundary,
        bool remove_unknown_characters,
        bool split_on_special_characters,
        pv_normalizer_token_list_t **token_list);

pv_status_t PV_MOCKABLE(pv_normalizer_tokenizer_generic_stream_tokenize_verbalizable)(
        pv_normalizer_tokenizer_generic_stream_t *object,
        const pv_normalizer_token_list_t *verbalizable_tokens,
        char word_boundary_character,
        bool preserve_word_boundary,
        bool remove_unknown_characters,
        bool split_on_special_characters,
        pv_normalizer_token_list_t **token_list);

#endif // PV_NORMALIZER_TOKENIZER_GENERIC_H
