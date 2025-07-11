#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "core/picovoice.h"
#include "core/pv_assert.h"
#include "core/pv_error_messages.h"
#include "orca/normalizer/it/pv_normalizer_data_it/pv_normalizer_number_data_it.h"
#include "orca/normalizer/pv_normalizer_token.h"
#include "orca/normalizer/pv_normalizer_use_cases.h"
#include "orca/normalizer/pv_normalizer_verbalizer.h"

#include "orca/normalizer/it/pv_normalizer_data_it/pv_normalizer_data_it.h"
#include "orca/normalizer/it/pv_normalizer_tags_it.h"
#include "orca/normalizer/it/pv_normalizer_util_it.h"
#include "orca/normalizer/it/pv_normalizer_verbalizer_it.h"

#include "lm/pv_noun_gender_dict.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_normalizer_mock.h"

#endif

static void pv_normalizer_verbalizer_synchronize_language_agnostic_tags(pv_normalizer_token_list_t *token_list);
;

static pv_status_t pv_normalizer_verbalizer_verbalize_word(const pv_normalizer_verbalizer_it_t *object, pv_normalizer_token_t *token);

struct pv_normalizer_verbalizer_it {
    int32_t num_use_cases;
    const pv_normalizer_use_cases_it_t *use_cases;
    pv_noun_gender_dict_t *noun_gender_dict;
};

static pv_status_t pv_normalizer_verbalizer_verbalize_punctuation(pv_normalizer_token_t *token, bool *verbalize_previous_as_ordinary_one);

static pv_status_t pv_normalizer_verbalizer_verbalize_custom_pronunciation(pv_normalizer_token_t *token, bool *verbalize_previous_as_ordinary_one);

static pv_status_t pv_normalizer_verbalizer_verbalize_special_character(pv_normalizer_token_t *token, bool *verbalize_previous_as_ordinary_one);

static pv_status_t pv_normalizer_verbalizer_verbalize_abbreviation(pv_normalizer_token_t *token, bool *verbalize_previous_as_ordinary_one);

static pv_status_t pv_normalizer_verbalizer_verbalize_cardinal(pv_normalizer_token_t *token, bool *verbalize_previous_as_ordinary_one);

static pv_status_t pv_normalizer_verbalizer_cardinal_to_string(
        const char *number_string,
        bool to_digit_string,
        char **string);

static pv_status_t pv_normalizer_verbalizer_cardinal_to_string_helper(
        const char *number_string,
        int32_t *length,
        bool to_digit_string,
        bool dry_run,
        char **string);

static void cardinal_to_string_helper(char *dest, const char *src, int32_t *length, bool get_src_length);

static pv_status_t pv_normalizer_verbalizer_verbalize_negative_cardinal(pv_normalizer_token_t *token, bool *verbalize_previous_as_ordinary_one);

static pv_status_t pv_normalizer_verbalizer_verbalize_number_range(pv_normalizer_token_t *token, bool *verbalize_previous_as_ordinary_one);

static pv_status_t pv_normalizer_verbalizer_verbalize_decimal(pv_normalizer_token_t *token, bool *verbalize_previous_as_ordinary_one);

static pv_status_t pv_normalizer_verbalizer_cardinal_spell_out_helper(
        const char *digit_string,
        bool dry_run,
        int32_t *length,
        char *verbalized);

static pv_status_t pv_normalizer_verbalizer_cardinal_spell_out(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_alphanum_spell_out(pv_normalizer_token_t *token, bool *verbalize_previous_as_ordinary_one);

static pv_status_t pv_normalizer_verbalizer_verbalize_measurement(const pv_normalizer_verbalizer_it_t *object, pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_time(pv_normalizer_token_t *token, bool *verbalize_previous_as_ordinary_one);

static pv_status_t pv_normalizer_verbalizer_verbalize_dot(pv_normalizer_token_t *token, bool *verbalize_previous_as_ordinary_one);

static pv_status_t pv_normalizer_verbalizer_verbalize_colon(pv_normalizer_token_t *token, bool *verbalize_previous_as_ordinary_one);

static pv_status_t pv_normalizer_verbalizer_verbalize_top_level_domain(pv_normalizer_token_t *token, bool *verbalize_previous_as_ordinary_one);

static pv_status_t pv_normalizer_verbalizer_verbalize_currency(const pv_normalizer_verbalizer_it_t *object, pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_negative_currency(const pv_normalizer_verbalizer_it_t *object, pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_currency_symbol(const pv_normalizer_verbalizer_it_t *object, pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_verbalize_digits_sequence(pv_normalizer_token_t *token, bool *verbalize_previous_as_ordinary_one);

static pv_status_t pv_normalizer_verbalizer_verbalize_dates(pv_normalizer_token_t *token, bool *verbalize_previous_as_ordinary_one);

static pv_status_t pv_normalizer_verbalizer_verbalize_names(pv_normalizer_token_t *token, bool *verbalize_previous_as_ordinary_one);

static pv_status_t pv_normalizer_verbalizer_verbalize_ordinal(pv_normalizer_token_t *token, bool *verbalize_previous_as_ordinary_one);

static pv_status_t pv_normalizer_verbalizer_ordinal_to_string_masculine(const char *ordinal_string, char **string);

static pv_status_t pv_normalizer_verbalizer_ordinal_to_string_feminine(const char *ordinal_string, char **string);

static pv_status_t pv_normalizer_verbalizer_verbalize_negative_ordinal(pv_normalizer_token_t *token, bool *verbalize_previous_as_ordinary_one);

static pv_status_t pv_normalizer_verbalizer_verbalize_fraction_slash(pv_normalizer_token_t *token, bool *verbalize_previous_as_ordinary_one);

static pv_status_t pv_normalizer_verbalizer_verbalize_fraction_denominator(pv_normalizer_token_t *token);

static const char *pv_normalizer_verbalizer_get_previous_ordinary_one_or_negative_one_string(const pv_normalizer_token_t *token);

static pv_normalizer_token_t *pv_normalizer_verbalizer_get_previous_ordinary_one_or_negative_one_token(const pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_verbalizer_prepend_to_reverbalize(pv_normalizer_token_t *token, char **prefix, bool no_space, bool free_prefix);

static pv_status_t pv_normalizer_verbalizer_get_one_or_negative_one_variations(
        const pv_normalizer_verbalizer_it_t *object,
        const char *noun,
        bool negative,
        char **result,
        bool *no_space);

static pv_status_t pv_normalizer_verbalizer_verbalize_previous_one_or_negative_one(
        const pv_normalizer_token_t *curr,
        char **string,
        bool free_string);


pv_status_t PV_MOCKABLE(pv_normalizer_verbalizer_it_init)(
        int32_t num_use_cases,
        const pv_normalizer_use_cases_it_t *use_cases,
        pv_noun_gender_dict_t *noun_gender_dict,
        pv_normalizer_verbalizer_it_t **object) {
    PV_ASSERT(use_cases);
    PV_ASSERT(noun_gender_dict);
    PV_ASSERT(object);

    *object = NULL;

    pv_normalizer_verbalizer_it_t *o = (pv_normalizer_verbalizer_it_t *) calloc(1, sizeof(pv_normalizer_verbalizer_it_t));
    if (!o) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->num_use_cases = num_use_cases;
    o->use_cases = use_cases;
    o->noun_gender_dict = noun_gender_dict;

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_normalizer_verbalizer_it_delete)(pv_normalizer_verbalizer_it_t *object) {
    if (object) {
        free(object);
    }
}

pv_status_t PV_MOCKABLE(pv_normalizer_verbalizer_it_verbalize)(
        pv_normalizer_verbalizer_it_t *object,
        pv_normalizer_token_list_t *token_list,
        int32_t num_tokens_skip) {
    PV_ASSERT(object);
    PV_ASSERT(token_list);
    PV_ASSERT(num_tokens_skip >= 0);

    pv_normalizer_token_t *token = token_list->head;
    int32_t i = 0;
    bool verbalize_previous_as_ordinary_one = false;
    while (token != NULL) {
        if (i < num_tokens_skip) {
            token = token->next;
            i++;
            continue;
        }
        verbalize_previous_as_ordinary_one = false;

        for (int32_t j = 0; j < object->num_use_cases; j++) {
            switch (object->use_cases[j]) {
                case PV_NORMALIZER_USE_WORD_NORMALIZER_IT: {
                    pv_status_t status = pv_normalizer_verbalizer_verbalize_word(object, token);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_normalizer_verbalizer_verbalize_word,
                                pv_status_to_string(status));
                        return status;
                    }
                } break;
                case PV_NORMALIZER_USE_PUNCTUATION_NORMALIZER_IT: {
                    pv_status_t status = pv_normalizer_verbalizer_verbalize_punctuation(token, &verbalize_previous_as_ordinary_one);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_normalizer_verbalizer_verbalize_punctuation,
                                pv_status_to_string(status));
                        return status;
                    }
                } break;
                case PV_NORMALIZER_USE_CUSTOM_PRONUNCIATION_NORMALIZER_IT: {
                    pv_status_t status = pv_normalizer_verbalizer_verbalize_custom_pronunciation(token, &verbalize_previous_as_ordinary_one);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_normalizer_verbalizer_verbalize_custom_pronunciation,
                                pv_status_to_string(status));
                        return status;
                    }
                } break;
                case PV_NORMALIZER_USE_SPECIAL_CHARACTER_NORMALIZER_IT: {
                    pv_status_t status = pv_normalizer_verbalizer_verbalize_special_character(token, &verbalize_previous_as_ordinary_one);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_normalizer_verbalizer_verbalize_special_character,
                                pv_status_to_string(status));
                        return status;
                    }
                } break;
                case PV_NORMALIZER_USE_ABBREVIATION_NORMALIZER_IT: {
                    pv_status_t status = pv_normalizer_verbalizer_verbalize_abbreviation(token, &verbalize_previous_as_ordinary_one);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_normalizer_verbalizer_verbalize_abbreviation,
                                pv_status_to_string(status));
                        return status;
                    }
                } break;
                case PV_NORMALIZER_USE_CARDINAL_NORMALIZER_IT: {
                    if (pv_normalizer_token_get_nth_token_after(token, 1, true) == NULL && token->tag == PV_NORMALIZER_TAG_IT_CARDINAL_ONE) {
                        // Last token that is special "1" needs to be verbalized.
                        token->tag = PV_NORMALIZER_TAG_IT_CARDINAL;
                    }

                    pv_status_t status = pv_normalizer_verbalizer_verbalize_cardinal(token, &verbalize_previous_as_ordinary_one);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_normalizer_verbalizer_verbalize_cardinal,
                                pv_status_to_string(status));
                        return status;
                    }
                } break;
                case PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_IT: {
                    if (pv_normalizer_token_get_nth_token_after(token, 1, true) == NULL && token->tag == PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL_ONE) {
                        // Last token that is special "1" needs to be verbalized.
                        token->tag = PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL;
                    }

                    pv_status_t status = pv_normalizer_verbalizer_verbalize_negative_cardinal(token, &verbalize_previous_as_ordinary_one);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_normalizer_verbalizer_verbalize_negative_cardinal,
                                pv_status_to_string(status));
                        return status;
                    }
                } break;
                case PV_NORMALIZER_USE_NUMBER_RANGE_NORMALIZER_IT: {
                    pv_status_t status = pv_normalizer_verbalizer_verbalize_number_range(token, &verbalize_previous_as_ordinary_one);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_normalizer_verbalizer_verbalize_number_range,
                                pv_status_to_string(status));
                        return status;
                    }
                } break;
                case PV_NORMALIZER_USE_DECIMAL_NORMALIZER_IT: {
                    pv_status_t status = pv_normalizer_verbalizer_verbalize_decimal(token, &verbalize_previous_as_ordinary_one);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_normalizer_verbalizer_verbalize_decimal,
                                pv_status_to_string(status));
                        return status;
                    }
                } break;
                case PV_NORMALIZER_USE_ALPHANUM_SPELL_OUT_NORMALIZER_IT: {
                    pv_status_t status = pv_normalizer_verbalizer_verbalize_alphanum_spell_out(token, &verbalize_previous_as_ordinary_one);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_normalizer_verbalizer_verbalize_alphanum_spell_out,
                                pv_status_to_string(status));
                        return status;
                    }
                } break;
                case PV_NORMALIZER_USE_MEASUREMENT_NORMALIZER_IT: {
                    pv_status_t status = pv_normalizer_verbalizer_verbalize_measurement(object, token);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_normalizer_verbalizer_verbalize_measurement,
                                pv_status_to_string(status));
                        return status;
                    }
                } break;
                case PV_NORMALIZER_USE_TIME_NORMALIZER_IT: {
                    pv_status_t status = pv_normalizer_verbalizer_verbalize_time(token, &verbalize_previous_as_ordinary_one);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_normalizer_verbalizer_verbalize_time,
                                pv_status_to_string(status));
                        return status;
                    }
                } break;
                case PV_NORMALIZER_USE_URL_NORMALIZER_IT: {
                    pv_status_t status = pv_normalizer_verbalizer_verbalize_dot(token, &verbalize_previous_as_ordinary_one);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_normalizer_verbalizer_verbalize_dot,
                                pv_status_to_string(status));
                        return status;
                    }

                    status = pv_normalizer_verbalizer_verbalize_colon(token, &verbalize_previous_as_ordinary_one);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_normalizer_verbalizer_verbalize_colon,
                                pv_status_to_string(status));
                        return status;
                    }

                    status = pv_normalizer_verbalizer_verbalize_top_level_domain(token, &verbalize_previous_as_ordinary_one);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_normalizer_verbalizer_verbalize_top_level_domain,
                                pv_status_to_string(status));
                        return status;
                    }
                } break;
                case PV_NORMALIZER_USE_CURRENCY_NORMALIZER_IT: {
                    pv_status_t status = pv_normalizer_verbalizer_verbalize_currency(object, token);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_normalizer_verbalizer_verbalize_currency,
                                pv_status_to_string(status));
                        return status;
                    }

                    status = pv_normalizer_verbalizer_verbalize_negative_currency(object, token);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_normalizer_verbalizer_verbalize_negative_currency,
                                pv_status_to_string(status));
                        return status;
                    }

                    status = pv_normalizer_verbalizer_verbalize_currency_symbol(object, token);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_normalizer_verbalizer_verbalize_currency_symbol,
                                pv_status_to_string(status));
                        return status;
                    }
                } break;
                case PV_NORMALIZER_USE_DIGITS_SEQUENCE_NORMALIZER_IT: {
                    pv_status_t status = pv_normalizer_verbalizer_verbalize_digits_sequence(token, &verbalize_previous_as_ordinary_one);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_normalizer_verbalizer_verbalize_digits_sequence,
                                pv_status_to_string(status));
                        return status;
                    }
                } break;
                case PV_NORMALIZER_USE_DATE_NORMALIZER_IT: {
                    pv_status_t status = pv_normalizer_verbalizer_verbalize_dates(token, &verbalize_previous_as_ordinary_one);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_normalizer_verbalizer_verbalize_dates,
                                pv_status_to_string(status));
                        return status;
                    }
                } break;
                case PV_NORMALIZER_USE_NAME_NORMALIZER_IT: {
                    pv_status_t status = pv_normalizer_verbalizer_verbalize_names(token, &verbalize_previous_as_ordinary_one);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_normalizer_verbalizer_verbalize_names,
                                pv_status_to_string(status));
                        return status;
                    }
                } break;
                case PV_NORMALIZER_USE_ORDINAL_NORMALIZER_IT: {
                    pv_status_t status = pv_normalizer_verbalizer_verbalize_ordinal(token, &verbalize_previous_as_ordinary_one);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_normalizer_verbalizer_verbalize_ordinal,
                                pv_status_to_string(status));
                        return status;
                    }
                } break;
                case PV_NORMALIZER_USE_NEGATIVE_ORDINAL_NORMALIZER_IT: {
                    pv_status_t status = pv_normalizer_verbalizer_verbalize_negative_ordinal(token, &verbalize_previous_as_ordinary_one);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_normalizer_verbalizer_verbalize_negative_ordinal,
                                pv_status_to_string(status));
                        return status;
                    }
                } break;
                case PV_NORMALIZER_USE_FRACTION_NORMALIZER_IT: {
                    pv_status_t status = pv_normalizer_verbalizer_verbalize_fraction_slash(token, &verbalize_previous_as_ordinary_one);
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
                } break;
                default:
                    return PV_STATUS_INVALID_ARGUMENT;
            }
        }

        // Below added the case where the current token is the special "1" or "-1", because in case when the token sequence is "1", "1", "1", ... for example, then none except the last one is verbalized.
        if (verbalize_previous_as_ordinary_one ||
            token->tag == PV_NORMALIZER_TAG_IT_CARDINAL_ONE ||
            token->tag == PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL_ONE) {
            // Verbalize the previous "1" or "-1" as ordinary one, if they exist.

            const char *string_one_or_negative_one = pv_normalizer_verbalizer_get_previous_ordinary_one_or_negative_one_string(token);
            if (string_one_or_negative_one != NULL) {
                char *string_one_or_negative_one_calloc = (char *) calloc(strlen(string_one_or_negative_one) + 1, sizeof(char));
                if (!string_one_or_negative_one) {
                    PV_ERROR_REPORT(
                            &pv_error_msg_alloc,
                            PV_ERROR_ARGS_PUBLIC_EMPTY(),
                            PV_ERROR_ARGS_PRIVATE("string_one_or_negative_one"));
                    return PV_STATUS_OUT_OF_MEMORY;
                }
                strcpy(string_one_or_negative_one_calloc, string_one_or_negative_one);
                string_one_or_negative_one_calloc[strlen(string_one_or_negative_one)] = '\0';

                pv_status_t status = pv_normalizer_verbalizer_verbalize_previous_one_or_negative_one(token, &string_one_or_negative_one_calloc, true);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_previous_one_or_negative_one,
                            pv_status_to_string(status));
                    free(string_one_or_negative_one_calloc);
                    return status;
                }
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
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_PUNCTUATION,
            PV_NORMALIZER_TAG_IT_SINGLE_QUOTE,
            PV_NORMALIZER_TAG_IT_CUSTOM_PRONUNCIATION,
            PV_NORMALIZER_TAG_IT_LETTER_SPELL_OUT);
}

static pv_status_t pv_normalizer_verbalizer_verbalize_word(const pv_normalizer_verbalizer_it_t *object, pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_IT_WORD) {
        pv_status_t status = pv_normalizer_verbalizer_verbalize_word_common(token);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_verbalize_word_common,
                    pv_status_to_string(status));
            // Design decision: if the verbalization of the current token isn't successful, then will NOT bother to verbalize the previous oridinary "1" or "-1".
            return status;
        }

        bool found_previous_one = pv_normalizer_util_it_found_previous_one(token);
        bool found_previous_negative_one = false;
        if (!found_previous_one) {
            found_previous_negative_one = pv_normalizer_util_it_found_previous_negative_one(token);
        }
        bool negative = false;
        bool prepend_to_reverbalize = false;
        if (found_previous_one) {
            prepend_to_reverbalize = true;
            negative = false;
        } else if (found_previous_negative_one) {
            prepend_to_reverbalize = true;
            negative = true;
        }
        if (prepend_to_reverbalize) {
            char *prefix = NULL;
            bool no_space = false;
            status = pv_normalizer_verbalizer_get_one_or_negative_one_variations(object, token->verbalized, negative, &prefix, &no_space);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_verbalizer_get_one_or_negative_one_variations,
                        pv_status_to_string(status));
                return status;
            }
            if (strcmp(prefix, "UN'") == 0 || strcmp(prefix, "MENO UN'") == 0) {
                // In this case, we concatenate the "1" or "-1" with the noun. E.g. "1 automobile" --> "UN'AUTOMOBILE" as opposed to "UN' AUTOMOBILE".
                status = pv_normalizer_verbalizer_prepend_to_reverbalize(token, &prefix, no_space, true);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_prepend_to_reverbalize,
                            pv_status_to_string(status));
                    free(prefix);
                    return status;
                }
            } else {
                status = pv_normalizer_verbalizer_verbalize_previous_one_or_negative_one(token, &prefix, true);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_previous_one_or_negative_one,
                            pv_status_to_string(status));
                    free(prefix);
                    return status;
                }
            }
        }
    }
    return PV_STATUS_SUCCESS;
}


pv_status_t pv_normalizer_verbalizer_verbalize_punctuation(pv_normalizer_token_t *token, bool *verbalize_previous_as_ordinary_one) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_IT_PUNCTUATION) {
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
        *verbalize_previous_as_ordinary_one = true;
    }

    if (token->tag == PV_NORMALIZER_TAG_IT_SINGLE_QUOTE) {
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

pv_status_t pv_normalizer_verbalizer_verbalize_custom_pronunciation(pv_normalizer_token_t *token, bool *verbalize_previous_as_ordinary_one) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_IT_CUSTOM_PRONUNCIATION) {
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
        *verbalize_previous_as_ordinary_one = true;
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_special_character(pv_normalizer_token_t *token, bool *verbalize_previous_as_ordinary_one) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_IT_SPECIAL_CHARACTER) {
        int32_t i = 0;
        for (i = 0; i < PV_ARRAY_LEN(PV_NORMALIZER_SPECIAL_CHARACTERS_IT); i++) {
            int32_t num_bytes = 0;
            pv_status_t status = pv_language_num_bytes_character(PV_NORMALIZER_SPECIAL_CHARACTERS_IT[i][0], &num_bytes);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_language_num_bytes_character,
                        pv_status_to_string(status));
                return status;
            }
            if (strcmp(token->string, PV_NORMALIZER_SPECIAL_CHARACTERS_IT[i]) == 0) {
                break;
            }
        }
        const char *verbalized_special_character = PV_NORMALIZER_SPECIAL_CHARACTERS_VERBALIZED_IT[i];
        char *token_verbalized = calloc(strlen(verbalized_special_character) + 1, sizeof(char));
        if (!token_verbalized) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("token_verbalized"));
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(token_verbalized, verbalized_special_character);
        pv_normalizer_token_set_verbalized(token, token_verbalized);

        if (PV_NORMALIZER_SPECIAL_CHARACTERS_IS_VERBALIZED_TO_PUNCTUATION_IT[i]) {
            // If a special character is mapped to a punctuation (e.g. comma or period), then it needs to be tagged below because otherwise when passing to Hippo, Hippo will read it as invalid argument.
            token->tag = PV_NORMALIZER_TAG_IT_PUNCTUATION;
        }
        *verbalize_previous_as_ordinary_one = true;
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_abbreviation(pv_normalizer_token_t *token, bool *verbalize_previous_as_ordinary_one) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_IT_ABBREVIATION) {
        PV_ASSERT((token->tag_data_index >= 0) && (token->tag_data_index < PV_ARRAY_LEN(PV_NORMALIZER_ABBREVIATIONS_VERBALIZED_IT)));
        const char *verbalized = PV_NORMALIZER_ABBREVIATIONS_VERBALIZED_IT[token->tag_data_index];
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

        *verbalize_previous_as_ordinary_one = true;
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_cardinal(pv_normalizer_token_t *token, bool *verbalize_previous_as_ordinary_one) {
    PV_ASSERT(token);
    bool is_ordinal_denominator =
            ((token->previous != NULL) &&
             (token->previous->tag == PV_NORMALIZER_TAG_IT_FRACTION_SLASH) &&
             (token->previous->verbalized == NULL));

    if ((token->tag == PV_NORMALIZER_TAG_IT_CARDINAL) && !is_ordinal_denominator) {
        char *verbalized_cardinal = NULL;
        pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(token->string, false, &verbalized_cardinal);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_cardinal_to_string,
                    pv_status_to_string(status));
            free(verbalized_cardinal);
            return status;
        }
        pv_normalizer_token_set_verbalized(token, verbalized_cardinal);

        *verbalize_previous_as_ordinary_one = true;
    }

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

pv_status_t pv_normalizer_verbalizer_cardinal_to_string_helper(
        const char *number_string,
        int32_t *length,
        bool to_digit_string,
        bool dry_run,
        char **string) {
    PV_ASSERT(number_string);
    PV_ASSERT(*length >= 0);
    PV_ASSERT(string);

    free(*string);
    *string = NULL;
    char *result = NULL;

    if (!dry_run && (*length >= 0)) {
        result = (char *) calloc(*length + 1, sizeof(char));
        if (!result) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("result"));
            return PV_STATUS_OUT_OF_MEMORY;
        }
    }

    bool extra_space = false;

    if (to_digit_string ||
        (*number_string == '0') ||
        pv_normalizer_util_string_number_greater_than_int(number_string, MAX_VERBALIZED_CARDINAL_IT)) {
        while (*number_string != '\0') {
            int32_t digit = (int32_t) *number_string - '0';
            // Since we are reading digit one-by-one, only zero-to-nine are accessed.
            cardinal_to_string_helper(result, ZERO_TO_NINETY_NINE_ACCENTED_IT[(int32_t) digit], length, dry_run);
            cardinal_to_string_helper(result, " ", length, dry_run);
            extra_space = true;
            number_string++;
        }
    } else {
        int64_t number = (int64_t) strtoll(number_string, NULL, 10);
        if (number < 0) {
            return PV_STATUS_SUCCESS;
        }

        if (number < 100) {
            cardinal_to_string_helper(result, ZERO_TO_NINETY_NINE_ACCENTED_IT[number], length, dry_run);
            free(*string);
            *string = NULL;
            *string = result;
            return PV_STATUS_SUCCESS;
        }

        int64_t current_multiplier = MAX_MULTIPLIER_IT;
        int64_t current_hundred = 0;
        int32_t current_multiplier_i = 0;
        int64_t i = number;
        bool singular;
        bool centoed;
        while (i > 0 && current_multiplier != 0) {
            centoed = false;
            extra_space = false;
            current_hundred = i / current_multiplier;

            while (current_hundred == 0) {
                i %= current_multiplier;
                current_multiplier /= 1000;
                current_hundred = i / current_multiplier;
                current_multiplier_i++;
            }

            singular = (current_hundred == 1) ? true : false;

            if (current_hundred >= 100) {
                if (current_hundred < 200) {
                    // In Italian "100" is "cento" but NOT "un cento".
                    cardinal_to_string_helper(result, HUNDRED_IT, length, dry_run);
                } else {
                    // <ZERO_TO_NINETY_NINE_NON_ACCENTED_IT> is okay because doesn't matter accented or not because <current_hundred> will be strictly smaller than 1000.
                    cardinal_to_string_helper(
                            result,
                            ZERO_TO_NINETY_NINE_NON_ACCENTED_IT[(int32_t) ((int32_t) current_hundred / 100)],
                            length,
                            dry_run);
                    cardinal_to_string_helper(result, HUNDRED_IT, length, dry_run);
                }
                centoed = true;
            }

            current_hundred %= 100;
            current_multiplier_i++;

            if (current_hundred >= 0 && current_hundred < 100) {
                // Accented "É" if the multiplier is 1. Otherwise unaccented "E".
                // (Singular v.s. plural) & (space v.s. no space) & ("un" and no "un"): E.g. uno, dieci, cento, mille, duemila, un milione, due milioni, un miliardo, due miliardi, ....
                if (current_multiplier_i == 5) { // If the current multiplier is "1".
                    if (current_hundred != 0) {
                        const char *to_be_concat = ZERO_TO_NINETY_NINE_ACCENTED_IT[(int32_t) current_hundred];
                        if (centoed && to_be_concat[0] == 'O') { // E.g. For "108" it's "centotto" NOT "centootto".
                            to_be_concat += 1;
                        }
                        if (centoed && strcmp(to_be_concat, "TRE") == 0) {
                            to_be_concat = "TRÉ";
                        }
                        cardinal_to_string_helper(
                                result,
                                to_be_concat,
                                length,
                                dry_run);
                    }
                } else if (current_multiplier_i == 4) { // Plural & no space & no "un".
                    if (current_hundred != 0) {
                        const char *to_be_concat = ZERO_TO_NINETY_NINE_NON_ACCENTED_IT[(int32_t) current_hundred];
                        if (centoed && to_be_concat[0] == 'O') {
                            to_be_concat += 1;
                        }
                        if (strcmp(to_be_concat, "UNO") != 0) {
                            cardinal_to_string_helper(
                                    result,
                                    to_be_concat,
                                    length,
                                    dry_run);
                        }
                    }
                    if (singular) {
                        cardinal_to_string_helper(result, MULTIPLIERS_SINGULAR_IT[current_multiplier_i], length, dry_run);
                    } else {
                        cardinal_to_string_helper(result, MULTIPLIERS_PLURAL_IT[current_multiplier_i], length, dry_run);
                    }
                } else { // Plural & space & "un".
                    if (current_hundred != 0) {
                        const char *to_be_concat = ZERO_TO_NINETY_NINE_ACCENTED_IT[(int32_t) current_hundred];
                        if (centoed && to_be_concat[0] == 'O') {
                            to_be_concat += 1;
                        }
                        if (strcmp(to_be_concat, "UNO") == 0) {
                            to_be_concat = ONE_MASCULINE_IT;
                        }
                        if (centoed && strcmp(to_be_concat, "TRE") == 0) {
                            to_be_concat = "TRÉ";
                        }
                        cardinal_to_string_helper(
                                result,
                                to_be_concat,
                                length,
                                dry_run);
                    }
                    cardinal_to_string_helper(result, " ", length, dry_run);
                    if (singular) {
                        cardinal_to_string_helper(result, MULTIPLIERS_SINGULAR_IT[current_multiplier_i], length, dry_run);
                    } else {
                        cardinal_to_string_helper(result, MULTIPLIERS_PLURAL_IT[current_multiplier_i], length, dry_run);
                    }
                    cardinal_to_string_helper(result, " ", length, dry_run);
                    extra_space = true;
                }
            }

            i %= current_multiplier;
            current_multiplier /= 1000;
        }
    }

    if (!dry_run) {
        if (extra_space) {
            result[*length - 1] = '\0';
        } else {
            result[*length] = '\0';
        }
    }

    free(*string);
    *string = NULL;
    *string = result;

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

pv_status_t pv_normalizer_verbalizer_verbalize_negative_cardinal(pv_normalizer_token_t *token, bool *verbalize_previous_as_ordinary_one) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL) {
        char *verbalized_cardinal = NULL;
        const char *digit_string = token->string + 1;
        pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(digit_string, false, &verbalized_cardinal);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_cardinal_to_string,
                    pv_status_to_string(status));
            free(verbalized_cardinal);
            verbalized_cardinal = NULL;
            return status;
        }

        const char *negative = "MENO ";
        char *token_verbalized = calloc(strlen(verbalized_cardinal) + strlen(negative) + 1, sizeof(char));
        if (!token_verbalized) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("token_verbalized"));
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(token_verbalized, negative);
        strcat(token_verbalized, verbalized_cardinal);
        free(verbalized_cardinal);
        pv_normalizer_token_set_verbalized(token, token_verbalized);

        *verbalize_previous_as_ordinary_one = true;
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_number_range(pv_normalizer_token_t *token, bool *verbalize_previous_as_ordinary_one) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_IT_NUMBER_RANGE_TO) {
        char *token_verbalized = calloc(strlen(NUMBER_RANGE_INBETWEEN_STRING_IT) + 1, sizeof(char));
        if (!token_verbalized) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("token_verbalized"));
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(token_verbalized, NUMBER_RANGE_INBETWEEN_STRING_IT);
        pv_normalizer_token_set_verbalized(token, token_verbalized);

        *verbalize_previous_as_ordinary_one = true;
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_decimal(pv_normalizer_token_t *token, bool *verbalize_previous_as_ordinary_one) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_IT_DECIMAL_COMMA) {
        const char *point = "VIRGOLA";
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

        // For example, "1 ,5" should be tagged into <TAG_IT_CARDINAL_ONE> -> <TAG_IT_SPACE> -> <TAG_IT_DECIMAL_COMMA> -> <TAG_IT_DECIMAL_DIGITS>, and verbalized into "UNO" -> NULL -> "VIRGOLA" -> "CINQUE".
        *verbalize_previous_as_ordinary_one = true;
    } else if (token->tag == PV_NORMALIZER_TAG_IT_DECIMAL_DIGITS) {
        char *verbalized_decimal_digits = NULL;
        pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(token->string, true, &verbalized_decimal_digits);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_cardinal_to_string,
                    pv_status_to_string(status));
            free(verbalized_decimal_digits);
            return status;
        }

        pv_normalizer_token_set_verbalized(token, verbalized_decimal_digits);

        // After inspecting tagger, there should never be the case where the previous token of decimal digits is NOT decimal comma. Hence, no need to set <*verbalize_previous_as_ordinary_one = true> here to check for potential previous ordinary "1" & "-1" to prepend to this token.
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
            verbalized_internal = NULL;
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

    char *verbalized_internal = (char *) calloc(length + 1, sizeof(char));
    if (!verbalized_internal) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("verbalized_internal"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    status = pv_normalizer_verbalizer_cardinal_spell_out_helper(token->string, false, NULL, verbalized_internal);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_verbalizer_cardinal_spell_out_helper,
                pv_status_to_string(status));
        free(verbalized_internal);
        return status;
    }
    pv_normalizer_token_set_verbalized(token, verbalized_internal);

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_alphanum_spell_out(pv_normalizer_token_t *token, bool *verbalize_previous_as_ordinary_one) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_IT_CARDINAL_SPELL_OUT) {
        pv_status_t status = pv_normalizer_verbalizer_cardinal_spell_out(token);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_cardinal_spell_out,
                    pv_status_to_string(status));
            return status;
        }
        *verbalize_previous_as_ordinary_one = true;
    } else if (token->tag == PV_NORMALIZER_TAG_IT_LETTER_SPELL_OUT) { // pronounced later in phonemizer
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
        *verbalize_previous_as_ordinary_one = true;
    }

    return PV_STATUS_SUCCESS;
}

static pv_status_t pv_normalizer_verbalizer_verbalize_measurement(const pv_normalizer_verbalizer_it_t *object, pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_IT_MEASUREMENT) {
        const char *verbalized = NULL;
        pv_normalizer_token_t *previous = pv_normalizer_token_get_nth_token_before(token, 1, true);
        if ((token->previous != NULL) &&
            ((previous != NULL && previous->tag == PV_NORMALIZER_TAG_IT_CARDINAL_ONE && strcmp(previous->string, "1") == 0) ||
             (previous != NULL && previous->tag == PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL_ONE && strcmp(previous->string, "-1") == 0) ||
             token->previous->tag == PV_NORMALIZER_TAG_IT_PER_SLASH)) {
            verbalized = PV_NORMALIZER_MEASUREMENTS_VERBALIZED_SINGULAR_IT[token->tag_data_index]; // TODO: What about <g/hz> with no number in front of it? Where gram should be singular, but this logic makes it plural.
        } else {
            verbalized = PV_NORMALIZER_MEASUREMENTS_VERBALIZED_PLURAL_IT[token->tag_data_index];
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

        bool found_previous_one = pv_normalizer_util_it_found_previous_one(token);
        bool found_previous_negative_one = false;
        if (!found_previous_one) {
            found_previous_negative_one = pv_normalizer_util_it_found_previous_negative_one(token);
        }
        bool negative = false;
        bool prepend_to_reverbalize = false;
        if (found_previous_one) {
            prepend_to_reverbalize = true;
            negative = false;
        } else if (found_previous_negative_one) {
            prepend_to_reverbalize = true;
            negative = true;
        }
        if (prepend_to_reverbalize) {
            char *prefix = NULL;
            bool no_space = false;
            pv_status_t status = pv_normalizer_verbalizer_get_one_or_negative_one_variations(object, token->verbalized, negative, &prefix, &no_space);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_verbalizer_get_one_or_negative_one_variations,
                        pv_status_to_string(status));
                return status;
            }

            if (strcmp(prefix, "UN'") == 0 || strcmp(prefix, "MENO UN'") == 0) {
                // In this case, we concatenate the "1" or "-1" with the noun.
                status = pv_normalizer_verbalizer_prepend_to_reverbalize(token, &prefix, no_space, true);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_prepend_to_reverbalize,
                            pv_status_to_string(status));
                    free(prefix);
                    return status;
                }
            } else {
                status = pv_normalizer_verbalizer_verbalize_previous_one_or_negative_one(token, &prefix, true);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_previous_one_or_negative_one,
                            pv_status_to_string(status));
                    free(prefix);
                    return status;
                }
            }
        }
    }
    if (token->tag == PV_NORMALIZER_TAG_IT_PER_SLASH) {
        // TODO: Need an Italian speaker to decide which is used more commonly: "PER" or "AL".
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

        // Tagger should always place <PV_NORMALIZER_TAG_IT_PER_SLASH> after <PV_NORMALIZER_TAG_IT_MEASUREMENT>, therefore there is no need to worry about the "1" & "-1" in front of it because such case will never occur.
    }
    if (token->tag == PV_NORMALIZER_TAG_IT_MEASUREMENT) {
        pv_normalizer_token_t *previous = pv_normalizer_token_get_nth_token_before(token, 1, true);
        if ((previous != NULL) && (previous->tag == PV_NORMALIZER_TAG_IT_MEASUREMENT)) {
            const char *prev_verbalized = PV_NORMALIZER_MEASUREMENTS_VERBALIZED_SINGULAR_IT[previous->tag_data_index];

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
        else if ((previous != NULL) && (previous->tag == PV_NORMALIZER_TAG_IT_DOT)) {
            pv_normalizer_token_t *two_previous = pv_normalizer_token_get_nth_token_before(token, 2, true);
            if ((two_previous != NULL) && (two_previous->tag == PV_NORMALIZER_TAG_IT_MEASUREMENT)) {
                const char *prev_verbalized = PV_NORMALIZER_MEASUREMENTS_VERBALIZED_SINGULAR_IT[two_previous->tag_data_index];

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

pv_status_t pv_normalizer_verbalizer_verbalize_time(pv_normalizer_token_t *token, bool *verbalize_previous_as_ordinary_one) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_IT_TIME_COLON) {
        pv_normalizer_token_set_verbalized(token, NULL);
        // Should never be the case where "1" or "-1" as special or ordinary cardinal one occur in front of time colon, so don't set <*verbalize_previous_as_ordinary_one = true> here.
    } else if (token->tag == PV_NORMALIZER_TAG_IT_TIME_HOURS) {
        const char *number_string = token->string;

        int32_t len_time_hours = (int32_t) strlen(number_string);
        PV_ASSERT((len_time_hours == 1) || (len_time_hours == 2));

        int32_t offset = 0;
        if (len_time_hours == 2 && number_string[0] == '0') {
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

        *verbalize_previous_as_ordinary_one = true;
    } else if (token->tag == PV_NORMALIZER_TAG_IT_TIME_MINUTES) {
        const char *number_string = token->string;

        bool pronounce_time_minutes;
        if (number_string[0] == '0') {
            if (number_string[1] == '0') {
                pronounce_time_minutes = false;
                pv_normalizer_token_set_verbalized(token, NULL);
            } else {
                pronounce_time_minutes = true;
                number_string++;
                PV_ASSERT(strlen(number_string) == 1);
            }
        } else {
            pronounce_time_minutes = true;
        }
        if (pronounce_time_minutes) {
            char *verbalized_time_minutes = NULL;
            int32_t length_time_minutes = 0;
            pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(
                    number_string,
                    false,
                    &verbalized_time_minutes);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_verbalizer_cardinal_to_string,
                        pv_status_to_string(status));
                return status;
            }
            length_time_minutes = (int32_t) strlen(verbalized_time_minutes);

            char *verbalized = NULL;
            int32_t length = length_time_minutes;

            cardinal_to_string_helper(verbalized, "E ", &length, true);

            verbalized = (char *) calloc(length + 1, sizeof(char));
            if (!verbalized) {
                PV_ERROR_REPORT(
                        &pv_error_msg_alloc,
                        PV_ERROR_ARGS_PUBLIC_EMPTY(),
                        PV_ERROR_ARGS_PRIVATE("verbalized"));
                return PV_STATUS_OUT_OF_MEMORY;
            }

            cardinal_to_string_helper(verbalized, "E ", &length, false);
            strcat(verbalized, verbalized_time_minutes);
            verbalized[length] = '\0';
            free(verbalized_time_minutes);
            verbalized_time_minutes = NULL;

            pv_normalizer_token_set_verbalized(token, verbalized);
        }
        // Should never be the case where "1" or "-1" as special or ordinary cardinal one occur in front of time colon, so don't set <*verbalize_previous_as_ordinary_one = true> here.
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_dot(pv_normalizer_token_t *token, bool *verbalize_previous_as_ordinary_one) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_IT_DOT) {
        pv_normalizer_token_t *previous = pv_normalizer_token_get_nth_token_before(token, 1, true);
        if ((previous != NULL) && (previous->tag == PV_NORMALIZER_TAG_IT_MEASUREMENT)) {
            return PV_STATUS_SUCCESS;
        }

        const char *dot = "PUNTO";

        char *verbalized_internal = (char *) calloc(strlen(dot) + 1, sizeof(char));
        if (!verbalized_internal) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("verbalized_internal"));
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(verbalized_internal, dot);

        pv_normalizer_token_set_verbalized(token, verbalized_internal);

        *verbalize_previous_as_ordinary_one = true;
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_colon(pv_normalizer_token_t *token, bool *verbalize_previous_as_ordinary_one) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_IT_COLON) {
        const char *colon = "DUE PUNTI";

        char *verbalized_internal = (char *) calloc(strlen(colon) + 1, sizeof(char));
        if (!verbalized_internal) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("verbalized_internal"));
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(verbalized_internal, colon);
        pv_normalizer_token_set_verbalized(token, verbalized_internal);

        *verbalize_previous_as_ordinary_one = true;
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_top_level_domain(pv_normalizer_token_t *token, bool *verbalize_previous_as_ordinary_one) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_IT_TOP_LEVEL_DOMAIN) {
        PV_ASSERT(!PV_NORMALIZER_TOP_LEVEL_DOMAINS_IS_SPELL_OUT_IT[token->tag_data_index]);

        char *verbalized_internal = (char *) calloc(strlen(token->string) + 1, sizeof(char));
        if (!verbalized_internal) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("verbalized_internal"));
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(verbalized_internal, token->string);
        pv_status_t status = pv_normalizer_util_upper_inplace(verbalized_internal);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_util_upper_inplace,
                    pv_status_to_string(status));
            free(verbalized_internal);
            return status;
        }

        pv_normalizer_token_set_verbalized(token, verbalized_internal);

        *verbalize_previous_as_ordinary_one = true;
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_currency(const pv_normalizer_verbalizer_it_t *object, pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_IT_CURRENCY) {
        char *string = token->string;
        int32_t length = (int32_t) strlen(string);

        char *space = " ";
        char *and = " E ";

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

        char *number_string = (char *) calloc(number_string_length + 1, sizeof(char));
        if (!number_string) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("number_string"));
            return PV_STATUS_OUT_OF_MEMORY;
        }
        memcpy(number_string, string + number_start_index, number_string_length * sizeof(char));

        const char *currency_string = PV_NORMALIZER_CURRENCIES_VERBALIZED_PLURAL_IT[token->tag_data_index];
        if (strcmp(number_string, "1") == 0) {
            currency_string = PV_NORMALIZER_CURRENCIES_VERBALIZED_SINGULAR_IT[token->tag_data_index];
        }

        char *verbalized_number_string = NULL;
        if (strcmp(number_string, "1") == 0) {
            bool no_space = false;
            pv_status_t status = pv_normalizer_verbalizer_get_one_or_negative_one_variations(object, currency_string, false, &verbalized_number_string, &no_space);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_verbalizer_get_one_or_negative_one_variations,
                        pv_status_to_string(status));
                free(number_string);
                return status;
            }
        } else {
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
        }

        free(number_string);
        number_string = NULL;

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

        char *digit_string = (char *) calloc(digit_string_length + 1, sizeof(char));
        if (!digit_string) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("digit_string"));
            free(verbalized_number_string);
            return PV_STATUS_OUT_OF_MEMORY;
        }
        memcpy(digit_string, string + digit_start_index, digit_string_length * sizeof(char));

        const char *sub_currency_string = PV_NORMALIZER_SUB_CURRENCIES_VERBALIZED_PLURAL_IT[token->tag_data_index];
        if (strcmp(digit_string, "1") == 0) {
            sub_currency_string = PV_NORMALIZER_SUB_CURRENCIES_VERBALIZED_SINGULAR_IT[token->tag_data_index];
        }

        char *verbalized_digit_string = NULL;
        if (strcmp(digit_string, "1") == 0) {
            bool no_space = false;
            pv_status_t status = pv_normalizer_verbalizer_get_one_or_negative_one_variations(object, sub_currency_string, false, &verbalized_digit_string, &no_space);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_verbalizer_get_one_or_negative_one_variations,
                        pv_status_to_string(status));
                free(verbalized_number_string);
                free(digit_string);
                return status;
            }
        } else {
            pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(
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
        }

        free(digit_string);
        digit_string = NULL;

        int32_t verbalized_length =
                (int32_t) strlen(verbalized_number_string) +
                (int32_t) strlen(space) +
                (int32_t) strlen(currency_string) +
                (int32_t) strlen(and) +
                (int32_t) strlen(verbalized_digit_string) +
                (int32_t) strlen(space) +
                (int32_t) strlen(sub_currency_string);

        char *verbalized_internal = (char *) calloc(verbalized_length + 1, sizeof(char));
        if (!verbalized_internal) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("verbalized_internal"));
            free(verbalized_number_string);
            free(verbalized_digit_string);
            return PV_STATUS_OUT_OF_MEMORY;
        }

        strcpy(verbalized_internal, verbalized_number_string);
        strcat(verbalized_internal, space);
        strcat(verbalized_internal, currency_string);
        strcat(verbalized_internal, and);
        strcat(verbalized_internal, verbalized_digit_string);
        strcat(verbalized_internal, space);
        strcat(verbalized_internal, sub_currency_string);

        free(verbalized_number_string);
        free(verbalized_digit_string);
        pv_normalizer_token_set_verbalized(token, verbalized_internal);
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_negative_currency(const pv_normalizer_verbalizer_it_t *object, pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_IT_NEGATIVE_CURRENCY) {
        char *string = token->string;
        int32_t length = (int32_t) strlen(string);

        char *negative = "MENO ";
        char *space = " ";
        char *and = " E ";

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

        char *number_string = (char *) calloc(number_string_length + 1, sizeof(char));
        if (!number_string) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("number_string"));
            return PV_STATUS_OUT_OF_MEMORY;
        }
        memcpy(number_string, string + number_start_index, number_string_length * sizeof(char));

        const char *currency_string = PV_NORMALIZER_CURRENCIES_VERBALIZED_PLURAL_IT[token->tag_data_index];
        if (strcmp(number_string, "1") == 0) {
            currency_string = PV_NORMALIZER_CURRENCIES_VERBALIZED_SINGULAR_IT[token->tag_data_index];
        }

        char *verbalized_number_string = NULL;
        if (strcmp(number_string, "1") == 0) {
            bool no_space = false;
            pv_status_t status = pv_normalizer_verbalizer_get_one_or_negative_one_variations(object, currency_string, false, &verbalized_number_string, &no_space);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_verbalizer_get_one_or_negative_one_variations,
                        pv_status_to_string(status));
                free(number_string);
                return status;
            }
        } else {
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
        }

        free(number_string);
        number_string = NULL;

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

        // .00 is not verbalized e.g $1.00 -> one dollar
        if ((string[digit_start_index] == '0') && (string[digit_start_index + 1] == '0')) {
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
            // .0X is verbalized as X e.g. $1.05 -> one dollar and five cents
        } else if (string[digit_start_index] == '0') {
            digit_start_index++;
        }


        char *digit_string = (char *) calloc(digit_string_length + 1, sizeof(char));
        if (!digit_string) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("digit_string"));
            free(verbalized_number_string);
            return PV_STATUS_OUT_OF_MEMORY;
        }
        memcpy(digit_string, string + digit_start_index, digit_string_length * sizeof(char));

        const char *sub_currency_string = PV_NORMALIZER_SUB_CURRENCIES_VERBALIZED_PLURAL_IT[token->tag_data_index];
        if (strcmp(digit_string, "1") == 0) {
            sub_currency_string = PV_NORMALIZER_SUB_CURRENCIES_VERBALIZED_SINGULAR_IT[token->tag_data_index];
        }

        char *verbalized_digit_string = NULL;
        if (strcmp(digit_string, "1") == 0) {
            bool no_space = false;
            pv_status_t status = pv_normalizer_verbalizer_get_one_or_negative_one_variations(object, sub_currency_string, false, &verbalized_digit_string, &no_space);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_verbalizer_get_one_or_negative_one_variations,
                        pv_status_to_string(status));
                free(verbalized_number_string);
                free(digit_string);
                return status;
            }
        } else {
            pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(
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
        }

        free(digit_string);
        digit_string = NULL;

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

        free(verbalized_number_string);
        free(verbalized_digit_string);
        pv_normalizer_token_set_verbalized(token, token_verbalized);
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_currency_symbol(const pv_normalizer_verbalizer_it_t *object, pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_IT_CURRENCY_SYMBOL) {
        pv_normalizer_token_t *previous = pv_normalizer_token_get_nth_token_before(token, 1, true);
        if (token->tag_data_index < 0) {
            // Early return to NOT prepend and reverbalize because tagger went wrong so silently fail.
            return PV_STATUS_SUCCESS;
        }

        bool early_return = false;
        const char *currency_string = NULL;
        if (previous == NULL) {
            currency_string = PV_NORMALIZER_CURRENCIES_VERBALIZED_SINGULAR_IT[token->tag_data_index];
            // Early return to NOT prepend and reverbalize because there is nothing before it.
            early_return = true;
        } else {
            char *string = previous->string;
            if (strcmp(string, "1") == 0 ||
                strcmp(string, "-1") == 0 ||
                !(previous->tag == PV_NORMALIZER_TAG_IT_CARDINAL ||
                  previous->tag == PV_NORMALIZER_TAG_IT_CARDINAL_ONE ||
                  previous->tag == PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL ||
                  previous->tag == PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL_ONE ||
                  previous->tag == PV_NORMALIZER_TAG_IT_DECIMAL_DIGITS)) {
                currency_string = PV_NORMALIZER_CURRENCIES_VERBALIZED_SINGULAR_IT[token->tag_data_index];
            } else {
                currency_string = PV_NORMALIZER_CURRENCIES_VERBALIZED_PLURAL_IT[token->tag_data_index];
            }
        }
        char *verbalized_internal = (char *) calloc(strlen(currency_string) + 1, sizeof(char));
        if (!verbalized_internal) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("verbalized_internal"));
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(verbalized_internal, currency_string);
        pv_normalizer_token_set_verbalized(token, verbalized_internal);

        if (early_return) {
            return PV_STATUS_SUCCESS;
        }

        bool found_previous_one = pv_normalizer_util_it_found_previous_one(token);
        bool found_previous_negative_one = false;
        if (!found_previous_one) {
            found_previous_negative_one = pv_normalizer_util_it_found_previous_negative_one(token);
        }
        bool negative = false;
        bool prepend_to_reverbalize = false;
        if (found_previous_one) {
            prepend_to_reverbalize = true;
            negative = false;
        } else if (found_previous_negative_one) {
            prepend_to_reverbalize = true;
            negative = true;
        }
        if (prepend_to_reverbalize) {
            char *prefix = NULL;
            bool no_space = false;
            pv_status_t status = pv_normalizer_verbalizer_get_one_or_negative_one_variations(object, token->verbalized, negative, &prefix, &no_space);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_verbalizer_get_one_or_negative_one_variations,
                        pv_status_to_string(status));
                return status;
            }

            if (strcmp(prefix, "UN'") == 0 || strcmp(prefix, "MENO UN'") == 0) {
                // In this case, we concatenate the "1" or "-1" with the noun.
                status = pv_normalizer_verbalizer_prepend_to_reverbalize(token, &prefix, no_space, true);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_prepend_to_reverbalize,
                            pv_status_to_string(status));
                    free(prefix);
                    return status;
                }
            } else {
                status = pv_normalizer_verbalizer_verbalize_previous_one_or_negative_one(token, &prefix, true);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_verbalizer_verbalize_previous_one_or_negative_one,
                            pv_status_to_string(status));
                    free(prefix);
                    return status;
                }
            }
        }
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_digits_sequence(pv_normalizer_token_t *token, bool *verbalize_previous_as_ordinary_one) {
    PV_ASSERT(token);

    const char *comma_string = ",";

    const char *string = token->string;
    int32_t length = (int32_t) strlen(string);

    if (token->tag == PV_NORMALIZER_TAG_IT_DIGITS_WITH_PARENTHESES) {
        int32_t digits_length = length - 2;
        char *number_string = (char *) calloc(digits_length + 1, sizeof(char));
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
        number_string = NULL;
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_cardinal_to_string,
                    pv_status_to_string(status));
            free(verbalized);
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

        *verbalize_previous_as_ordinary_one = true;
    } else if (token->tag == PV_NORMALIZER_TAG_IT_DIGITS) {
        char *verbalized = NULL;
        pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(
                string,
                true,
                &verbalized);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_cardinal_to_string,
                    pv_status_to_string(status));
            free(token->verbalized);
            return status;
        }
        pv_normalizer_token_set_verbalized(token, verbalized);

        *verbalize_previous_as_ordinary_one = true;
    } else if (token->tag == PV_NORMALIZER_TAG_IT_DIGITS_SEPARATOR) {
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
        token->tag = PV_NORMALIZER_TAG_IT_PUNCTUATION;

        *verbalize_previous_as_ordinary_one = true;
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_dates(pv_normalizer_token_t *token, bool *verbalize_previous_as_ordinary_one) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_IT_DATE_SEPARATOR) {
        pv_normalizer_token_set_verbalized(token, NULL);
        // Should never be the case where "1" or "-1" as special or ordinary cardinal one occur in front of date separator, so don't set <*verbalize_previous_as_ordinary_one = true> here.
    } else if (token->tag == PV_NORMALIZER_TAG_IT_DATE_DAY) {
        // Definitely verbalize the day. Then if see month, will append month. Then if see year, will append year.
        char *verbalized_internal = NULL;
        int32_t number = (int32_t) strtol(token->string, NULL, 10);
        PV_ASSERT(number > 0 && number <= 31);

        char *day_verbalized = NULL;
        int32_t length = 0;
        if (number == 1) {
            day_verbalized = (char *) calloc(6, sizeof(char));
            if (!day_verbalized) {
                PV_ERROR_REPORT(
                        &pv_error_msg_alloc,
                        PV_ERROR_ARGS_PUBLIC_EMPTY(),
                        PV_ERROR_ARGS_PRIVATE("day_verbalized"));
                return PV_STATUS_OUT_OF_MEMORY;
            }
            strcpy(day_verbalized, "PRIMO");
            day_verbalized[5] = '\0';
        } else {
            int32_t token_string_length = (int32_t) strlen(token->string);
            int32_t offset = 0;
            PV_ASSERT(token_string_length == 1 || token_string_length == 2);
            if (token_string_length == 2 && token->string[0] == '0') {
                offset++;
            }
            pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(token->string + offset, false, &day_verbalized);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_verbalizer_cardinal_to_string,
                        pv_status_to_string(status));
                return status;
            }
        }
        cardinal_to_string_helper(verbalized_internal, day_verbalized, &length, true);

        pv_normalizer_token_t *previous = pv_normalizer_token_get_nth_token_before(token, 1, true);
        pv_normalizer_token_t *two_previous = pv_normalizer_token_get_nth_token_before(token, 2, true);
        pv_normalizer_token_t *three_previous = pv_normalizer_token_get_nth_token_before(token, 3, true);
        pv_normalizer_token_t *four_previous = pv_normalizer_token_get_nth_token_before(token, 4, true);

        bool has_month_before = false;
        char *month_verbalized = NULL;
        pv_normalizer_token_t *month_token = NULL;
        if ((previous != NULL) && (previous->tag == PV_NORMALIZER_TAG_IT_DATE_MONTH)) {
            has_month_before = true;
            month_token = previous;
        } else if (((previous != NULL) && (previous->tag == PV_NORMALIZER_TAG_IT_DATE_SEPARATOR)) &&
                   ((two_previous != NULL) && (two_previous->tag == PV_NORMALIZER_TAG_IT_DATE_MONTH))) {
            has_month_before = true;
            month_token = two_previous;
        }
        if (has_month_before) {
            const char *tmp = NULL;
            if (month_token->tag_data_index < 0) {
                int64_t month_index = (int64_t) strtol(month_token->string, NULL, 10);
                month_index--;
                tmp = PV_NORMALIZER_MONTH_NAMES_IT[month_index];
            } else {
                tmp = PV_NORMALIZER_MONTH_NAMES_VERBALIZED_IT[month_token->tag_data_index];
            }
            int32_t tmp_length = (int32_t) strlen(tmp);
            month_verbalized = (char *) calloc(tmp_length + 1, sizeof(char));
            if (!month_verbalized) {
                PV_ERROR_REPORT(
                        &pv_error_msg_alloc,
                        PV_ERROR_ARGS_PUBLIC_EMPTY(),
                        PV_ERROR_ARGS_PRIVATE("month_verbalized"));
                free(day_verbalized);
                return PV_STATUS_OUT_OF_MEMORY;
            }
            strcpy(month_verbalized, tmp);
            month_verbalized[tmp_length] = '\0';

            cardinal_to_string_helper(verbalized_internal, " ", &length, true);
            cardinal_to_string_helper(verbalized_internal, month_verbalized, &length, true);
        }

        bool has_year_before = false;
        char *year_verbalized = NULL;
        pv_normalizer_token_t *year_token = NULL;
        if ((previous != NULL && previous->tag == PV_NORMALIZER_TAG_IT_DATE_SEPARATOR) &&
            (two_previous != NULL && two_previous->tag == PV_NORMALIZER_TAG_IT_DATE_MONTH) &&
            (three_previous != NULL && three_previous->tag == PV_NORMALIZER_TAG_IT_DATE_SEPARATOR) &&
            (four_previous != NULL && four_previous->tag == PV_NORMALIZER_TAG_IT_DATE_YEAR)) {
            has_year_before = true;
            year_token = four_previous;
        }
        if (has_year_before) {
            pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(year_token->string, false, &year_verbalized);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_verbalizer_cardinal_to_string,
                        pv_status_to_string(status));
                free(day_verbalized);
                if (has_month_before) {
                    free(month_verbalized);
                }
                return status;
            }
            cardinal_to_string_helper(verbalized_internal, " ", &length, true);
            cardinal_to_string_helper(verbalized_internal, year_verbalized, &length, true);
        }

        verbalized_internal = (char *) calloc(length + 1, sizeof(char));
        if (!verbalized_internal) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("verbalized_internal"));
            free(day_verbalized);
            if (has_month_before) {
                free(month_verbalized);
            }
            return PV_STATUS_OUT_OF_MEMORY;
        }
        if (day_verbalized) {
            cardinal_to_string_helper(verbalized_internal, day_verbalized, &length, false);
            free(day_verbalized);
            day_verbalized = NULL;
        }
        if (has_month_before && month_verbalized) {
            cardinal_to_string_helper(verbalized_internal, " ", &length, false);
            cardinal_to_string_helper(verbalized_internal, month_verbalized, &length, false);
            free(month_verbalized);
            month_verbalized = NULL;
        }
        if (has_year_before && year_verbalized) {
            cardinal_to_string_helper(verbalized_internal, " ", &length, false);
            cardinal_to_string_helper(verbalized_internal, year_verbalized, &length, false);
            free(year_verbalized);
            year_verbalized = NULL;
        }
        verbalized_internal[length] = '\0';

        pv_normalizer_token_set_verbalized(token, verbalized_internal);

        *verbalize_previous_as_ordinary_one = true;
    } else if (token->tag == PV_NORMALIZER_TAG_IT_DATE_MONTH) {
        // Verbalize the month for this token if and only if there's no year and digit separator right before it.
        pv_normalizer_token_t *previous = pv_normalizer_token_get_nth_token_before(token, 1, true);
        pv_normalizer_token_t *two_previous = pv_normalizer_token_get_nth_token_before(token, 2, true);

        if (!(((previous != NULL) && (previous->tag == PV_NORMALIZER_TAG_IT_DATE_SEPARATOR)) &&
              ((two_previous != NULL) && (two_previous->tag == PV_NORMALIZER_TAG_IT_DATE_YEAR)))) {
            char *month_verbalized = NULL;
            const char *tmp = NULL;
            if (token->tag_data_index < 0) {
                int64_t month_index = (int64_t) strtol(token->string, NULL, 10);
                month_index--;
                tmp = PV_NORMALIZER_MONTH_NAMES_IT[month_index];
            } else {
                tmp = PV_NORMALIZER_MONTH_NAMES_VERBALIZED_IT[token->tag_data_index];
            }
            int32_t tmp_length = (int32_t) strlen(tmp);
            month_verbalized = (char *) calloc(tmp_length + 1, sizeof(char));
            if (!month_verbalized) {
                PV_ERROR_REPORT(
                        &pv_error_msg_alloc,
                        PV_ERROR_ARGS_PUBLIC_EMPTY(),
                        PV_ERROR_ARGS_PRIVATE("month_verbalized"));
                return PV_STATUS_OUT_OF_MEMORY;
            }
            strcpy(month_verbalized, tmp);
            month_verbalized[tmp_length] = '\0';
            pv_normalizer_token_set_verbalized(token, month_verbalized);

            *verbalize_previous_as_ordinary_one = true;
        }
    } else if (token->tag == PV_NORMALIZER_TAG_IT_DATE_YEAR) {
        // Verbalize the year for this token if and only if has month before.
        pv_normalizer_token_t *previous = pv_normalizer_token_get_nth_token_before(token, 1, true);
        pv_normalizer_token_t *two_previous = pv_normalizer_token_get_nth_token_before(token, 2, true);
        pv_normalizer_token_t *three_previous = pv_normalizer_token_get_nth_token_before(token, 3, true);
        pv_normalizer_token_t *four_previous = pv_normalizer_token_get_nth_token_before(token, 4, true);

        bool has_month_before = false;
        if (((previous != NULL && previous->tag == PV_NORMALIZER_TAG_IT_DATE_SEPARATOR) &&
             (two_previous != NULL && two_previous->tag == PV_NORMALIZER_TAG_IT_DATE_MONTH) &&
             (three_previous != NULL && three_previous->tag == PV_NORMALIZER_TAG_IT_DATE_SEPARATOR) &&
             (four_previous != NULL && four_previous->tag == PV_NORMALIZER_TAG_IT_DATE_DAY)) ||
            ((previous != NULL && previous->tag == PV_NORMALIZER_TAG_IT_DATE_SEPARATOR) &&
             (two_previous != NULL && two_previous->tag == PV_NORMALIZER_TAG_IT_DATE_DAY) &&
             (three_previous != NULL && three_previous->tag == PV_NORMALIZER_TAG_IT_DATE_SEPARATOR) &&
             (four_previous != NULL && four_previous->tag == PV_NORMALIZER_TAG_IT_DATE_MONTH))) {
            has_month_before = true;
        }

        if (has_month_before) {
            char *verbalized_internal = NULL;
            pv_status_t status = pv_normalizer_verbalizer_cardinal_to_string(token->string, false, &verbalized_internal);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_verbalizer_cardinal_to_string,
                        pv_status_to_string(status));
                return status;
            }
            pv_normalizer_token_set_verbalized(token, verbalized_internal);

            *verbalize_previous_as_ordinary_one = true;
        }
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_names(pv_normalizer_token_t *token, bool *verbalize_previous_as_ordinary_one) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_IT_NAME_INITIAL_DOT) {
        pv_normalizer_token_set_verbalized(token, NULL);

        *verbalize_previous_as_ordinary_one = true;
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_ordinal(pv_normalizer_token_t *token, bool *verbalize_previous_as_ordinary_one) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_IT_ORDINAL_MASCULINE) {
        pv_status_t status = pv_normalizer_verbalizer_ordinal_to_string_masculine(token->string, &(token->verbalized));
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_ordinal_to_string_masculine,
                    pv_status_to_string(status));
            return status;
        }
        *verbalize_previous_as_ordinary_one = true;
    }

    if (token->tag == PV_NORMALIZER_TAG_IT_ORDINAL_FEMININE) {
        pv_status_t status = pv_normalizer_verbalizer_ordinal_to_string_feminine(token->string, &(token->verbalized));
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_ordinal_to_string_feminine,
                    pv_status_to_string(status));
            return status;
        }
        *verbalize_previous_as_ordinary_one = true;
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_ordinal_to_string_masculine(const char *ordinal_string, char **string) {
    PV_ASSERT(ordinal_string);
    PV_ASSERT(string);

    *string = NULL;

    char *ordinal_symbol_masculine_no_underline = "°";
    int32_t ordinal_symbol_masculine_no_underline_num_bytes = 0;
    pv_status_t status = pv_language_num_bytes_character((unsigned char) ordinal_symbol_masculine_no_underline[0], &ordinal_symbol_masculine_no_underline_num_bytes);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_language_num_bytes_character,
                pv_status_to_string(status));
        return PV_STATUS_SUCCESS;
    }

    char *ordinal_symbol_masculine_underline = "º";
    int32_t ordinal_symbol_masculine_underline_num_bytes = 0;
    status = pv_language_num_bytes_character((unsigned char) ordinal_symbol_masculine_underline[0], &ordinal_symbol_masculine_underline_num_bytes);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_language_num_bytes_character,
                pv_status_to_string(status));
        return PV_STATUS_SUCCESS;
    }

    int32_t ordinal_symbol_masculine_num_bytes = 0;
    int32_t ordinal_string_length = (int32_t) strlen(ordinal_string);
    int32_t offset = 0;
    if (ordinal_string_length > ordinal_symbol_masculine_no_underline_num_bytes && strcmp(ordinal_string + ordinal_string_length - ordinal_symbol_masculine_no_underline_num_bytes, ordinal_symbol_masculine_no_underline) == 0) {
        ordinal_symbol_masculine_num_bytes = ordinal_symbol_masculine_no_underline_num_bytes;
    } else if (ordinal_string_length > ordinal_symbol_masculine_underline_num_bytes && strcmp(ordinal_string + ordinal_string_length - ordinal_symbol_masculine_underline_num_bytes, ordinal_symbol_masculine_underline) == 0) {
        ordinal_symbol_masculine_num_bytes = ordinal_symbol_masculine_underline_num_bytes;
    } else {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("ordinal_string"));
        return PV_STATUS_SUCCESS;
    }
    offset = ordinal_symbol_masculine_num_bytes;

    char *number_string = (char *) calloc(ordinal_string_length - offset + 1, sizeof(char));
    if (!number_string) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("number_string"));
        return PV_STATUS_OUT_OF_MEMORY;
    }
    memcpy(number_string, ordinal_string, (ordinal_string_length - offset) * sizeof(char));
    number_string[ordinal_string_length - offset] = '\0';

    if (((*number_string == '0') && (strlen(number_string) > 1)) ||
        pv_normalizer_util_string_number_greater_than_int(number_string, MAX_VERBALIZED_CARDINAL_IT)) {
        free(number_string);
        number_string = NULL;
        // Because user might input this! So we just ignore it by NOT verbalize it.
        return PV_STATUS_SUCCESS;
    }

    if (!pv_normalizer_util_string_number_greater_than_int(number_string, 10) ||
        (number_string != NULL &&
         strlen(number_string) > 0 &&
         (strcmp(number_string, "1000") == 0 ||
          strcmp(number_string, "1000000") == 0 ||
          strcmp(number_string, "1000000000") == 0 ||
          strcmp(number_string, "1000000000000") == 0))) {
        int32_t k = 0;
        while ((strcmp(ORDINAL_KEYS_IT[k], number_string) != 0) && (k < PV_ARRAY_LEN(ORDINAL_KEYS_IT))) {
            k++;
        }

        if (k >= PV_ARRAY_LEN(ORDINAL_KEYS_IT)) {
            PV_ERROR_REPORT(
                    &pv_error_msg_invalid_argument,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("number_string"));
            free(number_string);
            return PV_STATUS_SUCCESS;
        }

        const char *ordinal_verbalized = ORDINAL_VALUES_MASCULINE_IT[k];
        int32_t ordinal_verbalized_length = (int32_t) strlen(ordinal_verbalized);

        *string = (char *) calloc(ordinal_verbalized_length + 1, sizeof(char));
        if (!(*string)) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("string"));
            free(number_string);
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(*string, ordinal_verbalized);
        (*string)[ordinal_verbalized_length] = '\0';

        free(number_string);
        number_string = NULL;
        return PV_STATUS_SUCCESS;
    } else {
        char *cardinal_string = NULL;

        status = pv_normalizer_verbalizer_cardinal_to_string(number_string, false, &cardinal_string);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_cardinal_to_string_helper,
                    pv_status_to_string(status));
            free(number_string);
            return status;
        }

        free(number_string);
        number_string = NULL;

        // Deal with <cardinal_string> ending:
        // - If ending with "TRÉ" or "SEI", then append "ESIMO".
        // - If NOT ending with "TRÉ" and "SEI" but end with vowel, then remove the vowel and append "ESIMO".
        // - Else, append "ESIMO".

        int32_t cardinal_string_length = (int32_t) strlen(cardinal_string);
        bool cardinal_string_end_with_vowel = false;
        char *cardinal_string_new_end = cardinal_string + cardinal_string_length;
        for (int32_t i = 1; (i <= PV_NORMALIZER_MAX_NUM_BYTES_PER_CHARACTER) && (i <= (int32_t) strlen(cardinal_string)); ++i) {
            if (pv_normalizer_util_it_is_vowel(cardinal_string_new_end - i)) {
                cardinal_string_end_with_vowel = true;
                cardinal_string_new_end -= i;
                break;
            }
        }

        bool cardinal_string_end_with_TRE = false;
        bool cardinal_string_end_with_SEI = false;
        int32_t accented_E_num_bytes = 0;
        int32_t accented_TRE_num_bytes = 0;
        int32_t SEI_num_bytes = 3;
        status = pv_language_num_bytes_character((unsigned char) *"É", &accented_E_num_bytes);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_language_num_bytes_character,
                    pv_status_to_string(status));
            free(cardinal_string);
            return status;
        }
        accented_TRE_num_bytes = 2 + accented_E_num_bytes;

        if (cardinal_string_length >= accented_TRE_num_bytes) {
            if (strcmp(cardinal_string + cardinal_string_length - accented_TRE_num_bytes, "TRÉ") == 0) {
                cardinal_string_end_with_TRE = true;
            }
        }
        if (cardinal_string_length >= SEI_num_bytes) {
            if (strcmp(cardinal_string + cardinal_string_length - SEI_num_bytes, "SEI") == 0) {
                cardinal_string_end_with_SEI = true;
            }
        }

        if (cardinal_string_end_with_TRE) {
            cardinal_string_new_end++;
            *(cardinal_string_new_end - 1) = 'E';
        } else if (!cardinal_string_end_with_vowel || cardinal_string_end_with_SEI) {
            cardinal_string_new_end = cardinal_string + cardinal_string_length;
        }

        *cardinal_string_new_end = '\0';
        cardinal_string_new_end = NULL;

        cardinal_string_length = (int32_t) strlen(cardinal_string);

        *string = (char *) calloc(cardinal_string_length + ORDINAL_SUFFIX_MASCULINE_LENGTH_IT + 1, sizeof(char));
        if (!(*string)) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("string"));
            free(cardinal_string);
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(*string, cardinal_string);
        (*string)[cardinal_string_length] = '\0';
        strcat(*string, ORDINAL_SUFFIX_MASCULINE_IT);
        (*string)[cardinal_string_length + ORDINAL_SUFFIX_MASCULINE_LENGTH_IT] = '\0';

        free(cardinal_string);
        cardinal_string = NULL;

        return PV_STATUS_SUCCESS;
    }
}

pv_status_t pv_normalizer_verbalizer_ordinal_to_string_feminine(const char *ordinal_string, char **string) {
    PV_ASSERT(ordinal_string);
    PV_ASSERT(string);

    *string = NULL;

    char *ordinal_symbol_feminine = "ª";
    int32_t ordinal_symbol_feminine_num_bytes = 0;
    pv_status_t status = pv_language_num_bytes_character((unsigned char) ordinal_symbol_feminine[0], &ordinal_symbol_feminine_num_bytes);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_language_num_bytes_character,
                pv_status_to_string(status));
        return PV_STATUS_INVALID_ARGUMENT;
    }
    int32_t ordinal_string_length = (int32_t) strlen(ordinal_string);
    int32_t offset = 0;
    if (ordinal_string_length <= ordinal_symbol_feminine_num_bytes || strcmp(ordinal_string + ordinal_string_length - ordinal_symbol_feminine_num_bytes, ordinal_symbol_feminine) != 0) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("ordinal_string"));
        return PV_STATUS_SUCCESS;
    }
    offset = ordinal_symbol_feminine_num_bytes;

    char *number_string = (char *) calloc(ordinal_string_length - offset + 1, sizeof(char));
    if (!number_string) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("number_string"));
        return PV_STATUS_OUT_OF_MEMORY;
    }
    memcpy(number_string, ordinal_string, (ordinal_string_length - offset) * sizeof(char));
    number_string[ordinal_string_length - offset] = '\0';

    if (((*number_string == '0') && (strlen(number_string) > 1)) ||
        pv_normalizer_util_string_number_greater_than_int(number_string, MAX_VERBALIZED_CARDINAL_IT)) {
        free(number_string);
        number_string = NULL;
        // Because user might input this! So we just ignore it by NOT verbalize it.
        return PV_STATUS_SUCCESS;
    }

    if (!pv_normalizer_util_string_number_greater_than_int(number_string, 10) ||
        (number_string != NULL &&
         strlen(number_string) > 0 &&
         (strcmp(number_string, "1000") == 0 ||
          strcmp(number_string, "1000000") == 0 ||
          strcmp(number_string, "1000000000") == 0 ||
          strcmp(number_string, "1000000000000") == 0))) {
        int32_t k = 0;
        while ((strcmp(ORDINAL_KEYS_IT[k], number_string) != 0) && (k < PV_ARRAY_LEN(ORDINAL_KEYS_IT))) {
            k++;
        }

        if (k >= PV_ARRAY_LEN(ORDINAL_KEYS_IT)) {
            PV_ERROR_REPORT(
                    &pv_error_msg_invalid_argument,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("number_string"));
            free(number_string);
            return PV_STATUS_SUCCESS;
        }

        const char *ordinal_verbalized = ORDINAL_VALUES_FEMININE_IT[k];
        int32_t ordinal_verbalized_length = (int32_t) strlen(ordinal_verbalized);

        *string = (char *) calloc(ordinal_verbalized_length + 1, sizeof(char));
        if (!(*string)) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("string"));
            free(number_string);
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(*string, ordinal_verbalized);
        (*string)[ordinal_verbalized_length] = '\0';

        free(number_string);
        number_string = NULL;
        return PV_STATUS_SUCCESS;
    } else {
        char *cardinal_string = NULL;

        status = pv_normalizer_verbalizer_cardinal_to_string(number_string, false, &cardinal_string);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_cardinal_to_string_helper,
                    pv_status_to_string(status));
            free(number_string);
            return status;
        }


        free(number_string);
        number_string = NULL;

        // Deal with <cardinal_string> ending:
        // - If ending with "TRÉ" or "SEI", then append "ESIMA".
        // - If NOT ending with "TRÉ" and "SEI" but end with vowel, then remove the vowel and append "ESIMA".
        // - Else, append "ESIMA".

        int32_t cardinal_string_length = (int32_t) strlen(cardinal_string);
        bool cardinal_string_end_with_vowel = false;
        char *cardinal_string_new_end = cardinal_string + cardinal_string_length;
        for (int32_t i = 1; (i <= PV_NORMALIZER_MAX_NUM_BYTES_PER_CHARACTER) && (i <= (int32_t) strlen(cardinal_string)); ++i) {
            if (pv_normalizer_util_it_is_vowel(cardinal_string_new_end - i)) {
                cardinal_string_end_with_vowel = true;
                cardinal_string_new_end -= i;
                break;
            }
        }

        bool cardinal_string_end_with_TRE = false;
        bool cardinal_string_end_with_SEI = false;
        int32_t accented_E_num_bytes = 0;
        int32_t accented_TRE_num_bytes = 0;
        int32_t SEI_num_bytes = 3;
        status = pv_language_num_bytes_character((unsigned char) *"É", &accented_E_num_bytes);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_language_num_bytes_character,
                    pv_status_to_string(status));
            free(cardinal_string);
            return status;
        }
        accented_TRE_num_bytes = 2 + accented_E_num_bytes;

        if (cardinal_string_length >= accented_TRE_num_bytes) {
            if (strcmp(cardinal_string + cardinal_string_length - accented_TRE_num_bytes, "TRÉ") == 0) {
                cardinal_string_end_with_TRE = true;
            }
        }
        if (cardinal_string_length >= SEI_num_bytes) {
            if (strcmp(cardinal_string + cardinal_string_length - SEI_num_bytes, "SEI") == 0) {
                cardinal_string_end_with_SEI = true;
            }
        }

        if (cardinal_string_end_with_TRE) {
            cardinal_string_new_end++;
            *(cardinal_string_new_end - 1) = 'E';
        } else if (!cardinal_string_end_with_vowel || cardinal_string_end_with_SEI) {
            cardinal_string_new_end = cardinal_string + cardinal_string_length;
        }

        *cardinal_string_new_end = '\0';
        cardinal_string_new_end = NULL;

        cardinal_string_length = (int32_t) strlen(cardinal_string);

        (*string) = (char *) calloc(cardinal_string_length + ORDINAL_SUFFIX_FEMININE_LENGTH_IT + 1, sizeof(char));
        if (!(*string)) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("string"));
            free(cardinal_string);
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(*string, cardinal_string);
        (*string)[cardinal_string_length] = '\0';
        strcat(*string, ORDINAL_SUFFIX_FEMININE_IT);
        (*string)[cardinal_string_length + ORDINAL_SUFFIX_FEMININE_LENGTH_IT] = '\0';

        free(cardinal_string);
        cardinal_string = NULL;

        return PV_STATUS_SUCCESS;
    }
}

pv_status_t pv_normalizer_verbalizer_verbalize_negative_ordinal(pv_normalizer_token_t *token, bool *verbalize_previous_as_ordinary_one) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_IT_NEGATIVE_ORDINAL_MASCULINE) {
        const char *number_string = token->string + 1;
        char *verbalized_number_string = NULL;
        pv_status_t status = pv_normalizer_verbalizer_ordinal_to_string_masculine(number_string, &verbalized_number_string);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_ordinal_to_string_masculine,
                    pv_status_to_string(status));
            free(verbalized_number_string);
            return status;
        }

        const char *negative = "MENO ";

        char *verbalized_internal = (char *) calloc(strlen(verbalized_number_string) + strlen(negative) + 1, sizeof(char));
        if (!verbalized_internal) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("verbalized_internal"));
            free(verbalized_number_string);
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(verbalized_internal, negative);
        strcat(verbalized_internal, verbalized_number_string);
        free(verbalized_number_string);
        verbalized_number_string = NULL;

        pv_normalizer_token_set_verbalized(token, verbalized_internal);

        *verbalize_previous_as_ordinary_one = true;
    } else if (token->tag == PV_NORMALIZER_TAG_IT_NEGATIVE_ORDINAL_FEMININE) {
        const char *number_string = token->string + 1;
        char *verbalized_number_string = NULL;
        pv_status_t status = pv_normalizer_verbalizer_ordinal_to_string_feminine(number_string, &verbalized_number_string);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_verbalizer_ordinal_to_string_feminine,
                    pv_status_to_string(status));
            return status;
        }

        const char *negative = "MENO ";

        char *verbalized_internal = (char *) calloc(strlen(verbalized_number_string) + strlen(negative) + 1, sizeof(char));
        if (!verbalized_internal) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("verbalized_internal"));
            free(verbalized_number_string);
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(verbalized_internal, negative);
        strcat(verbalized_internal, verbalized_number_string);
        free(verbalized_number_string);
        verbalized_number_string = NULL;

        pv_normalizer_token_set_verbalized(token, verbalized_internal);

        *verbalize_previous_as_ordinary_one = true;
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_fraction_slash(pv_normalizer_token_t *token, bool *verbalize_previous_as_ordinary_one) {
    PV_ASSERT(token);

    if (token->tag == PV_NORMALIZER_TAG_IT_FRACTION_SLASH) {
        // Recall that in tagger <pv_normalizer_tag_fraction()>, we retag the denominator special "1" & "-1" back to ordinary cardinal.
        if ((token->next != NULL) &&
            ((token->next->tag == PV_NORMALIZER_TAG_IT_CARDINAL) ||
             (token->next->tag == PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL))) {
            bool next_is_decimal =
                    ((token->next != NULL) &&
                     (token->next->next != NULL) &&
                     (token->next->next->tag == PV_NORMALIZER_TAG_IT_DECIMAL_COMMA));

            bool next_is_negative =
                    ((token->next != NULL) &&
                     (token->next->tag == PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL));

            if (next_is_negative || next_is_decimal) {
                const char *over = "FRATTO";

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

                *verbalize_previous_as_ordinary_one = true; // Need to append prefix as ordinary "1" or "-1".
            } else {
                pv_normalizer_token_set_verbalized(token, NULL);

                free(token->string);
                token->string = (char *) calloc(1, sizeof(char));
                if (!token->string) {
                    PV_ERROR_REPORT(
                            &pv_error_msg_alloc,
                            PV_ERROR_ARGS_PUBLIC_EMPTY(),
                            PV_ERROR_ARGS_PRIVATE("token->string"));
                    return PV_STATUS_OUT_OF_MEMORY;
                }
                token->string[0] = '\0';
            }
        }
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_verbalizer_verbalize_fraction_denominator(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    bool is_ordinal_denominator =
            ((token->previous != NULL) &&
             (token->previous->tag == PV_NORMALIZER_TAG_IT_FRACTION_SLASH) &&
             (token->previous->verbalized == NULL));

    // Recall that in tagger <pv_normalizer_tag_fraction()>, we retag the denominator special "1" & "-1" back to ordinary cardinal.
    if ((token->tag == PV_NORMALIZER_TAG_IT_CARDINAL) && is_ordinal_denominator) {
        // Decide to NOT handle negative denominator smartly (e.g. bring out the minus sign from the denominator), where English normalizer also don't handle this.

        // 1. Verbalize numerator iff it's special "1" or "-1".
        char *verbalized_numerator_tmp = "";
        if (token->previous != NULL &&
            token->previous->previous != NULL &&
            token->previous->previous->tag == PV_NORMALIZER_TAG_IT_CARDINAL_ONE) {
            verbalized_numerator_tmp = "UN";
        } else if (token->previous != NULL &&
                   token->previous->previous != NULL &&
                   token->previous->previous->tag == PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL_ONE) {
            verbalized_numerator_tmp = "MENO UN";
        }
        char *verbalized_numerator = (char *) calloc(strlen(verbalized_numerator_tmp) + 1, sizeof(char));
        if (!verbalized_numerator) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("verbalized_numerator"));
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(verbalized_numerator, verbalized_numerator_tmp);

        // 2. Verbalize denominator.
        char *verbalized_denominator = NULL;

        char *string = token->string;

        bool is_special_ordinal = false;
        int32_t index = 0;
        for (int32_t i = 0; i < PV_ARRAY_LEN(DENOMINATOR_ORDINAL_KEYS_IT); i++) {
            if (strcmp(string, DENOMINATOR_ORDINAL_KEYS_IT[i]) == 0) {
                is_special_ordinal = true;
                index = i;
                break;
            }
        }

        bool is_singular =
                ((token->previous != NULL) &&
                 (token->previous->previous != NULL) &&
                 ((strcmp(token->previous->previous->string, "1") == 0) ||
                  (strcmp(token->previous->previous->string, "-1") == 0)));
        if (is_special_ordinal) {
            const char *verbalized = NULL;
            if (is_singular) {
                verbalized = DENOMINATOR_ORDINAL_VALUES_SINGULAR_IT[index];
            } else {
                verbalized = DENOMINATOR_ORDINAL_VALUES_PLURAL_IT[index];
            }

            verbalized_denominator = (char *) calloc(strlen(verbalized) + 1, sizeof(char));
            if (!verbalized_denominator) {
                PV_ERROR_REPORT(
                        &pv_error_msg_alloc,
                        PV_ERROR_ARGS_PUBLIC_EMPTY(),
                        PV_ERROR_ARGS_PRIVATE("verbalized_denominator"));
                return PV_STATUS_OUT_OF_MEMORY;
            }
            strcpy(verbalized_denominator, verbalized);
        } else {
            // 1. Append "º" to string to make the denominator officially an ordinal..
            char *ordinal_symbol_masculine = "º";
            int32_t ordinal_symbol_masculine_num_bytes = 0;
            pv_status_t status = pv_language_num_bytes_character((unsigned char) ordinal_symbol_masculine[0], &ordinal_symbol_masculine_num_bytes);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_language_num_bytes_character,
                        pv_status_to_string(status));
                return PV_STATUS_SUCCESS;
            }
            char *ordinal_string = (char *) calloc(strlen(string) + ordinal_symbol_masculine_num_bytes + 1, sizeof(char));
            if (!ordinal_string) {
                PV_ERROR_REPORT(
                        &pv_error_msg_alloc,
                        PV_ERROR_ARGS_PUBLIC_EMPTY(),
                        PV_ERROR_ARGS_PRIVATE("ordinal_string"));
                return PV_STATUS_OUT_OF_MEMORY;
            }
            strcpy(ordinal_string, string);
            strcat(ordinal_string, ordinal_symbol_masculine);

            // 2. Call <pv_normalizer_verbalizer_ordinal_to_string_masculine()>.
            char *verbalized = NULL;
            status = pv_normalizer_verbalizer_ordinal_to_string_masculine(ordinal_string, &verbalized);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_verbalizer_ordinal_to_string_masculine,
                        pv_status_to_string(status));
                free(ordinal_string);
                return status;
            }
            free(ordinal_string);
            ordinal_string = NULL;

            // 3. If got non-empty string, then pluralize it by replacing the ending "O" with the ending "I".
            if (!verbalized) {
                return PV_STATUS_SUCCESS;
            }
            int32_t verbalized_length = (int32_t) strlen(verbalized);
            if ((!is_singular) && (strlen(verbalized) > 0) && (verbalized[verbalized_length - 1] == 'O')) {
                // All Italian singular ordinals end with "o". All Italian plural ordinals end with "i".
                verbalized[verbalized_length - 1] = 'I';
            }
            verbalized_denominator = verbalized;
            verbalized = NULL;
        }

        // Concatenate everything:
        char *verbalized_internal = NULL;
        int32_t verbalized_internal_length = 0;
        verbalized_internal_length += strlen(verbalized_numerator);
        bool add_space = false;
        if (strlen(verbalized_numerator) > 0) {
            verbalized_internal_length++;
            add_space = true;
        }
        verbalized_internal_length += strlen(verbalized_denominator);
        verbalized_internal = (char *) calloc(verbalized_internal_length + 1, sizeof(char));
        if (!verbalized_internal) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("verbalized_internal"));
            free(verbalized_numerator);
            free(verbalized_denominator);
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(verbalized_internal, verbalized_numerator);
        free(verbalized_numerator);
        verbalized_numerator = NULL;
        if (add_space) {
            strcat(verbalized_internal, " ");
        }
        strcat(verbalized_internal, verbalized_denominator);
        free(verbalized_denominator);
        verbalized_denominator = NULL;

        pv_normalizer_token_set_verbalized(token, verbalized_internal);
    }

    return PV_STATUS_SUCCESS;
}

const char *pv_normalizer_verbalizer_get_previous_ordinary_one_or_negative_one_string(const pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    pv_normalizer_token_t *prev_token = pv_normalizer_token_get_nth_token_before(token, 1, true);
    if (prev_token != NULL && prev_token->tag == PV_NORMALIZER_TAG_IT_CARDINAL_ONE) {
        return "UNO";
    } else if (prev_token != NULL && prev_token->tag == PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL_ONE) {
        return "MENO UNO";
    } else {
        return NULL;
    }
}

pv_normalizer_token_t *pv_normalizer_verbalizer_get_previous_ordinary_one_or_negative_one_token(
        const pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    pv_normalizer_token_t *prev_token = pv_normalizer_token_get_nth_token_before(token, 1, true);
    return (prev_token != NULL &&
            (prev_token->tag == PV_NORMALIZER_TAG_IT_CARDINAL_ONE ||
             prev_token->tag == PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL_ONE))
                   ? prev_token
                   : NULL;
}

pv_status_t pv_normalizer_verbalizer_prepend_to_reverbalize(pv_normalizer_token_t *token, char **prefix, bool no_space, bool free_prefix) {
    PV_ASSERT(token);
    PV_ASSERT(prefix);

    if (*prefix == NULL) {
        // Nothing to append/re-verbalize.
        return PV_STATUS_SUCCESS;
    }
    if (*prefix != NULL && strlen(*prefix) == 0) {
        if (free_prefix) {
            free(*prefix);
            *prefix = NULL;
        }
        return PV_STATUS_SUCCESS;
    }

    char *verbalized_internal = NULL;
    if (token->verbalized == NULL || strlen(token->verbalized) == 0) {
        // Just need to add prefix.
        verbalized_internal = (char *) calloc(strlen(*prefix) + 1, sizeof(char));
        if (!verbalized_internal) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("verbalized_internal"));
            if (free_prefix) {
                free(*prefix);
            }
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(verbalized_internal, *prefix);
        if (free_prefix) {
            free(*prefix);
        }

        pv_normalizer_token_set_verbalized(token, verbalized_internal);

        return PV_STATUS_SUCCESS;
    } else {
        // Append <prefix> to <verbalized_internal>.
        // Append <" "> to <verbalized_internal>.
        // Append copied <token->verbalized> to <verbalized_internal>.
        // Free <token->verbalized>.
        // Assign <verbalized_internal> to <token->verbalized>.

        if (no_space) {
            verbalized_internal = (char *) calloc(strlen(*prefix) + strlen(token->verbalized) + 1, sizeof(char));
        } else {
            verbalized_internal = (char *) calloc(strlen(*prefix) + strlen(" ") + strlen(token->verbalized) + 1, sizeof(char));
        }
        if (!verbalized_internal) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("verbalized_internal"));
            if (free_prefix) {
                free(*prefix);
            }
            return PV_STATUS_OUT_OF_MEMORY;
        }
        strcpy(verbalized_internal, *prefix);
        if (free_prefix) {
            free(*prefix);
        }
        if (!no_space) {
            strcat(verbalized_internal, " ");
        }
        strcat(verbalized_internal, token->verbalized);

        pv_normalizer_token_set_verbalized(token, verbalized_internal);

        return PV_STATUS_SUCCESS;
    }
}

pv_status_t pv_normalizer_verbalizer_get_one_or_negative_one_variations(
        const pv_normalizer_verbalizer_it_t *object,
        const char *noun,
        bool negative,
        char **result,
        bool *no_space) {
    PV_ASSERT(object);
    PV_ASSERT(noun);
    PV_ASSERT(result);
    PV_ASSERT(no_space);

    *no_space = false;

    char *upper_string = NULL;
    pv_status_t status = pv_normalizer_util_upper(noun, &upper_string);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_util_upper,
                pv_status_to_string(status));
        return status;
    }

    pv_noun_gender_t gender = PV_NOUN_GENDER_NO_MATCH;
    gender = pv_noun_gender_dict_gender(object->noun_gender_dict, upper_string);
    if (gender != PV_NOUN_GENDER_MASCULINE && gender != PV_NOUN_GENDER_FEMININE) {
        // Assumed to be default if no gender is found for the noun. Note that when generating gender-noun-dictionary file, if can't decide, then will put Neutral gender, which is meant to be interpreted as unknown because in Italian language, a noun has exactly two genders.
        gender = PV_NOUN_GENDER_MASCULINE;
    }

    char *result_tmp = NULL;
    // Below rule according to "https://www.thoughtco.com/italian-indefinite-articles-4092996".
    if (gender == PV_NOUN_GENDER_MASCULINE) {
        // Use "UNO" for masculine nouns that starts with "z", or that starts with "s" + consonant.
        // Use "UN" for the rest masculine nouns.
        if ((strlen(upper_string) > 0 && upper_string[0] == 'Z') ||
            (strlen(upper_string) > 0 &&
             upper_string[0] == 'S' &&
             strlen(upper_string) > 1 &&
             pv_normalizer_util_it_is_consonant(upper_string[1]))) {
            result_tmp = negative ? "MENO UNO" : "UNO";
        } else {
            result_tmp = negative ? "MENO UN" : "UN";
        }
    } else {
        PV_ASSERT(gender == PV_NOUN_GENDER_FEMININE);
        // Use "UNA" for feminine nouns that begin with a consonant.
        // Use "UN'" for feminine nouns that begin with a vowel.
        if (strlen(upper_string) > 0 && pv_normalizer_util_it_is_consonant(upper_string[0])) {
            result_tmp = negative ? "MENO UNA" : "UNA";
        } else {
            result_tmp = negative ? "MENO UN'" : "UN'";
            *no_space = true;
        }
    }
    free(upper_string);
    upper_string = NULL;

    char *result_internal = calloc(strlen(result_tmp) + 1, sizeof(char));
    if (!result_internal) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("result_internal"));
        return PV_STATUS_OUT_OF_MEMORY;
    }
    strcpy(result_internal, result_tmp);

    free(*result);
    *result = NULL;
    *result = result_internal;
    result_internal = NULL;

    return PV_STATUS_SUCCESS;
}

/*
    Ted: Due to Italian Orca word alignment, can no longer put the verbalization of "1 elephant" into token "elephant", but instead, put the verbalization of "1" into token"1", and the verbalization of "elephant" into "elephant". Hence, below function will verbalize the previous "1" or "-1" token.

    In streaming, we ensure that we don't always decide the streaming chunk to be verbalizable whenever we see space, but rather, we decide the chunk is NOT verbalizable if "1" or "-1" occurs right before.

    However, this has the issue of unable to stream up-to-pace on string "1 1 1 1 ..." due to the current streamer interface because at no token can we say it is verbalizable until the end.

    The easiest fix would be to augment streamer interface to not simply just say if the current token is verbalizable, but rather, the latest token that is verbalizable, e.g., at the second space, it should say the first space is verbalizable.
*/
pv_status_t pv_normalizer_verbalizer_verbalize_previous_one_or_negative_one(const pv_normalizer_token_t *curr, char **string, bool free_string) {
    PV_ASSERT(curr);
    PV_ASSERT(string);

    if (*string == NULL) {
        return PV_STATUS_SUCCESS;
    }
    if (*string != NULL && strlen(*string) == 0) {
        if (free_string) {
            free(*string);
            *string = NULL;
        }
        return PV_STATUS_SUCCESS;
    }

    pv_normalizer_token_t *prev = pv_normalizer_verbalizer_get_previous_ordinary_one_or_negative_one_token(curr);
    if (prev == NULL) {
        if (free_string) {
            free(*string);
            *string = NULL;
        }
        return PV_STATUS_SUCCESS;
    }

    char *verbalized_internal = (char *) calloc(strlen(*string) + 1, sizeof(char));
    if (!verbalized_internal) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("verbalized_internal"));
        if (free_string) {
            free(*string);
            *string = NULL;
        }
        return PV_STATUS_OUT_OF_MEMORY;
    }
    strcpy(verbalized_internal, *string);
    verbalized_internal[strlen(*string)] = '\0';
    pv_normalizer_token_set_verbalized(prev, verbalized_internal);

    if (free_string) {
        free(*string);
        *string = NULL;
    }

    return PV_STATUS_SUCCESS;
}
