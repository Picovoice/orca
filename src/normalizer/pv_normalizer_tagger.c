#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "core/pv_assert.h"
#include "core/pv_error_messages.h"
#include "lm/pv_noun_gender_dict.h"
#include "core/pv_language_internal.h"
#include "orca/normalizer/pv_normalizer_tagger.h"
#include "orca/normalizer/pv_normalizer_token.h"
#include "orca/normalizer/pv_normalizer_use_cases.h"
#include "orca/normalizer/pv_normalizer_util.h"

#include "orca/normalizer/de/pv_normalizer_tagger_de.h"
#include "orca/normalizer/en/pv_normalizer_tagger_en.h"
#include "orca/normalizer/es/pv_normalizer_tagger_es.h"
#include "orca/normalizer/fr/pv_normalizer_tagger_fr.h"
#include "orca/normalizer/it/pv_normalizer_tagger_it.h"
#include "orca/normalizer/pt/pv_normalizer_tagger_pt.h"
#include "orca/normalizer/ko/pv_normalizer_tagger_ko.h"
#include "orca/normalizer/ja/pv_normalizer_tagger_ja.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_normalizer_mock.h"

#endif

static void pv_normalizer_tagger_tag_language_agnostic(pv_normalizer_token_list_t *token_list) {
    PV_ASSERT(token_list);

    pv_normalizer_token_t *token = token_list->head;
    while (token != NULL) {
        if (strcmp(token->string, " ") == 0) {
            token->tag_language_agnostic = PV_NORMALIZER_TAG_SPACE;
        }
        token = token->next;
    }
}

struct pv_normalizer_tagger {
    pv_normalizer_language_t language;
    void *tagger;
};

pv_status_t PV_MOCKABLE(pv_normalizer_tagger_init)(
        const pv_language_info_t *language_info,
        int32_t num_use_cases,
        const int32_t *use_cases,
        pv_normalizer_tokenizer_t *tokenizer,
        pv_noun_gender_dict_t *noun_gender_dict,
        pv_normalizer_tagger_t **object) {
    PV_ASSERT(language_info);
    PV_ASSERT(num_use_cases >= 0);
    PV_ASSERT(use_cases);
    PV_ASSERT(tokenizer);
    PV_ASSERT(object);

    *object = NULL;

    pv_normalizer_tagger_t *o = calloc(1, sizeof(pv_normalizer_tagger_t));
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
        pv_normalizer_tagger_delete(o);
        return status;
    }

    switch (language) {
        case PV_NORMALIZER_LANGUAGE_EN: {
            status = pv_normalizer_tagger_en_init(
                    language_info,
                    num_use_cases,
                    (const pv_normalizer_use_cases_en_t *) use_cases,
                    tokenizer,
                    (pv_normalizer_tagger_en_t **) (&(o->tagger)));
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_en_init,
                        pv_status_to_string(status));
                pv_normalizer_tagger_delete(o);
                return status;
            }
        } break;
        case PV_NORMALIZER_LANGUAGE_DE: {
            status = pv_normalizer_tagger_de_init(
                    language_info,
                    num_use_cases,
                    (const pv_normalizer_use_cases_de_t *) use_cases,
                    tokenizer,
                    noun_gender_dict,
                    (pv_normalizer_tagger_de_t **) (&(o->tagger)));
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_de_init,
                        pv_status_to_string(status));
                pv_normalizer_tagger_delete(o);
                return status;
            }
        } break;
        case PV_NORMALIZER_LANGUAGE_FR: {
            status = pv_normalizer_tagger_fr_init(
                    language_info,
                    num_use_cases,
                    (const pv_normalizer_use_cases_fr_t *) use_cases,
                    tokenizer,
                    noun_gender_dict,
                    (pv_normalizer_tagger_fr_t **) (&(o->tagger)));
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_fr_init,
                        pv_status_to_string(status));
                pv_normalizer_tagger_delete(o);
                return status;
            }
        } break;
        case PV_NORMALIZER_LANGUAGE_ES: {
            status = pv_normalizer_tagger_es_init(
                    language_info,
                    num_use_cases,
                    (const pv_normalizer_use_cases_es_t *) use_cases,
                    tokenizer,
                    noun_gender_dict,
                    (pv_normalizer_tagger_es_t **) (&(o->tagger)));
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_es_init,
                        pv_status_to_string(status));
                pv_normalizer_tagger_delete(o);
                return status;
            }
        } break;
        case PV_NORMALIZER_LANGUAGE_IT: {
            status = pv_normalizer_tagger_it_init(
                    language_info,
                    num_use_cases,
                    (const pv_normalizer_use_cases_it_t *) use_cases,
                    tokenizer,
                    noun_gender_dict,
                    (pv_normalizer_tagger_it_t **) (&(o->tagger)));
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_it_init,
                        pv_status_to_string(status));
                pv_normalizer_tagger_delete(o);
                return status;
            }
        } break;
        case PV_NORMALIZER_LANGUAGE_PT: {
            status = pv_normalizer_tagger_pt_init(
                    language_info,
                    num_use_cases,
                    (const pv_normalizer_use_cases_pt_t *) use_cases,
                    tokenizer,
                    noun_gender_dict,
                    (pv_normalizer_tagger_pt_t **) (&(o->tagger)));
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_pt_init,
                        pv_status_to_string(status));
                pv_normalizer_tagger_delete(o);
                return status;
            }
        } break;
        case PV_NORMALIZER_LANGUAGE_KO: {
            status = pv_normalizer_tagger_ko_init(
                    language_info,
                    num_use_cases,
                    (const pv_normalizer_use_cases_ko_t *) use_cases,
                    tokenizer,
                    (pv_normalizer_tagger_ko_t **) (&(o->tagger)));
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_ko_init,
                        pv_status_to_string(status));
                pv_normalizer_tagger_delete(o);
                return status;
            }
        } break;
        case PV_NORMALIZER_LANGUAGE_JA: {
            status = pv_normalizer_tagger_ja_init(
                    language_info,
                    num_use_cases,
                    (const pv_normalizer_use_cases_ja_t *) use_cases,
                    tokenizer,
                    (pv_normalizer_tagger_ja_t **) (&(o->tagger)));
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_ja_init,
                        pv_status_to_string(status));
                pv_normalizer_tagger_delete(o);
                return status;
            }
        } break;
        default: {
            PV_ERROR_REPORT(
                    &pv_error_msg_invalid_argument,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("language_info"));
            pv_normalizer_tagger_delete(o);
            return PV_STATUS_INVALID_ARGUMENT;
        }
    }

    o->language = language;

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_normalizer_tagger_delete)(pv_normalizer_tagger_t *object) {
    if (object) {
        switch (object->language) {
            case PV_NORMALIZER_LANGUAGE_EN:
                pv_normalizer_tagger_en_delete(object->tagger);
                break;
            case PV_NORMALIZER_LANGUAGE_DE:
                pv_normalizer_tagger_de_delete(object->tagger);
                break;
            case PV_NORMALIZER_LANGUAGE_FR:
                pv_normalizer_tagger_fr_delete(object->tagger);
                break;
            case PV_NORMALIZER_LANGUAGE_ES:
                pv_normalizer_tagger_es_delete(object->tagger);
                break;
            case PV_NORMALIZER_LANGUAGE_IT:
                pv_normalizer_tagger_it_delete(object->tagger);
                break;
            case PV_NORMALIZER_LANGUAGE_PT:
                pv_normalizer_tagger_pt_delete(object->tagger);
                break;
            case PV_NORMALIZER_LANGUAGE_KO:
                pv_normalizer_tagger_ko_delete(object->tagger);
                break;
            case PV_NORMALIZER_LANGUAGE_JA:
                pv_normalizer_tagger_ja_delete(object->tagger);
                break;
            default:
                break;
        }
        free(object);
    }
}

pv_status_t PV_MOCKABLE(pv_normalizer_tagger_tag)(
        pv_normalizer_tagger_t *object,
        pv_normalizer_token_list_t *token_list,
        int32_t num_tokens_skip,
        bool split_untagged) {
    PV_ASSERT(object);
    PV_ASSERT(token_list);
    PV_ASSERT(num_tokens_skip >= 0);

    pv_normalizer_tagger_tag_language_agnostic(token_list);

    switch (object->language) {
        case PV_NORMALIZER_LANGUAGE_EN: {
            pv_status_t status = pv_normalizer_tagger_en_tag(
                    object->tagger,
                    token_list,
                    num_tokens_skip,
                    split_untagged);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_en_tag,
                        pv_status_to_string(status));
                return status;
            }
        } break;
        case PV_NORMALIZER_LANGUAGE_DE: {
            pv_status_t status = pv_normalizer_tagger_de_tag(
                    object->tagger,
                    token_list,
                    num_tokens_skip,
                    split_untagged);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_de_tag,
                        pv_status_to_string(status));
                return status;
            }
        } break;
        case PV_NORMALIZER_LANGUAGE_FR: {
            pv_status_t status = pv_normalizer_tagger_fr_tag(
                    object->tagger,
                    token_list,
                    num_tokens_skip,
                    split_untagged);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_fr_tag,
                        pv_status_to_string(status));
                return status;
            }
        } break;
        case PV_NORMALIZER_LANGUAGE_ES: {
            pv_status_t status = pv_normalizer_tagger_es_tag(
                    object->tagger,
                    token_list,
                    num_tokens_skip,
                    split_untagged);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_es_tag,
                        pv_status_to_string(status));
                return status;
            }
        } break;
        case PV_NORMALIZER_LANGUAGE_IT: {
            pv_status_t status = pv_normalizer_tagger_it_tag(
                    object->tagger,
                    token_list,
                    num_tokens_skip,
                    split_untagged);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_it_tag,
                        pv_status_to_string(status));
                return status;
            }
        } break;
        case PV_NORMALIZER_LANGUAGE_PT: {
            pv_status_t status = pv_normalizer_tagger_pt_tag(
                    object->tagger,
                    token_list,
                    num_tokens_skip,
                    split_untagged);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_pt_tag,
                        pv_status_to_string(status));
                return status;
            }
        } break;
        case PV_NORMALIZER_LANGUAGE_KO: {
            pv_status_t status = pv_normalizer_tagger_ko_tag(
                    object->tagger,
                    token_list,
                    num_tokens_skip,
                    split_untagged);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_pt_tag,
                        pv_status_to_string(status));
                return status;
            }
        } break;
        case PV_NORMALIZER_LANGUAGE_JA: {
            pv_status_t status = pv_normalizer_tagger_ja_tag(
                    object->tagger,
                    token_list,
                    num_tokens_skip,
                    split_untagged);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_ja_tag,
                        pv_status_to_string(status));
                return status;
            }
        } break;
        default:
            return PV_STATUS_INVALID_STATE;
    }

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_normalizer_tagger_tag_from_language_agnostic_common)(
        pv_normalizer_token_t *token,
        int32_t tag_none,
        int32_t tag_space,
        int32_t tag_punctuation,
        int32_t tag_custom_pronunciation) {
    PV_ASSERT(token);
    PV_ASSERT(tag_none >= 0);

    if (token->tag == tag_none) {
        if ((tag_space > 0) && (token->tag_language_agnostic == PV_NORMALIZER_TAG_SPACE)) {
            token->tag = tag_space;
        }

        if ((tag_punctuation > 0) && (token->tag_language_agnostic == PV_NORMALIZER_TAG_PUNCTUATION)) {
            token->tag = tag_punctuation;
        }

        if ((tag_custom_pronunciation > 0) && (token->tag_language_agnostic == PV_NORMALIZER_TAG_CUSTOM_PRONUNCIATION)) {
            token->tag = tag_custom_pronunciation;
        }
    }
}

pv_status_t PV_MOCKABLE(pv_normalizer_tagger_tag_word_common)(
        const pv_language_info_t *language_info,
        pv_normalizer_token_t *token,
        int32_t tag) {
    PV_ASSERT(language_info);
    PV_ASSERT(token);
    PV_ASSERT(tag >= 0);

    const char *string = token->string;
    size_t length = strlen(string);

    char character[PV_NORMALIZER_MAX_NUM_BYTES_PER_CHARACTER] = {0};

    bool found_invalid = false;
    size_t i = 0;
    while (i < length) {
        int32_t num_bytes_character = 0;
        pv_status_t status = pv_language_num_bytes_character((unsigned char) string[i], &num_bytes_character);
        if (status != PV_STATUS_SUCCESS) {
            found_invalid = true;
            break;
        }

        for (int32_t j = 0; j < num_bytes_character; j++) {
            character[j] = string[i + j];
        }
        character[num_bytes_character] = '\0';

        pv_normalizer_language_t language = 0;
        status = pv_normalizer_util_infer_language_from_language_info(language_info, &language);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_util_infer_language_from_language_info,
                    pv_status_to_string(status));
            return status;
        }

        bool is_word_character = false;
        status = pv_normalizer_util_is_word_character(language, character, &is_word_character);
        found_invalid = !is_word_character;
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_util_is_word_character,
                    pv_status_to_string(status));
            return status;
        }
        if (found_invalid) {
            break;
        }

        i += num_bytes_character;
    }

    if (!found_invalid) {
        token->tag = tag;
    }

    return PV_STATUS_SUCCESS;
}


pv_status_t PV_MOCKABLE(pv_normalizer_tagger_tag_punctuation_common)(
        pv_normalizer_language_t language,
        pv_normalizer_token_t *token,
        int32_t tag) {
    PV_ASSERT(language);
    PV_ASSERT(token);
    PV_ASSERT(tag >= 0);

    bool is_punctuation = false;
    pv_status_t status = pv_normalizer_util_is_punctuation(language, token->string, &is_punctuation);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_util_is_punctuation,
                pv_status_to_string(status));
        return status;
    }

    if (is_punctuation) {
        token->tag = tag;
    }

    return PV_STATUS_SUCCESS;
}