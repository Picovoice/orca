#ifndef PV_NORMALIZER_TOKENIZER_JA_H
#define PV_NORMALIZER_TOKENIZER_JA_H

#include <stdio.h>

#include "core/picovoice.h"
#include "orca/normalizer/pv_normalizer_token.h"
#include "orca/normalizer/pv_normalizer_util.h"

typedef struct pv_normalizer_tokenizer_ja pv_normalizer_tokenizer_ja_t;

pv_status_t PV_MOCKABLE(pv_normalizer_tokenizer_ja_init)(
        pv_normalizer_language_t language,
        pv_language_info_t *language_info,
        const void **tokenizer_data,
        pv_normalizer_tokenizer_ja_t **object);

void PV_MOCKABLE(pv_normalizer_tokenizer_ja_delete)(pv_normalizer_tokenizer_ja_t *object);

pv_status_t PV_MOCKABLE(pv_normalizer_tokenizer_ja_tokenize)(
        const pv_normalizer_tokenizer_ja_t *object,
        const char *text,
        bool preserve_word_boundary,
        bool remove_unknown_characters,
        bool normalize_token_strings,
        pv_normalizer_token_list_t **token_list);

pv_status_t PV_MOCKABLE(pv_normalizer_tokenizer_ja_tokenize_on_character)(
        const pv_normalizer_tokenizer_ja_t *object,
        const char *text,
        char word_boundary_character,
        bool preserve_word_boundary,
        bool remove_unknown_characters,
        bool split_on_special_characters,
        pv_normalizer_token_list_t **token_list);

pv_status_t PV_MOCKABLE(pv_normalizer_tokenizer_ja_token_list_split_verbalized)(
        const pv_normalizer_tokenizer_ja_t *tokenizer,
        pv_normalizer_token_list_t *token_list,
        pv_normalizer_token_list_t **split_token_list);

typedef struct pv_normalizer_tokenizer_ja_stream pv_normalizer_tokenizer_ja_stream_t;

pv_status_t PV_MOCKABLE(pv_normalizer_tokenizer_ja_stream_open)(
        pv_normalizer_tokenizer_ja_t *object,
        pv_normalizer_tokenizer_ja_stream_t **stream);

void PV_MOCKABLE(pv_normalizer_tokenizer_ja_stream_close)(pv_normalizer_tokenizer_ja_stream_t *object);

pv_status_t PV_MOCKABLE(pv_normalizer_tokenizer_ja_stream_tokenize)(
        pv_normalizer_tokenizer_ja_stream_t *object,
        const char *text,
        bool preserve_word_boundary,
        bool remove_unknown_characters,
        pv_normalizer_token_list_t **token_list);

pv_status_t PV_MOCKABLE(pv_normalizer_tokenizer_ja_stream_flush)(
        pv_normalizer_tokenizer_ja_stream_t *object,
        bool preserve_word_boundary,
        bool remove_unknown_characters,
        pv_normalizer_token_list_t **token_list);

#endif //PV_NORMALIZER_TOKENIZER_JA_H
