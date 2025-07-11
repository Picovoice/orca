#include <string.h>

#include "core/picovoice.h"
#include "core/pv_assert.h"
#include "core/pv_error_messages.h"
#include "core/pv_type.h"
#include "orca/normalizer/it/pv_normalizer_data_it/pv_normalizer_data_it.h"
#include "orca/normalizer/it/pv_normalizer_data_it/pv_normalizer_word_character_data_it.h"
#include "orca/normalizer/it/pv_normalizer_tags_it.h"
#include "orca/normalizer/it/pv_normalizer_util_it.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_normalizer_mock.h"

#endif

bool PV_MOCKABLE(pv_normalizer_util_it_is_special_character)(const char *character, int32_t *length) {
    PV_ASSERT(character);
    PV_ASSERT(length);

    for (int32_t i = 0; i < PV_ARRAY_LEN(PV_NORMALIZER_SPECIAL_CHARACTERS_IT); ++i) {
        if (strncmp(character, PV_NORMALIZER_SPECIAL_CHARACTERS_IT[i], strlen(PV_NORMALIZER_SPECIAL_CHARACTERS_IT[i])) == 0) {
            *length = (int32_t) strlen(PV_NORMALIZER_SPECIAL_CHARACTERS_IT[i]);
            return true;
        }
    }
    return false;
}

bool PV_MOCKABLE(pv_normalizer_util_it_is_word_character)(const char *character) {
    PV_ASSERT(character);

    for (int32_t i = 0; i < PV_ARRAY_LEN(PV_NORMALIZER_WORD_CHARACTERS_IT); ++i) {
        if (strcmp(character, PV_NORMALIZER_WORD_CHARACTERS_IT[i]) == 0) {
            return true;
        }
    }
    return false;
}

bool PV_MOCKABLE(pv_normalizer_util_it_is_punctuation)(const char *character) {
    PV_ASSERT(character);

    for (int32_t i = 0; i < PV_ARRAY_LEN(PV_NORMALIZER_PUNCTUATION_IT); ++i) {
        if (strcmp(character, PV_NORMALIZER_PUNCTUATION_IT[i]) == 0) {
            return true;
        }
    }
    return false;
}

bool PV_MOCKABLE(pv_normalizer_util_it_is_normalizable_character)(const char *character) {
    PV_ASSERT(character);

    for (int32_t i = 0; i < PV_ARRAY_LEN(PV_NORMALIZER_NORMALIZABLE_CHARACTERS_IT); ++i) {
        if (strcmp(character, PV_NORMALIZER_NORMALIZABLE_CHARACTERS_IT[i]) == 0) {
            return true;
        }
    }
    return false;
}

bool PV_MOCKABLE(pv_normalizer_util_it_is_vowel)(const char *character) {
    PV_ASSERT(character);

    for (int32_t i = 0; i < PV_ARRAY_LEN(PV_NORMALIZER_VOWELS_IT); ++i) {
        if (strcmp(character, PV_NORMALIZER_VOWELS_IT[i]) == 0) {
            return true;
        }
    }
    return false;
}

bool PV_MOCKABLE(pv_normalizer_util_it_is_consonant)(char character) {
    // Italian consonants are all single-byte character.
    for (int32_t i = 0; i < PV_ARRAY_LEN(PV_NORMALIZER_CONSONANTS_IT); ++i) {
        if (character == PV_NORMALIZER_CONSONANTS_IT[i]) {
            return true;
        }
    }
    return false;
}

bool PV_MOCKABLE(pv_normalizer_util_it_is_alphabet_character)(const char *character) {
    PV_ASSERT(character);

    for (int32_t i = 0; i < PV_ARRAY_LEN(PV_NORMALIZER_ALPHABET_CHARACTERS_IT); ++i) {
        if (strcmp(character, PV_NORMALIZER_ALPHABET_CHARACTERS_IT[i]) == 0) {
            return true;
        }
    }
    return false;
}

bool PV_MOCKABLE(pv_normalizer_util_it_found_previous_one)(const pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    pv_normalizer_token_t *prev_token = pv_normalizer_token_get_nth_token_before(token, 1, true);
    return (prev_token != NULL && prev_token->tag == PV_NORMALIZER_TAG_IT_CARDINAL_ONE) ? true : false;
}

bool PV_MOCKABLE(pv_normalizer_util_it_found_previous_negative_one)(const pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    pv_normalizer_token_t *prev_token = pv_normalizer_token_get_nth_token_before(token, 1, true);
    return (prev_token != NULL && prev_token->tag == PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL_ONE) ? true : false;
}

bool PV_MOCKABLE(pv_normalizer_util_it_is_word_token)(const pv_normalizer_token_t *token) {
    return (token->tag == PV_NORMALIZER_TAG_IT_WORD);
}
