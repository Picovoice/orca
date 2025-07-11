#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "core/pv_assert.h"
#include "core/pv_error_messages.h"
#include "orca/normalizer/pv_normalizer_token.h"
#include "orca/normalizer/pv_normalizer_use_cases.h"

#include "orca/normalizer/ja/pv_normalizer_data_ja/pv_normalizer_data_ja.h"
#include "orca/normalizer/ja/pv_normalizer_tags_ja.h"
#include "orca/normalizer/ja/pv_normalizer_verbalizer_ja.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_normalizer_mock.h"

#endif

struct pv_normalizer_verbalizer_ja {
    int32_t num_use_cases;
    const pv_normalizer_use_cases_ja_t *use_cases;
};

static void pv_normalizer_verbalizer_synchronize_language_agnostic_tags(pv_normalizer_token_list_t *token_list);

static pv_status_t pv_normalizer_verbalizer_verbalize_word(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_punctuation(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_cardinal(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_negative_cardinal(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_custom_pronunciation(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_number_range(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_special_character(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_decimal(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_measurement(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_alphanum_spell_out(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_time(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_fraction_slash(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_dot(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_colon(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_top_level_domain(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_currency(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_negative_currency(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_currency_symbol(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_abbreviation(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_digits_sequence(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_dates(pv_normalizer_token_t *token);

static void pv_normalizer_verbalizer_verbalize_names(pv_normalizer_token_t *token);

pv_status_t PV_MOCKABLE(pv_normalizer_verbalizer_ja_init)(
        int32_t num_use_cases,
        const pv_normalizer_use_cases_ja_t *use_cases,
        pv_normalizer_verbalizer_ja_t **object) {
    PV_ASSERT(num_use_cases >= 0);
    PV_ASSERT(use_cases);
    PV_ASSERT(object);

    *object = NULL;

    pv_normalizer_verbalizer_ja_t *o = calloc(1, sizeof(pv_normalizer_verbalizer_ja_t));
    if (!o) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->num_use_cases = num_use_cases;
    o->use_cases = use_cases;

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_normalizer_verbalizer_ja_delete)(pv_normalizer_verbalizer_ja_t *object) {
    if (object) {
        free(object);
    }
}

pv_status_t PV_MOCKABLE(pv_normalizer_verbalizer_ja_verbalize)(
        pv_normalizer_verbalizer_ja_t *object,
        pv_normalizer_token_list_t *token_list,
        int32_t num_tokens_skip) {
    PV_ASSERT(object);
    PV_ASSERT(token_list);
    PV_ASSERT(num_tokens_skip >= 0);

    pv_normalizer_token_t *token = token_list->head;
    int32_t i = 0;
    while (token != NULL) {
        if (i < num_tokens_skip) {
            token = token->next;
            i++;
            continue;
        }

        for (int32_t j = 0; j < object->num_use_cases; j++) {
            if (object->use_cases[j] == PV_NORMALIZER_USE_WORD_NORMALIZER_JA) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_word(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_word,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_PUNCTUATION_NORMALIZER_JA) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_punctuation(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_punctuation,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_CARDINAL_NORMALIZER_JA) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_cardinal(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_cardinal,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_JA) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_negative_cardinal(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_negative_cardinal,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_CUSTOM_PRONUNCIATION_NORMALIZER_JA) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_custom_pronunciation(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_custom_pronunciation,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_NUMBER_RANGE_NORMALIZER_JA) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_number_range(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_number_range,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_SPECIAL_CHARACTER_NORMALIZER_JA) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_special_character(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_special_character,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_DECIMAL_NORMALIZER_JA) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_decimal(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_decimal,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_MEASUREMENT_NORMALIZER_JA) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_measurement(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_measurement,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_ALPHANUM_SPELL_OUT_NORMALIZER_JA) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_alphanum_spell_out(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_alphanum_spell_out,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_TIME_NORMALIZER_JA) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_time(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_time,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_FRACTION_NORMALIZER_JA) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_fraction_slash(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_fraction_slash,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_URL_NORMALIZER_JA) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_dot(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_dot,
                            pv_status_to_string(status));
                    return status;
                }

                status = pv_normalizer_verbalizer_verbalize_colon(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_colon,
                            pv_status_to_string(status));
                    return status;
                }

                status = pv_normalizer_verbalizer_verbalize_top_level_domain(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_top_level_domain,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_CURRENCY_NORMALIZER_JA) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_currency(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_currency,
                            pv_status_to_string(status));
                    return status;
                }

                status = pv_normalizer_verbalizer_verbalize_negative_currency(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_negative_currency,
                            pv_status_to_string(status));
                    return status;
                }

                status = pv_normalizer_verbalizer_verbalize_currency_symbol(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_currency_symbol,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_ABBREVIATION_NORMALIZER_JA) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_abbreviation(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_abbreviation,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_DIGITS_SEQUENCE_NORMALIZER_JA) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_digits_sequence(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_digits_sequence,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_DATE_NORMALIZER_JA) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_dates(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_dates,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_NAME_NORMALIZER_JA) {
                pv_normalizer_verbalizer_verbalize_names(token);
            } else {
                return PV_STATUS_INVALID_ARGUMENT;
            }
        }

        token = token->next;
    }

    pv_normalizer_verbalizer_synchronize_language_agnostic_tags(token_list);

    return PV_STATUS_SUCCESS;
}

void pv_normalizer_verbalizer_synchronize_language_agnostic_tags(pv_normalizer_token_list_t *token_list) {
    PV_ASSERT(token_list);

    pv_normalizer_token_list_synchronize_language_agnostic_tags_common(
            token_list,
            PV_NORMALIZER_TAG_JA_SPACE,
            PV_NORMALIZER_TAG_JA_PUNCTUATION,
            -1,
            PV_NORMALIZER_TAG_JA_CUSTOM_PRONUNCIATION,
            -1);
}

pv_status_t pv_normalizer_verbalizer_verbalize_word(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_JA_WORD) {
        char *token_verbalized = calloc(strlen(token->string) + 1, sizeof(char));
        if (!token_verbalized) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("token_verbalized"));
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(token_verbalized, token->string);
        pv_normalizer_token_set_verbalized(token, token_verbalized);
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_punctuation(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_JA_PUNCTUATION) {
        // TODO(Tina): For all similar logic, throw an error if i out of range
        int32_t i = 0;
        for (i = 0; i < PV_ARRAY_LEN(PV_NORMALIZER_PUNCTUATION_JA); i++) {
            if (strcmp(token->string, PV_NORMALIZER_PUNCTUATION_JA[i]) == 0) {
                break;
            }
        }
        const char *verbalized = PV_NORMALIZER_PUNCTUATION_VERBALIZED_JA[i];
        char *token_verbalized = calloc(strlen(verbalized) + 1, sizeof(char));
        if (!token_verbalized) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("token_verbalized"));
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(token_verbalized, verbalized);
        pv_normalizer_token_set_verbalized(token, token_verbalized);
    }

    return PV_STATUS_SUCCESS;
}

void cardinal_to_string_helper(char *dest, const char *src, int32_t *length, bool get_src_length) {
    PV_ASSERT(get_src_length || (!get_src_length && dest));
    PV_ASSERT(src);
    PV_ASSERT(length);

    if (get_src_length) {
        *length += (int32_t) strlen(src);
    } else {
        strcat(dest, src);
    }
}

pv_status_t pv_normalizer_verbalizer_cardinal_to_string_helper(
        const char *number_string,
        bool to_digit_string,
        int32_t *length,
        char **string) {
    PV_ASSERT(number_string);
    PV_ASSERT((*length) >= 0);
    PV_ASSERT(string);

    *string = NULL;
    char *result = NULL;

    bool get_src_length = true;
    if ((*length) > 0) {
        get_src_length = false;
        result = calloc((*length) + 1, sizeof(char));
        if (!result) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("result"));
            return PV_STATUS_OUT_OF_MEMORY;
        }
    }

    if (to_digit_string ||
        (*number_string == '0') ||
        pv_normalizer_util_string_number_greater_than_int(number_string, MAX_VERBALIZED_CARDINAL_JA)) {
        while (*number_string != '\0') {
            int32_t digit = (int32_t) *number_string - '0';
            if (digit == 0) {
                cardinal_to_string_helper(result, "ゼロ", length, get_src_length);
            } else {
                cardinal_to_string_helper(result, ONE_TO_NINE_JA[(int32_t) digit], length, get_src_length);
            }
            number_string++;
        }
    } else {
        int64_t number = (int64_t) strtoll(number_string, NULL, 10);
        if (number < 0) {
            return PV_STATUS_INVALID_ARGUMENT;
        }

        if (number < 10) {
            cardinal_to_string_helper(result, ONE_TO_NINE_JA[number], length, get_src_length);
            *string = result;

            return PV_STATUS_SUCCESS;
        }

        int64_t current_multiplier = MAX_MULTIPLIER_JA;
        int64_t current_group = 0;
        int32_t current_multiplier_i = 0;
        int64_t i = number;
        while (i > 0) {
            current_group = i / current_multiplier;
            while (current_group == 0) {
                i %= current_multiplier;
                current_multiplier /= 10000;
                current_group = i / current_multiplier;
                current_multiplier_i++;
            }

            int32_t group_divisor = 1000;
            for (int j = 0; j < PV_ARRAY_LEN(SMALL_UNITS_JA); j++) {
                int64_t digit = current_group / group_divisor;
                if (digit != 0) {
                    if (digit > 1) {
                        cardinal_to_string_helper(
                                result,
                                ONE_TO_NINE_JA[digit],
                                length,
                                get_src_length);
                    }
                    cardinal_to_string_helper(
                            result,
                            SMALL_UNITS_JA[j],
                            length,
                            get_src_length);
                }
                current_group %= group_divisor;
                group_divisor /= 10;
            }

            cardinal_to_string_helper(
                    result,
                    ONE_TO_NINE_JA[current_group],
                    length,
                    get_src_length);

            if (current_multiplier_i < PV_ARRAY_LEN(LARGE_UNITS_JA)) {
                cardinal_to_string_helper(
                        result,
                        LARGE_UNITS_JA[current_multiplier_i],
                        length,
                        get_src_length);
                current_multiplier_i++;
            }

            i %= current_multiplier;
            current_multiplier /= 10000;
        }
    }

    if (!get_src_length) {
        result[(*length)] = '\0';
    }

    *string = result;
    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_cardinal_to_string(
        const char *number_string,
        bool to_digit_string,
        char **string) {
    PV_ASSERT(number_string);
    PV_ASSERT(string);

    int32_t length = 0;
    pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string_helper(
            number_string,
            to_digit_string,
            &length,
            string);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_verbalizer_cardinal_to_string_helper,
                pv_status_to_string(status));
        return status;
    }

    status = pv_normalizer_verbalizer_cardinal_to_string_helper(
            number_string,
            to_digit_string,
            &length,
            string);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_verbalizer_cardinal_to_string_helper,
                pv_status_to_string(status));
    }

    return status;
}

pv_status_t pv_normalizer_verbalizer_verbalize_cardinal(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    bool is_ordinal_denominator =
            ((token->previous != NULL) &&
             (token->previous->tag == PV_NORMALIZER_TAG_JA_FRACTION_SLASH) &&
             (token->previous->verbalized == NULL));

    if ((token->tag == PV_NORMALIZER_TAG_JA_CARDINAL) && !is_ordinal_denominator) {
        char *verbalized = NULL;
        pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(token->string, false, &verbalized);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_cardinal_to_string,
                    pv_status_to_string(status));
            return status;
        }

        pv_normalizer_token_set_verbalized(token, verbalized);
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_negative_cardinal(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_JA_NEGATIVE_CARDINAL) {
        char *verbalized = NULL;
        const char *digit_string = token->string + 1;
        pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(digit_string, false, &verbalized);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_cardinal_to_string,
                    pv_status_to_string(status));
            return status;
        }

        const char *negative = "マイナス";

        char *token_verbalized = calloc(strlen(verbalized) + strlen(negative) + 1, sizeof(char));
        if (!token_verbalized) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("token_verbalized"));
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(token_verbalized, negative);
        strcat(token_verbalized, verbalized);
        free(verbalized);
        pv_normalizer_token_set_verbalized(token, token_verbalized);
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_custom_pronunciation(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_JA_CUSTOM_PRONUNCIATION) {
        int32_t len = (int32_t) strlen(token->pronunciation);
        char *token_verbalized = calloc(len + 3, sizeof(char));
        if (!token_verbalized) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("token_verbalized"));
            return PV_STATUS_OUT_OF_MEMORY;
        }
        memcpy(token_verbalized, &PV_NORMALIZER_CUSTOM_PRON_OPENING_MARKER, 1);
        strcat(token_verbalized, token->pronunciation);
        memcpy(&token_verbalized[len + 1], &PV_NORMALIZER_CUSTOM_PRON_CLOSING_MARKER, 1);
        pv_normalizer_token_set_verbalized(token, token_verbalized);
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_number_range(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_JA_NUMBER_RANGE_TO) {
        char *token_verbalized = calloc(strlen(NUMBER_RANGE_INBETWEEN_STRING_JA) + 1, sizeof(char));
        if (!token_verbalized) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("token_verbalized"));
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(token_verbalized, NUMBER_RANGE_INBETWEEN_STRING_JA);
        pv_normalizer_token_set_verbalized(token, token_verbalized);
    }

    return PV_STATUS_SUCCESS;
}


pv_status_t pv_normalizer_verbalizer_verbalize_special_character(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_JA_SPECIAL_CHARACTER) {
        int32_t i = 0;
        for (i = 0; i < PV_ARRAY_LEN(PV_NORMALIZER_SPECIAL_CHARACTERS_JA); i++) {
            int32_t num_bytes = 0;
            pv_status_t status = pv_language_num_bytes_character(PV_NORMALIZER_SPECIAL_CHARACTERS_JA[i][0], &num_bytes);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_language_num_bytes_character,
                        pv_status_to_string(status));
                return status;
            }
            if (strcmp(token->string, PV_NORMALIZER_SPECIAL_CHARACTERS_JA[i]) == 0) {
                break;
            }
        }
        const char *verbalized = PV_NORMALIZER_SPECIAL_CHARACTERS_VERBALIZED_JA[i];

        char *token_verbalized = calloc(strlen(verbalized) + 1, sizeof(char));
        if (!token_verbalized) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("token_verbalized"));
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(token_verbalized, verbalized);
        pv_normalizer_token_set_verbalized(token, token_verbalized);

        if (PV_NORMALIZER_SPECIAL_CHARACTERS_IS_VERBALIZED_TO_PUNCTUATION_JA[i]) {
            token->tag = PV_NORMALIZER_TAG_JA_PUNCTUATION;
        }
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_decimal(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_JA_DECIMAL_POINT) {
        const char *point = "テン";
        char *token_verbalized = calloc(strlen(point) + 1, sizeof(char));
        if (!token_verbalized) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("token_verbalized"));
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(token_verbalized, point);
        pv_normalizer_token_set_verbalized(token, token_verbalized);
    } else if (token->tag == PV_NORMALIZER_TAG_JA_DECIMAL_DIGITS) {
        char *verbalized = NULL;
        pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(token->string, true, &verbalized);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_cardinal_to_string,
                    pv_status_to_string(status));
            return status;
        }

        pv_normalizer_token_set_verbalized(token, verbalized);
    }

    return PV_STATUS_SUCCESS;
}

static pv_status_t pv_normalizer_verbalizer_verbalize_measurement(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    const char *verbalized = NULL;
    if ((token->tag == PV_NORMALIZER_TAG_JA_MEASUREMENT) && (token->tag_data_index >= 0)) {
        verbalized = PV_NORMALIZER_MEASUREMENTS_VERBALIZED_JA[token->tag_data_index];
    } else if (token->tag == PV_NORMALIZER_TAG_JA_PER_SLASH) {
        verbalized = "マイ";
    }

    if (verbalized != NULL) {
        char *token_verbalized = calloc(strlen(verbalized) + 1, sizeof(char));
        if (!token_verbalized) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("token_verbalized"));
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(token_verbalized, verbalized);
        pv_normalizer_token_set_verbalized(token, token_verbalized);
    }
    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_alphanum_spell_out(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_JA_LETTER_SPELL_OUT) {
        pv_status_t status = pv_normalizer_util_upper_inplace(token->string);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_util_upper_inplace,
                    pv_status_to_string(status));
            return status;
        }

        if (token->tag_data_index >= 0) {
            size_t verbalized_length = strlen(PV_NORMALIZER_ALPHANUM_CHARS_VERBALIZED_JA[token->tag_data_index]);
            if (verbalized_length > 0) {
                char *token_verbalized = calloc(verbalized_length + 1, sizeof(char));
                if (!token_verbalized) {
                    PV_ERROR_REPORT(
                            &pv_error_msg_alloc,
                            PV_ERROR_ARGS_PUBLIC_EMPTY(),
                            PV_ERROR_ARGS_PRIVATE("token_verbalized"));
                    return PV_STATUS_OUT_OF_MEMORY;
                }
                strcpy(token_verbalized, PV_NORMALIZER_ALPHANUM_CHARS_VERBALIZED_JA[token->tag_data_index]);
                pv_normalizer_token_set_verbalized(token, token_verbalized);
            }
        }
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_time(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_JA_TIME_COLON) {
        pv_normalizer_token_set_verbalized(token, NULL);
    } else if (token->tag == PV_NORMALIZER_TAG_JA_TIME_HOURS) {
        const char *number_string = token->string;

        int32_t offset = 0;
        if (number_string[0] == '0') {
            offset = 1;
        }

        char *o_clock_string = "ジ";
        char *hour_string = NULL;
        char *am_pm_string = "";

        pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(
                token->string + offset,
                false,
                &hour_string);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_cardinal_to_string,
                    pv_status_to_string(status));
            return status;
        }
        pv_normalizer_token_t *next = pv_normalizer_token_get_nth_token_after(token, 1, true);
        pv_normalizer_token_t *two_next = pv_normalizer_token_get_nth_token_after(token, 2, true);
        pv_normalizer_token_t *three_next = pv_normalizer_token_get_nth_token_after(token, 3, true);

        bool is_am_pm_hours =
                (next != NULL && (next->tag == PV_NORMALIZER_TAG_JA_TIME_AM_PM)) ||
                ((three_next != NULL && (three_next->tag == PV_NORMALIZER_TAG_JA_TIME_AM_PM)) &&
                 (two_next != NULL && (two_next->tag == PV_NORMALIZER_TAG_JA_TIME_MINUTES)) &&
                 (next != NULL && (next->tag == PV_NORMALIZER_TAG_JA_TIME_COLON)));

        if (is_am_pm_hours) {
            char *orig_am_pm = NULL;
            if (next != NULL && next->tag == PV_NORMALIZER_TAG_JA_TIME_AM_PM) {
                orig_am_pm = next->string;
            } else if (three_next != NULL) {
                orig_am_pm = three_next->string;
            }

            if (orig_am_pm != NULL) {
                if ((orig_am_pm[0] == 'a') || (orig_am_pm[0] == 'A')) {
                    am_pm_string = "ゴゼン";
                } else {
                    am_pm_string = "ゴゴ";
                }
            }
        }

        char *token_verbalized = calloc(strlen(am_pm_string) + strlen(hour_string) + strlen(o_clock_string) + 1, sizeof(char));
        if (!token_verbalized) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("token_verbalized"));
            return PV_STATUS_OUT_OF_MEMORY;
        }

        strcpy(token_verbalized, am_pm_string);
        strcat(token_verbalized, hour_string);
        strcat(token_verbalized, o_clock_string);
        free(hour_string);
        pv_normalizer_token_set_verbalized(token, token_verbalized);
    } else if ((token->tag == PV_NORMALIZER_TAG_JA_TIME_MINUTES) ||
               (token->tag == PV_NORMALIZER_TAG_JA_TIME_SECONDS)) {
        const char *number_string = token->string;

        int32_t offset = 0;
        if (number_string[0] == '0') {
            offset = 1;
            if (number_string[1] == '0') {
                return PV_STATUS_SUCCESS;
            }
        }

        char *verbalized = NULL;
        pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(
                number_string + offset,
                false,
                &verbalized);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_cardinal_to_string,
                    pv_status_to_string(status));
            return status;
        }

        char *time_string = token->tag == PV_NORMALIZER_TAG_JA_TIME_MINUTES ? "プン" : "ビョウ";
        char *token_verbalized = calloc(strlen(verbalized) + strlen(time_string) + 1, sizeof(char));
        if (!token_verbalized) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("token_verbalized"));
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcat(token_verbalized, verbalized);
        strcat(token_verbalized, time_string);
        free(verbalized);
        pv_normalizer_token_set_verbalized(token, token_verbalized);

    } else if (token->tag == PV_NORMALIZER_TAG_JA_TIME_AM_PM) {
        pv_normalizer_token_t *prev = pv_normalizer_token_get_nth_token_before(token, 1, true);
        pv_normalizer_token_t *two_prev = pv_normalizer_token_get_nth_token_before(token, 2, true);
        pv_normalizer_token_t *three_prev = pv_normalizer_token_get_nth_token_before(token, 3, true);

        bool is_prev_hour =
                (prev != NULL) &&
                (prev->tag == PV_NORMALIZER_TAG_JA_TIME_HOURS);
        bool is_prev_time =
                (prev != NULL && prev->tag == PV_NORMALIZER_TAG_JA_TIME_MINUTES) &&
                (two_prev != NULL && two_prev->tag == PV_NORMALIZER_TAG_JA_TIME_COLON) &&
                (three_prev != NULL && three_prev->tag == PV_NORMALIZER_TAG_JA_TIME_HOURS);

        if (!(is_prev_hour || is_prev_time)) {
            char *am_pm_string = NULL;
            if ((token->string[0] == 'a') || (token->string[0] == 'A')) {
                am_pm_string = "ゴゼン";
            } else {
                am_pm_string = "ゴゴ";
            }

            char *token_verbalized = calloc(strlen(am_pm_string) + 1, sizeof(char));
            if (!token_verbalized) {
                PV_ERROR_REPORT(
                        &pv_error_msg_alloc,
                        PV_ERROR_ARGS_PUBLIC_EMPTY(),
                        PV_ERROR_ARGS_PRIVATE("token->verbalized"));
                return PV_STATUS_OUT_OF_MEMORY;
            }
            strcpy(token_verbalized, am_pm_string);
            pv_normalizer_token_set_verbalized(token, token_verbalized);
        }
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_fraction_slash(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_JA_FRACTION_SLASH) {
        if ((token->next != NULL) &&
            ((token->next->tag == PV_NORMALIZER_TAG_JA_CARDINAL) ||
             (token->next->tag == PV_NORMALIZER_TAG_JA_NEGATIVE_CARDINAL))) {

            const char *over = "ワル";

            char *token_verbalized = calloc(strlen(over) + 1, sizeof(char));
            if (!token_verbalized) {
                PV_ERROR_REPORT(
                        &pv_error_msg_alloc,
                        PV_ERROR_ARGS_PUBLIC_EMPTY(),
                        PV_ERROR_ARGS_PRIVATE("token_verbalized"));
                return PV_STATUS_OUT_OF_MEMORY;
            }
            strcpy(token_verbalized, over);
            pv_normalizer_token_set_verbalized(token, token_verbalized);
        }
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_dot(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_JA_DOT) {
        pv_normalizer_token_t *previous = pv_normalizer_token_get_nth_token_before(token, 1, true);
        if ((previous != NULL) && (previous->tag == PV_NORMALIZER_TAG_JA_MEASUREMENT)) {
            return PV_STATUS_SUCCESS;
        }

        const char *dot = "ドット";

        char *token_verbalized = calloc(strlen(dot) + 1, sizeof(char));
        if (!token_verbalized) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("token_verbalized"));
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(token_verbalized, dot);
        pv_normalizer_token_set_verbalized(token, token_verbalized);
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_colon(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_JA_COLON) {
        const char *colon = "コロン";

        char *token_verbalized = calloc(strlen(colon) + 1, sizeof(char));
        if (!token_verbalized) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("token_verbalized"));
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(token_verbalized, colon);
        pv_normalizer_token_set_verbalized(token, token_verbalized);
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_top_level_domain(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_JA_TOP_LEVEL_DOMAIN) {
        if (token->tag_data_index >= 0) {
            const char *verbalized = PV_NORMALIZER_TOP_LEVEL_DOMAINS_VERBALIZED_JA[token->tag_data_index];
            char *token_verbalized = calloc(strlen(verbalized) + 1, sizeof(char));
            if (!token_verbalized) {
                PV_ERROR_REPORT(
                        &pv_error_msg_alloc,
                        PV_ERROR_ARGS_PUBLIC_EMPTY(),
                        PV_ERROR_ARGS_PRIVATE("token_verbalized"));
                return PV_STATUS_OUT_OF_MEMORY;
            }
            strcpy(token_verbalized, verbalized);
            pv_normalizer_token_set_verbalized(token, token_verbalized);
        }
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_currency(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_JA_CURRENCY) {
        char *string = token->string;
        int32_t length = (int32_t) strlen(string);

        int32_t i = 0;
        while (!isdigit(string[i])) {
            i++;
        }

        int32_t number_start_index = i;
        int32_t number_end_index = i;
        while ((i < length) && isdigit(string[number_end_index])) {
            number_end_index += 1;
        }

        int32_t number_string_length = number_end_index - number_start_index;

        char *number_string = calloc(number_string_length + 1, sizeof(char));
        if (!number_string) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("number_string"));
            return PV_STATUS_OUT_OF_MEMORY;
        }
        memcpy(number_string, string + number_start_index, number_string_length * sizeof(char));

        char *verbalized_number_string = NULL;
        pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(
                number_string,
                false,
                &verbalized_number_string);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_cardinal_to_string,
                    pv_status_to_string(status));
            free(number_string);
            return status;
        }

        const char *currency_string = PV_NORMALIZER_CURRENCIES_VERBALIZED_JA[token->tag_data_index];

        free(number_string);

        bool has_no_decimal = ((number_end_index == length) || (string[number_end_index] != '.'));
        if (has_no_decimal) {
            int32_t verbalized_length =
                    (int32_t) strlen(verbalized_number_string) +
                    (int32_t) strlen(currency_string);

            char *token_verbalized = calloc(verbalized_length + 1, sizeof(char));
            if (!token_verbalized) {
                PV_ERROR_REPORT(
                        &pv_error_msg_alloc,
                        PV_ERROR_ARGS_PUBLIC_EMPTY(),
                        PV_ERROR_ARGS_PRIVATE("token_verbalized"));
                free(verbalized_number_string);
                return PV_STATUS_OUT_OF_MEMORY;
            }
            strcpy(token_verbalized, verbalized_number_string);
            strcat(token_verbalized, currency_string);
            pv_normalizer_token_set_verbalized(token, token_verbalized);
            free(verbalized_number_string);
            return PV_STATUS_SUCCESS;
        }

        int32_t digit_start_index = number_end_index + 1;
        int32_t digit_end_index = number_end_index + 1;
        while ((i < length) && isdigit(string[digit_end_index])) {
            digit_end_index += 1;
        }

        int32_t digit_string_length = digit_end_index - digit_start_index;

        // .00 is not verbalized e.g $1.00 -> one dollar
        if ((string[digit_start_index] == '0') && (string[digit_start_index + 1] == '0')) {
            int32_t verbalized_length =
                    (int32_t) strlen(verbalized_number_string) +
                    (int32_t) strlen(currency_string);

            char *token_verbalized = calloc(verbalized_length + 1, sizeof(char));
            if (!token_verbalized) {
                PV_ERROR_REPORT(
                        &pv_error_msg_alloc,
                        PV_ERROR_ARGS_PUBLIC_EMPTY(),
                        PV_ERROR_ARGS_PRIVATE("token_verbalized"));
                free(verbalized_number_string);
                return PV_STATUS_OUT_OF_MEMORY;
            }
            strcpy(token_verbalized, verbalized_number_string);
            strcat(token_verbalized, currency_string);
            pv_normalizer_token_set_verbalized(token, token_verbalized);
            free(verbalized_number_string);
            return PV_STATUS_SUCCESS;
            // .0X is verbalized as X e.g. $1.05 -> one dollar and five cents
        } else if (string[digit_start_index] == '0') {
            digit_start_index++;
        }

        char *digit_string = calloc(digit_string_length + 1, sizeof(char));
        if (!digit_string) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("digit_string"));
            free(verbalized_number_string);
            return PV_STATUS_OUT_OF_MEMORY;
        }
        memcpy(digit_string, string + digit_start_index, digit_string_length * sizeof(char));

        char *verbalized_digit_string = NULL;
        status = pv_normalizer_verbalizer_cardinal_to_string(
                digit_string,
                false,
                &verbalized_digit_string);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_cardinal_to_string,
                    pv_status_to_string(status));
            free(verbalized_number_string);
            free(digit_string);
            return status;
        }

        const char *sub_currency_string = PV_NORMALIZER_SUB_CURRENCIES_VERBALIZED_JA[token->tag_data_index];
        free(digit_string);

        int32_t verbalized_length =
                (int32_t) strlen(verbalized_number_string) +
                (int32_t) strlen(currency_string) +
                (int32_t) strlen(verbalized_digit_string) +
                (int32_t) strlen(sub_currency_string);

        char *token_verbalized = calloc(verbalized_length + 1, sizeof(char));
        if (!token_verbalized) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("token_verbalized"));
            free(verbalized_number_string);
            free(verbalized_digit_string);
            return PV_STATUS_OUT_OF_MEMORY;
        }

        strcpy(token_verbalized, verbalized_number_string);
        strcat(token_verbalized, currency_string);
        strcat(token_verbalized, verbalized_digit_string);
        strcat(token_verbalized, sub_currency_string);
        free(verbalized_number_string);
        free(verbalized_digit_string);
        pv_normalizer_token_set_verbalized(token, token_verbalized);
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_negative_currency(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_JA_NEGATIVE_CURRENCY) {
        char *string = token->string;
        int32_t length = (int32_t) strlen(string);

        char *negative = "マイナス";

        int32_t i = 1;
        while (!isdigit(string[i])) {
            i++;
        }

        int32_t number_start_index = i;
        int32_t number_end_index = i;
        while ((i < length) && isdigit(string[number_end_index])) {
            number_end_index += 1;
        }

        int32_t number_string_length = number_end_index - number_start_index;

        char *number_string = calloc(number_string_length + 1, sizeof(char));
        if (!number_string) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("number_string"));
            return PV_STATUS_OUT_OF_MEMORY;
        }
        memcpy(number_string, string + number_start_index, number_string_length * sizeof(char));

        char *verbalized_number_string = NULL;
        pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(
                number_string,
                false,
                &verbalized_number_string);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_cardinal_to_string,
                    pv_status_to_string(status));
            free(number_string);
            return status;
        }

        const char *currency_string = PV_NORMALIZER_CURRENCIES_VERBALIZED_JA[token->tag_data_index];

        free(number_string);

        bool has_no_decimal = ((number_end_index == length) || (string[number_end_index] != '.'));
        if (has_no_decimal) {
            int32_t verbalized_length =
                    (int32_t) strlen(negative) +
                    (int32_t) strlen(verbalized_number_string) +
                    (int32_t) strlen(currency_string);

            char *token_verbalized = calloc(verbalized_length + 1, sizeof(char));
            if (!token_verbalized) {
                PV_ERROR_REPORT(
                        &pv_error_msg_alloc,
                        PV_ERROR_ARGS_PUBLIC_EMPTY(),
                        PV_ERROR_ARGS_PRIVATE("token_verbalized"));
                free(verbalized_number_string);
                return PV_STATUS_OUT_OF_MEMORY;
            }

            strcpy(token_verbalized, negative);
            strcat(token_verbalized, verbalized_number_string);
            strcat(token_verbalized, currency_string);

            free(verbalized_number_string);
            pv_normalizer_token_set_verbalized(token, token_verbalized);
            return PV_STATUS_SUCCESS;
        }

        int32_t digit_start_index = number_end_index + 1;
        int32_t digit_end_index = number_end_index + 1;
        while ((i < length) && isdigit(string[digit_end_index])) {
            digit_end_index += 1;
        }

        int32_t digit_string_length = digit_end_index - digit_start_index;

        char *digit_string = calloc(digit_string_length + 1, sizeof(char));
        if (!digit_string) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("digit_string"));
            free(verbalized_number_string);
            return PV_STATUS_OUT_OF_MEMORY;
        }
        memcpy(digit_string, string + digit_start_index, digit_string_length * sizeof(char));

        char *verbalized_digit_string = NULL;
        status = pv_normalizer_verbalizer_cardinal_to_string(
                digit_string,
                false,
                &verbalized_digit_string);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_cardinal_to_string,
                    pv_status_to_string(status));
            free(verbalized_number_string);
            free(digit_string);
            return status;
        }

        const char *sub_currency_string = PV_NORMALIZER_SUB_CURRENCIES_VERBALIZED_JA[token->tag_data_index];
        free(digit_string);

        int32_t verbalized_length =
                (int32_t) strlen(negative) +
                (int32_t) strlen(verbalized_number_string) +
                (int32_t) strlen(currency_string) +
                (int32_t) strlen(verbalized_digit_string) +
                (int32_t) strlen(sub_currency_string);

        char *token_verbalized = calloc(verbalized_length + 1, sizeof(char));
        if (!token_verbalized) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("token_verbalized"));
            free(verbalized_number_string);
            return PV_STATUS_OUT_OF_MEMORY;
        }

        strcpy(token_verbalized, negative);
        strcat(token_verbalized, verbalized_number_string);
        strcat(token_verbalized, currency_string);
        strcat(token_verbalized, verbalized_digit_string);
        strcat(token_verbalized, sub_currency_string);

        free(verbalized_number_string);
        free(verbalized_digit_string);
        pv_normalizer_token_set_verbalized(token, token_verbalized);
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_currency_symbol(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_JA_CURRENCY_SYMBOL) {
        pv_normalizer_token_t *previous = pv_normalizer_token_get_nth_token_before(token, 1, true);
        if ((previous == NULL) || (token->tag_data_index < 0)) {
            return PV_STATUS_SUCCESS;
        }

        const char *currency_string = PV_NORMALIZER_CURRENCIES_VERBALIZED_JA[token->tag_data_index];

        char *token_verbalized = calloc(strlen(currency_string) + 1, sizeof(char));
        if (!token_verbalized) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("token_verbalized"));
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(token_verbalized, currency_string);
        pv_normalizer_token_set_verbalized(token, token_verbalized);
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_abbreviation(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_JA_ABBREVIATION) {
        PV_ASSERT((token->tag_data_index >= 0) && (token->tag_data_index < PV_ARRAY_LEN(PV_NORMALIZER_ABBREVIATIONS_VERBALIZED_JA)));
        const char *verbalized = PV_NORMALIZER_ABBREVIATIONS_VERBALIZED_JA[token->tag_data_index];
        char *token_verbalized = calloc(strlen(verbalized) + 1, sizeof(char));
        if (!token_verbalized) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("token_verbalized"));
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(token_verbalized, verbalized);
        pv_normalizer_token_set_verbalized(token, token_verbalized);
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_digits_sequence(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    const char *comma_string = "、";

    const char *string = token->string;
    int32_t length = (int32_t) strlen(string);

    if (token->tag == PV_NORMALIZER_TAG_JA_DIGITS_WITH_PARENTHESES) {
        int32_t digits_length = length - 2;
        char *number_string = calloc(digits_length + 1, sizeof(char));
        if (!number_string) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("number_string"));
            return PV_STATUS_OUT_OF_MEMORY;
        }
        memcpy(number_string, string + 1, digits_length * sizeof(char));
        number_string[digits_length] = '\0';

        char *verbalized = NULL;
        pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(number_string, true, &verbalized);
        free(number_string);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_cardinal_to_string,
                    pv_status_to_string(status));
            return status;
        }

        const char *comma_string_with_space = "、 ";
        int32_t total_length =
                (int32_t) strlen(verbalized) +
                (int32_t) strlen(comma_string) +
                (int32_t) strlen(comma_string_with_space);
        char *token_verbalized = calloc(total_length + 1, sizeof(char));
        if (!token_verbalized) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("token_verbalized"));
            free(verbalized);
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(token_verbalized, comma_string_with_space);
        strcat(token_verbalized, verbalized);
        strcat(token_verbalized, comma_string);
        free(verbalized);
        pv_normalizer_token_set_verbalized(token, token_verbalized);
    } else if (token->tag == PV_NORMALIZER_TAG_JA_DIGITS) {
        char *verbalized = NULL;
        pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(
                string,
                true,
                &verbalized);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_cardinal_to_string,
                    pv_status_to_string(status));
            return status;
        }
        pv_normalizer_token_set_verbalized(token, verbalized);
    } else if (token->tag == PV_NORMALIZER_TAG_JA_DIGITS_SEPARATOR) {
        char *token_verbalized = calloc(strlen(comma_string) + 1, sizeof(char));
        if (!token_verbalized) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("token_verbalized"));
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(token_verbalized, comma_string);
        pv_normalizer_token_set_verbalized(token, token_verbalized);
        token->tag = PV_NORMALIZER_TAG_JA_PUNCTUATION;
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_dates(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_JA_DATE_SEPARATOR) {
        pv_normalizer_token_set_verbalized(token, NULL);
    } else if (token->tag == PV_NORMALIZER_TAG_JA_DATE_DAY) {
        if (token->tag_data_index >= 0) {
            const char *day_name = PV_NORMALIZER_DAY_NAMES_VERBALIZED_JA[token->tag_data_index];
            pv_normalizer_token_t *next_token = pv_normalizer_token_get_nth_token_after(token, 1, true);
            if ((next_token == NULL) || (next_token->tag != PV_NORMALIZER_TAG_JA_DATE_DAY)) {
                const char *day_marker_name = PV_NORMALIZER_DAY_MARKER_NAMES_VERBALIZED_JA[token->tag_data_index];
                char *token_verbalized = calloc(strlen(day_name) + strlen(day_marker_name) + 1, sizeof(char));
                if (!token_verbalized) {
                    PV_ERROR_REPORT(
                            &pv_error_msg_alloc,
                            PV_ERROR_ARGS_PUBLIC_EMPTY(),
                            PV_ERROR_ARGS_PRIVATE("token_verbalized"));
                    return PV_STATUS_OUT_OF_MEMORY;
                }
                strcat(token_verbalized, day_name);
                strcat(token_verbalized, day_marker_name);
                pv_normalizer_token_set_verbalized(token, token_verbalized);
            } else {
                char *token_verbalized = calloc(strlen(day_name) + 1, sizeof(char));
                if (!token_verbalized) {
                    PV_ERROR_REPORT(
                            &pv_error_msg_alloc,
                            PV_ERROR_ARGS_PUBLIC_EMPTY(),
                            PV_ERROR_ARGS_PRIVATE("token_verbalized"));
                    return PV_STATUS_OUT_OF_MEMORY;
                }
                strcpy(token_verbalized, day_name);
                pv_normalizer_token_set_verbalized(token, token_verbalized);
            }
        } else {
            pv_normalizer_token_t *prev_token = pv_normalizer_token_get_nth_token_before(token, 1, true);
            if ((prev_token != NULL) && (prev_token->tag_data_index >= 0)) {
                pv_normalizer_token_set_reading(token, NULL);
                const char *day_marker_reading = PV_NORMALIZER_DAY_MARKER_NAMES_VERBALIZED_JA[prev_token->tag_data_index];
                char *token_verbalized = calloc(strlen(day_marker_reading) + 1, sizeof(char));
                if (!token_verbalized) {
                    PV_ERROR_REPORT(
                            &pv_error_msg_alloc,
                            PV_ERROR_ARGS_PUBLIC_EMPTY(),
                            PV_ERROR_ARGS_PRIVATE("token_verbalized"));
                    return PV_STATUS_OUT_OF_MEMORY;
                }
                strcpy(token_verbalized, day_marker_reading);
                pv_normalizer_token_set_verbalized(token, token_verbalized);
            }
        }
    } else if (token->tag == PV_NORMALIZER_TAG_JA_DATE_MONTH) {
        const char *month_marker_reading = "ガツ";
        if (token->tag_data_index >= 0) {
            const char *month_name = PV_NORMALIZER_MONTH_NAMES_VERBALIZED_JA[token->tag_data_index];
            pv_normalizer_token_t *next_token = pv_normalizer_token_get_nth_token_after(token, 1, true);
            if (next_token == NULL || next_token->tag != PV_NORMALIZER_TAG_JA_DATE_MONTH) {
                char *token_verbalized = calloc(strlen(month_name) + strlen(month_marker_reading) + 1, sizeof(char));
                if (!token_verbalized) {
                    PV_ERROR_REPORT(
                            &pv_error_msg_alloc,
                            PV_ERROR_ARGS_PUBLIC_EMPTY(),
                            PV_ERROR_ARGS_PRIVATE("token_verbalized"));
                    return PV_STATUS_OUT_OF_MEMORY;
                }
                strcat(token_verbalized, month_name);
                strcat(token_verbalized, month_marker_reading);
                pv_normalizer_token_set_verbalized(token, token_verbalized);
            } else {
                char *token_verbalized = calloc(strlen(month_name) + 1, sizeof(char));
                if (!token_verbalized) {
                    PV_ERROR_REPORT(
                            &pv_error_msg_alloc,
                            PV_ERROR_ARGS_PUBLIC_EMPTY(),
                            PV_ERROR_ARGS_PRIVATE("token_verbalized"));
                    return PV_STATUS_OUT_OF_MEMORY;
                }
                strcpy(token_verbalized, month_name);
                pv_normalizer_token_set_verbalized(token, token_verbalized);
            }
        } else {
            pv_normalizer_token_t *prev_token = pv_normalizer_token_get_nth_token_before(token, 1, true);
            if ((prev_token != NULL) && (prev_token->tag_data_index >= 0)) {
                pv_normalizer_token_set_reading(token, NULL);
                char *token_verbalized = calloc(strlen(month_marker_reading) + 1, sizeof(char));
                if (!token_verbalized) {
                    PV_ERROR_REPORT(
                            &pv_error_msg_alloc,
                            PV_ERROR_ARGS_PUBLIC_EMPTY(),
                            PV_ERROR_ARGS_PRIVATE("token_verbalized"));
                    return PV_STATUS_OUT_OF_MEMORY;
                }
                strcpy(token_verbalized, month_marker_reading);
                pv_normalizer_token_set_verbalized(token, token_verbalized);
            }
        }
    } else if (token->tag == PV_NORMALIZER_TAG_JA_DATE_YEAR) {
        const char *year_marker_reading = "ネン";
        if (pv_normalizer_util_only_contains_digits(token->string)) {
            char *year_name = NULL;
            pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(
                    token->string,
                    false,
                    &year_name);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_verbalizer_cardinal_to_string,
                        pv_status_to_string(status));
                return status;
            }

            pv_normalizer_token_t *next_token = pv_normalizer_token_get_nth_token_after(token, 1, true);
            if (next_token == NULL || next_token->tag != PV_NORMALIZER_TAG_JA_DATE_YEAR) {
                char *token_verbalized = calloc(strlen(year_name) + strlen(year_marker_reading) + 1, sizeof(char));
                if (!token_verbalized) {
                    PV_ERROR_REPORT(
                            &pv_error_msg_alloc,
                            PV_ERROR_ARGS_PUBLIC_EMPTY(),
                            PV_ERROR_ARGS_PRIVATE("token_verbalized"));
                    return PV_STATUS_OUT_OF_MEMORY;
                }
                strcat(token_verbalized, year_name);
                strcat(token_verbalized, year_marker_reading);
                pv_normalizer_token_set_verbalized(token, token_verbalized);
            } else {
                char *token_verbalized = calloc(strlen(year_name) + 1, sizeof(char));
                if (!token_verbalized) {
                    PV_ERROR_REPORT(
                            &pv_error_msg_alloc,
                            PV_ERROR_ARGS_PUBLIC_EMPTY(),
                            PV_ERROR_ARGS_PRIVATE("token_verbalized"));
                    return PV_STATUS_OUT_OF_MEMORY;
                }
                strcpy(token_verbalized, year_name);
                pv_normalizer_token_set_verbalized(token, token_verbalized);
            }
            free(year_name);
        } else {
            pv_normalizer_token_t *prev_token = pv_normalizer_token_get_nth_token_before(token, 1, true);
            if ((prev_token != NULL) && (prev_token->tag == PV_NORMALIZER_TAG_JA_DATE_YEAR)) {
                pv_normalizer_token_set_reading(token, NULL);
                char *token_verbalized = calloc(strlen(year_marker_reading) + 1, sizeof(char));
                if (!token_verbalized) {
                    PV_ERROR_REPORT(
                            &pv_error_msg_alloc,
                            PV_ERROR_ARGS_PUBLIC_EMPTY(),
                            PV_ERROR_ARGS_PRIVATE("token_verbalized"));
                    return PV_STATUS_OUT_OF_MEMORY;
                }
                strcpy(token_verbalized, year_marker_reading);
                pv_normalizer_token_set_verbalized(token, token_verbalized);
            }
        }
    }
    return PV_STATUS_SUCCESS;
}

void pv_normalizer_verbalizer_verbalize_names(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_JA_NAME_INITIAL_DOT) {
        pv_normalizer_token_set_verbalized(token, NULL);
    }
}
