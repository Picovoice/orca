#ifndef PV_NORMALIZER_UTIL_KO_H
#define PV_NORMALIZER_UTIL_KO_H

#include <stdbool.h>
#include <stdint.h>

#include "core/picovoice.h"
#include "orca/normalizer/pv_normalizer_token.h"

bool PV_MOCKABLE(pv_normalizer_util_ko_is_special_character)(const char *character, int32_t *length);

bool PV_MOCKABLE(pv_normalizer_util_ko_is_word_character)(const char *character);

bool PV_MOCKABLE(pv_normalizer_util_ko_is_punctuation)(const char *character);

bool PV_MOCKABLE(pv_normalizer_util_ko_is_normalizable_character)(const char *character);

bool PV_MOCKABLE(pv_normalizer_util_ko_is_word_token)(const pv_normalizer_token_t *token);

#endif // PV_NORMALIZER_UTIL_KO_H
