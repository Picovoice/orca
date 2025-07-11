#include <stdlib.h>
#include <string.h>

#include "core/picovoice.h"
#include "core/pv_assert.h"
#include "core/pv_error_messages.h"
#include "orca/normalizer/pv_normalizer_stream.h"
#include "orca/normalizer/pv_normalizer_stream_context_scanner.h"
#include "orca/normalizer/pv_normalizer_util.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_normalizer_mock.h"

#endif

static const size_t MAX_CUSTOM_PRON_LENGTH = 256;

typedef struct pv_normalizer_stream_custom_pron {
    bool is_in_custom_pron;
    bool is_in_phoneme_string;

    char *text_buffer;
} pv_normalizer_stream_custom_pron_t;

struct pv_normalizer_stream {
    pv_normalizer_token_list_t *raw_token_list;
    pv_normalizer_token_list_t *normalized_token_list;

    const pv_normalizer_t *normalizer;
    const int32_t *use_cases;
    int32_t num_use_cases;

    pv_normalizer_stream_context_scanner_t *context_scanner;
    pv_normalizer_tokenizer_stream_t *tokenizer_stream;
    pv_normalizer_stream_custom_pron_t *custom_pron;
};

pv_status_t PV_MOCKABLE(pv_normalizer_stream_open)(const pv_normalizer_t *normalizer, pv_normalizer_stream_t **object) {
    PV_ASSERT(normalizer);
    PV_ASSERT(object);

    *object = NULL;

    pv_normalizer_stream_t *o = calloc(1, sizeof(pv_normalizer_stream_t));
    if (!o) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->normalizer = normalizer;

    pv_status_t status = pv_normalizer_get_use_cases_from_language(
            pv_normalizer_get_language(normalizer),
            &(o->num_use_cases),
            &(o->use_cases));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_get_use_cases_from_language,
                pv_status_to_string(status));
        pv_normalizer_stream_close(o);
        return status;
    }

    status = pv_normalizer_token_list_init(&(o->raw_token_list));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_token_list_init,
                pv_status_to_string(status));
        pv_normalizer_stream_close(o);
        return status;
    }

    status = pv_normalizer_token_list_init(&(o->normalized_token_list));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_token_list_init,
                pv_status_to_string(status));
        pv_normalizer_stream_close(o);
        return status;
    }

    status = pv_normalizer_stream_context_scanner_init(pv_normalizer_get_language(normalizer), &(o->context_scanner));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_stream_context_scanner_init,
                pv_status_to_string(status));
        pv_normalizer_stream_close(o);
        return status;
    }

    status = pv_normalizer_tokenizer_stream_open(pv_normalizer_get_tokenizer(normalizer), &(o->tokenizer_stream));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_tokenizer_stream_open,
                pv_status_to_string(status));
        pv_normalizer_stream_close(o);
        return status;
    }

    o->custom_pron = calloc(1, sizeof(pv_normalizer_stream_custom_pron_t));
    if (!(o->custom_pron)) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o->custom_pron"));
        pv_normalizer_stream_close(o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_normalizer_stream_close)(pv_normalizer_stream_t *object) {
    if (object) {
        pv_normalizer_tokenizer_stream_close(object->tokenizer_stream);
        pv_normalizer_stream_context_scanner_delete(object->context_scanner);
        pv_normalizer_token_list_delete(object->normalized_token_list);
        pv_normalizer_token_list_delete(object->raw_token_list);

        if (object->custom_pron) {
            free(object->custom_pron->text_buffer);
        }
        free(object->custom_pron);

        free(object);
    }
}

static void pv_normalizer_stream_reset_custom_pron_state(pv_normalizer_stream_custom_pron_t *object) {
    PV_ASSERT(object);

    if (object->text_buffer) {
        free(object->text_buffer);
        object->text_buffer = NULL;
    }
    object->is_in_custom_pron = false;
    object->is_in_phoneme_string = false;
}

void PV_MOCKABLE(pv_normalizer_stream_reset)(pv_normalizer_stream_t *object) {
    PV_ASSERT(object);

    pv_normalizer_token_list_reset(object->normalized_token_list);
    pv_normalizer_token_list_reset(object->raw_token_list);
    pv_normalizer_stream_reset_custom_pron_state(object->custom_pron);
}

static pv_normalizer_token_t *pv_normalizer_token_get_first_nonverbalized_token(
        const pv_normalizer_token_list_t *token_list) {
    PV_ASSERT(token_list);

    pv_normalizer_token_t *first_nonverbalizable_token = NULL;
    pv_normalizer_token_t *current = token_list->tail;
    while (current) {
        if (current->is_verbalizable) {
            break;
        } else {
            first_nonverbalizable_token = current;
        }
        current = current->previous;
    }

    return first_nonverbalizable_token;
}

static pv_status_t pv_normalizer_stream_get_next_verbalizable_tokens(
        const pv_normalizer_stream_t *object,
        pv_normalizer_token_list_t **verbalizable_token_list) {
    PV_ASSERT(object);
    PV_ASSERT(verbalizable_token_list);

    *verbalizable_token_list = NULL;

    pv_normalizer_token_t *first_nonverbalized_tokens =
            pv_normalizer_token_get_first_nonverbalized_token(object->raw_token_list);

    if (!first_nonverbalized_tokens) {
        return PV_STATUS_SUCCESS;
    }

    pv_normalizer_token_t *last_verbalizable_token = NULL;
    pv_normalizer_token_t *current = first_nonverbalized_tokens;

    bool is_verbalizable;

    while (current) {
        pv_status_t status = pv_normalizer_stream_context_scanner_is_verbalizable(
                object->context_scanner,
                current,
                object->num_use_cases,
                object->use_cases,
                &is_verbalizable);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_stream_context_scanner_is_verbalizable,
                    pv_status_to_string(status));
            return status;
        }
        if (is_verbalizable) {
            current->is_verbalizable = true;
            last_verbalizable_token = current;
        }

        current = current->next;
    }

    pv_normalizer_token_list_t *verbalizable_token_list_internal = NULL;
    if (last_verbalizable_token) {
        pv_status_t status = pv_normalizer_token_list_copy_portion(
                object->raw_token_list,
                first_nonverbalized_tokens,
                last_verbalizable_token->next,
                &verbalizable_token_list_internal);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_token_list_copy_portion,
                    pv_status_to_string(status));
            return status;
        }
    }

    *verbalizable_token_list = verbalizable_token_list_internal;

    return PV_STATUS_SUCCESS;
}

static pv_status_t pv_normalizer_stream_process_new_tokens(
        pv_normalizer_stream_t *object,
        pv_normalizer_token_list_t *incoming_token_list) {
    PV_ASSERT(object);
    PV_ASSERT(incoming_token_list);

    pv_normalizer_remove_invalid_custom_pron_markers(incoming_token_list);

    int32_t num_tokens_processed = object->raw_token_list->size;
    pv_normalizer_token_list_append_list(object->raw_token_list, incoming_token_list);
    incoming_token_list = NULL;

    pv_status_t status = pv_normalizer_tagger_tag(
            pv_normalizer_get_tagger(object->normalizer),
            object->raw_token_list,
            num_tokens_processed,
            false);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_tagger_tag,
                pv_status_to_string(status));
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static pv_status_t pv_normalizer_stream_buffer_custom_pron(
        pv_normalizer_stream_custom_pron_t *object,
        pv_normalizer_language_t language,
        pv_language_info_t *language_info,
        const char *text,
        char **new_text) {
    PV_ASSERT(object);
    PV_ASSERT(language_info);
    PV_ASSERT(text);
    PV_ASSERT(new_text);

    size_t text_length = strlen(text);
    size_t start_index = 0;

    *new_text = NULL;
    char *output_text = calloc(1, text_length + 1);
    if (!output_text) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("output_text"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    char character[PV_NORMALIZER_MAX_NUM_BYTES_PER_CHARACTER] = {0};

    int32_t num_bytes_character = 0;
    for (size_t i = 0; i < text_length; i += num_bytes_character) {
        num_bytes_character = 0;
        pv_status_t status = pv_language_num_bytes_character(text[i], &num_bytes_character);
        if (status != PV_STATUS_SUCCESS || num_bytes_character == 0) {
            PV_ERROR_REPORT(
                    &pv_error_msg_orca_invalid_custom_pronunciation,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE_EMPTY());
            return PV_STATUS_INVALID_ARGUMENT;
        }

        for (int32_t j = 0; j < num_bytes_character; j++) {
            character[j] = text[i + j];
        }
        character[num_bytes_character] = '\0';

        char c = text[i];

        if (object->is_in_custom_pron && !object->is_in_phoneme_string) {
            bool is_normalizable_character = false;
            status = pv_normalizer_util_is_normalizable_character(
                    language,
                    character,
                    &is_normalizable_character);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_util_is_normalizable_character,
                        pv_status_to_string(status));
                free(output_text);
                return status;
            }

            if (!is_normalizable_character || c == '\n') {
                PV_ERROR_REPORT(
                        &pv_error_msg_orca_invalid_character,
                        PV_ERROR_ARGS_PUBLIC(character),
                        PV_ERROR_ARGS_PRIVATE_EMPTY());
                free(output_text);
                return PV_STATUS_INVALID_ARGUMENT;
            }
        } else if (object->is_in_phoneme_string) {
            if (c == ' ' || c == PV_NORMALIZER_CUSTOM_PRON_CLOSING_MARKER) {
                bool found_phoneme = false;

                int32_t text_phoneme_length = 0;
                for (int32_t j = (int32_t) i - 1; j >= 0; j--) {
                    if (text[j] == ' ' || text[j] == PV_NORMALIZER_CUSTOM_PRON_SEPARATOR) {
                        found_phoneme = true;
                        break;
                    }
                    text_phoneme_length++;
                }

                size_t text_buffer_length = (object->text_buffer != NULL) ? strlen(object->text_buffer) : 0;
                int32_t text_buffer_phoneme_length = 0;
                if (object->text_buffer != NULL && !found_phoneme) {
                    for (int32_t j = (int32_t) text_buffer_length - 1; j >= 0; j--) {
                        if (object->text_buffer[j] == ' ' || object->text_buffer[j] == PV_NORMALIZER_CUSTOM_PRON_SEPARATOR) {
                            break;
                        }
                        text_buffer_phoneme_length++;
                    }
                }

                if ((text_phoneme_length + text_buffer_phoneme_length) > 0) {
                    char *phoneme = calloc(1, text_phoneme_length + text_buffer_phoneme_length + 1);
                    if (!phoneme) {
                        PV_ERROR_REPORT(
                                &pv_error_msg_alloc,
                                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                                PV_ERROR_ARGS_PRIVATE("phoneme"));
                        free(output_text);
                        return PV_STATUS_OUT_OF_MEMORY;
                    }
                    if (text_buffer_phoneme_length > 0) {
                        strncpy(phoneme,
                                object->text_buffer + text_buffer_length - text_buffer_phoneme_length,
                                text_buffer_phoneme_length);
                    }
                    if (text_phoneme_length > 0) {
                        strncpy(phoneme + text_buffer_phoneme_length,
                                text + i - text_phoneme_length,
                                text_phoneme_length);
                    }

                    int32_t phoneme_index;
                    status = pv_language_info_phoneme_index_from_string(
                            language_info,
                            phoneme,
                            &phoneme_index);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT(
                                &pv_error_msg_orca_invalid_custom_pronunciation_phoneme,
                                PV_ERROR_ARGS_PUBLIC(phoneme),
                                PV_ERROR_ARGS_PRIVATE_EMPTY());
                        free(output_text);
                        free(phoneme);
                        return status;
                    }
                    free(phoneme);
                } else {
                    PV_ERROR_REPORT(
                            &pv_error_msg_orca_invalid_custom_pronunciation_phoneme,
                            PV_ERROR_ARGS_PUBLIC(character),
                            PV_ERROR_ARGS_PRIVATE_EMPTY());
                    free(output_text);
                    return PV_STATUS_INVALID_ARGUMENT;
                }
            }
        }

        if (c == PV_NORMALIZER_CUSTOM_PRON_OPENING_MARKER) {
            object->is_in_custom_pron = true;
            start_index = i;
        } else if (c == PV_NORMALIZER_CUSTOM_PRON_CLOSING_MARKER) {
            if (object->is_in_custom_pron && object->is_in_phoneme_string) {
                size_t past_text_length = (object->text_buffer != NULL) ? strlen(object->text_buffer) : 0;
                size_t custom_pron_length = (i - start_index) + 1;
                size_t new_text_length = past_text_length + strlen(output_text) + custom_pron_length;
                char *combined_text = calloc(1, new_text_length + 1);
                if (!combined_text) {
                    PV_ERROR_REPORT(
                            &pv_error_msg_alloc,
                            PV_ERROR_ARGS_PUBLIC_EMPTY(),
                            PV_ERROR_ARGS_PRIVATE("combined_text"));
                    free(output_text);
                    return PV_STATUS_OUT_OF_MEMORY;
                }
                if (object->text_buffer) {
                    strcpy(combined_text, object->text_buffer);
                }
                strcat(combined_text, output_text);
                strncat(combined_text, text + start_index, custom_pron_length);
                free(output_text);
                output_text = combined_text;

                pv_normalizer_stream_reset_custom_pron_state(object);
            } else {
                PV_ERROR_REPORT(
                        &pv_error_msg_orca_invalid_custom_pronunciation_format,
                        PV_ERROR_ARGS_PUBLIC_EMPTY(),
                        PV_ERROR_ARGS_PRIVATE_EMPTY());
                free(output_text);
                return PV_STATUS_INVALID_ARGUMENT;
            }
        } else if (c == PV_NORMALIZER_CUSTOM_PRON_SEPARATOR) {
            if (object->is_in_custom_pron) {
                object->is_in_phoneme_string = true;
            } else {
                strcat(output_text, character);
            }
        } else {
            if (!object->is_in_custom_pron) {
                strcat(output_text, character);
            }
        }
    }

    if (object->is_in_custom_pron) {
        size_t new_text_buffer_length = text_length - start_index;
        if (object->text_buffer != NULL) {
            new_text_buffer_length += strlen(object->text_buffer);
        }

        if (new_text_buffer_length > MAX_CUSTOM_PRON_LENGTH) {
            PV_ERROR_REPORT(
                    &pv_error_msg_orca_invalid_custom_pronunciation_format,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE_EMPTY());
            free(output_text);
            return PV_STATUS_INVALID_ARGUMENT;
        }

        char *combined_text_buffer = calloc(1, new_text_buffer_length + 1);
        if (!combined_text_buffer) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("combined_text_buffer"));
            free(output_text);
            return PV_STATUS_OUT_OF_MEMORY;
        }

        if (object->text_buffer != NULL) {
            strcpy(combined_text_buffer, object->text_buffer);
        }
        strcat(combined_text_buffer, text + start_index);

        free(object->text_buffer);
        object->text_buffer = combined_text_buffer;
    }

    if (strlen(output_text) > 0) {
        *new_text = output_text;
    } else {
        free(output_text);
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_normalizer_stream_add_to_input_buffer)(
        pv_normalizer_stream_t *object,
        const char *text,
        pv_normalizer_token_list_t **verbalizable_token_list) {
    PV_ASSERT(object);
    PV_ASSERT(text);
    PV_ASSERT(verbalizable_token_list);

    *verbalizable_token_list = NULL;

    bool is_empty = (strlen(text) == 0) || (text[0] == '\0');
    if (is_empty) {
        return PV_STATUS_SUCCESS;
    }

    char *text_buffer = NULL;
    pv_status_t status = pv_normalizer_stream_buffer_custom_pron(
            object->custom_pron,
            pv_normalizer_get_language(object->normalizer),
            pv_normalizer_get_language_info(object->normalizer),
            text,
            &text_buffer);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_stream_buffer_custom_pron,
                pv_status_to_string(status));
        pv_normalizer_stream_reset_custom_pron_state(object->custom_pron);
        free(text_buffer);
        return status;
    }

    if (!text_buffer) {
        return PV_STATUS_SUCCESS;
    }

    pv_normalizer_token_list_t *incoming_token_list = NULL;
    status = pv_normalizer_tokenizer_stream_tokenize_initial(
            object->tokenizer_stream,
            text_buffer,
            pv_normalizer_tokenizer_default_word_boundary_character(pv_normalizer_get_tokenizer(object->normalizer)),
            true,
            true,
            false,
            &incoming_token_list);
    free(text_buffer);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_tokenizer_stream_tokenize_initial,
                pv_status_to_string(status));
        return status;
    }

    if (!incoming_token_list) {
        return PV_STATUS_SUCCESS;
    }

    status = pv_normalizer_stream_process_new_tokens(object, incoming_token_list);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_stream_process_new_tokens,
                pv_status_to_string(status));
        return status;
    }

    status = pv_normalizer_stream_get_next_verbalizable_tokens(object, verbalizable_token_list);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_stream_get_next_verbalizable_tokens,
                pv_status_to_string(status));
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static pv_status_t pv_normalizer_stream_flush_input_buffer(pv_normalizer_stream_t *object) {
    PV_ASSERT(object);

    if (object->custom_pron->text_buffer != NULL) {
        pv_normalizer_token_list_t *incoming_token_list = NULL;
        pv_status_t status = pv_normalizer_tokenizer_stream_tokenize_initial(
                object->tokenizer_stream,
                object->custom_pron->text_buffer,
                pv_normalizer_tokenizer_default_word_boundary_character(pv_normalizer_get_tokenizer(object->normalizer)),
                true,
                true,
                false,
                &incoming_token_list);

        pv_normalizer_stream_reset_custom_pron_state(object->custom_pron);

        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_tokenizer_stream_tokenize_initial,
                    pv_status_to_string(status));
            return status;
        }

        status = pv_normalizer_stream_process_new_tokens(object, incoming_token_list);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_stream_process_new_tokens,
                    pv_status_to_string(status));
            return status;
        }
    }

    pv_normalizer_token_list_t *flushed_token_list = NULL;
    pv_status_t status = pv_normalizer_tokenizer_stream_flush(
            object->tokenizer_stream,
            true,
            true,
            &flushed_token_list);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_tokenizer_stream_flush,
                pv_status_to_string(status));
        return status;
    }

    if (!flushed_token_list) {
        return PV_STATUS_SUCCESS;
    }

    status = pv_normalizer_stream_process_new_tokens(object, flushed_token_list);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_stream_process_new_tokens,
                pv_status_to_string(status));
        return status;
    }

    return PV_STATUS_SUCCESS;
}


static pv_status_t pv_normalizer_stream_verbalize_new_tokens(
        pv_normalizer_stream_t *object,
        const pv_normalizer_token_list_t *verbalizable_tokens,
        pv_normalizer_token_list_t **phonemizable_token_list) {
    PV_ASSERT(object);
    PV_ASSERT(verbalizable_tokens);
    PV_ASSERT(phonemizable_token_list);

    pv_normalizer_tokenizer_t *tokenizer = pv_normalizer_get_tokenizer(object->normalizer);

    pv_normalizer_token_list_t *new_token_list = NULL;
    pv_status_t status = pv_normalizer_tokenizer_stream_tokenize_verbalizable(
            object->tokenizer_stream,
            verbalizable_tokens,
            pv_normalizer_tokenizer_default_word_boundary_character(tokenizer),
            true,
            true,
            false,
            &new_token_list);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_tokenizer_stream_tokenize_verbalizable,
                pv_status_to_string(status));
        return status;
    }

    if (!new_token_list) {
        return PV_STATUS_SUCCESS;
    }

    pv_normalizer_stream_context_scanner_remove_hyphen_only_tokens(object->context_scanner, new_token_list);

    pv_normalizer_remove_invalid_custom_pron_markers(new_token_list);

    int32_t num_tokens_processed = object->normalized_token_list->size;

    if (num_tokens_processed == 0) {
        pv_normalizer_token_t *head = new_token_list->head;

        while (head != NULL) {
            bool is_space = strcmp(head->string, " ") == 0;
            bool is_eos_punctuation = pv_normalizer_is_valid_eos(
                    pv_normalizer_get_language(object->normalizer),
                    head->string);
            if (!(is_eos_punctuation || is_space)) {
                break;
            }
            pv_normalizer_token_t *tmp = head;
            head = head->next;
            pv_normalizer_token_list_remove_token(new_token_list, tmp);
        }
    }

    if (new_token_list->size == 0) {
        pv_normalizer_token_list_delete(new_token_list);
        return PV_STATUS_SUCCESS;
    }

    pv_normalizer_token_list_append_list(object->normalized_token_list, new_token_list);

    status = pv_normalizer_tagger_tag(
            pv_normalizer_get_tagger(object->normalizer),
            object->normalized_token_list,
            num_tokens_processed,
            true);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_tagger_tag,
                pv_status_to_string(status));
        return status;
    }

    status = pv_normalizer_verbalizer_verbalize(
            pv_normalizer_get_verbalizer(object->normalizer),
            object->normalized_token_list,
            num_tokens_processed);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_verbalizer_verbalize,
                pv_status_to_string(status));
        return status;
    }

    pv_normalizer_token_t *new_tokens_head = object->normalized_token_list->head;
    for (int32_t i = 0; i < num_tokens_processed; i++) {
        new_tokens_head = new_tokens_head->next;
        PV_ASSERT(new_tokens_head);
    }

    pv_normalizer_token_list_t *phonemizable_token_list_internal = NULL;
    status = pv_normalizer_token_list_copy_portion(
            object->normalized_token_list,
            new_tokens_head,
            NULL,
            &phonemizable_token_list_internal);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_token_list_copy_portion,
                pv_status_to_string(status));
        return status;
    }

    pv_normalizer_token_list_remove_nonverbalized_tokens(phonemizable_token_list_internal);

    if (phonemizable_token_list_internal->size == 0) {
        pv_normalizer_token_list_delete(phonemizable_token_list_internal);
        return PV_STATUS_SUCCESS;
    }

    pv_normalizer_token_list_t *split_token_list = NULL;
    status = pv_normalizer_tokenizer_token_list_split_verbalized(
            tokenizer,
            phonemizable_token_list_internal,
            &split_token_list);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_tokenizer_token_list_split_verbalized,
                pv_status_to_string(status));
        pv_normalizer_token_list_delete(phonemizable_token_list_internal);
        return status;
    }
    if (split_token_list) {
        pv_normalizer_token_list_delete(phonemizable_token_list_internal);
        phonemizable_token_list_internal = split_token_list;
    }

    pv_normalizer_token_list_remove_extra_spaces(phonemizable_token_list_internal);

    *phonemizable_token_list = phonemizable_token_list_internal;

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_normalizer_stream_add)(
        pv_normalizer_stream_t *object,
        const char *text,
        pv_normalizer_token_list_t **phonemizable_token_list) {
    PV_ASSERT(object);
    PV_ASSERT(text);
    PV_ASSERT(phonemizable_token_list);

    *phonemizable_token_list = NULL;

    bool is_empty = (strlen(text) == 0) || (text[0] == '\0');
    if (is_empty) {
        return PV_STATUS_SUCCESS;
    }

    pv_normalizer_token_list_t *verbalizable_token_list = NULL;
    pv_status_t status = pv_normalizer_stream_add_to_input_buffer(
            object,
            text,
            &verbalizable_token_list);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_stream_add_to_input_buffer,
                pv_status_to_string(status));
        return status;
    }

    if (!verbalizable_token_list) {
        return PV_STATUS_SUCCESS;
    }

    pv_normalizer_token_list_t *phonemizable_token_list_internal = NULL;
    status = pv_normalizer_stream_verbalize_new_tokens(
            object,
            verbalizable_token_list,
            &phonemizable_token_list_internal);
    pv_normalizer_token_list_delete(verbalizable_token_list);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_stream_verbalize_new_tokens,
                pv_status_to_string(status));
        return status;
    }

    *phonemizable_token_list = phonemizable_token_list_internal;

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_normalizer_stream_flush_verbalizable_tokens)(
        pv_normalizer_stream_t *object,
        pv_normalizer_token_list_t **verbalizable_token_list) {
    PV_ASSERT(object);
    PV_ASSERT(verbalizable_token_list);

    *verbalizable_token_list = NULL;

    pv_status_t status = pv_normalizer_stream_flush_input_buffer(object);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_stream_flush_input_buffer,
                pv_status_to_string(status));
        return status;
    }

    pv_normalizer_token_t *first_nonverbalized_tokens =
            pv_normalizer_token_get_first_nonverbalized_token(object->raw_token_list);

    if (!first_nonverbalized_tokens) {
        return PV_STATUS_SUCCESS;
    }

    pv_normalizer_token_list_t *verbalizable_token_list_internal = NULL;
    status = pv_normalizer_token_list_copy_portion(
            object->raw_token_list,
            first_nonverbalized_tokens,
            NULL,
            &verbalizable_token_list_internal);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_token_list_copy_portion,
                pv_status_to_string(status));
        return status;
    }

    *verbalizable_token_list = verbalizable_token_list_internal;

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_normalizer_stream_flush)(
        pv_normalizer_stream_t *object,
        pv_normalizer_token_list_t **phonemizable_token_list) {
    PV_ASSERT(object);
    PV_ASSERT(phonemizable_token_list);

    *phonemizable_token_list = NULL;

    pv_normalizer_token_list_t *verbalizable_tokens = NULL;
    pv_status_t status = pv_normalizer_stream_flush_verbalizable_tokens(
            object,
            &verbalizable_tokens);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_stream_flush_verbalizable_tokens,
                pv_status_to_string(status));
        return status;
    }

    if (!verbalizable_tokens) {
        return PV_STATUS_SUCCESS;
    }

    pv_normalizer_token_list_t *phonemizable_token_list_internal = NULL;
    status = pv_normalizer_stream_verbalize_new_tokens(
            object,
            verbalizable_tokens,
            &phonemizable_token_list_internal);
    pv_normalizer_token_list_delete(verbalizable_tokens);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_stream_verbalize_new_tokens,
                pv_status_to_string(status));
        return status;
    }

    *phonemizable_token_list = phonemizable_token_list_internal;

    return PV_STATUS_SUCCESS;
}
