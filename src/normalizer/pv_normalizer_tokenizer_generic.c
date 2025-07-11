#include <stdlib.h>
#include <string.h>

#include "core/pv_assert.h"
#include "core/pv_error_messages.h"
#include "io/pv_log.h"
#include "orca/normalizer/pv_normalizer_token.h"
#include "orca/normalizer/pv_normalizer_tokenizer_generic.h"

#include "orca/normalizer/pv_normalizer_data/pv_normalizer_shared_data.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_normalizer_mock.h"

#endif

static const char PV_TOKENIZER_GENERIC_DEFAULT_WORD_BOUNDARY_CHARACTER = ' ';

struct pv_normalizer_tokenizer_generic {
    pv_normalizer_language_t language;
    const pv_language_info_t *language_info;
};


static pv_status_t pv_normalizer_tokenizer_generic_tokenize_helper(
        const pv_normalizer_tokenizer_generic_t *object,
        int32_t start_index,
        int32_t end_index,
        const char *text,
        bool next_character_is_space,
        bool split_on_special_characters,
        bool split_on_apostrophe,
        pv_normalizer_token_list_t **token_list);

static pv_status_t pv_normalizer_tokenizer_check_hyphen_helper(
        int32_t start_index,
        const char *text,
        bool *is_hyphen);

pv_status_t PV_MOCKABLE(pv_normalizer_tokenizer_generic_init)(
        pv_normalizer_language_t language,
        pv_language_info_t *language_info,
        pv_normalizer_tokenizer_generic_t **object) {
    PV_ASSERT(language_info);
    PV_ASSERT(object);

    *object = NULL;

    pv_normalizer_tokenizer_generic_t *o = calloc(1, sizeof(pv_normalizer_tokenizer_generic_t));
    if (!o) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->language = language;
    o->language_info = language_info;

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_normalizer_tokenizer_generic_delete)(pv_normalizer_tokenizer_generic_t *object) {
    if (object) {
        free(object);
    }
}

pv_status_t PV_MOCKABLE(pv_normalizer_tokenizer_generic_token_list_split_verbalized)(
        const pv_normalizer_tokenizer_generic_t *tokenizer,
        pv_normalizer_token_list_t *token_list,
        pv_normalizer_token_list_t **split_token_list) {
    PV_ASSERT(tokenizer);
    PV_ASSERT(token_list);
    PV_ASSERT(split_token_list);

    *split_token_list = NULL;

    pv_normalizer_token_list_t *out_split_token_list = NULL;
    pv_status_t status = pv_normalizer_token_list_init(&out_split_token_list);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_token_list_init,
                pv_status_to_string(status));
        return status;
    }

    pv_normalizer_token_t *current = token_list->head;
    while (current != NULL) {
        if (current->tag_language_agnostic == PV_NORMALIZER_TAG_CUSTOM_PRONUNCIATION) {
            pv_normalizer_token_t *custom_pron_token = NULL;
            status = pv_normalizer_token_copy(current, &custom_pron_token);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_token_copy,
                        pv_status_to_string(status));
                pv_normalizer_token_list_delete(out_split_token_list);
                return status;
            }

            pv_normalizer_token_list_append_token(out_split_token_list, custom_pron_token);

            current = current->next;
            continue;
        }

        bool is_space =
                (strlen(current->string) == 1) &&
                ((current->string[0] == PV_TOKENIZER_GENERIC_DEFAULT_WORD_BOUNDARY_CHARACTER) &&
                 (current->tag_language_agnostic != PV_NORMALIZER_TAG_CUSTOM_PRONUNCIATION));
        if (is_space) {
            pv_normalizer_token_t *custom_pron_token = NULL;
            status = pv_normalizer_token_copy(current, &custom_pron_token);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_token_copy,
                        pv_status_to_string(status));
                pv_normalizer_token_list_delete(out_split_token_list);
                return status;
            }

            pv_normalizer_token_list_append_token(out_split_token_list, custom_pron_token);

            current = current->next;
            continue;
        }

        pv_normalizer_token_list_t *split_token_list_internal = NULL;
        status = pv_normalizer_tokenizer_generic_tokenize(
                tokenizer,
                current->verbalized,
                PV_TOKENIZER_GENERIC_DEFAULT_WORD_BOUNDARY_CHARACTER,
                true,
                false,
                false,
                false,
                &split_token_list_internal);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_tokenizer_generic_tokenize,
                    pv_status_to_string(status));
            pv_normalizer_token_list_delete(out_split_token_list);
            return status;
        }

        int32_t index_split_token_list_internal = 0;
        pv_normalizer_token_t *current_internal = split_token_list_internal->head;
        while (current_internal != NULL) {
            if (split_token_list_internal->size == 1) {
                pv_normalizer_token_t *split_token = NULL;
                status = pv_normalizer_token_copy(current, &split_token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_token_copy,
                            pv_status_to_string(status));
                    pv_normalizer_token_list_delete(split_token_list_internal);
                    pv_normalizer_token_list_delete(out_split_token_list);
                    return status;
                }

                pv_normalizer_token_list_append_token(out_split_token_list, split_token);
                break;
            }

            pv_normalizer_token_t *split_token = NULL;
            status = pv_normalizer_token_copy(current_internal, &split_token);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_token_copy,
                        pv_status_to_string(status));
                pv_normalizer_token_list_delete(split_token_list_internal);
                pv_normalizer_token_list_delete(out_split_token_list);
                return status;
            }

            free(split_token->original_string);
            split_token->original_string = calloc(strlen(current->original_string) + 1, sizeof(char));
            if (!split_token->original_string) {
                PV_ERROR_REPORT(
                        &pv_error_msg_alloc,
                        PV_ERROR_ARGS_PUBLIC_EMPTY(),
                        PV_ERROR_ARGS_PRIVATE("split_token->original_string"));
                pv_normalizer_token_list_delete(split_token_list_internal);
                pv_normalizer_token_list_delete(out_split_token_list);
                return PV_STATUS_OUT_OF_MEMORY;
            }
            strcpy(split_token->original_string, current->original_string);

            if (current_internal->tag_language_agnostic != PV_NORMALIZER_TAG_SPACE) {
                char *token_verbalized = calloc(strlen(current_internal->string) + 1, sizeof(char));
                if (!token_verbalized) {
                    PV_ERROR_REPORT(
                            &pv_error_msg_alloc,
                            PV_ERROR_ARGS_PUBLIC_EMPTY(),
                            PV_ERROR_ARGS_PRIVATE("token_verbalized"));
                    pv_normalizer_token_list_delete(split_token_list_internal);
                    pv_normalizer_token_list_delete(out_split_token_list);
                    return PV_STATUS_OUT_OF_MEMORY;
                }
                strcpy(token_verbalized, current_internal->string);
                pv_normalizer_token_set_verbalized(split_token, token_verbalized);
            }

            if (split_token->pronunciation) {
                split_token->tag_language_agnostic = PV_NORMALIZER_TAG_CUSTOM_PRONUNCIATION;
            }

            int32_t future_context = split_token_list_internal->size - 1 - index_split_token_list_internal;
            int32_t past_context = index_split_token_list_internal;

            split_token->length_future_context = current->length_future_context + future_context;
            split_token->length_past_context = current->length_past_context + past_context;

            pv_normalizer_token_list_append_token(out_split_token_list, split_token);

            index_split_token_list_internal++;
            current_internal = current_internal->next;
        }
        pv_normalizer_token_list_delete(split_token_list_internal);
        current = current->next;
    }

    *split_token_list = out_split_token_list;
    return PV_STATUS_SUCCESS;
}

static pv_status_t pv_normalizer_tokenizer_generic_append_tokenized_token(
        const pv_normalizer_tokenizer_generic_t *object,
        int32_t token_start_index,
        int32_t token_end_index,
        const char *text,
        const char word_boundary_character,
        bool preserve_word_boundary,
        bool remove_unknown_characters,
        bool split_on_special_characters,
        pv_normalizer_token_list_t *token_list) {
    PV_ASSERT(object);
    PV_ASSERT(token_start_index >= 0);
    PV_ASSERT(token_end_index >= 0);
    PV_ASSERT(token_end_index >= token_start_index);
    PV_ASSERT(text);
    PV_ASSERT(word_boundary_character);
    PV_ASSERT(token_list);

    int32_t token_length = token_end_index - token_start_index;

    char *invalid_text = calloc(token_length + 1, sizeof(char));
    if (!invalid_text) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("invalid_text"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    for (int32_t j = 0; j < token_length; j++) {
        invalid_text[j] = text[token_start_index + j];
    }
    invalid_text[token_length] = '\0';

    char *cleaned_invalid_text = NULL;
    if (remove_unknown_characters) {
        pv_status_t status = pv_normalizer_util_validate_text(
                object->language,
                object->language_info,
                invalid_text,
                preserve_word_boundary,
                false,
                &cleaned_invalid_text);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_util_validate_text,
                    pv_status_to_string(status));
            free(invalid_text);
            return status;
        }
        if (cleaned_invalid_text) {
            free(invalid_text);
            invalid_text = cleaned_invalid_text;
        }
    }

    pv_normalizer_token_list_t *split_token_list_internal = NULL;
    pv_status_t status = pv_normalizer_tokenizer_generic_tokenize(
            object,
            invalid_text,
            word_boundary_character,
            preserve_word_boundary,
            remove_unknown_characters,
            split_on_special_characters,
            false,
            &split_token_list_internal);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_tokenizer_generic_tokenize,
                pv_status_to_string(status));
        free(invalid_text);
        return status;
    }
    free(invalid_text);

    if (split_token_list_internal) {
        pv_normalizer_token_t *current = split_token_list_internal->head;
        while (current) {
            pv_normalizer_token_list_append_token(token_list, current);
            current = current->next;
        }
        free(split_token_list_internal);
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_normalizer_tokenizer_generic_tokenize)(
        const pv_normalizer_tokenizer_generic_t *object,
        const char *text,
        char word_boundary_character,
        bool preserve_word_boundary,
        bool remove_unknown_characters,
        bool split_on_special_characters,
        bool split_on_apostrophe,
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

    if (cleaned_text) {
        text = cleaned_text;
    }

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
        pv_normalizer_token_list_delete(token_list_internal);
        return status;
    }

    const size_t length = strlen(text);

    char character[PV_NORMALIZER_MAX_NUM_BYTES_PER_CHARACTER] = {0};

    int32_t token_start_index = 0;
    int32_t token_end_index = 0;
    bool is_in_custom_pron = false;

    int32_t num_custom_pron_opening_markers = 0;
    int32_t num_custom_pron_closing_markers = 0;
    int32_t num_custom_pron_separator_markers = 0;

    size_t i = 0;
    while (i <= length) {
        bool is_custom_pron_begin = false;
        bool is_custom_pron_end = false;
        bool is_word_boundary = false;
        bool is_eos = false;

        bool is_invalid_custom_pron = false;

        int32_t num_bytes_character = 0;
        status = pv_language_num_bytes_character((unsigned char) text[i], &num_bytes_character);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_language_num_bytes_character,
                    pv_status_to_string(status));
            free(cleaned_text);
            pv_normalizer_token_list_delete(token_list_internal);
            return status;
        }

        if (num_bytes_character == 1) {
            char c = text[i];

            is_custom_pron_begin = (c == PV_NORMALIZER_CUSTOM_PRON_OPENING_MARKER);
            if (is_custom_pron_begin) {
                is_in_custom_pron = true;
                num_custom_pron_opening_markers++;
            }

            is_custom_pron_end = (c == PV_NORMALIZER_CUSTOM_PRON_CLOSING_MARKER);
            if (is_custom_pron_end) {
                is_in_custom_pron = false;
                num_custom_pron_closing_markers++;
            }

            if (c == PV_NORMALIZER_CUSTOM_PRON_SEPARATOR) {
                num_custom_pron_separator_markers++;
            }

            is_word_boundary = (c == word_boundary_character);
            is_eos = (c == '\0') || (i == length);

            if (is_in_custom_pron) {
                // handle inputs with invalid custom pron such as `{never_closed|N EH {hi|HH AY}`
                if (((is_custom_pron_begin || is_eos) && ((token_end_index - token_start_index) > 0))) {
                    is_invalid_custom_pron = true;
                } else {
                    token_end_index += num_bytes_character;
                    i += num_bytes_character;
                    continue;
                }
            }
        }

        for (int32_t j = 0; j < num_bytes_character; j++) {
            character[j] = text[i + j];
        }
        character[num_bytes_character] = '\0';

        bool is_special_character = false;
        if (split_on_special_characters) {
            if (is_eos) {
                is_special_character = false;
            } else {
                status = pv_normalizer_util_is_special_character(
                        object->language,
                        text + token_end_index,
                        &is_special_character,
                        &num_bytes_character);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_util_is_special_character,
                            pv_status_to_string(status));
                    free(cleaned_text);
                    pv_normalizer_token_list_delete(token_list_internal);
                    return status;
                }
            }
        }
        bool is_apostrophe = false;
        if (split_on_apostrophe) {
            if (is_eos) {
                is_apostrophe = false;
            } else {
                is_apostrophe = (strcmp(character, APOSTROPHE) == 0 ||
                                 strcmp(character, LEFT_SINGLE_QUOTATION_MARK) == 0 ||
                                 strcmp(character, RIGHT_SINGLE_QUOTATION_MARK) == 0);
            }
        }

        bool is_punctuation = false;
        status = pv_normalizer_util_is_punctuation(object->language, character, &is_punctuation);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_util_is_punctuation,
                    pv_status_to_string(status));
            free(cleaned_text);
            pv_normalizer_token_list_delete(token_list_internal);
            return status;
        }

        bool is_new_token =
                is_word_boundary ||
                is_eos ||
                is_custom_pron_end ||
                is_invalid_custom_pron;
        if (is_punctuation || is_special_character || is_apostrophe) {
            bool next_character_is_space =
                    (((token_end_index + 1) < (int32_t) length) && (text[token_end_index + 1] == ' '));
            bool next_is_eos = (((token_end_index + 1) == (int32_t) length) || (text[token_end_index + 1] == '\0'));
            if (next_character_is_space || next_is_eos) {
                is_new_token = true;
            }
        }
        if (!is_new_token) {
            token_end_index += num_bytes_character;
        } else {
            int32_t token_length = token_end_index - token_start_index;

            if (is_invalid_custom_pron) {
                if (remove_unknown_characters) {
                    status = pv_normalizer_tokenizer_generic_append_tokenized_token(
                            object,
                            token_start_index,
                            token_end_index,
                            text,
                            word_boundary_character,
                            preserve_word_boundary,
                            true,
                            split_on_special_characters,
                            token_list_internal);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_normalizer_tokenizer_generic_append_tokenized_token,
                                pv_status_to_string(status));
                        free(cleaned_text);
                        pv_normalizer_token_list_delete(token_list_internal);
                        return status;
                    }
                } else {
                    PV_ERROR_REPORT(
                            &pv_error_msg_invalid_argument,
                            PV_ERROR_ARGS_PUBLIC("text"),
                            PV_ERROR_ARGS_PRIVATE_EMPTY());
                    free(cleaned_text);
                    pv_normalizer_token_list_delete(token_list_internal);
                    return PV_STATUS_INVALID_ARGUMENT;
                }

            } else {
                if (token_length > 0) {
                    bool next_character_is_space =
                            ((token_end_index < (int32_t) length) && (text[token_end_index] == ' '));
                    bool next_character_is_hyphen = false;
                    if (token_end_index < (int32_t) length) {
                        status = pv_normalizer_tokenizer_check_hyphen_helper(token_end_index, text, &next_character_is_hyphen);
                        if (status != PV_STATUS_SUCCESS) {
                            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                    pv_normalizer_tokenizer_check_hyphen_helper,
                                    pv_status_to_string(status));
                            free(cleaned_text);
                            pv_normalizer_token_list_delete(token_list_internal);
                            return status;
                        }
                    }

                    if (is_custom_pron_end) {
                        pv_normalizer_token_t *token = NULL;
                        status = pv_normalizer_token_init(
                                token_start_index,
                                token_end_index - 1,
                                text,
                                false,
                                true,
                                next_character_is_space,
                                &token);
                        if ((status == PV_STATUS_INVALID_ARGUMENT) && remove_unknown_characters) {
                            status = pv_normalizer_tokenizer_generic_append_tokenized_token(
                                    object,
                                    token_start_index,
                                    token_end_index,
                                    text,
                                    word_boundary_character,
                                    preserve_word_boundary,
                                    remove_unknown_characters,
                                    split_on_special_characters,
                                    token_list_internal);
                            if (status != PV_STATUS_SUCCESS) {
                                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                        pv_normalizer_tokenizer_generic_append_tokenized_token,
                                        pv_status_to_string(status));
                                free(cleaned_text);
                                pv_normalizer_token_list_delete(token_list_internal);
                                return status;
                            }

                        } else if (status != PV_STATUS_SUCCESS) {
                            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                    pv_normalizer_token_init,
                                    pv_status_to_string(status));
                            free(cleaned_text);
                            pv_normalizer_token_list_delete(token_list_internal);
                            return status;
                        } else {
                            pv_normalizer_token_list_append_token(token_list_internal, token);
                        }
                    } else {
                        pv_normalizer_token_list_t *token_list_same_original = NULL;
                        status = pv_normalizer_tokenizer_generic_tokenize_helper(
                                object,
                                token_start_index,
                                token_end_index - 1,
                                text,
                                next_character_is_space,
                                split_on_special_characters,
                                split_on_apostrophe,
                                &token_list_same_original);
                        if (status != PV_STATUS_SUCCESS) {
                            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                    pv_normalizer_tokenizer_generic_tokenize_helper,
                                    pv_status_to_string(status));
                            free(cleaned_text);
                            pv_normalizer_token_list_delete(token_list_internal);
                            return status;
                        }
                        pv_normalizer_token_list_append_list(token_list_internal, token_list_same_original);
                        token_list_internal->tail->next_character_is_hyphen = next_character_is_hyphen;
                        token_list_same_original = NULL;
                    }
                }

                bool next_character_is_space =
                        (((token_end_index + 1) < (int32_t) length) && (text[token_end_index + 1] == ' '));
                bool next_character_is_hyphen = false;
                if ((token_end_index + 1) < (int32_t) length) {
                    status = pv_normalizer_tokenizer_check_hyphen_helper(token_end_index + 1, text, &next_character_is_hyphen);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_normalizer_tokenizer_check_hyphen_helper,
                                pv_status_to_string(status));
                        free(cleaned_text);
                        pv_normalizer_token_list_delete(token_list_internal);
                        return status;
                    }
                }

                if (is_word_boundary && preserve_word_boundary) {
                    pv_normalizer_token_t *token = NULL;
                    status = pv_normalizer_token_init(
                            token_end_index,
                            token_end_index,
                            text,
                            false,
                            false,
                            next_character_is_space,
                            &token);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_normalizer_token_init,
                                pv_status_to_string(status));
                        free(cleaned_text);
                        pv_normalizer_token_list_delete(token_list_internal);
                        return status;
                    }

                    pv_normalizer_token_list_append_token(token_list_internal, token);
                }
                if (is_punctuation || is_special_character || is_apostrophe) {
                    token_start_index = token_end_index;
                    int32_t offset = (num_bytes_character > 1) ? num_bytes_character - 1 : 0;
                    pv_normalizer_token_t *single_character_token = NULL;
                    status = pv_normalizer_token_init(
                            token_start_index,
                            token_end_index + offset,
                            text,
                            is_punctuation,
                            false,
                            next_character_is_space,
                            &single_character_token);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_normalizer_token_init,
                                pv_status_to_string(status));
                        free(cleaned_text);
                        pv_normalizer_token_list_delete(token_list_internal);
                        return status;
                    }
                    single_character_token->next_character_is_hyphen = next_character_is_hyphen;
                    pv_normalizer_token_list_append_token(token_list_internal, single_character_token);
                }
            }

            if (!is_invalid_custom_pron) {
                token_start_index = (token_end_index + num_bytes_character);
            } else {
                token_start_index = token_end_index;
            }
            token_end_index = token_start_index;
        }

        if (!is_invalid_custom_pron) {
            i += num_bytes_character;
        } else {
            is_invalid_custom_pron = false;
        }
    }
    free(cleaned_text);

    bool is_invalid_num_custom_pron_markers =
            (num_custom_pron_opening_markers != num_custom_pron_closing_markers) ||
            (num_custom_pron_opening_markers != num_custom_pron_separator_markers);

    if (is_invalid_num_custom_pron_markers && !remove_unknown_characters) {
        PV_ERROR_REPORT(
                &pv_error_msg_orca_invalid_custom_pronunciation_format,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        pv_normalizer_token_list_delete(token_list_internal);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    *token_list = token_list_internal;

    return PV_STATUS_SUCCESS;
}

char PV_MOCKABLE(pv_normalizer_tokenizer_generic_default_word_boundary_character)(void) {
    return PV_TOKENIZER_GENERIC_DEFAULT_WORD_BOUNDARY_CHARACTER;
}

pv_status_t pv_normalizer_tokenizer_check_hyphen_helper(
        int32_t start_index,
        const char *text,
        bool *is_hyphen) {
    PV_ASSERT(start_index >= 0);
    PV_ASSERT(text);
    PV_ASSERT(is_hyphen);

    int32_t num_bytes_character = 0;
    pv_status_t status = pv_language_num_bytes_character((unsigned char) text[start_index], &num_bytes_character);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_language_num_bytes_character,
                pv_status_to_string(status));
        return status;
    }

    char character[PV_NORMALIZER_MAX_NUM_BYTES_PER_CHARACTER] = {0};
    for (int32_t j = 0; j < num_bytes_character; j++) {
        character[j] = text[start_index + j];
    }
    character[num_bytes_character] = '\0';

    *is_hyphen = strcmp(character, HYPHEN) == 0 ||
                 strcmp(character, MINUS_SIGN) == 0 ||
                 strcmp(character, EN_DASH) == 0 ||
                 strcmp(character, FIGURE_DASH) == 0;
    return PV_STATUS_SUCCESS;
}

static pv_status_t pv_normalizer_tokenizer_generic_tokenize_helper(
        const pv_normalizer_tokenizer_generic_t *object,
        int32_t start_index,
        int32_t end_index,
        const char *text,
        bool next_character_is_space,
        bool split_on_special_characters,
        bool split_on_apostrophe,
        pv_normalizer_token_list_t **token_list) {

    PV_ASSERT(object);
    PV_ASSERT(start_index >= 0);
    PV_ASSERT(end_index >= start_index);
    PV_ASSERT(text);
    PV_ASSERT(token_list);

    *token_list = NULL;
    pv_normalizer_token_list_t *token_list_internal = NULL;
    pv_status_t status = pv_normalizer_token_list_init(&token_list_internal);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_token_list_init,
                pv_status_to_string(status));
        pv_normalizer_token_list_delete(token_list_internal);
        return status;
    }

    char *unsplit_original_string = NULL;
    status = pv_normalizer_token_read_text_segment(start_index, end_index + 1, text, &unsplit_original_string);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_token_read_text_segment,
                pv_status_to_string(status));
        pv_normalizer_token_list_delete(token_list_internal);
        return status;
    }

    int32_t token_start_index = start_index;
    int32_t token_end_index = start_index;
    int32_t current_num_token = 0;
    bool last_is_punctuation = false;

    int32_t i = start_index;
    while (i <= end_index) {
        char character[PV_NORMALIZER_MAX_NUM_BYTES_PER_CHARACTER] = {0};
        int32_t num_bytes_character = 0;
        status = pv_language_num_bytes_character((unsigned char) text[i], &num_bytes_character);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT(
                    &pv_error_msg_invalid_argument,
                    PV_ERROR_ARGS_PUBLIC("text"),
                    PV_ERROR_ARGS_PRIVATE_EMPTY());
            free(unsplit_original_string);
            pv_normalizer_token_list_delete(token_list_internal);
            return status;
        }

        for (int32_t j = 0; j < num_bytes_character; j++) {
            character[j] = text[i + j];
        }
        character[num_bytes_character] = '\0';

        bool is_special_character = false;
        if (split_on_special_characters) {
            status = pv_normalizer_util_is_special_character(
                    object->language,
                    text + token_end_index,
                    &is_special_character,
                    &num_bytes_character);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_util_is_special_character,
                        pv_status_to_string(status));
                free(unsplit_original_string);
                pv_normalizer_token_list_delete(token_list_internal);
                return status;
            }
        }

        bool is_apostrophe = false;
        if (split_on_apostrophe) {
            is_apostrophe = (strcmp(character, APOSTROPHE) == 0 ||
                             strcmp(character, LEFT_SINGLE_QUOTATION_MARK) == 0 ||
                             strcmp(character, RIGHT_SINGLE_QUOTATION_MARK) == 0);
        }

        bool is_punctuation = false;
        status = pv_normalizer_util_is_punctuation(object->language, character, &is_punctuation);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_util_is_punctuation,
                    pv_status_to_string(status));
            free(unsplit_original_string);
            pv_normalizer_token_list_delete(token_list_internal);
            return status;
        }
        last_is_punctuation = is_punctuation;

        bool is_new_token = is_punctuation || is_special_character || is_apostrophe;
        if (!is_new_token) {
            token_end_index += num_bytes_character;
        } else {
            int32_t token_length = token_end_index - token_start_index;
            if (token_length > 0) {
                char *original_string = calloc((strlen(unsplit_original_string) + 1), sizeof(char));
                if (!original_string) {
                    PV_ERROR_REPORT(
                            &pv_error_msg_alloc,
                            PV_ERROR_ARGS_PUBLIC_EMPTY(),
                            PV_ERROR_ARGS_PRIVATE("original_string"));
                    free(unsplit_original_string);
                    pv_normalizer_token_list_delete(token_list_internal);
                    return PV_STATUS_OUT_OF_MEMORY;
                }
                strcpy(original_string, unsplit_original_string);

                char *string = NULL;
                status = pv_normalizer_token_read_text_segment(token_start_index, token_end_index, text, &string);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_token_read_text_segment,
                            pv_status_to_string(status));
                    free(unsplit_original_string);
                    pv_normalizer_token_list_delete(token_list_internal);
                    free(original_string);
                    return status;
                }

                char *remapped_string = NULL;
                status = pv_normalizer_util_remap_characters(string, &remapped_string);
                free(string);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_util_remap_characters,
                            pv_status_to_string(status));
                    free(unsplit_original_string);
                    pv_normalizer_token_list_delete(token_list_internal);
                    free(original_string);
                    return status;
                }

                pv_normalizer_token_t *new_token = NULL;
                status = pv_normalizer_token_init_with_original_string(
                        remapped_string,
                        original_string,
                        false,
                        false,
                        1,
                        current_num_token,
                        &new_token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_token_list_init,
                            pv_status_to_string(status));
                    free(unsplit_original_string);
                    free(original_string);
                    free(remapped_string);
                    pv_normalizer_token_list_delete(token_list_internal);
                    return status;
                }
                current_num_token += 1;
                pv_normalizer_token_list_append_token(token_list_internal, new_token);
                token_start_index = token_end_index;
            }

            char *original_string = calloc((strlen(unsplit_original_string) + 1), sizeof(char));
            if (!original_string) {
                PV_ERROR_REPORT(
                        &pv_error_msg_alloc,
                        PV_ERROR_ARGS_PUBLIC_EMPTY(),
                        PV_ERROR_ARGS_PRIVATE("original_string"));
                free(unsplit_original_string);
                pv_normalizer_token_list_delete(token_list_internal);
                return PV_STATUS_OUT_OF_MEMORY;
            }
            strcpy(original_string, unsplit_original_string);

            char *string = NULL;
            int32_t offset = (num_bytes_character > 1) ? num_bytes_character - 1 : 0;

            bool next_character_is_hyphen = false;
            if ((token_end_index + 1) <= (int32_t) end_index) {
                status = pv_normalizer_tokenizer_check_hyphen_helper(token_end_index + offset + 1, text, &next_character_is_hyphen);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_tokenizer_check_hyphen_helper,
                            pv_status_to_string(status));
                    free(unsplit_original_string);
                    pv_normalizer_token_list_delete(token_list_internal);
                    return status;
                }
            }

            status = pv_normalizer_token_read_text_segment(token_start_index, token_end_index + offset + 1, text, &string);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_token_read_text_segment,
                        pv_status_to_string(status));
                free(unsplit_original_string);
                pv_normalizer_token_list_delete(token_list_internal);
                free(original_string);
                return status;
            }

            char *remapped_string = NULL;
            status = pv_normalizer_util_remap_characters(string, &remapped_string);
            free(string);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_util_remap_characters,
                        pv_status_to_string(status));
                free(unsplit_original_string);
                pv_normalizer_token_list_delete(token_list_internal);
                free(original_string);
                return status;
            }

            pv_normalizer_token_t *new_token = NULL;
            status = pv_normalizer_token_init_with_original_string(
                    remapped_string,
                    original_string,
                    is_punctuation,
                    false,
                    1,
                    current_num_token,
                    &new_token);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_token_init_with_original_string,
                        pv_status_to_string(status));
                free(unsplit_original_string);
                pv_normalizer_token_list_delete(token_list_internal);
                free(original_string);
                free(remapped_string);
                return status;
            }
            current_num_token += 1;
            pv_normalizer_token_list_append_token(token_list_internal, new_token);
            new_token->next_character_is_hyphen = next_character_is_hyphen;
            token_end_index += num_bytes_character;

            token_start_index = token_end_index;
        }
        i += num_bytes_character;
    }

    // take care of the last token
    if (token_end_index != token_start_index) {
        int32_t token_length = token_end_index - token_start_index;
        char *original_string = calloc((strlen(unsplit_original_string) + 1), sizeof(char));
        if (!original_string) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("original_string"));
            free(unsplit_original_string);
            pv_normalizer_token_list_delete(token_list_internal);
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(original_string, unsplit_original_string);

        char *string = NULL;
        status = pv_normalizer_token_read_text_segment(token_start_index, token_end_index, text, &string);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_token_read_text_segment,
                    pv_status_to_string(status));
            free(unsplit_original_string);
            pv_normalizer_token_list_delete(token_list_internal);
            free(original_string);
            return status;
        }

        char *remapped_string = NULL;
        status = pv_normalizer_util_remap_characters(string, &remapped_string);
        free(string);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_util_remap_characters,
                    pv_status_to_string(status));
            free(unsplit_original_string);
            pv_normalizer_token_list_delete(token_list_internal);
            free(original_string);
            return status;
        }

        pv_normalizer_token_t *new_token = NULL;
        status = pv_normalizer_token_init_with_original_string(
                remapped_string,
                original_string,
                (token_length > 1) ? false : last_is_punctuation,
                next_character_is_space,
                1,
                current_num_token,
                &new_token);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_token_init_with_original_string,
                    pv_status_to_string(status));
            free(unsplit_original_string);
            pv_normalizer_token_list_delete(token_list_internal);
            free(original_string);
            free(remapped_string);
            return status;
        }

        pv_normalizer_token_list_append_token(token_list_internal, new_token);
    }

    //update length_future_context
    pv_normalizer_token_t *current = token_list_internal->head;
    int32_t token_list_size = token_list_internal->size;
    for (int32_t j = 0; j < token_list_internal->size; j++) {
        current->length_future_context = token_list_size - j - 1;
        current = current->next;
    }

    free(unsplit_original_string);
    token_list_internal->tail->next_character_is_space = next_character_is_space;
    *token_list = token_list_internal;
    return PV_STATUS_SUCCESS;
}

struct pv_normalizer_tokenizer_generic_stream {
    const pv_normalizer_tokenizer_generic_t *tokenizer;
};

pv_status_t PV_MOCKABLE(pv_normalizer_tokenizer_generic_stream_open)(
        pv_normalizer_tokenizer_generic_t *object,
        pv_normalizer_tokenizer_generic_stream_t **stream) {
    PV_ASSERT(object);
    PV_ASSERT(stream);

    *stream = NULL;

    pv_normalizer_tokenizer_generic_stream_t *s = calloc(1, sizeof(pv_normalizer_tokenizer_generic_stream_t));
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

void PV_MOCKABLE(pv_normalizer_tokenizer_generic_stream_close)(pv_normalizer_tokenizer_generic_stream_t *object) {
    if (object) {
        free(object);
    }
}

pv_status_t PV_MOCKABLE(pv_normalizer_tokenizer_generic_stream_tokenize_initial)(
        pv_normalizer_tokenizer_generic_stream_t *object,
        const char *text,
        char word_boundary_character,
        bool preserve_word_boundary,
        bool remove_unknown_characters,
        bool split_on_special_characters,
        pv_normalizer_token_list_t **token_list) {
    PV_ASSERT(object);
    PV_ASSERT(text);
    PV_ASSERT(token_list);

    return pv_normalizer_tokenizer_generic_tokenize(
            object->tokenizer,
            text,
            word_boundary_character,
            preserve_word_boundary,
            remove_unknown_characters,
            split_on_special_characters,
            false,
            token_list);
}

pv_status_t PV_MOCKABLE(pv_normalizer_tokenizer_generic_stream_tokenize_verbalizable)(
        pv_normalizer_tokenizer_generic_stream_t *object,
        const pv_normalizer_token_list_t *verbalizable_tokens,
        char word_boundary_character,
        bool preserve_word_boundary,
        bool remove_unknown_characters,
        bool split_on_special_characters,
        pv_normalizer_token_list_t **token_list) {
    PV_ASSERT(object);
    PV_ASSERT(verbalizable_tokens);
    PV_ASSERT(token_list);

    char *text = NULL;
    pv_status_t status = pv_normalizer_token_list_to_string_text(
            verbalizable_tokens,
            &text);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_token_list_to_string_text,
                pv_status_to_string(status));
        return status;
    }
    status = pv_normalizer_tokenizer_generic_tokenize(
            object->tokenizer,
            text,
            word_boundary_character,
            preserve_word_boundary,
            remove_unknown_characters,
            split_on_special_characters,
            true,
            token_list);
    free(text);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_tokenizer_generic_tokenize,
                pv_status_to_string(status));
    }
    return status;
}
