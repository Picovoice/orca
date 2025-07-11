#ifndef PV_NORMALIZER_TOKEN_H
#define PV_NORMALIZER_TOKEN_H

#include <stdbool.h>

#include "core/pv_error.h"
#include "orca/normalizer/pv_normalizer_tags.h"

#define PV_NORMALIZER_MAX_NUM_BYTES_PER_CHARACTER (5)

static const char PV_NORMALIZER_CUSTOM_PRON_OPENING_MARKER = '{';
static const char PV_NORMALIZER_CUSTOM_PRON_CLOSING_MARKER = '}';
static const char PV_NORMALIZER_CUSTOM_PRON_SEPARATOR = '|';

typedef struct pv_normalizer_token pv_normalizer_token_t;

typedef enum pv_normalizer_token_gender {
    PV_NORMALIZER_TOKEN_GENDER_NONE = 0,
    PV_NORMALIZER_TOKEN_GENDER_NEUTRAL,
    PV_NORMALIZER_TOKEN_GENDER_FEMININE,
    PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
} pv_normalizer_token_gender_t;

struct pv_normalizer_token {
    char *string;
    char *original_string;
    char *verbalized;
    char *reading;

    int32_t tag;
    pv_normalizer_token_tag_t tag_language_agnostic;
    int32_t length_future_context;
    int32_t length_past_context;
    int32_t tag_data_index;

    char *pronunciation;

    bool is_verbalizable;
    bool next_character_is_space;
    bool is_noun;
    bool use_short_one;
    pv_normalizer_token_gender_t gender;
    bool next_character_is_hyphen;

    pv_normalizer_token_t *next;
    pv_normalizer_token_t *previous;
};

pv_status_t PV_MOCKABLE(pv_normalizer_token_init)(
        int32_t start_index,
        int32_t end_index,
        const char *text,
        bool is_punctuation,
        bool has_pronunciation,
        bool next_character_is_space,
        pv_normalizer_token_t **object);

pv_status_t PV_MOCKABLE(pv_normalizer_token_init_with_original_string)(
        char *string,
        char *original_string,
        bool is_punctuation,
        bool next_character_is_space,
        int32_t length_future_context,
        int32_t length_past_context,
        pv_normalizer_token_t **object);

pv_status_t PV_MOCKABLE(pv_normalizer_token_copy)(pv_normalizer_token_t *source, pv_normalizer_token_t **destination);

void PV_MOCKABLE(pv_normalizer_token_set_verbalized)(
        pv_normalizer_token_t *object,
        char *new_verbalized);

void PV_MOCKABLE(pv_normalizer_token_set_reading)(
        pv_normalizer_token_t *object,
        char *new_reading);

void PV_MOCKABLE(pv_normalizer_token_delete)(pv_normalizer_token_t *object);

pv_status_t PV_MOCKABLE(pv_normalizer_token_read_text_segment)(
        int32_t start_index,
        int32_t end_index,
        const char *text,
        char **segment);

pv_status_t PV_MOCKABLE(pv_normalizer_token_read_custom_pronunciation)(
        int32_t start_index,
        int32_t end_index,
        const char *text,
        char **segment,
        char **pronunciation);

typedef struct pv_normalizer_token_list pv_normalizer_token_list_t;

struct pv_normalizer_token_list {
    int32_t size;
    pv_normalizer_token_t *head;
    pv_normalizer_token_t *tail;
};

pv_status_t PV_MOCKABLE(pv_normalizer_token_list_init)(pv_normalizer_token_list_t **object);

void PV_MOCKABLE(pv_normalizer_token_list_delete)(pv_normalizer_token_list_t *object);

void PV_MOCKABLE(pv_normalizer_token_list_reset)(pv_normalizer_token_list_t *object);

pv_status_t PV_MOCKABLE(pv_normalizer_token_list_copy)(
        const pv_normalizer_token_list_t *object,
        pv_normalizer_token_list_t **list);

pv_status_t PV_MOCKABLE(pv_normalizer_token_list_copy_portion)(
        const pv_normalizer_token_list_t *object,
        pv_normalizer_token_t *start,
        pv_normalizer_token_t *end,
        pv_normalizer_token_list_t **list);

void PV_MOCKABLE(pv_normalizer_token_list_append_token)(
        pv_normalizer_token_list_t *object,
        pv_normalizer_token_t *token);

void PV_MOCKABLE(pv_normalizer_token_list_append_list)(
        pv_normalizer_token_list_t *object,
        pv_normalizer_token_list_t *list);

void PV_MOCKABLE(pv_normalizer_token_list_remove_token)(
        pv_normalizer_token_list_t *token_list,
        pv_normalizer_token_t *token);

void PV_MOCKABLE(pv_normalizer_token_list_replace_token_with_list)(
        pv_normalizer_token_list_t *original_token_list,
        pv_normalizer_token_t **original_token,
        pv_normalizer_token_list_t *new_token_list);

void PV_MOCKABLE(pv_normalizer_token_list_insert_token)(
        pv_normalizer_token_list_t *token_list,
        pv_normalizer_token_t *token,
        pv_normalizer_token_t *token_to_insert);

void PV_MOCKABLE(pv_normalizer_token_list_remove_extra_spaces)(pv_normalizer_token_list_t *token_list);

void PV_MOCKABLE(pv_normalizer_token_list_remove_nonverbalized_tokens)(pv_normalizer_token_list_t *token_list);

void PV_MOCKABLE(pv_normalizer_token_list_remove_space_tokens)(pv_normalizer_token_list_t *token_list);

pv_status_t PV_MOCKABLE(pv_normalizer_token_list_to_string_text)(
        const pv_normalizer_token_list_t *token_list,
        char **text);

pv_status_t PV_MOCKABLE(pv_normalizer_token_list_to_verbalized_text)(
        const pv_normalizer_token_list_t *token_list,
        char seperator_character,
        char **text);

pv_status_t PV_MOCKABLE(pv_normalizer_token_list_to_token_array)(
        pv_normalizer_token_list_t *token_list,
        pv_normalizer_token_t ***token_array);

pv_status_t PV_MOCKABLE(pv_normalizer_token_list_collapse_tokens)(
        pv_normalizer_token_list_t *token_list,
        const char *collapsed_token_string,
        pv_normalizer_token_t *first_token,
        pv_normalizer_token_t *last_token,
        pv_normalizer_token_t **collapsed_token);

pv_status_t PV_MOCKABLE(pv_normalizer_token_list_merge_tokens)(
        pv_normalizer_token_list_t *token_list,
        const char *merged_token_string,
        const char *merged_original_string,
        pv_normalizer_token_t *first_token,
        pv_normalizer_token_t *last_token,
        pv_normalizer_token_t **collapsed_token);

pv_status_t PV_MOCKABLE(pv_normalizer_token_list_unroll_token)(
        int32_t string_split_index,
        pv_normalizer_token_t *token,
        pv_normalizer_token_list_t *token_list);

int32_t PV_MOCKABLE(pv_normalizer_token_get_num_tokens_before)(const pv_normalizer_token_t *token);

pv_normalizer_token_t *PV_MOCKABLE(pv_normalizer_token_get_nth_token_before)(
        const pv_normalizer_token_t *token,
        int32_t n,
        bool skip_space);

pv_normalizer_token_t *PV_MOCKABLE(pv_normalizer_token_get_nth_token_after)(
        const pv_normalizer_token_t *token,
        int32_t n,
        bool skip_space);

bool PV_MOCKABLE(pv_normalizer_token_is_space_or_null)(const pv_normalizer_token_t *token);

pv_status_t PV_MOCKABLE(pv_normalizer_token_concatenate_token_strings)(
        pv_normalizer_token_t *first,
        pv_normalizer_token_t *last,
        char **text);

pv_normalizer_token_t *PV_MOCKABLE(pv_normalizer_token_get_token_after_previous_space)(pv_normalizer_token_t *token);

void PV_MOCKABLE(pv_normalizer_token_list_synchronize_language_agnostic_tags_common)(
        pv_normalizer_token_list_t *token_list,
        int32_t tag_space,
        int32_t tag_punctuation,
        int32_t tag_single_quote,
        int32_t tag_custom_pronunciation,
        int32_t tag_letter_spell_out);

#endif // PV_NORMALIZER_TOKEN_H
