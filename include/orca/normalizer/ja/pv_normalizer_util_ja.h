#ifndef PV_NORMALIZER_UTIL_JA_H
#define PV_NORMALIZER_UTIL_JA_H

#include <stdbool.h>
#include <stdint.h>

#include "core/picovoice.h"
#include "orca/normalizer/pv_normalizer_token.h"

pv_status_t PV_MOCKABLE(pv_normalizer_util_ja_normalize_full_width_text)(
        const char *text,
        char **normalized_text);

bool PV_MOCKABLE(pv_normalizer_util_ja_is_special_character)(
        const char *character,
        int32_t *length);

bool PV_MOCKABLE(pv_normalizer_util_ja_is_word_character)(const char *character);

bool PV_MOCKABLE(pv_normalizer_util_ja_is_punctuation)(const char *character);

bool PV_MOCKABLE(pv_normalizer_util_ja_is_normalizable_character)(const char *character);

pv_status_t PV_MOCKABLE(pv_normalizer_util_ja_is_capitalized_word)(
        const char *string,
        bool *is_capitalized_word);

bool PV_MOCKABLE(pv_normalizer_util_ja_is_word_token)(const pv_normalizer_token_t *token);


bool PV_MOCKABLE(pv_normalizer_util_ja_is_skippable_word_separator)(const char *string);


#endif // PV_NORMALIZER_UTIL_JA_H
