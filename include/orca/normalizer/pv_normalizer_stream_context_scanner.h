#ifndef PV_NORMALIZER_STREAM_CONTEXT_SCANNER_H
#define PV_NORMALIZER_STREAM_CONTEXT_SCANNER_H

#include "core/picovoice.h"
#include "orca/normalizer/pv_normalizer_token.h"
#include "orca/normalizer/pv_normalizer_use_cases.h"

typedef struct pv_normalizer_stream_context_scanner pv_normalizer_stream_context_scanner_t;

pv_status_t PV_MOCKABLE(pv_normalizer_stream_context_scanner_init)(
        pv_normalizer_language_t language,
        pv_normalizer_stream_context_scanner_t **object);

void PV_MOCKABLE(pv_normalizer_stream_context_scanner_delete)(pv_normalizer_stream_context_scanner_t *object);

pv_status_t PV_MOCKABLE(pv_normalizer_stream_context_scanner_is_verbalizable)(
        const pv_normalizer_stream_context_scanner_t *object,
        const pv_normalizer_token_t *token,
        int32_t num_use_cases,
        const int32_t *use_cases,
        bool *is_verbalizable);

void PV_MOCKABLE(pv_normalizer_stream_context_scanner_remove_hyphen_only_tokens)(
        const pv_normalizer_stream_context_scanner_t *object,
        pv_normalizer_token_list_t *token_list);

#endif // PV_NORMALIZER_STREAM_CONTEXT_SCANNER_H
