#ifndef TEST_PV_NORMALIZER_STREAM_HELPER_H
#define TEST_PV_NORMALIZER_STREAM_HELPER_H

#include <stdint.h>
#include <stdio.h>

#include "orca/normalizer/pv_normalizer_stream.h"

void test_pv_normalizer_stream_helper(
        pv_normalizer_stream_t *normalizer_stream_object,
        int32_t num_tokens,
        char **tokens,
        pv_status_t *add_statuses,
        pv_status_t flush_status);

void test_pv_normalizer_stream_helper_verbalizable(
        pv_normalizer_stream_t *normalizer_stream_object,
        int32_t num_tokens,
        char **tokens,
        char **expected);

void test_pv_normalizer_stream_helper_phonemizable(
        pv_normalizer_stream_t *normalizer_stream_object,
        int32_t num_tokens,
        char **tokens,
        int32_t *expected_size,
        const char *expected);

#endif // TEST_PV_NORMALIZER_STREAM_HELPER_H
