#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "core/pv_assert.h"
#include "core/pv_error_messages.h"
#include "core/pv_language.h"
#include "orca/normalizer/pv_normalizer_token.h"
#include "orca/normalizer/pv_normalizer_use_cases.h"
#include "orca/normalizer/pv_normalizer_util.h"
#include "orca/normalizer/pv_normalizer_verbalizer.h"

#include "orca/normalizer/en/pv_normalizer_data_en/pv_normalizer_data_en.h"
#include "orca/normalizer/en/pv_normalizer_tags_en.h"
#include "orca/normalizer/en/pv_normalizer_verbalizer_en.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_normalizer_mock.h"

#endif

struct pv_normalizer_verbalizer_en {
    int32_t num_use_cases;
    const pv_normalizer_use_cases_en_t *use_cases;
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
        char **string);

static pv_status_t pv_normalizer_verbalizer_cardinal_to_string(
        const char *number_string,
        bool to_digit_string,
        char **string);

static pv_status_t pv_normalizer_verbalizer_verbalize_custom_pronunciation(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_number_range(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_ordinal(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_ordinal_to_string(const char *ordinal_string, char **string);

static pv_status_t pv_normalizer_verbalizer_verbalize_negative_ordinal(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_special_character(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_decimal(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_measurement(pv_normalizer_token_t *token);

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

static pv_status_t pv_normalizer_verbalizer_verbalize_abbreviation(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_digits_sequence(pv_normalizer_token_t *token);

static bool pv_normalizer_verbalizer_is_year_first(pv_normalizer_token_t *token, bool check_previous);

static pv_status_t pv_normalizer_verbalizer_verbalize_year(pv_normalizer_token_t *token, char **verbalized);

static pv_status_t pv_normalizer_verbalizer_verbalize_dates(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_names(pv_normalizer_token_t *token);

pv_status_t PV_MOCKABLE(pv_normalizer_verbalizer_en_init)(
        int32_t num_use_cases,
        const pv_normalizer_use_cases_en_t *use_cases,
        pv_normalizer_verbalizer_en_t **object) {
    PV_ASSERT(use_cases);
    PV_ASSERT(object);

    *object = NULL;

    pv_normalizer_verbalizer_en_t *o = calloc(1, sizeof(pv_normalizer_verbalizer_en_t));
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

void PV_MOCKABLE(pv_normalizer_verbalizer_en_delete)(pv_normalizer_verbalizer_en_t *object) {
    if (object) {
        free(object);
    }
}

pv_status_t PV_MOCKABLE(pv_normalizer_verbalizer_en_verbalize)(
        pv_normalizer_verbalizer_en_t *object,
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
            if (object->use_cases[j] == PV_NORMALIZER_USE_WORD_NORMALIZER) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_word(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_word,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_PUNCTUATION_NORMALIZER) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_punctuation(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_punctuation,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_CARDINAL_NORMALIZER) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_cardinal(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_cardinal,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_negative_cardinal(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_negative_cardinal,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_CUSTOM_PRONUNCIATION_NORMALIZER) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_custom_pronunciation(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_custom_pronunciation,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_NUMBER_RANGE_NORMALIZER) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_number_range(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_number_range,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_ORDINAL_NORMALIZER) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_ordinal(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_ordinal,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_NEGATIVE_ORDINAL_NORMALIZER) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_negative_ordinal(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_negative_ordinal,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_SPECIAL_CHARACTER_NORMALIZER) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_special_character(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_special_character,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_DECIMAL_NORMALIZER) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_decimal(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_decimal,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_MEASUREMENT_NORMALIZER) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_measurement(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_measurement,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_ALPHANUM_SPELL_OUT_NORMALIZER) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_alphanum_spell_out(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_alphanum_spell_out,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_TIME_NORMALIZER) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_time(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_time,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_FRACTION_NORMALIZER) {
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
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_URL_NORMALIZER) {
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
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_CURRENCY_NORMALIZER) {
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
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_ABBREVIATION_NORMALIZER) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_abbreviation(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_abbreviation,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_DIGITS_SEQUENCE_NORMALIZER) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_digits_sequence(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_digits_sequence,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_DATE_NORMALIZER) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_dates(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_dates,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_NAME_NORMALIZER) {
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
            PV_NORMALIZER_TAG_EN_SPACE,
            PV_NORMALIZER_TAG_EN_PUNCTUATION,
            PV_NORMALIZER_TAG_EN_SINGLE_QUOTE,
            PV_NORMALIZER_TAG_EN_CUSTOM_PRONUNCIATION,
            PV_NORMALIZER_TAG_EN_LETTER_SPELL_OUT);
}

pv_status_t pv_normalizer_verbalizer_verbalize_word(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_EN_WORD) {
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

    if (token->tag == PV_NORMALIZER_TAG_EN_PUNCTUATION) {
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

    if (token->tag == PV_NORMALIZER_TAG_EN_SINGLE_QUOTE) {
        const char *verbalized = "\"";
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
        int32_t *length,
        bool to_digit_string,
        bool dry_run,
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
        pv_normalizer_util_string_number_greater_than_int(number_string, MAX_VERBALIZED_CARDINAL)) {
        while (*number_string != '\0') {
            int32_t digit = (int32_t) *number_string - '0';
            if (digit == 0) {
                cardinal_to_string_helper(result, "ZERO", length, dry_run);
            } else {
                cardinal_to_string_helper(result, ONE_TO_NINETEEN[(int32_t) digit], length, dry_run);
            }
            cardinal_to_string_helper(result, " ", length, dry_run);
            number_string++;
        }
    } else {
        int64_t number = (int64_t) strtoll(number_string, NULL, 10);
        if (number < 0) {
            return PV_STATUS_SUCCESS;
        }

        if (number < 20) {
            cardinal_to_string_helper(result, ONE_TO_NINETEEN[number], length, dry_run);
            *string = result;

            return PV_STATUS_SUCCESS;
        }

        int64_t current_multiplier = MAX_MULTIPLIER;
        int64_t current_hundred = 0;
        int32_t current_multiplier_i = 0;
        int64_t i = number;
        while (i > 0) {
            current_hundred = i / current_multiplier;

            while (current_hundred == 0) {
                i %= current_multiplier;
                current_multiplier /= 1000;
                current_hundred = i / current_multiplier;
                current_multiplier_i++;
            }

            if (current_hundred > 99) {
                cardinal_to_string_helper(
                        result,
                        ONE_TO_NINETEEN[(int32_t) ((int32_t) current_hundred / 100)],
                        length,
                        dry_run);
                cardinal_to_string_helper(result, " HUNDRED ", length, dry_run);
            }

            current_hundred %= 100;

            if (current_hundred > 0 && current_hundred < 20) {
                cardinal_to_string_helper(result, ONE_TO_NINETEEN[(int32_t) current_hundred], length, dry_run);
                cardinal_to_string_helper(result, " ", length, dry_run);
            } else if (current_hundred % 10 == 0 && current_hundred != 0) {
                cardinal_to_string_helper(
                        result,
                        TENS[((int32_t) (int32_t) current_hundred / 10) - 1],
                        length,
                        dry_run);
                cardinal_to_string_helper(result, " ", length, dry_run);
            } else if (current_hundred > 20 && current_hundred < 100) {
                cardinal_to_string_helper(
                        result,
                        TENS[((int32_t) (int32_t) current_hundred / 10) - 1],
                        length,
                        dry_run);
                cardinal_to_string_helper(result, " ", length, dry_run);
                cardinal_to_string_helper(result, ONE_TO_NINETEEN[(int32_t) current_hundred % 10], length, dry_run);
                cardinal_to_string_helper(result, " ", length, dry_run);
            }

            if (current_multiplier_i < 4) {
                current_multiplier_i++;
                cardinal_to_string_helper(result, MULTIPLIERS[current_multiplier_i], length, dry_run);
                cardinal_to_string_helper(result, " ", length, dry_run);
            }

            i %= current_multiplier;
            current_multiplier /= 1000;
        }
    }

    if (!dry_run) {
        result[*length - 1] = '\0';
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
            &length,
            to_digit_string,
            true,
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
            string);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_verbalizer_cardinal_to_string_helper,
                pv_status_to_string(status));
        return status;
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_cardinal(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    bool is_ordinal_denominator =
            ((token->previous != NULL) &&
             (token->previous->tag == PV_NORMALIZER_TAG_EN_FRACTION_SLASH) &&
             (token->previous->verbalized == NULL));

    if ((token->tag == PV_NORMALIZER_TAG_EN_CARDINAL) && !is_ordinal_denominator) {
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

    if (token->tag == PV_NORMALIZER_TAG_EN_NEGATIVE_CARDINAL) {
        char *verbalized = NULL;
        const char *digit_string = token->string + 1;
        pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(digit_string, false, &verbalized);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_cardinal_to_string,
                    pv_status_to_string(status));
            return status;
        }

        const char *negative = "NEGATIVE ";
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

    if (token->tag == PV_NORMALIZER_TAG_EN_CUSTOM_PRONUNCIATION) {
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

    if (token->tag == PV_NORMALIZER_TAG_EN_NUMBER_RANGE_TO) {
        char *token_verbalized = calloc(strlen(NUMBER_RANGE_INBETWEEN_STRING) + 1, sizeof(char));
        if (!token_verbalized) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("token_verbalized"));
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(token_verbalized, NUMBER_RANGE_INBETWEEN_STRING);
        pv_normalizer_token_set_verbalized(token, token_verbalized);
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_ordinal(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_EN_ORDINAL) {
        char *verbalized = NULL;
        pv_status_t status = pv_normalizer_verbalizer_ordinal_to_string(
                token->string,
                &verbalized);
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

pv_status_t pv_normalizer_verbalizer_ordinal_to_string(const char *ordinal_string, char **string) {
    PV_ASSERT(ordinal_string);
    PV_ASSERT(string);

    *string = NULL;

    int32_t ordinal_string_length = (int32_t) strlen(ordinal_string);
    int32_t offset = 0;
    if (isalpha(ordinal_string[ordinal_string_length - 1])) {
        offset++;
    }
    if (ordinal_string_length > 1) {
        if (isalpha(ordinal_string[ordinal_string_length - 2])) {
            offset++;
        }
    }

    char *number_string = calloc(ordinal_string_length - offset + 1, sizeof(char));
    if (!number_string) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("number_string"));
        return PV_STATUS_OUT_OF_MEMORY;
    }
    memcpy(number_string, ordinal_string, (ordinal_string_length - offset) * sizeof(char));

    if (((*number_string == '0') && (strlen(number_string) > 1)) ||
        pv_normalizer_util_string_number_greater_than_int(number_string, MAX_VERBALIZED_CARDINAL)) {
        free(number_string);
        return PV_STATUS_SUCCESS;
    }

    char *cardinal_string = NULL;

    pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(number_string, false, &cardinal_string);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_verbalizer_cardinal_to_string_helper,
                pv_status_to_string(status));
        free(number_string);
        return status;
    }

    free(number_string);

    int32_t i = 0;
    int32_t j = 0;
    while (cardinal_string[j] != '\0') {
        while ((cardinal_string[j] != '\0') && (cardinal_string[j] != ' ')) {
            j++;
        }
        if (cardinal_string[j] == ' ') {
            i = j + 1;
            j++;
        }
    }

    char *last_cardinal = calloc(j - i + 1, sizeof(char));
    if (!last_cardinal) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("last_cardinal"));
        free(cardinal_string);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    strcpy(last_cardinal, cardinal_string + i);

    int32_t k = 0;
    while ((strcmp(ORDINAL_KEYS[k], last_cardinal) != 0) && (k < PV_ARRAY_LEN(ORDINAL_KEYS))) {
        k++;
    }

    free(last_cardinal);

    if (k >= PV_ARRAY_LEN(ORDINAL_KEYS)) {
        free(cardinal_string);
        return PV_STATUS_SUCCESS;
    }
    const char *last_ordinal = ORDINAL_VALUES[k];

    *string = calloc(i + (int32_t) strlen(last_ordinal) + 1, sizeof(char));
    if (!(*string)) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("string"));
        free(cardinal_string);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    strncpy(*string, cardinal_string, i);
    strcat(*string, last_ordinal);

    free(cardinal_string);

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_negative_ordinal(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_EN_NEGATIVE_ORDINAL) {
        char *verbalized = NULL;
        const char *digit_string = token->string + 1;
        pv_status_t status = pv_normalizer_verbalizer_ordinal_to_string(digit_string, &verbalized);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_cardinal_to_string,
                    pv_status_to_string(status));
            return status;
        }

        const char *negative = "NEGATIVE ";
        char *token_verbalized = calloc(strlen(verbalized) + strlen(negative) + 1, sizeof(char));
        if (!token_verbalized) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("token_verbalized"));
            free(verbalized);
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(token_verbalized, negative);
        strcat(token_verbalized, verbalized);
        free(verbalized);
        pv_normalizer_token_set_verbalized(token, token_verbalized);
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_special_character(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_EN_SPECIAL_CHARACTER) {
        int32_t i = 0;
        for (i = 0; i < PV_ARRAY_LEN(PV_NORMALIZER_SPECIAL_CHARACTERS); i++) {
            int32_t num_bytes = 0;
            pv_status_t status = pv_language_num_bytes_character(PV_NORMALIZER_SPECIAL_CHARACTERS[i][0], &num_bytes);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_language_num_bytes_character,
                        pv_status_to_string(status));
                return status;
            }
            if (strcmp(token->string, PV_NORMALIZER_SPECIAL_CHARACTERS[i]) == 0) {
                break;
            }
        }
        const char *verbalized = PV_NORMALIZER_SPECIAL_CHARACTERS_VERBALIZED[i];
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
        if (PV_NORMALIZER_SPECIAL_CHARACTERS_IS_VERBALIZED_TO_PUNCTUATION[i]) {
            token->tag = PV_NORMALIZER_TAG_EN_PUNCTUATION;
        }
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_decimal(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_EN_DECIMAL_POINT) {
        const char *point = "POINT";
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
    } else if (token->tag == PV_NORMALIZER_TAG_EN_DECIMAL_DIGITS) {
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

    if (token->tag == PV_NORMALIZER_TAG_EN_MEASUREMENT) {
        pv_normalizer_token_t *previous = pv_normalizer_token_get_nth_token_before(token, 1, true);
        const char *verbalized = NULL;
        if ((previous != NULL) &&
            (((previous->tag == PV_NORMALIZER_TAG_EN_CARDINAL) &&
              (strcmp(previous->string, "1") == 0)) ||
             previous->tag == PV_NORMALIZER_TAG_EN_PER_SLASH)) {
            verbalized = PV_NORMALIZER_MEASUREMENTS_VERBALIZED_SINGULAR[token->tag_data_index];
        } else {
            verbalized = PV_NORMALIZER_MEASUREMENTS_VERBALIZED_PLURAL[token->tag_data_index];
        }

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
    if (token->tag == PV_NORMALIZER_TAG_EN_PER_SLASH) {
        const char *per = "PER";

        char *token_verbalized = calloc(strlen(per) + 1, sizeof(char));
        if (!token_verbalized) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("token_verbalized"));
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(token_verbalized, per);
        pv_normalizer_token_set_verbalized(token, token_verbalized);
    }
    if (token->tag == PV_NORMALIZER_TAG_EN_MEASUREMENT) {
        pv_normalizer_token_t *previous = pv_normalizer_token_get_nth_token_before(token, 1, true);
        if ((previous != NULL) && (previous->tag == PV_NORMALIZER_TAG_EN_MEASUREMENT)) {
            const char *prev_verbalized = PV_NORMALIZER_MEASUREMENTS_VERBALIZED_SINGULAR[previous->tag_data_index];

            char *token_verbalized = calloc(strlen(prev_verbalized) + 1, sizeof(char));
            if (!token_verbalized) {
                PV_ERROR_REPORT(
                        &pv_error_msg_alloc,
                        PV_ERROR_ARGS_PUBLIC_EMPTY(),
                        PV_ERROR_ARGS_PRIVATE("token_verbalized"));
                return PV_STATUS_OUT_OF_MEMORY;
            }
            strcpy(token_verbalized, prev_verbalized);
            pv_normalizer_token_set_verbalized(previous, token_verbalized);
        }
        else if ((previous != NULL) && (previous->tag == PV_NORMALIZER_TAG_EN_DOT)) {
            pv_normalizer_token_t *two_previous = pv_normalizer_token_get_nth_token_before(token, 2, true);
            if ((two_previous != NULL) && (two_previous->tag == PV_NORMALIZER_TAG_EN_MEASUREMENT)) {
                const char *prev_verbalized = PV_NORMALIZER_MEASUREMENTS_VERBALIZED_SINGULAR[two_previous->tag_data_index];

                char *token_verbalized = calloc(strlen(prev_verbalized) + 1, sizeof(char));
                if (!token_verbalized) {
                    PV_ERROR_REPORT(
                            &pv_error_msg_alloc,
                            PV_ERROR_ARGS_PUBLIC_EMPTY(),
                            PV_ERROR_ARGS_PRIVATE("token_verbalized"));
                    return PV_STATUS_OUT_OF_MEMORY;
                }
                strcpy(token_verbalized, prev_verbalized);
                pv_normalizer_token_set_verbalized(two_previous, token_verbalized);
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
            pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(character, false, &verbalized_internal);
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

pv_status_t pv_normalizer_verbalizer_cardinal_spell_out(pv_normalizer_token_t *token) {
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

    if (token->tag == PV_NORMALIZER_TAG_EN_CARDINAL_SPELL_OUT) {
        pv_status_t status = pv_normalizer_verbalizer_cardinal_spell_out(token);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_cardinal_spell_out,
                    pv_status_to_string(status));
            return status;
        }
    } else if (token->tag == PV_NORMALIZER_TAG_EN_LETTER_SPELL_OUT) { // pronounced later in phonemizer
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

        pv_status_t status = pv_normalizer_util_upper_inplace(token->verbalized);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_util_upper_inplace,
                    pv_status_to_string(status));
            return status;
        }
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_time(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_EN_TIME_COLON) {
        pv_normalizer_token_set_verbalized(token, NULL);
    } else if (token->tag == PV_NORMALIZER_TAG_EN_TIME_HOURS) {
        const char *number_string = token->string;

        int32_t offset = 0;
        if (number_string[0] == '0') {
            offset = 1;
        }

        char *verbalized = NULL;
        pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(
                token->string + offset,
                false,
                &verbalized);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_cardinal_to_string,
                    pv_status_to_string(status));
            return status;
        }
        pv_normalizer_token_set_verbalized(token, verbalized);

    } else if (token->tag == PV_NORMALIZER_TAG_EN_TIME_MINUTES) {
        const char *number_string = token->string;

        if (number_string[0] == '0') {
            if ((strlen(number_string) > 1) && (number_string[1] == '0')) {
                pv_normalizer_token_set_verbalized(token, NULL);
            } else {
                char *verbalized = NULL;
                int32_t length = 0;

                while (*number_string != '\0') {
                    int32_t digit = (int32_t) *number_string - '0';
                    if (digit == 0) {
                        cardinal_to_string_helper(verbalized, "OH", &length, true);
                    } else {
                        cardinal_to_string_helper(verbalized, ONE_TO_NINETEEN[(int32_t) digit], &length, true);
                    }
                    if (*(number_string + 1) != '\0') {
                        cardinal_to_string_helper(verbalized, " ", &length, true);
                    }
                    number_string++;
                }

                verbalized = calloc(length + 1, sizeof(char));
                if (!verbalized) {
                    PV_ERROR_REPORT(
                            &pv_error_msg_alloc,
                            PV_ERROR_ARGS_PUBLIC_EMPTY(),
                            PV_ERROR_ARGS_PRIVATE("verbalized"));
                    return PV_STATUS_OUT_OF_MEMORY;
                }

                number_string = token->string;
                while (*number_string != '\0') {
                    int32_t digit = (int32_t) *number_string - '0';
                    if (digit == 0) {
                        cardinal_to_string_helper(verbalized, "OH", &length, false);
                    } else {
                        cardinal_to_string_helper(verbalized, ONE_TO_NINETEEN[(int32_t) digit], &length, false);
                    }
                    if (*(number_string + 1) != '\0') {
                        cardinal_to_string_helper(verbalized, " ", &length, false);
                    }
                    number_string++;
                }

                pv_normalizer_token_set_verbalized(token, verbalized);
            }
        } else {
            pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(
                    token->string,
                    false,
                    &(token->verbalized));
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_verbalizer_cardinal_to_string,
                        pv_status_to_string(status));
                return status;
            }
        }
    } else if (token->tag == PV_NORMALIZER_TAG_EN_TIME_AM_PM) {
        const char *am_pm_string = token->string;

        if ((am_pm_string[0] == 'a') || (am_pm_string[0] == 'A')) {
            char *pronunciation = "EY EH M";
            token->pronunciation = calloc(strlen(pronunciation) + 1, sizeof(char));
            if (!token->pronunciation) {
                PV_ERROR_REPORT(
                        &pv_error_msg_alloc,
                        PV_ERROR_ARGS_PUBLIC_EMPTY(),
                        PV_ERROR_ARGS_PRIVATE("pronunciation"));
                return PV_STATUS_OUT_OF_MEMORY;
            }
            strcpy(token->pronunciation, pronunciation);
        } else {
            char *pronunciation = "P IY EH M";
            token->pronunciation = calloc(strlen(pronunciation) + 1, sizeof(char));
            if (!token->pronunciation) {
                PV_ERROR_REPORT(
                        &pv_error_msg_alloc,
                        PV_ERROR_ARGS_PUBLIC_EMPTY(),
                        PV_ERROR_ARGS_PRIVATE("pronunciation"));
                return PV_STATUS_OUT_OF_MEMORY;
            }
            strcpy(token->pronunciation, pronunciation);
        }

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

        pv_status_t status = pv_normalizer_util_upper_inplace(token->verbalized);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_util_upper_inplace,
                    pv_status_to_string(status));
            return status;
        }

        token->tag = PV_NORMALIZER_TAG_EN_CUSTOM_PRONUNCIATION;
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_fraction_slash(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_EN_FRACTION_SLASH) {
        if ((token->next != NULL) &&
            ((token->next->tag == PV_NORMALIZER_TAG_EN_CARDINAL) ||
             (token->next->tag == PV_NORMALIZER_TAG_EN_NEGATIVE_CARDINAL))) {
            char *denominator_string = token->next->string;
            bool is_ordinal = false;
            for (int32_t i = 0; i < PV_ARRAY_LEN(DENOMINATOR_ORDINAL_KEYS); i++) {
                if (strcmp(denominator_string, DENOMINATOR_ORDINAL_KEYS[i]) == 0) {
                    is_ordinal = true;
                    break;
                }
            }

            bool next_is_decimal =
                    ((token->next->next != NULL) &&
                     (token->next->next->tag == PV_NORMALIZER_TAG_EN_DECIMAL_POINT));

            if (!is_ordinal || next_is_decimal) {
                const char *over = "OVER";

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
            } else {
                pv_normalizer_token_set_verbalized(token, NULL);

                free(token->string);
                token->string = calloc(2, sizeof(char));
                if (!token->string) {
                    PV_ERROR_REPORT(
                            &pv_error_msg_alloc,
                            PV_ERROR_ARGS_PUBLIC_EMPTY(),
                            PV_ERROR_ARGS_PRIVATE("token->string"));
                    return PV_STATUS_OUT_OF_MEMORY;
                }
                token->string[0] = ' ';
                token->string[1] = '\0';
            }
        }
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_fraction_denominator(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    bool is_ordinal_denominator =
            ((token->previous != NULL) &&
             (token->previous->tag == PV_NORMALIZER_TAG_EN_FRACTION_SLASH) &&
             (token->previous->verbalized == NULL));

    // Certain cardinal denominators have ordinal verbalizations eg. 1/3 -> ONE THIRD
    if ((token->tag == PV_NORMALIZER_TAG_EN_CARDINAL) && is_ordinal_denominator) {
        char *string = token->string;

        bool is_ordinal = false;
        int32_t index = 0;
        for (int32_t i = 0; i < PV_ARRAY_LEN(DENOMINATOR_ORDINAL_KEYS); i++) {
            if (strcmp(string, DENOMINATOR_ORDINAL_KEYS[i]) == 0) {
                is_ordinal = true;
                index = i;
                break;
            }
        }

        if (is_ordinal) {
            bool is_singular =
                    ((token->previous != NULL) &&
                     (token->previous->previous != NULL) &&
                     (strcmp(token->previous->previous->string, "1") == 0));

            const char *verbalized = NULL;
            if (is_singular) {
                verbalized = DENOMINATOR_ORDINAL_VALUES_SINGULAR[index];
            } else {
                verbalized = DENOMINATOR_ORDINAL_VALUES_PLURAL[index];
            }

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

pv_status_t pv_normalizer_verbalizer_verbalize_dot(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_EN_DOT) {
        pv_normalizer_token_t *previous = pv_normalizer_token_get_nth_token_before(token, 1, true);
        if ((previous != NULL) && (previous->tag == PV_NORMALIZER_TAG_EN_MEASUREMENT)) {
            return PV_STATUS_SUCCESS;
        }

        const char *dot = "DOT";

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

    if (token->tag == PV_NORMALIZER_TAG_EN_COLON) {
        const char *colon = "COLON";

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

    if (token->tag == PV_NORMALIZER_TAG_EN_TOP_LEVEL_DOMAIN) {
        PV_ASSERT(!PV_NORMALIZER_TOP_LEVEL_DOMAINS_IS_SPELL_OUT[token->tag_data_index]);

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

        pv_status_t status = pv_normalizer_util_upper_inplace(token->verbalized);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_util_upper_inplace,
                    pv_status_to_string(status));
            return status;
        }
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_currency(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_EN_CURRENCY) {
        char *string = token->string;
        int32_t length = (int32_t) strlen(string);

        char *space = " ";
        char *and = " AND ";

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

        const char *currency_string = PV_NORMALIZER_CURRENCIES_VERBALIZED_PLURAL[token->tag_data_index];
        if (strcmp(number_string, "1") == 0) {
            currency_string = PV_NORMALIZER_CURRENCIES_VERBALIZED_SINGULAR[token->tag_data_index];
        }

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
                &verbalized_digit_string);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_cardinal_to_string,
                    pv_status_to_string(status));
            free(verbalized_number_string);
            free(digit_string);
            return status;
        }

        const char *sub_currency_string = PV_NORMALIZER_SUB_CURRENCIES_VERBALIZED_PLURAL[token->tag_data_index];
        if (strcmp(digit_string, "1") == 0) {
            sub_currency_string = PV_NORMALIZER_SUB_CURRENCIES_VERBALIZED_SINGULAR[token->tag_data_index];
        }

        free(digit_string);

        int32_t verbalized_length =
                (int32_t) strlen(verbalized_number_string) +
                (int32_t) strlen(space) +
                (int32_t) strlen(currency_string) +
                (int32_t) strlen(and) +
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
        strcat(token_verbalized, and);
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

    if (token->tag == PV_NORMALIZER_TAG_EN_NEGATIVE_CURRENCY) {
        char *string = token->string;
        int32_t length = (int32_t) strlen(string);

        char *negative = "NEGATIVE ";
        char *space = " ";
        char *and = " AND ";

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

        const char *currency_string = PV_NORMALIZER_CURRENCIES_VERBALIZED_PLURAL[token->tag_data_index];
        if (strcmp(number_string, "1") == 0) {
            currency_string = PV_NORMALIZER_CURRENCIES_VERBALIZED_SINGULAR[token->tag_data_index];
        }

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
                &verbalized_digit_string);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_cardinal_to_string,
                    pv_status_to_string(status));
            free(verbalized_number_string);
            free(digit_string);
            return status;
        }

        const char *sub_currency_string = PV_NORMALIZER_SUB_CURRENCIES_VERBALIZED_PLURAL[token->tag_data_index];
        if (strcmp(digit_string, "1") == 0) {
            currency_string = PV_NORMALIZER_SUB_CURRENCIES_VERBALIZED_SINGULAR[token->tag_data_index];
        }

        free(digit_string);

        int32_t verbalized_length =
                (int32_t) strlen(negative) +
                (int32_t) strlen(verbalized_number_string) +
                (int32_t) strlen(space) +
                (int32_t) strlen(currency_string) +
                (int32_t) strlen(and) +
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
        strcat(token_verbalized, and);
        strcat(token_verbalized, verbalized_digit_string);
        strcat(token_verbalized, space);
        strcat(token_verbalized, sub_currency_string);

        free(verbalized_digit_string);
        free(verbalized_number_string);
        pv_normalizer_token_set_verbalized(token, token_verbalized);
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_currency_symbol(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_EN_CURRENCY_SYMBOL) {
        pv_normalizer_token_t *previous = pv_normalizer_token_get_nth_token_before(token, 1, true);
        if ((previous == NULL) || (token->tag_data_index < 0)) {
            return PV_STATUS_SUCCESS;
        }

        char *string = previous->string;
        const char *currency_string = PV_NORMALIZER_CURRENCIES_VERBALIZED_PLURAL[token->tag_data_index];
        if (strcmp(string, "1") == 0) {
            currency_string = PV_NORMALIZER_CURRENCIES_VERBALIZED_SINGULAR[token->tag_data_index];
        }

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

    if (token->tag == PV_NORMALIZER_TAG_EN_ABBREVIATION) {
        PV_ASSERT((token->tag_data_index >= 0) && (token->tag_data_index < PV_ARRAY_LEN(PV_NORMALIZER_ABBREVIATIONS_VERBALIZED)));
        const char *verbalized = PV_NORMALIZER_ABBREVIATIONS_VERBALIZED[token->tag_data_index];

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

    const char *comma_string = ",";

    const char *string = token->string;
    int32_t length = (int32_t) strlen(string);

    if (token->tag == PV_NORMALIZER_TAG_EN_DIGITS_WITH_PARENTHESES) {
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
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(token_verbalized, comma_string_with_space);
        strcat(token_verbalized, verbalized);
        strcat(token_verbalized, comma_string);
        token_verbalized[total_length] = '\0';
        free(verbalized);
        pv_normalizer_token_set_verbalized(token, token_verbalized);
    } else if (token->tag == PV_NORMALIZER_TAG_EN_DIGITS) {
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
    } else if (token->tag == PV_NORMALIZER_TAG_EN_DIGITS_SEPARATOR) {
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
        token->tag = PV_NORMALIZER_TAG_EN_PUNCTUATION;
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_year(
        pv_normalizer_token_t *token,
        char **verbalized) {
    PV_ASSERT(token);
    PV_ASSERT(verbalized);

    *verbalized = NULL;

    char *verbalized_internal = NULL;

    int32_t number = (int32_t) strtol(token->string, NULL, 10);
    if (number < 0) {
        return PV_STATUS_SUCCESS;
    }

    if ((number < 1112) || ((number >= 2000) && (number < 2010))) {
        pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(token->string, false, &verbalized_internal);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_cardinal_to_string,
                    pv_status_to_string(status));
            return status;
        }
    } else if (((number >= 1112) && (number < 2000)) || (number >= 2010)) {
        int32_t two_digit_length = 2;
        char *first_number = calloc(two_digit_length + 1, sizeof(char));
        if (!first_number) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("first_number"));
            return PV_STATUS_OUT_OF_MEMORY;
        }
        memcpy(first_number, token->string, two_digit_length * sizeof(char));
        first_number[two_digit_length] = '\0';

        char *second_number = NULL;
        if (strlen(token->string) == 4) {
            second_number = calloc(two_digit_length + 1, sizeof(char));
            if (!second_number) {
                PV_ERROR_REPORT(
                        &pv_error_msg_alloc,
                        PV_ERROR_ARGS_PUBLIC_EMPTY(),
                        PV_ERROR_ARGS_PRIVATE("second_number"));
                free(first_number);
                return PV_STATUS_OUT_OF_MEMORY;
            }
            memcpy(second_number, token->string + two_digit_length, two_digit_length * sizeof(char));
            second_number[two_digit_length] = '\0';
        }

        char *first_number_verbalized = NULL;
        pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(
                first_number,
                false,
                &first_number_verbalized);
        free(first_number);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_cardinal_to_string,
                    pv_status_to_string(status));
            free(second_number);
            return status;
        }

        char *second_number_verbalized = NULL;
        if (second_number != NULL) {
            // standard case: `1918`
            if (second_number[0] != '0') {
                status = pv_normalizer_verbalizer_cardinal_to_string(second_number, false, &second_number_verbalized);
                free(second_number);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_cardinal_to_string,
                            pv_status_to_string(status));
                    free(first_number_verbalized);
                    return status;
                }
            } else if (second_number[1] == '0') { // `1900` -> `nineteen hundred`
                free(second_number);
                const char *hundred = "HUNDRED";
                second_number_verbalized = calloc(strlen(hundred) + 1, sizeof(char));
                if (!second_number_verbalized) {
                    PV_ERROR_REPORT(
                            &pv_error_msg_alloc,
                            PV_ERROR_ARGS_PUBLIC_EMPTY(),
                            PV_ERROR_ARGS_PRIVATE("second_number_verbalized"));
                    free(first_number_verbalized);
                    return PV_STATUS_OUT_OF_MEMORY;
                }
                strcpy(second_number_verbalized, hundred);
            } else { // `1901` -> `nineteen oh one`
                const char *oh = "OH ";
                char *single_digit_verbalized = NULL;
                status = pv_normalizer_verbalizer_cardinal_to_string(
                        second_number + 1,
                        false,
                        &single_digit_verbalized);
                free(second_number);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_cardinal_to_string,
                            pv_status_to_string(status));
                    free(first_number_verbalized);
                    return status;
                }

                int32_t total_length = (int32_t) strlen(oh) + (int32_t) strlen(single_digit_verbalized);
                second_number_verbalized = calloc(total_length + 1, sizeof(char));
                if (!second_number_verbalized) {
                    PV_ERROR_REPORT(
                            &pv_error_msg_alloc,
                            PV_ERROR_ARGS_PUBLIC_EMPTY(),
                            PV_ERROR_ARGS_PRIVATE("second_number_verbalized"));
                    free(first_number_verbalized);
                    free(single_digit_verbalized);
                    return PV_STATUS_OUT_OF_MEMORY;
                }
                strcpy(second_number_verbalized, oh);
                strcat(second_number_verbalized, single_digit_verbalized);
                free(single_digit_verbalized);
            }
        }

        int32_t total_length = (int32_t) strlen(first_number_verbalized);
        if (second_number_verbalized != NULL) {
            total_length += (int32_t) strlen(" ");
            total_length += (int32_t) strlen(second_number_verbalized);
        }

        verbalized_internal = calloc(total_length + 1, sizeof(char));
        if (!verbalized_internal) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("token->verbalized"));
            free(first_number_verbalized);
            free(second_number_verbalized);
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(verbalized_internal, first_number_verbalized);
        if (second_number_verbalized != NULL) {
            strcat(verbalized_internal, " ");
            strcat(verbalized_internal, second_number_verbalized);
        }
        verbalized_internal[total_length] = '\0';

        free(first_number_verbalized);
        free(second_number_verbalized);
    }

    *verbalized = verbalized_internal;

    return PV_STATUS_SUCCESS;
}

bool pv_normalizer_verbalizer_is_year_first(pv_normalizer_token_t *token, bool check_previous) {
    PV_ASSERT(token);

    bool is_year_first = false;
    if (check_previous) {
        pv_normalizer_token_t *neighbor = pv_normalizer_token_get_nth_token_before(token, 1, false);
        pv_normalizer_token_t *second_neighbor = pv_normalizer_token_get_nth_token_before(token, 2, false);
        pv_normalizer_token_t *third_neighbor = pv_normalizer_token_get_nth_token_before(token, 3, false);
        pv_normalizer_token_t *fourth_neighbor = pv_normalizer_token_get_nth_token_before(token, 4, false);
        is_year_first =
                (fourth_neighbor != NULL) &&
                (fourth_neighbor->tag == PV_NORMALIZER_TAG_EN_DATE_YEAR) &&
                (third_neighbor->tag == PV_NORMALIZER_TAG_EN_DATE_SEPARATOR) &&
                (second_neighbor->tag == PV_NORMALIZER_TAG_EN_DATE_MONTH) &&
                (neighbor->tag == PV_NORMALIZER_TAG_EN_DATE_SEPARATOR);
    } else {
        pv_normalizer_token_t *neighbor = pv_normalizer_token_get_nth_token_after(token, 1, false);
        pv_normalizer_token_t *second_neighbor = pv_normalizer_token_get_nth_token_after(token, 2, false);
        pv_normalizer_token_t *third_neighbor = pv_normalizer_token_get_nth_token_after(token, 3, false);
        pv_normalizer_token_t *fourth_neighbor = pv_normalizer_token_get_nth_token_after(token, 4, false);
        is_year_first =
                (fourth_neighbor != NULL) &&
                ((fourth_neighbor->tag == PV_NORMALIZER_TAG_EN_DATE_DAY) ||
                 (fourth_neighbor->tag == PV_NORMALIZER_TAG_EN_DATE_MONTH)) &&
                (third_neighbor->tag == PV_NORMALIZER_TAG_EN_DATE_SEPARATOR) &&
                ((second_neighbor->tag == PV_NORMALIZER_TAG_EN_DATE_DAY) ||
                 (second_neighbor->tag == PV_NORMALIZER_TAG_EN_DATE_MONTH)) &&
                (neighbor->tag == PV_NORMALIZER_TAG_EN_DATE_SEPARATOR);
    }

    return is_year_first;
}

pv_status_t pv_normalizer_verbalizer_verbalize_dates(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    const char *comma_string = ",";

    if (token->tag == PV_NORMALIZER_TAG_EN_DATE_SEPARATOR) {
        pv_normalizer_token_set_verbalized(token, NULL);
    } else if (token->tag == PV_NORMALIZER_TAG_EN_DATE_DAY) {
        int32_t offset = (token->string[0] == '0') ? 1 : 0;
        PV_ASSERT(strcmp(token->string + offset, "\0") != 0);

        int32_t number_length = (int32_t) strlen(token->string + offset);
        char *ordinal_string = calloc(number_length + 2 + 1, sizeof(char));
        if (!ordinal_string) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("ordinal_string"));
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(ordinal_string, token->string + offset);

        int32_t number = (int32_t) strtol(token->string, NULL, 10);
        if (number < 0) {
            free(ordinal_string);
            return PV_STATUS_SUCCESS;
        }

        int32_t last_digit = number % 10;
        int32_t last_two_digits = number % 100;
        if (last_two_digits == 11 || last_two_digits == 12 || last_two_digits == 13) {
            strcat(ordinal_string, "TH");
        } else {
            if (last_digit == 1) {
                strcat(ordinal_string, "ST");
            } else if (last_digit == 2) {
                strcat(ordinal_string, "ND");
            } else if (last_digit == 3) {
                strcat(ordinal_string, "RD");
            } else {
                strcat(ordinal_string, "TH");
            }
        }

        char *ordinal_verbalized = NULL;
        pv_status_t status = pv_normalizer_verbalizer_ordinal_to_string(ordinal_string, &ordinal_verbalized);
        free(ordinal_string);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_ordinal_to_string,
                    pv_status_to_string(status));
            return status;
        }

        pv_normalizer_token_t *previous = pv_normalizer_token_get_nth_token_before(token, 1, true);
        pv_normalizer_token_t *two_previous = pv_normalizer_token_get_nth_token_before(token, 2, true);
        pv_normalizer_token_t *next = pv_normalizer_token_get_nth_token_after(token, 1, true);
        pv_normalizer_token_t *two_next = pv_normalizer_token_get_nth_token_after(token, 2, true);
        bool has_month_before =
                ((previous != NULL) && (previous->tag == PV_NORMALIZER_TAG_EN_DATE_MONTH)) ||
                ((two_previous != NULL) && (two_previous->tag == PV_NORMALIZER_TAG_EN_DATE_MONTH));
        if (has_month_before) {
            int32_t total_length = (int32_t) strlen(ordinal_verbalized);

            bool is_year_first = pv_normalizer_verbalizer_is_year_first(token, true);
            pv_normalizer_token_t *year_token = pv_normalizer_token_get_nth_token_before(token, 4, true);

            if (is_year_first && (year_token != NULL) && (year_token->tag == PV_NORMALIZER_TAG_EN_DATE_YEAR)) {
                total_length += (int32_t) strlen(comma_string);
                total_length += 2; // space before and after comma

                char *verbalized_year = NULL;
                status = pv_normalizer_verbalizer_verbalize_year(year_token, &verbalized_year);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_year,
                            pv_status_to_string(status));
                    free(ordinal_verbalized);
                    return status;
                }
                total_length += (int32_t) strlen(verbalized_year);

                char *token_verbalized = calloc(total_length + 1, sizeof(char));
                if (!token_verbalized) {
                    PV_ERROR_REPORT(
                            &pv_error_msg_alloc,
                            PV_ERROR_ARGS_PUBLIC_EMPTY(),
                            PV_ERROR_ARGS_PRIVATE("token_verbalized"));
                    free(ordinal_verbalized);
                    free(verbalized_year);
                    return PV_STATUS_OUT_OF_MEMORY;
                }
                strcpy(token_verbalized, ordinal_verbalized);
                strcat(token_verbalized, " ");
                strcat(token_verbalized, comma_string);
                strcat(token_verbalized, " ");
                strcat(token_verbalized, verbalized_year);
                token_verbalized[total_length] = '\0';
                free(verbalized_year);
                pv_normalizer_token_set_verbalized(token, token_verbalized);
            } else {
                bool has_year_after =
                        ((next != NULL) && (next->tag == PV_NORMALIZER_TAG_EN_DATE_YEAR)) ||
                        ((two_next != NULL) && (two_next->tag == PV_NORMALIZER_TAG_EN_DATE_YEAR));
                bool add_comma = has_month_before && has_year_after && (next != NULL) && (next->tag != PV_NORMALIZER_TAG_EN_PUNCTUATION);
                if (add_comma) {
                    total_length += (int32_t) strlen(comma_string);
                    // add space before comma
                    total_length += 1;
                }

                char *token_verbalized = calloc(total_length + 1, sizeof(char));
                if (!token_verbalized) {
                    PV_ERROR_REPORT(
                            &pv_error_msg_alloc,
                            PV_ERROR_ARGS_PUBLIC_EMPTY(),
                            PV_ERROR_ARGS_PRIVATE("token_verbalized"));
                    free(ordinal_verbalized);
                    return PV_STATUS_OUT_OF_MEMORY;
                }
                strcpy(token_verbalized, ordinal_verbalized);
                if (add_comma) {
                    strcat(token_verbalized, " ");
                    strcat(token_verbalized, comma_string);
                }
                token_verbalized[total_length] = '\0';
                pv_normalizer_token_set_verbalized(token, token_verbalized);
            }
        } else {
            const char *the_string = "THE ";
            const char *of_string = " OF";

            bool has_the_string_before = false;
            pv_normalizer_token_t *day_previous = pv_normalizer_token_get_nth_token_before(token, 1, true);
            if (day_previous != NULL) {
                char *upper_prev_str = NULL;
                status = pv_normalizer_util_upper(day_previous->string, &upper_prev_str);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_util_upper,
                            pv_status_to_string(status));
                    return status;
                }
                if (strcmp(upper_prev_str, "THE") == 0) {
                    has_the_string_before = true;
                }
                free(upper_prev_str);
            }

            int32_t total_length = (int32_t) strlen(ordinal_verbalized) + (int32_t) strlen(of_string);
            if (!has_the_string_before) {
                total_length += (int32_t) strlen(the_string);
            }
            char *token_verbalized = calloc(total_length + 1, sizeof(char));
            if (!token_verbalized) {
                PV_ERROR_REPORT(
                        &pv_error_msg_alloc,
                        PV_ERROR_ARGS_PUBLIC_EMPTY(),
                        PV_ERROR_ARGS_PRIVATE("token->verbalized"));
                free(ordinal_verbalized);
                return PV_STATUS_OUT_OF_MEMORY;
            }
            if (!has_the_string_before) {
                strcpy(token_verbalized, the_string);
                strcat(token_verbalized, ordinal_verbalized);
            } else {
                strcpy(token_verbalized, ordinal_verbalized);
            }
            strcat(token_verbalized, of_string);
            token_verbalized[total_length] = '\0';
            pv_normalizer_token_set_verbalized(token, token_verbalized);
        }
        free(ordinal_verbalized);
    } else if (token->tag == PV_NORMALIZER_TAG_EN_DATE_MONTH) {
        if (token->tag_data_index < 0) {
            int32_t number = (int32_t) strtol(token->string, NULL, 10);
            if (number < 0) {
                return PV_STATUS_SUCCESS;
            }
            token->tag_data_index = number - 1;
            if (token->tag_data_index < 0) {
                return PV_STATUS_SUCCESS;
            }
        }

        const char *verbalized_string = PV_NORMALIZER_MONTH_NAMES_VERBALIZED[token->tag_data_index];

        pv_normalizer_token_t *previous = pv_normalizer_token_get_nth_token_before(token, 1, true);
        pv_normalizer_token_t *two_previous = pv_normalizer_token_get_nth_token_before(token, 2, true);
        pv_normalizer_token_t *next = pv_normalizer_token_get_nth_token_after(token, 1, true);
        pv_normalizer_token_t *two_next = pv_normalizer_token_get_nth_token_after(token, 2, true);
        bool has_day_before =
                ((previous != NULL) && (previous->tag == PV_NORMALIZER_TAG_EN_DATE_DAY)) ||
                ((two_previous != NULL) && (two_previous->tag == PV_NORMALIZER_TAG_EN_DATE_DAY));
        bool has_year_after =
                ((next != NULL) && (next->tag == PV_NORMALIZER_TAG_EN_DATE_YEAR)) ||
                ((two_next != NULL) && (two_next->tag == PV_NORMALIZER_TAG_EN_DATE_YEAR));
        if (has_day_before) {
            int32_t total_length = (int32_t) strlen(verbalized_string);
            if (has_year_after) {
                total_length += (int32_t) strlen(comma_string);
                // add space before comma
                total_length += 1;
            }

            char *token_verbalized = calloc(total_length + 1, sizeof(char));
            if (!token_verbalized) {
                PV_ERROR_REPORT(
                        &pv_error_msg_alloc,
                        PV_ERROR_ARGS_PUBLIC_EMPTY(),
                        PV_ERROR_ARGS_PRIVATE("token_verbalized"));
                return PV_STATUS_OUT_OF_MEMORY;
            }
            strcpy(token_verbalized, verbalized_string);
            if (has_year_after) {
                strcat(token_verbalized, " ");
                strcat(token_verbalized, comma_string);
            }
            token_verbalized[total_length] = '\0';
            pv_normalizer_token_set_verbalized(token, token_verbalized);
        } else {
            char *token_verbalized = calloc(strlen(verbalized_string) + 1, sizeof(char));
            if (!token_verbalized) {
                PV_ERROR_REPORT(
                        &pv_error_msg_alloc,
                        PV_ERROR_ARGS_PUBLIC_EMPTY(),
                        PV_ERROR_ARGS_PRIVATE("token_verbalized"));
                return PV_STATUS_OUT_OF_MEMORY;
            }
            strcpy(token_verbalized, verbalized_string);
            pv_normalizer_token_set_verbalized(token, token_verbalized);
        }
    } else if (token->tag == PV_NORMALIZER_TAG_EN_DATE_YEAR) {
        bool is_year_first = pv_normalizer_verbalizer_is_year_first(token, false);

        if (is_year_first) {
            // If year is first, append the year to the end of the date. Handled in the `day token` case.
            pv_normalizer_token_set_verbalized(token, NULL);
        } else {
            char *verbalized = NULL;
            pv_status_t status = pv_normalizer_verbalizer_verbalize_year(token, &verbalized);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_verbalizer_verbalize_year,
                        pv_status_to_string(status));
                return status;
            }
            pv_normalizer_token_set_verbalized(token, verbalized);
        }
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_names(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_EN_NAME_INITIAL_DOT) {
        pv_normalizer_token_set_verbalized(token, NULL);
    }

    return PV_STATUS_SUCCESS;
}
