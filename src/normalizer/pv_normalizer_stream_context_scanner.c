#include <stdlib.h>

#include "core/picovoice.h"
#include "core/pv_error_messages.h"
#include "orca/normalizer/de/pv_normalizer_stream_context_scanner_de.h"
#include "orca/normalizer/en/pv_normalizer_stream_context_scanner_en.h"
#include "orca/normalizer/es/pv_normalizer_stream_context_scanner_es.h"
#include "orca/normalizer/fr/pv_normalizer_stream_context_scanner_fr.h"
#include "orca/normalizer/it/pv_normalizer_stream_context_scanner_it.h"
#include "orca/normalizer/pt/pv_normalizer_stream_context_scanner_pt.h"
#include "orca/normalizer/ko/pv_normalizer_stream_context_scanner_ko.h"
#include "orca/normalizer/ja/pv_normalizer_stream_context_scanner_ja.h"
#include "orca/normalizer/pv_normalizer.h"
#include "orca/normalizer/pv_normalizer_stream_context_scanner.h"
#include "orca/normalizer/pv_normalizer_token.h"
#include "orca/normalizer/pv_normalizer_use_cases.h"
#include "orca/normalizer/pv_normalizer_util.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_normalizer_mock.h"

#endif

struct pv_normalizer_stream_context_scanner {
    pv_normalizer_language_t language;
};

pv_status_t PV_MOCKABLE(pv_normalizer_stream_context_scanner_init)(
        const pv_normalizer_language_t language,
        pv_normalizer_stream_context_scanner_t **object) {
    PV_ASSERT(language >= 0);
    PV_ASSERT(object);

    *object = NULL;

    pv_normalizer_stream_context_scanner_t *o = calloc(1, sizeof(pv_normalizer_stream_context_scanner_t));
    if (!o) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->language = language;

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_normalizer_stream_context_scanner_delete)(pv_normalizer_stream_context_scanner_t *object) {
    if (object) {
        free(object);
    }
}

pv_status_t PV_MOCKABLE(pv_normalizer_stream_context_scanner_is_verbalizable)(
        const pv_normalizer_stream_context_scanner_t *object,
        const pv_normalizer_token_t *token,
        int32_t num_use_cases,
        const int32_t *use_cases,
        bool *is_verbalizable) {
    PV_ASSERT(object);
    PV_ASSERT(token);
    PV_ASSERT(num_use_cases >= 0);
    PV_ASSERT(use_cases);
    PV_ASSERT(is_verbalizable);

    switch (object->language) {
        case PV_NORMALIZER_LANGUAGE_EN: {
            *is_verbalizable = pv_normalizer_stream_context_scanner_en_is_verbalizable(
                    token,
                    num_use_cases,
                    (const pv_normalizer_use_cases_en_t *) use_cases);
        } break;
        case PV_NORMALIZER_LANGUAGE_DE: {
            *is_verbalizable = pv_normalizer_stream_context_scanner_de_is_verbalizable(
                    token,
                    num_use_cases,
                    (const pv_normalizer_use_cases_de_t *) use_cases);
        } break;
        case PV_NORMALIZER_LANGUAGE_FR: {
            *is_verbalizable = pv_normalizer_stream_context_scanner_fr_is_verbalizable(
                    token,
                    num_use_cases,
                    (const pv_normalizer_use_cases_fr_t *) use_cases);
        } break;
        case PV_NORMALIZER_LANGUAGE_ES: {
            *is_verbalizable = pv_normalizer_stream_context_scanner_es_is_verbalizable(
                    token,
                    num_use_cases,
                    (const pv_normalizer_use_cases_es_t *) use_cases);
        } break;
        case PV_NORMALIZER_LANGUAGE_IT: {
            *is_verbalizable = pv_normalizer_stream_context_scanner_it_is_verbalizable(
                    token,
                    num_use_cases,
                    (const pv_normalizer_use_cases_it_t *) use_cases);
        } break;
        case PV_NORMALIZER_LANGUAGE_PT: {
            *is_verbalizable = pv_normalizer_stream_context_scanner_pt_is_verbalizable(
                    token,
                    num_use_cases,
                    (const pv_normalizer_use_cases_pt_t *) use_cases);
        } break;
        case PV_NORMALIZER_LANGUAGE_KO: {
            *is_verbalizable = pv_normalizer_stream_context_scanner_ko_is_verbalizable(
                    token,
                    num_use_cases,
                    (const pv_normalizer_use_cases_ko_t *) use_cases);
        } break;
        case PV_NORMALIZER_LANGUAGE_JA: {
            *is_verbalizable = pv_normalizer_stream_context_scanner_ja_is_verbalizable(
                    token,
                    num_use_cases,
                    (const pv_normalizer_use_cases_ja_t *) use_cases);
        } break;
        default:
            PV_ERROR_REPORT(
                    &pv_error_msg_invalid_argument_internal,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("language"));
            return PV_STATUS_INVALID_ARGUMENT;
    }
    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_normalizer_stream_context_scanner_remove_hyphen_only_tokens)(
        const pv_normalizer_stream_context_scanner_t *object,
        pv_normalizer_token_list_t *token_list) {
    PV_ASSERT(token_list);

    switch (object->language) {
        case PV_NORMALIZER_LANGUAGE_EN: {
            pv_normalizer_stream_context_scanner_en_remove_hyphen_only_tokens(token_list);
        } break;
        case PV_NORMALIZER_LANGUAGE_DE: {
            pv_normalizer_stream_context_scanner_de_remove_hyphen_only_tokens(token_list);
        } break;
        case PV_NORMALIZER_LANGUAGE_ES: {
            pv_normalizer_stream_context_scanner_es_remove_hyphen_only_tokens(token_list);
        } break;
        case PV_NORMALIZER_LANGUAGE_FR: {
            pv_normalizer_stream_context_scanner_fr_remove_hyphen_only_tokens(token_list);
        } break;
        case PV_NORMALIZER_LANGUAGE_IT: {
            pv_normalizer_stream_context_scanner_it_remove_hyphen_only_tokens(token_list);
        } break;
        case PV_NORMALIZER_LANGUAGE_PT: {
            pv_normalizer_stream_context_scanner_pt_remove_hyphen_only_tokens(token_list);
        } break;
        case PV_NORMALIZER_LANGUAGE_KO: {
            pv_normalizer_stream_context_scanner_ko_remove_hyphen_only_tokens(token_list);
        } break;
        case PV_NORMALIZER_LANGUAGE_JA: {
            pv_normalizer_stream_context_scanner_ja_remove_hyphen_only_tokens(token_list);
        } break;
        default:
            pv_normalizer_remove_hyphen_only_tokens(token_list);
            break;
    }
}
