#include <string.h>

#include "core/picovoice.h"
#include "core/pv_assert.h"
#include "core/pv_error_messages.h"
#include "core/pv_type.h"
#include "orca/normalizer/pt/pv_normalizer_data_pt/pv_normalizer_data_pt.h"
#include "orca/normalizer/pt/pv_normalizer_tags_pt.h"
#include "orca/normalizer/pv_normalizer_token.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_normalizer_mock.h"

#endif

bool PV_MOCKABLE(pv_normalizer_util_pt_is_special_character)(const char *character, int32_t *length) {
    PV_ASSERT(character);
    PV_ASSERT(length);

    for (int32_t i = 0; i < PV_ARRAY_LEN(PV_NORMALIZER_SPECIAL_CHARACTERS_PT); ++i) {
        if (strncmp(character, PV_NORMALIZER_SPECIAL_CHARACTERS_PT[i], strlen(PV_NORMALIZER_SPECIAL_CHARACTERS_PT[i])) == 0) {
            *length = (int32_t) strlen(PV_NORMALIZER_SPECIAL_CHARACTERS_PT[i]);
            return true;
        }
    }
    return false;
}

bool PV_MOCKABLE(pv_normalizer_util_pt_is_word_character)(const char *character) {
    PV_ASSERT(character);

    for (int32_t i = 0; i < PV_ARRAY_LEN(PV_NORMALIZER_WORD_CHARACTERS_PT); ++i) {
        if (strcmp(character, PV_NORMALIZER_WORD_CHARACTERS_PT[i]) == 0) {
            return true;
        }
    }
    return false;
}

bool PV_MOCKABLE(pv_normalizer_util_pt_is_punctuation)(const char *character) {
    PV_ASSERT(character);

    for (int32_t i = 0; i < PV_ARRAY_LEN(PV_NORMALIZER_PUNCTUATION_PT); ++i) {
        if (strcmp(character, PV_NORMALIZER_PUNCTUATION_PT[i]) == 0) {
            return true;
        }
    }
    return false;
}

bool PV_MOCKABLE(pv_normalizer_util_pt_is_normalizable_character)(const char *character) {
    PV_ASSERT(character);

    for (int32_t i = 0; i < PV_ARRAY_LEN(PV_NORMALIZER_NORMALIZABLE_CHARACTERS_PT); ++i) {
        if (strcmp(character, PV_NORMALIZER_NORMALIZABLE_CHARACTERS_PT[i]) == 0) {
            return true;
        }
    }
    return false;
}

bool PV_MOCKABLE(pv_normalizer_util_pt_is_alphabet_character)(const char *character) {
    PV_ASSERT(character);

    for (int32_t i = 0; i < PV_ARRAY_LEN(PV_NORMALIZER_ALPHABET_CHARACTERS_PT); ++i) {
        if (strcmp(character, PV_NORMALIZER_ALPHABET_CHARACTERS_PT[i]) == 0) {
            return true;
        }
    }
    return false;
}

bool PV_MOCKABLE(pv_normalizer_util_pt_is_word_token)(const pv_normalizer_token_t *token) {
    return (token->tag == PV_NORMALIZER_TAG_PT_WORD);
}
