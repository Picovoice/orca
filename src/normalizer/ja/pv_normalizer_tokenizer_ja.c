#include <stdlib.h>
#include <string.h>

#include "mecab.h"

#include "core/pv_assert.h"
#include "core/pv_error_messages.h"
#include "core/pv_language.h"
#include "math/pv_math.h"
#include "orca/normalizer/ja/pv_normalizer_tags_ja.h"
#include "orca/normalizer/ja/pv_normalizer_tokenizer_ja.h"
#include "orca/normalizer/ja/pv_normalizer_util_ja.h"
#include "orca/normalizer/pv_normalizer_tokenizer_generic.h"
#include "util/pv_string.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_normalizer_mock.h"

#endif

#define PV_TOKENIZER_NEXT_CHARS_SWITCH_LIMIT (1)
#define PV_TOKENIZER_NEXT_CHARS_SAME_LIMIT (10)
#define PV_TOKENIZER_PREVIOUS_TOKENS_COUNT (2)
#define PV_TOKENIZER_FEATURE_INDEX (8)

struct pv_normalizer_tokenizer_ja {
    pv_normalizer_language_t language;
    const pv_language_info_t *language_info;
    mecab_t *mecab;
    pv_normalizer_tokenizer_generic_t *generic_tokenizer;
};

// these correspond with Mecab's char definitions
typedef enum {
    PV_NORMALIZER_TOKENIZER_JA_CHAR_TYPE_DEFAULT = 0,
    PV_NORMALIZER_TOKENIZER_JA_CHAR_TYPE_SPACE = 1,
    PV_NORMALIZER_TOKENIZER_JA_CHAR_TYPE_KANJI = 2,
    PV_NORMALIZER_TOKENIZER_JA_CHAR_TYPE_SYMBOL = 3,
    PV_NORMALIZER_TOKENIZER_JA_CHAR_TYPE_NUMERIC = 4,
    PV_NORMALIZER_TOKENIZER_JA_CHAR_TYPE_ALPHA = 5,
    PV_NORMALIZER_TOKENIZER_JA_CHAR_TYPE_HIRAGANA = 6,
    PV_NORMALIZER_TOKENIZER_JA_CHAR_TYPE_KATAKANA = 7,
    PV_NORMALIZER_TOKENIZER_JA_CHAR_TYPE_KANJI_NUMERIC = 8
} pv_normalizer_tokenizer_ja_char_type_t;

pv_status_t PV_MOCKABLE(pv_normalizer_tokenizer_ja_init)(
        pv_normalizer_language_t language,
        pv_language_info_t *language_info,
        const void **tokenizer_data,
        pv_normalizer_tokenizer_ja_t **object) {
    PV_ASSERT(language_info);
    PV_ASSERT(tokenizer_data);
    PV_ASSERT(object);

    *object = NULL;

    pv_normalizer_tokenizer_ja_t *o = calloc(1, sizeof(pv_normalizer_tokenizer_ja_t));
    if (!o) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->mecab = mecab_new_from_buffer((const char **) tokenizer_data);
    if (!o->mecab) {
        pv_normalizer_tokenizer_ja_delete(o);
        PV_ERROR_REPORT(
                &pv_error_msg_module_function_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("mecab_new_from_buffer"));
        return PV_STATUS_IO_ERROR;
    }

    pv_status_t status = pv_normalizer_tokenizer_generic_init(
            language,
            language_info,
            &(o->generic_tokenizer));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_tokenizer_generic_init,
                pv_status_to_string(status));
        pv_normalizer_tokenizer_ja_delete(o);
        return status;
    }

    o->language = language;
    o->language_info = language_info;

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_normalizer_tokenizer_ja_delete)(pv_normalizer_tokenizer_ja_t *object) {
    if (object) {
        if (object->mecab) {
            mecab_destroy(object->mecab);
        }
        pv_normalizer_tokenizer_generic_delete(object->generic_tokenizer);
        free(object);
    }
}

static pv_status_t extract_custom_pron(
        const mecab_node_t *node,
        pv_normalizer_token_t **token,
        const mecab_node_t **new_node) {
    PV_ASSERT(node);
    PV_ASSERT(token);
    PV_ASSERT(new_node);

    *token = NULL;
    *new_node = NULL;

    int32_t surface_idx = 1;
    bool is_custom_pron_sep = false;
    bool is_custom_pron_end = false;
    bool is_invalid_custom_pron = false;

    pv_normalizer_token_t *t = NULL;

    while ((surface_idx < ((int32_t) strlen(node->surface))) && (!is_invalid_custom_pron)) {
        int32_t num_bytes_character;
        pv_status_t status = pv_language_num_bytes_character(
                (unsigned char) node->surface[surface_idx],
                &num_bytes_character);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT(
                    &pv_error_msg_invalid_argument,
                    PV_ERROR_ARGS_PUBLIC("text"),
                    PV_ERROR_ARGS_PRIVATE_EMPTY());
            return status;
        }

        if (num_bytes_character == 1) {
            char c = node->surface[surface_idx];
            if (c == PV_NORMALIZER_CUSTOM_PRON_OPENING_MARKER) {
                is_invalid_custom_pron = true;
            } else if (c == PV_NORMALIZER_CUSTOM_PRON_SEPARATOR) {
                if (is_custom_pron_sep) {
                    is_invalid_custom_pron = true;
                } else {
                    is_custom_pron_sep = true;
                }
            } else if (c == PV_NORMALIZER_CUSTOM_PRON_CLOSING_MARKER) {
                if (!is_custom_pron_sep) {
                    is_invalid_custom_pron = true;
                } else {
                    is_custom_pron_end = true;
                    break;
                }
            }
        }
        surface_idx += num_bytes_character;
    }
    if (surface_idx >= ((int32_t) strlen(node->surface))) {
        is_invalid_custom_pron = true;
    } else if (is_custom_pron_sep && is_custom_pron_end) {
        pv_status_t status = pv_normalizer_token_init(
                0,
                surface_idx - 1,
                node->surface,
                false,
                true,
                false,
                &t);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_token_init,
                    pv_status_to_string(status));
            return status;
        }
    }

    if (is_invalid_custom_pron || !t) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument,
                PV_ERROR_ARGS_PUBLIC("text"),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }

    while (node->surface[0] != PV_NORMALIZER_CUSTOM_PRON_CLOSING_MARKER) {
        node = node->next;
    }

    *token = t;
    *new_node = node;

    return PV_STATUS_SUCCESS;
}

static pv_status_t check_ja_char_composition(
        const char *str,
        int32_t str_len,
        bool *contains_kana,
        bool *contains_kanji) {
    PV_ASSERT(str);
    PV_ASSERT(str_len);

    *contains_kana = false;
    *contains_kanji = false;

    int32_t i = 0;
    while (i < str_len) {
        int32_t num_bytes = 0;
        pv_status_t status = pv_language_num_bytes_character((unsigned char) str[i], &num_bytes);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_language_num_bytes_character,
                    pv_status_to_string(status));
            return status;
        }

        if (num_bytes == 3) {
            int32_t unicode_val = ((str[i] & 0x0F) << 12) | ((str[i + 1] & 0x3F) << 6) | (str[i + 2] & 0x3F);
            if (((unicode_val >= 0x3040) && (unicode_val <= 0x309F)) || // hiragana
                ((unicode_val >= 0x30A0) && (unicode_val <= 0x30FF))) { // katakana
                *contains_kana = true;
            } else if ((unicode_val >= 0x4E00) && (unicode_val <= 0x9FFF)) {
                *contains_kanji = true;
            }
        }
        i += num_bytes;
    }

    return PV_STATUS_SUCCESS;
}

static pv_status_t append_space_token(pv_normalizer_token_list_t *token_list) {
    PV_ASSERT(token_list);
    if ((token_list->tail != NULL) && strcmp(token_list->tail->string, " ") == 0) {
        return PV_STATUS_SUCCESS;
    }

    pv_normalizer_token_t *space_token = NULL;
    pv_status_t status = pv_normalizer_token_init(
            0,
            0,
            " ",
            false,
            false,
            false,
            &space_token);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_token_init,
                pv_status_to_string(status));
        return status;
    }

    space_token->tag_language_agnostic = PV_NORMALIZER_TAG_SPACE;
    pv_normalizer_token_list_append_token(token_list, space_token);

    return PV_STATUS_SUCCESS;
}

static pv_status_t extract_alphanum_string(
        const mecab_node_t *node,
        char **alphanum_string,
        const mecab_node_t **new_node) {
    PV_ASSERT(node);
    PV_ASSERT(alphanum_string);
    PV_ASSERT(new_node);

    *alphanum_string = NULL;
    *new_node = NULL;

    const mecab_node_t *alphanum_ptr = node;
    while ((alphanum_ptr->next->stat != MECAB_EOS_NODE) &&
           (alphanum_ptr->next->char_type != PV_NORMALIZER_TOKENIZER_JA_CHAR_TYPE_KANJI) &&
           (alphanum_ptr->next->char_type != PV_NORMALIZER_TOKENIZER_JA_CHAR_TYPE_HIRAGANA) &&
           (alphanum_ptr->next->char_type != PV_NORMALIZER_TOKENIZER_JA_CHAR_TYPE_KATAKANA) &&
           (alphanum_ptr->next->char_type != PV_NORMALIZER_TOKENIZER_JA_CHAR_TYPE_KANJI_NUMERIC)) {
        bool contains_kana, contains_kanji = false;
        pv_status_t status = check_ja_char_composition(
                alphanum_ptr->next->surface,
                alphanum_ptr->next->length,
                &contains_kana,
                &contains_kanji);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    check_ja_char_composition,
                    pv_status_to_string(status));
            return status;
        }
        if (contains_kana || contains_kanji) {
            break;
        }

        alphanum_ptr = alphanum_ptr->next;
    }

    size_t alphanum_size = alphanum_ptr->next->stat != MECAB_EOS_NODE ? strlen(node->surface) - strlen(alphanum_ptr->next->surface) : strlen(node->surface);

    char *alphanum_text = calloc(alphanum_size + 1, sizeof(char));
    if (!alphanum_text) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("digit_group"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memcpy(alphanum_text, node->surface, alphanum_size);

    *new_node = alphanum_ptr;
    *alphanum_string = alphanum_text;

    return PV_STATUS_SUCCESS;
}

void replace_fullwidth_spaces(char *str) {
    if ((str == NULL) || (strlen(str) < 3)) {
        return;
    }

    for (size_t i = 0; i < strlen(str) - 3; i++) {
        if ((unsigned char) str[i] == 0xE3 &&
            (unsigned char) str[i + 1] == 0x80 &&
            (unsigned char) str[i + 2] == 0x80) {
            str[i] = ' ';
            memmove(&str[i + 1], &str[i + 3], strlen(&str[i + 3]) + 1);
        }
    }
}

pv_status_t PV_MOCKABLE(pv_normalizer_tokenizer_ja_tokenize)(
        const pv_normalizer_tokenizer_ja_t *object,
        const char *text,
        bool preserve_word_boundary,
        bool remove_unknown_characters,
        bool normalize_token_strings_var,
        pv_normalizer_token_list_t **token_list) {
    PV_ASSERT(object);
    PV_ASSERT(text);
    PV_ASSERT(token_list);

    *token_list = NULL;

    char *cleaned_text = NULL;
    pv_status_t status = pv_normalizer_util_validate_text(
            object->language,
            object->language_info,
            text,
            preserve_word_boundary,
            true,
            remove_unknown_characters ? &cleaned_text : NULL);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_util_validate_text,
                pv_status_to_string(status));
        return status;
    }
    if (!cleaned_text) {
        cleaned_text = malloc(strlen(text) + 1);
        if (!cleaned_text) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("cleaned_text"));
        }
        strcpy(cleaned_text, text);
    }

    replace_fullwidth_spaces(cleaned_text);
    text = cleaned_text;

    if (strlen(text) == 0) {
        free(cleaned_text);
        return PV_STATUS_SUCCESS;
    }

    pv_normalizer_token_list_t *token_list_internal = NULL;
    status = pv_normalizer_token_list_init(&token_list_internal);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_token_list_init,
                pv_status_to_string(status));
        free(cleaned_text);
        return status;
    }

    const mecab_node_t *node = mecab_sparse_tonode(object->mecab, text);
    if (!node) {
        PV_ERROR_REPORT(
                &pv_error_msg_module_function_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("mecab_sparse_tonode"));
        free(cleaned_text);
        pv_normalizer_token_list_delete(token_list_internal);
        return PV_STATUS_RUNTIME_ERROR;
    }

    int32_t node_count = 0;
    const mecab_node_t *ptr = node;
    for (; ptr; ptr = ptr->next) {
        if ((ptr->stat == MECAB_NOR_NODE) || (ptr->stat == MECAB_UNK_NODE)) {
            node_count++;
        }
    }

    if (node_count == 0) {
        free(cleaned_text);
        *token_list = token_list_internal;
        return PV_STATUS_SUCCESS;
    }

    char character[PV_NORMALIZER_MAX_NUM_BYTES_PER_CHARACTER] = {0};

    ptr = node;
    for (; ptr; ptr = ptr->next) {
        if ((ptr->stat != MECAB_NOR_NODE) && (ptr->stat != MECAB_UNK_NODE)) {
            continue;
        }

        int32_t num_bytes_character = 0;
        status = pv_language_num_bytes_character((unsigned char) ptr->surface[0], &num_bytes_character);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT(
                    &pv_error_msg_invalid_argument,
                    PV_ERROR_ARGS_PUBLIC("text"),
                    PV_ERROR_ARGS_PRIVATE_EMPTY());
            free(cleaned_text);
            pv_normalizer_token_list_delete(token_list_internal);
            return status;
        }

        if (num_bytes_character == 1) {
            char c = ptr->surface[0];
            if (c == PV_NORMALIZER_CUSTOM_PRON_OPENING_MARKER) {
                const mecab_node_t *new_ptr = NULL;
                pv_normalizer_token_t *token = NULL;
                status = extract_custom_pron(ptr, &token, &new_ptr);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            extract_custom_pron,
                            pv_status_to_string(status));
                    free(cleaned_text);
                    pv_normalizer_token_list_delete(token_list_internal);
                    return status;
                }
                ptr = new_ptr;
                pv_normalizer_token_list_append_token(token_list_internal, token);
                if (preserve_word_boundary) {
                    token->next_character_is_space = true;
                    status = append_space_token(token_list_internal);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                append_space_token,
                                pv_status_to_string(status));
                        free(cleaned_text);
                        pv_normalizer_token_list_delete(token_list_internal);
                        return status;
                    }
                }
                continue;
            } else if ((c == PV_NORMALIZER_CUSTOM_PRON_SEPARATOR) || (c == PV_NORMALIZER_CUSTOM_PRON_CLOSING_MARKER)) {
                PV_ERROR_REPORT(
                        &pv_error_msg_invalid_argument,
                        PV_ERROR_ARGS_PUBLIC("text"),
                        PV_ERROR_ARGS_PRIVATE_EMPTY());
                free(cleaned_text);
                pv_normalizer_token_list_delete(token_list_internal);
                return PV_STATUS_INVALID_ARGUMENT;
            }
        }

        bool is_punctuation = false;
        if (num_bytes_character == ptr->length) {
            memcpy(character, ptr->surface, ptr->length);
            character[num_bytes_character] = '\0';

            status = pv_normalizer_util_is_punctuation(object->language, character, &is_punctuation);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_util_is_punctuation,
                        pv_status_to_string(status));
                free(cleaned_text);
                pv_normalizer_token_list_delete(token_list_internal);
                return status;
            }
        }

        bool contains_kana, contains_kanji = false;
        status = check_ja_char_composition(
                ptr->surface,
                ptr->length,
                &contains_kana,
                &contains_kanji);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    check_ja_char_composition,
                    pv_status_to_string(status));
            free(cleaned_text);
            pv_normalizer_token_list_delete(token_list_internal);
            return status;
        }

        bool is_alphanum = false;
        bool copy_reading_from_feature = false;
        if (((ptr->char_type == PV_NORMALIZER_TOKENIZER_JA_CHAR_TYPE_KANJI) ||
             (ptr->char_type == PV_NORMALIZER_TOKENIZER_JA_CHAR_TYPE_KANJI_NUMERIC))) {
            copy_reading_from_feature = ptr->stat == MECAB_NOR_NODE;
        } else if ((ptr->char_type == PV_NORMALIZER_TOKENIZER_JA_CHAR_TYPE_HIRAGANA) ||
                   (ptr->char_type == PV_NORMALIZER_TOKENIZER_JA_CHAR_TYPE_KATAKANA)) {
            copy_reading_from_feature = true;
        } else {
            if (contains_kana || contains_kanji) {
                // mecab sometimes tokenizes something like '３つ' and provides a reading
                copy_reading_from_feature = true;
            } else {
                is_alphanum = true;
            }
        }

        if (is_alphanum) {
            char *alphanum_string = NULL;
            const mecab_node_t *new_ptr = NULL;
            status = extract_alphanum_string(
                    ptr,
                    &alphanum_string,
                    &new_ptr);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        extract_alphanum_string,
                        pv_status_to_string(status));
                free(cleaned_text);
                pv_normalizer_token_list_delete(token_list_internal);
                return status;
            }

            pv_normalizer_token_list_t *alphanum_tokens = NULL;
            status = pv_normalizer_tokenizer_generic_tokenize(
                    object->generic_tokenizer,
                    alphanum_string,
                    pv_normalizer_tokenizer_generic_default_word_boundary_character(),
                    preserve_word_boundary,
                    remove_unknown_characters,
                    false,
                    false,
                    &alphanum_tokens);
            free(alphanum_string);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tokenizer_generic_tokenize,
                        pv_status_to_string(status));
                free(cleaned_text);
                pv_normalizer_token_list_delete(token_list_internal);
                return status;
            }

            if (alphanum_tokens) {
                ptr = new_ptr;
                pv_normalizer_token_t *current = alphanum_tokens->head;
                while (current) {
                    if (normalize_token_strings_var) {
                        char *normalized_token_text = NULL;
                        status = pv_normalizer_util_ja_normalize_full_width_text(
                                current->string,
                                &normalized_token_text);
                        if (status != PV_STATUS_SUCCESS) {
                            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                    pv_normalizer_util_ja_normalize_full_width_text,
                                    pv_status_to_string(status));
                            free(cleaned_text);
                            pv_normalizer_token_list_delete(token_list_internal);
                            return status;
                        }
                        free(current->string);
                        current->string = normalized_token_text;
                    }

                    pv_normalizer_token_list_append_token(token_list_internal, current);
                    current = current->next;
                }
                free(alphanum_tokens);
                if (preserve_word_boundary) {
                    if (token_list_internal->tail != NULL) {
                        token_list_internal->tail->next_character_is_space = true;
                    }

                    status = append_space_token(token_list_internal);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                append_space_token,
                                pv_status_to_string(status));
                        free(cleaned_text);
                        pv_normalizer_token_list_delete(token_list_internal);
                        return status;
                    }
                }
            }
            continue;
        }

        char *token_text = calloc(ptr->length + 1, sizeof(char));
        if (!token_text) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("token_text"));
            free(cleaned_text);
            pv_normalizer_token_list_delete(token_list_internal);
            return PV_STATUS_OUT_OF_MEMORY;
        }
        memcpy(token_text, ptr->surface, ptr->length);

        char *normalized_token_text = NULL;
        if (normalize_token_strings_var) {
            status = pv_normalizer_util_ja_normalize_full_width_text(token_text, &normalized_token_text);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_util_ja_normalize_full_width_text,
                        pv_status_to_string(status));
                free(token_text);
                free(cleaned_text);
                pv_normalizer_token_list_delete(token_list_internal);
                return status;
            }
        } else {
            normalized_token_text = malloc(strlen(token_text) + 1);
            if (!normalized_token_text) {
                PV_ERROR_REPORT(
                        &pv_error_msg_alloc,
                        PV_ERROR_ARGS_PUBLIC_EMPTY(),
                        PV_ERROR_ARGS_PRIVATE("normalized_token_text"));
                free(token_text);
                free(cleaned_text);
                pv_normalizer_token_list_delete(token_list_internal);
                return PV_STATUS_OUT_OF_MEMORY;
            }
            strcpy(normalized_token_text, token_text);
        }

        if (normalize_token_strings_var) {
            bool skip_token = pv_normalizer_util_ja_is_skippable_word_separator(normalized_token_text);
            if (skip_token) {
                free(normalized_token_text);
                free(token_text);
                continue;
            }

            char *remapped_string = NULL;
            status = pv_normalizer_util_remap_characters(normalized_token_text, &remapped_string);
            free(normalized_token_text);
            normalized_token_text = NULL;
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_util_remap_characters,
                        pv_status_to_string(status));
                free(token_text);
                free(cleaned_text);
                pv_normalizer_token_list_delete(token_list_internal);
                return status;
            }
            normalized_token_text = remapped_string;
            remapped_string = NULL;
        }

        pv_normalizer_token_t *token = NULL;
        status = pv_normalizer_token_init_with_original_string(
                normalized_token_text,
                token_text,
                is_punctuation,
                preserve_word_boundary,
                0,
                0,
                &token);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_token_init_with_original_string,
                    pv_status_to_string(status));
            free(normalized_token_text);
            free(token_text);
            free(cleaned_text);
            pv_normalizer_token_list_delete(token_list_internal);
            return status;
        }

        const char *reading_src = NULL;
        size_t reading_len = 0;
        if (copy_reading_from_feature) {
            int32_t item_count = 0;
            size_t i = 0, start = 0;
            while (ptr->feature[i] != '\0') {
                if (ptr->feature[i] == ',') {
                    item_count++;
                    if (item_count == PV_TOKENIZER_FEATURE_INDEX) {
                        start = i + 1;
                    } else if (item_count > PV_TOKENIZER_FEATURE_INDEX) {
                        break;
                    }
                }
                i++;
            }

            if ((start > 0) && (i > start)) {
                reading_src = &(ptr->feature[start]);
                reading_len = i - start;
            } else if (!contains_kanji) {
                reading_src = ptr->surface;
                reading_len = ptr->length;
            }
        }

        if (reading_src != NULL && reading_len > 0) {
            char *reading = calloc(reading_len + 1, sizeof(char));
            if (!reading) {
                PV_ERROR_REPORT(
                        &pv_error_msg_alloc,
                        PV_ERROR_ARGS_PUBLIC_EMPTY(),
                        PV_ERROR_ARGS_PRIVATE("reading"));
                pv_normalizer_token_delete(token);
                free(cleaned_text);
                pv_normalizer_token_list_delete(token_list_internal);
                return PV_STATUS_OUT_OF_MEMORY;
            }
            memcpy(reading, reading_src, reading_len);

            // we tag and add a reading for this now because Mecab interprets Japanese words for us
            token->tag = PV_NORMALIZER_TAG_JA_WORD;
            pv_normalizer_token_set_reading(token, reading);
        }

        pv_normalizer_token_list_append_token(token_list_internal, token);

        if (preserve_word_boundary) {
            token->next_character_is_space = true;
            status = append_space_token(token_list_internal);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        append_space_token,
                        pv_status_to_string(status));
                free(cleaned_text);
                pv_normalizer_token_list_delete(token_list_internal);
                return status;
            }
        }
    }
    free(cleaned_text);

    *token_list = token_list_internal;

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_normalizer_tokenizer_ja_tokenize_on_character)(
        const pv_normalizer_tokenizer_ja_t *object,
        const char *text,
        char word_boundary_character,
        bool preserve_word_boundary,
        bool remove_unknown_characters,
        bool split_on_special_characters,
        pv_normalizer_token_list_t **token_list) {
    PV_ASSERT(object);
    PV_ASSERT(text);
    PV_ASSERT(token_list);

    pv_status_t status = pv_normalizer_tokenizer_generic_tokenize(
            object->generic_tokenizer,
            text,
            word_boundary_character,
            preserve_word_boundary,
            remove_unknown_characters,
            split_on_special_characters,
            false,
            token_list);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_tokenizer_generic_tokenize,
                pv_status_to_string(status));
    }

    return status;
}

pv_status_t PV_MOCKABLE(pv_normalizer_tokenizer_ja_token_list_split_verbalized)(
        const pv_normalizer_tokenizer_ja_t *object,
        pv_normalizer_token_list_t *token_list,
        pv_normalizer_token_list_t **split_token_list) {
    PV_ASSERT(object);
    PV_ASSERT(token_list);
    PV_ASSERT(split_token_list);

    pv_status_t status = pv_normalizer_tokenizer_generic_token_list_split_verbalized(
            object->generic_tokenizer,
            token_list,
            split_token_list);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_tokenizer_generic_token_list_split_verbalized,
                pv_status_to_string(status));
    }

    return status;
}

struct pv_normalizer_tokenizer_ja_stream {
    const pv_normalizer_tokenizer_ja_t *tokenizer;
    pv_normalizer_token_list_t *previous_tokens;
    char *text_buffer;
};

pv_status_t PV_MOCKABLE(pv_normalizer_tokenizer_ja_stream_open)(
        pv_normalizer_tokenizer_ja_t *object,
        pv_normalizer_tokenizer_ja_stream_t **stream) {
    PV_ASSERT(object);
    PV_ASSERT(stream);

    *stream = NULL;

    pv_normalizer_tokenizer_ja_stream_t *s = calloc(1, sizeof(pv_normalizer_tokenizer_ja_stream_t));
    if (!s) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("s"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    s->tokenizer = object;

    *stream = s;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_normalizer_tokenizer_ja_stream_close)(pv_normalizer_tokenizer_ja_stream_t *object) {
    if (object) {
        pv_normalizer_token_list_delete(object->previous_tokens);
        free(object->text_buffer);
        free(object);
    }
}

static pv_status_t pv_normalizer_tokenizer_ja_stream_tokenize_current_context(
        pv_normalizer_tokenizer_ja_stream_t *object,
        const char *current_text_buffer,
        bool preserve_word_boundary,
        bool remove_unknown_characters,
        pv_normalizer_token_t *start_token,
        pv_normalizer_token_list_t **token_list) {

    size_t prev_str_len = 0;
    if (start_token != NULL) {
        pv_normalizer_token_t *t = start_token;
        while (t != NULL) {
            if (t->tag_language_agnostic != PV_NORMALIZER_TAG_SPACE) {
                if (t->pronunciation != NULL) {
                    prev_str_len += strlen(t->pronunciation) + 3;
                }
                prev_str_len += strlen(t->string);
            }
            t = t->next;
        }
    }

    char *tokenize_str = calloc(prev_str_len + strlen(current_text_buffer) + 1, sizeof(char));
    if (!tokenize_str) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("tokenize_str"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    if (start_token != NULL) {
        pv_normalizer_token_t *t = start_token;
        while (t != NULL) {
            if (t->tag_language_agnostic != PV_NORMALIZER_TAG_SPACE) {
                if (t->pronunciation != NULL) {
                    memcpy(&tokenize_str[strlen(tokenize_str)], &PV_NORMALIZER_CUSTOM_PRON_OPENING_MARKER, 1);
                    strcat(tokenize_str, t->string);
                    memcpy(&tokenize_str[strlen(tokenize_str)], &PV_NORMALIZER_CUSTOM_PRON_SEPARATOR, 1);
                    strcat(tokenize_str, t->pronunciation);
                    memcpy(&tokenize_str[strlen(tokenize_str)], &PV_NORMALIZER_CUSTOM_PRON_CLOSING_MARKER, 1);
                } else {
                    strcat(tokenize_str, t->string);
                }
            }
            pv_normalizer_token_t *rmv = t;
            t = t->next;
            pv_normalizer_token_list_remove_token(object->previous_tokens, rmv);
        }
    }

    strcat(tokenize_str, current_text_buffer);

    pv_normalizer_token_list_t *new_previous_tokens = NULL;
    pv_status_t status = pv_normalizer_tokenizer_ja_tokenize(
            object->tokenizer,
            tokenize_str,
            preserve_word_boundary,
            remove_unknown_characters,
            false,
            &new_previous_tokens);
    free(tokenize_str);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_tokenizer_ja_tokenize,
                pv_status_to_string(status));
        return status;
    }

    if (object->previous_tokens != NULL) {
        if (object->previous_tokens->size == 0) {
            pv_normalizer_token_list_delete(object->previous_tokens);
        } else {
            *token_list = object->previous_tokens;
        }
    }

    object->previous_tokens = new_previous_tokens;

    return PV_STATUS_SUCCESS;
}

static pv_status_t normalize_token_strings(pv_normalizer_token_list_t *token_list) {
    if ((token_list == NULL) || (token_list->size == 0)) {
        return PV_STATUS_SUCCESS;
    }

    pv_normalizer_token_t *current = token_list->head;
    while (current != NULL) {
        char *normalized_string = NULL;
        pv_status_t status = pv_normalizer_util_ja_normalize_full_width_text(
                current->string,
                &normalized_string);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_util_ja_normalize_full_width_text,
                    pv_status_to_string(status));
            return status;
        }

        char *remapped_string = NULL;
        status = pv_normalizer_util_remap_characters(normalized_string, &remapped_string);
        free(normalized_string);
        normalized_string = NULL;
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_util_remap_characters,
                    pv_status_to_string(status));
            return status;
        }
        normalized_string = remapped_string;
        remapped_string = NULL;

        bool remove_token = pv_normalizer_util_ja_is_skippable_word_separator(normalized_string);
        if (remove_token) {
            free(normalized_string);
            pv_normalizer_token_t *old = current;
            current = current->next;
            pv_normalizer_token_list_remove_token(token_list, old);
        } else {
            free(current->string);
            current->string = normalized_string;
            current = current->next;
        }
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_normalizer_tokenizer_ja_stream_tokenize)(
        pv_normalizer_tokenizer_ja_stream_t *object,
        const char *text,
        bool preserve_word_boundary,
        bool remove_unknown_characters,
        pv_normalizer_token_list_t **token_list) {
    PV_ASSERT(object);
    PV_ASSERT(text);
    PV_ASSERT(token_list);

    *token_list = NULL;

    if (strlen(text) == 0) {
        return PV_STATUS_SUCCESS;
    }

    size_t new_text_buffer_len = object->text_buffer != NULL ? strlen(object->text_buffer) + strlen(text) : strlen(text);

    char *tmp_text_buffer = calloc(new_text_buffer_len + 1, sizeof(char));
    if (!tmp_text_buffer) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("new_text_buffer"));
        return PV_STATUS_OUT_OF_MEMORY;
    }
    if (object->text_buffer != NULL) {
        strcat(tmp_text_buffer, object->text_buffer);
    }
    strcat(tmp_text_buffer, text);

    int32_t char_idx = 0;
    int32_t char_type = -1;
    int32_t next_chars_switch_count = 0;
    int32_t next_chars_same_count = 0;
    while ((tmp_text_buffer[char_idx] != '\0') &&
           (next_chars_switch_count < PV_TOKENIZER_NEXT_CHARS_SWITCH_LIMIT) &&
           (next_chars_same_count < PV_TOKENIZER_NEXT_CHARS_SAME_LIMIT)) {
        int32_t num_bytes = 0;
        pv_status_t status = pv_language_num_bytes_character(
                (unsigned char) tmp_text_buffer[char_idx],
                &num_bytes);
        if (status != PV_STATUS_SUCCESS) {
            free(tmp_text_buffer);
            PV_ERROR_REPORT(
                    &pv_error_msg_invalid_argument,
                    PV_ERROR_ARGS_PUBLIC("text"),
                    PV_ERROR_ARGS_PRIVATE_EMPTY());
            return status;
        }

        int32_t cur_char_type = -1;
        if (num_bytes == 3) {
            int32_t unicode_val =
                    ((tmp_text_buffer[char_idx] & 0x0F) << 12) |
                    ((tmp_text_buffer[char_idx + 1] & 0x3F) << 6) |
                    (tmp_text_buffer[char_idx + 2] & 0x3F);
            if ((unicode_val >= 0x30A0) && (unicode_val <= 0x30FF)) {
                cur_char_type = PV_NORMALIZER_TOKENIZER_JA_CHAR_TYPE_HIRAGANA;
            } else if ((unicode_val >= 0x3040) && (unicode_val <= 0x309F)) {
                cur_char_type = PV_NORMALIZER_TOKENIZER_JA_CHAR_TYPE_KATAKANA;
            } else if ((unicode_val >= 0x4E00) && (unicode_val <= 0x9FFF)) {
                cur_char_type = PV_NORMALIZER_TOKENIZER_JA_CHAR_TYPE_KANJI;
            } else {
                cur_char_type = PV_NORMALIZER_TOKENIZER_JA_CHAR_TYPE_DEFAULT;
            }
        }

        if (char_type == -1) {
            char_type = cur_char_type;
        }

        if (char_type != cur_char_type) {
            next_chars_switch_count++;
            next_chars_same_count = 0;
            char_type = cur_char_type;
        } else {
            next_chars_same_count++;
        }

        char_idx += num_bytes;
    }

    if ((next_chars_switch_count < PV_TOKENIZER_NEXT_CHARS_SWITCH_LIMIT) &&
        (next_chars_same_count < PV_TOKENIZER_NEXT_CHARS_SAME_LIMIT)) {
        free(object->text_buffer);
        object->text_buffer = tmp_text_buffer;
        return PV_STATUS_SUCCESS;
    }

    pv_normalizer_token_t *start_token = NULL;
    if ((object->previous_tokens != NULL) && (object->previous_tokens->size > 0)) {
        pv_normalizer_token_t *tail = object->previous_tokens->tail;
        if ((tail != NULL) && (tail->tag_language_agnostic == PV_NORMALIZER_TAG_SPACE) && (tail->previous != NULL)) {
            tail = tail->previous;
        }

        start_token = pv_normalizer_token_get_nth_token_before(
                tail,
                PV_TOKENIZER_PREVIOUS_TOKENS_COUNT - 1,
                true);
        if (start_token == NULL) {
            start_token = object->previous_tokens->head;
        }
    }

    pv_normalizer_token_list_t *token_list_internal = NULL;
    pv_status_t status = pv_normalizer_tokenizer_ja_stream_tokenize_current_context(
            object,
            tmp_text_buffer,
            preserve_word_boundary,
            remove_unknown_characters,
            start_token,
            &token_list_internal);
    free(tmp_text_buffer);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_tokenizer_ja_stream_tokenize_current_context,
                pv_status_to_string(status));
        return status;
    }

    status = normalize_token_strings(token_list_internal);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                normalize_token_strings,
                pv_status_to_string(status));
        pv_normalizer_token_list_delete(token_list_internal);
        return status;
    }

    *token_list = token_list_internal;
    free(object->text_buffer);
    object->text_buffer = NULL;

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_normalizer_tokenizer_ja_stream_flush)(
        pv_normalizer_tokenizer_ja_stream_t *object,
        bool preserve_word_boundary,
        bool remove_unknown_characters,
        pv_normalizer_token_list_t **token_list) {
    PV_ASSERT(object);
    PV_ASSERT(token_list);

    *token_list = NULL;

    pv_normalizer_token_list_t *remaining_tokens = NULL;
    if (object->text_buffer != NULL) {
        pv_normalizer_token_t *start_token = NULL;
        if (object->previous_tokens && (object->previous_tokens->size > 0)) {
            start_token = object->previous_tokens->head;
        }

        pv_status_t status = pv_normalizer_tokenizer_ja_stream_tokenize_current_context(
                object,
                object->text_buffer,
                preserve_word_boundary,
                remove_unknown_characters,
                start_token,
                &remaining_tokens);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_tokenizer_ja_tokenize,
                    pv_status_to_string(status));
            return status;
        }
        free(object->text_buffer);
        object->text_buffer = NULL;
    }

    if (remaining_tokens != NULL) {
        if (object->previous_tokens != NULL) {
            pv_normalizer_token_list_append_list(object->previous_tokens, remaining_tokens);
        } else {
            object->previous_tokens = remaining_tokens;
        }
    }

    pv_status_t status = normalize_token_strings(object->previous_tokens);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                normalize_token_strings,
                pv_status_to_string(status));
        return status;
    }

    *token_list = object->previous_tokens;
    object->previous_tokens = NULL;

    return PV_STATUS_SUCCESS;
}
