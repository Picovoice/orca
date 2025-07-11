#include <stdlib.h>
#include <string.h>

#include "core/pv_assert.h"
#include "core/pv_error_messages.h"
#include "orca/normalizer/pv_normalizer_token.h"
#include "orca/normalizer/pv_normalizer_tokenizer.h"
#include "orca/normalizer/pv_normalizer_util.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_normalizer_mock.h"

#endif

static pv_status_t is_valid_pronunciation(const char *token) {
    PV_ASSERT(token);

    if (token[0] == ' ') {
        PV_ERROR_REPORT(
                &pv_error_msg_orca_invalid_custom_pronunciation_format,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }

    size_t length = strlen(token);

    if (token[length - 1] == ' ') {
        PV_ERROR_REPORT(
                &pv_error_msg_orca_invalid_custom_pronunciation_format,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_normalizer_token_init)(
        const int32_t start_index,
        const int32_t end_index,
        const char *text,
        bool is_punctuation,
        bool has_pronunciation,
        bool next_character_is_space,
        pv_normalizer_token_t **object) {
    PV_ASSERT(start_index >= 0);
    PV_ASSERT(end_index >= 0);
    PV_ASSERT(end_index >= start_index);
    PV_ASSERT(text);
    PV_ASSERT(object);

    *object = NULL;

    pv_normalizer_token_t *o = calloc(1, sizeof(pv_normalizer_token_t));
    if (!o) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    char *string = NULL;
    char *pronunciation = NULL;

    if (has_pronunciation) {
        pv_status_t status = pv_normalizer_token_read_custom_pronunciation(
                start_index,
                end_index + 1,
                text,
                &string,
                &pronunciation);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_token_read_custom_pronunciation,
                    pv_status_to_string(status));
            free(o);
            return status;
        }
    } else {
        pv_status_t status = pv_normalizer_token_read_text_segment(start_index, end_index + 1, text, &string);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_token_read_text_segment,
                    pv_status_to_string(status));
            free(o);
            return status;
        }
    }

    o->original_string = calloc(strlen(string) + 1, sizeof(char));
    if (!o->original_string) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o->original_string"));
        free(string);
        free(o);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    strcpy(o->original_string, string);

    o->string = string;

    o->pronunciation = pronunciation;
    o->verbalized = NULL;
    o->reading = NULL;

    o->length_future_context = 0;
    o->length_past_context = 0;
    o->is_verbalizable = false;
    o->tag_data_index = -1;
    o->next_character_is_space = next_character_is_space;

    o->next = NULL;
    o->previous = NULL;

    if (is_punctuation) {
        o->tag_language_agnostic = PV_NORMALIZER_TAG_PUNCTUATION;
    } else if (has_pronunciation) {
        o->tag_language_agnostic = PV_NORMALIZER_TAG_CUSTOM_PRONUNCIATION;
    } else {
        o->tag_language_agnostic = PV_NORMALIZER_TAG_NONE;
    }
    o->tag = 0;

    *object = o;

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_normalizer_token_init_with_original_string)(
        char *string,
        char *original_string,
        bool is_punctuation,
        bool next_character_is_space,
        int32_t length_future_context,
        int32_t length_past_context,
        pv_normalizer_token_t **object) {
    PV_ASSERT(string);
    PV_ASSERT(original_string);
    PV_ASSERT(object);

    *object = NULL;

    pv_normalizer_token_t *o = calloc(1, sizeof(pv_normalizer_token_t));
    if (!o) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->string = string;
    o->original_string = original_string;
    o->verbalized = NULL;
    o->reading = NULL;
    o->pronunciation = NULL;

    o->length_future_context = length_future_context;
    o->length_past_context = length_past_context;
    o->is_verbalizable = false;
    o->tag_data_index = -1;
    o->next_character_is_space = next_character_is_space;

    o->next = NULL;
    o->previous = NULL;

    if (is_punctuation) {
        o->tag_language_agnostic = PV_NORMALIZER_TAG_PUNCTUATION;
    } else {
        o->tag_language_agnostic = PV_NORMALIZER_TAG_NONE;
    }
    o->tag = 0;

    *object = o;

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_normalizer_token_copy)(pv_normalizer_token_t *source, pv_normalizer_token_t **destination) {
    PV_ASSERT(source);
    PV_ASSERT(destination);

    *destination = NULL;

    pv_normalizer_token_t *o = calloc(1, sizeof(pv_normalizer_token_t));
    if (!o) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    char *string = calloc(strlen(source->string) + 1, sizeof(char));
    if (!string) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("string"));
        free(o);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    strcpy(string, source->string);
    o->string = string;

    o->original_string = calloc(strlen(source->original_string) + 1, sizeof(char));
    if (!o->original_string) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o->original_string"));
        pv_normalizer_token_delete(o);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    strcpy(o->original_string, source->original_string);

    o->verbalized = NULL;
    if (source->verbalized) {
        o->verbalized = calloc(strlen(source->verbalized) + 1, sizeof(char));
        if (!o->verbalized) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("o->verbalized"));
            pv_normalizer_token_delete(o);
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(o->verbalized, source->verbalized);
    }

    o->reading = NULL;
    if (source->reading) {
        o->reading = calloc(strlen(source->reading) + 1, sizeof(char));
        if (!o->reading) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("o->verbalized"));
            pv_normalizer_token_delete(o);
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(o->reading, source->reading);
    }

    o->pronunciation = NULL;
    if (source->pronunciation) {
        char *pronunciation = calloc(strlen(source->pronunciation) + 1, sizeof(char));
        if (!pronunciation) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("o->pronunciation"));
            pv_normalizer_token_delete(o);
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(pronunciation, source->pronunciation);
        o->pronunciation = pronunciation;
    }

    o->tag = source->tag;
    o->tag_language_agnostic = source->tag_language_agnostic;
    o->length_future_context = source->length_future_context;
    o->length_past_context = source->length_past_context;
    o->is_verbalizable = source->is_verbalizable;
    o->tag_data_index = source->tag_data_index;
    o->next_character_is_space = source->next_character_is_space;

    o->next = NULL;
    o->previous = NULL;

    *destination = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_normalizer_token_set_verbalized)(
        pv_normalizer_token_t *object,
        char *new_verbalized) {
    PV_ASSERT(object);
    if (object->verbalized) {
        free(object->verbalized);
    }
    object->verbalized = new_verbalized;
}

void PV_MOCKABLE(pv_normalizer_token_set_reading)(
        pv_normalizer_token_t *object,
        char *new_reading) {
    PV_ASSERT(object);
    if (object->reading) {
        free(object->reading);
    }
    object->reading = new_reading;
}

void PV_MOCKABLE(pv_normalizer_token_delete)(pv_normalizer_token_t *object) {
    if (object) {
        free((char *) object->reading);
        free((char *) object->verbalized);
        free((char *) object->pronunciation);
        free((char *) object->original_string);
        free((char *) object->string);
        free(object);
    }
}

pv_status_t PV_MOCKABLE(pv_normalizer_token_read_text_segment)(
        int32_t start_index,
        int32_t end_index,
        const char *text,
        char **segment) {
    PV_ASSERT(start_index >= 0);
    PV_ASSERT(end_index >= 0);
    PV_ASSERT(end_index >= start_index);
    PV_ASSERT(text);
    PV_ASSERT(segment);

    *segment = NULL;

    const size_t length = end_index - start_index;
    char *t = malloc((length + 1) * sizeof(char));
    if (!t) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("t"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memcpy(t, text + start_index, length);
    t[length] = '\0';

    *segment = t;

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_normalizer_token_read_custom_pronunciation)(
        int32_t start_index,
        int32_t end_index,
        const char *text,
        char **segment,
        char **pronunciation) {
    PV_ASSERT(start_index >= 0);
    PV_ASSERT(end_index >= 0);
    PV_ASSERT(end_index >= start_index);
    PV_ASSERT(text);
    PV_ASSERT(segment);
    PV_ASSERT(pronunciation);

    *segment = NULL;
    *pronunciation = NULL;

    int32_t opening_custom_pron_index = -1;
    for (int32_t i = (int32_t) start_index; i <= (int32_t) end_index; i++) {
        if (text[i] == PV_NORMALIZER_CUSTOM_PRON_OPENING_MARKER) {
            opening_custom_pron_index = i;
            break;
        }
    }

    if (opening_custom_pron_index == -1) {
        PV_ERROR_REPORT(
                &pv_error_msg_orca_invalid_custom_pronunciation_format,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }

    int32_t separator_index = 0;
    for (int32_t i = start_index; i <= end_index; i++) {
        if (text[i] == PV_NORMALIZER_CUSTOM_PRON_SEPARATOR) {
            separator_index = i;
            break;
        }
    }

    if (separator_index == 0) {
        PV_ERROR_REPORT(
                &pv_error_msg_orca_invalid_custom_pronunciation_format,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }

    int32_t closing_custom_pron_index = 0;
    for (int32_t i = start_index; i <= end_index; i++) {
        if (text[i] == PV_NORMALIZER_CUSTOM_PRON_CLOSING_MARKER) {
            closing_custom_pron_index = i;
            break;
        }
    }

    if ((closing_custom_pron_index == 0) || (closing_custom_pron_index <= (separator_index + 1))) {
        PV_ERROR_REPORT(
                &pv_error_msg_orca_invalid_custom_pronunciation_format,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }

    char *segment_internal = NULL;
    pv_status_t status = pv_normalizer_token_read_text_segment(
            start_index + 1,
            separator_index,
            text,
            &segment_internal);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_token_read_text_segment,
                pv_status_to_string(status));
        return status;
    }

    char *pronunciation_internal = NULL;
    status = pv_normalizer_token_read_text_segment(
            separator_index + 1,
            end_index,
            text,
            &pronunciation_internal);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_token_read_text_segment,
                pv_status_to_string(status));
        free(segment_internal);
        return status;
    }

    status = is_valid_pronunciation(pronunciation_internal);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                is_valid_pronunciation,
                pv_status_to_string(status));
        free(segment_internal);
        free(pronunciation_internal);
        return status;
    }

    *segment = segment_internal;
    *pronunciation = pronunciation_internal;

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_normalizer_token_list_init)(pv_normalizer_token_list_t **object) {
    PV_ASSERT(object);

    *object = NULL;
    pv_normalizer_token_list_t *o = calloc(1, sizeof(pv_normalizer_token_list_t));
    if (!o) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->size = 0;
    o->head = NULL;
    o->tail = NULL;

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_normalizer_token_list_delete)(pv_normalizer_token_list_t *object) {
    if (object) {
        pv_normalizer_token_list_reset(object);
        free(object);
    }
}

void PV_MOCKABLE(pv_normalizer_token_list_reset)(pv_normalizer_token_list_t *object) {
    PV_ASSERT(object);

    pv_normalizer_token_t *current = object->head;
    while (current) {
        pv_normalizer_token_t *next = current->next;
        pv_normalizer_token_delete(current);
        current = next;
    }

    object->size = 0;
    object->head = NULL;
    object->tail = NULL;
}

pv_status_t PV_MOCKABLE(pv_normalizer_token_list_copy)(
        const pv_normalizer_token_list_t *object,
        pv_normalizer_token_list_t **list) {
    PV_ASSERT(object);
    PV_ASSERT(list);

    *list = NULL;

    if (object->size == 0) {
        return PV_STATUS_SUCCESS;
    }

    return pv_normalizer_token_list_copy_portion(
            object,
            object->head,
            NULL,
            list);
}

pv_status_t PV_MOCKABLE(pv_normalizer_token_list_copy_portion)(
        const pv_normalizer_token_list_t *object,
        pv_normalizer_token_t *start,
        pv_normalizer_token_t *end,
        pv_normalizer_token_list_t **list_copy) {
    PV_ASSERT(object);
    PV_ASSERT(start);
    PV_ASSERT(list_copy);

    *list_copy = NULL;

    if (object->size == 0) {
        return PV_STATUS_SUCCESS;
    }

    pv_normalizer_token_list_t *token_list_internal = NULL;
    pv_status_t status = pv_normalizer_token_list_init(&token_list_internal);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_token_list_init,
                pv_status_to_string(status));
        return status;
    }

    pv_normalizer_token_t *current = start;
    while (current != end) {
        pv_normalizer_token_t *new_token = NULL;
        status = pv_normalizer_token_copy(current, &new_token);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_token_copy,
                    pv_status_to_string(status));
            pv_normalizer_token_list_delete(token_list_internal);
            return status;
        }

        pv_normalizer_token_list_append_token(token_list_internal, new_token);

        current = current->next;
    }

    *list_copy = token_list_internal;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_normalizer_token_list_append_token)(
        pv_normalizer_token_list_t *object,
        pv_normalizer_token_t *token) {
    PV_ASSERT(object);
    PV_ASSERT(token);

    if (object->tail) {
        token->previous = object->tail;
        object->tail->next = token;
        object->tail = token;
    } else {
        object->head = token;
        object->tail = token;
        token->previous = NULL;
    }

    object->size++;
}

void PV_MOCKABLE(pv_normalizer_token_list_append_list)(
        pv_normalizer_token_list_t *object,
        pv_normalizer_token_list_t *list) {
    PV_ASSERT(object);
    PV_ASSERT(list);

    pv_normalizer_token_t *current = list->head;
    while (current) {
        pv_normalizer_token_list_append_token(object, current);
        current = current->next;
    }
    free(list);
}

void PV_MOCKABLE(pv_normalizer_token_list_remove_token)(
        pv_normalizer_token_list_t *token_list,
        pv_normalizer_token_t *token) {
    PV_ASSERT(token_list);
    PV_ASSERT(token);

    pv_normalizer_token_t *previous_token = token->previous;
    pv_normalizer_token_t *next_token = token->next;

    if (previous_token == NULL) {
        PV_ASSERT(token_list->head == token);
        token_list->head = next_token;
    } else {
        previous_token->next = next_token;
    }

    if (next_token == NULL) {
        PV_ASSERT(token_list->tail == token);
        token_list->tail = previous_token;
    } else {
        next_token->previous = previous_token;
    }

    token_list->size--;

    pv_normalizer_token_delete(token);
}

void PV_MOCKABLE(pv_normalizer_token_list_replace_token_with_list)(
        pv_normalizer_token_list_t *original_token_list,
        pv_normalizer_token_t **original_token,
        pv_normalizer_token_list_t *new_token_list) {
    PV_ASSERT(original_token_list);
    PV_ASSERT(original_token);
    PV_ASSERT(new_token_list);

    pv_normalizer_token_t *previous_token = (*original_token)->previous;
    pv_normalizer_token_t *old_token = *original_token;

    if (previous_token != NULL && previous_token->length_future_context > 0) {
        pv_normalizer_token_t *next_token = new_token_list->head;
        previous_token->length_future_context += next_token->length_future_context;
        while (next_token != NULL) {
            next_token->length_past_context += previous_token->length_past_context + 1;
            next_token = next_token->next;
        }
    }

    pv_normalizer_token_t *current = new_token_list->head;
    while (current != NULL && current->next != NULL) {
        current = current->next;
    }
    if (current != NULL) {
        current->next = (*original_token)->next;
    }

    if (original_token_list->head == *original_token) {
        original_token_list->head = new_token_list->head;
    }
    if (original_token_list->tail == *original_token) {
        original_token_list->tail = new_token_list->tail;
    }

    *original_token = new_token_list->head;

    if (previous_token != NULL) {
        (previous_token)->next = *original_token;
        (*original_token)->previous = previous_token;
    }

    original_token_list->size += (new_token_list->size - 1);

    pv_normalizer_token_delete(old_token);
}

void PV_MOCKABLE(pv_normalizer_token_list_insert_token)(
        pv_normalizer_token_list_t *token_list,
        pv_normalizer_token_t *token,
        pv_normalizer_token_t *token_to_insert) {
    PV_ASSERT(token_list);
    PV_ASSERT(token);
    PV_ASSERT(token_to_insert);

    pv_normalizer_token_t *next = token->next;

    token->next = token_to_insert;

    if (next != NULL) {
        next->previous = token_to_insert;
    } else {
        token_list->tail = token_to_insert;
    }

    token_to_insert->next = next;
    token_to_insert->previous = token;

    token_list->size++;
}

void PV_MOCKABLE(pv_normalizer_token_list_remove_extra_spaces)(pv_normalizer_token_list_t *token_list) {
    PV_ASSERT(token_list);

    bool previous_is_space = false;
    pv_normalizer_token_t *current = token_list->head;
    while (current) {
        pv_normalizer_token_t *next = current->next;

        bool current_is_space = (current->tag_language_agnostic == PV_NORMALIZER_TAG_SPACE);
        if (current_is_space && previous_is_space) {
            pv_normalizer_token_list_remove_token(token_list, current);
        }

        previous_is_space = current_is_space;
        current = next;
    }
}

void PV_MOCKABLE(pv_normalizer_token_list_remove_nonverbalized_tokens)(pv_normalizer_token_list_t *token_list) {
    PV_ASSERT(token_list);

    pv_normalizer_token_t *current = token_list->head;
    while (current) {
        pv_normalizer_token_t *next = current->next;
        if (!current->verbalized && !current->reading) {
            bool is_space =
                    (strlen(current->string) == 1) &&
                    (current->string[0] == ' ');
            if (!is_space && (current->tag_language_agnostic != PV_NORMALIZER_TAG_CUSTOM_PRONUNCIATION)) {
                pv_normalizer_token_list_remove_token(token_list, current);
            }
        }
        current = next;
    }
}

void PV_MOCKABLE(pv_normalizer_token_list_remove_space_tokens)(pv_normalizer_token_list_t *token_list) {
    PV_ASSERT(token_list);

    pv_normalizer_token_t *current = token_list->head;
    while (current) {
        pv_normalizer_token_t *next = current->next;

        if (current->tag_language_agnostic == PV_NORMALIZER_TAG_SPACE) {
            pv_normalizer_token_list_remove_token(token_list, current);
        }

        current = next;
    }
}

pv_status_t PV_MOCKABLE(pv_normalizer_token_list_to_string_text)(
        const pv_normalizer_token_list_t *token_list,
        char **text) {
    PV_ASSERT(token_list);
    PV_ASSERT(text);

    *text = NULL;

    if (token_list->size == 0) {
        return PV_STATUS_SUCCESS;
    }

    size_t text_length = 0;

    pv_normalizer_token_t *current = token_list->head;
    const char *custom_pron_markers = "{|}";
    while (current != NULL) {
        if (current->tag_language_agnostic != PV_NORMALIZER_TAG_CUSTOM_PRONUNCIATION) {
            text_length += strlen(current->string);
        } else {
            text_length += strlen(custom_pron_markers);
            text_length += strlen(current->string);
            text_length += strlen(current->pronunciation);
        }
        current = current->next;
    }

    char *text_internal = calloc(text_length + 1, sizeof(char));
    if (!text_internal) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("text_internal"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    current = token_list->head;
    while (current != NULL) {
        if (current->tag_language_agnostic != PV_NORMALIZER_TAG_CUSTOM_PRONUNCIATION) {
            strcat(text_internal, current->string);
        } else {
            memcpy(&text_internal[strlen(text_internal)], &PV_NORMALIZER_CUSTOM_PRON_OPENING_MARKER, 1);
            strcat(text_internal, current->string);
            memcpy(&text_internal[strlen(text_internal)], &PV_NORMALIZER_CUSTOM_PRON_SEPARATOR, 1);
            strcat(text_internal, current->pronunciation);
            memcpy(&text_internal[strlen(text_internal)], &PV_NORMALIZER_CUSTOM_PRON_CLOSING_MARKER, 1);
        }
        current = current->next;
    }

    *text = text_internal;

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_normalizer_token_list_to_verbalized_text)(
        const pv_normalizer_token_list_t *token_list,
        char seperator_character,
        char **text) {
    PV_ASSERT(token_list);
    PV_ASSERT(text);

    if (token_list->size == 0) {
        return PV_STATUS_SUCCESS;
    }

    *text = NULL;

    size_t text_length = 0;

    pv_normalizer_token_t *current = token_list->head;
    while (current != NULL) {
        if ((current->verbalized != NULL) && (strcmp(current->verbalized, "") != 0)) {
            text_length += strlen(current->verbalized);
            if (seperator_character) {
                text_length += 1;
            }
        }
        current = current->next;
    }

    char *text_internal = calloc(text_length + 1, sizeof(char));
    if (!text_internal) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("text_internal"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    current = token_list->head;
    while (current != NULL) {
        if ((current->verbalized != NULL) && (strcmp(current->verbalized, "") != 0)) {
            strcat(text_internal, current->verbalized);
            if (seperator_character) {
                memcpy(&text_internal[strlen(text_internal)], &seperator_character, 1);
            }
        }
        current = current->next;
    }

    if (seperator_character) {
        text_internal[text_length - 1] = '\0';
    }

    *text = text_internal;

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_normalizer_token_list_to_token_array)(
        pv_normalizer_token_list_t *token_list,
        pv_normalizer_token_t ***token_array) {
    PV_ASSERT(token_list);
    PV_ASSERT(token_array);

    *token_array = NULL;

    pv_normalizer_token_t **out_token_array = calloc(token_list->size, sizeof(pv_normalizer_token_t));
    if (!out_token_array) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("out_token_array"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_normalizer_token_t *current = token_list->head;
    for (int32_t i = 0; i < token_list->size; i++) {
        out_token_array[i] = current;
        current = current->next;
    }

    *token_array = out_token_array;

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_normalizer_token_list_collapse_tokens)(
        pv_normalizer_token_list_t *token_list,
        const char *collapsed_token_string,
        pv_normalizer_token_t *first_token,
        pv_normalizer_token_t *last_token,
        pv_normalizer_token_t **collapsed_token) {
    PV_ASSERT(token_list);
    PV_ASSERT(collapsed_token_string);
    PV_ASSERT(first_token);
    PV_ASSERT(last_token);
    PV_ASSERT(collapsed_token);

    *collapsed_token = NULL;

    int32_t original_string_length = 0;
    pv_normalizer_token_t *current = first_token;
    while ((current != last_token) && (current != NULL)) {
        original_string_length += (int32_t) strlen(current->original_string);
        current = current->next;
    }
    if (current != NULL) {
        original_string_length += (int32_t) strlen(current->original_string);
    }

    free(last_token->string);
    last_token->string = calloc(strlen(collapsed_token_string) + 1, sizeof(char));
    if (!first_token->string) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("last_token->string"));
        return PV_STATUS_OUT_OF_MEMORY;
    }
    strcpy(last_token->string, collapsed_token_string);

    char *original_string = calloc(original_string_length + 1, sizeof(char));
    if (!original_string) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("original_string"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    current = first_token;
    while ((current != last_token) && (current != NULL)) {
        pv_normalizer_token_t *next = current->next;
        strcat(original_string, current->original_string);
        pv_normalizer_token_list_remove_token(token_list, current);
        current = next;
    }
    if (current != NULL) {
        strcat(original_string, current->original_string);
    }

    free(last_token->original_string);
    last_token->original_string = original_string;

    *collapsed_token = last_token;

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_normalizer_token_list_merge_tokens)(
        pv_normalizer_token_list_t *token_list,
        const char *merged_token_string,
        const char *merged_original_string,
        pv_normalizer_token_t *first_token,
        pv_normalizer_token_t *last_token,
        pv_normalizer_token_t **merged_token) {
    PV_ASSERT(token_list);
    PV_ASSERT(merged_token_string);
    PV_ASSERT(merged_original_string);
    PV_ASSERT(first_token);
    PV_ASSERT(last_token);
    PV_ASSERT(merged_token);

    *merged_token = NULL;

    free(last_token->string);
    last_token->string = calloc(strlen(merged_token_string) + 1, sizeof(char));
    if (!first_token->string) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("last_token->string"));
        return PV_STATUS_OUT_OF_MEMORY;
    }
    strcpy(last_token->string, merged_token_string);

    free(last_token->original_string);
    last_token->original_string = calloc(strlen(merged_original_string) + 1, sizeof(char));
    if (!last_token->original_string) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("last_token->original_string)"));
        return PV_STATUS_OUT_OF_MEMORY;
    }
    strcpy(last_token->original_string, merged_original_string);

    int32_t num_removed = 0;
    pv_normalizer_token_t *current = first_token;
    while ((current != last_token) && (current != NULL)) {
        pv_normalizer_token_t *next = current->next;
        pv_normalizer_token_list_remove_token(token_list, current);
        current = next;
        num_removed++;
    }

    last_token->length_past_context -= num_removed;
    if (last_token->length_past_context < 0) {
        last_token->length_past_context = 0;
    }

    int32_t i = last_token->length_future_context;
    current = last_token->next;
    while ((i > 0) && (current != NULL)) {
        current->length_past_context -= num_removed;
        if (current->length_past_context < 0) {
            current->length_past_context = 0;
        }
        current = current->next;
        i--;
    }

    *merged_token = last_token;

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_normalizer_token_list_unroll_token)(
        int32_t string_split_index,
        pv_normalizer_token_t *token,
        pv_normalizer_token_list_t *token_list) {
    PV_ASSERT(string_split_index > 0);
    PV_ASSERT(token);
    PV_ASSERT(token_list);

    int32_t length = (int32_t) strlen(token->string);
    if (string_split_index >= length) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("string_split_index"));
        return PV_STATUS_INVALID_ARGUMENT;
    }

    pv_normalizer_token_t *new_next_token = NULL;
    pv_status_t status = pv_normalizer_token_copy(token, &new_next_token);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_token_copy,
                pv_status_to_string(status));
        return status;
    }

    memcpy(new_next_token->string, token->string + string_split_index, (length - string_split_index) * sizeof(char));

    token->string[string_split_index] = '\0';
    new_next_token->string[length - string_split_index] = '\0';

    pv_normalizer_token_t *next = token->next;

    token->next = new_next_token;
    if (strcmp(new_next_token->string, " ") == 0) {
        token->next_character_is_space = true;
    } else {
        token->next_character_is_space = false;
    }

    new_next_token->previous = token;
    new_next_token->next = next;

    if (next != NULL) {
        next->previous = new_next_token;
    } else {
        token_list->tail = new_next_token;
    }

    pv_normalizer_token_t *previous = token;
    while (previous && strcmp(previous->original_string, token->original_string) == 0) {
        previous->length_future_context += 1;
        previous = previous->previous;
    }
    pv_normalizer_token_t *after = new_next_token;
    while (after && strcmp(after->original_string, token->original_string) == 0) {
        after->length_past_context += 1;
        after = after->next;
    }

    token_list->size++;

    return PV_STATUS_SUCCESS;
}

int32_t PV_MOCKABLE(pv_normalizer_token_get_num_tokens_before)(const pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    int32_t num_previous_tokens = 0;
    const pv_normalizer_token_t *current = token->previous;
    while (current != NULL) {
        num_previous_tokens++;
        current = current->previous;
    }

    return num_previous_tokens;
}

pv_normalizer_token_t *PV_MOCKABLE(pv_normalizer_token_get_nth_token_before)(
        const pv_normalizer_token_t *token,
        int32_t n,
        bool skip_space) {
    PV_ASSERT(token);
    PV_ASSERT(n >= 0);

    pv_normalizer_token_t *current = (pv_normalizer_token_t *) token;
    for (int32_t i = 0; i < n; i++) {
        if (current == NULL) {
            return NULL;
        }

        current = current->previous;

        if (skip_space) {
            while ((current != NULL) && (current->tag_language_agnostic == PV_NORMALIZER_TAG_SPACE)) {
                current = current->previous;
            }
        }
    }

    return current;
}

pv_normalizer_token_t *PV_MOCKABLE(pv_normalizer_token_get_nth_token_after)(
        const pv_normalizer_token_t *token,
        int32_t n,
        bool skip_space) {
    PV_ASSERT(token);
    PV_ASSERT(n >= 0);

    pv_normalizer_token_t *current = (pv_normalizer_token_t *) token;
    for (int32_t i = 0; i < n; i++) {
        if (current == NULL) {
            return NULL;
        }

        current = current->next;

        if (skip_space) {
            while ((current != NULL) && (current->tag_language_agnostic == PV_NORMALIZER_TAG_SPACE)) {
                current = current->next;
            }
        }
    }

    return current;
}

bool PV_MOCKABLE(pv_normalizer_token_is_space_or_null)(const pv_normalizer_token_t *token) {
    return (((token != NULL) && (token->tag_language_agnostic == PV_NORMALIZER_TAG_SPACE)) || (token == NULL));
}

pv_status_t PV_MOCKABLE(pv_normalizer_token_concatenate_token_strings)(
        pv_normalizer_token_t *first,
        pv_normalizer_token_t *last,
        char **text) {
    PV_ASSERT(first);
    PV_ASSERT(text);

    *text = NULL;

    size_t text_length = 0;

    pv_normalizer_token_t *current = first;
    while ((current != NULL) && (current != last)) {
        text_length += strlen(current->string);
        current = current->next;
    }

    char *text_internal = calloc(text_length + 1, sizeof(char));
    if (!text_internal) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("text_internal"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    current = first;
    while ((current != NULL) && (current != last)) {
        strcat(text_internal, current->string);
        current = current->next;
    }

    *text = text_internal;

    return PV_STATUS_SUCCESS;
}

pv_normalizer_token_t *PV_MOCKABLE(pv_normalizer_token_get_token_after_previous_space)(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    pv_normalizer_token_t *previous = token;
    pv_normalizer_token_t *current = token->previous;
    while ((current != NULL) && (strcmp(current->string, " ") != 0) && (strcmp(current->string, "\n") != 0)) {
        previous = current;
        current = current->previous;
    }

    return previous;
}

void PV_MOCKABLE(pv_normalizer_token_list_synchronize_language_agnostic_tags_common)(
        pv_normalizer_token_list_t *token_list,
        int32_t tag_space,
        int32_t tag_punctuation,
        int32_t tag_single_quote,
        int32_t tag_custom_pronunciation,
        int32_t tag_letter_spell_out) {
    PV_ASSERT(token_list);

    pv_normalizer_token_t *token = token_list->head;
    while (token != NULL) {
        if (tag_space > 0) {
            if (token->tag == tag_space) {
                token->tag_language_agnostic = PV_NORMALIZER_TAG_SPACE;
            } else if (token->tag_language_agnostic == PV_NORMALIZER_TAG_SPACE) {
                token->tag_language_agnostic = PV_NORMALIZER_TAG_NONE;
            }
        }

        if (tag_punctuation > 0) {
            if (token->tag == tag_punctuation) {
                token->tag_language_agnostic = PV_NORMALIZER_TAG_PUNCTUATION;
            } else if (token->tag_language_agnostic == PV_NORMALIZER_TAG_PUNCTUATION) {
                token->tag_language_agnostic = PV_NORMALIZER_TAG_NONE;
            }

            if (tag_single_quote > 0) {
                if (token->tag == tag_single_quote) {
                    token->tag_language_agnostic = PV_NORMALIZER_TAG_PUNCTUATION;
                }
            }
        }

        if (tag_custom_pronunciation > 0) {
            if (token->tag == tag_custom_pronunciation) {
                token->tag_language_agnostic = PV_NORMALIZER_TAG_CUSTOM_PRONUNCIATION;
            } else if (token->tag_language_agnostic == PV_NORMALIZER_TAG_CUSTOM_PRONUNCIATION) {
                token->tag_language_agnostic = PV_NORMALIZER_TAG_NONE;
            }
        }

        if (tag_letter_spell_out > 0) {
            if (token->tag == tag_letter_spell_out) {
                token->tag_language_agnostic = PV_NORMALIZER_TAG_LETTER_SPELL_OUT;
            } else if (token->tag_language_agnostic == PV_NORMALIZER_TAG_LETTER_SPELL_OUT) {
                token->tag_language_agnostic = PV_NORMALIZER_TAG_NONE;
            }
        }

        token = token->next;
    }
}
