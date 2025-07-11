#include <ctype.h>
#include <string.h>

#include "core/picovoice.h"
#include "core/pv_assert.h"
#include "core/pv_error_messages.h"
#include "core/pv_language.h"
#include "core/pv_type.h"
#include "orca/normalizer/ja/pv_normalizer_data_ja/pv_normalizer_data_ja.h"
#include "orca/normalizer/ja/pv_normalizer_tags_ja.h"
#include "orca/normalizer/pv_normalizer_token.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_normalizer_mock.h"

#endif


const char DOUBLE_HYPHEN[] = "゠";
const char MIDDLE_DOT[] = "・";


static int32_t get_unicode_value(const unsigned char *bytes) {
    PV_ASSERT(bytes);

    int32_t unicode_val = 0;
    if ((bytes[0] & 0x80) == 0) {
        unicode_val = bytes[0];
    } else if ((bytes[0] & 0xE0) == 0xC0) {
        unicode_val = ((bytes[0] & 0x1F) << 6) | (bytes[1] & 0x3F);
    } else if ((bytes[0] & 0xF0) == 0xE0) {
        unicode_val = ((bytes[0] & 0x0F) << 12) |
                      ((bytes[1] & 0x3F) << 6) |
                      (bytes[2] & 0x3F);
    } else if ((bytes[0] & 0xF8) == 0xF0) {
        unicode_val = ((bytes[0] & 0x07) << 18) |
                      ((bytes[1] & 0x3F) << 12) |
                      ((bytes[2] & 0x3F) << 6) |
                      (bytes[3] & 0x3F);
    }

    return unicode_val;
}

pv_status_t PV_MOCKABLE(pv_normalizer_util_ja_normalize_full_width_text)(
        const char *text,
        char **normalized_text) {
    PV_ASSERT(text);
    PV_ASSERT(normalized_text);

    *normalized_text = NULL;

    size_t i = 0;
    int32_t normalize_text_size = 0;
    while (i < strlen(text)) {
        unsigned char *unsigned_text = ((unsigned char *) &text[i]);

        int32_t num_bytes_character = 0;
        pv_status_t status = pv_language_num_bytes_character(unsigned_text[0], &num_bytes_character);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_language_num_bytes_character,
                    pv_status_to_string(status));
            return status;
        }

        int32_t unicode_val = get_unicode_value(unsigned_text);
        if ((num_bytes_character == 3) && unicode_val >= 0xFF01 && unicode_val <= 0xFF5E) {
            normalize_text_size++;
        } else {
            normalize_text_size += num_bytes_character;
        }
        i += num_bytes_character;
    }

    char *norm = calloc(normalize_text_size + 1, sizeof(char));
    if (!norm) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("norm"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    i = 0;
    int32_t offset = 0;
    while (i < strlen(text)) {
        unsigned char *unsigned_text = ((unsigned char *) &text[i]);

        int32_t num_bytes_character = 0;
        pv_status_t status = pv_language_num_bytes_character(unsigned_text[0], &num_bytes_character);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_language_num_bytes_character,
                    pv_status_to_string(status));
            free(norm);
            return status;
        }

        int32_t unicode_val = get_unicode_value(unsigned_text);
        if (num_bytes_character == 3) {
            if ((unicode_val >= 0xFF01) && (unicode_val <= 0xFF5E)) {
                norm[offset++] = (char) (unicode_val - 0xFEE0);
            } else if (unicode_val == 0xFF64) {
                norm[offset++] = ',';
            } else if (unicode_val == 0x301C) {
                norm[offset++] = '~';
            } else {
                memcpy(norm + offset, text + i, num_bytes_character);
                offset += num_bytes_character;
            }
        } else {
            memcpy(norm + offset, text + i, num_bytes_character);
            offset += num_bytes_character;
        }
        i += num_bytes_character;
    }

    *normalized_text = norm;

    return PV_STATUS_SUCCESS;
}

bool PV_MOCKABLE(pv_normalizer_util_ja_is_special_character)(
        const char *character,
        int32_t *length) {
    PV_ASSERT(character);

    char *normalized_character = NULL;
    pv_status_t status = pv_normalizer_util_ja_normalize_full_width_text(character, &normalized_character);
    if (status != PV_STATUS_SUCCESS) {
        return false;
    }

    for (int32_t i = 0; i < PV_NORMALIZER_NUM_SPECIAL_CHARACTERS_JA; i++) {
        const char *special_character = PV_NORMALIZER_SPECIAL_CHARACTERS_JA[i];
        if (strncmp(normalized_character, special_character, strlen(special_character)) == 0) {
            *length = (int32_t) strlen(special_character);
            free(normalized_character);
            return true;
        }
    }
    free(normalized_character);
    return false;
}

bool PV_MOCKABLE(pv_normalizer_util_ja_is_word_character)(const char *character) {
    PV_ASSERT(character);

    int32_t num_bytes = 0;
    pv_status_t status = pv_language_num_bytes_character((unsigned char) character[0], &num_bytes);
    if (status != PV_STATUS_SUCCESS) {
        return false;
    }

    if (num_bytes != 3) {
        return false;
    }

    int32_t unicode_val = ((character[0] & 0x0F) << 12) | ((character[1] & 0x3F) << 6) | (character[2] & 0x3F);
    if (((unicode_val >= 0x3040) && (unicode_val <= 0x309F)) || // hiragana
        ((unicode_val >= 0x30A0) && (unicode_val <= 0x30FF)) || // katakana
        ((unicode_val >= 0x4E00) && (unicode_val <= 0x9FFF))) { // kanji
        return true;
    }

    return false;
}

bool PV_MOCKABLE(pv_normalizer_util_ja_is_punctuation)(const char *character) {
    PV_ASSERT(character);

    char *normalized_character = NULL;
    pv_status_t status = pv_normalizer_util_ja_normalize_full_width_text(character, &normalized_character);
    if (status != PV_STATUS_SUCCESS) {
        return false;
    }

    for (int32_t i = 0; i < PV_NORMALIZER_NUM_PUNCTUATION_JA; i++) {
        if (strcmp(normalized_character, PV_NORMALIZER_PUNCTUATION_JA[i]) == 0) {
            free(normalized_character);
            return true;
        }
    }

    free(normalized_character);
    return false;
}

bool PV_MOCKABLE(pv_normalizer_util_ja_is_normalizable_character)(const char *character) {
    PV_ASSERT(character);

    int32_t num_bytes = 0;
    pv_status_t status = pv_language_num_bytes_character((unsigned char) character[0], &num_bytes);
    if (status != PV_STATUS_SUCCESS) {
        return false;
    }

    if (num_bytes == 3) {
        int32_t unicode_val = ((character[0] & 0x0F) << 12) | ((character[1] & 0x3F) << 6) | (character[2] & 0x3F);
        if (((unicode_val >= 0x3001) && (unicode_val <= 0x301F)) || // punctuation
            ((unicode_val >= 0x3040) && (unicode_val <= 0x309F)) || // hiragana
            ((unicode_val >= 0x30A0) && (unicode_val <= 0x30FF)) || // katakana
            ((unicode_val >= 0x4E00) && (unicode_val <= 0x9FFF))) { // kanji
            return true;
        }
    }

    for (int32_t i = 0; i < PV_NORMALIZER_NUM_ADDITIONAL_NORMALIZABLE_CHARACTERS_JA; i++) {
        if (strcmp(character, PV_NORMALIZER_ADDITIONAL_NORMALIZABLE_CHARACTERS_JA[i]) == 0) {
            return true;
        }
    }

    return false;
}

pv_status_t PV_MOCKABLE(pv_normalizer_util_ja_is_capitalized_word)(
        const char *string,
        bool *is_capitalized_word) {
    PV_ASSERT(string);
    PV_ASSERT(is_capitalized_word);

    *is_capitalized_word = false;

    if (strlen(string) == 0) {
        *is_capitalized_word = false;
        return PV_STATUS_SUCCESS;
    }

    int32_t num_bytes_character = 0;
    pv_status_t status = pv_language_num_bytes_character((unsigned char) string[0], &num_bytes_character);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_language_num_bytes_character,
                pv_status_to_string(status));
        return status;
    }

    if ((num_bytes_character != 1) || (!isalpha(string[0]))) {
        *is_capitalized_word = false;
        return PV_STATUS_SUCCESS;
    }

    *is_capitalized_word = isupper(string[0]);
    return PV_STATUS_SUCCESS;
}

bool PV_MOCKABLE(pv_normalizer_util_ja_is_word_token)(const pv_normalizer_token_t *token) {
    return (token->tag == PV_NORMALIZER_TAG_JA_WORD);
}


bool PV_MOCKABLE(pv_normalizer_util_ja_is_skippable_word_separator)(const char *string) {
    return ((strcmp(string, DOUBLE_HYPHEN) == 0) || (strcmp(string, MIDDLE_DOT) == 0));
}
