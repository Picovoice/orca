#ifndef PV_NORMALIZER_VERBALIZER_PT_H
#define PV_NORMALIZER_VERBALIZER_PT_H

#include "orca/normalizer/pv_normalizer_token.h"
#include "orca/normalizer/pv_normalizer_use_cases.h"

typedef struct pv_normalizer_verbalizer_pt pv_normalizer_verbalizer_pt_t;

pv_status_t PV_MOCKABLE(pv_normalizer_verbalizer_pt_init)(
        int32_t num_use_cases,
        const pv_normalizer_use_cases_pt_t *use_cases,
        pv_normalizer_verbalizer_pt_t **object);

void PV_MOCKABLE(pv_normalizer_verbalizer_pt_delete)(pv_normalizer_verbalizer_pt_t *object);

pv_status_t PV_MOCKABLE(pv_normalizer_verbalizer_pt_verbalize)(
        pv_normalizer_verbalizer_pt_t *object,
        pv_normalizer_token_list_t *token_list,
        int32_t num_tokens_skip);

#endif // PV_NORMALIZER_VERBALIZER_PT_H
