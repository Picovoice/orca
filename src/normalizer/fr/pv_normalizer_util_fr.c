#include <ctype.h>
#include <string.h>

#include "core/picovoice.h"
#include "core/pv_assert.h"
#include "core/pv_error_messages.h"
#include "core/pv_type.h"
#include "orca/normalizer/fr/pv_normalizer_data_fr/pv_normalizer_data_fr.h"
#include "orca/normalizer/fr/pv_normalizer_tags_fr.h"
#include "orca/normalizer/pv_normalizer_token.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_normalizer_mock.h"

#endif

static const char *UPPER_CHARS[] = {
        "Á",
        "É",
        "Í",
        "Ó",
        "Ú",
        "Ü",
};

bool PV_MOCKABLE(pv_normalizer_util_fr_is_special_character)(const char *character, int32_t *length) {
    PV_ASSERT(character);
    PV_ASSERT(length);

    for (int32_t i = 0; i < PV_ARRAY_LEN(PV_NORMALIZER_SPECIAL_CHARACTERS_FR); ++i) {
        if (strncmp(character, PV_NORMALIZER_SPECIAL_CHARACTERS_FR[i], strlen(PV_NORMALIZER_SPECIAL_CHARACTERS_FR[i])) == 0) {
            *length = (int32_t) strlen(PV_NORMALIZER_SPECIAL_CHARACTERS_FR[i]);
            return true;
        }
    }
    return false;
}

bool PV_MOCKABLE(pv_normalizer_util_fr_is_word_character)(const char *character) {
    PV_ASSERT(character);

    for (int32_t i = 0; i < PV_ARRAY_LEN(PV_NORMALIZER_WORD_CHARACTERS_FR); ++i) {
        if (strcmp(character, PV_NORMALIZER_WORD_CHARACTERS_FR[i]) == 0) {
            return true;
        }
    }
    return false;
}

bool PV_MOCKABLE(pv_normalizer_util_fr_is_punctuation)(const char *character) {
    PV_ASSERT(character);

    for (int32_t i = 0; i < PV_ARRAY_LEN(PV_NORMALIZER_PUNCTUATION_FR); ++i) {
        if (strcmp(character, PV_NORMALIZER_PUNCTUATION_FR[i]) == 0) {
            return true;
        }
    }
    return false;
}

bool PV_MOCKABLE(pv_normalizer_util_fr_is_normalizable_character)(const char *character) {
    PV_ASSERT(character);

    for (int32_t i = 0; i < PV_ARRAY_LEN(PV_NORMALIZER_NORMALIZABLE_CHARACTERS_FR); ++i) {
        if (strcmp(character, PV_NORMALIZER_NORMALIZABLE_CHARACTERS_FR[i]) == 0) {
            return true;
        }
    }
    return false;
}

bool PV_MOCKABLE(pv_normalizer_util_fr_is_capitalized)(const char *character) {
    PV_ASSERT(character);

    if (isupper(character[0])) {
        return true;
    }

    for (int32_t i = 0; i < PV_ARRAY_LEN(UPPER_CHARS); ++i) {
        int32_t upper_char_length = (int32_t) strlen(UPPER_CHARS[i]);
        if (strncmp(character, UPPER_CHARS[i], upper_char_length) == 0) {
            return true;
        }
    }

    return false;
}

bool PV_MOCKABLE(pv_normalizer_util_fr_is_alphabet_character)(const char *character) {
    PV_ASSERT(character);

    for (int32_t i = 0; i < PV_ARRAY_LEN(PV_NORMALIZER_ALPHABET_CHARACTERS_FR); ++i) {
        if (strcmp(character, PV_NORMALIZER_ALPHABET_CHARACTERS_FR[i]) == 0) {
            return true;
        }
    }
    return false;
}

bool PV_MOCKABLE(pv_normalizer_util_fr_is_word_token)(const pv_normalizer_token_t *token) {
    return (token->tag == PV_NORMALIZER_TAG_FR_WORD);
}
