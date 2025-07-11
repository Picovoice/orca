#include <ctype.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "core/picovoice.h"
#include "core/pv_assert.h"
#include "core/pv_error_messages.h"
#include "core/pv_language.h"
#include "orca/normalizer/pv_normalizer_data/pv_normalizer_shared_data.h"
#include "orca/normalizer/pv_normalizer_token.h"
#include "orca/normalizer/pv_normalizer_use_cases.h"
#include "orca/normalizer/pv_normalizer_util.h"

#include "orca/normalizer/de/pv_normalizer_data_de/pv_normalizer_data_de.h"
#include "orca/normalizer/de/pv_normalizer_util_de.h"
#include "orca/normalizer/en/pv_normalizer_data_en/pv_normalizer_data_en.h"
#include "orca/normalizer/en/pv_normalizer_util_en.h"
#include "orca/normalizer/es/pv_normalizer_data_es/pv_normalizer_data_es.h"
#include "orca/normalizer/es/pv_normalizer_util_es.h"
#include "orca/normalizer/fr/pv_normalizer_data_fr/pv_normalizer_data_fr.h"
#include "orca/normalizer/fr/pv_normalizer_util_fr.h"
#include "orca/normalizer/it/pv_normalizer_data_it/pv_normalizer_data_it.h"
#include "orca/normalizer/it/pv_normalizer_util_it.h"
#include "orca/normalizer/ja/pv_normalizer_data_ja/pv_normalizer_data_ja.h"
#include "orca/normalizer/ja/pv_normalizer_util_ja.h"
#include "orca/normalizer/ko/pv_normalizer_data_ko/pv_normalizer_data_ko.h"
#include "orca/normalizer/ko/pv_normalizer_util_ko.h"
#include "orca/normalizer/pt/pv_normalizer_data_pt/pv_normalizer_data_pt.h"
#include "orca/normalizer/pt/pv_normalizer_util_pt.h"

#include "orca/normalizer/pv_normalizer_language_data.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_normalizer_mock.h"

#endif


static pv_status_t pv_normalizer_util_remap_characters_helper(
        const char *text,
        char *remapped_text,
        int32_t *remapped_text_length,
        bool dry_run);


pv_status_t PV_MOCKABLE(pv_normalizer_util_is_special_character)(
        pv_normalizer_language_t language,
        const char *character,
        bool *is_special,
        int32_t *length) {
    PV_ASSERT(language >= 0);
    PV_ASSERT(character);
    PV_ASSERT(is_special);

    switch (language) {
        case PV_NORMALIZER_LANGUAGE_EN: {
            *is_special = pv_normalizer_util_en_is_special_character(character, length);
        } break;
        case PV_NORMALIZER_LANGUAGE_DE: {
            *is_special = pv_normalizer_util_de_is_special_character(character, length);
        } break;
        case PV_NORMALIZER_LANGUAGE_FR: {
            *is_special = pv_normalizer_util_fr_is_special_character(character, length);
        } break;
        case PV_NORMALIZER_LANGUAGE_ES: {
            *is_special = pv_normalizer_util_es_is_special_character(character, length);
        } break;
        case PV_NORMALIZER_LANGUAGE_IT: {
            *is_special = pv_normalizer_util_it_is_special_character(character, length);
        } break;
        case PV_NORMALIZER_LANGUAGE_PT: {
            *is_special = pv_normalizer_util_pt_is_special_character(character, length);
        } break;
        case PV_NORMALIZER_LANGUAGE_KO: {
            *is_special = pv_normalizer_util_ko_is_special_character(character, length);
        } break;
        case PV_NORMALIZER_LANGUAGE_JA: {
            *is_special = pv_normalizer_util_ja_is_special_character(character, length);
        } break;
        default:
            PV_ERROR_REPORT(
                    &pv_error_msg_invalid_argument_internal,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("language"));
            return PV_STATUS_INVALID_ARGUMENT;
    }
    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_normalizer_util_get_normalizable_characters)(
        pv_normalizer_language_t language,
        int32_t *num_characters,
        const char *const **characters) {
    PV_ASSERT(language >= 0);
    PV_ASSERT(num_characters);
    PV_ASSERT(characters);

    switch (language) {
        case PV_NORMALIZER_LANGUAGE_EN: {
            *num_characters = PV_NORMALIZER_NUM_NORMALIZABLE_CHARACTERS_EN;
            *characters = PV_NORMALIZER_NORMALIZABLE_CHARACTERS_EN;
        } break;
        case PV_NORMALIZER_LANGUAGE_DE: {
            *num_characters = PV_NORMALIZER_NUM_NORMALIZABLE_CHARACTERS_DE;
            *characters = PV_NORMALIZER_NORMALIZABLE_CHARACTERS_DE;
        } break;
        case PV_NORMALIZER_LANGUAGE_FR: {
            *num_characters = PV_NORMALIZER_NUM_NORMALIZABLE_CHARACTERS_FR;
            *characters = PV_NORMALIZER_NORMALIZABLE_CHARACTERS_FR;
        } break;
        case PV_NORMALIZER_LANGUAGE_ES: {
            *num_characters = PV_NORMALIZER_NUM_NORMALIZABLE_CHARACTERS_ES;
            *characters = PV_NORMALIZER_NORMALIZABLE_CHARACTERS_ES;
        } break;
        case PV_NORMALIZER_LANGUAGE_IT: {
            *num_characters = PV_NORMALIZER_NUM_NORMALIZABLE_CHARACTERS_IT;
            *characters = PV_NORMALIZER_NORMALIZABLE_CHARACTERS_IT;
        } break;
        case PV_NORMALIZER_LANGUAGE_PT: {
            *num_characters = PV_NORMALIZER_NUM_NORMALIZABLE_CHARACTERS_PT;
            *characters = PV_NORMALIZER_NORMALIZABLE_CHARACTERS_PT;
        } break;
        case PV_NORMALIZER_LANGUAGE_KO: {
            *num_characters = PV_NORMALIZER_NUM_NORMALIZABLE_CHARACTERS_KO;
            *characters = PV_NORMALIZER_NORMALIZABLE_CHARACTERS_KO;
        } break;
        case PV_NORMALIZER_LANGUAGE_JA: {
            *num_characters = PV_NORMALIZER_NUM_ADDITIONAL_NORMALIZABLE_CHARACTERS_JA;
            *characters = PV_NORMALIZER_ADDITIONAL_NORMALIZABLE_CHARACTERS_JA;
        } break;
        default:
            PV_ERROR_REPORT(
                    &pv_error_msg_invalid_argument_internal,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("language"));
            return PV_STATUS_INVALID_ARGUMENT;
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_normalizer_util_is_punctuation)(
        pv_normalizer_language_t language,
        const char *character,
        bool *is_punctuation) {
    PV_ASSERT(language >= 0);
    PV_ASSERT(character);
    PV_ASSERT(is_punctuation);

    switch (language) {
        case PV_NORMALIZER_LANGUAGE_EN: {
            *is_punctuation = pv_normalizer_util_en_is_punctuation(character);
        } break;
        case PV_NORMALIZER_LANGUAGE_DE: {
            *is_punctuation = pv_normalizer_util_de_is_punctuation(character);
        } break;
        case PV_NORMALIZER_LANGUAGE_FR: {
            *is_punctuation = pv_normalizer_util_fr_is_punctuation(character);
        } break;
        case PV_NORMALIZER_LANGUAGE_ES: {
            *is_punctuation = pv_normalizer_util_es_is_punctuation(character);
        } break;
        case PV_NORMALIZER_LANGUAGE_IT: {
            *is_punctuation = pv_normalizer_util_it_is_punctuation(character);
        } break;
        case PV_NORMALIZER_LANGUAGE_PT: {
            *is_punctuation = pv_normalizer_util_pt_is_punctuation(character);
        } break;
        case PV_NORMALIZER_LANGUAGE_KO: {
            *is_punctuation = pv_normalizer_util_ko_is_punctuation(character);
        } break;
        case PV_NORMALIZER_LANGUAGE_JA: {
            *is_punctuation = pv_normalizer_util_ja_is_punctuation(character);
        } break;
        default:
            PV_ERROR_REPORT(
                    &pv_error_msg_invalid_argument_internal,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("language"));
            return PV_STATUS_INVALID_ARGUMENT;
    }
    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_normalizer_util_is_word_character)(
        pv_normalizer_language_t language,
        const char *character,
        bool *is_word) {
    PV_ASSERT(language >= 0);
    PV_ASSERT(character);
    PV_ASSERT(is_word);

    switch (language) {
        case PV_NORMALIZER_LANGUAGE_EN: {
            *is_word = pv_normalizer_util_en_is_word_character(character);
        } break;
        case PV_NORMALIZER_LANGUAGE_DE: {
            *is_word = pv_normalizer_util_de_is_word_character(character);
        } break;
        case PV_NORMALIZER_LANGUAGE_FR: {
            *is_word = pv_normalizer_util_fr_is_word_character(character);
        } break;
        case PV_NORMALIZER_LANGUAGE_ES: {
            *is_word = pv_normalizer_util_es_is_word_character(character);
        } break;
        case PV_NORMALIZER_LANGUAGE_IT: {
            *is_word = pv_normalizer_util_it_is_word_character(character);
        } break;
        case PV_NORMALIZER_LANGUAGE_PT: {
            *is_word = pv_normalizer_util_pt_is_word_character(character);
        } break;
        case PV_NORMALIZER_LANGUAGE_KO: {
            *is_word = pv_normalizer_util_ko_is_word_character(character);
        } break;
        case PV_NORMALIZER_LANGUAGE_JA: {
            *is_word = pv_normalizer_util_ja_is_word_character(character);
        } break;
        default:
            PV_ERROR_REPORT(
                    &pv_error_msg_invalid_argument_internal,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("language"));
            return PV_STATUS_INVALID_ARGUMENT;
    }
    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_normalizer_util_is_normalizable_character)(
        pv_normalizer_language_t language,
        const char *character,
        bool *is_normalizable) {
    PV_ASSERT(language >= 0);
    PV_ASSERT(character);
    PV_ASSERT(is_normalizable);

    switch (language) {
        case PV_NORMALIZER_LANGUAGE_EN: {
            *is_normalizable = pv_normalizer_util_en_is_normalizable_character(character);
        } break;
        case PV_NORMALIZER_LANGUAGE_DE: {
            *is_normalizable = pv_normalizer_util_de_is_normalizable_character(character);
        } break;
        case PV_NORMALIZER_LANGUAGE_FR: {
            *is_normalizable = pv_normalizer_util_fr_is_normalizable_character(character);
        } break;
        case PV_NORMALIZER_LANGUAGE_ES: {
            *is_normalizable = pv_normalizer_util_es_is_normalizable_character(character);
        } break;
        case PV_NORMALIZER_LANGUAGE_IT: {
            *is_normalizable = pv_normalizer_util_it_is_normalizable_character(character);
        } break;
        case PV_NORMALIZER_LANGUAGE_PT: {
            *is_normalizable = pv_normalizer_util_pt_is_normalizable_character(character);
        } break;
        case PV_NORMALIZER_LANGUAGE_KO: {
            *is_normalizable = pv_normalizer_util_ko_is_normalizable_character(character);
        } break;
        case PV_NORMALIZER_LANGUAGE_JA: {
            *is_normalizable = pv_normalizer_util_ja_is_normalizable_character(character);
        } break;
        default:
            PV_ERROR_REPORT(
                    &pv_error_msg_invalid_argument_internal,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("language"));
            return PV_STATUS_INVALID_ARGUMENT;
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_normalizer_util_character_index)(
        const char *character,
        int32_t num_characters,
        const char *const *characters,
        int32_t *index) {
    PV_ASSERT(character);
    PV_ASSERT(num_characters > 0);
    PV_ASSERT(characters);
    PV_ASSERT(index);

    for (int32_t j = 0; j < num_characters; ++j) {
        if (strcmp(character, characters[j]) == 0) {
            *index = j;
            return PV_STATUS_SUCCESS;
        }
    }
    return PV_STATUS_INVALID_ARGUMENT;
}

pv_status_t PV_MOCKABLE(pv_normalizer_util_validate_text)(
        pv_normalizer_language_t language,
        const pv_language_info_t *language_info,
        const char *text,
        bool preserve_word_boundary,
        bool preserve_custom_pron_markers,
        char **cleaned_text) {
    PV_ASSERT(text);

    bool return_cleaned_text = false;
    if (cleaned_text) {
        return_cleaned_text = true;
        *cleaned_text = NULL;
    }

    size_t length = strlen(text);
    if (length == 0) {
        return PV_STATUS_SUCCESS;
    }

    char *cleaned_text_internal = calloc(strlen(text) + 1, sizeof(char));
    if (!cleaned_text_internal) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("cleaned_text_internal"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    char character[PV_NORMALIZER_MAX_NUM_BYTES_PER_CHARACTER] = {0};
    char previous_character[PV_NORMALIZER_MAX_NUM_BYTES_PER_CHARACTER] = {0};

    bool is_in_custom_pron = false;
    bool is_in_phoneme = false;

    size_t i_orig = 0;
    size_t i_cleaned = 0;

    size_t word_length = 0;
    size_t phoneme_length = 0;
    size_t custom_pron_start_index = 0;

    pv_status_t status;

    while (i_orig <= length) {
        int32_t num_bytes_character = 0;

        if (!is_in_phoneme) {
            status = pv_language_num_bytes_character((unsigned char) text[i_orig], &num_bytes_character);
            if (status != PV_STATUS_SUCCESS) {
                if (!return_cleaned_text) {
                    PV_ERROR_REPORT(
                            &pv_error_msg_invalid_argument,
                            PV_ERROR_ARGS_PUBLIC("text"),
                            PV_ERROR_ARGS_PRIVATE_EMPTY());
                    free(cleaned_text_internal);
                    return PV_STATUS_INVALID_ARGUMENT;
                }
                i_orig += 1;
                continue;
            }

            if (is_in_custom_pron) {
                word_length += num_bytes_character;
            }

            for (int32_t j = 0; j < num_bytes_character; j++) {
                character[j] = text[i_orig + j];
            }
            character[num_bytes_character] = '\0';
        } else {
            num_bytes_character = 1;
            character[0] = text[i_orig];
            character[1] = '\0';

            if (custom_pron_start_index == 0) {
                custom_pron_start_index = i_orig;
            }
        }

        if (num_bytes_character == 1) {
            char c = text[i_orig];

            bool is_space = (c == ' ');
            bool is_eos = (c == '\0') || (i_orig == length);

            if (is_in_phoneme && (is_space || is_eos || c == PV_NORMALIZER_CUSTOM_PRON_CLOSING_MARKER)) {
                if (phoneme_length > 0) {
                    char *ph = malloc((phoneme_length + 1) * sizeof(char));
                    if (!ph) {
                        PV_ERROR_REPORT(
                                &pv_error_msg_alloc,
                                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                                PV_ERROR_ARGS_PRIVATE("token"));
                        free(cleaned_text_internal);
                        return PV_STATUS_OUT_OF_MEMORY;
                    }

                    memcpy(ph, text + custom_pron_start_index, phoneme_length);
                    ph[phoneme_length] = '\0';

                    int32_t phoneme_index;
                    status = pv_language_info_phoneme_index_from_string(
                            language_info,
                            ph,
                            &phoneme_index);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT(
                                &pv_error_msg_orca_invalid_custom_pronunciation_phoneme,
                                PV_ERROR_ARGS_PUBLIC(ph),
                                PV_ERROR_ARGS_PRIVATE_EMPTY());
                        free(ph);
                        free(cleaned_text_internal);
                        return status;
                    }
                    strncat(cleaned_text_internal, ph, phoneme_length);
                    free(ph);
                    i_cleaned += phoneme_length;
                } else {
                    PV_ERROR_REPORT(
                            &pv_error_msg_orca_invalid_custom_pronunciation_format,
                            PV_ERROR_ARGS_PUBLIC_EMPTY(),
                            PV_ERROR_ARGS_PRIVATE_EMPTY());
                    free(cleaned_text_internal);
                    return PV_STATUS_INVALID_ARGUMENT;
                }
                phoneme_length = 0;
                custom_pron_start_index = 0;
            }

            if (is_space) {
                bool previous_was_space = ((i_cleaned > 0) && (cleaned_text_internal[i_cleaned - 1] == ' '));
                bool add_space = !previous_was_space && ((i_cleaned != 0) || preserve_word_boundary);
                if (add_space) {
                    cleaned_text_internal[i_cleaned] = c;
                    i_cleaned++;
                }
                i_orig++;
                continue;
            }

            if (is_eos) {
                cleaned_text_internal[i_cleaned] = c;
                i_cleaned++;
                i_orig++;
                continue;
            }

            if (c == PV_NORMALIZER_CUSTOM_PRON_OPENING_MARKER) {
                is_in_custom_pron = true;
            } else if (c == PV_NORMALIZER_CUSTOM_PRON_CLOSING_MARKER) {
                if (!is_in_custom_pron) {
                    PV_ERROR_REPORT(
                            &pv_error_msg_orca_invalid_custom_pronunciation_format,
                            PV_ERROR_ARGS_PUBLIC_EMPTY(),
                            PV_ERROR_ARGS_PRIVATE_EMPTY());
                    free(cleaned_text_internal);
                    return PV_STATUS_INVALID_ARGUMENT;
                }
                word_length = 0;
                is_in_custom_pron = false;
                is_in_phoneme = false;
            } else if (c == PV_NORMALIZER_CUSTOM_PRON_SEPARATOR) {
                if (!is_in_custom_pron && !return_cleaned_text) {
                    PV_ERROR_REPORT(
                            &pv_error_msg_orca_invalid_custom_pronunciation_format,
                            PV_ERROR_ARGS_PUBLIC_EMPTY(),
                            PV_ERROR_ARGS_PRIVATE_EMPTY());
                    free(cleaned_text_internal);
                    return PV_STATUS_INVALID_ARGUMENT;
                } else if (is_in_custom_pron && !return_cleaned_text && word_length == 0) {
                    PV_ERROR_REPORT(
                            &pv_error_msg_orca_invalid_custom_pronunciation_format,
                            PV_ERROR_ARGS_PUBLIC_EMPTY(),
                            PV_ERROR_ARGS_PRIVATE_EMPTY());
                    free(cleaned_text_internal);
                    return PV_STATUS_INVALID_ARGUMENT;
                }
                if (is_in_custom_pron) {
                    is_in_phoneme = true;
                }
            }

            if ((c == PV_NORMALIZER_CUSTOM_PRON_OPENING_MARKER) ||
                (c == PV_NORMALIZER_CUSTOM_PRON_CLOSING_MARKER) ||
                (c == PV_NORMALIZER_CUSTOM_PRON_SEPARATOR)) {
                if (preserve_custom_pron_markers) {
                    cleaned_text_internal[i_cleaned] = c;
                    i_cleaned++;
                }
                i_orig++;
                continue;
            }

            if (is_in_custom_pron && is_in_phoneme) {
                phoneme_length++;
                i_orig++;
                continue;
            }
        }

        bool is_normalizable_character = false;
        status = pv_normalizer_util_is_normalizable_character(language, character, &is_normalizable_character);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_util_is_normalizable_character,
                    pv_status_to_string(status));
            free(cleaned_text_internal);
            return status;
        }

        if (!is_normalizable_character) {
            if (!return_cleaned_text || is_in_custom_pron) {
                size_t error_message_buffer_length = strlen(character) + 1;
                char *error_message_buffer = calloc(error_message_buffer_length, sizeof(char));
                if (!error_message_buffer) {
                    PV_ERROR_REPORT(
                            &pv_error_msg_alloc,
                            PV_ERROR_ARGS_PUBLIC_EMPTY(),
                            PV_ERROR_ARGS_PRIVATE("error_message_buffer"));
                    free(cleaned_text_internal);
                    return PV_STATUS_OUT_OF_MEMORY;
                }
                memcpy(error_message_buffer, character, error_message_buffer_length);

                PV_ERROR_REPORT(
                        &pv_error_msg_normalizer_invalid_character,
                        PV_ERROR_ARGS_PUBLIC(error_message_buffer),
                        PV_ERROR_ARGS_PRIVATE_EMPTY());
                free(cleaned_text_internal);
                free(error_message_buffer);
                return PV_STATUS_INVALID_ARGUMENT;
            }
            bool previous_was_space = ((i_cleaned > 0) && (cleaned_text_internal[i_cleaned - 1] == ' '));
            bool add_space = !previous_was_space && ((i_cleaned != 0) || preserve_word_boundary);
            if (add_space) {
                strcat(cleaned_text_internal, " ");
                i_cleaned += 1;
            }
        } else {
            strncat(cleaned_text_internal, character, num_bytes_character);
            i_cleaned += num_bytes_character;
        }

        memcpy(previous_character, character, PV_NORMALIZER_MAX_NUM_BYTES_PER_CHARACTER);
        i_orig += num_bytes_character;
    }

    if (is_in_custom_pron) {
        PV_ERROR_REPORT(
                &pv_error_msg_orca_invalid_custom_pronunciation_format,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        free(cleaned_text_internal);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    if (return_cleaned_text) {
        *cleaned_text = cleaned_text_internal;
    } else {
        free(cleaned_text_internal);
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_normalizer_util_upper_inplace)(char *word) {
    PV_ASSERT(word);

    const size_t length = strlen(word);

    size_t index = 0;
    while (index < length) {
        int32_t num_bytes = 0;
        unsigned char one_byte_char = (unsigned char) word[index];
        pv_status_t status = pv_language_num_bytes_character(one_byte_char, &num_bytes);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_language_num_bytes_character,
                    pv_status_to_string(status));
            return status;
        }
        char multi_byte_char[PV_NORMALIZER_MAX_NUM_BYTES_PER_CHARACTER] = {0};
        for (int32_t i = 0; i < num_bytes; i++) {
            multi_byte_char[i] = word[index + i];
        }
        multi_byte_char[num_bytes] = '\0';

        int32_t num_bytes_upper = 0;
        char multi_byte_char_upper[PV_NORMALIZER_MAX_NUM_BYTES_PER_CHARACTER] = {0};
        status = pv_language_utf8_to_upper(
                (unsigned char *) multi_byte_char,
                (unsigned char *) multi_byte_char_upper,
                &num_bytes_upper);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_language_utf8_to_upper,
                    pv_status_to_string(status));
            return status;
        } else if (num_bytes != num_bytes_upper) {
            return PV_STATUS_INVALID_ARGUMENT;
        }

        for (int32_t i = 0; i < num_bytes_upper; i++) {
            word[index + i] = (char) multi_byte_char_upper[i];
        }
        index += num_bytes_upper;
    }
    word[length] = '\0';

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_normalizer_util_upper)(const char *word, char **upper) {
    PV_ASSERT(word);
    PV_ASSERT(upper);

    *upper = NULL;

    char *upper_internal = calloc(strlen(word) + 1, sizeof(char));
    if (!upper_internal) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("upper_internal"));
        return PV_STATUS_OUT_OF_MEMORY;
    }
    strcpy(upper_internal, word);

    pv_status_t status = pv_normalizer_util_upper_inplace(upper_internal);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_util_upper_inplace,
                pv_status_to_string(status));
        free(upper_internal);
        return status;
    }

    *upper = upper_internal;

    return PV_STATUS_SUCCESS;
}

bool PV_MOCKABLE(pv_normalizer_util_string_number_greater_than_int)(const char *string_number, int64_t int_number) {
    PV_ASSERT(string_number);
    PV_ASSERT(int_number >= 0);

    int32_t string_number_num_digits = (int32_t) strlen(string_number);
    int32_t int_number_num_digits = (int32_t) snprintf(NULL, 0, "%" PRId64, int_number);

    if (string_number_num_digits > int_number_num_digits) {
        return true;
    } else if (string_number_num_digits == int_number_num_digits) {
        char *int_number_as_string = calloc(int_number_num_digits + 1, sizeof(char));
        if (!int_number_as_string) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("int_number_as_string"));
            return PV_STATUS_OUT_OF_MEMORY;
        }
        snprintf(
                int_number_as_string,
                int_number_num_digits + 1,
                "%" PRId64, int_number);

        for (int32_t i = 0; i < int_number_num_digits; i++) {
            if (string_number[i] > int_number_as_string[i]) {
                free(int_number_as_string);
                return true;
            } else if (string_number[i] < int_number_as_string[i]) {
                free(int_number_as_string);
                return false;
            }
        }
        free(int_number_as_string);
        return false;
    }
    return false;
}

pv_status_t PV_MOCKABLE(pv_normalizer_util_trie_node_init)(
        int32_t num_characters,
        pv_normalizer_util_trie_node_t **node) {
    PV_ASSERT(num_characters > 0);
    PV_ASSERT(node);

    *node = NULL;

    pv_normalizer_util_trie_node_t *o = calloc(1, sizeof(pv_normalizer_util_trie_node_t));
    if (!o) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->children = calloc(num_characters, sizeof(pv_normalizer_util_trie_node_t));
    if (!o->children) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o->children"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    for (int32_t i = 0; i < num_characters; i++) {
        o->children[i] = NULL;
    }

    o->num_children = num_characters;
    o->index = -1;

    *node = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_normalizer_util_trie_node_delete)(pv_normalizer_util_trie_node_t *object) {
    if (object) {
        for (int32_t i = 0; i < object->num_children; i++) {
            if (object->children[i]) {
                pv_normalizer_util_trie_node_delete(object->children[i]);
            }
        }
        free(object->children);
        free(object);
    }
}

void PV_MOCKABLE(pv_normalizer_util_trie_delete)(pv_normalizer_util_trie_t *object) {
    if (object) {
        pv_normalizer_util_trie_node_delete(object->root);
        free(object);
    }
}

pv_status_t PV_MOCKABLE(pv_normalizer_util_trie_insert)(
        const pv_normalizer_util_trie_t *object,
        const char *string,
        int32_t index) {
    PV_ASSERT(object);
    PV_ASSERT(string);
    PV_ASSERT(index >= 0);

    int32_t length = (int32_t) strlen(string);

    char character[PV_NORMALIZER_MAX_NUM_BYTES_PER_CHARACTER] = {0};

    pv_normalizer_util_trie_node_t *current = object->root;
    int32_t i = 0;
    while (i < length) {
        int32_t num_bytes_character = 0;
        pv_status_t status = pv_normalizer_util_get_next_character(string, i, character, &num_bytes_character);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_util_get_next_character,
                    pv_status_to_string(status));
            return status;
        }

        // Want everything to be upper case in TRIE so to avoid duplicates due to upper and lower cases.
        char *upper_character = NULL;
        status = pv_normalizer_util_upper(character, &upper_character);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_util_upper,
                    pv_status_to_string(status));
            free(upper_character);
            return status;
        }

        int32_t c = -1;
        status = pv_normalizer_util_character_index(upper_character, object->num_characters, object->characters, &c);
        free(upper_character);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_util_character_index,
                    pv_status_to_string(status));
            return status;
        }

        if (current->children[c] == NULL) {
            status = pv_normalizer_util_trie_node_init(object->num_characters, &(current->children[c]));
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_util_trie_node_init,
                        pv_status_to_string(status));
                return status;
            }
        }
        current = current->children[c];

        i += num_bytes_character;
    }

    current->index = index;

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_normalizer_util_trie_create)(
        int32_t num_characters,
        const char *const *characters,
        int32_t num_strings,
        const char **strings,
        pv_normalizer_util_trie_t **trie) {
    PV_ASSERT(num_characters > 0);
    PV_ASSERT(characters);
    PV_ASSERT(num_strings > 0);
    PV_ASSERT(strings);
    PV_ASSERT(trie);

    *trie = NULL;

    pv_normalizer_util_trie_t *o = calloc(1, sizeof(pv_normalizer_util_trie_t));
    if (!o) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("trie_internal"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->num_characters = num_characters;
    o->characters = characters;

    pv_normalizer_util_trie_node_t *root_internal = NULL;
    pv_status_t status = pv_normalizer_util_trie_node_init(num_characters, &root_internal);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_util_trie_node_init,
                pv_status_to_string(status));
        free(o);
        return status;
    }

    o->root = root_internal;

    for (int32_t i = 0; i < num_strings; i++) {
        status = pv_normalizer_util_trie_insert(o, strings[i], i);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_util_trie_insert,
                    pv_status_to_string(status));
            pv_normalizer_util_trie_delete(o);
            return status;
        }
    }

    *trie = o;

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_normalizer_util_trie_search)(
        const pv_normalizer_util_trie_t *object,
        const char *string,
        int32_t *index) {
    PV_ASSERT(object);
    PV_ASSERT(string);
    PV_ASSERT(index);

    *index = -1;

    int32_t length = (int32_t) strlen(string);

    char character[PV_NORMALIZER_MAX_NUM_BYTES_PER_CHARACTER] = {0};

    pv_normalizer_util_trie_node_t *current = object->root;
    int32_t i = 0;
    while (i < length) {
        int32_t num_bytes_character = 0;
        pv_status_t status = pv_normalizer_util_get_next_character(string, i, character, &num_bytes_character);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_util_get_next_character,
                    pv_status_to_string(status));
            return status;
        }

        char *upper_character = NULL;
        status = pv_normalizer_util_upper(character, &upper_character);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_util_upper,
                    pv_status_to_string(status));
            return status;
        }

        int32_t c = -1;
        status = pv_normalizer_util_character_index(upper_character, object->num_characters, object->characters, &c);
        if ((status == PV_STATUS_INVALID_ARGUMENT) || (current->children[c] == NULL)) {
            *index = -1;
            free(upper_character);
            return PV_STATUS_SUCCESS; // Successful search which did not find the target.
        }
        current = current->children[c];

        free(upper_character);

        i += num_bytes_character;
    }

    *index = current->index;

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_normalizer_util_get_next_character)(
        const char *string,
        int32_t index,
        char *character,
        int32_t *num_bytes_character) {
    PV_ASSERT(string);
    PV_ASSERT(index >= 0);
    PV_ASSERT(character);
    PV_ASSERT(num_bytes_character);

    *num_bytes_character = 0;

    int32_t num_bytes_character_internal = 0;

    pv_status_t status = pv_language_num_bytes_character((unsigned char) string[index], &num_bytes_character_internal);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("string[index]"));
        return status;
    }

    for (int32_t j = 0; j < num_bytes_character_internal; j++) {
        character[j] = string[index + j];
    }

    character[num_bytes_character_internal] = '\0';

    *num_bytes_character = num_bytes_character_internal;

    return PV_STATUS_SUCCESS;
}

bool PV_MOCKABLE(pv_normalizer_util_only_contains_digits)(const char *string) {
    PV_ASSERT(string);

    int32_t length = (int32_t) strlen(string);

    bool found_non_digit = false;
    int32_t i = 0;
    while (i < length) {
        if (!(isdigit(string[i]))) {
            found_non_digit = true;
            break;
        }
        i++;
    }

    return !found_non_digit;
}

/*
    Only checks for the first character (which could be non-ascii multibyte!).
*/
pv_status_t PV_MOCKABLE(pv_normalizer_util_is_capitalized_word)(
        const pv_language_info_t *language_info,
        const char *string,
        bool *is_capitalized_word) {
    PV_ASSERT(language_info);
    PV_ASSERT(string);
    PV_ASSERT(is_capitalized_word);

    *is_capitalized_word = false;

    if (strlen(string) == 0) {
        *is_capitalized_word = false;
        return PV_STATUS_SUCCESS;
    }

    bool is_word = false;
    pv_status_t status = pv_normalizer_util_is_word(
            language_info,
            string,
            &is_word);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_util_is_word,
                pv_status_to_string(status));
        return status;
    }
    if (!is_word) {
        *is_capitalized_word = false;
        return PV_STATUS_SUCCESS;
    }

    int32_t num_bytes_character = 0;
    status = pv_language_num_bytes_character((unsigned char) string[0], &num_bytes_character);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_language_num_bytes_character,
                pv_status_to_string(status));
        return status;
    }

    char first_character[PV_NORMALIZER_MAX_NUM_BYTES_PER_CHARACTER] = {0};
    char upper[PV_NORMALIZER_MAX_NUM_BYTES_PER_CHARACTER] = {0};

    memcpy(first_character, string, num_bytes_character * sizeof(char));
    first_character[num_bytes_character] = '\0';

    // First character needs to be alphabetic, not just a word character like apostrophe or hyphen.
    pv_normalizer_language_t language = 0;
    status = pv_normalizer_util_infer_language_from_language_info(language_info, &language);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_util_infer_language_from_language_info,
                pv_status_to_string(status));
        return status;
    }
    bool is_first_character_alphabetic = false;
    status = pv_normalizer_util_is_alphabet_character(language, first_character, &is_first_character_alphabetic);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_util_is_alphabet_character,
                pv_status_to_string(status));
        return status;
    }
    if (!is_first_character_alphabetic) {
        *is_capitalized_word = false;
        return PV_STATUS_SUCCESS;
    }

    int32_t num_bytes_character_upper = 0;
    status = pv_language_utf8_to_upper((unsigned char *) first_character, (unsigned char *) upper, &num_bytes_character_upper);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_language_utf8_to_upper,
                pv_status_to_string(status));
        return status;
    } else if (num_bytes_character != num_bytes_character_upper) {
        return PV_STATUS_INVALID_ARGUMENT;
    }

    *is_capitalized_word = (strcmp(first_character, upper) == 0) ? true : false;

    return PV_STATUS_SUCCESS;
}

/*
    Used e.g. in Orca phonemizer to determine if a word is a spell-out.
    A word is a spell-out if and only if it is all upper case and it
    contains only alphabetic letters and it contain strictly more than
    1 characters.
*/
pv_status_t PV_MOCKABLE(pv_normalizer_util_is_spellout)(
        const pv_language_info_t *language_info,
        const char *string,
        bool *is_spellout) {
    PV_ASSERT(language_info);
    PV_ASSERT(string);
    PV_ASSERT(is_spellout);

    *is_spellout = false;

    int32_t length = (int32_t) strlen(string);
    if (strlen(string) == 0) {
        *is_spellout = false;
        return PV_STATUS_SUCCESS;
    }

    pv_normalizer_language_t language = 0;
    pv_status_t status = pv_normalizer_util_infer_language_from_language_info(language_info, &language);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_util_infer_language_from_language_info,
                pv_status_to_string(status));
        return status;
    }

    if ((language == PV_NORMALIZER_LANGUAGE_KO) || (language == PV_NORMALIZER_LANGUAGE_JA)) {
        *is_spellout = false;
        return PV_STATUS_SUCCESS;
    }

    int32_t num_bytes_character = 0;
    status = pv_language_num_bytes_character((unsigned char) string[0], &num_bytes_character);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_language_num_bytes_character,
                pv_status_to_string(status));
        return status;
    }
    if (num_bytes_character == length) {
        *is_spellout = false;
        return PV_STATUS_SUCCESS;
    }

    bool is_alphabetic = false;
    status = pv_normalizer_util_is_alphabetic(
            language_info,
            string,
            &is_alphabetic);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_util_is_alphabetic,
                pv_status_to_string(status));
        return status;
    }
    if (!is_alphabetic) {
        *is_spellout = false;
        return PV_STATUS_SUCCESS;
    }

    char *upper = (char *) calloc(length + 1, sizeof(char));
    if (!upper) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("upper"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    status = pv_language_utf8_str_to_upper((unsigned char *) string, (unsigned char *) upper);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_language_utf8_to_upper,
                pv_status_to_string(status));
        free(upper);
        return status;
    }

    *is_spellout = (strcmp(string, upper) == 0) ? true : false;
    free(upper);

    return PV_STATUS_SUCCESS;
}

bool PV_MOCKABLE(pv_normalizer_util_is_in_use_cases)(
        int32_t num_use_cases,
        const int32_t *all_use_cases,
        int32_t target_use_case) {
    PV_ASSERT(num_use_cases > 0);
    PV_ASSERT(all_use_cases);
    PV_ASSERT(target_use_case);

    for (int32_t i = 0; i < num_use_cases; i++) {
        if (all_use_cases[i] == target_use_case) {
            return true;
        }
    }

    return false;
}

pv_status_t PV_MOCKABLE(pv_normalizer_util_infer_language_from_language_info)(
        const pv_language_info_t *language_info,
        pv_normalizer_language_t *language) {
    PV_ASSERT(language_info);
    PV_ASSERT(language);

    const char *language_code = pv_language_info_language_code(language_info);

    int32_t type_internal = 0;
    if (strcmp(language_code, "en") == 0) {
        type_internal = PV_NORMALIZER_LANGUAGE_EN;
    } else if (strcmp(language_code, "de") == 0) {
        type_internal = PV_NORMALIZER_LANGUAGE_DE;
    } else if (strcmp(language_code, "fr") == 0) {
        type_internal = PV_NORMALIZER_LANGUAGE_FR;
    } else if (strcmp(language_code, "es") == 0) {
        type_internal = PV_NORMALIZER_LANGUAGE_ES;
    } else if (strcmp(language_code, "it") == 0) {
        type_internal = PV_NORMALIZER_LANGUAGE_IT;
    } else if (strcmp(language_code, "pt") == 0) {
        type_internal = PV_NORMALIZER_LANGUAGE_PT;
    } else if (strcmp(language_code, "ko") == 0) {
        type_internal = PV_NORMALIZER_LANGUAGE_KO;
    } else if (strcmp(language_code, "ja") == 0) {
        type_internal = PV_NORMALIZER_LANGUAGE_JA;
    } else {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("language_info"));
        return PV_STATUS_INVALID_ARGUMENT;
    }

    *language = type_internal;

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_normalizer_get_use_cases_from_language)(
        pv_normalizer_language_t language,
        int32_t *num_use_cases,
        const int32_t **use_cases) {
    PV_ASSERT(language >= 0);
    PV_ASSERT(num_use_cases);
    PV_ASSERT(use_cases);

    *num_use_cases = 0;
    *use_cases = NULL;

    switch (language) {
        case PV_NORMALIZER_LANGUAGE_EN: {
            *num_use_cases = PV_NORMALIZER_NUM_CASES_EN;
            *use_cases = (const int32_t *) ALL_USE_CASES_EN;
        } break;
        case PV_NORMALIZER_LANGUAGE_DE: {
            *num_use_cases = PV_NORMALIZER_NUM_CASES_DE;
            *use_cases = (const int32_t *) ALL_USE_CASES_DE;
        } break;
        case PV_NORMALIZER_LANGUAGE_FR: {
            *num_use_cases = PV_NORMALIZER_NUM_CASES_FR;
            *use_cases = (const int32_t *) ALL_USE_CASES_FR;
        } break;
        case PV_NORMALIZER_LANGUAGE_ES: {
            *num_use_cases = PV_NORMALIZER_NUM_CASES_ES;
            *use_cases = (const int32_t *) ALL_USE_CASES_ES;
        } break;
        case PV_NORMALIZER_LANGUAGE_IT: {
            *num_use_cases = PV_NORMALIZER_NUM_CASES_IT;
            *use_cases = (const int32_t *) ALL_USE_CASES_IT;
        } break;
        case PV_NORMALIZER_LANGUAGE_PT: {
            *num_use_cases = PV_NORMALIZER_NUM_CASES_PT;
            *use_cases = (const int32_t *) ALL_USE_CASES_PT;
        } break;
        case PV_NORMALIZER_LANGUAGE_JA: {
            *num_use_cases = PV_NORMALIZER_NUM_CASES_JA;
            *use_cases = (const int32_t *) ALL_USE_CASES_JA;
        } break;
        case PV_NORMALIZER_LANGUAGE_KO: {
            *num_use_cases = PV_NORMALIZER_NUM_CASES_KO;
            *use_cases = (const int32_t *) ALL_USE_CASES_KO;
        } break;
        default:
            PV_ERROR_REPORT(
                    &pv_error_msg_invalid_argument_internal,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("language"));
            return PV_STATUS_INVALID_ARGUMENT;
    }

    return PV_STATUS_SUCCESS;
}

bool PV_MOCKABLE(pv_normalizer_is_valid_eos)(
        pv_normalizer_language_t language,
        const char *character) {
    PV_ASSERT(character);

    int32_t num_eos_punctuations = 0;
    const char **eos_punctuations = NULL;

    switch (language) {
        case PV_NORMALIZER_LANGUAGE_EN: {
            num_eos_punctuations = PV_ARRAY_LEN(PV_ORCA_STREAM_EN_EOS_PUNCTUATIONS);
            eos_punctuations = PV_ORCA_STREAM_EN_EOS_PUNCTUATIONS;
        } break;
        case PV_NORMALIZER_LANGUAGE_DE: {
            num_eos_punctuations = PV_ARRAY_LEN(PV_ORCA_STREAM_DE_EOS_PUNCTUATIONS);
            eos_punctuations = PV_ORCA_STREAM_DE_EOS_PUNCTUATIONS;
        } break;
        case PV_NORMALIZER_LANGUAGE_FR: {
            num_eos_punctuations = PV_ARRAY_LEN(PV_ORCA_STREAM_FR_EOS_PUNCTUATIONS);
            eos_punctuations = PV_ORCA_STREAM_FR_EOS_PUNCTUATIONS;
        } break;
        case PV_NORMALIZER_LANGUAGE_ES: {
            num_eos_punctuations = PV_ARRAY_LEN(PV_ORCA_STREAM_ES_EOS_PUNCTUATIONS);
            eos_punctuations = PV_ORCA_STREAM_ES_EOS_PUNCTUATIONS;
        } break;
        case PV_NORMALIZER_LANGUAGE_IT: {
            num_eos_punctuations = PV_ARRAY_LEN(PV_ORCA_STREAM_IT_EOS_PUNCTUATIONS);
            eos_punctuations = PV_ORCA_STREAM_IT_EOS_PUNCTUATIONS;
        } break;
        case PV_NORMALIZER_LANGUAGE_PT: {
            num_eos_punctuations = PV_ARRAY_LEN(PV_ORCA_STREAM_PT_EOS_PUNCTUATIONS);
            eos_punctuations = PV_ORCA_STREAM_PT_EOS_PUNCTUATIONS;
        } break;
        case PV_NORMALIZER_LANGUAGE_KO: {
            num_eos_punctuations = PV_ARRAY_LEN(PV_ORCA_STREAM_KO_EOS_PUNCTUATIONS);
            eos_punctuations = PV_ORCA_STREAM_KO_EOS_PUNCTUATIONS;
        } break;
        case PV_NORMALIZER_LANGUAGE_JA: {
            num_eos_punctuations = PV_ARRAY_LEN(PV_ORCA_STREAM_JA_EOS_PUNCTUATIONS);
            eos_punctuations = PV_ORCA_STREAM_JA_EOS_PUNCTUATIONS;
        } break;
        default:
            PV_ERROR_REPORT(
                    &pv_error_msg_invalid_argument_internal,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("language"));
            return false;
    }

    for (int32_t i = 0; i < num_eos_punctuations; i++) {
        if (strcmp(character, eos_punctuations[i]) == 0) {
            return true;
        }
    }

    return false;
}

/*
    Checks for all characters in the string only contain alphabet characters (which could be non-ascii multibyte!).
*/
pv_status_t PV_MOCKABLE(pv_normalizer_util_is_alphabetic)(
        const pv_language_info_t *language_info,
        const char *string,
        bool *is_alphabetic) {
    PV_ASSERT(language_info);
    PV_ASSERT(string);
    PV_ASSERT(is_alphabetic);

    *is_alphabetic = false;

    int32_t length = (int32_t) strlen(string);
    if (strlen(string) == 0) {
        *is_alphabetic = false;
        return PV_STATUS_SUCCESS;
    }

    char character[PV_NORMALIZER_MAX_NUM_BYTES_PER_CHARACTER] = {0};

    bool found_non_alphabetic_character = false;
    int32_t i = 0;
    while (i < length) {
        int32_t num_bytes_character = 0;
        pv_status_t status = pv_language_num_bytes_character((unsigned char) string[i], &num_bytes_character);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_language_num_bytes_character,
                    pv_status_to_string(status));
            return status;
        }

        for (int32_t j = 0; j < num_bytes_character; j++) {
            character[j] = string[i + j];
        }
        character[num_bytes_character] = '\0';

        pv_normalizer_language_t language = 0;
        status = pv_normalizer_util_infer_language_from_language_info(language_info, &language);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_util_infer_language_from_language_info,
                    pv_status_to_string(status));
            return status;
        }

        bool is_alphabet_character = false;
        status = pv_normalizer_util_is_alphabet_character(language, character, &is_alphabet_character);
        found_non_alphabetic_character = !is_alphabet_character;
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_util_is_alphabet_character,
                    pv_status_to_string(status));
            return status;
        }
        if (found_non_alphabetic_character) {
            break;
        }

        i += num_bytes_character;
    }

    if (found_non_alphabetic_character) {
        *is_alphabetic = false;
    } else {
        *is_alphabetic = true;
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_normalizer_util_is_alphabet_character)(
        pv_normalizer_language_t language,
        const char *character,
        bool *is_alphabet_character) {
    PV_ASSERT(language >= 0);
    PV_ASSERT(character);
    PV_ASSERT(is_alphabet_character);

    *is_alphabet_character = false;

    switch (language) {
        case PV_NORMALIZER_LANGUAGE_EN: {
            *is_alphabet_character = pv_normalizer_util_en_is_alphabet_character(character);
        } break;
        case PV_NORMALIZER_LANGUAGE_DE: {
            *is_alphabet_character = pv_normalizer_util_de_is_alphabet_character(character);
        } break;
        case PV_NORMALIZER_LANGUAGE_FR: {
            *is_alphabet_character = pv_normalizer_util_fr_is_alphabet_character(character);
        } break;
        case PV_NORMALIZER_LANGUAGE_ES: {
            *is_alphabet_character = pv_normalizer_util_es_is_alphabet_character(character);
        } break;
        case PV_NORMALIZER_LANGUAGE_IT: {
            *is_alphabet_character = pv_normalizer_util_it_is_alphabet_character(character);
        } break;
        case PV_NORMALIZER_LANGUAGE_PT: {
            *is_alphabet_character = pv_normalizer_util_pt_is_alphabet_character(character);
        } break;
        case PV_NORMALIZER_LANGUAGE_KO: {
            *is_alphabet_character = pv_normalizer_util_ko_is_word_character(character); // (Ted): Assuming Korean word character don't use things like apostrophe or hyphen.
        } break;
        default:
            PV_ERROR_REPORT(
                    &pv_error_msg_invalid_argument_internal,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("language"));
            return PV_STATUS_INVALID_ARGUMENT;
    }
    return PV_STATUS_SUCCESS;
}

/*
    Checks for all characters in the string only contain word characters (which could be non-ascii multibyte and may or may not contain apostrophe and hyphen, depending on the language).
*/
pv_status_t PV_MOCKABLE(pv_normalizer_util_is_word)(
        const pv_language_info_t *language_info,
        const char *string,
        bool *is_word) {
    PV_ASSERT(language_info);
    PV_ASSERT(string);
    PV_ASSERT(is_word);

    *is_word = false;

    int32_t length = (int32_t) strlen(string);
    if (strlen(string) == 0) {
        *is_word = false;
        return PV_STATUS_SUCCESS;
    }

    char character[PV_NORMALIZER_MAX_NUM_BYTES_PER_CHARACTER] = {0};

    bool found_non_word_character = false;
    int32_t i = 0;
    while (i < length) {
        int32_t num_bytes_character = 0;
        pv_status_t status = pv_language_num_bytes_character((unsigned char) string[i], &num_bytes_character);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_language_num_bytes_character,
                    pv_status_to_string(status));
            return status;
        }

        for (int32_t j = 0; j < num_bytes_character; j++) {
            character[j] = string[i + j];
        }
        character[num_bytes_character] = '\0';

        pv_normalizer_language_t language = 0;
        status = pv_normalizer_util_infer_language_from_language_info(language_info, &language);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_util_infer_language_from_language_info,
                    pv_status_to_string(status));
            return status;
        }

        bool is_word_character = false;
        status = pv_normalizer_util_is_word_character(language, character, &is_word_character);
        found_non_word_character = !is_word_character;
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_util_is_word_character,
                    pv_status_to_string(status));
            return status;
        }
        if (found_non_word_character) {
            break;
        }

        i += num_bytes_character;
    }

    if (found_non_word_character) {
        *is_word = false;
    } else {
        *is_word = true;
    }

    return PV_STATUS_SUCCESS;
}

// TODO (Ted): Future might consider refactoring this to use language agnostic tag for word instead, if that even makes sense.
pv_status_t PV_MOCKABLE(pv_normalizer_util_is_word_token)(
        const pv_language_info_t *language_info,
        const pv_normalizer_token_t *token,
        bool *is_word) {
    PV_ASSERT(language_info);
    PV_ASSERT(token);

    pv_normalizer_language_t language = 0;
    pv_status_t status = pv_normalizer_util_infer_language_from_language_info(language_info, &language);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_util_infer_language_from_language_info,
                pv_status_to_string(status));
        return status;
    }

    switch (language) {
        case PV_NORMALIZER_LANGUAGE_EN: {
            *is_word = pv_normalizer_util_en_is_word_token(token);
        } break;
        case PV_NORMALIZER_LANGUAGE_DE: {
            *is_word = pv_normalizer_util_de_is_word_token(token);
        } break;
        case PV_NORMALIZER_LANGUAGE_FR: {
            *is_word = pv_normalizer_util_fr_is_word_token(token);
        } break;
        case PV_NORMALIZER_LANGUAGE_ES: {
            *is_word = pv_normalizer_util_es_is_word_token(token);
        } break;
        case PV_NORMALIZER_LANGUAGE_IT: {
            *is_word = pv_normalizer_util_it_is_word_token(token);
        } break;
        case PV_NORMALIZER_LANGUAGE_PT: {
            *is_word = pv_normalizer_util_pt_is_word_token(token);
        } break;
        case PV_NORMALIZER_LANGUAGE_KO: {
            *is_word = pv_normalizer_util_ko_is_word_token(token);
        } break;
        case PV_NORMALIZER_LANGUAGE_JA: {
            *is_word = pv_normalizer_util_ja_is_word_token(token);
        } break;
        default:
            PV_ERROR_REPORT(
                    &pv_error_msg_invalid_argument_internal,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("language"));
            return PV_STATUS_INVALID_ARGUMENT;
    }

    return PV_STATUS_SUCCESS;
}


pv_status_t pv_normalizer_util_remap_characters_helper(
        const char *text,
        char *remapped_text,
        int32_t *remapped_text_length,
        bool dry_run) {
    PV_ASSERT(text);
    PV_ASSERT(remapped_text || dry_run);
    PV_ASSERT(remapped_text_length || !dry_run);
    if (dry_run) {
        *remapped_text_length = 0;
    }

    int32_t hyphen_length = 0;
    int32_t tilde_length = 0;
    int32_t apostrophe_length = 0;
    int32_t slash_length = 0;
    int32_t small_s_length = 0;

    pv_status_t status = pv_language_num_bytes_character((unsigned char) HYPHEN[0], &hyphen_length);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_language_num_bytes_character,
                pv_status_to_string(status));
        return status;
    }

    status = pv_language_num_bytes_character((unsigned char) TILDE[0], &tilde_length);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_language_num_bytes_character,
                pv_status_to_string(status));
        return status;
    }

    status = pv_language_num_bytes_character((unsigned char) APOSTROPHE[0], &apostrophe_length);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_language_num_bytes_character,
                pv_status_to_string(status));
        return status;
    }

    status = pv_language_num_bytes_character((unsigned char) SLASH[0], &slash_length);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_language_num_bytes_character,
                pv_status_to_string(status));
        return status;
    }

    status = pv_language_num_bytes_character((unsigned char) SMALL_SHARP_S[0], &small_s_length);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_language_num_bytes_character,
                pv_status_to_string(status));
        return status;
    }

    int32_t i = 0;
    int32_t i_remapped_text = 0;
    int32_t text_length = (int32_t) strlen(text);
    char character[PV_NORMALIZER_MAX_NUM_BYTES_PER_CHARACTER] = {0};
    while (i < text_length) {
        int32_t character_num_bytes = 0;
        status = pv_language_num_bytes_character((unsigned char) text[i], &character_num_bytes);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_language_num_bytes_character,
                    pv_status_to_string(status));
            return status;
        }

        memcpy(character, text + i, character_num_bytes * sizeof(char));
        character[character_num_bytes] = '\0';

        bool is_en_dash = (strcmp(EN_DASH, character) == 0);
        bool is_figure_dash = (strcmp(FIGURE_DASH, character) == 0);
        bool is_minus_sign = (strcmp(MINUS_SIGN, character) == 0);
        bool is_wave_dash = (strcmp(WAVE_DASH, character) == 0);
        bool is_full_width_tilde = (strcmp(FULL_WIDTH_TILDE, character) == 0);
        bool is_single_quotation_mark = (strcmp(RIGHT_SINGLE_QUOTATION_MARK, character) == 0) ||
            (strcmp(LEFT_SINGLE_QUOTATION_MARK, character) == 0);
        bool is_slash = (strcmp(FRACTION_SLASH, character) == 0);
        bool is_capital_sharp_s = (strcmp(CAPITAL_SHARP_S, character) == 0);

        if (is_en_dash || is_figure_dash || is_minus_sign || is_wave_dash || is_full_width_tilde ||
            is_single_quotation_mark || is_slash || is_capital_sharp_s) {
            int32_t target_length = 0;
            char target_character[PV_NORMALIZER_MAX_NUM_BYTES_PER_CHARACTER] = {0};
            if (is_single_quotation_mark) {
                target_length = apostrophe_length;
                strcpy(target_character, APOSTROPHE);
            } else if (is_slash) {
                target_length = slash_length;
                strcpy(target_character, SLASH);
            } else if (is_wave_dash || is_full_width_tilde) {
                target_length = tilde_length;
                strcpy(target_character, TILDE);
            } else if (is_capital_sharp_s) {
                target_length = small_s_length;
                strcpy(target_character, SMALL_SHARP_S);
            } else {
                target_length = hyphen_length;
                strcpy(target_character, HYPHEN);
            }

            if (!dry_run) {
                memcpy(remapped_text + i_remapped_text, target_character, target_length * sizeof(char));
            }
            i_remapped_text += target_length;
        } else {
            if (!dry_run) {
                memcpy(remapped_text + i_remapped_text, character, character_num_bytes * sizeof(char));
            }
            i_remapped_text += character_num_bytes;
        }
        i += character_num_bytes;
    }

    if (dry_run) {
        *remapped_text_length = i_remapped_text;
    } else {
        remapped_text[i_remapped_text] = '\0';
    }

    return PV_STATUS_SUCCESS;
}


pv_status_t PV_MOCKABLE(pv_normalizer_util_remap_characters)(
        const char *text,
        char **remapped_text) {
    PV_ASSERT(text);
    PV_ASSERT(remapped_text);
    *remapped_text = NULL;

    char *remapped_text_internal = NULL;
    int32_t remapped_text_internal_length = 0;
    pv_status_t status = pv_normalizer_util_remap_characters_helper(
            text,
            NULL,
            &remapped_text_internal_length,
            true);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_util_remap_characters_helper,
                pv_status_to_string(status));
        return status;
    }

    remapped_text_internal = calloc(remapped_text_internal_length + 1, sizeof(char));
    if (!remapped_text_internal) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("remapped_text_internal"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    status = pv_normalizer_util_remap_characters_helper(
            text,
            remapped_text_internal,
            NULL,
            false);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_util_remap_characters_helper,
                pv_status_to_string(status));
        free(remapped_text_internal);
        return status;
    }

    *remapped_text = remapped_text_internal;
    return PV_STATUS_SUCCESS;
}


// TODO(Tina): Refactor with pv_normalizer_util_ja.c and return a status
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

bool is_variation_of_space(unsigned char *bytes) {
    PV_ASSERT(bytes);

    int32_t unicode = get_unicode_value(bytes);
    if (unicode >= 0x2000 && unicode <= 0x200A) {
        return true;
    }
    return false;
}

pv_status_t pv_normalizer_util_remap_space_helper(
        const char *text,
        char *remapped_text,
        int32_t *remapped_text_length,
        bool dry_run) {
    PV_ASSERT(text);
    PV_ASSERT(remapped_text || dry_run);
    PV_ASSERT(remapped_text_length || !dry_run);
    if (dry_run) {
        *remapped_text_length = 0;
    }

    int32_t i = 0;
    int32_t i_remapped_text = 0;
    int32_t text_length = (int32_t) strlen(text);
    char character[PV_NORMALIZER_MAX_NUM_BYTES_PER_CHARACTER] = {0};
    while (i < text_length) {
        int32_t character_num_bytes = 0;
        pv_status_t status = pv_language_num_bytes_character((unsigned char) text[i], &character_num_bytes);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_language_num_bytes_character,
                    pv_status_to_string(status));
            return status;
        }

        memcpy(character, text + i, character_num_bytes * sizeof(char));
        character[character_num_bytes] = '\0';

        if (is_variation_of_space((unsigned char *) &text[i])) {
            int32_t target_length = 1;
            char *target_character = " ";
            if (!dry_run) {
                memcpy(remapped_text + i_remapped_text, target_character, target_length * sizeof(char));
            }
            i_remapped_text += target_length;
        } else {
            if (!dry_run) {
                memcpy(remapped_text + i_remapped_text, character, character_num_bytes * sizeof(char));
            }
            i_remapped_text += character_num_bytes;
        }
        i += character_num_bytes;
    }

    if (dry_run) {
        *remapped_text_length = i_remapped_text;
    } else {
        remapped_text[i_remapped_text] = '\0';
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_normalizer_util_remap_space)(
        const char *text,
        char **remapped_text) {
    PV_ASSERT(text);
    PV_ASSERT(remapped_text);
    *remapped_text = NULL;

    char *remapped_text_internal = NULL;
    int32_t remapped_text_internal_length = 0;
    pv_status_t status = pv_normalizer_util_remap_space_helper(
            text,
            NULL,
            &remapped_text_internal_length,
            true);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_util_remap_characters_helper,
                pv_status_to_string(status));
        return status;
    }

    remapped_text_internal = calloc(remapped_text_internal_length + 1, sizeof(char));
    if (!remapped_text_internal) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("remapped_text_internal"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    status = pv_normalizer_util_remap_space_helper(
            text,
            remapped_text_internal,
            NULL,
            false);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_util_remap_characters_helper,
                pv_status_to_string(status));
        free(remapped_text_internal);
        return status;
    }

    *remapped_text = remapped_text_internal;
    return PV_STATUS_SUCCESS;
}


// Assuming this is used only for number range, cardinal, and currency, where we have the assumption that the hyphen will be present in the next token.
// This function should not be used for example for words "hi-hi" because tokenizer will remove the hyphen!
pv_status_t PV_MOCKABLE(pv_normalizer_util_check_token_is_before_character)(
        pv_normalizer_token_t *token,
        const char *target,
        bool *is_before_character) {
    PV_ASSERT(token);
    PV_ASSERT(target);
    PV_ASSERT(is_before_character);

    int32_t target_num_bytes = 0;
    pv_status_t status = pv_language_num_bytes_character((unsigned char) target[0], &target_num_bytes);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_language_num_bytes_character,
                pv_status_to_string(status));
        return status;
    }

    if ((token->next) != NULL &&
        (strlen(token->next->string) > 0)) {
        int32_t first_character_num_bytes = 0;
        pv_status_t status = pv_language_num_bytes_character((unsigned char) token->next->string[0], &first_character_num_bytes);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_language_num_bytes_character,
                    pv_status_to_string(status));
            return status;
        }
        if ((first_character_num_bytes == target_num_bytes) &&
            (strncmp(token->next->string, target, target_num_bytes) == 0)) {
            *is_before_character = true;
        } else {
            *is_before_character = false;
        }
    } else {
        *is_before_character = false;
    }

    return PV_STATUS_SUCCESS;
}
