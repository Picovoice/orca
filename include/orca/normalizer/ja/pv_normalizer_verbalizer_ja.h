#ifndef PV_NORMALIZER_VERBALIZER_JA_H
#define PV_NORMALIZER_VERBALIZER_JA_H

#include "orca/normalizer/pv_normalizer_token.h"
#include "orca/normalizer/pv_normalizer_use_cases.h"

typedef struct pv_normalizer_verbalizer_ja pv_normalizer_verbalizer_ja_t;

pv_status_t PV_MOCKABLE(pv_normalizer_verbalizer_ja_init)(
        int32_t num_use_cases,
        const pv_normalizer_use_cases_ja_t *use_cases,
        pv_normalizer_verbalizer_ja_t **object);

void PV_MOCKABLE(pv_normalizer_verbalizer_ja_delete)(pv_normalizer_verbalizer_ja_t *object);

pv_status_t PV_MOCKABLE(pv_normalizer_verbalizer_ja_verbalize)(
        pv_normalizer_verbalizer_ja_t *object,
        pv_normalizer_token_list_t *token_list,
        int32_t num_tokens_skip);

#endif // PV_NORMALIZER_VERBALIZER_JA_H
