#include "core/picovoice.h"
#include "core/pv_assert.h"
#include "core/pv_error_messages.h"
#include "core/pv_type.h"
#include "orca/normalizer/de/pv_normalizer_data_de/pv_normalizer_data_de.h"
#include "orca/normalizer/de/pv_normalizer_tags_de.h"
#include "orca/normalizer/pv_normalizer_token.h"
#include <string.h>

#ifdef __PV_MOCKS__

#include "orca/mock/pv_normalizer_mock.h"

#endif

bool PV_MOCKABLE(pv_normalizer_util_de_is_special_character)(const char *character, int32_t *length) {
    PV_ASSERT(character);
    PV_ASSERT(length);

    for (int32_t i = 0; i < PV_ARRAY_LEN(PV_NORMALIZER_SPECIAL_CHARACTERS_DE); ++i) {
        if (strncmp(character, PV_NORMALIZER_SPECIAL_CHARACTERS_DE[i], strlen(PV_NORMALIZER_SPECIAL_CHARACTERS_DE[i])) == 0) {
            *length = (int32_t) strlen(PV_NORMALIZER_SPECIAL_CHARACTERS_DE[i]);
            return true;
        }
    }
    return false;
}

bool PV_MOCKABLE(pv_normalizer_util_de_is_word_character)(const char *character) {
    PV_ASSERT(character);

    for (int32_t i = 0; i < PV_ARRAY_LEN(PV_NORMALIZER_WORD_CHARACTERS_DE); ++i) {
        if (strcmp(character, PV_NORMALIZER_WORD_CHARACTERS_DE[i]) == 0) {
            return true;
        }
    }
    return false;
}

bool PV_MOCKABLE(pv_normalizer_util_de_is_punctuation)(const char *character) {
    PV_ASSERT(character);

    for (int32_t i = 0; i < PV_ARRAY_LEN(PV_NORMALIZER_PUNCTUATION_DE); ++i) {
        if (strcmp(character, PV_NORMALIZER_PUNCTUATION_DE[i]) == 0) {
            return true;
        }
    }
    return false;
}

bool PV_MOCKABLE(pv_normalizer_util_de_is_normalizable_character)(const char *character) {
    PV_ASSERT(character);

    for (int32_t i = 0; i < PV_ARRAY_LEN(PV_NORMALIZER_NORMALIZABLE_CHARACTERS_DE); ++i) {
        if (strcmp(character, PV_NORMALIZER_NORMALIZABLE_CHARACTERS_DE[i]) == 0) {
            return true;
        }
    }
    return false;
}

bool PV_MOCKABLE(pv_normalizer_util_de_is_alphabet_character)(const char *character) {
    PV_ASSERT(character);

    for (int32_t i = 0; i < PV_ARRAY_LEN(PV_NORMALIZER_ALPHABET_CHARACTERS_DE); ++i) {
        if (strcmp(character, PV_NORMALIZER_ALPHABET_CHARACTERS_DE[i]) == 0) {
            return true;
        }
    }
    return false;
}

bool PV_MOCKABLE(pv_normalizer_util_de_is_word_token)(const pv_normalizer_token_t *token) {
    return (token->tag == PV_NORMALIZER_TAG_DE_WORD);
}
