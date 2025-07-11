#ifndef PV_NORMALIZER_UTIL_H
#define PV_NORMALIZER_UTIL_H

#include <stdbool.h>

#include "core/picovoice.h"
#include "core/pv_language.h"
#include "orca/normalizer/pv_normalizer_token.h"

typedef enum {
    PV_NORMALIZER_LANGUAGE_EN = 0,
    PV_NORMALIZER_LANGUAGE_DE = 1,
    PV_NORMALIZER_LANGUAGE_FR = 2,
    PV_NORMALIZER_LANGUAGE_ES = 3,
    PV_NORMALIZER_LANGUAGE_IT = 4,
    PV_NORMALIZER_LANGUAGE_PT = 5,
    PV_NORMALIZER_LANGUAGE_KO = 6,
    PV_NORMALIZER_LANGUAGE_JA = 7
} pv_normalizer_language_t;


pv_status_t PV_MOCKABLE(pv_normalizer_util_get_normalizable_characters)(
        pv_normalizer_language_t language,
        int32_t *num_characters,
        const char *const **characters);

pv_status_t PV_MOCKABLE(pv_normalizer_util_is_special_character)(
        pv_normalizer_language_t language,
        const char *character,
        bool *is_special,
        int32_t *length);

pv_status_t PV_MOCKABLE(pv_normalizer_util_is_punctuation)(
        pv_normalizer_language_t language,
        const char *token,
        bool *is_punctuation);

pv_status_t PV_MOCKABLE(pv_normalizer_util_is_normalizable_character)(
        pv_normalizer_language_t language,
        const char *character,
        bool *is_normalizable);

pv_status_t PV_MOCKABLE(pv_normalizer_util_is_word_character)(
        pv_normalizer_language_t language,
        const char *character,
        bool *is_word);

pv_status_t PV_MOCKABLE(pv_normalizer_util_character_index)(
        const char *character,
        int32_t num_characters,
        const char *const *characters,
        int32_t *index);

pv_status_t PV_MOCKABLE(pv_normalizer_util_validate_text)(
        pv_normalizer_language_t language,
        const pv_language_info_t *language_info,
        const char *text,
        bool preserve_word_boundary,
        bool preserve_custom_pron_markers,
        char **cleaned_text);

pv_status_t PV_MOCKABLE(pv_normalizer_util_upper_inplace)(char *word);

pv_status_t PV_MOCKABLE(pv_normalizer_util_upper)(const char *word, char **upper);

bool PV_MOCKABLE(pv_normalizer_util_string_number_greater_than_int)(const char *string_number, int64_t int_number);

typedef struct pv_normalizer_util_trie_node pv_normalizer_util_trie_node_t;

struct pv_normalizer_util_trie_node {
    int32_t num_children;
    pv_normalizer_util_trie_node_t **children;
    int32_t index;
};

typedef struct pv_normalizer_util_trie {
    int32_t num_characters;
    const char *const *characters;
    pv_normalizer_util_trie_node_t *root;
} pv_normalizer_util_trie_t;

pv_status_t PV_MOCKABLE(pv_normalizer_util_trie_node_init)(
        int32_t num_characters,
        pv_normalizer_util_trie_node_t **node);

void PV_MOCKABLE(pv_normalizer_util_trie_node_delete)(pv_normalizer_util_trie_node_t *object);

pv_status_t PV_MOCKABLE(pv_normalizer_util_trie_create)(
        int32_t num_characters,
        const char *const *characters,
        int32_t num_strings,
        const char **strings,
        pv_normalizer_util_trie_t **trie);

void PV_MOCKABLE(pv_normalizer_util_trie_delete)(pv_normalizer_util_trie_t *object);

pv_status_t PV_MOCKABLE(pv_normalizer_util_trie_insert)(
        const pv_normalizer_util_trie_t *object,
        const char *string,
        int32_t index);

pv_status_t PV_MOCKABLE(pv_normalizer_util_trie_search)(
        const pv_normalizer_util_trie_t *object,
        const char *string,
        int32_t *index);

pv_status_t PV_MOCKABLE(pv_normalizer_util_get_next_character)(
        const char *string,
        int32_t index,
        char *character,
        int32_t *num_bytes_character);

bool PV_MOCKABLE(pv_normalizer_util_only_contains_digits)(const char *string);

pv_status_t PV_MOCKABLE(pv_normalizer_util_is_capitalized_word)(
        const pv_language_info_t *language_info,
        const char *string,
        bool *is_capitalized_word);

pv_status_t PV_MOCKABLE(pv_normalizer_util_is_spellout)(
        const pv_language_info_t *language_info,
        const char *string,
        bool *is_spellout);

bool PV_MOCKABLE(pv_normalizer_util_is_in_use_cases)(
        int32_t num_use_cases,
        const int32_t *all_use_cases,
        int32_t target_use_case);

pv_status_t PV_MOCKABLE(pv_normalizer_util_infer_language_from_language_info)(
        const pv_language_info_t *language_info,
        pv_normalizer_language_t *language);

pv_status_t PV_MOCKABLE(pv_normalizer_get_use_cases_from_language)(
        pv_normalizer_language_t language,
        int32_t *num_use_cases,
        const int32_t **use_cases);

bool PV_MOCKABLE(pv_normalizer_is_valid_eos)(
        pv_normalizer_language_t language,
        const char *character);

pv_status_t PV_MOCKABLE(pv_normalizer_util_is_alphabetic)(
        const pv_language_info_t *language_info,
        const char *string,
        bool *is_alphabetic);

pv_status_t PV_MOCKABLE(pv_normalizer_util_is_alphabet_character)(
        pv_normalizer_language_t language,
        const char *character,
        bool *is_alphabetic_character);

pv_status_t PV_MOCKABLE(pv_normalizer_util_is_word)(
        const pv_language_info_t *language_info,
        const char *string,
        bool *is_word);

pv_status_t PV_MOCKABLE(pv_normalizer_util_is_word_token)(
        const pv_language_info_t *language_info,
        const pv_normalizer_token_t *token,
        bool *is_word);

pv_status_t PV_MOCKABLE(pv_normalizer_util_remap_characters)(
        const char *text,
        char **remapped_text);

pv_status_t PV_MOCKABLE(pv_normalizer_util_remap_space)(
        const char *text,
        char **remapped_text);

pv_status_t PV_MOCKABLE(pv_normalizer_util_check_token_is_before_character)(
        pv_normalizer_token_t *token,
        const char *target,
        bool *is_before_character);


#endif // PV_NORMALIZER_UTIL_H
