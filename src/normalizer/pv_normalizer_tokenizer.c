#include <stdlib.h>
#include <string.h>

#include "core/pv_assert.h"
#include "core/pv_error_messages.h"
#include "io/pv_log.h"
#include "orca/normalizer/pv_normalizer_token.h"
#include "orca/normalizer/pv_normalizer_tokenizer.h"
#include "orca/normalizer/pv_normalizer_tokenizer_generic.h"
#include "orca/normalizer/pv_normalizer_util.h"
#include "orca/normalizer/ja/pv_normalizer_tokenizer_ja.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_normalizer_mock.h"

#endif

struct pv_normalizer_tokenizer {
    pv_normalizer_language_t language;
    void *tokenizer;
};

pv_status_t PV_MOCKABLE(pv_normalizer_tokenizer_init)(
        pv_language_info_t *language_info,
        const void **tokenizer_data,
        pv_normalizer_tokenizer_t **object) {
    PV_ASSERT(language_info);
    PV_ASSERT(object);

    *object = NULL;

    pv_normalizer_tokenizer_t *o = calloc(1, sizeof(pv_normalizer_tokenizer_t));
    if (!o) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_normalizer_language_t language = 0;
    pv_status_t status = pv_normalizer_util_infer_language_from_language_info(language_info, &language);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_util_infer_language_from_language_info,
                pv_status_to_string(status));
        pv_normalizer_tokenizer_delete(o);
        return status;
    }

    switch (language) {
        case PV_NORMALIZER_LANGUAGE_EN:
        case PV_NORMALIZER_LANGUAGE_DE:
        case PV_NORMALIZER_LANGUAGE_FR:
        case PV_NORMALIZER_LANGUAGE_ES:
        case PV_NORMALIZER_LANGUAGE_IT:
        case PV_NORMALIZER_LANGUAGE_PT:
        case PV_NORMALIZER_LANGUAGE_KO: {
            status = pv_normalizer_tokenizer_generic_init(
                    language,
                    language_info,
                    (pv_normalizer_tokenizer_generic_t **) (&(o->tokenizer)));
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tokenizer_generic_init,
                        pv_status_to_string(status));
                pv_normalizer_tokenizer_delete(o);
                return status;
            }
        } break;
        case PV_NORMALIZER_LANGUAGE_JA: {
            status = pv_normalizer_tokenizer_ja_init(
                    language,
                    language_info,
                    tokenizer_data,
                    (pv_normalizer_tokenizer_ja_t **) (&(o->tokenizer)));
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tokenizer_ja_init,
                        pv_status_to_string(status));
                pv_normalizer_tokenizer_delete(o);
                return status;
            }
        } break;
        default: {
            PV_ERROR_REPORT(
                    &pv_error_msg_invalid_argument,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("language_info"));
            pv_normalizer_tokenizer_delete(o);
            return PV_STATUS_INVALID_ARGUMENT;
        }
    }

    o->language = language;

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_normalizer_tokenizer_delete)(pv_normalizer_tokenizer_t *object) {
    if (object) {
        switch (object->language) {
            case PV_NORMALIZER_LANGUAGE_EN:
            case PV_NORMALIZER_LANGUAGE_DE:
            case PV_NORMALIZER_LANGUAGE_FR:
            case PV_NORMALIZER_LANGUAGE_ES:
            case PV_NORMALIZER_LANGUAGE_IT:
            case PV_NORMALIZER_LANGUAGE_PT:
            case PV_NORMALIZER_LANGUAGE_KO:
                pv_normalizer_tokenizer_generic_delete(object->tokenizer);
                break;
            case PV_NORMALIZER_LANGUAGE_JA:
                pv_normalizer_tokenizer_ja_delete(object->tokenizer);
                break;
            default:
                break;
        }
        free(object);
    }
}

pv_status_t PV_MOCKABLE(pv_normalizer_tokenizer_token_list_split_verbalized)(
        const pv_normalizer_tokenizer_t *object,
        pv_normalizer_token_list_t *token_list,
        pv_normalizer_token_list_t **split_token_list) {
    PV_ASSERT(object);
    PV_ASSERT(token_list);
    PV_ASSERT(split_token_list);

    switch (object->language) {
        case PV_NORMALIZER_LANGUAGE_EN:
        case PV_NORMALIZER_LANGUAGE_DE:
        case PV_NORMALIZER_LANGUAGE_FR:
        case PV_NORMALIZER_LANGUAGE_ES:
        case PV_NORMALIZER_LANGUAGE_IT:
        case PV_NORMALIZER_LANGUAGE_PT:
        case PV_NORMALIZER_LANGUAGE_KO: {
            pv_status_t status = pv_normalizer_tokenizer_generic_token_list_split_verbalized(
                    object->tokenizer,
                    token_list,
                    split_token_list);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tokenizer_generic_token_list_split_verbalized,
                        pv_status_to_string(status));
                return status;
            }
        } break;
        case PV_NORMALIZER_LANGUAGE_JA: {
            pv_status_t status = pv_normalizer_tokenizer_ja_token_list_split_verbalized(
                    object->tokenizer,
                    token_list,
                    split_token_list);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tokenizer_ja_token_list_split_verbalized,
                        pv_status_to_string(status));
                return status;
            }
        } break;
        default:
            return PV_STATUS_INVALID_STATE;
    }
    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_normalizer_tokenizer_tokenize)(
        const pv_normalizer_tokenizer_t *object,
        const char *text,
        const char word_boundary_character,
        bool preserve_word_boundary,
        bool remove_unknown_characters,
        bool split_on_special_characters,
        bool split_on_apostrophe,
        pv_normalizer_token_list_t **token_list) {
    PV_ASSERT(object);
    PV_ASSERT(text);
    PV_ASSERT(token_list);

    switch (object->language) {
        case PV_NORMALIZER_LANGUAGE_EN:
        case PV_NORMALIZER_LANGUAGE_DE:
        case PV_NORMALIZER_LANGUAGE_FR:
        case PV_NORMALIZER_LANGUAGE_ES:
        case PV_NORMALIZER_LANGUAGE_IT:
        case PV_NORMALIZER_LANGUAGE_PT:
        case PV_NORMALIZER_LANGUAGE_KO: {
            pv_status_t status = pv_normalizer_tokenizer_generic_tokenize(
                    object->tokenizer,
                    text,
                    word_boundary_character,
                    preserve_word_boundary,
                    remove_unknown_characters,
                    split_on_special_characters,
                    split_on_apostrophe,
                    token_list);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tokenizer_generic_tokenize,
                        pv_status_to_string(status));
                return status;
            }
        } break;
        case PV_NORMALIZER_LANGUAGE_JA: {
            pv_status_t status;
            if (word_boundary_character == pv_normalizer_tokenizer_default_word_boundary_character(object)) {
                status = pv_normalizer_tokenizer_ja_tokenize(
                        object->tokenizer,
                        text,
                        preserve_word_boundary,
                        remove_unknown_characters,
                        true,
                        token_list);

                // trim last space when batch tokenizing
                if (((*token_list) != NULL) && ((*token_list)->tail != NULL) &&
                    (strcmp((*token_list)->tail->string, " ") == 0)) {
                    pv_normalizer_token_list_remove_token((*token_list), (*token_list)->tail);
                    if ((*token_list)->tail != NULL) {
                        (*token_list)->tail->next_character_is_space = false;
                    }
                }
            } else {
                status = pv_normalizer_tokenizer_ja_tokenize_on_character(
                        object->tokenizer,
                        text,
                        word_boundary_character,
                        preserve_word_boundary,
                        remove_unknown_characters,
                        split_on_special_characters,
                        token_list);
            }
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tokenizer_ja_tokenize,
                        pv_status_to_string(status));
                return status;
            }
        } break;
        default:
            return PV_STATUS_INVALID_STATE;
    }
    return PV_STATUS_SUCCESS;
}

char PV_MOCKABLE(pv_normalizer_tokenizer_default_word_boundary_character)(const pv_normalizer_tokenizer_t *object) {
    switch (object->language) {
        case PV_NORMALIZER_LANGUAGE_EN:
        case PV_NORMALIZER_LANGUAGE_DE:
        case PV_NORMALIZER_LANGUAGE_FR:
        case PV_NORMALIZER_LANGUAGE_ES:
        case PV_NORMALIZER_LANGUAGE_IT:
        case PV_NORMALIZER_LANGUAGE_PT:
        case PV_NORMALIZER_LANGUAGE_KO:
            return pv_normalizer_tokenizer_generic_default_word_boundary_character();
        case PV_NORMALIZER_LANGUAGE_JA:
        default:
            return 0;
    }
}

struct pv_normalizer_tokenizer_stream {
    pv_normalizer_language_t language;
    void *tokenizer_stream;
};

pv_status_t PV_MOCKABLE(pv_normalizer_tokenizer_stream_open)(
        pv_normalizer_tokenizer_t *object,
        pv_normalizer_tokenizer_stream_t **stream) {
    PV_ASSERT(object);
    PV_ASSERT(stream);

    *stream = NULL;

    pv_normalizer_tokenizer_stream_t *s = calloc(1, sizeof(pv_normalizer_tokenizer_stream_t));
    if (!s) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("s"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    switch (object->language) {
        case PV_NORMALIZER_LANGUAGE_EN:
        case PV_NORMALIZER_LANGUAGE_DE:
        case PV_NORMALIZER_LANGUAGE_FR:
        case PV_NORMALIZER_LANGUAGE_ES:
        case PV_NORMALIZER_LANGUAGE_IT:
        case PV_NORMALIZER_LANGUAGE_PT:
        case PV_NORMALIZER_LANGUAGE_KO: {
            pv_status_t status = pv_normalizer_tokenizer_generic_stream_open(
                    (pv_normalizer_tokenizer_generic_t *) object->tokenizer,
                    (pv_normalizer_tokenizer_generic_stream_t **) (&(s->tokenizer_stream)));
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tokenizer_generic_stream_open,
                        pv_status_to_string(status));
                pv_normalizer_tokenizer_stream_close(s);
                return status;
            }
        } break;
        case PV_NORMALIZER_LANGUAGE_JA: {
            pv_status_t status = pv_normalizer_tokenizer_ja_stream_open(
                    (pv_normalizer_tokenizer_ja_t *) object->tokenizer,
                    (pv_normalizer_tokenizer_ja_stream_t **) (&(s->tokenizer_stream)));
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tokenizer_ja_stream_open,
                        pv_status_to_string(status));
                pv_normalizer_tokenizer_stream_close(s);
                return status;
            }
        } break;
        default: {
            PV_ERROR_REPORT(
                    &pv_error_msg_invalid_argument,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("language"));
            pv_normalizer_tokenizer_stream_close(s);
            return PV_STATUS_INVALID_ARGUMENT;
        }
    }

    s->language = object->language;

    *stream = s;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_normalizer_tokenizer_stream_close)(pv_normalizer_tokenizer_stream_t *object) {
    if (object) {
        switch (object->language) {
            case PV_NORMALIZER_LANGUAGE_EN:
            case PV_NORMALIZER_LANGUAGE_DE:
            case PV_NORMALIZER_LANGUAGE_FR:
            case PV_NORMALIZER_LANGUAGE_ES:
            case PV_NORMALIZER_LANGUAGE_IT:
            case PV_NORMALIZER_LANGUAGE_PT:
            case PV_NORMALIZER_LANGUAGE_KO:
                pv_normalizer_tokenizer_generic_stream_close(object->tokenizer_stream);
                break;
            case PV_NORMALIZER_LANGUAGE_JA:
                pv_normalizer_tokenizer_ja_stream_close(object->tokenizer_stream);
                break;
            default:
                break;
        }
        free(object);
    }
}

pv_status_t PV_MOCKABLE(pv_normalizer_tokenizer_stream_tokenize_initial)(
        pv_normalizer_tokenizer_stream_t *object,
        const char *text,
        char word_boundary_character,
        bool preserve_word_boundary,
        bool remove_unknown_characters,
        bool split_on_special_characters,
        pv_normalizer_token_list_t **token_list) {
    switch (object->language) {
        case PV_NORMALIZER_LANGUAGE_EN:
        case PV_NORMALIZER_LANGUAGE_DE:
        case PV_NORMALIZER_LANGUAGE_FR:
        case PV_NORMALIZER_LANGUAGE_ES:
        case PV_NORMALIZER_LANGUAGE_IT:
        case PV_NORMALIZER_LANGUAGE_PT:
        case PV_NORMALIZER_LANGUAGE_KO: {
            pv_status_t status = pv_normalizer_tokenizer_generic_stream_tokenize_initial(
                    object->tokenizer_stream,
                    text,
                    word_boundary_character,
                    preserve_word_boundary,
                    remove_unknown_characters,
                    split_on_special_characters,
                    token_list);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tokenizer_generic_stream_tokenize_initial,
                        pv_status_to_string(status));
                return status;
            }
        } break;
        case PV_NORMALIZER_LANGUAGE_JA: {
            pv_status_t status = pv_normalizer_tokenizer_ja_stream_tokenize(
                    object->tokenizer_stream,
                    text,
                    preserve_word_boundary,
                    remove_unknown_characters,
                    token_list);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tokenizer_ja_stream_tokenize,
                        pv_status_to_string(status));
                return status;
            }
        } break;
        default:
            return PV_STATUS_INVALID_STATE;
    }
    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_normalizer_tokenizer_stream_tokenize_verbalizable)(
        pv_normalizer_tokenizer_stream_t *object,
        const pv_normalizer_token_list_t *verbalizable_tokens,
        char word_boundary_character,
        bool preserve_word_boundary,
        bool remove_unknown_characters,
        bool split_on_special_characters,
        pv_normalizer_token_list_t **token_list) {
    switch (object->language) {
        case PV_NORMALIZER_LANGUAGE_EN:
        case PV_NORMALIZER_LANGUAGE_DE:
        case PV_NORMALIZER_LANGUAGE_FR:
        case PV_NORMALIZER_LANGUAGE_ES:
        case PV_NORMALIZER_LANGUAGE_IT:
        case PV_NORMALIZER_LANGUAGE_PT:
        case PV_NORMALIZER_LANGUAGE_KO: {
            pv_status_t status = pv_normalizer_tokenizer_generic_stream_tokenize_verbalizable(
                    object->tokenizer_stream,
                    verbalizable_tokens,
                    word_boundary_character,
                    preserve_word_boundary,
                    remove_unknown_characters,
                    split_on_special_characters,
                    token_list);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tokenizer_generic_stream_tokenize_verbalizable,
                        pv_status_to_string(status));
                return status;
            }
        } break;
        case PV_NORMALIZER_LANGUAGE_JA: {
            pv_status_t status = pv_normalizer_token_list_copy(
                    verbalizable_tokens,
                    token_list);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_token_list_copy,
                        pv_status_to_string(status));
                return status;
            }
        } break;
        default:
            return PV_STATUS_INVALID_STATE;
    }
    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_normalizer_tokenizer_stream_flush)(
        pv_normalizer_tokenizer_stream_t *object,
        bool preserve_word_boundary,
        bool remove_unknown_characters,
        pv_normalizer_token_list_t **token_list) {
    switch (object->language) {
        case PV_NORMALIZER_LANGUAGE_EN:
        case PV_NORMALIZER_LANGUAGE_DE:
        case PV_NORMALIZER_LANGUAGE_FR:
        case PV_NORMALIZER_LANGUAGE_ES:
        case PV_NORMALIZER_LANGUAGE_IT:
        case PV_NORMALIZER_LANGUAGE_PT:
        case PV_NORMALIZER_LANGUAGE_KO: {
            *token_list = NULL;
        } break;
        case PV_NORMALIZER_LANGUAGE_JA: {
            pv_status_t status = pv_normalizer_tokenizer_ja_stream_flush(
                    object->tokenizer_stream,
                    preserve_word_boundary,
                    remove_unknown_characters,
                    token_list);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tokenizer_ja_stream_flush,
                        pv_status_to_string(status));
                return status;
            }

            // trim last space when finished streaming
            if (((*token_list) != NULL) && ((*token_list)->tail != NULL) &&
                (strcmp((*token_list)->tail->string, " ") == 0)) {
                pv_normalizer_token_list_remove_token((*token_list), (*token_list)->tail);
                if ((*token_list)->tail != NULL) {
                    (*token_list)->tail->next_character_is_space = false;
                }
            }
        } break;
        default:
            return PV_STATUS_INVALID_STATE;
    }
    return PV_STATUS_SUCCESS;
}