#ifndef TEST_PV_NORMALIZER_CASES_HELPER_H
#define TEST_PV_NORMALIZER_CASES_HELPER_H

#include <stdio.h>
#include <stdbool.h>

#include "orca/normalizer/pv_normalizer.h"
#include "tokenizer/pv_tokenizer.h"

typedef struct pv_normalizer_cases_helper pv_normalizer_cases_helper_t;

pv_status_t pv_normalizer_cases_helper_init(
        const char *file_path,
        pv_normalizer_cases_helper_t **object);

void pv_normalizer_cases_helper_delete(pv_normalizer_cases_helper_t *object);

void pv_normalizer_cases_helper_reindex(pv_normalizer_cases_helper_t *object);

bool pv_normalizer_cases_helper_next_case(
        pv_normalizer_cases_helper_t *object,
        int32_t *case_no,
        char *text_raw,
        int32_t text_raw_length,
        char *text_batch,
        int32_t text_length_batch,
        char *text_stream,
        int32_t text_length_stream);

pv_status_t pv_normalizer_cases_normalize_batch(
        pv_normalizer_t *normalizer_object,
        const char *input,
        char **output);

pv_status_t pv_normalizer_cases_normalize_stream(
        pv_tokenizer_t *stream_tokenizer,
        pv_normalizer_t *normalizer_object,
        bool add_spaces,
        const char *input,
        char **output);

bool pv_normalizer_cases_is_ignored(int32_t case_number, const char *language_code);

#endif // TEST_PV_NORMALIZER_CASES_HELPER_H
