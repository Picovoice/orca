#include <stdlib.h>
#include <string.h>

#include "core/picovoice.h"
#include "core/pv_assert.h"
#include "core/pv_error_messages.h"
#include "core/pv_type.h"
#include "hippo/pv_hippo_normalizer.h"
#include "orca/normalizer/ko/pv_normalizer_tags_ko.h"
#include "orca/normalizer/pv_normalizer_token.h"
#include "orca/normalizer/pv_normalizer_util.h"

#include "orca/normalizer/ko/pv_normalizer_data_ko/pv_normalizer_data_ko.h"

#ifdef __PV_MOCKS__

#include "hippo/mock/pv_hippo_normalizer_mock.h"
#include "orca/mock/pv_normalizer_mock.h"

#endif

bool PV_MOCKABLE(pv_normalizer_util_ko_is_special_character)(const char *character, int32_t *length) {
    PV_ASSERT(character);
    PV_ASSERT(length);

    for (int32_t i = 0; i < PV_ARRAY_LEN(PV_NORMALIZER_SPECIAL_CHARACTERS_KO); ++i) {
        if (strncmp(character, PV_NORMALIZER_SPECIAL_CHARACTERS_KO[i], strlen(PV_NORMALIZER_SPECIAL_CHARACTERS_KO[i])) == 0) {
            *length = (int32_t) strlen(PV_NORMALIZER_SPECIAL_CHARACTERS_KO[i]);
            return true;
        }
    }
    return false;
}

bool PV_MOCKABLE(pv_normalizer_util_ko_is_word_character)(const char *character) {
    PV_ASSERT(character);

    char *decomposed_character = NULL;
    pv_status_t status = pv_hippo_normalizer(character, &decomposed_character);
    if (status != PV_STATUS_SUCCESS) {
        return false;
    }

    int32_t total_length = (int32_t) strlen(decomposed_character);

    int32_t i_pos = 0;
    char jamo[PV_NORMALIZER_MAX_NUM_BYTES_PER_CHARACTER] = {0};

    while (i_pos < total_length) {
        int32_t num_bytes_character = 0;
        status = pv_language_num_bytes_character(decomposed_character[0], &num_bytes_character);
        if (status != PV_STATUS_SUCCESS) {
            free(decomposed_character);
            return false;
        }

        for (int32_t j = 0; j < num_bytes_character; j++) {
            jamo[j] = decomposed_character[i_pos + j];
        }
        jamo[num_bytes_character] = '\0';

        bool is_valid_character = false;
        for (int32_t i = 0; i < PV_ARRAY_LEN(PV_NORMALIZER_WORD_CHARACTERS_KO); ++i) {
            if (strcmp(jamo, PV_NORMALIZER_WORD_CHARACTERS_KO[i]) == 0) {
                is_valid_character = true;
                break;
            }
        }
        if (!is_valid_character) {
            free(decomposed_character);
            return false;
        }

        i_pos += num_bytes_character;
    }

    free(decomposed_character);
    return true;
}

bool PV_MOCKABLE(pv_normalizer_util_ko_is_punctuation)(const char *character) {
    PV_ASSERT(character);

    for (int32_t i = 0; i < PV_ARRAY_LEN(PV_NORMALIZER_PUNCTUATION_KO); ++i) {
        if (strcmp(character, PV_NORMALIZER_PUNCTUATION_KO[i]) == 0) {
            return true;
        }
    }
    return false;
}

bool PV_MOCKABLE(pv_normalizer_util_ko_is_normalizable_character)(const char *character) {
    PV_ASSERT(character);

    char *decomposed_character = NULL;
    pv_status_t status = pv_hippo_normalizer(character, &decomposed_character);
    if (status != PV_STATUS_SUCCESS) {
        return false;
    }

    int32_t total_length = (int32_t) strlen(decomposed_character);

    int32_t i_pos = 0;
    char jamo[PV_NORMALIZER_MAX_NUM_BYTES_PER_CHARACTER] = {0};

    while (i_pos < total_length) {
        int32_t num_bytes_character = 0;
        status = pv_language_num_bytes_character(decomposed_character[0], &num_bytes_character);
        if (status != PV_STATUS_SUCCESS) {
            free(decomposed_character);
            return false;
        }

        for (int32_t j = 0; j < num_bytes_character; j++) {
            jamo[j] = decomposed_character[i_pos + j];
        }
        jamo[num_bytes_character] = '\0';

        bool is_normalizable = false;
        for (int32_t i = 0; i < PV_ARRAY_LEN(PV_NORMALIZER_NORMALIZABLE_CHARACTERS_KO); ++i) {
            if (strcmp(jamo, PV_NORMALIZER_NORMALIZABLE_CHARACTERS_KO[i]) == 0) {
                is_normalizable = true;
                break;
            }
        }
        if (!is_normalizable) {
            free(decomposed_character);
            return false;
        }

        i_pos += num_bytes_character;
    }

    free(decomposed_character);
    return true;
}

bool PV_MOCKABLE(pv_normalizer_util_ko_is_word_token)(const pv_normalizer_token_t *token) {
    return (token->tag == PV_NORMALIZER_TAG_KO_WORD);
}
