#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "core/pv_assert.h"
#include "core/pv_error_messages.h"
#include "core/pv_language.h"
#include "lm/pv_noun_gender_dict.h"
#include "orca/normalizer/pv_normalizer_data/pv_normalizer_shared_data.h"
#include "orca/normalizer/pv_normalizer_tagger.h"
#include "orca/normalizer/pv_normalizer_token.h"
#include "orca/normalizer/pv_normalizer_use_cases.h"
#include "orca/normalizer/pv_normalizer_util.h"

#include "orca/normalizer/de/pv_normalizer_data_de/pv_normalizer_data_de.h"
#include "orca/normalizer/de/pv_normalizer_tagger_de.h"
#include "orca/normalizer/de/pv_normalizer_tags_de.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_normalizer_mock.h"

#endif

struct pv_normalizer_tagger_de {
    const pv_language_info_t *language_info;
    pv_normalizer_language_t language;

    int32_t num_use_cases;
    const pv_normalizer_use_cases_de_t *use_cases;
    pv_normalizer_tokenizer_t *tokenizer;
    pv_normalizer_util_trie_t *measurement_trie;

    pv_noun_gender_dict_t *noun_gender_dict;
};

static pv_status_t pv_normalizer_tagger_de_tag_postprocess(pv_normalizer_token_list_t *token_list);

static void pv_normalizer_tagger_tag_from_language_agnostic(pv_normalizer_token_t *token);

static void pv_normalizer_tagger_synchronize_language_agnostic_tags(pv_normalizer_token_list_t *token_list);

static void pv_normalizer_tagger_tag_word(const pv_language_info_t *language_info, pv_normalizer_token_t *token);

static void pv_normalizer_tagger_tag_custom_pronunciation(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_tagger_tag_punctuation(
        pv_normalizer_language_t language,
        pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_tagger_tag_cardinal(
        const pv_normalizer_tagger_de_t *object,
        pv_normalizer_token_t *token,
        pv_normalizer_token_list_t *token_list);

static void pv_normalizer_tagger_tag_negative_cardinal(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_tagger_tag_number_range(
        const pv_normalizer_tagger_de_t *object,
        pv_normalizer_token_t *token,
        pv_normalizer_token_list_t *token_list);

static pv_status_t get_noun_gender(const char *noun, pv_noun_gender_dict_t *noun_gender_dict, pv_noun_gender_t *noun_gender);

static pv_status_t pv_normalizer_tagger_tag_ordinal(
        const pv_normalizer_tagger_de_t *object,
        pv_normalizer_token_t *tokens,
        pv_normalizer_token_list_t *token_list);

static pv_status_t pv_normalizer_tagger_tag_special_character(
        const pv_normalizer_tagger_de_t *object,
        pv_normalizer_token_t *token);

static void pv_normalizer_tagger_tag_decimal(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_tagger_tag_measurement(
        const pv_normalizer_tagger_de_t *object,
        pv_normalizer_token_list_t *token_list,
        pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_tagger_tag_alphanum_spell_out(
        pv_normalizer_token_t *token,
        pv_normalizer_token_list_t *token_list);

static pv_status_t pv_normalizer_tagger_tag_dot_number(
        pv_normalizer_token_t *token,
        pv_normalizer_token_list_t *token_list);

static pv_status_t pv_normalizer_tagger_tag_fraction(
        const pv_normalizer_tagger_de_t *object,
        pv_normalizer_token_t *token,
        pv_normalizer_token_list_t *token_list);

static pv_status_t merge_comma_number_tokens(
        pv_normalizer_token_t *first,
        pv_normalizer_token_t *last,
        pv_normalizer_token_list_t *token_list);

static bool pv_normalizer_tagger_is_digit_separator(char string);

pv_status_t pv_normalizer_tagger_tag_digits_sequence(
        const pv_normalizer_tokenizer_t *tokenizer,
        pv_normalizer_token_t *token,
        pv_normalizer_token_list_t *token_list);

static bool pv_normalizer_tagger_is_date_separator(const char *string);

static bool pv_normalizer_tagger_is_date_separator_character(char character);

static int32_t pv_normalizer_tagger_get_month_index(const char *string);

static pv_status_t pv_normalizer_tagger_tag_date(
        const pv_normalizer_tokenizer_t *tokenizer,
        pv_normalizer_token_t *token,
        pv_normalizer_token_list_t *token_list);

static pv_status_t pv_normalizer_tagger_tag_url(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_tagger_tag_currency(
        const pv_normalizer_tagger_de_t *object,
        pv_normalizer_token_t *token,
        pv_normalizer_token_list_t *token_list);

static pv_status_t pv_normalizer_tagger_tag_abbreviation(
        pv_normalizer_token_t *token,
        pv_normalizer_token_list_t *token_list);

static pv_status_t pv_normalizer_tagger_tag_name(
        const pv_language_info_t *language_info,
        pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_tagger_de_merge_word_token(pv_normalizer_token_list_t *token_list);

pv_status_t PV_MOCKABLE(pv_normalizer_tagger_de_init)(
        const pv_language_info_t *language_info,
        int32_t num_use_cases,
        const pv_normalizer_use_cases_de_t *use_cases,
        pv_normalizer_tokenizer_t *tokenizer,
        pv_noun_gender_dict_t *noun_gender_dict,
        pv_normalizer_tagger_de_t **object) {
    PV_ASSERT(language_info);
    PV_ASSERT(num_use_cases >= 0);
    PV_ASSERT(use_cases);
    PV_ASSERT(tokenizer);
    PV_ASSERT(noun_gender_dict);

    PV_ASSERT(object);

    *object = NULL;

    pv_normalizer_tagger_de_t *o = calloc(1, sizeof(pv_normalizer_tagger_de_t));
    if (!o) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->language_info = language_info;
    o->num_use_cases = num_use_cases;
    o->use_cases = use_cases;
    o->tokenizer = tokenizer;
    o->noun_gender_dict = noun_gender_dict;

    pv_status_t status = pv_normalizer_util_infer_language_from_language_info(language_info, &(o->language));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_util_infer_language_from_language_info,
                pv_status_to_string(status));
        free(o);
        return status;
    }

    int32_t num_normalizable_characters = 0;
    const char *const *normalizable_characters = NULL;
    status = pv_normalizer_util_get_normalizable_characters(
            o->language,
            &num_normalizable_characters,
            &normalizable_characters);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_util_get_normalizable_characters,
                pv_status_to_string(status));
        free(o);
        return status;
    }

    for (int32_t i = 0; i < num_use_cases; i++) {
        if (use_cases[i] == PV_NORMALIZER_USE_MEASUREMENT_NORMALIZER_DE) {
            status = pv_normalizer_util_trie_create(
                    num_normalizable_characters,
                    normalizable_characters,
                    PV_ARRAY_LEN(PV_NORMALIZER_MEASUREMENTS_DE),
                    (const char **) PV_NORMALIZER_MEASUREMENTS_DE,
                    &(o->measurement_trie));
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_util_trie_create,
                        pv_status_to_string(status));
                free(o);
                return status;
            }
            break;
        }
    }
    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_normalizer_tagger_de_delete)(pv_normalizer_tagger_de_t *object) {
    if (object) {
        pv_normalizer_util_trie_delete(object->measurement_trie);
        free(object);
    }
}

pv_status_t PV_MOCKABLE(pv_normalizer_tagger_de_tag_token)(
        const pv_normalizer_tagger_de_t *object,
        pv_normalizer_token_t *token,
        pv_normalizer_token_list_t *token_list) {
    PV_ASSERT(object);
    PV_ASSERT(token);
    PV_ASSERT(token_list);

    for (int32_t j = 0; j < object->num_use_cases; j++) {
        pv_normalizer_tagger_tag_from_language_agnostic(token);
        if (object->use_cases[j] == PV_NORMALIZER_USE_WORD_NORMALIZER_DE) {
            pv_normalizer_tagger_tag_word(object->language_info, token);
        } else if (object->use_cases[j] == PV_NORMALIZER_USE_CUSTOM_PRONUNCIATION_NORMALIZER_DE) {
            pv_normalizer_tagger_tag_custom_pronunciation(token);
        } else if (object->use_cases[j] == PV_NORMALIZER_USE_PUNCTUATION_NORMALIZER_DE) {
            pv_status_t status = pv_normalizer_tagger_tag_punctuation(object->language, token);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_tag_punctuation,
                        pv_status_to_string(status));
                return status;
            }
        } else if (object->use_cases[j] == PV_NORMALIZER_USE_CARDINAL_NORMALIZER_DE) {
            pv_status_t status = pv_normalizer_tagger_tag_cardinal(object, token, token_list);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_tag_cardinal,
                        pv_status_to_string(status));
                return status;
            }
        } else if (object->use_cases[j] == PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_DE) {
            pv_normalizer_tagger_tag_negative_cardinal(token);
        } else if (object->use_cases[j] == PV_NORMALIZER_USE_NUMBER_RANGE_NORMALIZER_DE) {
            pv_status_t status = pv_normalizer_tagger_tag_number_range(object, token, token_list);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_tag_number_range,
                        pv_status_to_string(status));
                return status;
            }
        } else if (object->use_cases[j] == PV_NORMALIZER_USE_ORDINAL_NORMALIZER_DE) {
            pv_status_t status = pv_normalizer_tagger_tag_ordinal(object, token, token_list);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_tag_ordinal,
                        pv_status_to_string(status));
                return status;
            }
        } else if (object->use_cases[j] == PV_NORMALIZER_USE_SPECIAL_CHARACTER_NORMALIZER_DE) {
            pv_status_t status = pv_normalizer_tagger_tag_special_character(object, token);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_tag_special_character,
                        pv_status_to_string(status));
                return status;
            }
        } else if (object->use_cases[j] == PV_NORMALIZER_USE_DECIMAL_NORMALIZER_DE) {
            pv_normalizer_tagger_tag_decimal(token);
        } else if (object->use_cases[j] == PV_NORMALIZER_USE_MEASUREMENT_NORMALIZER_DE) {
            pv_status_t status = pv_normalizer_tagger_tag_measurement(object, token_list, token);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_tag_measurement,
                        pv_status_to_string(status));
                return status;
            }
        } else if (object->use_cases[j] == PV_NORMALIZER_USE_FRACTION_NORMALIZER_DE) {
            pv_status_t status = pv_normalizer_tagger_tag_fraction(object, token, token_list);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_tag_fraction,
                        pv_status_to_string(status));
                return status;
            }
        } else if (object->use_cases[j] == PV_NORMALIZER_USE_ALPHANUM_SPELL_OUT_NORMALIZER_DE) {
            pv_status_t status = pv_normalizer_tagger_tag_alphanum_spell_out(token, token_list);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_tag_alphanum_spell_out,
                        pv_status_to_string(status));
                return status;
            }
        } else if (object->use_cases[j] == PV_NORMALIZER_USE_URL_NORMALIZER_DE) {
            PV_ASSERT(pv_normalizer_util_is_in_use_cases(
                    object->num_use_cases,
                    (const int32_t *) object->use_cases,
                    PV_NORMALIZER_USE_ALPHANUM_SPELL_OUT_NORMALIZER));

            pv_status_t status = pv_normalizer_tagger_tag_url(token);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_tag_url,
                        pv_status_to_string(status));
                return status;
            }
        } else if (object->use_cases[j] == PV_NORMALIZER_USE_CURRENCY_NORMALIZER_DE) {
            pv_status_t status = pv_normalizer_tagger_tag_currency(object, token, token_list);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_tag_currency,
                        pv_status_to_string(status));
                return status;
            }
        } else if (object->use_cases[j] == PV_NORMALIZER_USE_TIME_NORMALIZER_DE) {
            pv_status_t status = pv_normalizer_tagger_de_tag_time(token, token_list);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_de_tag_time,
                        pv_status_to_string(status));
                return status;
            }
        } else if (object->use_cases[j] == PV_NORMALIZER_USE_ABBREVIATION_NORMALIZER_DE) {
            pv_status_t status = pv_normalizer_tagger_tag_abbreviation(token, token_list);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_tag_abbreviation,
                        pv_status_to_string(status));
                return status;
            }
        } else if (object->use_cases[j] == PV_NORMALIZER_USE_DIGITS_SEQUENCE_NORMALIZER_DE) {
            pv_status_t status = pv_normalizer_tagger_tag_digits_sequence(
                    object->tokenizer,
                    token,
                    token_list);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_tag_digits_sequence,
                        pv_status_to_string(status));
                return status;
            }
        } else if (object->use_cases[j] == PV_NORMALIZER_USE_DATE_NORMALIZER_DE) {
            pv_status_t status = pv_normalizer_tagger_tag_date(
                    object->tokenizer,
                    token,
                    token_list);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_tag_date,
                        pv_status_to_string(status));
                return status;
            }
        } else if (object->use_cases[j] == PV_NORMALIZER_USE_NAME_NORMALIZER_DE) {
            PV_ASSERT(pv_normalizer_util_is_in_use_cases(
                    object->num_use_cases,
                    (const int32_t *) object->use_cases,
                    PV_NORMALIZER_USE_ALPHANUM_SPELL_OUT_NORMALIZER_DE));

            pv_status_t status = pv_normalizer_tagger_tag_name(
                    object->language_info,
                    token);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_tag_name,
                        pv_status_to_string(status));
                return status;
            }
        } else {
            return PV_STATUS_INVALID_ARGUMENT;
        }
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_normalizer_tagger_de_tag)(
        pv_normalizer_tagger_de_t *object,
        pv_normalizer_token_list_t *token_list,
        int32_t num_tokens_skip,
        bool split_untagged) {
    PV_ASSERT(object);
    PV_ASSERT(token_list);
    PV_ASSERT(num_tokens_skip >= 0);

    (void) split_untagged;

    pv_normalizer_token_t *token = token_list->head;
    int32_t i = 0;
    while (token != NULL) {
        if (i < num_tokens_skip) {
            token = token->next;
            i++;
            continue;
        }

        pv_status_t status = pv_normalizer_tagger_de_tag_token(object, token, token_list);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_tagger_de_tag_token,
                    pv_status_to_string(status));
            return status;
        }

        bool did_split = false;
        if (split_untagged && (token->tag == PV_NORMALIZER_TAG_DE_NONE)) {
            pv_normalizer_token_list_t *split_token_list = NULL;

            status = pv_normalizer_tagger_de_split_untagged_token(
                    object,
                    &token,
                    &split_token_list,
                    &did_split);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_de_split_untagged_token,
                        pv_status_to_string(status));
                return status;
            }

            if (split_token_list != NULL) {
                if (split_token_list->size == 0) {
                    did_split = false;
                } else {
                    pv_normalizer_token_list_replace_token_with_list(token_list, &token, split_token_list);
                }
                free(split_token_list);
            }
        }

        if (!did_split) {
            if (token->next != NULL) {
                token->next->previous = token;
            }
            token = token->next;
        }

        i++;
    }

    pv_status_t status = pv_normalizer_tagger_de_tag_postprocess(token_list);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_tagger_de_tag_postprocess,
                pv_status_to_string(status));
        return status;
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_tagger_de_tag_postprocess(pv_normalizer_token_list_t *token_list) {
    PV_ASSERT(token_list);

    pv_status_t status = pv_normalizer_tagger_de_merge_word_token(token_list);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_tagger_de_merge_word_token,
                pv_status_to_string(status));
        return status;
    }

    pv_normalizer_token_t *token = token_list->head;
    while (token != NULL) {
        bool is_spell_out =
                (token->tag == PV_NORMALIZER_TAG_DE_ALPHANUM_SPELL_OUT) ||
                (token->tag == PV_NORMALIZER_TAG_DE_LETTER_SPELL_OUT) ||
                (token->tag == PV_NORMALIZER_TAG_DE_ACRONYM) ||
                (token->tag == PV_NORMALIZER_TAG_DE_NAME_INITIAL_LETTER) ||
                ((token->tag == PV_NORMALIZER_TAG_DE_TOP_LEVEL_DOMAIN) &&
                 PV_NORMALIZER_TOP_LEVEL_DOMAINS_IS_SPELL_OUT_DE[token->tag_data_index]);
        if (is_spell_out) {
            pv_normalizer_token_t *current = token;
            while (strlen(current->string) >= 2) {
                pv_status_t status = pv_normalizer_token_list_unroll_token(1, current, token_list);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_token_list_unroll_token,
                            pv_status_to_string(status));
                    return status;
                }

                if (isdigit(current->string[0])) {
                    current->tag = PV_NORMALIZER_TAG_DE_CARDINAL_SPELL_OUT;
                } else {
                    current->tag = PV_NORMALIZER_TAG_DE_LETTER_SPELL_OUT;
                }

                current = current->next;
            }

            if (isdigit(current->string[0])) {
                current->tag = PV_NORMALIZER_TAG_DE_CARDINAL_SPELL_OUT;
            } else {
                current->tag = PV_NORMALIZER_TAG_DE_LETTER_SPELL_OUT;
            }
        }

        token = token->next;
    }

    pv_normalizer_tagger_synchronize_language_agnostic_tags(token_list);

    return PV_STATUS_SUCCESS;
}

void pv_normalizer_tagger_tag_from_language_agnostic(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    pv_normalizer_tagger_tag_from_language_agnostic_common(
            token,
            PV_NORMALIZER_TAG_DE_NONE,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_PUNCTUATION,
            PV_NORMALIZER_TAG_DE_CUSTOM_PRONUNCIATION);
}

void pv_normalizer_tagger_synchronize_language_agnostic_tags(pv_normalizer_token_list_t *token_list) {
    PV_ASSERT(token_list);

    pv_normalizer_token_list_synchronize_language_agnostic_tags_common(
            token_list,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_PUNCTUATION,
            PV_NORMALIZER_TAG_DE_SINGLE_QUOTE,
            PV_NORMALIZER_TAG_DE_CUSTOM_PRONUNCIATION,
            -1);
}

void pv_normalizer_tagger_tag_word(const pv_language_info_t *language_info, pv_normalizer_token_t *token) {
    PV_ASSERT(language_info);
    PV_ASSERT(token);

    if (PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[PV_NORMALIZER_TAG_DE_WORD]) {
        pv_normalizer_tagger_tag_word_common(language_info, token, PV_NORMALIZER_TAG_DE_WORD);
    }
}

void pv_normalizer_tagger_tag_custom_pronunciation(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[PV_NORMALIZER_TAG_DE_CUSTOM_PRONUNCIATION]) {
        if (token->pronunciation != NULL) {
            token->tag = PV_NORMALIZER_TAG_DE_CUSTOM_PRONUNCIATION;
        }
    }
}

pv_status_t pv_normalizer_tagger_tag_punctuation(
        pv_normalizer_language_t language,
        pv_normalizer_token_t *token) {
    PV_ASSERT(language);
    PV_ASSERT(token);

    if (PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[PV_NORMALIZER_TAG_DE_PUNCTUATION]) {
        return pv_normalizer_tagger_tag_punctuation_common(language, token, PV_NORMALIZER_TAG_DE_PUNCTUATION);
    }

    if (PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[PV_NORMALIZER_TAG_DE_SINGLE_QUOTE]) {
        if (strcmp(token->string, APOSTROPHE) == 0) {
            token->tag = PV_NORMALIZER_TAG_DE_SINGLE_QUOTE;
        }
    }

    return PV_STATUS_SUCCESS;
}


pv_status_t pv_normalizer_tagger_tag_cardinal(
        const pv_normalizer_tagger_de_t *object,
        pv_normalizer_token_t *token,
        pv_normalizer_token_list_t *token_list) {
    PV_ASSERT(object);
    PV_ASSERT(token);
    PV_ASSERT(token_list);

    if (PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[PV_NORMALIZER_TAG_DE_CARDINAL]) {
        const char *string = token->string;
        if (pv_normalizer_util_only_contains_digits(string)) {
            token->tag = PV_NORMALIZER_TAG_DE_CARDINAL;
        } else {
            return PV_STATUS_SUCCESS;
        }

        bool is_before_hyphen = false;
        pv_status_t status = pv_normalizer_util_check_token_is_before_character(token, HYPHEN, &is_before_hyphen);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_util_check_token_is_before_character,
                    pv_status_to_string(status));
            return status;
        }

        bool is_before_decimal =
                (!token->next_character_is_space &&
                 (token->next != NULL) &&
                 (strcmp(token->next->string, ",") == 0) &&
                 !token->next->next_character_is_space);

        bool is_before_punctuation = false;
        if (!token->next_character_is_space && (token->next != NULL)) {
            bool next_is_punctuation = false;
            pv_status_t status = pv_normalizer_util_is_punctuation(object->language, token->next->string, &next_is_punctuation);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_util_is_punctuation,
                        pv_status_to_string(status));
                return status;
            }
            is_before_punctuation = next_is_punctuation && (token->next->next_character_is_space || token->next->next == NULL);
        }
        bool is_before_apostrophe = token->next != NULL && strcmp(token->next->string, APOSTROPHE) == 0;

        if ((token->next_character_is_space ||
             (token->next == NULL) ||
             is_before_hyphen ||
             is_before_decimal ||
             is_before_punctuation ||
             is_before_apostrophe) &&
            (strlen(string) == 3)) {
            pv_status_t status = pv_normalizer_tagger_tag_dot_number(token, token_list);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_tag_dot_number,
                        pv_status_to_string(status));
                return status;
            }
        }
    }

    return PV_STATUS_SUCCESS;
}

void pv_normalizer_tagger_tag_negative_cardinal(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[PV_NORMALIZER_TAG_DE_NEGATIVE_CARDINAL]) {
        const char *string = token->string;

        if (strlen(string) < 2) {
            return;
        }

        if (*string == '-') {
            string++;
        } else {
            return;
        }

        if (pv_normalizer_util_only_contains_digits(string)) {
            token->tag = PV_NORMALIZER_TAG_DE_NEGATIVE_CARDINAL;
        }
    }
}

pv_status_t PV_MOCKABLE(pv_normalizer_tagger_de_split_untagged_token)(
        const pv_normalizer_tagger_de_t *object,
        pv_normalizer_token_t **token,
        pv_normalizer_token_list_t **token_list,
        bool *did_split) {
    PV_ASSERT(object);
    PV_ASSERT(token);
    PV_ASSERT(token_list);
    PV_ASSERT(did_split);

    *token_list = NULL;
    *did_split = false;

    pv_normalizer_token_list_t *split_token_list_internal = NULL;

    char *unsplit_original_string = (*token)->original_string;

    if ((*token)->tag == PV_NORMALIZER_TAG_DE_NONE) {
        const char *string = (*token)->string;

        bool found_hyphen = false;
        bool found_special_character = false;
        char character[PV_NORMALIZER_MAX_NUM_BYTES_PER_CHARACTER] = {0};

        while (*string != '\0') {
            if (*string == '-') {
                found_hyphen = true;
            }

            int32_t num_bytes_character = 0;
            pv_status_t status = pv_language_num_bytes_character((unsigned char) *string, &num_bytes_character);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT(
                        &pv_error_msg_invalid_argument,
                        PV_ERROR_ARGS_PUBLIC("text"),
                        PV_ERROR_ARGS_PRIVATE_EMPTY());
                return status;
            }

            for (int32_t j = 0; j < num_bytes_character; j++) {
                character[j] = *string;
                string++;
            }
            character[num_bytes_character] = '\0';

            bool is_special_character = false;
            status = pv_normalizer_util_is_special_character(
                    object->language,
                    character,
                    &is_special_character,
                    &num_bytes_character);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_util_is_special_character,
                        pv_status_to_string(status));
                return status;
            }
            if (is_special_character) {
                found_special_character = true;
            }
        }

        if (found_special_character) {
            pv_status_t status = pv_normalizer_tokenizer_tokenize(
                    object->tokenizer,
                    (*token)->string,
                    pv_normalizer_tokenizer_default_word_boundary_character(object->tokenizer),
                    false,
                    false,
                    true,
                    false,
                    &split_token_list_internal);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tokenizer_tokenize,
                        pv_status_to_string(status));
                return status;
            }

            *did_split = true;
        } else if (found_hyphen) {
            pv_status_t status = pv_normalizer_tokenizer_tokenize(
                    object->tokenizer,
                    (*token)->string,
                    '-',
                    false,
                    false,
                    false,
                    false,
                    &split_token_list_internal);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tokenizer_tokenize,
                        pv_status_to_string(status));
                return status;
            }
            *did_split = true;
        }
    }
    if (split_token_list_internal) {
        pv_normalizer_token_list_t *correct_original_string_token_list = NULL;
        pv_status_t status = pv_normalizer_token_list_init(&correct_original_string_token_list);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_token_list_init,
                    pv_status_to_string(status));
            free(split_token_list_internal);
            return status;
        }

        pv_normalizer_token_t *current = split_token_list_internal->head;
        for (int32_t i = 0; i < split_token_list_internal->size; i++) {
            char *string = calloc(strlen(current->string) + 1, sizeof(char));
            if (!string) {
                PV_ERROR_REPORT(
                        &pv_error_msg_alloc,
                        PV_ERROR_ARGS_PUBLIC_EMPTY(),
                        PV_ERROR_ARGS_PRIVATE("string"));
                free(split_token_list_internal);
                return PV_STATUS_OUT_OF_MEMORY;
            }
            strcpy(string, current->string);

            char *original_string = calloc(strlen(unsplit_original_string) + 1, sizeof(char));
            if (!original_string) {
                PV_ERROR_REPORT(
                        &pv_error_msg_alloc,
                        PV_ERROR_ARGS_PUBLIC_EMPTY(),
                        PV_ERROR_ARGS_PRIVATE("original_string"));
                free(string);
                free(split_token_list_internal);
                return PV_STATUS_OUT_OF_MEMORY;
            }
            strcpy(original_string, unsplit_original_string);

            bool next_character_is_space = current->next_character_is_space;
            if (i == (split_token_list_internal->size - 1)) {
                next_character_is_space = (*token)->next_character_is_space;
            }

            pv_normalizer_token_t *new_token = NULL;
            status = pv_normalizer_token_init_with_original_string(
                    string,
                    original_string,
                    false,
                    next_character_is_space,
                    split_token_list_internal->size - 1 - i,
                    i,
                    &new_token);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_token_list_init,
                        pv_status_to_string(status));
                free(original_string);
                free(string);
                free(split_token_list_internal);
                return status;
            }

            new_token->next_character_is_hyphen = current->next_character_is_hyphen;
            pv_normalizer_token_list_append_token(correct_original_string_token_list, new_token);

            current = current->next;
        }

        pv_normalizer_token_list_delete(split_token_list_internal);
        split_token_list_internal = correct_original_string_token_list;
    }

    *token_list = split_token_list_internal;

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_tagger_tag_number_range(
        const pv_normalizer_tagger_de_t *object,
        pv_normalizer_token_t *token,
        pv_normalizer_token_list_t *token_list) {
    PV_ASSERT(object);
    PV_ASSERT(token);
    PV_ASSERT(token_list);

    if (PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[PV_NORMALIZER_TAG_DE_NUMBER_RANGE_TO]) {
        const char *string = token->string;

        bool is_range = true;
        int32_t num_hyphens = 0;
        int32_t num_cardinals = 0;
        int32_t hyphen_index = 0;

        int32_t i = 0;
        while (string[i] != '\0' && is_range) {
            if (string[i] == '-') {
                hyphen_index = i;
                num_hyphens++;
                i++;
            } else {
                num_cardinals++;
                while ((string[i] != '\0') && (string[i] != '-')) {
                    if (!isdigit(string[i])) {
                        is_range = false;
                        break;
                    }
                    i++;
                }
            }
        }

        if (is_range && (num_hyphens == 1) && (num_cardinals == 2)) {
            pv_status_t status = pv_normalizer_token_list_unroll_token(hyphen_index, token, token_list);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_token_list_unroll_token,
                        pv_status_to_string(status));
                return status;
            }

            status = pv_normalizer_tagger_tag_cardinal(object, token, token_list);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_tag_cardinal,
                        pv_status_to_string(status));
                return status;
            }

            status = pv_normalizer_token_list_unroll_token(1, token->next, token_list);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_token_list_unroll_token,
                        pv_status_to_string(status));
                return status;
            }

            status = pv_normalizer_tagger_tag_cardinal(object, token->next->next, token_list);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_tag_cardinal,
                        pv_status_to_string(status));
                return status;
            }

            token->next->tag = PV_NORMALIZER_TAG_DE_NUMBER_RANGE_TO;
        }
    }
    return PV_STATUS_SUCCESS;
}

pv_status_t get_noun_gender(const char *noun, pv_noun_gender_dict_t *noun_gender_dict, pv_noun_gender_t *gender) {
    const int32_t max_token_str_len = (int32_t) (strlen(noun) * PV_LANGUAGE_MAX_NUM_BYTES_PER_CODEPOINT_UTF8);
    char noun_uppercased[max_token_str_len + 1];
    memset(noun_uppercased, 0, max_token_str_len + 1);

    pv_status_t status = pv_language_utf8_str_to_upper((unsigned char *) noun, (unsigned char *) noun_uppercased);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_language_utf8_str_to_upper,
                pv_status_to_string(status));
        return status;
    }

    const char *seasons_masculine[] = {"SOMMER", "HERBST", "WINTER"};
    for (int i = 0; i < 3; i++) {
        if (strcmp(noun_uppercased, seasons_masculine[i]) == 0) {
            *gender = PV_NOUN_GENDER_MASCULINE;
            return PV_STATUS_SUCCESS;
        }
    }

    if (strcmp(noun_uppercased, "FRÜHLING") == 0) {
        *gender = PV_NOUN_GENDER_NEUTER;
        return PV_STATUS_SUCCESS;
    }

    const char *months[] = {
            "JANUAR", "FEBRUAR", "MÄRZ", "APRIL", "MAI", "JUNI", "JULI", "AUGUST", "SEPTEMBER", "OKTOBER", "NOVEMBER", "DEZEMBER"};
    for (int i = 0; i < 12; i++) {
        if (strcmp(noun_uppercased, months[i]) == 0) {
            *gender = PV_NOUN_GENDER_MASCULINE;
            return PV_STATUS_SUCCESS;
        }
    }

    const char *days_of_week[] = {"MONTAG", "DIENSTAG", "MITTWOCH", "DONNERSTAG", "FREITAG", "SAMSTAG", "SONNTAG"};
    for (int i = 0; i < 7; i++) {
        if (strcmp(noun_uppercased, days_of_week[i]) == 0) {
            *gender = PV_NOUN_GENDER_MASCULINE;
            return PV_STATUS_SUCCESS;
        }
    }

    const char nouns_gender = pv_noun_gender_dict_gender(noun_gender_dict, noun_uppercased);
    if (nouns_gender != PV_NOUN_GENDER_NO_MATCH) {
        *gender = nouns_gender;
        return PV_STATUS_SUCCESS;
    }

    *gender = PV_NOUN_GENDER_NO_MATCH;
    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_tagger_tag_ordinal(
        const pv_normalizer_tagger_de_t *object,
        pv_normalizer_token_t *token,
        pv_normalizer_token_list_t *token_list) {
    PV_ASSERT(token);

    if (strlen(token->string) > 0) {
        pv_noun_gender_t gender = PV_NOUN_GENDER_NO_MATCH;
        if (token->tag == PV_NORMALIZER_TAG_DE_WORD || token->tag == PV_NORMALIZER_TAG_DE_DATE_MONTH) {
            pv_status_t status = get_noun_gender(token->string, object->noun_gender_dict, &gender);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        get_noun_gender,
                        pv_status_to_string(status));
                return status;
            }
        }

        if (gender == PV_NOUN_GENDER_NO_MATCH) {
            return PV_STATUS_SUCCESS;
        }

        pv_normalizer_token_t *three_previous = pv_normalizer_token_get_nth_token_before(token, 3, false);

        if (three_previous != NULL && strlen(three_previous->string) > 0) {
            pv_normalizer_token_t *two_previous = pv_normalizer_token_get_nth_token_before(token, 2, false);
            pv_normalizer_token_t *one_previous = pv_normalizer_token_get_nth_token_before(token, 1, false);

            bool is_higher_or_equal_weight =
                    (PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[two_previous->tag] <=
                     PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[PV_NORMALIZER_TAG_DE_ORDINAL_DOT]) &&
                    (PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[three_previous->tag] <=
                     PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[PV_NORMALIZER_TAG_DE_ORDINAL_FEMININE]) &&
                    (PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[three_previous->tag] <=
                     PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[PV_NORMALIZER_TAG_DE_ORDINAL_MASCULINE]) &&
                    (PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[three_previous->tag] <=
                     PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[PV_NORMALIZER_TAG_DE_ORDINAL_NEUTER]) &&
                    (PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[three_previous->tag] <=
                     PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[PV_NORMALIZER_TAG_DE_NEGATIVE_ORDINAL_FEMININE]) &&
                    (PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[three_previous->tag] <=
                     PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[PV_NORMALIZER_TAG_DE_NEGATIVE_ORDINAL_MASCULINE]) &&
                    (PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[three_previous->tag] <=
                     PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[PV_NORMALIZER_TAG_DE_NEGATIVE_ORDINAL_NEUTER]);
            if (!is_higher_or_equal_weight) {
                return PV_STATUS_SUCCESS;
            }

            int32_t number_token_idx = 3;
            bool is_negative_symbol_found = false;
            while (!is_negative_symbol_found) {
                pv_normalizer_token_t *number_token = pv_normalizer_token_get_nth_token_before(token, number_token_idx, false);
                if (pv_normalizer_token_is_space_or_null(number_token)) {
                    break;
                }

                if (strcmp(number_token->string, ".") == 0) {
                    number_token_idx++;
                    continue;
                } else {
                    int32_t i = (int32_t) (strlen(number_token->string) - 1);
                    while (i >= 0) {
                        if (!isdigit(number_token->string[i])) {
                            if (i == 0 && number_token->string[i] == '-') {
                                is_negative_symbol_found = true;
                            }

                            break;
                        }
                        i--;
                    }
                }

                number_token_idx++;
            }

            if (!is_negative_symbol_found && !isdigit(three_previous->string[0])) {
                return PV_STATUS_SUCCESS;
            }

            int32_t i = 1;
            while (three_previous->string[i] != '\0') {
                if (!isdigit(three_previous->string[i])) {
                    return PV_STATUS_SUCCESS;
                }
                i++;
            }

            bool is_after_number_and_dot_and_space =
                    (strcmp(two_previous->string, ".") == 0) &&
                    (one_previous->tag == PV_NORMALIZER_TAG_DE_SPACE);

            if (is_after_number_and_dot_and_space) {
                // Handle invalid dot numbers (e.g. 1.2345., 1.1.000.)
                pv_normalizer_token_t *start_token = pv_normalizer_token_get_token_after_previous_space(three_previous);
                char *full_number_str = NULL;
                pv_status_t status = pv_normalizer_token_concatenate_token_strings(
                        start_token,
                        three_previous->next,
                        &full_number_str);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_token_concatenate_token_strings,
                            pv_status_to_string(status));
                    free(full_number_str);
                    return status;
                }
                int32_t full_number_str_idx = 0;
                bool is_number_contains_dot = false;
                while (full_number_str[full_number_str_idx] != '\0') {
                    if (full_number_str[full_number_str_idx] == '.') {
                        is_number_contains_dot = true;
                        break;
                    }
                    full_number_str_idx++;
                }
                free(full_number_str);

                if (is_number_contains_dot) {
                    pv_normalizer_token_t *current_num_check = three_previous;
                    while (!pv_normalizer_token_is_space_or_null(current_num_check)) {
                        if (strcmp(current_num_check->string, ".") != 0) {
                            if (pv_normalizer_token_is_space_or_null(current_num_check->previous)) {
                                size_t max_num_str_len = current_num_check->string[0] == '-' ? 4 : 3;
                                if (strlen(current_num_check->string) > max_num_str_len) {
                                    return PV_STATUS_SUCCESS;
                                }
                                break;
                            }

                            if (strlen(current_num_check->string) != 3) {
                                return PV_STATUS_SUCCESS;
                            }
                        }

                        current_num_check = current_num_check->previous;
                    }
                }

                if (strlen(three_previous->string) == 3) {
                    status = pv_normalizer_tagger_tag_dot_number(three_previous, token_list);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_normalizer_tagger_tag_dot_number,
                                pv_status_to_string(status));
                        return status;
                    }
                }

                if (pv_normalizer_util_string_number_greater_than_int(three_previous->string, MAX_VERBALIZED_CARDINAL_DE)) {
                    return PV_STATUS_SUCCESS;
                }

                two_previous->tag = PV_NORMALIZER_TAG_DE_ORDINAL_DOT;
                if (is_negative_symbol_found) {
                    if (gender == PV_NOUN_GENDER_FEMININE) {
                        three_previous->tag = PV_NORMALIZER_TAG_DE_NEGATIVE_ORDINAL_FEMININE;
                    } else if (gender == PV_NOUN_GENDER_MASCULINE) {
                        three_previous->tag = PV_NORMALIZER_TAG_DE_NEGATIVE_ORDINAL_MASCULINE;
                    } else {
                        three_previous->tag = PV_NORMALIZER_TAG_DE_NEGATIVE_ORDINAL_NEUTER;
                    }
                } else {
                    if (gender == PV_NOUN_GENDER_FEMININE) {
                        three_previous->tag = PV_NORMALIZER_TAG_DE_ORDINAL_FEMININE;
                    } else if (gender == PV_NOUN_GENDER_MASCULINE) {
                        three_previous->tag = PV_NORMALIZER_TAG_DE_ORDINAL_MASCULINE;
                    } else {
                        three_previous->tag = PV_NORMALIZER_TAG_DE_ORDINAL_NEUTER;
                    }
                }
            }
        }
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_tagger_tag_special_character(
        const pv_normalizer_tagger_de_t *object,
        pv_normalizer_token_t *token) {
    PV_ASSERT(object);
    PV_ASSERT(token);

    if (PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[PV_NORMALIZER_TAG_DE_SPECIAL_CHARACTER]) {
        const char *string = token->string;

        bool is_special_character = false;
        int32_t special_character_length = 0;
        pv_status_t status = pv_normalizer_util_is_special_character(
                object->language,
                string,
                &is_special_character,
                &special_character_length);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_util_is_special_character,
                    pv_status_to_string(status));
            return status;
        }

        if (is_special_character && special_character_length == (int32_t) strlen(string)) {
            token->tag = PV_NORMALIZER_TAG_DE_SPECIAL_CHARACTER;
        }
    }

    return PV_STATUS_SUCCESS;
}

void pv_normalizer_tagger_tag_decimal(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[PV_NORMALIZER_TAG_DE_DECIMAL_DIGITS]) {

        bool has_comma_before_current =
                (token->previous != NULL) &&
                (strcmp(token->previous->string, ",") == 0) &&
                (!token->previous->next_character_is_space);

        if (has_comma_before_current) {
            pv_normalizer_token_t *two_previous = pv_normalizer_token_get_nth_token_before(token, 2, false);
            bool has_space_or_null_before_comma =
                    (two_previous == NULL) ||
                    two_previous->next_character_is_space ||
                    (two_previous->tag == PV_NORMALIZER_TAG_DE_SPACE);
            bool has_cardinal_before_comma =
                    ((two_previous != NULL) &&
                     ((two_previous->tag == PV_NORMALIZER_TAG_DE_CARDINAL) ||
                      (two_previous->tag == PV_NORMALIZER_TAG_DE_NEGATIVE_CARDINAL)) &&
                     !two_previous->next_character_is_space);

            if (has_space_or_null_before_comma || has_cardinal_before_comma) {
                const char *string = token->string;

                bool found_non_digit = false;
                while (*string != '\0') {
                    if (!(isdigit(*string))) {
                        found_non_digit = true;
                        break;
                    }
                    string++;
                }
                if (!found_non_digit) {
                    token->tag = PV_NORMALIZER_TAG_DE_DECIMAL_DIGITS;
                    token->previous->tag = PV_NORMALIZER_TAG_DE_DECIMAL_POINT;
                }
            }
        }
    }
}

pv_status_t pv_normalizer_tagger_tag_measurement(
        const pv_normalizer_tagger_de_t *object,
        pv_normalizer_token_list_t *token_list,
        pv_normalizer_token_t *token) {
    PV_ASSERT(object);
    PV_ASSERT(token_list);
    PV_ASSERT(token);

    if (PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[PV_NORMALIZER_TAG_DE_MEASUREMENT]) {
        const char *string = token->string;

        // Space-separated number + measurement eg. `2 ml`
        pv_normalizer_token_t *previous = pv_normalizer_token_get_nth_token_before(token, 1, true);
        if ((previous != NULL) &&
            ((previous->tag == PV_NORMALIZER_TAG_DE_CARDINAL) ||
             (previous->tag == PV_NORMALIZER_TAG_DE_NEGATIVE_CARDINAL) ||
             (previous->tag == PV_NORMALIZER_TAG_DE_DECIMAL_DIGITS))) {
            int32_t trie_index = -1;
            pv_status_t status = pv_normalizer_util_trie_search(object->measurement_trie, string, &trie_index);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_util_trie_search,
                        pv_status_to_string(status));
                return status;
            }

            if (trie_index >= 0) {
                token->tag = PV_NORMALIZER_TAG_DE_MEASUREMENT;
                token->tag_data_index = trie_index;
                return PV_STATUS_SUCCESS;
            }
        }

        // No context measurement chains eg. `lb-ft` and `lb.-ft.`
        pv_normalizer_token_t *two_previous = pv_normalizer_token_get_nth_token_before(token, 2, true);
        if ((previous != NULL) && (previous->tag == PV_NORMALIZER_TAG_DE_WORD)) {
            int32_t trie_index = -1;
            pv_status_t status = pv_normalizer_util_trie_search(object->measurement_trie, string, &trie_index);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_util_trie_search,
                        pv_status_to_string(status));
                return status;
            }

            int32_t prev_trie_index = -1;
            status = pv_normalizer_util_trie_search(object->measurement_trie, previous->string, &prev_trie_index);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_util_trie_search,
                        pv_status_to_string(status));
                return status;
            }

            if (trie_index >= 0 && prev_trie_index >= 0) {
                token->tag = PV_NORMALIZER_TAG_DE_MEASUREMENT;
                token->tag_data_index = trie_index;

                previous->tag = PV_NORMALIZER_TAG_DE_MEASUREMENT;
                previous->tag_data_index = prev_trie_index;
                return PV_STATUS_SUCCESS;
            }
        } else if ((previous != NULL) && (previous->tag == PV_NORMALIZER_TAG_DE_DOT) &&
                   (two_previous != NULL) && (two_previous->tag == PV_NORMALIZER_TAG_DE_WORD)) {
            int32_t trie_index = -1;
            pv_status_t status = pv_normalizer_util_trie_search(object->measurement_trie, string, &trie_index);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_util_trie_search,
                        pv_status_to_string(status));
                return status;
            }

            int32_t prev_trie_index = -1;
            status = pv_normalizer_util_trie_search(object->measurement_trie, two_previous->string, &prev_trie_index);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_util_trie_search,
                        pv_status_to_string(status));
                return status;
            }

            if (trie_index >= 0 && prev_trie_index >= 0) {
                token->tag = PV_NORMALIZER_TAG_DE_MEASUREMENT;
                token->tag_data_index = trie_index;

                two_previous->tag = PV_NORMALIZER_TAG_DE_MEASUREMENT;
                two_previous->tag_data_index = prev_trie_index;
                return PV_STATUS_SUCCESS;
            }
        }

        // Contextual measurement chains eg. `2 lb ft` and `2 lb.-ft.`
        if (((previous != NULL) && (previous->tag == PV_NORMALIZER_TAG_DE_MEASUREMENT)) ||
            ((two_previous != NULL) && (two_previous->tag == PV_NORMALIZER_TAG_DE_MEASUREMENT))) {
            int32_t trie_index = -1;
            pv_status_t status = pv_normalizer_util_trie_search(object->measurement_trie, string, &trie_index);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_util_trie_search,
                        pv_status_to_string(status));
                return status;
            }

            if (trie_index >= 0) {
                token->tag = PV_NORMALIZER_TAG_DE_MEASUREMENT;
                token->tag_data_index = trie_index;
                return PV_STATUS_SUCCESS;
            }
        }

        int32_t length = (int32_t) strlen(string);

        // Non-space-separated number + measurement eg. `-2ml`
        if ((length > 1) && (isdigit(string[0]) || ((string[0] == '-') && isdigit(string[1])))) {
            int32_t split_index = 1;
            while ((split_index < length) && isdigit(string[split_index])) {
                split_index++;
            }

            if ((split_index) && (split_index < length)) {
                int32_t trie_index = -1;
                pv_status_t status = pv_normalizer_util_trie_search(
                        object->measurement_trie,
                        string + split_index,
                        &trie_index);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_util_trie_search,
                            pv_status_to_string(status));
                    return status;
                }

                if (trie_index >= 0) {
                    status = pv_normalizer_token_list_unroll_token(split_index, token, token_list);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_normalizer_token_list_unroll_token,
                                pv_status_to_string(status));
                        return status;
                    }

                    pv_normalizer_token_t *measurement_token = token->next;

                    status = pv_normalizer_tagger_de_tag_token(object, token, token_list);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_normalizer_tagger_de_tag_token,
                                pv_status_to_string(status));
                        return status;
                    }

                    if ((token->tag == PV_NORMALIZER_TAG_DE_CARDINAL ||
                         token->tag == PV_NORMALIZER_TAG_DE_DIGITS) &&
                        strlen(token->string) == 3) {
                        status = pv_normalizer_tagger_tag_dot_number(token, token_list);
                        if (status != PV_STATUS_SUCCESS) {
                            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                    pv_normalizer_tagger_tag_dot_number,
                                    pv_status_to_string(status));
                            return status;
                        }
                        if (token->tag == PV_NORMALIZER_TAG_DE_DIGITS) {
                            token->tag = PV_NORMALIZER_TAG_DE_CARDINAL;
                        }
                    }

                    if (token->tag == PV_NORMALIZER_TAG_DE_CARDINAL ||
                        token->tag == PV_NORMALIZER_TAG_DE_NEGATIVE_CARDINAL ||
                        token->tag == PV_NORMALIZER_TAG_DE_DECIMAL_DIGITS) {
                        measurement_token->tag = PV_NORMALIZER_TAG_DE_MEASUREMENT;
                        measurement_token->tag_data_index = trie_index;
                    }

                    return PV_STATUS_SUCCESS;
                }
            }
        }

        // Per measurements eg. `1.2m/s`
        if (length > 2) {
            int32_t split_index = 1;
            while ((split_index < length) && (string[split_index] != '/')) {
                split_index++;
            }

            if ((split_index > 0) && (split_index < length)) {
                int32_t trie_index = -1;
                pv_status_t status = pv_normalizer_util_trie_search(
                        object->measurement_trie,
                        string + (split_index + 1),
                        &trie_index);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_util_trie_search,
                            pv_status_to_string(status));
                    return status;
                }

                if (trie_index >= 0) {
                    status = pv_normalizer_token_list_unroll_token(split_index, token, token_list);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_normalizer_token_list_unroll_token,
                                pv_status_to_string(status));
                        return status;
                    }

                    pv_normalizer_token_t *next = token->next;

                    status = pv_normalizer_tagger_de_tag_token(object, token, token_list);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_normalizer_tagger_de_tag_token,
                                pv_status_to_string(status));
                        return status;
                    }

                    pv_normalizer_token_t *next_previous = pv_normalizer_token_get_nth_token_before(next, 1, false);
                    if (next_previous->tag == PV_NORMALIZER_TAG_DE_MEASUREMENT) {
                        status = pv_normalizer_token_list_unroll_token(1, next, token_list);
                        if (status != PV_STATUS_SUCCESS) {
                            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                    pv_normalizer_token_list_unroll_token,
                                    pv_status_to_string(status));
                            return status;
                        }
                        next->tag = PV_NORMALIZER_TAG_DE_PER_SLASH;

                        next->next->tag = PV_NORMALIZER_TAG_DE_MEASUREMENT;
                        next->next->tag_data_index = trie_index;
                    } else {
                        int32_t trie_index_two = -1;
                        status = pv_normalizer_util_trie_search(
                                object->measurement_trie,
                                next->previous->string,
                                &trie_index_two);
                        if (status != PV_STATUS_SUCCESS) {
                            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                    pv_normalizer_util_trie_search,
                                    pv_status_to_string(status));
                            return status;
                        }

                        if (trie_index_two >= 0) {
                            status = pv_normalizer_token_list_unroll_token(1, next, token_list);
                            if (status != PV_STATUS_SUCCESS) {
                                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                        pv_normalizer_token_list_unroll_token,
                                        pv_status_to_string(status));
                                return status;
                            }
                            next->tag = PV_NORMALIZER_TAG_DE_PER_SLASH;

                            next_previous->tag = PV_NORMALIZER_TAG_DE_MEASUREMENT;
                            next_previous->tag_data_index = trie_index_two;

                            next->next->tag = PV_NORMALIZER_TAG_DE_MEASUREMENT;
                            next->next->tag_data_index = trie_index;
                        }
                    }

                    return PV_STATUS_SUCCESS;
                }
            }
        }
    }

    // `km/hour`
    if (PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[PV_NORMALIZER_TAG_DE_PER_SLASH]) {

        pv_normalizer_token_t *previous = pv_normalizer_token_get_nth_token_before(token, 1, false);
        pv_normalizer_token_t *next = pv_normalizer_token_get_nth_token_after(token, 1, false);
        if ((strcmp(token->string, "/") == 0) && (previous != NULL) && (next != NULL)) {
            char *previous_upper_string = NULL;
            pv_status_t status = pv_normalizer_util_upper(previous->string, &previous_upper_string);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_util_upper,
                        pv_status_to_string(status));
                return status;
            }

            char *next_upper_string = NULL;
            status = pv_normalizer_util_upper(next->string, &next_upper_string);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_util_upper,
                        pv_status_to_string(status));
                return status;
            }

            bool measurement_before = false;
            bool measurement_after = false;

            for (int32_t i = 0; i < PV_NORMALIZER_NUM_MEASUREMENTS_DE; i++) {
                if (strcmp(previous_upper_string, PV_NORMALIZER_MEASUREMENTS_DE[i]) == 0) {
                    measurement_before = true;
                    break;
                }
            }
            for (int32_t i = 0; i < PV_NORMALIZER_NUM_MEASUREMENTS_DE; i++) {
                if (strcmp(previous_upper_string, PV_NORMALIZER_MEASUREMENTS_VERBALIZED_PLURAL_DE[i]) == 0) {
                    measurement_before = true;
                    break;
                }
            }
            for (int32_t i = 0; i < PV_NORMALIZER_NUM_MEASUREMENTS_DE; i++) {
                if (strcmp(previous_upper_string, PV_NORMALIZER_MEASUREMENTS_VERBALIZED_SINGULAR_DE[i]) == 0) {
                    measurement_before = true;
                    break;
                }
            }
            free(previous_upper_string);

            for (int32_t i = 0; i < PV_NORMALIZER_NUM_MEASUREMENTS_DE; i++) {
                if (strcmp(next_upper_string, PV_NORMALIZER_MEASUREMENTS_DE[i]) == 0) {
                    measurement_after = true;
                    break;
                }
            }
            for (int32_t i = 0; i < PV_NORMALIZER_NUM_MEASUREMENTS_DE; i++) {
                if (strcmp(next_upper_string, PV_NORMALIZER_MEASUREMENTS_VERBALIZED_PLURAL_DE[i]) == 0) {
                    measurement_after = true;
                    break;
                }
            }
            for (int32_t i = 0; i < PV_NORMALIZER_NUM_MEASUREMENTS_DE; i++) {
                if (strcmp(next_upper_string, PV_NORMALIZER_MEASUREMENTS_VERBALIZED_SINGULAR_DE[i]) == 0) {
                    measurement_after = true;
                    break;
                }
            }
            free(next_upper_string);

            if (measurement_before && measurement_after) {
                token->tag = PV_NORMALIZER_TAG_DE_PER_SLASH;
            }
        }
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_tagger_tag_alphanum_spell_out(
        pv_normalizer_token_t *token,
        pv_normalizer_token_list_t *token_list) {
    PV_ASSERT(token);
    PV_ASSERT(token_list);

    const char *string = token->string;
    int32_t length = (int32_t) strlen(string);

    if (length <= 1) {
        return PV_STATUS_SUCCESS;
    }

    if (PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[PV_NORMALIZER_TAG_DE_ALPHANUM_SPELL_OUT]) {

        int32_t found_digit = false;
        int32_t found_alpha = false;
        int32_t i = 0;
        while (i < length) {
            if (!(isalnum(string[i]))) {
                return PV_STATUS_SUCCESS;
            }
            if (isdigit(string[i])) {
                found_digit = true;
            }
            if (isalpha(string[i])) {
                found_alpha = true;
            }

            i++;
        }

        if (found_digit && found_alpha) {
            token->tag = PV_NORMALIZER_TAG_DE_ALPHANUM_SPELL_OUT;
        }
    }

    // SPECIAL CASES: 400BC, 800AD, 800BCE, etc.
    if ((token->tag == PV_NORMALIZER_TAG_DE_ALPHANUM_SPELL_OUT) && (length >= 3)) {
        if (isdigit(string[length - 1]) || isdigit(string[length - 2])) {
            return PV_STATUS_SUCCESS;
        } else {
            int32_t index_last_digit = -1;
            if ((string[length - 1] == 'C') || (string[length - 1] == 'D') || (string[length - 1] == 'E')) {
                if ((string[length - 2] == 'B') || (string[length - 2] == 'A')) { // BC, AD
                    index_last_digit = length - 3;
                } else if ((string[length - 2] == 'C') && (string[length - 3] == 'B') && length >= 4) { // BCE
                    index_last_digit = length - 4;
                }
            }

            if ((index_last_digit == -1) || (length - index_last_digit > 6)) { // maximum year is 999,999
                return PV_STATUS_SUCCESS;
            }

            for (int32_t i = 0; i < index_last_digit + 1; i++) {
                if (!isdigit(string[i])) {
                    return PV_STATUS_SUCCESS;
                }
            }

            pv_status_t status = pv_normalizer_token_list_unroll_token(index_last_digit + 1, token, token_list);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_token_list_unroll_token,
                        pv_status_to_string(status));
                return status;
            }

            pv_normalizer_token_t *ad_bc_token = token->next;

            ad_bc_token->tag = PV_NORMALIZER_TAG_DE_LETTER_SPELL_OUT;
            token->tag = PV_NORMALIZER_TAG_DE_CARDINAL;
        }
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_normalizer_tagger_de_tag_time)(
        pv_normalizer_token_t *token,
        pv_normalizer_token_list_t *token_list) {
    PV_ASSERT(token);
    PV_ASSERT(token_list);

    if (PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[token->tag] >=
        PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[PV_NORMALIZER_TAG_DE_TIME_HOURS]) {
        return PV_STATUS_SUCCESS;
    }

    int32_t num_tokens_before = pv_normalizer_token_get_num_tokens_before(token);

    // TAG: hours/minutes (e.g. 7:31, tokenized to [7, :, 31])
    bool can_be_minutes = (strlen(token->string) == 2) && isdigit(token->string[0]) && isdigit(token->string[1]);
    if (can_be_minutes) {
        bool has_colon_before_current = (num_tokens_before > 0) && (strcmp(token->previous->string, ":") == 0);
        bool has_hour_before_colon =
                (num_tokens_before > 1) &&
                (token->previous->previous->tag == PV_NORMALIZER_TAG_DE_CARDINAL) &&
                !pv_normalizer_util_string_number_greater_than_int(token->previous->previous->string, 24);

        if (has_colon_before_current && has_hour_before_colon) {
            bool smaller_than_61 =
                    !pv_normalizer_util_string_number_greater_than_int(token->string, 60);

            if (smaller_than_61) {
                token->previous->previous->tag = PV_NORMALIZER_TAG_DE_TIME_HOURS;
                token->previous->tag = PV_NORMALIZER_TAG_DE_TIME_COLON;
                token->tag = PV_NORMALIZER_TAG_DE_TIME_MINUTES;

                return PV_STATUS_SUCCESS;
            }
        }
    }

    // TAG: " uhr", " UHR", " Uhr"
    if (num_tokens_before > 0) {
        if ((strcmp(token->string, "uhr") == 0) ||
            (strcmp(token->string, "UHR") == 0) ||
            (strcmp(token->string, "Uhr") == 0)) {

            bool is_space_before = (token->previous->tag == PV_NORMALIZER_TAG_DE_SPACE);
            if (is_space_before) {
                pv_normalizer_token_t *two_previous = pv_normalizer_token_get_nth_token_before(token, 2, false);
                bool is_only_hour_time_before_current =
                        (two_previous != NULL) &&
                        (two_previous->tag == PV_NORMALIZER_TAG_DE_CARDINAL) &&
                        !pv_normalizer_util_string_number_greater_than_int(two_previous->string, 24) &&
                        pv_normalizer_token_is_space_or_null(pv_normalizer_token_get_nth_token_before(token, 3, false));
                bool has_time_before_current =
                        ((two_previous != NULL) &&
                         ((two_previous->tag == PV_NORMALIZER_TAG_DE_TIME_HOURS) ||
                          (two_previous->tag == PV_NORMALIZER_TAG_DE_TIME_MINUTES))) ||
                        is_only_hour_time_before_current;
                if (has_time_before_current) {
                    token->tag = PV_NORMALIZER_TAG_DE_TIME_UHR;
                    pv_status_t status = pv_normalizer_util_upper_inplace(token->string);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_normalizer_util_upper_inplace,
                                pv_status_to_string(status));
                        return status;
                    }

                    return PV_STATUS_SUCCESS;
                }
            }
        }
    }

    // TAG: "7:00uhr" (tokenized to [7, :, 00uhr])
    if (num_tokens_before > 1) {
        const char *string = token->string;
        bool can_be_minutes_and_uhr = (strlen(string) == 5);
        if (can_be_minutes_and_uhr) {
            bool has_colon_before_current = (strcmp(token->previous->string, ":") == 0);
            bool has_hour_before_colon =
                    (token->previous->previous->tag == PV_NORMALIZER_TAG_DE_CARDINAL) &&
                    !pv_normalizer_util_string_number_greater_than_int(token->previous->previous->string, 24);

            bool has_digits = isdigit(string[0]) && isdigit(string[1]);
            bool has_uhr =
                    ((string[2] == 'u') || (string[2] == 'U')) &&
                    ((string[3] == 'h') || (string[3] == 'H')) &&
                    ((string[4] == 'r') || (string[4] == 'R'));

            if (has_digits && has_uhr && has_colon_before_current && has_hour_before_colon) {
                char minute_string[3] = {0};
                minute_string[0] = string[0];
                minute_string[1] = string[1];

                if (!pv_normalizer_util_string_number_greater_than_int(minute_string, 59)) {
                    pv_status_t status = pv_normalizer_token_list_unroll_token(2, token, token_list);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_normalizer_token_list_unroll_token,
                                pv_status_to_string(status));
                        return status;
                    }

                    pv_normalizer_token_t *uhr_token = token->next;

                    token->tag = PV_NORMALIZER_TAG_DE_TIME_MINUTES;
                    uhr_token->tag = PV_NORMALIZER_TAG_DE_TIME_UHR;
                    token->previous->tag = PV_NORMALIZER_TAG_DE_TIME_COLON;
                    token->previous->previous->tag = PV_NORMALIZER_TAG_DE_TIME_HOURS;

                    return PV_STATUS_SUCCESS;
                }
            }
        }
    }

    // TAG: " 7uhr ", " 12uhr " (tokenized to [9uhr])
    if ((strlen(token->string) >= 4) && pv_normalizer_token_is_space_or_null(token->previous)) {
        const char *string = token->string;

        bool found_time = false;
        if (strlen(token->string) == 4) {
            bool has_digits = isdigit(string[0]);
            bool has_uhr =
                    ((string[1] == 'u') || (string[1] == 'U')) &&
                    ((string[2] == 'h') || (string[2] == 'H')) &&
                    ((string[3] == 'r') || (string[3] == 'R'));
            if (has_digits && has_uhr) {
                pv_status_t status = pv_normalizer_token_list_unroll_token(1, token, token_list);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_token_list_unroll_token,
                            pv_status_to_string(status));
                    return status;
                }
                found_time = true;
            }
        } else if (strlen(token->string) == 5) {
            bool has_digits = isdigit(string[0]) && isdigit(string[1]);
            bool has_uhr =
                    ((string[2] == 'u') || (string[2] == 'U')) &&
                    ((string[3] == 'h') || (string[3] == 'H')) &&
                    ((string[4] == 'r') || (string[4] == 'R'));
            if (has_digits && has_uhr) {
                char hour_string[3] = {0};
                hour_string[0] = string[0];
                hour_string[1] = string[1];

                if (!pv_normalizer_util_string_number_greater_than_int(hour_string, 24)) {
                    pv_status_t status = pv_normalizer_token_list_unroll_token(2, token, token_list);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_normalizer_token_list_unroll_token,
                                pv_status_to_string(status));
                        return status;
                    }
                    found_time = true;
                }
            }
        }

        if (found_time) {
            token->tag = PV_NORMALIZER_TAG_DE_TIME_HOURS;
            pv_normalizer_token_t *uhr_token = token->next;
            uhr_token->tag = PV_NORMALIZER_TAG_DE_TIME_UHR;

            char *uhr_string = "UHR";
            free(uhr_token->string);
            uhr_token->string = calloc(strlen(uhr_string) + 1, sizeof(char));
            if (!uhr_token->string) {
                PV_ERROR_REPORT(
                        &pv_error_msg_alloc,
                        PV_ERROR_ARGS_PUBLIC_EMPTY(),
                        PV_ERROR_ARGS_PRIVATE("uhr_token->string"));
                return PV_STATUS_OUT_OF_MEMORY;
            }
            strcpy(uhr_token->string, uhr_string);

            return PV_STATUS_SUCCESS;
        }
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t merge_comma_number_tokens(
        pv_normalizer_token_t *first,
        pv_normalizer_token_t *last,
        pv_normalizer_token_list_t *token_list) {
    PV_ASSERT(first);
    PV_ASSERT(last);
    PV_ASSERT(token_list);

    pv_normalizer_token_tag_de_t tag = first->tag;
    if (last->tag == PV_NORMALIZER_TAG_DE_ORDINAL_MASCULINE || last->tag == PV_NORMALIZER_TAG_DE_NEGATIVE_ORDINAL_MASCULINE) {
        tag = last->tag;
    } else if (last->tag == PV_NORMALIZER_TAG_DE_ORDINAL_FEMININE || last->tag == PV_NORMALIZER_TAG_DE_NEGATIVE_ORDINAL_FEMININE) {
        tag = last->tag;
    } else if (last->tag == PV_NORMALIZER_TAG_DE_ORDINAL_NEUTER || last->tag == PV_NORMALIZER_TAG_DE_NEGATIVE_ORDINAL_NEUTER) {
        tag = last->tag;
    } else if (last->tag == PV_NORMALIZER_TAG_DE_CURRENCY) {
        if (first->tag == PV_NORMALIZER_TAG_DE_CARDINAL) {
            tag = PV_NORMALIZER_TAG_DE_CURRENCY;
        } else {
            tag = PV_NORMALIZER_TAG_DE_NEGATIVE_CURRENCY;
        }
    }

    int32_t currency_index = first->tag_data_index;
    int32_t length_future_context = last->length_future_context;
    int32_t length_past_context = first->length_past_context;

    int32_t combined_string_length = 0;

    pv_normalizer_token_t *current = first;
    int32_t num_merged = 0;
    while ((current != NULL) && (current != last->next)) {
        num_merged += 1;
        if (strcmp(current->string, ".") != 0) {
            combined_string_length += (int32_t) strlen(current->string);
        }
        current = current->next;
    }

    last->string = realloc(last->string, (combined_string_length + 1) * sizeof(char));
    if (!last->string) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("tail->string"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    char combined_string[combined_string_length + 1];

    combined_string[0] = '\0';

    current = first;
    while ((current != NULL) && (current != last->next)) {
        pv_normalizer_token_t *next = current->next;

        if (strcmp(current->string, ".") != 0) {
            strcat(combined_string, current->string);
        }

        if (current != last) {
            pv_normalizer_token_list_remove_token(token_list, current);
        }

        current = next;
    }

    strcpy(last->string, combined_string);

    last->tag = tag;
    last->tag_data_index = currency_index;

    pv_normalizer_token_t *previous = last->previous;
    while (previous && strcmp(previous->original_string, last->original_string) == 0) {
        previous->length_future_context -= (num_merged - 1);
        previous = previous->previous;
    }
    pv_normalizer_token_t *after = last->next;
    while (after && strcmp(after->original_string, last->original_string) == 0) {
        after->length_past_context -= (num_merged - 1);
        after = after->next;
    }

    last->length_future_context = length_future_context;
    last->length_past_context = length_past_context;

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_tagger_tag_dot_number(pv_normalizer_token_t *token, pv_normalizer_token_list_t *token_list) {
    PV_ASSERT(token);
    PV_ASSERT(token_list);

    bool current_is_cardinal = true;
    pv_normalizer_token_t *current = token;

    pv_normalizer_token_t *previous = pv_normalizer_token_get_nth_token_before(current, 1, true);
    bool is_first_token =
            (previous == NULL) ||
            previous->next_character_is_space ||
            (strcmp(previous->string, "-") == 0) ||
            (strcmp(previous->string, "/") == 0) ||
            (strcmp(previous->string, "(") == 0) ||
            (strcmp(previous->string, APOSTROPHE) == 0);
    while (!is_first_token) {
        current = current->previous;
        char *string = current->string;

        previous = pv_normalizer_token_get_nth_token_before(current, 1, true);
        is_first_token =
                (previous == NULL) ||
                previous->next_character_is_space ||
                (strcmp(previous->string, "-") == 0) ||
                (strcmp(previous->string, "/") == 0) ||
                (strcmp(previous->string, "(") == 0) ||
                (strcmp(previous->string, APOSTROPHE) == 0);

        if (current_is_cardinal) {
            if (strcmp(current->string, ".") == 0) {
                current_is_cardinal = false;
            } else {
                return PV_STATUS_SUCCESS;
            }
        } else {
            if ((current->tag == PV_NORMALIZER_TAG_DE_CARDINAL || current->tag == PV_NORMALIZER_TAG_DE_DIGITS) &&
                ((is_first_token && (strlen(string) <= 3)) ||
                 (!is_first_token && (strlen(string) == 3)))) {
                current_is_cardinal = true;
            } else if (is_first_token &&
                       (current->tag == PV_NORMALIZER_TAG_DE_NEGATIVE_CARDINAL) &&
                       (strlen(string) <= 4)) {
                current_is_cardinal = true;
            } else if (is_first_token &&
                       (current->tag == PV_NORMALIZER_TAG_DE_CURRENCY)) {
                char character[PV_NORMALIZER_MAX_NUM_BYTES_PER_CHARACTER] = {0};
                int32_t num_bytes_character = 0;
                pv_status_t status = pv_normalizer_util_get_next_character(string, 0, character, &num_bytes_character);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_util_get_next_character,
                            pv_status_to_string(status));
                    return status;
                }

                if ((strlen(string) - num_bytes_character) <= 3) {
                    current_is_cardinal = true;
                }
            } else if (is_first_token &&
                       (current->tag == PV_NORMALIZER_TAG_DE_NEGATIVE_CURRENCY)) {
                char character[PV_NORMALIZER_MAX_NUM_BYTES_PER_CHARACTER] = {0};
                int32_t num_bytes_character = 0;

                pv_status_t status = pv_normalizer_util_get_next_character(string, 1, character, &num_bytes_character);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_util_get_next_character,
                            pv_status_to_string(status));
                    return status;
                }

                if ((strlen(string) - num_bytes_character - 1) <= 3) {
                    current_is_cardinal = true;
                }
            } else {
                return PV_STATUS_SUCCESS;
            }
        }
    }

    if (current != token) {
        pv_status_t status = merge_comma_number_tokens(current, token, token_list);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    merge_comma_number_tokens,
                    pv_status_to_string(status));
            return status;
        }
    }

    return PV_STATUS_SUCCESS;
}

static pv_status_t pv_normalizer_tagger_tag_fraction(
        const pv_normalizer_tagger_de_t *object,
        pv_normalizer_token_t *token,
        pv_normalizer_token_list_t *token_list) {
    PV_ASSERT(object);
    PV_ASSERT(token);
    PV_ASSERT(token_list);

    if (PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[PV_NORMALIZER_TAG_DE_FRACTION_SLASH]) {
        const char *string = token->string;

        bool is_fraction = true;
        int32_t num_slashes = 0;
        int32_t num_cardinals = 0;
        int32_t slash_index = 0;

        int32_t i = 0;
        while ((string[i] != '\0') && is_fraction) {
            if (string[i] == '/') {
                slash_index = i;
                num_slashes++;
                i++;
            } else {
                num_cardinals++;
                if (string[i] == '-') {
                    i++;
                }
                while ((string[i] != '\0') && (string[i] != '/')) {
                    if (!isdigit(string[i])) {
                        is_fraction = false;
                        break;
                    }
                    i++;
                }
            }
        }

        if (is_fraction && (num_slashes == 1) && (num_cardinals == 2)) {
            pv_status_t status = pv_normalizer_token_list_unroll_token(slash_index, token, token_list);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_token_list_unroll_token,
                        pv_status_to_string(status));
                return status;
            }

            status = pv_normalizer_tagger_de_tag_token(object, token, token_list);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_de_tag_token,
                        pv_status_to_string(status));
                return status;
            }

            status = pv_normalizer_token_list_unroll_token(1, token->next, token_list);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_token_list_unroll_token,
                        pv_status_to_string(status));
                return status;
            }

            status = pv_normalizer_tagger_de_tag_token(object, token->next->next, token_list);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_de_tag_token,
                        pv_status_to_string(status));
                return status;
            }

            bool before_slash_is_number =
                    ((token->tag == PV_NORMALIZER_TAG_DE_CARDINAL) ||
                     (token->tag == PV_NORMALIZER_TAG_DE_NEGATIVE_CARDINAL) ||
                     (token->tag == PV_NORMALIZER_TAG_DE_DECIMAL_DIGITS));
            bool after_slash_is_number =
                    ((token->next->next->tag == PV_NORMALIZER_TAG_DE_CARDINAL) ||
                     (token->next->next->tag == PV_NORMALIZER_TAG_DE_NEGATIVE_CARDINAL) ||
                     (token->next->next->tag == PV_NORMALIZER_TAG_DE_DECIMAL_DIGITS));

            if (before_slash_is_number && after_slash_is_number) {
                token->next->tag = PV_NORMALIZER_TAG_DE_FRACTION_SLASH;
            }
        }
    }

    return PV_STATUS_SUCCESS;
}

bool pv_normalizer_tagger_is_digit_separator(char string) {
    PV_ASSERT(string);

    return (string == ',') || (string == '.') || (string == '/' || string == '-');
}

pv_status_t pv_normalizer_tagger_tag_digits_sequence(
        const pv_normalizer_tokenizer_t *tokenizer,
        pv_normalizer_token_t *token,
        pv_normalizer_token_list_t *token_list) {
    PV_ASSERT(tokenizer);
    PV_ASSERT(token);
    PV_ASSERT(token_list);

    const char *string = token->string;
    int32_t length = (int32_t) strlen(string);

    if (PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[PV_NORMALIZER_TAG_DE_DIGITS_WITH_PARENTHESES]) {

        // TAG: `(XXX)` e.g. in phone number `(XXX) XXX-XXXX` tokenized to [(XXX), XXX-XXXX]
        if ((length == 5) && (string[0] == '(') && (string[4] == ')')) {
            if (isdigit(string[1]) && isdigit(string[2]) && isdigit(string[3])) {
                token->tag = PV_NORMALIZER_TAG_DE_DIGITS_WITH_PARENTHESES;
            }
        }
    }

    if (PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[PV_NORMALIZER_TAG_DE_DIGITS]) {

        // TAG: `+1`
        pv_normalizer_token_t *previous_no_skip_space = pv_normalizer_token_get_nth_token_before(token, 1, false);
        if ((previous_no_skip_space != NULL) && (strcmp(previous_no_skip_space->string, "+") == 0)) {
            if (pv_normalizer_util_only_contains_digits(string)) {
                token->tag = PV_NORMALIZER_TAG_DE_DIGITS;
                return PV_STATUS_SUCCESS;
            }
        }

        // TAG: `123-456-7890` (tokenized to [123, -, 456, -, 7890])
        bool has_double_dash = false;
        bool only_digits_and_dashes = true;
        int32_t num_separators = 0;
        int32_t num_digits = 0;
        char previous_char = string[0];
        char previous_separator = ' ';

        int32_t i = 0;
        while (i < length) {
            if (isdigit(string[i])) {
                num_digits++;
            } else if (pv_normalizer_tagger_is_digit_separator(string[i])) {
                if (pv_normalizer_tagger_is_digit_separator(previous_char)) {
                    has_double_dash = true;
                    break;
                }
                if ((num_separators > 0) && (string[i] != previous_separator)) {
                    only_digits_and_dashes = false;
                    break;
                }
                previous_separator = string[i];
                num_separators++;
            } else {
                only_digits_and_dashes = false;
                break;
            }
            previous_char = string[i];
            i++;
        }

        // TAG: digits interleaved with dashes (`778-389-1289`)
        bool is_numbers_interleaved_with_dashes = (num_digits >= 4) && (num_separators >= 2) && isdigit(string[0]);
        bool is_digits_with_dashes =
                is_numbers_interleaved_with_dashes &&
                !has_double_dash &&
                only_digits_and_dashes;
        if (is_digits_with_dashes) {
            pv_normalizer_token_list_t *token_list_digits = NULL;
            pv_status_t status = pv_normalizer_tokenizer_tokenize(
                    tokenizer,
                    string,
                    previous_separator,
                    true,
                    false,
                    false,
                    false,
                    &token_list_digits);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tokenizer_tokenize,
                        pv_status_to_string(status));
                return status;
            }

            if (!token_list_digits || (token_list_digits->size == 0)) {
                free(token_list_digits);
                return PV_STATUS_SUCCESS;
            }

            pv_normalizer_token_t *head = token_list_digits->head;
            token->string[strlen(head->string)] = '\0';
            token->tag = PV_NORMALIZER_TAG_DE_DIGITS;
            token->length_future_context = token_list_digits->size - 1;
            token->length_past_context = 0;


            pv_normalizer_token_t *previous = token;
            pv_normalizer_token_t *current = head->next;
            int32_t length_past_context = 1;
            while (current != NULL) {
                pv_normalizer_token_t *next = current->next;

                if ((strlen(current->string) == 1) && pv_normalizer_tagger_is_digit_separator(current->string[0])) {
                    current->tag = PV_NORMALIZER_TAG_DE_DIGITS_SEPARATOR;
                } else {
                    current->tag = PV_NORMALIZER_TAG_DE_DIGITS;
                }

                free(current->original_string);
                current->original_string = calloc(strlen(token->original_string) + 1, sizeof(char));
                if (!current->original_string) {
                    PV_ERROR_REPORT(
                            &pv_error_msg_alloc,
                            PV_ERROR_ARGS_PUBLIC_EMPTY(),
                            PV_ERROR_ARGS_PRIVATE("current->original_string"));
                    free(token_list_digits);
                    return PV_STATUS_OUT_OF_MEMORY;
                }
                strcpy(current->original_string, token->original_string);

                current->length_future_context = token_list_digits->size - length_past_context - 1;
                current->length_past_context = length_past_context;
                length_past_context += 1;

                pv_normalizer_token_list_insert_token(token_list, previous, current);

                previous = current;
                current = next;
            }
            pv_normalizer_token_delete(token_list_digits->head);
            free(token_list_digits);
            return PV_STATUS_SUCCESS;
        }

        // TAG: Tokens already split e.g. [(778), 123, -, 4567]
        pv_normalizer_token_t *previous_skip_space = pv_normalizer_token_get_nth_token_before(token, 1, true);
        bool is_second_number_in_digits_sequence =
                (previous_skip_space != NULL) &&
                (previous_skip_space->tag == PV_NORMALIZER_TAG_DE_DIGITS_WITH_PARENTHESES);
        pv_normalizer_token_t *two_previous = pv_normalizer_token_get_nth_token_before(token, 2, false);
        pv_normalizer_token_t *four_previous = pv_normalizer_token_get_nth_token_before(token, 4, false);
        bool is_third_number_in_digits_sequence =
                (four_previous != NULL) &&
                (four_previous->tag == PV_NORMALIZER_TAG_DE_DIGITS_WITH_PARENTHESES) &&
                (two_previous->tag == PV_NORMALIZER_TAG_DE_DIGITS);

        bool is_part_of_digits_sequence =
                (is_second_number_in_digits_sequence || is_third_number_in_digits_sequence) &&
                !has_double_dash &&
                only_digits_and_dashes;
        if (is_part_of_digits_sequence) {
            pv_normalizer_token_t *current = token;
            while (current != NULL) {
                if (pv_normalizer_util_only_contains_digits(current->string)) {
                    current->tag = PV_NORMALIZER_TAG_DE_DIGITS;
                } else if (strcmp(current->string, "-") == 0) {
                    current->tag = PV_NORMALIZER_TAG_DE_DIGITS_SEPARATOR;
                } else {
                    break;
                }
                current = current->next;
            }
            return PV_STATUS_SUCCESS;
        }

        // TAG: `123.456.7890`
        pv_normalizer_token_t *three_previous = pv_normalizer_token_get_nth_token_before(token, 3, false);
        bool is_digits_with_dots =
                (four_previous != NULL) &&
                pv_normalizer_util_only_contains_digits(four_previous->string) &&
                (strcmp(three_previous->string, ".") == 0) &&
                pv_normalizer_util_only_contains_digits(two_previous->string) &&
                (strcmp(previous_no_skip_space->string, ".") == 0) &&
                pv_normalizer_util_only_contains_digits(token->string);
        if (is_digits_with_dots) {
            pv_normalizer_token_t *current = token;
            while (current != NULL) {
                if ((current->tag == PV_NORMALIZER_TAG_DE_CARDINAL) ||
                    pv_normalizer_util_only_contains_digits(current->string)) {
                    current->tag = PV_NORMALIZER_TAG_DE_DIGITS;
                } else if (strcmp(current->string, ".") == 0) {
                    current->tag = PV_NORMALIZER_TAG_DE_DIGITS_SEPARATOR;
                } else {
                    break;
                }
                current = current->previous;
            }
            return PV_STATUS_SUCCESS;
        }
    }

    return PV_STATUS_SUCCESS;
}

bool pv_normalizer_tagger_is_date_separator(const char *string) {
    PV_ASSERT(string);

    return (strcmp(string, "-") == 0) ||
           (strcmp(string, "/") == 0) ||
           (strcmp(string, ".") == 0);
}

bool pv_normalizer_tagger_is_date_separator_character(char character) {
    PV_ASSERT(character);

    return character == '-' || character == '/' || character == '.';
}

int32_t pv_normalizer_tagger_get_month_index(const char *string) {
    PV_ASSERT(string);

    for (int32_t i = 0; i < PV_NORMALIZER_NUM_MONTH_NAMES_DE; i++) {
        if (strcmp(string, PV_NORMALIZER_MONTH_NAMES_DE[i]) == 0) {
            return i;
        }
    }

    return -1;
}

pv_status_t pv_normalizer_tagger_tag_date(
        const pv_normalizer_tokenizer_t *tokenizer,
        pv_normalizer_token_t *token,
        pv_normalizer_token_list_t *token_list) {
    PV_ASSERT(tokenizer);
    PV_ASSERT(token);
    PV_ASSERT(token_list);

    if (PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[PV_NORMALIZER_TAG_DE_DATE_MONTH]) {
        const char *string = token->string;
        int32_t length = (int32_t) strlen(string);

        // TAG month: `Januar`, `Jan`, etc.
        int32_t index_month_name = pv_normalizer_tagger_get_month_index(string);
        if (index_month_name >= 0) {
            token->tag = PV_NORMALIZER_TAG_DE_DATE_MONTH;
            token->tag_data_index = index_month_name;

            pv_normalizer_token_t *previous_skip_space = pv_normalizer_token_get_nth_token_before(token, 1, true);
            bool is_previous_day =
                    (previous_skip_space != NULL) &&
                    previous_skip_space->tag == PV_NORMALIZER_TAG_DE_CARDINAL &&
                    !pv_normalizer_util_string_number_greater_than_int(previous_skip_space->string, 31) &&
                    pv_normalizer_util_string_number_greater_than_int(previous_skip_space->string, 0);
            if (is_previous_day) {
                previous_skip_space->tag = PV_NORMALIZER_TAG_DE_DATE_DAY;
            }
            return PV_STATUS_SUCCESS;
        }


        // SPLIT token `12/25/1997` or `1997-05-28` or `03-Jun-1829`
        char previous_date_separator_char = '\0';
        int32_t num_separators = 0;
        bool is_valid_date = true;
        int32_t num_digits[3] = {0, 0, 0};
        int32_t i = 0;
        while (i < length) {
            if (isdigit(string[i])) {
                num_digits[num_separators]++;
            } else if (pv_normalizer_tagger_is_date_separator_character(string[i])) {
                if (num_separators > 0) {
                    if (string[i] != previous_date_separator_char) {
                        is_valid_date = false;
                        break;
                    }
                }
                num_separators++;
                if (num_separators > 2) {
                    is_valid_date = false;
                    break;
                }
                previous_date_separator_char = string[i];
            } else {
                // allow for case `03-Jun-1829`
                if (num_separators != 1) {
                    is_valid_date = false;
                    break;
                }
            }
            i++;
        }
        is_valid_date = is_valid_date && (previous_date_separator_char != '\0');

        bool is_valid_num_digits =
                (((num_digits[0] == 2) || (num_digits[0] == 1)) &&
                 ((num_digits[1] == 2) || (num_digits[1] == 1)) &&
                 ((num_digits[2] == 4) || (num_digits[2] == 2))) ||
                (((num_digits[0] == 4) || (num_digits[0] == 2)) &&
                 ((num_digits[1] == 2) || (num_digits[1] == 1)) &&
                 ((num_digits[2] == 2) || (num_digits[2] == 1)));

        bool is_valid_num_digits_with_month_string = false;
        int32_t month_string_length = length - num_digits[0] - num_digits[2] - num_separators;
        if (month_string_length >= 0) {
            char *month_string = calloc(month_string_length + 1, sizeof(char));
            if (!month_string) {
                PV_ERROR_REPORT(
                        &pv_error_msg_alloc,
                        PV_ERROR_ARGS_PUBLIC_EMPTY(),
                        PV_ERROR_ARGS_PRIVATE("month_string"));
                return PV_STATUS_OUT_OF_MEMORY;
            }
            memcpy(month_string, string + num_digits[0] + 1, month_string_length * sizeof(char));
            month_string[month_string_length] = '\0';
            index_month_name = pv_normalizer_tagger_get_month_index(month_string);
            free(month_string);
            is_valid_num_digits_with_month_string =
                    ((num_digits[0] == 2) || (num_digits[0] == 1)) &&
                    ((num_digits[2] == 4) || (num_digits[2] == 2)) &&
                    (index_month_name >= 0);
        }

        if (is_valid_date && (num_separators == 2) && (is_valid_num_digits || is_valid_num_digits_with_month_string)) {
            pv_normalizer_token_list_t *token_list_date = NULL;
            pv_status_t status = pv_normalizer_tokenizer_tokenize(
                    tokenizer,
                    string,
                    previous_date_separator_char,
                    true,
                    false,
                    false,
                    false,
                    &token_list_date);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tokenizer_tokenize,
                        pv_status_to_string(status));
                return status;
            }

            if (!token_list_date || (token_list_date->size == 0)) {
                free(token_list_date);
                return PV_STATUS_SUCCESS;
            }

            pv_normalizer_token_t *head = token_list_date->head;
            token->string[strlen(head->string)] = '\0';
            token->tag = PV_NORMALIZER_TAG_DE_CARDINAL;

            pv_normalizer_token_t *previous = token;
            pv_normalizer_token_t *current = token_list_date->head->next;
            while (current != NULL) {
                pv_normalizer_token_t *next = current->next;

                free(current->original_string);
                current->original_string = calloc(strlen(token->original_string) + 1, sizeof(char));
                if (!current->original_string) {
                    PV_ERROR_REPORT(
                            &pv_error_msg_alloc,
                            PV_ERROR_ARGS_PUBLIC_EMPTY(),
                            PV_ERROR_ARGS_PRIVATE("current->original_string"));
                    free(token_list_date);
                    return PV_STATUS_OUT_OF_MEMORY;
                }
                strcpy(current->original_string, token->original_string);

                pv_normalizer_token_list_insert_token(token_list, previous, current);

                previous = current;
                current = next;
            }
            pv_normalizer_token_delete(head);
            free(token_list_date);

            return PV_STATUS_SUCCESS;
        }

        // TAG: Tokenized [`12`, `-`, `25`, `-`, `1992`] or [`28`, `-`, `Jun`, `-`, `1992`]
        pv_normalizer_token_t *previous = pv_normalizer_token_get_nth_token_before(token, 1, false);
        pv_normalizer_token_t *two_previous = pv_normalizer_token_get_nth_token_before(token, 2, false);
        pv_normalizer_token_t *three_previous = pv_normalizer_token_get_nth_token_before(token, 3, false);
        pv_normalizer_token_t *four_previous = pv_normalizer_token_get_nth_token_before(token, 4, false);

        int32_t two_previous_month_index = -1;
        if (two_previous != NULL) {
            two_previous_month_index = pv_normalizer_tagger_get_month_index(two_previous->string);
        }

        bool current_is_year =
                pv_normalizer_util_only_contains_digits(token->string) &&
                (length == 4) &&
                ((strlen(token->string) > 0) && (token->string[0] != '0')) &&
                (four_previous != NULL);
        if (current_is_year) {
            // Checking if its day-month-year format
            bool previous_is_date_separator = pv_normalizer_tagger_is_date_separator(previous->string);
            bool two_previous_is_month =
                    pv_normalizer_util_only_contains_digits(two_previous->string) &&
                    !pv_normalizer_util_string_number_greater_than_int(two_previous->string, 12);
            bool three_previous_is_date_separator =
                    pv_normalizer_tagger_is_date_separator(three_previous->string) &&
                    (strcmp(three_previous->string, previous->string) == 0);
            bool four_previous_is_day =
                    pv_normalizer_util_only_contains_digits(four_previous->string) &&
                    !pv_normalizer_util_string_number_greater_than_int(four_previous->string, 31);

            // Fallback to month-day-year format
            bool two_previous_is_day =
                    pv_normalizer_util_only_contains_digits(two_previous->string) &&
                    pv_normalizer_util_string_number_greater_than_int(two_previous->string, 12) &&
                    !pv_normalizer_util_string_number_greater_than_int(two_previous->string, 31);
            bool four_previous_is_month = false;
            if (two_previous_is_day) {
                four_previous_is_month =
                        pv_normalizer_util_only_contains_digits(four_previous->string) &&
                        !pv_normalizer_util_string_number_greater_than_int(four_previous->string, 12);
            }

            // take care of 3-jun-2024
            if ((strlen(two_previous->string) > 0) && !isdigit(two_previous->string[0])) {
                four_previous_is_day =
                        pv_normalizer_util_only_contains_digits(four_previous->string) &&
                        !pv_normalizer_util_string_number_greater_than_int(four_previous->string, 31);
                two_previous_is_month = (two_previous_month_index >= 0);
            }

            bool is_above_zero =
                    pv_normalizer_util_string_number_greater_than_int(two_previous->string, 0) &&
                    pv_normalizer_util_string_number_greater_than_int(four_previous->string, 0);

            bool is_date =
                    current_is_year &&
                    is_above_zero &&
                    previous_is_date_separator &&
                    three_previous_is_date_separator &&
                    ((two_previous_is_month && four_previous_is_day) ||
                     (two_previous_is_day && four_previous_is_month));


            if (is_date) {
                token->tag = PV_NORMALIZER_TAG_DE_DATE_YEAR;
                previous->tag = PV_NORMALIZER_TAG_DE_DATE_SEPARATOR;
                three_previous->tag = PV_NORMALIZER_TAG_DE_DATE_SEPARATOR;

                if (two_previous_is_month && four_previous_is_day) {
                    two_previous->tag = PV_NORMALIZER_TAG_DE_DATE_MONTH;
                    four_previous->tag = PV_NORMALIZER_TAG_DE_DATE_DAY;
                } else {
                    two_previous->tag = PV_NORMALIZER_TAG_DE_DATE_DAY;
                    four_previous->tag = PV_NORMALIZER_TAG_DE_DATE_MONTH;
                }

                return PV_STATUS_SUCCESS;
            }
        }

        // TAG: `1992-10-18` already tokenized to [`1992`, `-`, `10`, `-`, `18`]
        bool four_previous_is_year =
                four_previous != NULL &&
                pv_normalizer_util_only_contains_digits(four_previous->string) &&
                (strlen(four_previous->string) == 4) &&
                ((strlen(four_previous->string) > 0) && (four_previous->string[0] != '0'));
        if (four_previous_is_year) {
            bool previous_is_date_separator = pv_normalizer_tagger_is_date_separator(previous->string);
            bool three_previous_is_date_separator =
                    pv_normalizer_tagger_is_date_separator(three_previous->string) &&
                    (strcmp(three_previous->string, previous->string) == 0);
            bool two_previous_is_month =
                    pv_normalizer_util_only_contains_digits(two_previous->string) &&
                    !pv_normalizer_util_string_number_greater_than_int(two_previous->string, 12) &&
                    pv_normalizer_util_string_number_greater_than_int(two_previous->string, 0);
            bool current_is_day =
                    pv_normalizer_util_only_contains_digits(token->string) &&
                    !pv_normalizer_util_string_number_greater_than_int(token->string, 31) &&
                    pv_normalizer_util_string_number_greater_than_int(token->string, 0);

            bool is_date =
                    three_previous_is_date_separator &&
                    two_previous_is_month &&
                    previous_is_date_separator &&
                    current_is_day;
            if (is_date) {
                four_previous->tag = PV_NORMALIZER_TAG_DE_DATE_YEAR;
                three_previous->tag = PV_NORMALIZER_TAG_DE_DATE_SEPARATOR;
                two_previous->tag = PV_NORMALIZER_TAG_DE_DATE_MONTH;
                previous->tag = PV_NORMALIZER_TAG_DE_DATE_SEPARATOR;
                token->tag = PV_NORMALIZER_TAG_DE_DATE_DAY;

                return PV_STATUS_SUCCESS;
            }
        }

        // TAG: `03-Jun-2024` already tokenized to [`03`, `-`, `Jun`, `-`, `1992`]
        current_is_year =
                pv_normalizer_util_only_contains_digits(token->string) &&
                (length == 4) &&
                (four_previous != NULL);
        if (current_is_year && (four_previous != NULL)) {
            index_month_name = pv_normalizer_tagger_get_month_index(four_previous->string);
            bool is_date =
                    (index_month_name >= 0) &&
                    pv_normalizer_tagger_is_date_separator(previous->string) &&
                    pv_normalizer_tagger_is_date_separator(three_previous->string) &&
                    pv_normalizer_util_only_contains_digits(four_previous->string) &&
                    !pv_normalizer_util_string_number_greater_than_int(four_previous->string, 31) &&
                    pv_normalizer_util_string_number_greater_than_int(four_previous->string, 0);
            if (is_date) {
                four_previous->tag = PV_NORMALIZER_TAG_DE_DATE_DAY;
                three_previous->tag = PV_NORMALIZER_TAG_DE_DATE_SEPARATOR;
                two_previous->tag = PV_NORMALIZER_TAG_DE_DATE_MONTH;
                two_previous->tag_data_index = index_month_name;
                previous->tag = PV_NORMALIZER_TAG_DE_DATE_SEPARATOR;
                token->tag = PV_NORMALIZER_TAG_DE_DATE_YEAR;
            }
        }

        // TAG: 3 Jun[i] 2023
        pv_normalizer_token_t *previous_skip_space = pv_normalizer_token_get_nth_token_before(token, 1, true);
        current_is_year =
                pv_normalizer_util_only_contains_digits(string) &&
                (strlen(string) == 4) &&
                previous_skip_space != NULL &&
                previous_skip_space->tag == PV_NORMALIZER_TAG_DE_DATE_MONTH;
        if (current_is_year) {
            token->tag = PV_NORMALIZER_TAG_DE_DATE_YEAR;
        }
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_tagger_tag_url(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[PV_NORMALIZER_TAG_DE_DOT]) {
        const char *string = token->string;

        bool previous_is_non_space_character =
                (token->previous != NULL) &&
                (strcmp(token->previous->string, " ") != 0) &&
                !token->previous->next_character_is_space;
        bool previous_is_not_dot =
                (token->previous != NULL) &&
                (strcmp(token->previous->string, ".") != 0);
        bool next_is_non_space_character =
                (token->next != NULL) &&
                (strcmp(token->next->string, " ") != 0) &&
                !token->next_character_is_space &&
                (strcmp(token->next->string, "\n") != 0) &&
                (strcmp(token->next->string, "\"") != 0);
        bool next_is_not_dot =
                (token->next != NULL) &&
                (strcmp(token->next->string, ".") != 0);
        bool next_is_not_single_quote =
                (token->next != NULL) &&
                (strcmp(token->next->string, APOSTROPHE) != 0);

        if ((strcmp(string, ".") == 0) &&
            previous_is_non_space_character &&
            next_is_non_space_character &&
            previous_is_not_dot &&
            next_is_not_dot && next_is_not_single_quote) {
            token->tag = PV_NORMALIZER_TAG_DE_DOT;
        }
    }

    if (PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[PV_NORMALIZER_TAG_DE_COLON]) {
        const char *string = token->string;

        bool previous_is_non_space_character =
                (token->previous != NULL) &&
                (strcmp(token->previous->string, " ") != 0) &&
                !token->previous->next_character_is_space;
        bool next_is_non_space_character =
                (token->next != NULL) &&
                (strcmp(token->next->string, " ") != 0) &&
                !token->next_character_is_space &&
                (strcmp(token->next->string, "\n") != 0);


        if ((strcmp(string, ":") == 0) &&
            previous_is_non_space_character &&
            next_is_non_space_character) {
            token->tag = PV_NORMALIZER_TAG_DE_COLON;
        }
    }

    if (PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[PV_NORMALIZER_TAG_DE_ACRONYM]) {
        char *upper_string = NULL;
        pv_status_t status = pv_normalizer_util_upper(token->string, &upper_string);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_util_upper,
                    pv_status_to_string(status));
            return status;
        }

        for (int32_t i = 0; i < PV_NORMALIZER_NUM_ACRONYMS_DE; i++) {
            if (strcmp(upper_string, PV_NORMALIZER_ACRONYMS_DE[i]) == 0) {
                token->tag = PV_NORMALIZER_TAG_DE_ACRONYM;
                break;
            }
        }

        free(upper_string);
    }

    if (PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[PV_NORMALIZER_TAG_DE_TOP_LEVEL_DOMAIN]) {
        if ((token->previous != NULL) &&
            !token->previous->next_character_is_space &&
            (token->previous->tag == PV_NORMALIZER_TAG_DE_DOT || strcmp(token->previous->string, ".") == 0)) {
            char *upper_string = NULL;
            pv_status_t status = pv_normalizer_util_upper(token->string, &upper_string);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_util_upper,
                        pv_status_to_string(status));
                return status;
            }

            for (int32_t i = 0; i < PV_NORMALIZER_NUM_TOP_LEVEL_DOMAINS_DE; i++) {
                if (strcmp(upper_string, PV_NORMALIZER_TOP_LEVEL_DOMAINS_DE[i]) == 0) {
                    token->tag = PV_NORMALIZER_TAG_DE_TOP_LEVEL_DOMAIN;
                    token->tag_data_index = i;
                    if (token->previous->tag != PV_NORMALIZER_TAG_DE_DOT) {
                        token->previous->tag = PV_NORMALIZER_TAG_DE_DOT;
                    }
                    break;
                }
            }

            free(upper_string);
        }
    }

    return PV_STATUS_SUCCESS;
}

static pv_status_t pv_normalizer_tagger_tag_currency(
        const pv_normalizer_tagger_de_t *object,
        pv_normalizer_token_t *token,
        pv_normalizer_token_list_t *token_list) {
    PV_ASSERT(object);
    PV_ASSERT(token);
    PV_ASSERT(token_list);

    if (PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[PV_NORMALIZER_TAG_DE_CURRENCY]) {
        const char *string = token->string;
        int32_t length = (int32_t) strlen(string);

        int32_t currency_index = -1;
        int32_t num_bytes_currency = 1;
        bool starts_with_currency = false;
        for (int32_t i = 0; i < PV_NORMALIZER_NUM_CURRENCIES_DE; i++) {
            num_bytes_currency = (int32_t) strlen(PV_NORMALIZER_CURRENCIES_DE[i]);
            if (strncmp(string, PV_NORMALIZER_CURRENCIES_DE[i], num_bytes_currency) == 0) {
                currency_index = i;
                starts_with_currency = true;
                break;
            }
        }

        // `$5` - tokens: ['$5']
        if (starts_with_currency && (num_bytes_currency != length)) {
            bool only_digits = true;
            for (int32_t i = num_bytes_currency; i < length; i++) {
                if (!isdigit(string[i])) {
                    only_digits = false;
                    break;
                }
            }

            if (only_digits) {
                token->tag = PV_NORMALIZER_TAG_DE_CURRENCY;
                token->tag_data_index = currency_index;
                return PV_STATUS_SUCCESS;
            }
        }

        // `5¢` - tokens: ['5¢']
        if (isdigit(string[0])) {
            char character[PV_NORMALIZER_MAX_NUM_BYTES_PER_CHARACTER] = {0};
            int32_t num_bytes_character = 0;

            bool only_digits = true;
            bool ends_with_currency = false;

            int32_t i = 0;
            while (i < length) {
                pv_status_t status = pv_normalizer_util_get_next_character(string, i, character, &num_bytes_character);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_util_get_next_character,
                            pv_status_to_string(status));
                    return status;
                }

                if (!isdigit(string[i])) {
                    for (int32_t j = 0; j < PV_NORMALIZER_NUM_CURRENCIES_DE; j++) {
                        num_bytes_currency = (int32_t) strlen(PV_NORMALIZER_CURRENCIES_DE[j]);
                        if (strncmp(string + i, PV_NORMALIZER_CURRENCIES_DE[j], num_bytes_currency) == 0) {
                            if ((i + num_bytes_currency) == length) {
                                currency_index = j;
                                ends_with_currency = true;
                            } else {
                                only_digits = false;
                            }
                            break;
                        }
                    }
                }

                i += num_bytes_character;
            }

            if (only_digits && ends_with_currency) {
                int32_t length_digits = i - num_bytes_currency;

                bool invalid_digits =
                        (token->previous != NULL) &&
                        (strcmp(token->previous->string, ",") == 0) &&
                        (length_digits != 2);

                if (invalid_digits) {
                    return PV_STATUS_SUCCESS;
                }

                token->tag = PV_NORMALIZER_TAG_DE_CURRENCY;
                token->tag_data_index = currency_index;

                bool is_before_hyphen = false;
                pv_status_t status = pv_normalizer_util_check_token_is_before_character(token, HYPHEN, &is_before_hyphen);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_util_check_token_is_before_character,
                            pv_status_to_string(status));
                    return status;
                }

                bool is_before_decimal =
                        (!token->next_character_is_space &&
                         (token->next != NULL) &&
                         (strcmp(token->next->string, ",") == 0) &&
                         !token->next->next_character_is_space);

                bool is_before_punctuation = false;
                if (!token->next_character_is_space && (token->next != NULL)) {
                    bool next_is_punctuation = false;
                    pv_status_t status = pv_normalizer_util_is_punctuation(object->language, token->next->string, &next_is_punctuation);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_normalizer_util_is_punctuation,
                                pv_status_to_string(status));
                        return status;
                    }
                    is_before_punctuation = next_is_punctuation && (token->next->next_character_is_space || token->next->next == NULL);
                }
                bool is_before_apostrophe = token->next != NULL && strcmp(token->next->string, APOSTROPHE) == 0;

                // Possible end of comma number e.g. `1.000$`
                if ((token->next_character_is_space ||
                     (token->next == NULL) ||
                     is_before_hyphen ||
                     is_before_decimal ||
                     is_before_punctuation ||
                     is_before_apostrophe) &&
                    ((strlen(string) - num_bytes_character) == 3)) {
                    pv_status_t status = pv_normalizer_tagger_tag_dot_number(token, token_list);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_normalizer_tagger_tag_dot_number,
                                pv_status_to_string(status));
                        return status;
                    }
                    token->tag_data_index = currency_index;

                    return PV_STATUS_SUCCESS;
                }

                // `5,25$` - tokens: ['5', ',', '25$'] and -5,25$ - token: ['-5', ',', '25$']
                if ((token->previous != NULL) && token->previous->previous != NULL) {
                    pv_normalizer_token_t *previous = token->previous;
                    pv_normalizer_token_t *two_previous = token->previous->previous;

                    bool is_currency_decimal =
                            (strcmp(previous->string, ",") == 0) &&
                            (!previous->next_character_is_space) &&
                            ((two_previous->tag == PV_NORMALIZER_TAG_DE_CARDINAL) ||
                             (two_previous->tag == PV_NORMALIZER_TAG_DE_NEGATIVE_CARDINAL)) &&
                            (!two_previous->next_character_is_space);

                    if (is_currency_decimal) {
                        pv_normalizer_token_tag_de_t two_previous_tag = two_previous->tag;

                        int32_t length_previous_string = (int32_t) strlen(previous->string);
                        int32_t length_two_previous_string = (int32_t) strlen(two_previous->string);
                        int32_t merged_length = length + length_previous_string + length_two_previous_string;

                        char *merged_string = calloc(merged_length + 1, sizeof(char));
                        if (!merged_string) {
                            PV_ERROR_REPORT(
                                    &pv_error_msg_alloc,
                                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                                    PV_ERROR_ARGS_PRIVATE("merged_string"));
                            return PV_STATUS_OUT_OF_MEMORY;
                        }
                        strcpy(merged_string, two_previous->string);
                        strcat(merged_string, previous->string);
                        strcat(merged_string, string);

                        pv_normalizer_token_t *merged_token = NULL;
                        pv_status_t status = pv_normalizer_token_list_collapse_tokens(
                                token_list,
                                merged_string,
                                two_previous,
                                token,
                                &merged_token);
                        if (status != PV_STATUS_SUCCESS) {
                            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                    pv_normalizer_token_list_merge_tokens,
                                    pv_status_to_string(status));
                            free(merged_string);
                            return status;
                        }

                        free(merged_token->original_string);
                        merged_token->original_string = calloc(strlen(merged_string) + 1, sizeof(char));
                        if (!merged_token->original_string) {
                            PV_ERROR_REPORT(
                                    &pv_error_msg_alloc,
                                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                                    PV_ERROR_ARGS_PRIVATE("merged_token->original_string"));
                            free(merged_string);
                            return PV_STATUS_OUT_OF_MEMORY;
                        }
                        strcpy(merged_token->original_string, merged_string);

                        free(merged_string);

                        if (two_previous_tag == PV_NORMALIZER_TAG_DE_CARDINAL) {
                            merged_token->tag = PV_NORMALIZER_TAG_DE_CURRENCY;
                        } else {
                            merged_token->tag = PV_NORMALIZER_TAG_DE_NEGATIVE_CURRENCY;
                        }

                        merged_token->tag_data_index = currency_index;
                    }
                }

                return PV_STATUS_SUCCESS;
            }
        }

        bool only_digits = true;
        for (int32_t i = 0; i < length; i++) {
            if (!isdigit(string[i])) {
                only_digits = false;
                break;
            }
        }

        // `$5.25` - tokens: ['$5', ',', '25']
        if (only_digits && (token->previous != NULL) && (token->previous->previous != NULL)) {
            pv_normalizer_token_t *previous = token->previous;
            pv_normalizer_token_t *two_previous = token->previous->previous;

            pv_normalizer_token_tag_de_t two_previous_tag = two_previous->tag;
            int32_t two_previous_currency_index = two_previous->tag_data_index;

            bool is_currency_decimal =
                    (strcmp(previous->string, ",") == 0) &&
                    (!previous->next_character_is_space) &&
                    ((two_previous->tag == PV_NORMALIZER_TAG_DE_CURRENCY) ||
                     (two_previous->tag == PV_NORMALIZER_TAG_DE_NEGATIVE_CURRENCY)) &&
                    (!two_previous->next_character_is_space);

            if (is_currency_decimal) {
                if (strlen(string) != 2) {
                    return PV_STATUS_SUCCESS;
                }

                int32_t length_previous_string = (int32_t) strlen(previous->string);
                int32_t length_two_previous_string = (int32_t) strlen(two_previous->string);
                int32_t merged_length = length + length_previous_string + length_two_previous_string;

                char *merged_string = calloc(merged_length + 1, sizeof(char));
                if (!merged_string) {
                    PV_ERROR_REPORT(
                            &pv_error_msg_alloc,
                            PV_ERROR_ARGS_PUBLIC_EMPTY(),
                            PV_ERROR_ARGS_PRIVATE("merged_string"));
                    return PV_STATUS_OUT_OF_MEMORY;
                }
                strcpy(merged_string, two_previous->string);
                strcat(merged_string, previous->string);
                strcat(merged_string, string);

                pv_normalizer_token_t *merged_token = NULL;
                pv_status_t status = pv_normalizer_token_list_collapse_tokens(
                        token_list,
                        merged_string,
                        two_previous,
                        token,
                        &merged_token);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_token_list_merge_tokens,
                            pv_status_to_string(status));
                    free(merged_string);
                    return status;
                }

                free(merged_token->original_string);
                merged_token->original_string = calloc(strlen(merged_string) + 1, sizeof(char));
                if (!merged_token->original_string) {
                    PV_ERROR_REPORT(
                            &pv_error_msg_alloc,
                            PV_ERROR_ARGS_PUBLIC_EMPTY(),
                            PV_ERROR_ARGS_PRIVATE("merged_token->original_string"));
                    free(merged_string);
                    return PV_STATUS_OUT_OF_MEMORY;
                }
                strcpy(merged_token->original_string, merged_string);

                free(merged_string);

                merged_token->tag = two_previous_tag;
                merged_token->tag_data_index = two_previous_currency_index;
                return PV_STATUS_SUCCESS;
            }
        }

        if (string[0] == '-') {
            bool second_character_is_currency = false;
            for (int32_t i = 0; i < PV_NORMALIZER_NUM_CURRENCIES_DE; i++) {
                num_bytes_currency = (int32_t) strlen(PV_NORMALIZER_CURRENCIES_DE[i]);
                if (strncmp(string + 1, PV_NORMALIZER_CURRENCIES_DE[i], num_bytes_currency) == 0) {
                    currency_index = i;
                    second_character_is_currency = true;
                    break;
                }
            }

            // `-$5` - tokens: ['-$5']
            if (second_character_is_currency) {
                only_digits = true;
                for (int32_t i = num_bytes_currency + 1; i < length; i++) {
                    if (!isdigit(string[i])) {
                        only_digits = false;
                        break;
                    }
                }

                if (only_digits) {
                    token->tag = PV_NORMALIZER_TAG_DE_NEGATIVE_CURRENCY;
                    token->tag_data_index = currency_index;
                    return PV_STATUS_SUCCESS;
                }
            }

            // `-5¢` - tokens: ['-5¢']
            if (isdigit(string[1])) {
                char character[PV_NORMALIZER_MAX_NUM_BYTES_PER_CHARACTER] = {0};
                int32_t num_bytes_character = 0;

                only_digits = true;
                bool ends_with_currency = false;

                int32_t i = 1;
                while (i < length) {
                    pv_status_t status = pv_normalizer_util_get_next_character(
                            string,
                            i,
                            character,
                            &num_bytes_character);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_normalizer_util_get_next_character,
                                pv_status_to_string(status));
                        return status;
                    }

                    if (!isdigit(string[i])) {
                        for (int32_t j = 0; j < PV_NORMALIZER_NUM_CURRENCIES_DE; j++) {
                            num_bytes_currency = (int32_t) strlen(PV_NORMALIZER_CURRENCIES_DE[j]);
                            if (strncmp(string + i, PV_NORMALIZER_CURRENCIES_DE[j], num_bytes_currency) == 0) {
                                if ((i + num_bytes_currency) == length) {
                                    currency_index = j;
                                    ends_with_currency = true;
                                } else {
                                    only_digits = false;
                                }
                                break;
                            }
                        }
                    }

                    i += num_bytes_character;
                }

                if (only_digits && ends_with_currency) {
                    token->tag = PV_NORMALIZER_TAG_DE_NEGATIVE_CURRENCY;
                    token->tag_data_index = currency_index;
                    return PV_STATUS_SUCCESS;
                }
            }
        }

        bool is_currency = false;
        for (int32_t i = 0; i < PV_NORMALIZER_NUM_CURRENCIES_DE; i++) {
            if (strcmp(string, PV_NORMALIZER_CURRENCIES_DE[i]) == 0) {
                currency_index = i;
                is_currency = true;
                break;
            }
        }

        pv_normalizer_token_t *previous = pv_normalizer_token_get_nth_token_before(token, 1, true);
        bool previous_is_cardinal =
                (previous != NULL) &&
                ((previous->tag == PV_NORMALIZER_TAG_DE_CARDINAL) ||
                 (previous->tag == PV_NORMALIZER_TAG_DE_NEGATIVE_CARDINAL));
        bool previous_is_decimal =
                (previous != NULL) &&
                (previous->tag == PV_NORMALIZER_TAG_DE_DECIMAL_DIGITS);

        // `1 $` - tokens: ['1', ' ', '$']
        if (is_currency && (previous_is_cardinal || previous_is_decimal)) {
            token->tag = PV_NORMALIZER_TAG_DE_CURRENCY_SYMBOL;
            token->tag_data_index = currency_index;
            return PV_STATUS_SUCCESS;
        }

        if (starts_with_currency && num_bytes_currency == length) {
            token->tag = PV_NORMALIZER_TAG_DE_CURRENCY_SYMBOL;
            token->tag_data_index = currency_index;
            return PV_STATUS_SUCCESS;
        }
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_tagger_tag_abbreviation(
        pv_normalizer_token_t *token,
        pv_normalizer_token_list_t *token_list) {
    PV_ASSERT(token);
    PV_ASSERT(token_list);

    if (PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[PV_NORMALIZER_TAG_DE_ABBREVIATION]) {
        bool is_dot = (strcmp(token->string, ".") == 0);
        if (is_dot) {
            pv_normalizer_token_t *start_token = pv_normalizer_token_get_token_after_previous_space(token);
            while (start_token &&
                   (start_token->tag == PV_NORMALIZER_TAG_DE_SPECIAL_CHARACTER ||
                    start_token->tag == PV_NORMALIZER_TAG_DE_SINGLE_QUOTE) &&
                   start_token->next && start_token->next != token) {
                start_token = start_token->next;
            }

            char *potential_abbreviation = NULL;
            pv_status_t status = pv_normalizer_token_concatenate_token_strings(
                    start_token,
                    token->next,
                    &potential_abbreviation);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_token_concatenate_token_strings,
                        pv_status_to_string(status));
                return status;
            }

            status = pv_normalizer_util_upper_inplace(potential_abbreviation);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_util_upper_inplace,
                        pv_status_to_string(status));
                free(potential_abbreviation);
                return status;
            }

            for (int32_t i = 0; i < PV_NORMALIZER_NUM_ABBREVIATIONS_DE; i++) {
                if (strcmp(potential_abbreviation, PV_NORMALIZER_ABBREVIATIONS_DE[i]) == 0) {
                    token->tag = PV_NORMALIZER_TAG_DE_ABBREVIATION;
                    token->tag_data_index = i;
                    break;
                }
            }

            // TAG: abbreviations with a space (example: "z. B.")
            if (token->tag != PV_NORMALIZER_TAG_DE_ABBREVIATION) {
                pv_normalizer_token_t *previous = pv_normalizer_token_get_nth_token_before(token, 2, true);
                if (previous != NULL && (strcmp(previous->string, ".") == 0)) {
                    start_token = pv_normalizer_token_get_token_after_previous_space(previous);
                    while (start_token &&
                           (start_token->tag == PV_NORMALIZER_TAG_DE_SPECIAL_CHARACTER ||
                            start_token->tag == PV_NORMALIZER_TAG_DE_SINGLE_QUOTE) &&
                           start_token->next && start_token->next != previous) {
                        start_token = start_token->next;
                    }

                    free(potential_abbreviation);
                    potential_abbreviation = NULL;
                    pv_status_t status = pv_normalizer_token_concatenate_token_strings(
                            start_token,
                            token->next,
                            &potential_abbreviation);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_normalizer_token_concatenate_token_strings,
                                pv_status_to_string(status));
                        return status;
                    }

                    status = pv_normalizer_util_upper_inplace(potential_abbreviation);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_normalizer_util_upper_inplace,
                                pv_status_to_string(status));
                        free(potential_abbreviation);
                        return status;
                    }

                    for (int32_t i = 0; i < PV_NORMALIZER_NUM_ABBREVIATIONS_DE; i++) {
                        if (strcmp(potential_abbreviation, PV_NORMALIZER_ABBREVIATIONS_DE[i]) == 0) {
                            token->tag = PV_NORMALIZER_TAG_DE_ABBREVIATION;
                            token->tag_data_index = i;
                            break;
                        }
                    }
                }
            }

            if (token->tag == PV_NORMALIZER_TAG_DE_ABBREVIATION) {
                pv_normalizer_token_t *collapsed_token = NULL;
                status = pv_normalizer_token_list_collapse_tokens(
                        token_list,
                        potential_abbreviation,
                        start_token,
                        token,
                        &collapsed_token);
                free(potential_abbreviation);
                potential_abbreviation = NULL;
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_token_list_collapse_tokens,
                            pv_status_to_string(status));
                    return status;
                }
                token = collapsed_token;
            } else {
                free(potential_abbreviation);
                potential_abbreviation = NULL;
            }
        }
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_tagger_tag_name(
        const pv_language_info_t *language_info,
        pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS[PV_NORMALIZER_TAG_DE_NAME_INITIAL_LETTER]) {
        const char *string = token->string;

        bool is_dot = (strcmp(string, ".") == 0);
        if (is_dot) {
            pv_normalizer_token_t *previous_token = pv_normalizer_token_get_nth_token_before(token, 1, false);
            if (previous_token != NULL) {
                bool is_capitalized = false;
                pv_status_t status = pv_normalizer_util_is_capitalized_word(
                        language_info,
                        previous_token->string,
                        &is_capitalized);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_util_is_capitalized_word,
                            pv_status_to_string(status));
                    return status;
                }
                if (is_capitalized && (strlen(previous_token->string) == 1)) {
                    previous_token->tag = PV_NORMALIZER_TAG_DE_NAME_INITIAL_LETTER;
                    token->tag = PV_NORMALIZER_TAG_DE_NAME_INITIAL_DOT;
                }
            }
        }
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_tagger_de_merge_word_token(pv_normalizer_token_list_t *token_list) {
    PV_ASSERT(token_list);

    pv_normalizer_token_t *current = token_list->head;
    while (current) {
        if (current->tag == PV_NORMALIZER_TAG_DE_WORD || current->tag == PV_NORMALIZER_TAG_DE_SINGLE_QUOTE) {
            pv_normalizer_token_t *first = current;
            pv_normalizer_token_t *last = current;
            int32_t merged_token_string_len = (int32_t) strlen(first->string);
            while ((last->next) && (!last->next_character_is_space) && (!last->next_character_is_hyphen)) {
                if ((last->next->tag == PV_NORMALIZER_TAG_DE_WORD || last->next->tag == PV_NORMALIZER_TAG_DE_SINGLE_QUOTE) ||
                    (last->tag == PV_NORMALIZER_TAG_DE_SINGLE_QUOTE && last->next->tag == PV_NORMALIZER_TAG_DE_DATE_MONTH)) {
                    last = last->next;
                    merged_token_string_len += (int32_t) strlen(last->string);
                } else {
                    break;
                }
            }
            if (first != last) {
                bool merge_original = (last->tag == PV_NORMALIZER_TAG_DE_SINGLE_QUOTE &&
                                       (last->next_character_is_space ||
                                        last->next == NULL ||
                                        (last->next && last->next->tag == PV_NORMALIZER_TAG_DE_SPACE)));

                char *merged_token_string = calloc(merged_token_string_len + 1, sizeof(char));
                if (!merged_token_string) {
                    PV_ERROR_REPORT(
                            &pv_error_msg_alloc,
                            PV_ERROR_ARGS_PUBLIC_EMPTY(),
                            PV_ERROR_ARGS_PRIVATE("merged_token_string"));
                    return PV_STATUS_OUT_OF_MEMORY;
                }
                strcpy(merged_token_string, first->string);
                pv_normalizer_token_t *curr = first;
                while (curr != last) {
                    curr = curr->next;
                    strcat(merged_token_string, curr->string);
                }

                char *merged_original_string = NULL;
                if (merge_original) {
                    merged_original_string = calloc(strlen(first->original_string) +
                                                    strlen(last->original_string) + 1, sizeof(char));
                    if (!merged_original_string) {
                        PV_ERROR_REPORT(
                                &pv_error_msg_alloc,
                                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                                PV_ERROR_ARGS_PRIVATE("merged_original_string"));
                        free(merged_token_string);
                        return PV_STATUS_OUT_OF_MEMORY;
                    }
                    strcpy(merged_original_string, first->original_string);
                    strcat(merged_original_string, last->original_string);
                }

                pv_normalizer_token_t *merged_token = NULL;
                pv_status_t status = pv_normalizer_token_list_merge_tokens(
                        token_list,
                        merged_token_string,
                        merge_original ? merged_original_string : first->original_string,
                        first,
                        last,
                        &merged_token);
                free(merged_token_string);
                free(merged_original_string);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_token_list_merge_tokens,
                            pv_status_to_string(status));
                    return status;
                }
                merged_token->tag = PV_NORMALIZER_TAG_DE_WORD;
                current = merged_token->next;
                continue;
            }
        }
        current = current->next;
    }
    return PV_STATUS_SUCCESS;
}
