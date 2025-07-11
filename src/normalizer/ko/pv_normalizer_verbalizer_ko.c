#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "core/pv_assert.h"
#include "core/pv_error_messages.h"
#include "orca/normalizer/pv_normalizer_token.h"
#include "orca/normalizer/pv_normalizer_use_cases.h"
#include "orca/normalizer/pv_normalizer_verbalizer.h"

#include "orca/normalizer/ko/pv_normalizer_data_ko/pv_normalizer_data_ko.h"
#include "orca/normalizer/ko/pv_normalizer_tags_ko.h"
#include "orca/normalizer/ko/pv_normalizer_verbalizer_ko.h"

#include "orca/normalizer/pv_normalizer_data/pv_normalizer_shared_data.h"
#include "util/pv_string.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_normalizer_mock.h"

#endif

struct pv_normalizer_verbalizer_ko {
    int32_t num_use_cases;
    const pv_normalizer_use_cases_ko_t *use_cases;
};

static void pv_normalizer_verbalizer_synchronize_language_agnostic_tags(pv_normalizer_token_list_t *token_list);

static pv_status_t pv_normalizer_verbalizer_verbalize_word(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_punctuation(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_cardinal(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_negative_cardinal(pv_normalizer_token_t *token);

static void cardinal_to_string_helper(char *dest, const char *src, int32_t *length, bool get_src_length);

static pv_status_t pv_normalizer_verbalizer_cardinal_to_string_helper(
        const char *number_string,
        int32_t *length,
        bool to_digit_string,
        bool dry_run,
        bool use_native,
        char **string);

static pv_status_t pv_normalizer_verbalizer_cardinal_to_string(
        const char *number_string,
        bool to_digit_string,
        bool use_native,
        char **string);

static pv_status_t pv_normalizer_verbalizer_verbalize_custom_pronunciation(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_number_range(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_special_character(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_decimal(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_measurement(
        pv_normalizer_token_list_t *token_list,
        pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_cardinal_spell_out_helper(
        const char *digit_string,
        bool dry_run,
        int32_t *length,
        char *verbalized);

static pv_status_t pv_normalizer_verbalizer_verbalize_alphanum_spell_out(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_time(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_fraction_slash(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_fraction_denominator(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_dot(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_colon(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_top_level_domain(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_currency(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_negative_currency(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_currency_symbol(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_digits_sequence(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_dates(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_names(pv_normalizer_token_t *token);

pv_status_t PV_MOCKABLE(pv_normalizer_verbalizer_ko_init)(
        int32_t num_use_cases,
        const pv_normalizer_use_cases_ko_t *use_cases,
        pv_normalizer_verbalizer_ko_t **object) {
    PV_ASSERT(use_cases);
    PV_ASSERT(object);

    *object = NULL;

    pv_normalizer_verbalizer_ko_t *o = calloc(1, sizeof(pv_normalizer_verbalizer_ko_t));
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

void PV_MOCKABLE(pv_normalizer_verbalizer_ko_delete)(pv_normalizer_verbalizer_ko_t *object) {
    if (object) {
        free(object);
    }
}

pv_status_t PV_MOCKABLE(pv_normalizer_verbalizer_ko_verbalize)(
        pv_normalizer_verbalizer_ko_t *object,
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
            if (object->use_cases[j] == PV_NORMALIZER_USE_WORD_NORMALIZER_KO) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_word(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_word,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_PUNCTUATION_NORMALIZER_KO) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_punctuation(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_punctuation,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_CARDINAL_NORMALIZER_KO) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_cardinal(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_cardinal,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_KO) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_negative_cardinal(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_negative_cardinal,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_CUSTOM_PRONUNCIATION_NORMALIZER_KO) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_custom_pronunciation(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_custom_pronunciation,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_NUMBER_RANGE_NORMALIZER_KO) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_number_range(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_number_range,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_SPECIAL_CHARACTER_NORMALIZER_KO) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_special_character(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_special_character,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_DECIMAL_NORMALIZER_KO) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_decimal(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_decimal,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_MEASUREMENT_NORMALIZER_KO) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_measurement(token_list, token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_measurement,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_ALPHANUM_SPELL_OUT_NORMALIZER_KO) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_alphanum_spell_out(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_alphanum_spell_out,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_TIME_NORMALIZER_KO) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_time(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_time,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_FRACTION_NORMALIZER_KO) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_fraction_slash(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_fraction_slash,
                            pv_status_to_string(status));
                    return status;
                }

                status = pv_normalizer_verbalizer_verbalize_fraction_denominator(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_fraction_denominator,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_URL_NORMALIZER_KO) {
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
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_CURRENCY_NORMALIZER_KO) {
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
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_DIGITS_SEQUENCE_NORMALIZER_KO) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_digits_sequence(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_digits_sequence,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_DATE_NORMALIZER_KO) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_dates(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_dates,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_NAME_NORMALIZER_KO) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_names(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_names,
                            pv_status_to_string(status));
                    return status;
                }
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
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_PUNCTUATION,
            -1,
            PV_NORMALIZER_TAG_KO_CUSTOM_PRONUNCIATION,
            -1);
}

pv_status_t pv_normalizer_verbalizer_verbalize_word(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_KO_WORD) {
        pv_status_t status = pv_normalizer_verbalizer_verbalize_word_common(token);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_verbalize_word_common,
                    pv_status_to_string(status));
            return status;
        }
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_punctuation(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_KO_PUNCTUATION) {
        char *string = token->string;
        if (!(strcmp(string, ":") == 0 || strcmp(string, "\"") == 0 || strcmp(string, APOSTROPHE) == 0)) {
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
        int32_t *length,
        bool to_digit_string,
        bool dry_run,
        bool use_native,
        char **string) {
    PV_ASSERT(number_string);
    PV_ASSERT(*length >= 0);
    PV_ASSERT(string);

    *string = NULL;
    char *result = NULL;

    if (!dry_run && (*length >= 0)) {
        result = calloc(*length + 1, sizeof(char));
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
        pv_normalizer_util_string_number_greater_than_int(number_string, MAX_VERBALIZED_CARDINAL_KO)) {
        while (*number_string != '\0') {
            int32_t digit = (int32_t) *number_string - '0';
            if (digit == 0) {
                cardinal_to_string_helper(result, "영", length, dry_run);
            } else {
                cardinal_to_string_helper(result, ONE_TO_NINE_KO[(int32_t) digit], length, dry_run);
            }
            cardinal_to_string_helper(result, " ", length, dry_run);
            number_string++;
        }

        if (!dry_run) {
            result[*length - 1] = '\0';
        }
    } else {
        int64_t number = (int64_t) strtoll(number_string, NULL, 10);
        if (number < 0) {
            return PV_STATUS_SUCCESS;
        }

        if (use_native && (number < 100)) {
            int32_t tens = (int32_t) number / 10;
            int32_t ones = (int32_t) number % 10;

            cardinal_to_string_helper(result, TENS_NATIVE_KO[tens], length, dry_run);
            cardinal_to_string_helper(result, ONE_TO_NINE_NATIVE_KO[ones], length, dry_run);
            *string = result;

            return PV_STATUS_SUCCESS;
        }

        if (number < 10) {
            cardinal_to_string_helper(result, ONE_TO_NINE_KO[number], length, dry_run);
            *string = result;

            return PV_STATUS_SUCCESS;
        }

        int64_t remainder = number;
        int64_t current_multiplier;
        int64_t current_value;

        int32_t i = 0;
        while (i < PV_ARRAY_LEN(MULTIPLIERS_KO_VALUES)) {
            current_multiplier = MULTIPLIERS_KO_VALUES[i];

            current_value = remainder / current_multiplier;
            remainder %= current_multiplier;

            if (current_value >= 10) {
                int64_t thousands = current_value;
                for (int32_t j = 4; j < PV_ARRAY_LEN(MULTIPLIERS_KO_VALUES); j++) {
                    int32_t ones_value = (int32_t) (thousands / MULTIPLIERS_KO_VALUES[j]);
                    if (ones_value > 0) {
                        if (ones_value != 1) {
                            cardinal_to_string_helper(result, ONE_TO_NINE_KO[ones_value], length, dry_run);
                        }
                        cardinal_to_string_helper(result, MULTIPLIERS_KO[j], length, dry_run);
                        thousands %= MULTIPLIERS_KO_VALUES[j];
                    }
                }

                if (thousands > 0) {
                    cardinal_to_string_helper(result, ONE_TO_NINE_KO[thousands], length, dry_run);
                }
            } else if (i < 3 && current_value == 1) {
                cardinal_to_string_helper(result, ONE_TO_NINE_KO[1], length, dry_run);
            } else if (current_value > 1 && current_value < 10) {
                cardinal_to_string_helper(result, ONE_TO_NINE_KO[current_value], length, dry_run);
            }

            if (current_value > 0) {
                cardinal_to_string_helper(result, MULTIPLIERS_KO[i], length, dry_run);
            }

            i++;
        }

        if (remainder > 0) {
            cardinal_to_string_helper(result, ONE_TO_NINE_KO[remainder], length, dry_run);
        }

        if (!dry_run) {
            result[*length] = '\0';
        }
    }

    *string = result;

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_cardinal_to_string(
        const char *number_string,
        bool to_digit_string,
        bool use_native,
        char **string) {
    PV_ASSERT(number_string);
    PV_ASSERT(string);

    int32_t length = 0;

    pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string_helper(
            number_string,
            &length,
            to_digit_string,
            true,
            use_native,
            string);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_verbalizer_cardinal_to_string_helper,
                pv_status_to_string(status));
        return status;
    }

    status = pv_normalizer_verbalizer_cardinal_to_string_helper(
            number_string,
            &length,
            to_digit_string,
            false,
            use_native,
            string);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_verbalizer_cardinal_to_string_helper,
                pv_status_to_string(status));
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static bool pv_normalizer_verbalizer_use_native_cardinal(pv_normalizer_token_t *token) {
    if (token == NULL) {
        return false;
    }
    if (token->tag != PV_NORMALIZER_TAG_KO_WORD) {
        return false;
    }

    for (int32_t i = 0; i < PV_ARRAY_LEN(IGNORE_NATIVE_CASES_KO); i++) {
        if (strncmp(token->string, IGNORE_NATIVE_CASES_KO[i], strlen(IGNORE_NATIVE_CASES_KO[i])) == 0) {
            return false;
        }
    }

    for (int32_t i = 0; i < PV_ARRAY_LEN(NATIVE_CASES_KO); i++) {
        if (strncmp(token->string, NATIVE_CASES_KO[i], strlen(NATIVE_CASES_KO[i])) == 0) {
            return true;
        }
    }

    return false;
}

pv_status_t pv_normalizer_verbalizer_verbalize_cardinal(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    bool is_ordinal_denominator =
            ((token->previous != NULL) &&
             (token->previous->tag == PV_NORMALIZER_TAG_KO_FRACTION_SLASH) &&
             (token->previous->verbalized == NULL));

    if ((token->tag == PV_NORMALIZER_TAG_KO_CARDINAL) && !is_ordinal_denominator) {
        pv_normalizer_token_t *next = pv_normalizer_token_get_nth_token_after(token, 1, true);
        bool use_native = pv_normalizer_verbalizer_use_native_cardinal(next);

        char *verbalized = NULL;
        pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(token->string, false, use_native, &verbalized);
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

    if (token->tag == PV_NORMALIZER_TAG_KO_NEGATIVE_CARDINAL) {
        char *verbalized = NULL;
        const char *digit_string = token->string + 1;
        pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(digit_string, false, false, &verbalized);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_cardinal_to_string,
                    pv_status_to_string(status));
            return status;
        }

        const char *negative = "마이너스 ";

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

    if (token->tag == PV_NORMALIZER_TAG_KO_CUSTOM_PRONUNCIATION) {
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

    if (token->tag == PV_NORMALIZER_TAG_KO_NUMBER_RANGE_TO) {
        char *token_verbalized = calloc(strlen(NUMBER_RANGE_INBETWEEN_STRING_KO) + 1, sizeof(char));
        if (!token_verbalized) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("token_verbalized"));
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(token_verbalized, NUMBER_RANGE_INBETWEEN_STRING_KO);
        pv_normalizer_token_set_verbalized(token, token_verbalized);
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_special_character(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_KO_SPECIAL_CHARACTER) {
        int32_t i = 0;
        for (i = 0; i < PV_ARRAY_LEN(PV_NORMALIZER_SPECIAL_CHARACTERS_KO); i++) {
            int32_t num_bytes = 0;
            pv_status_t status = pv_language_num_bytes_character(PV_NORMALIZER_SPECIAL_CHARACTERS_KO[i][0], &num_bytes);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_language_num_bytes_character,
                        pv_status_to_string(status));
                return status;
            }
            if (strcmp(token->string, PV_NORMALIZER_SPECIAL_CHARACTERS_KO[i]) == 0) {
                break;
            }
        }
        const char *verbalized = PV_NORMALIZER_SPECIAL_CHARACTERS_VERBALIZED_KO[i];

        if (verbalized) {
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

        if (PV_NORMALIZER_SPECIAL_CHARACTERS_IS_VERBALIZED_TO_PUNCTUATION_KO[i]) {
            token->tag = PV_NORMALIZER_TAG_KO_PUNCTUATION;
        }
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_decimal(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_KO_DECIMAL_POINT) {
        const char *point = "점";
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
    } else if (token->tag == PV_NORMALIZER_TAG_KO_DECIMAL_DIGITS) {
        char *verbalized = NULL;
        pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(token->string, false, false, &verbalized);
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

static pv_status_t pv_normalizer_verbalizer_verbalize_measurement(
        pv_normalizer_token_list_t *token_list,
        pv_normalizer_token_t *token) {
    PV_ASSERT(token_list);
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_KO_MEASUREMENT) {
        pv_normalizer_token_t *prev = pv_normalizer_token_get_nth_token_before(token, 1, false);
        pv_normalizer_token_t *two_prev = pv_normalizer_token_get_nth_token_before(token, 2, false);
        pv_normalizer_token_t *next = pv_normalizer_token_get_nth_token_after(token, 1, false);

        bool is_prev_per_slash =
                (prev != NULL) &&
                (prev->tag == PV_NORMALIZER_TAG_KO_PER_SLASH);
        bool is_next_per_slash =
                (next != NULL) &&
                (next->tag == PV_NORMALIZER_TAG_KO_PER_SLASH);

        if (is_prev_per_slash) {
            bool is_two_prev_measurement =
                    (two_prev != NULL) &&
                    (two_prev->tag == PV_NORMALIZER_TAG_KO_MEASUREMENT);

            if (is_two_prev_measurement) {
                pv_normalizer_token_t *three_prev = pv_normalizer_token_get_nth_token_before(token, 3, true);
                pv_normalizer_token_t *four_prev = pv_normalizer_token_get_nth_token_before(token, 4, true);
                pv_normalizer_token_t *five_prev = pv_normalizer_token_get_nth_token_before(token, 5, true);

                bool is_three_prev_cardinal =
                        (three_prev != NULL) &&
                        (three_prev->tag == PV_NORMALIZER_TAG_KO_CARDINAL || three_prev->tag == PV_NORMALIZER_TAG_KO_NEGATIVE_CARDINAL);
                bool is_three_prev_decimal =
                        (three_prev != NULL) &&
                        (three_prev->tag == PV_NORMALIZER_TAG_KO_DECIMAL_DIGITS);

                const char *first_verbalized = PV_NORMALIZER_MEASUREMENTS_VERBALIZED_KO[token->tag_data_index];
                const char *second_verbalized = PV_NORMALIZER_MEASUREMENTS_VERBALIZED_KO[two_prev->tag_data_index];
                const char *dang = "당 ";
                char *number_string = "";

                if (is_three_prev_cardinal) {
                    number_string = three_prev->verbalized;
                } else if (is_three_prev_decimal) {
                    if (four_prev != NULL && five_prev != NULL) {
                        char *full_number = calloc(strlen(five_prev->verbalized) + strlen(four_prev->verbalized) + strlen(three_prev->verbalized) + 1, sizeof(char));
                        if (!full_number) {
                            PV_ERROR_REPORT(
                                    &pv_error_msg_alloc,
                                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                                    PV_ERROR_ARGS_PRIVATE("full_number"));
                            return PV_STATUS_OUT_OF_MEMORY;
                        }
                        strcpy(full_number, five_prev->verbalized);
                        strcat(full_number, four_prev->verbalized);
                        strcat(full_number, three_prev->verbalized);
                        number_string = full_number;
                    }
                }

                char *verbalized = calloc(strlen(first_verbalized) + strlen(dang) + strlen(number_string) + strlen(second_verbalized) + 1, sizeof(char));
                if (!verbalized) {
                    PV_ERROR_REPORT(
                            &pv_error_msg_alloc,
                            PV_ERROR_ARGS_PUBLIC_EMPTY(),
                            PV_ERROR_ARGS_PRIVATE("token->verbalized"));
                    if (is_three_prev_decimal) {
                        if (four_prev != NULL && five_prev != NULL) {
                            free(number_string);
                        }
                    }
                    return PV_STATUS_OUT_OF_MEMORY;
                }
                strcat(verbalized, first_verbalized);
                strcat(verbalized, dang);
                strcat(verbalized, number_string);
                strcat(verbalized, second_verbalized);

                pv_normalizer_token_t *first_token = NULL;

                if (is_three_prev_cardinal) {
                    first_token = three_prev;
                } else if (is_three_prev_decimal) {
                    if (four_prev != NULL && five_prev != NULL) {
                        free(number_string);
                    }
                    first_token = five_prev;
                } else {
                    first_token = two_prev;
                }

                char *original_string = NULL;
                if (three_prev->next_character_is_space) {
                    original_string = calloc(strlen(three_prev->original_string) + strlen(token->original_string) + 2, sizeof(char));
                    if (!original_string) {
                        PV_ERROR_REPORT(
                                &pv_error_msg_alloc,
                                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                                PV_ERROR_ARGS_PRIVATE("original_string"));
                        return PV_STATUS_OUT_OF_MEMORY;
                    }
                    strcpy(original_string, three_prev->original_string);
                    strcat(original_string, " ");
                    strcat(original_string, token->original_string);
                } else {
                    original_string = calloc(strlen(token->original_string) + 1, sizeof(char));
                    if (!original_string) {
                        PV_ERROR_REPORT(
                                &pv_error_msg_alloc,
                                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                                PV_ERROR_ARGS_PRIVATE("original_string"));
                        return PV_STATUS_OUT_OF_MEMORY;
                    }
                    strcpy(original_string, token->original_string);
                }

                pv_normalizer_token_t *collapsed_token = NULL;
                pv_status_t status = pv_normalizer_token_list_collapse_tokens(
                        token_list,
                        original_string,
                        first_token,
                        token,
                        &collapsed_token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_token_list_collapse_tokens,
                            pv_status_to_string(status));
                    return status;
                }
                free(token->original_string);

                token = collapsed_token;
                pv_normalizer_token_set_verbalized(token, verbalized);
                token->original_string = original_string;
            }
        } else if (!is_next_per_slash) {
            pv_normalizer_token_t *prev_no_space = pv_normalizer_token_get_nth_token_before(token, 1, true);

            bool is_prev_number =
                    (prev_no_space != NULL) &&
                    (prev_no_space->tag == PV_NORMALIZER_TAG_KO_CARDINAL ||
                     prev_no_space->tag == PV_NORMALIZER_TAG_KO_NEGATIVE_CARDINAL ||
                     prev_no_space->tag == PV_NORMALIZER_TAG_KO_DECIMAL_DIGITS);

            if (PV_NORMALIZER_MEASUREMENTS_IS_MULTIPLE_KO[token->tag_data_index] && is_prev_number) {
                char *measurement_verbalized = (char *) PV_NORMALIZER_MEASUREMENTS_VERBALIZED_KO[token->tag_data_index];
                char *first_part = NULL;
                char *second_part = NULL;

                pv_status_t status = pv_string_split(&measurement_verbalized, ' ', &first_part);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_string_split,
                            pv_status_to_string(status));
                    return status;
                }

                status = pv_string_split(&measurement_verbalized, ' ', &second_part);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_string_split,
                            pv_status_to_string(status));
                    free(first_part);
                    return status;
                }

                char *cardinal_number = "";
                char *cardinal_point = "";
                char *cardinal_decimal = "";

                pv_normalizer_token_t *two_previous = pv_normalizer_token_get_nth_token_before(token, 2, true);
                pv_normalizer_token_t *three_previous = pv_normalizer_token_get_nth_token_before(token, 3, true);

                if (prev_no_space->tag == PV_NORMALIZER_TAG_KO_DECIMAL_DIGITS) {
                    if (two_previous != NULL && three_previous != NULL) {
                        cardinal_number = three_previous->verbalized;
                        cardinal_point = two_previous->verbalized;
                        cardinal_decimal = prev_no_space->verbalized;
                    }
                } else {
                    cardinal_number = prev_no_space->verbalized;
                }

                char *verbalized = calloc(strlen(first_part) + strlen(second_part) + strlen(cardinal_number) +
                                                  strlen(cardinal_decimal) + strlen(cardinal_point) + 2,
                                          sizeof(char));
                if (!verbalized) {
                    PV_ERROR_REPORT(
                            &pv_error_msg_alloc,
                            PV_ERROR_ARGS_PUBLIC_EMPTY(),
                            PV_ERROR_ARGS_PRIVATE("token->verbalized"));
                    free((char *) verbalized);
                    return PV_STATUS_OUT_OF_MEMORY;
                }
                strcpy(verbalized, first_part);
                strcat(verbalized, " ");
                strcat(verbalized, cardinal_number);
                strcat(verbalized, cardinal_point);
                strcat(verbalized, cardinal_decimal);
                strcat(verbalized, second_part);

                free(first_part);
                free(second_part);

                pv_normalizer_token_t *first_token = NULL;

                if (prev_no_space->tag == PV_NORMALIZER_TAG_KO_DECIMAL_DIGITS) {
                    if (two_previous != NULL && three_previous != NULL) {
                        first_token = three_previous;
                    }
                } else {
                    first_token = prev_no_space;
                }

                char *original_string = calloc(strlen(token->original_string) + 1, sizeof(char));
                if (!original_string) {
                    PV_ERROR_REPORT(
                            &pv_error_msg_alloc,
                            PV_ERROR_ARGS_PUBLIC_EMPTY(),
                            PV_ERROR_ARGS_PRIVATE("original_string"));
                    return PV_STATUS_OUT_OF_MEMORY;
                }
                strcpy(original_string, token->original_string);

                pv_normalizer_token_t *collapsed_token = NULL;
                status = pv_normalizer_token_list_collapse_tokens(
                        token_list,
                        original_string,
                        first_token,
                        token,
                        &collapsed_token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_token_list_collapse_tokens,
                            pv_status_to_string(status));
                    return status;
                }
                free(token->original_string);

                token = collapsed_token;
                pv_normalizer_token_set_verbalized(token, verbalized);
                token->original_string = original_string;
            } else {
                const char *verbalized = PV_NORMALIZER_MEASUREMENTS_VERBALIZED_KO[token->tag_data_index];

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
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_cardinal_spell_out_helper(
        const char *digit_string,
        bool dry_run,
        int32_t *length,
        char *verbalized) {
    PV_ASSERT(digit_string);
    PV_ASSERT(!dry_run || length);

    if (dry_run) {
        PV_ASSERT(length);
        *length = 0;
    } else {
        PV_ASSERT(verbalized);
    }

    int32_t length_internal = 0;

    char character[2] = {0};

    int32_t verbalized_index = 0;
    for (int32_t i = 0; i < (int32_t) strlen(digit_string); i++) {
        character[0] = digit_string[i];
        character[1] = '\0';

        if (isdigit(digit_string[i])) {
            char *verbalized_internal = NULL;
            pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(character, false, false, &verbalized_internal);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_verbalizer_cardinal_to_string,
                        pv_status_to_string(status));
                return status;
            }

            if (dry_run) {
                length_internal += (int32_t) strlen(verbalized_internal);
            } else {
                strcpy(verbalized + verbalized_index, verbalized_internal);
                verbalized_index += (int32_t) strlen(verbalized_internal);
            }
            free(verbalized_internal);
        }

        if (i < (int32_t) strlen(digit_string) - 1) {
            if (dry_run) {
                length_internal++;
            } else {
                verbalized[verbalized_index++] = ' ';
            }
        }
    }

    if (dry_run) {
        *length = length_internal;
    }

    return PV_STATUS_SUCCESS;
}

static pv_status_t pv_normalizer_verbalizer_cardinal_spell_out(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    int32_t length = 0;
    pv_status_t status = pv_normalizer_verbalizer_cardinal_spell_out_helper(token->string, true, &length, NULL);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_verbalizer_cardinal_spell_out_helper,
                pv_status_to_string(status));
        return status;
    }

    char *verbalized = calloc(length + 1, sizeof(char));
    if (!verbalized) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("verbalized"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    status = pv_normalizer_verbalizer_cardinal_spell_out_helper(token->string, false, NULL, verbalized);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_verbalizer_cardinal_spell_out_helper,
                pv_status_to_string(status));
        free(verbalized);
        return status;
    }
    pv_normalizer_token_set_verbalized(token, verbalized);

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_alphanum_spell_out(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_KO_CARDINAL_SPELL_OUT) {
        pv_status_t status = pv_normalizer_verbalizer_cardinal_spell_out(token);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_cardinal_spell_out,
                    pv_status_to_string(status));
            return status;
        }
    } else if (token->tag == PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT) {
        char *upper_letter = calloc(strlen(token->string) + 1, sizeof(char));
        if (!upper_letter) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("upper_letter"));
            return PV_STATUS_OUT_OF_MEMORY;
        }
        pv_language_utf8_str_to_upper((uint8_t *) token->string, (uint8_t *) upper_letter);

        const char *letter_in_hangul = NULL;
        for (int32_t i = 0; i < PV_ARRAY_LEN(PV_NORMALIZER_ALPHABET_KO); i++) {
            if (strcmp(upper_letter, PV_NORMALIZER_ALPHABET_KO[i]) == 0) {
                letter_in_hangul = PV_NORMALIZER_ALPHABET_WORD_KO[i];
                break;
            }
        }
        free(upper_letter);

        if (!letter_in_hangul) {
            return PV_STATUS_SUCCESS;
        }

        char *token_verbalized = calloc(strlen(letter_in_hangul) + 1, sizeof(char));
        if (!token_verbalized) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("token_verbalized"));
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(token_verbalized, letter_in_hangul);
        pv_normalizer_token_set_verbalized(token, token_verbalized);
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_time(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_KO_TIME_COLON) {
        pv_normalizer_token_set_verbalized(token, NULL);
    } else if (token->tag == PV_NORMALIZER_TAG_KO_TIME_HOURS) {
        const char *number_string = token->string;
        bool use_native = true;

        int32_t offset = 0;
        if (number_string[0] == '0') {
            offset = 1;
        } else if (number_string[0] == '1' && (number_string[1] > '2')) {
            use_native = false;
        }

        char *o_clock_string = "시";
        char *hour_string = NULL;
        char *am_pm_string = "";

        pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(
                token->string + offset,
                false,
                use_native,
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
                (next != NULL && (next->tag == PV_NORMALIZER_TAG_KO_TIME_AM_PM)) ||
                ((three_next != NULL && (three_next->tag == PV_NORMALIZER_TAG_KO_TIME_AM_PM)) &&
                 (two_next != NULL && (two_next->tag == PV_NORMALIZER_TAG_KO_TIME_MINUTES)) &&
                 (next != NULL && (next->tag == PV_NORMALIZER_TAG_KO_TIME_COLON)));

        if (is_am_pm_hours) {
            char *orig_am_pm = NULL;
            if (next != NULL && next->tag == PV_NORMALIZER_TAG_KO_TIME_AM_PM) {
                orig_am_pm = next->string;
            } else if (three_next != NULL) {
                orig_am_pm = three_next->string;
            }

            if (orig_am_pm != NULL) {
                if ((orig_am_pm[0] == 'a') || (orig_am_pm[0] == 'A')) {
                    am_pm_string = "오전 ";
                } else {
                    am_pm_string = "오후 ";
                }
            }
        }

        char *token_verbalized = calloc(strlen(am_pm_string) + strlen(hour_string) + strlen(o_clock_string) + 1, sizeof(char));
        if (!token_verbalized) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("token_verbalized"));
            free(hour_string);
            return PV_STATUS_OUT_OF_MEMORY;
        }

        strcpy(token_verbalized, am_pm_string);
        strcat(token_verbalized, hour_string);
        strcat(token_verbalized, o_clock_string);

        free(hour_string);
        pv_normalizer_token_set_verbalized(token, token_verbalized);
    } else if (token->tag == PV_NORMALIZER_TAG_KO_TIME_MINUTES) {
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
                false,
                &verbalized);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_cardinal_to_string,
                    pv_status_to_string(status));
            return status;
        }

        char *minute_string = "분";
        char *token_verbalized = calloc(strlen(verbalized) + strlen(minute_string) + 1, sizeof(char));
        if (!token_verbalized) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("token_verbalized"));
            free(verbalized);
            return PV_STATUS_OUT_OF_MEMORY;
        }

        strcpy(token_verbalized, verbalized);
        strcat(token_verbalized, minute_string);
        free(verbalized);
        pv_normalizer_token_set_verbalized(token, token_verbalized);
    } else if (token->tag == PV_NORMALIZER_TAG_KO_TIME_AM_PM) {
        pv_normalizer_token_t *prev = pv_normalizer_token_get_nth_token_before(token, 1, true);
        pv_normalizer_token_t *two_prev = pv_normalizer_token_get_nth_token_before(token, 2, true);
        pv_normalizer_token_t *three_prev = pv_normalizer_token_get_nth_token_before(token, 3, true);

        bool is_prev_hour =
                (prev != NULL) &&
                (prev->tag == PV_NORMALIZER_TAG_KO_TIME_HOURS);
        bool is_prev_time =
                (prev != NULL && prev->tag == PV_NORMALIZER_TAG_KO_TIME_MINUTES) &&
                (two_prev != NULL && two_prev->tag == PV_NORMALIZER_TAG_KO_TIME_COLON) &&
                (three_prev != NULL && three_prev->tag == PV_NORMALIZER_TAG_KO_TIME_HOURS);

        if (!(is_prev_hour || is_prev_time)) {
            char *am_pm_string = NULL;
            if ((token->string[0] == 'a') || (token->string[0] == 'A')) {
                am_pm_string = "오전";
            } else {
                am_pm_string = "오후";
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

    if (token->tag == PV_NORMALIZER_TAG_KO_FRACTION_SLASH) {
        if ((token->next != NULL) &&
            ((token->next->tag == PV_NORMALIZER_TAG_KO_CARDINAL) ||
             (token->next->tag == PV_NORMALIZER_TAG_KO_NEGATIVE_CARDINAL))) {
            const char *over = "분의";

            char *token_verbalized = calloc(strlen(over) + 1, sizeof(char));
            if (!token_verbalized) {
                PV_ERROR_REPORT(
                        &pv_error_msg_alloc,
                        PV_ERROR_ARGS_PUBLIC_EMPTY(),
                        PV_ERROR_ARGS_PRIVATE("token->verbalized"));
                return PV_STATUS_OUT_OF_MEMORY;
            }
            strcpy(token_verbalized, over);
            pv_normalizer_token_set_verbalized(token, token_verbalized);
        }
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_fraction_denominator(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    pv_normalizer_token_t *previous = pv_normalizer_token_get_nth_token_before(token, 1, false);
    pv_normalizer_token_t *three_previous = pv_normalizer_token_get_nth_token_before(token, 3, false);

    bool is_previous_fraction_slash =
            (previous != NULL) &&
            (previous->tag == PV_NORMALIZER_TAG_KO_FRACTION_SLASH);
    bool is_three_previous_fraction_slash =
            (three_previous != NULL) &&
            (three_previous->tag == PV_NORMALIZER_TAG_KO_FRACTION_SLASH);

    if (!(is_previous_fraction_slash || is_three_previous_fraction_slash)) {
        return PV_STATUS_SUCCESS;
    }

    pv_normalizer_token_t *next = pv_normalizer_token_get_nth_token_after(token, 1, false);
    bool is_next_decimal_point =
            (next != NULL) &&
            (next->tag == PV_NORMALIZER_TAG_KO_DECIMAL_POINT);


    pv_normalizer_token_t *two_previous = pv_normalizer_token_get_nth_token_before(token, 2, false);
    pv_normalizer_token_t *four_previous = pv_normalizer_token_get_nth_token_before(token, 4, false);

    if ((token->tag == PV_NORMALIZER_TAG_KO_CARDINAL || token->tag == PV_NORMALIZER_TAG_KO_NEGATIVE_CARDINAL) && !is_next_decimal_point) {
        // denominator is a whole number
        bool is_two_previous_cardinal =
                (two_previous != NULL) &&
                (two_previous->tag != PV_NORMALIZER_TAG_KO_DECIMAL_DIGITS);

        char *denominator_cardinal = token->verbalized;

        char *verbalized = NULL;
        if (is_two_previous_cardinal) {
            // numerator is a whole number
            char *fraction_cardinal = two_previous->verbalized;
            char *fraction_slash = previous->verbalized;

            verbalized = calloc(strlen(fraction_cardinal) + strlen(fraction_slash) + strlen(denominator_cardinal) + 2, sizeof(char));
            if (!verbalized) {
                PV_ERROR_REPORT(
                        &pv_error_msg_alloc,
                        PV_ERROR_ARGS_PUBLIC_EMPTY(),
                        PV_ERROR_ARGS_PRIVATE("verbalized"));
                return PV_STATUS_OUT_OF_MEMORY;
            }

            strcpy(verbalized, denominator_cardinal);
            strcat(verbalized, fraction_slash);
            strcat(verbalized, " ");
            strcat(verbalized, fraction_cardinal);

            pv_normalizer_token_set_verbalized(two_previous, NULL);
            pv_normalizer_token_set_verbalized(previous, NULL);
        } else {
            // numerator is a decimal number
            if ((four_previous != NULL)) {
                char *fraction_cardinal = four_previous->verbalized;
                char *fraction_point = three_previous->verbalized;
                char *fraction_decimal = two_previous->verbalized;
                char *fraction_slash = previous->verbalized;

                verbalized = calloc(strlen(fraction_cardinal) + strlen(fraction_point) + strlen(fraction_decimal) +
                                            strlen(fraction_slash) + strlen(denominator_cardinal) + 2,
                                    sizeof(char));
                if (!verbalized) {
                    PV_ERROR_REPORT(
                            &pv_error_msg_alloc,
                            PV_ERROR_ARGS_PUBLIC_EMPTY(),
                            PV_ERROR_ARGS_PRIVATE("verbalized"));
                    return PV_STATUS_OUT_OF_MEMORY;
                }

                strcpy(verbalized, denominator_cardinal);
                strcat(verbalized, fraction_slash);
                strcat(verbalized, " ");
                strcat(verbalized, fraction_cardinal);
                strcat(verbalized, fraction_point);
                strcat(verbalized, fraction_decimal);

                pv_normalizer_token_set_verbalized(four_previous, NULL);
                pv_normalizer_token_set_verbalized(three_previous, NULL);
                pv_normalizer_token_set_verbalized(two_previous, NULL);
                pv_normalizer_token_set_verbalized(previous, NULL);
            }
        }

        pv_normalizer_token_set_verbalized(token, verbalized);
    } else if (token->tag == PV_NORMALIZER_TAG_KO_DECIMAL_DIGITS) {
        // denominator is a decimal number
        bool is_four_previous_cardinal =
                (four_previous != NULL) &&
                (four_previous->tag != PV_NORMALIZER_TAG_KO_DECIMAL_DIGITS);

        char *denominator_cardinal = two_previous->verbalized;
        char *denominator_point = previous->verbalized;
        char *denominator_decimal = token->verbalized;

        char *verbalized = NULL;
        if (is_four_previous_cardinal) {
            // numerator is a whole number
            char *fraction_cardinal = four_previous->verbalized;
            char *fraction_slash = three_previous->verbalized;

            verbalized = calloc(strlen(fraction_cardinal) + strlen(fraction_slash) +
                                        strlen(denominator_cardinal) + strlen(denominator_point) + strlen(denominator_decimal) + 2,
                                sizeof(char));
            if (!verbalized) {
                PV_ERROR_REPORT(
                        &pv_error_msg_alloc,
                        PV_ERROR_ARGS_PUBLIC_EMPTY(),
                        PV_ERROR_ARGS_PRIVATE("verbalized"));
                return PV_STATUS_OUT_OF_MEMORY;
            }

            strcpy(verbalized, denominator_cardinal);
            strcat(verbalized, denominator_point);
            strcat(verbalized, denominator_decimal);
            strcat(verbalized, fraction_slash);
            strcat(verbalized, " ");
            strcat(verbalized, fraction_cardinal);

            pv_normalizer_token_set_verbalized(four_previous, NULL);
            pv_normalizer_token_set_verbalized(three_previous, NULL);
        } else {
            pv_normalizer_token_t *five_previous = pv_normalizer_token_get_nth_token_before(token, 5, false);
            pv_normalizer_token_t *six_previous = pv_normalizer_token_get_nth_token_before(token, 6, false);

            // numerator is a decimal number
            if ((five_previous != NULL) && (six_previous != NULL)) {
                char *fraction_cardinal = six_previous->verbalized;
                char *fraction_point = five_previous->verbalized;
                char *fraction_decimal = four_previous->verbalized;
                char *fraction_slash = three_previous->verbalized;

                verbalized = calloc(strlen(fraction_cardinal) + strlen(fraction_point) + strlen(fraction_decimal) + strlen(fraction_slash) +
                                            strlen(denominator_cardinal) + strlen(denominator_point) + strlen(denominator_decimal) + 2,
                                    sizeof(char));
                if (!verbalized) {
                    PV_ERROR_REPORT(
                            &pv_error_msg_alloc,
                            PV_ERROR_ARGS_PUBLIC_EMPTY(),
                            PV_ERROR_ARGS_PRIVATE("verbalized"));
                    return PV_STATUS_OUT_OF_MEMORY;
                }

                strcpy(verbalized, denominator_cardinal);
                strcat(verbalized, denominator_point);
                strcat(verbalized, denominator_decimal);
                strcat(verbalized, fraction_slash);
                strcat(verbalized, " ");
                strcat(verbalized, fraction_cardinal);
                strcat(verbalized, fraction_point);
                strcat(verbalized, fraction_decimal);

                pv_normalizer_token_set_verbalized(six_previous, NULL);
                pv_normalizer_token_set_verbalized(five_previous, NULL);
                pv_normalizer_token_set_verbalized(four_previous, NULL);
                pv_normalizer_token_set_verbalized(three_previous, NULL);
            }
        }

        pv_normalizer_token_set_verbalized(two_previous, NULL);
        pv_normalizer_token_set_verbalized(previous, NULL);
        pv_normalizer_token_set_verbalized(token, verbalized);
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_dot(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_KO_DOT) {
        pv_normalizer_token_t *previous = pv_normalizer_token_get_nth_token_before(token, 1, true);
        if ((previous != NULL) && (previous->tag == PV_NORMALIZER_TAG_KO_MEASUREMENT)) {
            return PV_STATUS_SUCCESS;
        }

        const char *dot = "점";

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

    if (token->tag == PV_NORMALIZER_TAG_KO_COLON) {
        const char *colon = "쌍점";

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

    if (token->tag == PV_NORMALIZER_TAG_KO_TOP_LEVEL_DOMAIN) {
        PV_ASSERT(!PV_NORMALIZER_TOP_LEVEL_DOMAINS_IS_SPELL_OUT_KO[token->tag_data_index]);
        const char *domain_string = PV_NORMALIZER_TOP_LEVEL_DOMAINS_WORD_KO[token->tag_data_index];

        char *token_verbalized = calloc(strlen(domain_string) + 1, sizeof(char));
        if (!token_verbalized) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("token_verbalized"));
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(token_verbalized, domain_string);
        pv_normalizer_token_set_verbalized(token, token_verbalized);
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_currency(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_KO_CURRENCY) {
        char *string = token->string;
        int32_t length = (int32_t) strlen(string);

        char *space = " ";

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
                false,
                &verbalized_number_string);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_cardinal_to_string,
                    pv_status_to_string(status));
            free(number_string);
            return status;
        }

        const char *currency_string = PV_NORMALIZER_CURRENCIES_VERBALIZED_KO[token->tag_data_index];
        free(number_string);

        bool has_no_decimal = ((number_end_index == length) || (string[number_end_index] != '.'));
        if (has_no_decimal) {
            int32_t verbalized_length =
                    (int32_t) strlen(verbalized_number_string) +
                    (int32_t) strlen(space) +
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
            strcat(token_verbalized, space);
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

        // .00 is not verbalized e.g $1.00 -> one dollar
        if ((string[digit_start_index] == '0') && (string[digit_start_index + 1] == '0')) {
            int32_t verbalized_length =
                    (int32_t) strlen(verbalized_number_string) +
                    (int32_t) strlen(space) +
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
            strcat(token_verbalized, space);
            strcat(token_verbalized, currency_string);
            free(verbalized_number_string);
            pv_normalizer_token_set_verbalized(token, token_verbalized);

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

        const char *sub_currency_string = PV_NORMALIZER_SUB_CURRENCIES_VERBALIZED_KO[token->tag_data_index];
        free(digit_string);

        int32_t verbalized_length =
                (int32_t) strlen(verbalized_number_string) +
                (int32_t) strlen(space) +
                (int32_t) strlen(currency_string) +
                (int32_t) strlen(space) +
                (int32_t) strlen(verbalized_digit_string) +
                (int32_t) strlen(space) +
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
        strcat(token_verbalized, space);
        strcat(token_verbalized, currency_string);
        strcat(token_verbalized, space);
        strcat(token_verbalized, verbalized_digit_string);
        strcat(token_verbalized, space);
        strcat(token_verbalized, sub_currency_string);

        free(verbalized_number_string);
        free(verbalized_digit_string);
        pv_normalizer_token_set_verbalized(token, token_verbalized);
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_negative_currency(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_KO_NEGATIVE_CURRENCY) {
        char *string = token->string;
        int32_t length = (int32_t) strlen(string);

        char *negative = "마이너스 ";
        char *space = " ";

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
                false,
                &verbalized_number_string);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_cardinal_to_string,
                    pv_status_to_string(status));
            free(number_string);
            return status;
        }

        const char *currency_string = PV_NORMALIZER_CURRENCIES_VERBALIZED_KO[token->tag_data_index];
        free(number_string);

        bool has_no_decimal = ((number_end_index == length) || (string[number_end_index] != '.'));
        if (has_no_decimal) {
            int32_t verbalized_length =
                    (int32_t) strlen(negative) +
                    (int32_t) strlen(verbalized_number_string) +
                    (int32_t) strlen(space) +
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
            strcat(token_verbalized, space);
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

        const char *sub_currency_string = PV_NORMALIZER_SUB_CURRENCIES_VERBALIZED_KO[token->tag_data_index];
        free(digit_string);

        int32_t verbalized_length =
                (int32_t) strlen(negative) +
                (int32_t) strlen(verbalized_number_string) +
                (int32_t) strlen(space) +
                (int32_t) strlen(currency_string) +
                (int32_t) strlen(space) +
                (int32_t) strlen(verbalized_digit_string) +
                (int32_t) strlen(space) +
                (int32_t) strlen(sub_currency_string);

        char *token_verbalized = calloc(verbalized_length + 1, sizeof(char));
        if (!token_verbalized) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("token_verbalized"));
            free(verbalized_digit_string);
            free(verbalized_number_string);
            return PV_STATUS_OUT_OF_MEMORY;
        }

        strcpy(token_verbalized, negative);
        strcat(token_verbalized, verbalized_number_string);
        strcat(token_verbalized, space);
        strcat(token_verbalized, currency_string);
        strcat(token_verbalized, space);
        strcat(token_verbalized, verbalized_digit_string);
        strcat(token_verbalized, space);
        strcat(token_verbalized, sub_currency_string);

        free(verbalized_number_string);
        free(verbalized_digit_string);
        pv_normalizer_token_set_verbalized(token, token_verbalized);
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_currency_symbol(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_KO_CURRENCY_SYMBOL) {
        pv_normalizer_token_t *previous = pv_normalizer_token_get_nth_token_before(token, 1, true);
        if ((previous == NULL) || (token->tag_data_index < 0)) {
            return PV_STATUS_SUCCESS;
        }
        const char *currency_string = PV_NORMALIZER_CURRENCIES_VERBALIZED_KO[token->tag_data_index];

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

pv_status_t pv_normalizer_verbalizer_verbalize_digits_sequence(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    const char *comma_string = ",";

    const char *string = token->string;
    int32_t length = (int32_t) strlen(string);

    if (token->tag == PV_NORMALIZER_TAG_KO_DIGITS_WITH_PARENTHESES) {
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
        pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(number_string, true, false, &verbalized);
        free(number_string);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_cardinal_to_string,
                    pv_status_to_string(status));
            return status;
        }

        const char *comma_string_with_space = ", ";
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
    } else if (token->tag == PV_NORMALIZER_TAG_KO_DIGITS) {
        char *verbalized = NULL;
        pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(
                string,
                true,
                false,
                &verbalized);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_cardinal_to_string,
                    pv_status_to_string(status));
            return status;
        }
        pv_normalizer_token_set_verbalized(token, verbalized);
    } else if (token->tag == PV_NORMALIZER_TAG_KO_DIGITS_SEPARATOR) {
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
        token->tag = PV_NORMALIZER_TAG_KO_PUNCTUATION;
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_dates(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_KO_DATE_SEPARATOR) {
        pv_normalizer_token_set_verbalized(token, NULL);
    } else if (token->tag == PV_NORMALIZER_TAG_KO_DATE_DAY) {
        int32_t offset = (token->string[0] == '0') ? 1 : 0;
        PV_ASSERT(strcmp(token->string + offset, "\0") != 0);
        char *day_string = token->string + offset;

        char *day_verbalized = NULL;
        pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(day_string, false, false, &day_verbalized);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_cardinal_to_string,
                    pv_status_to_string(status));
            return status;
        }

        char *verbalized_string = "일";
        char *token_verbalized = calloc(strlen(day_verbalized) + strlen(verbalized_string) + 1, sizeof(char));
        if (!token_verbalized) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("token_verbalized"));
            free(day_verbalized);
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(token_verbalized, day_verbalized);
        strcat(token_verbalized, verbalized_string);

        free(day_verbalized);
        pv_normalizer_token_set_verbalized(token, token_verbalized);
    } else if (token->tag == PV_NORMALIZER_TAG_KO_DATE_MONTH) {
        int32_t offset = (token->string[0] == '0') ? 1 : 0;
        PV_ASSERT(strcmp(token->string + offset, "\0") != 0);
        char *month_string = token->string + offset;

        char *month_verbalized = NULL;
        pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(month_string, false, false, &month_verbalized);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_cardinal_to_string,
                    pv_status_to_string(status));
            return status;
        }

        const char *verbalized_string = "월";

        char *token_verbalized = calloc(strlen(month_verbalized) + strlen(verbalized_string) + 1, sizeof(char));
        if (!token_verbalized) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("token_verbalized"));
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(token_verbalized, month_verbalized);
        strcat(token_verbalized, verbalized_string);

        free(month_verbalized);
        pv_normalizer_token_set_verbalized(token, token_verbalized);
    } else if (token->tag == PV_NORMALIZER_TAG_KO_DATE_YEAR) {
        char *year_string = token->string;

        char *year_verbalized = NULL;
        pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(year_string, false, false, &year_verbalized);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_cardinal_to_string,
                    pv_status_to_string(status));
            return status;
        }

        const char *verbalized_string = "년";

        char *token_verbalized = calloc(strlen(year_verbalized) + strlen(verbalized_string) + 1, sizeof(char));
        if (!token_verbalized) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("token_verbalized"));
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(token_verbalized, year_verbalized);
        strcat(token_verbalized, verbalized_string);

        free(year_verbalized);
        pv_normalizer_token_set_verbalized(token, token_verbalized);
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_names(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_KO_NAME_INITIAL_DOT) {
        pv_normalizer_token_set_verbalized(token, NULL);
    }

    return PV_STATUS_SUCCESS;
}
