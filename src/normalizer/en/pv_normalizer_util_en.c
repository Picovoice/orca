#include <string.h>

#include "core/picovoice.h"
#include "core/pv_assert.h"
#include "core/pv_error_messages.h"
#include "core/pv_type.h"
#include "orca/normalizer/en/pv_normalizer_data_en/pv_normalizer_data_en.h"
#include "orca/normalizer/en/pv_normalizer_tags_en.h"
#include "orca/normalizer/pv_normalizer_token.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_normalizer_mock.h"

#endif

bool PV_MOCKABLE(pv_normalizer_util_en_is_special_character)(const char *character, int32_t *length) {
    PV_ASSERT(character);
    PV_ASSERT(length);

    for (int32_t i = 0; i < PV_ARRAY_LEN(PV_NORMALIZER_SPECIAL_CHARACTERS); ++i) { // TODO: Want to refactor this with "_EN" suffix, similarly for all things in each language's data directory for English, because this is left our from an old refactoring.
        if (strncmp(character, PV_NORMALIZER_SPECIAL_CHARACTERS[i], strlen(PV_NORMALIZER_SPECIAL_CHARACTERS[i])) == 0) {
            *length = (int32_t) strlen(PV_NORMALIZER_SPECIAL_CHARACTERS[i]);
            return true;
        }
    }
    return false;
}

bool PV_MOCKABLE(pv_normalizer_util_en_is_word_character)(const char *character) {
    PV_ASSERT(character);

    for (int32_t i = 0; i < PV_ARRAY_LEN(PV_NORMALIZER_WORD_CHARACTERS_EN); ++i) {
        if (strcmp(character, PV_NORMALIZER_WORD_CHARACTERS_EN[i]) == 0) {
            return true;
        }
    }
    return false;
}

bool PV_MOCKABLE(pv_normalizer_util_en_is_punctuation)(const char *character) {
    PV_ASSERT(character);

    for (int32_t i = 0; i < PV_ARRAY_LEN(PV_NORMALIZER_PUNCTUATION_EN); ++i) {
        if (strcmp(character, PV_NORMALIZER_PUNCTUATION_EN[i]) == 0) {
            return true;
        }
    }
    return false;
}

bool PV_MOCKABLE(pv_normalizer_util_en_is_normalizable_character)(const char *character) {
    PV_ASSERT(character);

    for (int32_t i = 0; i < PV_ARRAY_LEN(PV_NORMALIZER_NORMALIZABLE_CHARACTERS_EN); ++i) {
        if (strcmp(character, PV_NORMALIZER_NORMALIZABLE_CHARACTERS_EN[i]) == 0) {
            return true;
        }
    }
    return false;
}

bool PV_MOCKABLE(pv_normalizer_util_en_is_alphabet_character)(const char *character) {
    PV_ASSERT(character);

    for (int32_t i = 0; i < PV_ARRAY_LEN(PV_NORMALIZER_ALPHABET_CHARACTERS_EN); ++i) {
        if (strcmp(character, PV_NORMALIZER_ALPHABET_CHARACTERS_EN[i]) == 0) {
            return true;
        }
    }
    return false;
}

bool PV_MOCKABLE(pv_normalizer_util_en_is_word_token)(const pv_normalizer_token_t *token) {
    return (token->tag == PV_NORMALIZER_TAG_EN_WORD);
}
