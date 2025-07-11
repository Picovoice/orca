#ifndef PV_NORMALIZER_STREAM_CONTEXT_SCANNER_ES_H
#define PV_NORMALIZER_STREAM_CONTEXT_SCANNER_ES_H

#include "core/picovoice.h"
#include "orca/normalizer/pv_normalizer_token.h"
#include "orca/normalizer/pv_normalizer_use_cases.h"

bool PV_MOCKABLE(pv_normalizer_stream_context_scanner_es_is_verbalizable)(
        const pv_normalizer_token_t *token,
        int32_t num_use_cases,
        const pv_normalizer_use_cases_es_t *use_cases);

void PV_MOCKABLE(pv_normalizer_stream_context_scanner_es_remove_hyphen_only_tokens)(
        pv_normalizer_token_list_t *token_list);

#endif // PV_NORMALIZER_STREAM_CONTEXT_SCANNER_ES_H
