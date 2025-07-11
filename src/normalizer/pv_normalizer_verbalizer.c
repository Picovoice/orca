#include <stdlib.h>
#include <string.h>

#include "core/pv_assert.h"
#include "core/pv_error_messages.h"
#include "lm/pv_noun_gender_dict.h"
#include "orca/normalizer/pv_normalizer_token.h"
#include "orca/normalizer/pv_normalizer_use_cases.h"
#include "orca/normalizer/pv_normalizer_util.h"
#include "orca/normalizer/pv_normalizer_verbalizer.h"

#include "orca/normalizer/de/pv_normalizer_verbalizer_de.h"
#include "orca/normalizer/en/pv_normalizer_verbalizer_en.h"
#include "orca/normalizer/es/pv_normalizer_verbalizer_es.h"
#include "orca/normalizer/fr/pv_normalizer_verbalizer_fr.h"
#include "orca/normalizer/it/pv_normalizer_verbalizer_it.h"
#include "orca/normalizer/pt/pv_normalizer_verbalizer_pt.h"
#include "orca/normalizer/ko/pv_normalizer_verbalizer_ko.h"
#include "orca/normalizer/ja/pv_normalizer_verbalizer_ja.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_normalizer_mock.h"

#endif

struct pv_normalizer_verbalizer {
    pv_normalizer_language_t language;
    void *verbalizer;
};

pv_status_t PV_MOCKABLE(pv_normalizer_verbalizer_init)(
        const pv_normalizer_language_t language,
        int32_t num_use_cases,
        const int32_t *use_cases,
        pv_noun_gender_dict_t *noun_gender_dict,
        pv_normalizer_verbalizer_t **object) {
    PV_ASSERT(language >= 0);
    PV_ASSERT(num_use_cases >= 0);
    PV_ASSERT(use_cases);
    PV_ASSERT(object);

    *object = NULL;

    pv_normalizer_verbalizer_t *o = calloc(1, sizeof(pv_normalizer_verbalizer_t));
    if (!o) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->language = language;

    switch (o->language) {
        case PV_NORMALIZER_LANGUAGE_EN: {
            pv_status_t status = pv_normalizer_verbalizer_en_init(
                    num_use_cases,
                    (const pv_normalizer_use_cases_en_t *) use_cases,
                    (pv_normalizer_verbalizer_en_t **) (&(o->verbalizer)));
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_verbalizer_en_init,
                        pv_status_to_string(status));
                pv_normalizer_verbalizer_delete(o);
                return status;
            }
        } break;
        case PV_NORMALIZER_LANGUAGE_DE: {
            pv_status_t status = pv_normalizer_verbalizer_de_init(
                    num_use_cases,
                    (const pv_normalizer_use_cases_de_t *) use_cases,
                    (pv_normalizer_verbalizer_de_t **) (&(o->verbalizer)));
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_verbalizer_de_init,
                        pv_status_to_string(status));
                pv_normalizer_verbalizer_delete(o);
                return status;
            }
        } break;
        case PV_NORMALIZER_LANGUAGE_FR: {
            pv_status_t status = pv_normalizer_verbalizer_fr_init(
                    num_use_cases,
                    (const pv_normalizer_use_cases_fr_t *) use_cases,
                    (pv_normalizer_verbalizer_fr_t **) (&(o->verbalizer)));
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_verbalizer_fr_init,
                        pv_status_to_string(status));
                pv_normalizer_verbalizer_delete(o);
                return status;
            }
        } break;
        case PV_NORMALIZER_LANGUAGE_ES: {
            pv_status_t status = pv_normalizer_verbalizer_es_init(
                    num_use_cases,
                    (const pv_normalizer_use_cases_es_t *) use_cases,
                    (pv_normalizer_verbalizer_es_t **) (&(o->verbalizer)));
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_verbalizer_es_init,
                        pv_status_to_string(status));
                pv_normalizer_verbalizer_delete(o);
                return status;
            }
        } break;
        case PV_NORMALIZER_LANGUAGE_IT: {
            pv_status_t status = pv_normalizer_verbalizer_it_init(
                    num_use_cases,
                    (const pv_normalizer_use_cases_it_t *) use_cases,
                    noun_gender_dict,
                    (pv_normalizer_verbalizer_it_t **) (&(o->verbalizer)));
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_verbalizer_it_init,
                        pv_status_to_string(status));
                pv_normalizer_verbalizer_delete(o);
                return status;
            }
        } break;
        case PV_NORMALIZER_LANGUAGE_PT: {
            pv_status_t status = pv_normalizer_verbalizer_pt_init(
                    num_use_cases,
                    (const pv_normalizer_use_cases_pt_t *) use_cases,
                    (pv_normalizer_verbalizer_pt_t **) (&(o->verbalizer)));
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_verbalizer_pt_init,
                        pv_status_to_string(status));
                pv_normalizer_verbalizer_delete(o);
                return status;
            }
        } break;
        case PV_NORMALIZER_LANGUAGE_JA: {
            pv_status_t status = pv_normalizer_verbalizer_ja_init(
                    num_use_cases,
                    (const pv_normalizer_use_cases_ja_t *) use_cases,
                    (pv_normalizer_verbalizer_ja_t **) (&(o->verbalizer)));
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_verbalizer_ja_init,
                        pv_status_to_string(status));
                pv_normalizer_verbalizer_delete(o);
                return status;
            }
        } break;
        case PV_NORMALIZER_LANGUAGE_KO: {
            pv_status_t status = pv_normalizer_verbalizer_ko_init(
                    num_use_cases,
                    (const pv_normalizer_use_cases_ko_t *) use_cases,
                    (pv_normalizer_verbalizer_ko_t **) (&(o->verbalizer)));
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_verbalizer_pt_init,
                        pv_status_to_string(status));
                pv_normalizer_verbalizer_delete(o);
                return status;
            }
        } break;
        default: {
            PV_ERROR_REPORT(
                    &pv_error_msg_invalid_argument,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("language"));
            pv_normalizer_verbalizer_delete(o);
            return PV_STATUS_INVALID_ARGUMENT;
        }
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_normalizer_verbalizer_delete)(pv_normalizer_verbalizer_t *object) {
    if (object) {
        switch (object->language) {
            case PV_NORMALIZER_LANGUAGE_EN:
                pv_normalizer_verbalizer_en_delete(object->verbalizer);
                break;
            case PV_NORMALIZER_LANGUAGE_DE:
                pv_normalizer_verbalizer_de_delete(object->verbalizer);
                break;
            case PV_NORMALIZER_LANGUAGE_FR:
                pv_normalizer_verbalizer_fr_delete(object->verbalizer);
                break;
            case PV_NORMALIZER_LANGUAGE_ES:
                pv_normalizer_verbalizer_es_delete(object->verbalizer);
                break;
            case PV_NORMALIZER_LANGUAGE_IT:
                pv_normalizer_verbalizer_it_delete(object->verbalizer);
                break;
            case PV_NORMALIZER_LANGUAGE_PT:
                pv_normalizer_verbalizer_pt_delete(object->verbalizer);
                break;
            case PV_NORMALIZER_LANGUAGE_KO:
                pv_normalizer_verbalizer_ko_delete(object->verbalizer);
                break;
            case PV_NORMALIZER_LANGUAGE_JA:
                pv_normalizer_verbalizer_ja_delete(object->verbalizer);
                break;
            default:
                break;
        }
        free(object);
    }
}

pv_status_t PV_MOCKABLE(pv_normalizer_verbalizer_verbalize)(
        pv_normalizer_verbalizer_t *object,
        pv_normalizer_token_list_t *token_list,
        int32_t num_tokens_skip) {
    PV_ASSERT(object);
    PV_ASSERT(token_list);
    PV_ASSERT(num_tokens_skip >= 0);

    switch (object->language) {
        case PV_NORMALIZER_LANGUAGE_EN: {
            pv_status_t status = pv_normalizer_verbalizer_en_verbalize(object->verbalizer, token_list, num_tokens_skip);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_verbalizer_en_verbalize,
                        pv_status_to_string(status));
                return status;
            }
        } break;
        case PV_NORMALIZER_LANGUAGE_DE: {
            pv_status_t status = pv_normalizer_verbalizer_de_verbalize(object->verbalizer, token_list, num_tokens_skip);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_verbalizer_de_verbalize,
                        pv_status_to_string(status));
                return status;
            }
        } break;
        case PV_NORMALIZER_LANGUAGE_FR: {
            pv_status_t status = pv_normalizer_verbalizer_fr_verbalize(object->verbalizer, token_list, num_tokens_skip);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_verbalizer_fr_verbalize,
                        pv_status_to_string(status));
                return status;
            }
        } break;
        case PV_NORMALIZER_LANGUAGE_ES: {
            pv_status_t status = pv_normalizer_verbalizer_es_verbalize(object->verbalizer, token_list, num_tokens_skip);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_verbalizer_es_verbalize,
                        pv_status_to_string(status));
                return status;
            }
        } break;
        case PV_NORMALIZER_LANGUAGE_IT: {
            pv_status_t status = pv_normalizer_verbalizer_it_verbalize(object->verbalizer, token_list, num_tokens_skip);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_verbalizer_it_verbalize,
                        pv_status_to_string(status));
                return status;
            }
        } break;
        case PV_NORMALIZER_LANGUAGE_PT: {
            pv_status_t status = pv_normalizer_verbalizer_pt_verbalize(object->verbalizer, token_list, num_tokens_skip);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_verbalizer_pt_verbalize,
                        pv_status_to_string(status));
                return status;
            }
        } break;
        case PV_NORMALIZER_LANGUAGE_KO: {
            pv_status_t status = pv_normalizer_verbalizer_ko_verbalize(object->verbalizer, token_list, num_tokens_skip);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_verbalizer_ko_verbalize,
                        pv_status_to_string(status));
                return status;
            }
        } break;
        case PV_NORMALIZER_LANGUAGE_JA: {
            pv_status_t status = pv_normalizer_verbalizer_ja_verbalize(object->verbalizer, token_list, num_tokens_skip);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_verbalizer_ja_verbalize,
                        pv_status_to_string(status));
                return status;
            }
        } break;
        default:
            return PV_STATUS_INVALID_STATE;
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_normalizer_verbalizer_verbalize_word_common)(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    char *token_verbalized = calloc(strlen(token->string) + 1, sizeof(char));
    if (!token_verbalized) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("token_verbalized"));
        return PV_STATUS_OUT_OF_MEMORY;
    }
    strcpy(token_verbalized, token->string);
    pv_normalizer_token_set_verbalized(token, token_verbalized);

    pv_status_t status = pv_normalizer_util_upper_inplace(token->verbalized);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_util_upper_inplace,
                pv_status_to_string(status));
        return status;
    }

    return PV_STATUS_SUCCESS;
}
