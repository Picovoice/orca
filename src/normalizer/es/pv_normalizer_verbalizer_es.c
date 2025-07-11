#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "core/pv_assert.h"
#include "core/pv_error_messages.h"
#include "orca/normalizer/pv_normalizer_token.h"
#include "orca/normalizer/pv_normalizer_use_cases.h"
#include "orca/normalizer/pv_normalizer_verbalizer.h"

#include "orca/normalizer/es/pv_normalizer_data_es/pv_normalizer_data_es.h"
#include "orca/normalizer/es/pv_normalizer_tags_es.h"
#include "orca/normalizer/es/pv_normalizer_verbalizer_es.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_normalizer_mock.h"

#endif

struct pv_normalizer_verbalizer_es {
    int32_t num_use_cases;
    const pv_normalizer_use_cases_es_t *use_cases;
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
        pv_normalizer_token_gender_t gender,
        bool to_digit_string,
        bool dry_run,
        bool use_short_one,
        char **string);

static pv_status_t pv_normalizer_verbalizer_cardinal_to_string(
        const char *number_string,
        pv_normalizer_token_gender_t gender,
        bool to_digit_string,
        bool use_short_one,
        char **string);

static pv_status_t pv_normalizer_verbalizer_verbalize_custom_pronunciation(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_number_range(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_ordinal(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_ordinal_to_string(
        const char *ordinal_string,
        pv_normalizer_token_gender_t gender,
        char **string);

static pv_status_t pv_normalizer_verbalizer_verbalize_negative_ordinal(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_special_character(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_decimal(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_measurement(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_cardinal_spell_out_helper(
        const char *digit_string,
        bool dry_run,
        int32_t *length,
        pv_normalizer_token_gender_t gender,
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

static pv_status_t pv_normalizer_verbalizer_verbalize_dates(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_names(pv_normalizer_token_t *token);

pv_status_t PV_MOCKABLE(pv_normalizer_verbalizer_es_init)(
        int32_t num_use_cases,
        const pv_normalizer_use_cases_es_t *use_cases,
        pv_normalizer_verbalizer_es_t **object) {
    PV_ASSERT(use_cases);
    PV_ASSERT(object);

    *object = NULL;

    pv_normalizer_verbalizer_es_t *o = calloc(1, sizeof(pv_normalizer_verbalizer_es_t));
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

void PV_MOCKABLE(pv_normalizer_verbalizer_es_delete)(pv_normalizer_verbalizer_es_t *object) {
    if (object) {
        free(object);
    }
}

pv_status_t PV_MOCKABLE(pv_normalizer_verbalizer_es_verbalize)(
        pv_normalizer_verbalizer_es_t *object,
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
            if (object->use_cases[j] == PV_NORMALIZER_USE_WORD_NORMALIZER_ES) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_word(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_word,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_PUNCTUATION_NORMALIZER_ES) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_punctuation(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_punctuation,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_CARDINAL_NORMALIZER_ES) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_cardinal(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_cardinal,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_ES) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_negative_cardinal(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_negative_cardinal,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_CUSTOM_PRONUNCIATION_NORMALIZER_ES) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_custom_pronunciation(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_custom_pronunciation,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_NUMBER_RANGE_NORMALIZER_ES) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_number_range(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_number_range,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_ORDINAL_NORMALIZER_ES) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_ordinal(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_ordinal,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_NEGATIVE_ORDINAL_NORMALIZER_ES) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_negative_ordinal(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_negative_ordinal,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_SPECIAL_CHARACTER_NORMALIZER_ES) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_special_character(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_special_character,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_DECIMAL_NORMALIZER_ES) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_decimal(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_decimal,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_MEASUREMENT_NORMALIZER_ES) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_measurement(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_measurement,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_ALPHANUM_SPELL_OUT_NORMALIZER_ES) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_alphanum_spell_out(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_alphanum_spell_out,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_TIME_NORMALIZER_ES) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_time(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_time,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_FRACTION_NORMALIZER_ES) {
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
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_URL_NORMALIZER_ES) {
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
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_CURRENCY_NORMALIZER_ES) {
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
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_ABBREVIATION_NORMALIZER_ES) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_abbreviation(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_abbreviation,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_DIGITS_SEQUENCE_NORMALIZER_ES) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_digits_sequence(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_digits_sequence,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_DATE_NORMALIZER_ES) {
                pv_status_t status = pv_normalizer_verbalizer_verbalize_dates(token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_dates,
                            pv_status_to_string(status));
                    return status;
                }
            } else if (object->use_cases[j] == PV_NORMALIZER_USE_NAME_NORMALIZER_ES) {
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
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_PUNCTUATION,
            PV_NORMALIZER_TAG_ES_SINGLE_QUOTE,
            PV_NORMALIZER_TAG_ES_CUSTOM_PRONUNCIATION,
            -1);
}

pv_status_t pv_normalizer_verbalizer_verbalize_word(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_ES_WORD) {
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

    if (token->tag == PV_NORMALIZER_TAG_ES_PUNCTUATION) {
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

    if (token->tag == PV_NORMALIZER_TAG_ES_SINGLE_QUOTE) {
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
        pv_normalizer_token_gender_t gender,
        bool to_digit_string,
        bool dry_run,
        bool use_short_one,
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
        pv_normalizer_util_string_number_greater_than_int(number_string, MAX_VERBALIZED_CARDINAL_ES)) {
        while (*number_string != '\0') {
            int32_t digit = (int32_t) *number_string - '0';
            if (digit == 0) {
                cardinal_to_string_helper(result, "CERO", length, dry_run);
            } else {
                cardinal_to_string_helper(result, ONE_TO_TWENTY_NINE_ES[(int32_t) digit], length, dry_run);
            }
            cardinal_to_string_helper(result, " ", length, dry_run);
            number_string++;
        }
    } else {
        int64_t number = (int64_t) strtoll(number_string, NULL, 10);
        if (number < 0) {
            return PV_STATUS_SUCCESS;
        }

        if (number < 30) {
            if (number == 1 && gender == PV_NORMALIZER_TOKEN_GENDER_FEMININE) {
                cardinal_to_string_helper(result, "UNA", length, dry_run);
            } else {
                if (use_short_one) {
                    if (number == 1) {
                        cardinal_to_string_helper(result, "UN", length, dry_run);
                    } else if (number == 21) {
                        cardinal_to_string_helper(result, "VEINTIÚN", length, dry_run);
                    } else {
                        cardinal_to_string_helper(result, ONE_TO_TWENTY_NINE_ES[number], length, dry_run);
                    }
                } else {
                    cardinal_to_string_helper(result, ONE_TO_TWENTY_NINE_ES[number], length, dry_run);
                }
            }
            *string = result;

            return PV_STATUS_SUCCESS;
        }

        int64_t current_multiplier = MAX_MULTIPLIER_ES;
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

            if (current_hundred == 100) {
                cardinal_to_string_helper(result, "CIEN ", length, dry_run);
            } else if (current_hundred > 100 && current_hundred < 200) {
                cardinal_to_string_helper(result, "CIENTO ", length, dry_run);
            } else if (current_hundred >= 200 && current_hundred < 1000) {
                int64_t remaining = number % current_multiplier;
                if (gender == PV_NORMALIZER_TOKEN_GENDER_FEMININE && remaining < 100) {
                    cardinal_to_string_helper(
                            result,
                            HUNDREDS_FEMININE_ES[(int32_t) ((int32_t) current_hundred / 100) - 1],
                            length,
                            dry_run);
                } else {
                    cardinal_to_string_helper(
                            result,
                            HUNDREDS_ES[(int32_t) ((int32_t) current_hundred / 100) - 1],
                            length,
                            dry_run);
                }
                cardinal_to_string_helper(result, " ", length, dry_run);
            }

            current_hundred %= 100;

            if (current_hundred > 0 && current_hundred < 30) {
                if (current_multiplier >= 1000000 && current_hundred == 1) {
                    cardinal_to_string_helper(result, "UN ", length, dry_run);
                } else if (!(current_multiplier == 1000 && (i / current_multiplier) == 1)) {
                    // in spanish "one" is ignored when we say "one thousand"
                    if (use_short_one) {
                        if (current_hundred == 21) {
                            cardinal_to_string_helper(result, "VEINTIÚN", length, dry_run);
                        } else if (current_hundred == 1) {
                            cardinal_to_string_helper(result, "UN", length, dry_run);
                        } else {
                            cardinal_to_string_helper(result, ONE_TO_TWENTY_NINE_ES[current_hundred], length, dry_run);
                        }
                    } else {
                        cardinal_to_string_helper(result, ONE_TO_TWENTY_NINE_ES[current_hundred], length, dry_run);
                    }
                    cardinal_to_string_helper(result, " ", length, dry_run);
                }
            } else if (current_hundred % 10 == 0 && current_hundred != 0) {
                cardinal_to_string_helper(
                        result,
                        TENS_ES[((int32_t) (int32_t) current_hundred / 10) - 1],
                        length,
                        dry_run);
                cardinal_to_string_helper(result, " ", length, dry_run);
            } else if (current_hundred > 30 && current_hundred < 100) {
                cardinal_to_string_helper(
                        result,
                        TENS_ES[((int32_t) (int32_t) current_hundred / 10) - 1],
                        length,
                        dry_run);
                cardinal_to_string_helper(result, " ", length, dry_run);
                if (current_hundred % 10 > 0) {
                    cardinal_to_string_helper(result, "Y ", length, dry_run);
                }
                int32_t ones = (int32_t) current_hundred % 10;
                if (gender == PV_NORMALIZER_TOKEN_GENDER_FEMININE && ones == 1) {
                    cardinal_to_string_helper(result, "UNA", length, dry_run);
                } else if (use_short_one && ones == 1) {
                    cardinal_to_string_helper(result, "UN", length, dry_run);
                } else {
                    cardinal_to_string_helper(result, ONE_TO_TWENTY_NINE_ES[(int32_t) current_hundred % 10], length, dry_run);
                }
                cardinal_to_string_helper(result, " ", length, dry_run);
            }

            if (current_multiplier_i < 4) {
                current_multiplier_i++;
                if ((i / current_multiplier) > 1) {
                    // plural
                    cardinal_to_string_helper(result, MULTIPLIERS_PLURAL_ES[current_multiplier_i], length, dry_run);
                } else {
                    // singular
                    cardinal_to_string_helper(result, MULTIPLIERS_ES[current_multiplier_i], length, dry_run);
                }
                cardinal_to_string_helper(result, " ", length, dry_run);
            }

            i %= current_multiplier;
            if (i < 1000000 && current_multiplier == 1000000000) {
                cardinal_to_string_helper(result, MULTIPLIERS_PLURAL_ES[3], length, dry_run);
                cardinal_to_string_helper(result, " ", length, dry_run);
            }

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
        pv_normalizer_token_gender_t gender,
        bool to_digit_string,
        bool use_short_one,
        char **string) {
    PV_ASSERT(number_string);
    PV_ASSERT(string);

    int32_t length = 0;

    pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string_helper(
            number_string,
            &length,
            gender,
            to_digit_string,
            true,
            use_short_one,
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
            gender,
            to_digit_string,
            false,
            use_short_one,
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
             (token->previous->tag == PV_NORMALIZER_TAG_ES_FRACTION_SLASH) &&
             (token->previous->verbalized == NULL));

    if ((token->tag == PV_NORMALIZER_TAG_ES_CARDINAL) && !is_ordinal_denominator) {
        char *verbalized = NULL;
        pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(
                token->string,
                token->gender,
                false,
                token->use_short_one,
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

pv_status_t pv_normalizer_verbalizer_verbalize_negative_cardinal(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_ES_NEGATIVE_CARDINAL) {
        char *verbalized = NULL;
        const char *digit_string = token->string + 1;
        pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(
                digit_string,
                token->gender,
                false,
                false,
                &verbalized);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_cardinal_to_string,
                    pv_status_to_string(status));
            return status;
        }

        const char *negative = "NEGATIVO ";

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

    if (token->tag == PV_NORMALIZER_TAG_ES_CUSTOM_PRONUNCIATION) {
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

    if (token->tag == PV_NORMALIZER_TAG_ES_NUMBER_RANGE_TO) {
        char *token_verbalized = calloc(strlen(NUMBER_RANGE_INBETWEEN_STRING_ES) + 1, sizeof(char));
        if (!token_verbalized) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("token_verbalized"));
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(token_verbalized, NUMBER_RANGE_INBETWEEN_STRING_ES);
        pv_normalizer_token_set_verbalized(token, token_verbalized);
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_ordinal(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_ES_ORDINAL) {
        char *verbalized = NULL;
        pv_status_t status = pv_normalizer_verbalizer_ordinal_to_string(
                token->string,
                token->gender,
                &verbalized);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_ordinal_to_string,
                    pv_status_to_string(status));
            return status;
        }

        pv_normalizer_token_set_verbalized(token, verbalized);
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_ordinal_to_string(
        const char *ordinal_string,
        pv_normalizer_token_gender_t gender,
        char **string) {
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
        pv_normalizer_util_string_number_greater_than_int(number_string, MAX_VERBALIZED_CARDINAL_ES)) {
        free(number_string);
        return PV_STATUS_SUCCESS;
    }

    // handle case for '1er' and '3er' abbreviation ordinals
    if ((ordinal_string_length == 3) &&
        (ordinal_string[ordinal_string_length - 2] == 'E' ||
         ordinal_string[ordinal_string_length - 2] == 'e') &&
        (ordinal_string[ordinal_string_length - 1] == 'R' ||
         ordinal_string[ordinal_string_length - 1] == 'r')) {
        char *ordinal = NULL;
        if (*number_string == '1') {
            ordinal = "PRIMER";
        } else if (*number_string == '3') {
            ordinal = "TERCER";
        }

        *string = calloc((int32_t) strlen(ordinal) + 1, sizeof(char));
        if (!(*string)) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("string"));
            free(number_string);
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcat(*string, ordinal);

        free(number_string);

        return PV_STATUS_SUCCESS;
    }

    char *cardinal_string = NULL;

    pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(
            number_string,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            false,
            false,
            &cardinal_string);
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
    while ((k < PV_ARRAY_LEN(ORDINAL_KEYS_ES)) && (strcmp(ORDINAL_KEYS_ES[k], last_cardinal) != 0)) {
        k++;
    }

    free(last_cardinal);

    if (k >= PV_ARRAY_LEN(ORDINAL_KEYS_ES)) {
        free(cardinal_string);
        return PV_STATUS_SUCCESS;
    }
    const char *last_ordinal = (gender == PV_NORMALIZER_TOKEN_GENDER_FEMININE) ? ORDINAL_VALUES_FEMININE_ES[k] : ORDINAL_VALUES_MASCULINE_ES[k];

    if (strncmp(cardinal_string, "UN", i - 1) == 0) {
        // ignore "UN"
        i = 0;
    }

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

    if (token->tag == PV_NORMALIZER_TAG_ES_NEGATIVE_ORDINAL) {
        char *verbalized = NULL;
        const char *digit_string = token->string + 1;
        pv_status_t status = pv_normalizer_verbalizer_ordinal_to_string(
                digit_string,
                token->gender,
                &verbalized);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_cardinal_to_string,
                    pv_status_to_string(status));
            return status;
        }

        const char *negative = (token->gender == PV_NORMALIZER_TOKEN_GENDER_FEMININE) ? " NEGATIVA" : " NEGATIVO";
        char *token_verbalized = calloc(strlen(verbalized) + strlen(negative) + 1, sizeof(char));
        if (!token_verbalized) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("token_verbalized"));
            free(verbalized);
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(token_verbalized, verbalized);
        strcat(token_verbalized, negative);
        free(verbalized);
        pv_normalizer_token_set_verbalized(token, token_verbalized);
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_special_character(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_ES_SPECIAL_CHARACTER) {
        int32_t i = 0;
        for (i = 0; i < PV_ARRAY_LEN(PV_NORMALIZER_SPECIAL_CHARACTERS_ES); i++) {
            int32_t num_bytes = 0;
            pv_status_t status = pv_language_num_bytes_character(PV_NORMALIZER_SPECIAL_CHARACTERS_ES[i][0], &num_bytes);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_language_num_bytes_character,
                        pv_status_to_string(status));
                return status;
            }
            if (strcmp(token->string, PV_NORMALIZER_SPECIAL_CHARACTERS_ES[i]) == 0) {
                break;
            }
        }
        const char *verbalized = PV_NORMALIZER_SPECIAL_CHARACTERS_VERBALIZED_ES[i];

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

        if (PV_NORMALIZER_SPECIAL_CHARACTERS_IS_VERBALIZED_TO_PUNCTUATION_ES[i]) {
            token->tag = PV_NORMALIZER_TAG_ES_PUNCTUATION;
        }
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_decimal(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_ES_DECIMAL_POINT) {
        const char *comma = "COMA";
        char *token_verbalized = calloc(strlen(comma) + 1, sizeof(char));
        if (!token_verbalized) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("token_verbalized"));
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(token_verbalized, comma);
        pv_normalizer_token_set_verbalized(token, token_verbalized);
    } else if (token->tag == PV_NORMALIZER_TAG_ES_DECIMAL_DIGITS) {
        char *verbalized = NULL;
        pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(
                token->string,
                token->gender,
                false,
                false,
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

static pv_status_t pv_normalizer_verbalizer_verbalize_measurement(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_ES_MEASUREMENT) {
        pv_normalizer_token_t *previous = pv_normalizer_token_get_nth_token_before(token, 1, true);

        const char *verbalized = NULL;
        if ((previous != NULL) &&
            (((previous->tag == PV_NORMALIZER_TAG_ES_CARDINAL) &&
              (strcmp(previous->string, "1") == 0)) ||
             previous->tag == PV_NORMALIZER_TAG_ES_POR_SLASH)) {
            verbalized = PV_NORMALIZER_MEASUREMENTS_VERBALIZED_SINGULAR_ES[token->tag_data_index];
        } else {
            verbalized = PV_NORMALIZER_MEASUREMENTS_VERBALIZED_PLURAL_ES[token->tag_data_index];
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
    if (token->tag == PV_NORMALIZER_TAG_ES_POR_SLASH) {
        const char *por = "POR";

        char *token_verbalized = calloc(strlen(por) + 1, sizeof(char));
        if (!token_verbalized) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("token_verbalized"));
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(token_verbalized, por);
        pv_normalizer_token_set_verbalized(token, token_verbalized);
    }
    if (token->tag == PV_NORMALIZER_TAG_ES_MEASUREMENT) {
        pv_normalizer_token_t *previous = pv_normalizer_token_get_nth_token_before(token, 1, true);
        if ((previous != NULL) && (previous->tag == PV_NORMALIZER_TAG_ES_MEASUREMENT)) {
            const char *prev_verbalized = PV_NORMALIZER_MEASUREMENTS_VERBALIZED_SINGULAR_ES[previous->tag_data_index];

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
        else if ((previous != NULL) && (previous->tag == PV_NORMALIZER_TAG_ES_DOT)) {
            pv_normalizer_token_t *two_previous = pv_normalizer_token_get_nth_token_before(token, 2, true);
            if ((two_previous != NULL) && (two_previous->tag == PV_NORMALIZER_TAG_ES_MEASUREMENT)) {
                const char *prev_verbalized = PV_NORMALIZER_MEASUREMENTS_VERBALIZED_SINGULAR_ES[two_previous->tag_data_index];

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
        pv_normalizer_token_gender_t gender,
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
            pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(
                    character,
                    gender,
                    false,
                    false,
                    &verbalized_internal);
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
    pv_status_t status = pv_normalizer_verbalizer_cardinal_spell_out_helper(token->string, true, &length, token->gender, NULL);
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

    status = pv_normalizer_verbalizer_cardinal_spell_out_helper(token->string, false, NULL, token->gender, verbalized);
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

    if (token->tag == PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT) {
        pv_status_t status = pv_normalizer_verbalizer_cardinal_spell_out(token);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_cardinal_spell_out,
                    pv_status_to_string(status));
            return status;
        }
    } else if (token->tag == PV_NORMALIZER_TAG_ES_LETTER_SPELL_OUT) { // pronounced later in phonemizer
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

static pv_status_t pv_normalizer_verbalizer_get_time_string(
        pv_normalizer_token_t *token,
        int32_t max_lookups,
        bool is_pm,
        char **time_string) {
    int32_t i = 0;
    pv_normalizer_token_t *curr = token;

    *time_string = NULL;

    while (curr != NULL && i < max_lookups) {
        if (curr->tag == PV_NORMALIZER_TAG_ES_TIME_HOURS || curr->tag == PV_NORMALIZER_TAG_ES_CARDINAL) {
            int32_t hours = atoi(curr->string);
            if (hours > 24) {
                break;
            }
            if (hours > 12) {
                is_pm = true;
                hours -= 12;
            }

            if (!is_pm && (hours >= 1 && hours < 6)) {
                char *dawn = "DE LA MADRUGADA";
                *time_string = calloc(strlen(dawn) + 1, sizeof(char));
                if (!*time_string) {
                    return PV_STATUS_OUT_OF_MEMORY;
                }
                memcpy(*time_string, dawn, strlen(dawn));
                return PV_STATUS_SUCCESS;
            } else if (!is_pm && (hours >= 6 && hours < 12)) {
                char *morning = "DE LA MAÑANA";
                *time_string = calloc(strlen(morning) + 1, sizeof(char));
                if (!*time_string) {
                    return PV_STATUS_OUT_OF_MEMORY;
                }
                memcpy(*time_string, morning, strlen(morning));
            } else if (is_pm && ((hours > 0 && hours < 7) || hours == 12)) {
                char *afternoon = "DE LA TARDE";
                *time_string = calloc(strlen(afternoon) + 1, sizeof(char));
                if (!*time_string) {
                    return PV_STATUS_OUT_OF_MEMORY;
                }
                memcpy(*time_string, afternoon, strlen(afternoon));
            } else {
                char *night = "DE LA NOCHE";
                *time_string = calloc(strlen(night) + 1, sizeof(char));
                if (!*time_string) {
                    return PV_STATUS_OUT_OF_MEMORY;
                }
                memcpy(*time_string, night, strlen(night));
            }
            return PV_STATUS_SUCCESS;
        }

        curr = curr->previous;
        i++;
    }

    char *horas = (is_pm) ? "PM" : "AM";
    *time_string = calloc(strlen(horas) + 1, sizeof(char));
    if (!*time_string) {
        return PV_STATUS_OUT_OF_MEMORY;
    }
    memcpy(*time_string, horas, strlen(horas));

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_time(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_ES_TIME_COLON || token->tag == PV_NORMALIZER_TAG_ES_TIME_H) {
        pv_normalizer_token_set_verbalized(token, NULL);
    } else if (token->tag == PV_NORMALIZER_TAG_ES_TIME_HOURS) {
        const char *number_string = token->string;

        int32_t offset = 0;
        if (number_string[0] == '0' && strlen(token->string) > 1) {
            offset = 1;
        }

        char *verbalized = NULL;
        pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(
                token->string + offset,
                token->gender,
                false,
                false,
                &verbalized);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_cardinal_to_string,
                    pv_status_to_string(status));
            return status;
        }
        pv_normalizer_token_set_verbalized(token, verbalized);

    } else if (token->tag == PV_NORMALIZER_TAG_ES_TIME_MINUTES) {
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
                        cardinal_to_string_helper(verbalized, "Y", &length, true);
                    } else {
                        if (digit == 1) {
                            cardinal_to_string_helper(verbalized, "UN MINUTO", &length, true);
                        } else {
                            cardinal_to_string_helper(verbalized, ONE_TO_TWENTY_NINE_ES[(int32_t) digit], &length, true);
                        }
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
                        cardinal_to_string_helper(verbalized, "Y", &length, false);
                    } else {
                        if (digit == 1) {
                            cardinal_to_string_helper(verbalized, "UN MINUTO", &length, false);
                        } else {
                            cardinal_to_string_helper(verbalized, ONE_TO_TWENTY_NINE_ES[(int32_t) digit], &length, false);
                        }
                    }
                    if (*(number_string + 1) != '\0') {
                        cardinal_to_string_helper(verbalized, " ", &length, false);
                    }
                    number_string++;
                }
                pv_normalizer_token_set_verbalized(token, verbalized);
            }
        } else {
            char *verbalized = NULL;
            pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(
                    token->string,
                    token->gender,
                    false,
                    false,
                    &(verbalized));
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_verbalizer_cardinal_to_string,
                        pv_status_to_string(status));
                return status;
            }

            char *and = "Y ";
            char *token_verbalized = calloc(strlen(and) + strlen(verbalized) + 1, sizeof(char));
            if (!token_verbalized) {
                PV_ERROR_REPORT(
                        &pv_error_msg_alloc,
                        PV_ERROR_ARGS_PUBLIC_EMPTY(),
                        PV_ERROR_ARGS_PRIVATE("token_verbalized"));
                free(verbalized);
                return PV_STATUS_OUT_OF_MEMORY;
            }
            strcpy(token_verbalized, and);
            strcat(token_verbalized, verbalized);
            free(verbalized);
            pv_normalizer_token_set_verbalized(token, token_verbalized);
        }
    } else if (token->tag == PV_NORMALIZER_TAG_ES_TIME_AM_PM) {
        const char *am_pm_string = token->string;
        bool is_pm = am_pm_string[0] == 'p' || am_pm_string[0] == 'P';

        char *time_string = NULL;

        pv_normalizer_token_t *curr = token->previous;
        if (curr != NULL && curr->tag == PV_NORMALIZER_TAG_ES_SPACE) {
            curr = curr->previous;
        }
        if (curr != NULL && curr->tag == PV_NORMALIZER_TAG_ES_CARDINAL) {
            // 7am or 7 am
            pv_status_t status = pv_normalizer_verbalizer_get_time_string(curr, 1, is_pm, &time_string);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_verbalizer_get_time_string,
                        pv_status_to_string(status));
                return PV_STATUS_OUT_OF_MEMORY;
            }
        } else {
            // 7:00am, 7h00am, 7:00 am
            pv_status_t status = pv_normalizer_verbalizer_get_time_string(curr, 4, is_pm, &time_string);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_verbalizer_get_time_string,
                        pv_status_to_string(status));
                return PV_STATUS_OUT_OF_MEMORY;
            }
        }

        pv_normalizer_token_set_verbalized(token, time_string);
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_fraction_slash(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_ES_FRACTION_SLASH) {
        if ((token->next != NULL) &&
            ((token->next->tag == PV_NORMALIZER_TAG_ES_CARDINAL) ||
             (token->next->tag == PV_NORMALIZER_TAG_ES_NEGATIVE_CARDINAL))) {
            bool next_is_decimal =
                    ((token->next->next != NULL) &&
                     (token->next->next->tag == PV_NORMALIZER_TAG_ES_DECIMAL_POINT));
            bool next_is_negative = token->next->tag == PV_NORMALIZER_TAG_ES_NEGATIVE_CARDINAL;

            if (next_is_decimal || next_is_negative) {
                const char *sobre = "SOBRE";
                char *token_verbalized = calloc(strlen(sobre) + 1, sizeof(char));
                if (!token_verbalized) {
                    PV_ERROR_REPORT(
                            &pv_error_msg_alloc,
                            PV_ERROR_ARGS_PUBLIC_EMPTY(),
                            PV_ERROR_ARGS_PRIVATE("token_verbalized"));
                    return PV_STATUS_OUT_OF_MEMORY;
                }
                strcpy(token_verbalized, sobre);
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
             (token->previous->tag == PV_NORMALIZER_TAG_ES_FRACTION_SLASH) &&
             (token->previous->verbalized == NULL));

    // Certain cardinal denominators have ordinal verbalizations eg. 1/3 -> UN TERCIO, UNA TERCIA
    if ((token->tag == PV_NORMALIZER_TAG_ES_CARDINAL) && is_ordinal_denominator) {
        char *string = token->string;

        bool is_ordinal = false;
        int32_t index = 0;
        for (int32_t i = 0; i < PV_ARRAY_LEN(DENOMINATOR_ORDINAL_KEYS_ES); i++) {
            if (strcmp(string, DENOMINATOR_ORDINAL_KEYS_ES[i]) == 0) {
                is_ordinal = true;
                index = i;
                break;
            }
        }

        bool is_singular =
                ((token->previous != NULL) &&
                 (token->previous->previous != NULL) &&
                 (strcmp(token->previous->previous->string, "1") == 0));
        char *plural_char = (!is_singular) ? "S" : "";

        if (is_ordinal) {
            const char *verbalized = (token->gender == PV_NORMALIZER_TOKEN_GENDER_FEMININE) ? DENOMINATOR_ORDINAL_VALUES_FEMININE_ES[index] : DENOMINATOR_ORDINAL_VALUES_MASCULINE_ES[index];

            char *token_verbalized = calloc(strlen(verbalized) + strlen(plural_char) + 1, sizeof(char));
            if (!token_verbalized) {
                PV_ERROR_REPORT(
                        &pv_error_msg_alloc,
                        PV_ERROR_ARGS_PUBLIC_EMPTY(),
                        PV_ERROR_ARGS_PRIVATE("token_verbalized"));
                return PV_STATUS_OUT_OF_MEMORY;
            }
            strcpy(token_verbalized, verbalized);
            strcat(token_verbalized, plural_char);
            pv_normalizer_token_set_verbalized(token, token_verbalized);
        } else {
            char *number_string = NULL;
            char *avo = (token->gender == PV_NORMALIZER_TOKEN_GENDER_FEMININE) ? "AVA" : "AVO";
            pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(
                    token->string,
                    PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
                    false,
                    true,
                    &number_string);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_verbalizer_cardinal_to_string,
                        pv_status_to_string(status));
                return status;
            }

            char *token_verbalized = calloc(strlen(number_string) + strlen(avo) + strlen(plural_char) + 1, sizeof(char));
            if (!token_verbalized) {
                PV_ERROR_REPORT(
                        &pv_error_msg_alloc,
                        PV_ERROR_ARGS_PUBLIC_EMPTY(),
                        PV_ERROR_ARGS_PRIVATE("token_verbalized"));
                free(number_string);
                return PV_STATUS_OUT_OF_MEMORY;
            }
            strcpy(token_verbalized, number_string);
            strcat(token_verbalized, avo);
            strcat(token_verbalized, plural_char);
            free(number_string);
            pv_normalizer_token_set_verbalized(token, token_verbalized);
        }
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_dot(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_ES_DOT) {
        pv_normalizer_token_t *previous = pv_normalizer_token_get_nth_token_before(token, 1, true);
        if ((previous != NULL) && (previous->tag == PV_NORMALIZER_TAG_ES_MEASUREMENT)) {
            return PV_STATUS_SUCCESS;
        }

        const char *dot = "PUNTO";

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

    if (token->tag == PV_NORMALIZER_TAG_ES_COLON) {
        const char *colon = "DOS PUNTOS";

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

    if (token->tag == PV_NORMALIZER_TAG_ES_TOP_LEVEL_DOMAIN) {
        PV_ASSERT(!PV_NORMALIZER_TOP_LEVEL_DOMAINS_IS_SPELL_OUT_ES[token->tag_data_index]);

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

    if (token->tag == PV_NORMALIZER_TAG_ES_CURRENCY) {
        char *string = token->string;
        int32_t length = (int32_t) strlen(string);

        char *space = " ";
        char *con = " CON ";

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
                token->gender,
                false,
                true,
                &verbalized_number_string);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_cardinal_to_string,
                    pv_status_to_string(status));
            free(number_string);
            return status;
        }

        const char *currency_string = PV_NORMALIZER_CURRENCIES_VERBALIZED_PLURAL_ES[token->tag_data_index];
        if (strcmp(number_string, "1") == 0) {
            currency_string = PV_NORMALIZER_CURRENCIES_VERBALIZED_SINGULAR_ES[token->tag_data_index];
        }

        free(number_string);

        bool has_no_decimal = ((number_end_index == length) || (string[number_end_index] != ','));
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

        // .00 is not verbalized e.g $1,00 -> one dollar
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
            // ,0X is verbalized as X e.g. $1,05 -> one dollar and five cents
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
                token->gender,
                false,
                true,
                &verbalized_digit_string);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_cardinal_to_string,
                    pv_status_to_string(status));
            free(verbalized_number_string);
            free(digit_string);
            return status;
        }

        const char *sub_currency_string = PV_NORMALIZER_SUB_CURRENCIES_VERBALIZED_PLURAL_ES[token->tag_data_index];
        if (strcmp(digit_string, "1") == 0) {
            sub_currency_string = PV_NORMALIZER_SUB_CURRENCIES_VERBALIZED_SINGULAR_ES[token->tag_data_index];
        }

        free(digit_string);

        int32_t verbalized_length =
                (int32_t) strlen(verbalized_number_string) +
                (int32_t) strlen(space) +
                (int32_t) strlen(currency_string) +
                (int32_t) strlen(con) +
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
        strcat(token_verbalized, con);
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

    if (token->tag == PV_NORMALIZER_TAG_ES_NEGATIVE_CURRENCY) {
        char *string = token->string;
        int32_t length = (int32_t) strlen(string);

        char *negative = "NEGATIVO ";
        char *space = " ";
        char *con = " CON ";

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
                token->gender,
                false,
                true,
                &verbalized_number_string);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_cardinal_to_string,
                    pv_status_to_string(status));
            free(number_string);
            return status;
        }

        const char *currency_string = PV_NORMALIZER_CURRENCIES_VERBALIZED_PLURAL_ES[token->tag_data_index];
        if (strcmp(number_string, "1") == 0) {
            currency_string = PV_NORMALIZER_CURRENCIES_VERBALIZED_SINGULAR_ES[token->tag_data_index];
        }

        free(number_string);

        bool has_no_decimal = ((number_end_index == length) || (string[number_end_index] != ','));
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
                token->gender,
                false,
                true,
                &verbalized_digit_string);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_cardinal_to_string,
                    pv_status_to_string(status));
            free(verbalized_number_string);
            free(digit_string);
            return status;
        }

        const char *sub_currency_string = PV_NORMALIZER_SUB_CURRENCIES_VERBALIZED_PLURAL_ES[token->tag_data_index];
        if (strcmp(digit_string, "1") == 0) {
            currency_string = PV_NORMALIZER_SUB_CURRENCIES_VERBALIZED_SINGULAR_ES[token->tag_data_index];
        }

        free(digit_string);

        int32_t verbalized_length =
                (int32_t) strlen(negative) +
                (int32_t) strlen(verbalized_number_string) +
                (int32_t) strlen(space) +
                (int32_t) strlen(currency_string) +
                (int32_t) strlen(con) +
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
        strcat(token_verbalized, con);
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

    if (token->tag == PV_NORMALIZER_TAG_ES_CURRENCY_SYMBOL) {
        pv_normalizer_token_t *previous = pv_normalizer_token_get_nth_token_before(token, 1, true);
        if ((previous == NULL) || (token->tag_data_index < 0)) {
            return PV_STATUS_SUCCESS;
        }

        char *string = previous->string;
        const char *currency_string = PV_NORMALIZER_CURRENCIES_VERBALIZED_PLURAL_ES[token->tag_data_index];
        if (strcmp(string, "1") == 0) {
            currency_string = PV_NORMALIZER_CURRENCIES_VERBALIZED_SINGULAR_ES[token->tag_data_index];
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

    if (token->tag == PV_NORMALIZER_TAG_ES_ABBREVIATION) {
        PV_ASSERT((token->tag_data_index >= 0) && (token->tag_data_index < PV_ARRAY_LEN(PV_NORMALIZER_ABBREVIATIONS_VERBALIZED_ES)));
        const char *verbalized = PV_NORMALIZER_ABBREVIATIONS_VERBALIZED_ES[token->tag_data_index];
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

    if (token->tag == PV_NORMALIZER_TAG_ES_DIGITS_WITH_PARENTHESES) {
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
        pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(
                number_string,
                token->gender,
                true,
                false,
                &verbalized);
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
    } else if (token->tag == PV_NORMALIZER_TAG_ES_DIGITS) {
        char *verbalized = NULL;
        pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(
                string,
                token->gender,
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
    } else if (token->tag == PV_NORMALIZER_TAG_ES_DIGITS_SEPARATOR) {
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
        token->tag = PV_NORMALIZER_TAG_ES_PUNCTUATION;
    }

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
                (fourth_neighbor->tag == PV_NORMALIZER_TAG_ES_DATE_YEAR) &&
                (third_neighbor->tag == PV_NORMALIZER_TAG_ES_DATE_SEPARATOR) &&
                (second_neighbor->tag == PV_NORMALIZER_TAG_ES_DATE_MONTH) &&
                (neighbor->tag == PV_NORMALIZER_TAG_ES_DATE_SEPARATOR);
    } else {
        pv_normalizer_token_t *neighbor = pv_normalizer_token_get_nth_token_after(token, 1, false);
        pv_normalizer_token_t *second_neighbor = pv_normalizer_token_get_nth_token_after(token, 2, false);
        pv_normalizer_token_t *third_neighbor = pv_normalizer_token_get_nth_token_after(token, 3, false);
        pv_normalizer_token_t *fourth_neighbor = pv_normalizer_token_get_nth_token_after(token, 4, false);
        is_year_first =
                (fourth_neighbor != NULL) &&
                ((fourth_neighbor->tag == PV_NORMALIZER_TAG_ES_DATE_DAY) ||
                 (fourth_neighbor->tag == PV_NORMALIZER_TAG_ES_DATE_MONTH)) &&
                (third_neighbor->tag == PV_NORMALIZER_TAG_ES_DATE_SEPARATOR) &&
                ((second_neighbor->tag == PV_NORMALIZER_TAG_ES_DATE_DAY) ||
                 (second_neighbor->tag == PV_NORMALIZER_TAG_ES_DATE_MONTH)) &&
                (neighbor->tag == PV_NORMALIZER_TAG_ES_DATE_SEPARATOR);
    }

    return is_year_first;
}

pv_status_t pv_normalizer_verbalizer_verbalize_dates(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    const char *of_string = " DE";
    if (token->tag == PV_NORMALIZER_TAG_ES_DATE_SEPARATOR) {
        pv_normalizer_token_set_verbalized(token, NULL);
    } else if (token->tag == PV_NORMALIZER_TAG_ES_DATE_DAY) {
        int32_t offset = (token->string[0] == '0') ? 1 : 0;
        PV_ASSERT(strcmp(token->string + offset, "\0") != 0);
        char *day_string = token->string + offset;

        char *day_verbalized = NULL;
        pv_status_t status;
        if (strcmp(day_string, "1") == 0) {
            char *first_day = "PRIMERO";
            int32_t first_day_length = (int32_t) strlen(first_day);
            day_verbalized = calloc(first_day_length + 1, sizeof(char));
            if (!day_verbalized) {
                PV_ERROR_REPORT(
                        &pv_error_msg_alloc,
                        PV_ERROR_ARGS_PUBLIC_EMPTY(),
                        PV_ERROR_ARGS_PRIVATE("day_verbalized"));
                return PV_STATUS_OUT_OF_MEMORY;
            }

            memcpy(day_verbalized, first_day, first_day_length);
            day_verbalized[first_day_length] = '\0';
        } else {
            status = pv_normalizer_verbalizer_cardinal_to_string(
                    day_string,
                    token->gender,
                    false,
                    false,
                    &day_verbalized);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_verbalizer_cardinal_to_string,
                        pv_status_to_string(status));
                return status;
            }
        }

        pv_normalizer_token_t *previous = pv_normalizer_token_get_nth_token_before(token, 1, true);
        pv_normalizer_token_t *two_previous = pv_normalizer_token_get_nth_token_before(token, 2, true);
        pv_normalizer_token_t *next = pv_normalizer_token_get_nth_token_after(token, 1, true);
        pv_normalizer_token_t *two_next = pv_normalizer_token_get_nth_token_after(token, 2, true);
        bool has_month_before =
                ((previous != NULL) && (previous->tag == PV_NORMALIZER_TAG_ES_DATE_MONTH)) ||
                ((two_previous != NULL) && (two_previous->tag == PV_NORMALIZER_TAG_ES_DATE_MONTH));
        if (has_month_before) {
            int32_t total_length = (int32_t) strlen(day_verbalized);
            pv_normalizer_token_t *month_token;

            if ((previous != NULL) && (previous->tag == PV_NORMALIZER_TAG_ES_DATE_MONTH)) {
                month_token = previous;
            } else {
                month_token = two_previous;
            }
            // verbalized month
            if (month_token->tag_data_index < 0) {
                int32_t number = (int32_t) strtol(month_token->string, NULL, 10);
                if (number < 0) {
                    return PV_STATUS_SUCCESS;
                }
                month_token->tag_data_index = number - 1;
                if (month_token->tag_data_index < 0) {
                    return PV_STATUS_SUCCESS;
                }
            }
            const char *month_verbalized = PV_NORMALIZER_MONTH_NAMES_VERBALIZED_ES[month_token->tag_data_index];
            total_length += (int32_t) strlen(month_verbalized);

            // check year:
            bool is_year_first = pv_normalizer_verbalizer_is_year_first(token, true);
            pv_normalizer_token_t *year_token = pv_normalizer_token_get_nth_token_before(token, 4, true);
            if (is_year_first && (year_token != NULL) && (year_token->tag == PV_NORMALIZER_TAG_ES_DATE_YEAR)) {
                char *year_verbalized = NULL;
                status = pv_normalizer_verbalizer_cardinal_to_string(
                        year_token->string,
                        token->gender,
                        false,
                        false,
                        &year_verbalized);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_year,
                            pv_status_to_string(status));
                    free(day_verbalized);
                    return status;
                }
                total_length += (int32_t) strlen(year_verbalized);
                total_length += (int32_t) strlen(of_string) + (int32_t) strlen(of_string);
                total_length += 2;

                char *token_verbalized = calloc(total_length + 1, sizeof(char));
                if (!token_verbalized) {
                    PV_ERROR_REPORT(
                            &pv_error_msg_alloc,
                            PV_ERROR_ARGS_PUBLIC_EMPTY(),
                            PV_ERROR_ARGS_PRIVATE("token_verbalized"));
                    free(day_verbalized);
                    return PV_STATUS_OUT_OF_MEMORY;
                }

                strcat(token_verbalized, day_verbalized);
                strcat(token_verbalized, of_string);
                strcat(token_verbalized, " ");
                strcat(token_verbalized, month_verbalized);
                strcat(token_verbalized, of_string);
                strcat(token_verbalized, " ");
                strcat(token_verbalized, year_verbalized);
                free(year_verbalized);
                pv_normalizer_token_set_verbalized(token, token_verbalized);
            } else {
                bool has_year_after =
                        ((next != NULL) && (next->tag == PV_NORMALIZER_TAG_ES_DATE_YEAR)) ||
                        ((two_next != NULL) && (two_next->tag == PV_NORMALIZER_TAG_ES_DATE_YEAR));
                if (has_year_after) {
                    total_length += (int32_t) strlen(of_string) + (int32_t) strlen(of_string);
                    total_length += 1;
                    char *token_verbalized = calloc(total_length + 1, sizeof(char));
                    if (!token_verbalized) {
                        PV_ERROR_REPORT(
                                &pv_error_msg_alloc,
                                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                                PV_ERROR_ARGS_PRIVATE("token_verbalized"));
                        free(day_verbalized);
                        return PV_STATUS_OUT_OF_MEMORY;
                    }
                    strcat(token_verbalized, day_verbalized);
                    strcat(token_verbalized, of_string);
                    strcat(token_verbalized, " ");
                    strcat(token_verbalized, month_verbalized);
                    strcat(token_verbalized, of_string);
                    pv_normalizer_token_set_verbalized(token, token_verbalized);
                }
            }
        } else {
            int32_t total_length = (int32_t) strlen(day_verbalized) + (int32_t) strlen(of_string);
            char *token_verbalized = calloc(total_length + 1, sizeof(char));
            if (!token_verbalized) {
                PV_ERROR_REPORT(
                        &pv_error_msg_alloc,
                        PV_ERROR_ARGS_PUBLIC_EMPTY(),
                        PV_ERROR_ARGS_PRIVATE("token_verbalized"));
                free(day_verbalized);
                return PV_STATUS_OUT_OF_MEMORY;
            }
            strcat(token_verbalized, day_verbalized);
            strcat(token_verbalized, of_string);
            pv_normalizer_token_set_verbalized(token, token_verbalized);
        }
        free(day_verbalized);
    } else if (token->tag == PV_NORMALIZER_TAG_ES_DATE_MONTH) {
        pv_normalizer_token_t *next = pv_normalizer_token_get_nth_token_after(token, 1, true);
        pv_normalizer_token_t *two_next = pv_normalizer_token_get_nth_token_after(token, 2, true);
        bool has_year_after =
                ((next != NULL) && (next->tag == PV_NORMALIZER_TAG_ES_DATE_YEAR)) ||
                ((two_next != NULL) && (two_next->tag == PV_NORMALIZER_TAG_ES_DATE_YEAR));
        bool has_day_right_after =
                ((next != NULL) && (next->tag == PV_NORMALIZER_TAG_ES_DATE_SEPARATOR)) &&
                ((two_next != NULL) && (two_next->tag == PV_NORMALIZER_TAG_ES_DATE_DAY));

        if (has_day_right_after) {
            // Handled in the `day token` case.
            pv_normalizer_token_set_verbalized(token, NULL);
        } else {
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
            const char *verbalized_string = PV_NORMALIZER_MONTH_NAMES_VERBALIZED_ES[token->tag_data_index];

            int32_t total_length = (int32_t) strlen(verbalized_string);
            if (has_year_after) {
                total_length += (int32_t) strlen(of_string);
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
                strcat(token_verbalized, of_string);
            }
            pv_normalizer_token_set_verbalized(token, token_verbalized);
        }
    } else if (token->tag == PV_NORMALIZER_TAG_ES_DATE_YEAR) {
        bool is_year_first = pv_normalizer_verbalizer_is_year_first(token, false);

        if (is_year_first) {
            // If year is first, append the year to the end of the date. Handled in the `day token` case.
            pv_normalizer_token_set_verbalized(token, NULL);
        } else {
            char *verbalized = NULL;
            pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(
                    token->string,
                    token->gender,
                    false,
                    false,
                    &verbalized);
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

    if (token->tag == PV_NORMALIZER_TAG_ES_NAME_INITIAL_DOT) {
        pv_normalizer_token_set_verbalized(token, NULL);
    }

    return PV_STATUS_SUCCESS;
}
