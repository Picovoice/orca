#include <stdlib.h>
#include <string.h>

#include "core/picovoice.h"
#include "core/pv_assert.h"
#include "core/pv_error.h"
#include "core/pv_error_messages.h"
#include "lm/pv_noun_gender_dict.h"
#include "orca/normalizer/pv_normalizer.h"
#include "orca/normalizer/pv_normalizer_tagger.h"
#include "orca/normalizer/pv_normalizer_token.h"
#include "orca/normalizer/pv_normalizer_tokenizer.h"
#include "orca/normalizer/pv_normalizer_util.h"
#include "orca/normalizer/pv_normalizer_verbalizer.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_normalizer_mock.h"

#endif

struct pv_normalizer {
    pv_language_info_t *language_info;
    const int32_t *use_cases;
    pv_normalizer_language_t language;

    pv_normalizer_tokenizer_t *tokenizer;
    pv_normalizer_tagger_t *tagger;
    pv_normalizer_verbalizer_t *verbalizer;
};

pv_status_t PV_MOCKABLE(pv_normalizer_init)(
        pv_language_info_t *language_info,
        pv_noun_gender_dict_t *noun_gender_dict,
        const void **tokenizer_data,
        pv_normalizer_t **object) {
    PV_ASSERT(language_info);
    PV_ASSERT(object);

    *object = NULL;

    pv_normalizer_t *o = calloc(1, sizeof(pv_normalizer_t));
    if (!o) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_status_t status = pv_normalizer_util_infer_language_from_language_info(language_info, &(o->language));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_util_infer_language_from_language_info,
                pv_status_to_string(status));
        free(o);
        return status;
    }

    o->language_info = language_info;

    status = pv_normalizer_tokenizer_init(o->language_info, tokenizer_data, &(o->tokenizer));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_tokenizer_init,
                pv_status_to_string(status));
        free(o);
        return status;
    }

    int32_t num_use_cases = 0;
    status = pv_normalizer_get_use_cases_from_language(o->language, &num_use_cases, &(o->use_cases));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_get_use_cases_from_language,
                pv_status_to_string(status));
        free(o);
        return status;
    }

    status = pv_normalizer_tagger_init(
            o->language_info,
            num_use_cases,
            o->use_cases,
            o->tokenizer,
            noun_gender_dict,
            &(o->tagger));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_tagger_init,
                pv_status_to_string(status));
        free(o);
        return status;
    }

    status = pv_normalizer_verbalizer_init(
            o->language,
            num_use_cases,
            o->use_cases,
            noun_gender_dict,
            &(o->verbalizer));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_verbalizer_init,
                pv_status_to_string(status));
        free(o);
        return status;
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_normalizer_delete)(pv_normalizer_t *object) {
    if (object) {
        pv_normalizer_verbalizer_delete(object->verbalizer);
        pv_normalizer_tagger_delete(object->tagger);
        pv_normalizer_tokenizer_delete(object->tokenizer);
        free(object);
    }
}

pv_status_t PV_MOCKABLE(pv_normalizer_get_characters)(
        const pv_normalizer_t *object,
        int32_t *num_characters,
        const char *const **characters) {
    PV_ASSERT(object);
    PV_ASSERT(num_characters);
    PV_ASSERT(characters);

    pv_status_t status = pv_normalizer_util_get_normalizable_characters(
            object->language,
            num_characters,
            characters);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_util_get_normalizable_characters,
                pv_status_to_string(status));
        return status;
    }
    return PV_STATUS_SUCCESS;
}

pv_normalizer_language_t PV_MOCKABLE(pv_normalizer_get_language)(const pv_normalizer_t *object) {
    PV_ASSERT(object);

    return object->language;
}

pv_language_info_t *PV_MOCKABLE(pv_normalizer_get_language_info)(const pv_normalizer_t *object) {
    PV_ASSERT(object);

    return object->language_info;
}

pv_normalizer_tokenizer_t *PV_MOCKABLE(pv_normalizer_get_tokenizer)(const pv_normalizer_t *object) {
    PV_ASSERT(object);

    return object->tokenizer;
}

pv_normalizer_tagger_t *PV_MOCKABLE(pv_normalizer_get_tagger)(const pv_normalizer_t *object) {
    PV_ASSERT(object);

    return object->tagger;
}

pv_normalizer_verbalizer_t *PV_MOCKABLE(pv_normalizer_get_verbalizer)(const pv_normalizer_t *object) {
    PV_ASSERT(object);

    return object->verbalizer;
}

void PV_MOCKABLE(pv_normalizer_remove_hyphen_only_tokens)(pv_normalizer_token_list_t *token_list) {
    PV_ASSERT(token_list);

    pv_normalizer_token_t *current = token_list->head;
    while (current) {
        bool only_dashes = false;
        for (int32_t i = 0; i < (int32_t) strlen(current->string); i++) {
            if (current->string[i] != '-') {
                only_dashes = false;
                break;
            } else {
                only_dashes = true;
            }
        }
        pv_normalizer_token_t *next = current->next;
        if (only_dashes) {
            pv_normalizer_token_list_remove_token(token_list, current);
        }
        current = next;
    }
}

void PV_MOCKABLE(pv_normalizer_remove_invalid_custom_pron_markers)(pv_normalizer_token_list_t *token_list) {
    PV_ASSERT(token_list);

    pv_normalizer_token_t *current = token_list->head;
    while (current) {
        if (current->tag_language_agnostic != PV_NORMALIZER_TAG_CUSTOM_PRONUNCIATION) {
            char *string = current->string;
            int32_t valid_index = 0;
            for (size_t i = 0; i < strlen(string); i++) {
                bool is_custom_pron_marker =
                        (string[i] == PV_NORMALIZER_CUSTOM_PRON_OPENING_MARKER) ||
                        (string[i] == PV_NORMALIZER_CUSTOM_PRON_CLOSING_MARKER) ||
                        (string[i] == PV_NORMALIZER_CUSTOM_PRON_SEPARATOR);
                if (!is_custom_pron_marker) {
                    current->string[valid_index] = current->string[i];
                    valid_index++;
                }
            }
            current->string[valid_index] = '\0';
        }
        current = current->next;
    }
}

pv_status_t PV_MOCKABLE(pv_normalizer_normalize)(
        pv_normalizer_t *object,
        const char *text,
        bool preserve_word_boundary,
        bool remove_unknown_characters,
        char **normalized,
        pv_normalizer_token_list_t **token_list) {
    PV_ASSERT(object);
    PV_ASSERT(text);
    PV_ASSERT(normalized || token_list);

    if (normalized) {
        *normalized = NULL;
    }

    if (token_list) {
        *token_list = NULL;
    }

    char *remapped_text = NULL;
    pv_status_t status = pv_normalizer_util_remap_space(text, &remapped_text);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_util_remap_space,
                pv_status_to_string(status));
        return status;
    }

    if (remapped_text) {
        text = remapped_text;
    }

    if (strlen(text) == 0) {
        free(remapped_text);
        return PV_STATUS_SUCCESS;
    }

    pv_normalizer_token_list_t *token_list_internal = NULL;

    status = pv_normalizer_tokenizer_tokenize(
            object->tokenizer,
            text,
            pv_normalizer_tokenizer_default_word_boundary_character(object->tokenizer),
            preserve_word_boundary,
            remove_unknown_characters,
            false,
            true,
            &token_list_internal);
    free(remapped_text);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_tokenizer_tokenize,
                pv_status_to_string(status));
        return status;
    }
    if (!token_list_internal) {
        return PV_STATUS_SUCCESS;
    }

    pv_normalizer_remove_hyphen_only_tokens(token_list_internal);

    pv_normalizer_remove_invalid_custom_pron_markers(token_list_internal);

    status = pv_normalizer_tagger_tag(object->tagger, token_list_internal, 0, true);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_tagger_tag,
                pv_status_to_string(status));
        pv_normalizer_token_list_delete(token_list_internal);
        return status;
    }

    status = pv_normalizer_verbalizer_verbalize(object->verbalizer, token_list_internal, 0);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_verbalizer_verbalize,
                pv_status_to_string(status));
        pv_normalizer_token_list_delete(token_list_internal);
        return status;
    }

    pv_normalizer_token_list_remove_nonverbalized_tokens(token_list_internal);

    if (token_list) {
        pv_normalizer_token_list_t *split_token_list = NULL;
        status = pv_normalizer_tokenizer_token_list_split_verbalized(
                object->tokenizer,
                token_list_internal,
                &split_token_list);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_tokenizer_token_list_split_verbalized,
                    pv_status_to_string(status));
            pv_normalizer_token_list_delete(token_list_internal);
            return status;
        }
        if (split_token_list) {
            pv_normalizer_token_list_delete(token_list_internal);
            token_list_internal = split_token_list;
        }

        *token_list = token_list_internal;

    } else {
        char *out_normalized = NULL;
        status = pv_normalizer_token_list_to_verbalized_text(
                token_list_internal,
                pv_normalizer_tokenizer_default_word_boundary_character(object->tokenizer),
                &out_normalized);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_token_list_to_verbalized_text,
                    pv_status_to_string(status));
            pv_normalizer_token_list_delete(token_list_internal);
            return status;
        }

        *normalized = out_normalized;

        pv_normalizer_token_list_delete(token_list_internal);
    }

    return PV_STATUS_SUCCESS;
}
