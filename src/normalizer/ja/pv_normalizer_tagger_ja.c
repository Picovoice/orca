#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "core/pv_assert.h"
#include "core/pv_error_messages.h"
#include "orca/normalizer/pv_normalizer_data/pv_normalizer_shared_data.h"
#include "orca/normalizer/pv_normalizer_tagger.h"
#include "orca/normalizer/pv_normalizer_token.h"
#include "orca/normalizer/pv_normalizer_use_cases.h"
#include "orca/normalizer/pv_normalizer_util.h"

#include "orca/normalizer/ja/pv_normalizer_data_ja/pv_normalizer_data_ja.h"
#include "orca/normalizer/ja/pv_normalizer_tagger_ja.h"
#include "orca/normalizer/ja/pv_normalizer_tags_ja.h"
#include "orca/normalizer/ja/pv_normalizer_util_ja.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_normalizer_mock.h"

#endif

struct pv_normalizer_tagger_ja {
    const pv_language_info_t *language_info;
    pv_normalizer_language_t language;

    int32_t num_use_cases;
    const pv_normalizer_use_cases_ja_t *use_cases;
    pv_normalizer_tokenizer_t *tokenizer;
    pv_normalizer_util_trie_t *measurement_trie;
};

static pv_status_t pv_normalizer_tagger_ja_split_untagged_token(
        const pv_normalizer_tagger_ja_t *object,
        pv_normalizer_token_t **token,
        pv_normalizer_token_list_t **token_list,
        bool *did_split);

static pv_status_t pv_normalizer_tagger_ja_tag_postprocess(pv_normalizer_token_list_t *token_list);

static void pv_normalizer_tagger_tag_language_agnostic(pv_normalizer_token_t *token);

static void pv_normalizer_tagger_synchronize_language_agnostic_tags(pv_normalizer_token_list_t *token_list);

static pv_status_t pv_normalizer_tagger_tag_word(
        const pv_language_info_t *language_info,
        pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_tagger_tag_punctuation(
        pv_normalizer_language_t language,
        pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_tagger_tag_cardinal(
        const pv_normalizer_tagger_ja_t *object,
        pv_normalizer_token_t *token,
        pv_normalizer_token_list_t *token_list);

static void pv_normalizer_tagger_tag_negative_cardinal(pv_normalizer_token_t *token);

static void pv_normalizer_tagger_tag_custom_pronunciation(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_tagger_tag_number_range(
        const pv_normalizer_tagger_ja_t *object,
        pv_normalizer_token_t *token,
        pv_normalizer_token_list_t *token_list);

static pv_status_t pv_normalizer_tagger_tag_special_character(
        const pv_normalizer_tagger_ja_t *object,
        pv_normalizer_token_t *token);

static void pv_normalizer_tagger_tag_decimal(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_tagger_tag_measurement(
        const pv_normalizer_tagger_ja_t *object,
        pv_normalizer_token_list_t *token_list,
        pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_tagger_tag_alphanum_spell_out(
        pv_normalizer_token_t *token,
        pv_normalizer_token_list_t *token_list);

static pv_status_t pv_normalizer_tagger_tag_comma_number(
        pv_normalizer_token_t *token,
        pv_normalizer_token_list_t *token_list);

static pv_status_t pv_normalizer_tagger_tag_fraction(
        const pv_normalizer_tagger_ja_t *object,
        pv_normalizer_token_t *token,
        pv_normalizer_token_list_t *token_list);

static pv_status_t pv_normalizer_tagger_tag_url(pv_normalizer_token_t *token);

static pv_status_t pv_normalizer_tagger_tag_currency(
        const pv_normalizer_tagger_ja_t *object,
        pv_normalizer_token_t *token,
        pv_normalizer_token_list_t *token_list);

static pv_status_t pv_normalizer_tagger_tag_abbreviation(
        pv_normalizer_token_t *token,
        pv_normalizer_token_list_t *token_list);

static pv_status_t pv_normalizer_tagger_tag_time(
        pv_normalizer_token_t *token,
        pv_normalizer_token_list_t *token_list);

static pv_status_t pv_normalizer_tagger_tag_digits_sequence(
        const pv_normalizer_tokenizer_t *tokenizer,
        pv_normalizer_token_t *token,
        pv_normalizer_token_list_t *token_list);

static pv_status_t pv_normalizer_tagger_tag_date(
        const pv_normalizer_tokenizer_t *tokenizer,
        pv_normalizer_token_t *token,
        pv_normalizer_token_list_t *token_list);

static pv_status_t pv_normalizer_tagger_tag_name(pv_normalizer_token_t *token);

pv_status_t PV_MOCKABLE(pv_normalizer_tagger_ja_init)(
        const pv_language_info_t *language_info,
        int32_t num_use_cases,
        const pv_normalizer_use_cases_ja_t *use_cases,
        pv_normalizer_tokenizer_t *tokenizer,
        pv_normalizer_tagger_ja_t **object) {
    PV_ASSERT(language_info);
    PV_ASSERT(num_use_cases >= 0);
    PV_ASSERT(use_cases);
    PV_ASSERT(tokenizer);

    PV_ASSERT(object);

    *object = NULL;

    pv_normalizer_tagger_ja_t *o = calloc(1, sizeof(pv_normalizer_tagger_ja_t));
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
        if (use_cases[i] == PV_NORMALIZER_USE_MEASUREMENT_NORMALIZER_JA) {
            // TODO (Ted): Below line is passing the wrong <num_normalizable_characters> which creates more children because in TRIE we only use upper case, but this doesn't cause bug for now, so leave it for future refactoring. Do the same for other languages.
            status = pv_normalizer_util_trie_create(
                    num_normalizable_characters,
                    normalizable_characters,
                    PV_ARRAY_LEN(PV_NORMALIZER_MEASUREMENTS_JA),
                    (const char **) PV_NORMALIZER_MEASUREMENTS_JA,
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

void PV_MOCKABLE(pv_normalizer_tagger_ja_delete)(pv_normalizer_tagger_ja_t *object) {
    if (object) {
        pv_normalizer_util_trie_delete(object->measurement_trie);
        free(object);
    }
}

static pv_status_t pv_normalizer_tagger_ja_tag_token(
        const pv_normalizer_tagger_ja_t *object,
        pv_normalizer_token_t *token,
        pv_normalizer_token_list_t *token_list) {
    PV_ASSERT(object);
    PV_ASSERT(token);
    PV_ASSERT(token_list);

    (void) token_list;

    for (int32_t j = 0; j < object->num_use_cases; j++) {
        pv_normalizer_tagger_tag_language_agnostic(token);
        if (object->use_cases[j] == PV_NORMALIZER_USE_WORD_NORMALIZER_JA) {
            pv_status_t status = pv_normalizer_tagger_tag_word(object->language_info, token);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_tag_word,
                        pv_status_to_string(status));
                return status;
            }
        } else if (object->use_cases[j] == PV_NORMALIZER_USE_CUSTOM_PRONUNCIATION_NORMALIZER_JA) {
            pv_normalizer_tagger_tag_custom_pronunciation(token);
        } else if (object->use_cases[j] == PV_NORMALIZER_USE_PUNCTUATION_NORMALIZER_JA) {
            pv_status_t status = pv_normalizer_tagger_tag_punctuation(object->language, token);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_tag_punctuation,
                        pv_status_to_string(status));
                return status;
            }
        } else if (object->use_cases[j] == PV_NORMALIZER_USE_CARDINAL_NORMALIZER_JA) {
            pv_status_t status = pv_normalizer_tagger_tag_cardinal(object, token, token_list);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_tag_cardinal,
                        pv_status_to_string(status));
                return status;
            }
        } else if (object->use_cases[j] == PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_JA) {
            pv_normalizer_tagger_tag_negative_cardinal(token);
        } else if (object->use_cases[j] == PV_NORMALIZER_USE_NUMBER_RANGE_NORMALIZER_JA) {
            pv_status_t status = pv_normalizer_tagger_tag_number_range(object, token, token_list);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_tag_number_range,
                        pv_status_to_string(status));
                return status;
            }
        } else if (object->use_cases[j] == PV_NORMALIZER_USE_SPECIAL_CHARACTER_NORMALIZER_JA) {
            pv_status_t status = pv_normalizer_tagger_tag_special_character(object, token);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_tag_special_character,
                        pv_status_to_string(status));
                return status;
            }
        } else if (object->use_cases[j] == PV_NORMALIZER_USE_DECIMAL_NORMALIZER_JA) {
            pv_normalizer_tagger_tag_decimal(token);
        } else if (object->use_cases[j] == PV_NORMALIZER_USE_MEASUREMENT_NORMALIZER_JA) {
            pv_status_t status = pv_normalizer_tagger_tag_measurement(object, token_list, token);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_tag_measurement,
                        pv_status_to_string(status));
                return status;
            }
        } else if (object->use_cases[j] == PV_NORMALIZER_USE_FRACTION_NORMALIZER_JA) {
            pv_status_t status = pv_normalizer_tagger_tag_fraction(object, token, token_list);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_tag_fraction,
                        pv_status_to_string(status));
                return status;
            }
        } else if (object->use_cases[j] == PV_NORMALIZER_USE_ALPHANUM_SPELL_OUT_NORMALIZER_JA) {
            pv_status_t status = pv_normalizer_tagger_tag_alphanum_spell_out(token, token_list);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_tag_alphanum_spell_out,
                        pv_status_to_string(status));
                return status;
            }
        } else if (object->use_cases[j] == PV_NORMALIZER_USE_URL_NORMALIZER_JA) {
            PV_ASSERT(pv_normalizer_util_is_in_use_cases(
                    object->num_use_cases,
                    (const int32_t *) object->use_cases,
                    PV_NORMALIZER_USE_ALPHANUM_SPELL_OUT_NORMALIZER_JA));

            pv_status_t status = pv_normalizer_tagger_tag_url(token);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_tag_url,
                        pv_status_to_string(status));
                return status;
            }
        } else if (object->use_cases[j] == PV_NORMALIZER_USE_CURRENCY_NORMALIZER_JA) {
            pv_status_t status = pv_normalizer_tagger_tag_currency(object, token, token_list);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_tag_currency,
                        pv_status_to_string(status));
                return status;
            }
        } else if (object->use_cases[j] == PV_NORMALIZER_USE_TIME_NORMALIZER_JA) {
            pv_status_t status = pv_normalizer_tagger_tag_time(token, token_list);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_tag_time,
                        pv_status_to_string(status));
                return status;
            }
        } else if (object->use_cases[j] == PV_NORMALIZER_USE_ABBREVIATION_NORMALIZER_JA) {
            pv_status_t status = pv_normalizer_tagger_tag_abbreviation(token, token_list);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_tag_abbreviation,
                        pv_status_to_string(status));
                return status;
            }
        } else if (object->use_cases[j] == PV_NORMALIZER_USE_DIGITS_SEQUENCE_NORMALIZER_JA) {
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
        } else if (object->use_cases[j] == PV_NORMALIZER_USE_DATE_NORMALIZER_JA) {
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
        } else if (object->use_cases[j] == PV_NORMALIZER_USE_NAME_NORMALIZER_JA) {
            PV_ASSERT(pv_normalizer_util_is_in_use_cases(
                    object->num_use_cases,
                    (const int32_t *) object->use_cases,
                    PV_NORMALIZER_USE_ALPHANUM_SPELL_OUT_NORMALIZER_JA));

            pv_status_t status = pv_normalizer_tagger_tag_name(token);
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

pv_status_t PV_MOCKABLE(pv_normalizer_tagger_ja_tag)(
        pv_normalizer_tagger_ja_t *object,
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

        pv_status_t status = pv_normalizer_tagger_ja_tag_token(object, token, token_list);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_tagger_ja_tag_token,
                    pv_status_to_string(status));
            return status;
        }

        bool did_split = false;
        if (split_untagged && (token->tag == PV_NORMALIZER_TAG_JA_NONE)) {
            pv_normalizer_token_list_t *split_token_list = NULL;

            status = pv_normalizer_tagger_ja_split_untagged_token(
                    object,
                    &token,
                    &split_token_list,
                    &did_split);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_en_split_untagged_token,
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

    pv_status_t status = pv_normalizer_tagger_ja_tag_postprocess(token_list);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_tagger_ja_tag_postprocess,
                pv_status_to_string(status));
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static int32_t pv_normalizer_tagger_get_alphanum_char_index(char c) {
    for (int32_t i = 0; i < PV_NORMALIZER_NUM_ALPHANUM_CHARS_JA; i++) {
        if (c == PV_NORMALIZER_ALPHANUM_CHARS_JA[i]) {
            return i;
        }
    }

    return -1;
}

pv_status_t pv_normalizer_tagger_ja_tag_postprocess(pv_normalizer_token_list_t *token_list) {
    PV_ASSERT(token_list);

    pv_normalizer_token_t *token = token_list->head;
    while (token != NULL) {
        bool is_spell_out =
                (token->tag == PV_NORMALIZER_TAG_JA_ALPHANUM_SPELL_OUT) ||
                (token->tag == PV_NORMALIZER_TAG_JA_LETTER_SPELL_OUT) ||
                (token->tag == PV_NORMALIZER_TAG_JA_ACRONYM) ||
                (token->tag == PV_NORMALIZER_TAG_JA_NAME_INITIAL_LETTER);
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

                status = pv_normalizer_util_upper_inplace(current->string);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_util_upper_inplace,
                            pv_status_to_string(status));
                    return status;
                }

                current->tag = PV_NORMALIZER_TAG_JA_LETTER_SPELL_OUT;
                current->tag_data_index = pv_normalizer_tagger_get_alphanum_char_index(current->string[0]);
                current = current->next;
            }

            pv_status_t status = pv_normalizer_util_upper_inplace(current->string);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_util_upper_inplace,
                        pv_status_to_string(status));
                return status;
            }

            current->tag = PV_NORMALIZER_TAG_JA_LETTER_SPELL_OUT;
            current->tag_data_index = pv_normalizer_tagger_get_alphanum_char_index(current->string[0]);
        }

        token = token->next;
    }

    pv_normalizer_tagger_synchronize_language_agnostic_tags(token_list);

    return PV_STATUS_SUCCESS;
}

void pv_normalizer_tagger_tag_language_agnostic(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    pv_normalizer_tagger_tag_from_language_agnostic_common(
            token,
            PV_NORMALIZER_TAG_JA_NONE,
            PV_NORMALIZER_TAG_JA_SPACE,
            PV_NORMALIZER_TAG_JA_PUNCTUATION,
            PV_NORMALIZER_TAG_JA_CUSTOM_PRONUNCIATION);
}

void pv_normalizer_tagger_synchronize_language_agnostic_tags(pv_normalizer_token_list_t *token_list) {
    PV_ASSERT(token_list);

    pv_normalizer_token_list_synchronize_language_agnostic_tags_common(
            token_list,
            PV_NORMALIZER_TAG_JA_SPACE,
            PV_NORMALIZER_TAG_JA_PUNCTUATION,
            -1,
            PV_NORMALIZER_TAG_JA_CUSTOM_PRONUNCIATION,
            -1);
}

pv_status_t pv_normalizer_tagger_tag_word(const pv_language_info_t *language_info, pv_normalizer_token_t *token) {
    PV_ASSERT(language_info);
    PV_ASSERT(token);

    if (PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[PV_NORMALIZER_TAG_JA_WORD]) {
        pv_status_t status = pv_normalizer_tagger_tag_word_common(language_info, token, PV_NORMALIZER_TAG_JA_WORD);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_tagger_tag_word_common,
                    pv_status_to_string(status));
            return status;
        }
    }
    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_tagger_tag_punctuation(
        pv_normalizer_language_t language,
        pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[PV_NORMALIZER_TAG_JA_PUNCTUATION]) {
        bool is_punctuation = false;
        pv_status_t status = pv_normalizer_util_is_punctuation(language, token->string, &is_punctuation);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_util_is_punctuation,
                    pv_status_to_string(status));
            return status;
        }

        if (is_punctuation) {
            token->tag = PV_NORMALIZER_TAG_JA_PUNCTUATION;
        }
    }

    return PV_STATUS_SUCCESS;
}

static pv_status_t merge_comma_number_tokens(
        pv_normalizer_token_t *first,
        pv_normalizer_token_t *last,
        pv_normalizer_token_list_t *token_list) {
    PV_ASSERT(first);
    PV_ASSERT(last);
    PV_ASSERT(token_list);

    pv_normalizer_token_tag_ja_t tag = first->tag;
    if (last->tag == PV_NORMALIZER_TAG_JA_CURRENCY) {
        if (first->tag == PV_NORMALIZER_TAG_JA_CARDINAL) {
            tag = PV_NORMALIZER_TAG_JA_CURRENCY;
        } else {
            tag = PV_NORMALIZER_TAG_JA_NEGATIVE_CURRENCY;
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
        if ((strcmp(current->string, ",") != 0) && (strcmp(current->string, "、") != 0)) {
            combined_string_length += (int32_t) strlen(current->string);
        }
        current = current->next;
    }

    char *last_string = calloc((combined_string_length + 1), sizeof(char));
    if (!last->string) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("last_string"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    char combined_string[combined_string_length + 1];
    combined_string[0] = '\0';

    current = first;
    while ((current != NULL) && (current != last->next)) {
        pv_normalizer_token_t *next = current->next;

        if ((strcmp(current->string, ",") != 0) && (strcmp(current->string, "、") != 0)) {
            strcat(combined_string, current->string);
        }

        if (current != last) {
            pv_normalizer_token_list_remove_token(token_list, current);
        }

        current = next;
    }

    strcpy(last_string, combined_string);
    free(last->string);
    last->string = last_string;

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

pv_status_t pv_normalizer_tagger_tag_comma_number(pv_normalizer_token_t *token, pv_normalizer_token_list_t *token_list) {
    PV_ASSERT(token);
    PV_ASSERT(token_list);

    bool current_is_cardinal = true;
    pv_normalizer_token_t *current = token;

    pv_normalizer_token_t *previous = pv_normalizer_token_get_nth_token_before(current, 1, true);
    bool is_first_token =
            ((previous == NULL) ||
             previous->next_character_is_space ||
             (previous->tag == PV_NORMALIZER_TAG_JA_WORD) ||
             (previous->tag == PV_NORMALIZER_TAG_JA_SPECIAL_CHARACTER) ||
             (strcmp(previous->string, "-") == 0) ||
             (strcmp(previous->string, "~") == 0) ||
             (strcmp(previous->string, "/") == 0) ||
             (strcmp(previous->string, "(") == 0));

    while (!is_first_token) {
        current = current->previous;
        char *string = current->string;

        previous = pv_normalizer_token_get_nth_token_before(current, 1, true);
        is_first_token =
                ((previous == NULL) ||
                 previous->next_character_is_space ||
                 (previous->tag == PV_NORMALIZER_TAG_JA_WORD) ||
                 (previous->tag == PV_NORMALIZER_TAG_JA_SPECIAL_CHARACTER) ||
                 (previous->tag == PV_NORMALIZER_TAG_JA_PUNCTUATION &&
                  !((strcmp(previous->string, ",") == 0) || (strcmp(previous->string, "、") == 0))) ||
                 (strcmp(previous->string, "-") == 0) ||
                 (strcmp(previous->string, "~") == 0) ||
                 (strcmp(previous->string, "/") == 0) ||
                 (strcmp(previous->string, "(") == 0));

        if (current_is_cardinal) {
            if ((strcmp(current->string, ",") == 0) || (strcmp(current->string, "、") == 0)) {
                current_is_cardinal = false;
            } else {
                return PV_STATUS_SUCCESS;
            }
        } else {
            if ((current->tag == PV_NORMALIZER_TAG_JA_CARDINAL) &&
                ((is_first_token && (strlen(string) <= 3)) ||
                 (!is_first_token && (strlen(string) == 3)))) {
                current_is_cardinal = true;
            } else if (is_first_token &&
                       (current->tag == PV_NORMALIZER_TAG_JA_NEGATIVE_CARDINAL) &&
                       (strlen(string) <= 4)) {
                current_is_cardinal = true;
            } else if (is_first_token &&
                       (current->tag == PV_NORMALIZER_TAG_JA_CURRENCY)) {
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
                       (current->tag == PV_NORMALIZER_TAG_JA_NEGATIVE_CURRENCY)) {
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

    if ((current != token) && current_is_cardinal) {
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


pv_status_t pv_normalizer_tagger_tag_cardinal(
        const pv_normalizer_tagger_ja_t *object,
        pv_normalizer_token_t *token,
        pv_normalizer_token_list_t *token_list) {
    PV_ASSERT(object);
    PV_ASSERT(token);
    PV_ASSERT(token_list);

    if (PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[PV_NORMALIZER_TAG_JA_CARDINAL]) {
        const char *string = token->string;
        if (pv_normalizer_util_only_contains_digits(string)) {
            token->tag = PV_NORMALIZER_TAG_JA_CARDINAL;
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
        bool is_before_tilde = false;
        status = pv_normalizer_util_check_token_is_before_character(token, TILDE, &is_before_tilde);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_util_check_token_is_before_character,
                    pv_status_to_string(status));
            return status;
        }

        bool is_before_decimal =
                (!token->next_character_is_space &&
                 (token->next != NULL) &&
                 (strcmp(token->next->string, ".") == 0) &&
                 !token->next->next_character_is_space);

        bool is_before_punctuation = false;
        if (!token->next_character_is_space && (token->next != NULL)) {
            bool next_is_punctuation = false;
            pv_status_t status = pv_normalizer_util_is_punctuation(
                    object->language,
                    token->next->string,
                    &next_is_punctuation);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_util_is_punctuation,
                        pv_status_to_string(status));
                return status;
            }
            is_before_punctuation = next_is_punctuation &&
                                    ((token->next->next_character_is_space) ||
                                     ((token->next->next != NULL) && ((token->next->next->tag == PV_NORMALIZER_TAG_JA_WORD) ||
                                                                      (strcmp(token->next->next->string, "\n") == 0))) ||
                                     (token->next->next == NULL));
        }

        bool is_before_word_or_parenthesis =
                (token->next != NULL) &&
                ((token->next->tag == PV_NORMALIZER_TAG_JA_WORD) || (strcmp(token->next->string, ")") == 0));
        bool could_be_fraction_numerator = (token->next != NULL) && (token->next->string[0] == '/');
        if ((token->next_character_is_space ||
             (token->next == NULL) ||
             is_before_word_or_parenthesis ||
             could_be_fraction_numerator ||
             is_before_hyphen || is_before_tilde ||
             is_before_decimal || is_before_punctuation) &&
            (strlen(string) == 3)) {
            pv_status_t status = pv_normalizer_tagger_tag_comma_number(token, token_list);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_tag_comma_number,
                        pv_status_to_string(status));
                return status;
            }
        }
    }

    return PV_STATUS_SUCCESS;
}

void pv_normalizer_tagger_tag_negative_cardinal(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[PV_NORMALIZER_TAG_JA_NEGATIVE_CARDINAL]) {
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
            token->tag = PV_NORMALIZER_TAG_JA_NEGATIVE_CARDINAL;
        }
    }
}

static pv_status_t is_all_non_ja(const char *string, bool *all_non_ja) {
    PV_ASSERT(string);
    PV_ASSERT(all_non_ja);

    *all_non_ja = true;

    while (*string != '\0') {
        int32_t num_bytes_character = 0;
        pv_status_t status = pv_language_num_bytes_character((unsigned char) *string, &num_bytes_character);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT(
                    &pv_error_msg_invalid_argument,
                    PV_ERROR_ARGS_PUBLIC("text"),
                    PV_ERROR_ARGS_PRIVATE_EMPTY());
            return status;
        }

        char character[PV_NORMALIZER_MAX_NUM_BYTES_PER_CHARACTER] = {0};

        strncpy(character, string, num_bytes_character);
        string += num_bytes_character;
        character[num_bytes_character] = '\0';

        if (pv_normalizer_util_ja_is_word_character(character)) {
            *all_non_ja = false;
            return PV_STATUS_SUCCESS;
        }
    }

    return PV_STATUS_SUCCESS;
}

static pv_status_t pv_normalizer_tagger_tag_measurement(
        const pv_normalizer_tagger_ja_t *object,
        pv_normalizer_token_list_t *token_list,
        pv_normalizer_token_t *token) {
    PV_ASSERT(object);
    PV_ASSERT(token_list);
    PV_ASSERT(token);

    if (PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[PV_NORMALIZER_TAG_JA_MEASUREMENT]) {
        const char *string = token->string;

        // Space-separated number + measurement eg. `2 ml`
        pv_normalizer_token_t *previous = pv_normalizer_token_get_nth_token_before(token, 1, true);
        if ((previous != NULL) &&
            ((previous->tag == PV_NORMALIZER_TAG_JA_CARDINAL) ||
             (previous->tag == PV_NORMALIZER_TAG_JA_NEGATIVE_CARDINAL) ||
             (previous->tag == PV_NORMALIZER_TAG_JA_DECIMAL_DIGITS))) {
            int32_t trie_index = -1;
            pv_status_t status = pv_normalizer_util_trie_search(object->measurement_trie, string, &trie_index);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_util_trie_search,
                        pv_status_to_string(status));
                return status;
            }

            if (trie_index >= 0) {
                pv_normalizer_token_t *next = pv_normalizer_token_get_nth_token_after(token, 1, false);
                if ((next == NULL) || (next->tag != PV_NORMALIZER_TAG_JA_LETTER_SPELL_OUT)) {
                    token->tag = PV_NORMALIZER_TAG_JA_MEASUREMENT;
                    token->tag_data_index = trie_index;
                    return PV_STATUS_SUCCESS;
                }
            }
        }

        // No context measurement chains eg. `lb-ft` and `lb.-ft.`
        pv_normalizer_token_t *two_previous = pv_normalizer_token_get_nth_token_before(token, 2, true);
        bool all_non_ja = true;
        pv_status_t status = is_all_non_ja(token->original_string, &all_non_ja);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    is_all_non_ja,
                    pv_status_to_string(status));
            return status;
        }
        if (all_non_ja) {
            if ((previous != NULL) && (previous->tag == PV_NORMALIZER_TAG_JA_ALPHANUM_SPELL_OUT)) {
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
                    token->tag = PV_NORMALIZER_TAG_JA_MEASUREMENT;
                    token->tag_data_index = trie_index;

                    previous->tag = PV_NORMALIZER_TAG_JA_MEASUREMENT;
                    previous->tag_data_index = prev_trie_index;
                    return PV_STATUS_SUCCESS;
                }
            } else if ((previous != NULL) && (previous->tag == PV_NORMALIZER_TAG_JA_DOT) &&
                       (two_previous != NULL) && (two_previous->tag == PV_NORMALIZER_TAG_JA_ALPHANUM_SPELL_OUT)) {
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
                    token->tag = PV_NORMALIZER_TAG_JA_MEASUREMENT;
                    token->tag_data_index = trie_index;

                    two_previous->tag = PV_NORMALIZER_TAG_JA_MEASUREMENT;
                    two_previous->tag_data_index = prev_trie_index;
                    return PV_STATUS_SUCCESS;
                }
            }
        }

        // Contextual measurement chains eg. `2 lb ft` and `2 lb.-ft.`
        if (((previous != NULL) && (previous->tag == PV_NORMALIZER_TAG_JA_MEASUREMENT)) ||
            ((two_previous != NULL) && (two_previous->tag == PV_NORMALIZER_TAG_JA_MEASUREMENT))) {
            int32_t trie_index = -1;
            pv_status_t status = pv_normalizer_util_trie_search(object->measurement_trie, string, &trie_index);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_util_trie_search,
                        pv_status_to_string(status));
                return status;
            }

            if (trie_index >= 0) {
                token->tag = PV_NORMALIZER_TAG_JA_MEASUREMENT;
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

                    status = pv_normalizer_tagger_ja_tag_token(object, token, token_list);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_normalizer_tagger_ja_tag_token,
                                pv_status_to_string(status));
                        return status;
                    }

                    if (token->tag == PV_NORMALIZER_TAG_JA_CARDINAL && strlen(token->string) == 3) {
                        status = pv_normalizer_tagger_tag_comma_number(token, token_list);
                        if (status != PV_STATUS_SUCCESS) {
                            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                    pv_normalizer_tagger_tag_comma_number,
                                    pv_status_to_string(status));
                            return status;
                        }
                    }

                    if (token->tag == PV_NORMALIZER_TAG_JA_CARDINAL ||
                        token->tag == PV_NORMALIZER_TAG_JA_NEGATIVE_CARDINAL ||
                        token->tag == PV_NORMALIZER_TAG_JA_DECIMAL_DIGITS) {
                        measurement_token->tag = PV_NORMALIZER_TAG_JA_MEASUREMENT;
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

                    status = pv_normalizer_tagger_ja_tag_token(object, token, token_list);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_normalizer_tagger_ja_tag_token,
                                pv_status_to_string(status));
                        return status;
                    }

                    pv_normalizer_token_t *next_previous = pv_normalizer_token_get_nth_token_before(next, 1, false);
                    if (next_previous->tag == PV_NORMALIZER_TAG_JA_MEASUREMENT) {
                        status = pv_normalizer_token_list_unroll_token(1, next, token_list);
                        if (status != PV_STATUS_SUCCESS) {
                            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                    pv_normalizer_token_list_unroll_token,
                                    pv_status_to_string(status));
                            return status;
                        }
                        next->tag = PV_NORMALIZER_TAG_JA_PER_SLASH;

                        next->next->tag = PV_NORMALIZER_TAG_JA_MEASUREMENT;
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
                            next->tag = PV_NORMALIZER_TAG_JA_PER_SLASH;

                            next_previous->tag = PV_NORMALIZER_TAG_JA_MEASUREMENT;
                            next_previous->tag_data_index = trie_index_two;

                            next->next->tag = PV_NORMALIZER_TAG_JA_MEASUREMENT;
                            next->next->tag_data_index = trie_index;
                        }
                    }

                    return PV_STATUS_SUCCESS;
                }
            }
        }
    }

    // `miles/hour`
    if (PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[PV_NORMALIZER_TAG_JA_PER_SLASH]) {

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

            for (int32_t i = 0; i < PV_NORMALIZER_NUM_MEASUREMENTS_JA; i++) {
                if (strcmp(previous_upper_string, PV_NORMALIZER_MEASUREMENTS_JA[i]) == 0) {
                    measurement_before = true;
                    break;
                }
            }
            for (int32_t i = 0; i < PV_NORMALIZER_NUM_MEASUREMENTS_JA; i++) {
                if (strcmp(previous_upper_string, PV_NORMALIZER_MEASUREMENTS_VERBALIZED_JA[i]) == 0) {
                    measurement_before = true;
                    break;
                }
            }
            free(previous_upper_string);

            for (int32_t i = 0; i < PV_NORMALIZER_NUM_MEASUREMENTS_JA; i++) {
                if (strcmp(next_upper_string, PV_NORMALIZER_MEASUREMENTS_JA[i]) == 0) {
                    measurement_after = true;
                    break;
                }
            }
            for (int32_t i = 0; i < PV_NORMALIZER_NUM_MEASUREMENTS_JA; i++) {
                if (strcmp(next_upper_string, PV_NORMALIZER_MEASUREMENTS_VERBALIZED_JA[i]) == 0) {
                    measurement_after = true;
                    break;
                }
            }
            free(next_upper_string);

            if (measurement_before && measurement_after) {
                token->tag = PV_NORMALIZER_TAG_JA_PER_SLASH;
            }
        }
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_tagger_ja_split_untagged_token(
        const pv_normalizer_tagger_ja_t *object,
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

    if ((*token)->tag == PV_NORMALIZER_TAG_JA_NONE) {
        const char *string = (*token)->string;

        bool found_hyphen = false;
        bool found_wavedash = false;
        bool found_special_character = false;
        char character[PV_NORMALIZER_MAX_NUM_BYTES_PER_CHARACTER] = {0};

        while (*string != '\0') {
            if (*string == '-') {
                found_hyphen = true;
            }
            if (*string == '~') {
                found_wavedash = true;
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

            strncpy(character, string, num_bytes_character);
            string += num_bytes_character;
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
                    ' ',
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
        } else if (found_wavedash) {
            pv_status_t status = pv_normalizer_tokenizer_tokenize(
                    object->tokenizer,
                    (*token)->string,
                    '~',
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
        const pv_normalizer_tagger_ja_t *object,
        pv_normalizer_token_t *token,
        pv_normalizer_token_list_t *token_list) {
    PV_ASSERT(object);
    PV_ASSERT(token);
    PV_ASSERT(token_list);

    if (PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[PV_NORMALIZER_TAG_JA_NUMBER_RANGE_TO]) {
        const char *string = token->string;

        bool is_range = true;
        int32_t num_hyphens = 0;
        int32_t num_cardinals = 0;
        int32_t hyphen_index = 0;

        int32_t i = 0;
        while (string[i] != '\0' && is_range) {
            if (string[i] == '-' || string[i] == '~') {
                hyphen_index = i;
                num_hyphens++;
                i++;
            } else {
                num_cardinals++;
                while ((string[i] != '\0') && (string[i] != '-') && (string[i] != '~')) {
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

            token->next->tag = PV_NORMALIZER_TAG_JA_NUMBER_RANGE_TO;
        }
    }
    return PV_STATUS_SUCCESS;
}

void pv_normalizer_tagger_tag_custom_pronunciation(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[PV_NORMALIZER_TAG_JA_CUSTOM_PRONUNCIATION]) {
        if (token->pronunciation != NULL) {
            token->tag = PV_NORMALIZER_TAG_JA_CUSTOM_PRONUNCIATION;
        }
    }
}

pv_status_t
pv_normalizer_tagger_tag_special_character(const pv_normalizer_tagger_ja_t *object, pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[PV_NORMALIZER_TAG_JA_SPECIAL_CHARACTER]) {
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
            token->tag = PV_NORMALIZER_TAG_JA_SPECIAL_CHARACTER;
        }
    }
    return PV_STATUS_SUCCESS;
}

void pv_normalizer_tagger_tag_decimal(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[PV_NORMALIZER_TAG_JA_DECIMAL_DIGITS]) {

        bool has_dot_before_current =
                (token->previous != NULL) &&
                (strcmp(token->previous->string, ".") == 0) &&
                (!token->previous->next_character_is_space);

        if (has_dot_before_current) {
            pv_normalizer_token_t *two_previous = pv_normalizer_token_get_nth_token_before(token, 2, false);
            bool has_space_or_null_before_dot =
                    (two_previous == NULL) ||
                    two_previous->next_character_is_space ||
                    (two_previous->tag == PV_NORMALIZER_TAG_JA_WORD) ||
                    (two_previous->tag == PV_NORMALIZER_TAG_JA_SPACE);
            bool has_cardinal_before_dot =
                    ((two_previous != NULL) &&
                     ((two_previous->tag == PV_NORMALIZER_TAG_JA_CARDINAL) ||
                      (two_previous->tag == PV_NORMALIZER_TAG_JA_NEGATIVE_CARDINAL)) &&
                     !two_previous->next_character_is_space);

            if (has_space_or_null_before_dot || has_cardinal_before_dot) {
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
                    token->tag = PV_NORMALIZER_TAG_JA_DECIMAL_DIGITS;
                    token->previous->tag = PV_NORMALIZER_TAG_JA_DECIMAL_POINT;
                }
            }
        }
    }
}

static pv_status_t pv_normalizer_tagger_tag_fraction(
        const pv_normalizer_tagger_ja_t *object,
        pv_normalizer_token_t *token,
        pv_normalizer_token_list_t *token_list) {
    PV_ASSERT(object);
    PV_ASSERT(token);
    PV_ASSERT(token_list);

    if (PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[PV_NORMALIZER_TAG_JA_FRACTION_SLASH]) {
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

            status = pv_normalizer_tagger_ja_tag_token(object, token, token_list);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_ja_tag_token,
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

            status = pv_normalizer_tagger_ja_tag_token(object, token->next->next, token_list);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_tagger_ja_tag_token,
                        pv_status_to_string(status));
                return status;
            }

            bool before_slash_is_number =
                    ((token->tag == PV_NORMALIZER_TAG_JA_CARDINAL) ||
                     (token->tag == PV_NORMALIZER_TAG_JA_NEGATIVE_CARDINAL) ||
                     (token->tag == PV_NORMALIZER_TAG_JA_DECIMAL_DIGITS));
            bool after_slash_is_number =
                    ((token->next->next->tag == PV_NORMALIZER_TAG_JA_CARDINAL) ||
                     (token->next->next->tag == PV_NORMALIZER_TAG_JA_NEGATIVE_CARDINAL) ||
                     (token->next->next->tag == PV_NORMALIZER_TAG_JA_DECIMAL_DIGITS));

            if (before_slash_is_number && after_slash_is_number) {
                token->next->tag = PV_NORMALIZER_TAG_JA_FRACTION_SLASH;
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

    if (length == 0) {
        return PV_STATUS_SUCCESS;
    }

    if (PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[PV_NORMALIZER_TAG_JA_ALPHANUM_SPELL_OUT]) {

        int32_t found_alpha = false;
        int32_t i = 0;
        while (i < length) {
            if (!(isalnum(string[i]))) {
                return PV_STATUS_SUCCESS;
            }
            if (isalpha(string[i])) {
                found_alpha = true;
            }

            i++;
        }

        if (found_alpha) {
            token->tag = PV_NORMALIZER_TAG_JA_ALPHANUM_SPELL_OUT;
        }
    }

    // SPECIAL CASES: 400BC, 800AD, 800BCE, etc.
    if ((token->tag == PV_NORMALIZER_TAG_JA_ALPHANUM_SPELL_OUT) && (length >= 3)) {
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

            ad_bc_token->tag = PV_NORMALIZER_TAG_JA_LETTER_SPELL_OUT;
            token->tag = PV_NORMALIZER_TAG_JA_CARDINAL;
        }
    }

    return PV_STATUS_SUCCESS;
}

static pv_status_t pv_normalizer_tagger_tag_time(
        pv_normalizer_token_t *token,
        pv_normalizer_token_list_t *token_list) {

    if (PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[token->tag] >=
        PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[PV_NORMALIZER_TAG_JA_TIME_HOURS]) {
        return PV_STATUS_SUCCESS;
    }

    int32_t num_tokens_before = pv_normalizer_token_get_num_tokens_before(token);

    // TAG: hours/minutes (e.g. 7:31, tokenized to [7, :, 31])
    bool can_be_seconds_or_minutes = (strlen(token->string) == 2) &&
                                     isdigit(token->string[0]) &&
                                     isdigit(token->string[1]);

    if (can_be_seconds_or_minutes) {
        bool has_colon_before_current = (num_tokens_before > 0) && (strcmp(token->previous->string, ":") == 0);

        bool has_minute_before_colon =
                (num_tokens_before > 1) &&
                (token->previous->previous->tag == PV_NORMALIZER_TAG_JA_TIME_MINUTES) &&
                !pv_normalizer_util_string_number_greater_than_int(token->previous->previous->string, 59);
        if (has_colon_before_current && has_minute_before_colon) {
            if (!pv_normalizer_util_string_number_greater_than_int(token->string, 59)) {
                token->previous->tag = PV_NORMALIZER_TAG_JA_TIME_COLON;
                token->tag = PV_NORMALIZER_TAG_JA_TIME_SECONDS;

                return PV_STATUS_SUCCESS;
            }
        }

        bool has_hour_before_colon =
                (num_tokens_before > 1) &&
                (token->previous->previous->tag == PV_NORMALIZER_TAG_JA_CARDINAL) &&
                !pv_normalizer_util_string_number_greater_than_int(token->previous->previous->string, 24) &&
                pv_normalizer_util_string_number_greater_than_int(token->previous->previous->string, 0);
        if (has_colon_before_current && has_hour_before_colon) {
            if (!pv_normalizer_util_string_number_greater_than_int(token->string, 59)) {
                token->previous->previous->tag = PV_NORMALIZER_TAG_JA_TIME_HOURS;
                token->previous->tag = PV_NORMALIZER_TAG_JA_TIME_COLON;
                token->tag = PV_NORMALIZER_TAG_JA_TIME_MINUTES;

                return PV_STATUS_SUCCESS;
            }
        }
    }

    // TAG: " am", " pm", " PM", " AM"
    if (num_tokens_before > 0) {
        if ((strcmp(token->string, "am") == 0) ||
            (strcmp(token->string, "pm") == 0) ||
            (strcmp(token->string, "AM") == 0) ||
            (strcmp(token->string, "PM") == 0)) {

            bool is_space_before = (token->previous->tag == PV_NORMALIZER_TAG_JA_SPACE);
            if (is_space_before) {
                pv_normalizer_token_t *two_previous = pv_normalizer_token_get_nth_token_before(token, 2, false);
                bool is_only_hour_time_before_current =
                        (two_previous != NULL) &&
                        (two_previous->tag == PV_NORMALIZER_TAG_JA_CARDINAL) &&
                        !pv_normalizer_util_string_number_greater_than_int(two_previous->string, 12) &&
                        pv_normalizer_token_is_space_or_null(pv_normalizer_token_get_nth_token_before(token, 3, false));
                bool has_time_before_current =
                        ((two_previous != NULL) &&
                         ((two_previous->tag == PV_NORMALIZER_TAG_JA_TIME_HOURS) ||
                          (two_previous->tag == PV_NORMALIZER_TAG_JA_TIME_MINUTES))) ||
                        is_only_hour_time_before_current;
                if (has_time_before_current) {
                    if (is_only_hour_time_before_current) {
                        two_previous->tag = PV_NORMALIZER_TAG_JA_TIME_HOURS;
                    }
                    token->tag = PV_NORMALIZER_TAG_JA_TIME_AM_PM;
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

    // TAG: "<time> a.m." (tokenized to [a, ., m, .])
    bool is_dot = (strcmp(token->string, ".") == 0);
    if (is_dot && (num_tokens_before > 4)) {
        bool has_m_dot =
                (((strcmp(token->previous->string, "m") == 0)) ||
                 (strcmp(token->previous->string, "M") == 0)) &&
                (strcmp(token->previous->previous->string, ".") == 0);

        pv_normalizer_token_t *three_previous = pv_normalizer_token_get_nth_token_before(token, 3, false);
        bool is_pm =
                has_m_dot &&
                (((strcmp(three_previous->string, "p") == 0)) ||
                 (((strcmp(three_previous->string, "P") == 0))));
        bool is_am =
                has_m_dot &&
                (((strcmp(three_previous->string, "a") == 0)) ||
                 (((strcmp(three_previous->string, "A") == 0))));
        if (is_am || is_pm) {
            pv_normalizer_token_t *five_previous = pv_normalizer_token_get_nth_token_before(token, 5, false);
            bool is_only_hour_time_before_current =
                    (five_previous->tag == PV_NORMALIZER_TAG_JA_CARDINAL) &&
                    !pv_normalizer_util_string_number_greater_than_int(five_previous->string, 12);
            bool has_time_before_current =
                    (five_previous->tag == PV_NORMALIZER_TAG_JA_TIME_HOURS) ||
                    (five_previous->tag == PV_NORMALIZER_TAG_JA_TIME_MINUTES) ||
                    is_only_hour_time_before_current;

            if (has_time_before_current) {
                char *original_string = calloc(strlen(three_previous->original_string) + strlen(token->original_string) + 1, sizeof(char));
                if (!original_string) {
                    PV_ERROR_REPORT(
                            &pv_error_msg_alloc,
                            PV_ERROR_ARGS_PUBLIC_EMPTY(),
                            PV_ERROR_ARGS_PRIVATE("original_string"));
                    return PV_STATUS_OUT_OF_MEMORY;
                }
                strcpy(original_string, three_previous->original_string);
                strcat(original_string, token->original_string);

                char *collapsed_token_string = is_pm ? "PM" : "AM";
                pv_normalizer_token_t *collapsed_token = NULL;
                pv_status_t status = pv_normalizer_token_list_merge_tokens(
                        token_list,
                        collapsed_token_string,
                        original_string,
                        three_previous,
                        token,
                        &collapsed_token);
                free(original_string);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_token_list_merge_tokens,
                            pv_status_to_string(status));
                    return status;
                }

                pv_normalizer_token_t *previous = pv_normalizer_token_get_nth_token_before(token, 1, true);

                if (previous != NULL && previous->tag == PV_NORMALIZER_TAG_JA_CARDINAL) {
                    pv_normalizer_token_t *two_previous = pv_normalizer_token_get_nth_token_before(previous, 1, false);
                    if ((two_previous == NULL) ||
                        (two_previous != NULL && two_previous->tag == PV_NORMALIZER_TAG_JA_SPACE)) {
                        previous->tag = PV_NORMALIZER_TAG_JA_TIME_HOURS;
                    }
                }

                token = collapsed_token;
                token->tag = PV_NORMALIZER_TAG_JA_TIME_AM_PM;

                return PV_STATUS_SUCCESS;
            }
        }
    }

    // TAG: "7:00am" (tokenized to [7, :, 00am])
    if (num_tokens_before > 1) {
        const char *string = token->string;
        bool can_be_minutes_and_am_pm = (strlen(string) == 4);
        if (can_be_minutes_and_am_pm) {
            bool has_colon_before_current = (strcmp(token->previous->string, ":") == 0);
            bool has_hour_before_colon =
                    (token->previous->previous->tag == PV_NORMALIZER_TAG_JA_CARDINAL) &&
                    !pv_normalizer_util_string_number_greater_than_int(token->previous->previous->string, 24);

            bool has_digits = isdigit(string[0]) && isdigit(string[1]);
            bool has_m = (string[3] == 'm') || (string[3] == 'M');
            bool is_pm = has_m && ((string[2] == 'p') || (string[2] == 'P'));
            bool is_am = has_m && ((string[2] == 'a') || (string[2] == 'A'));

            if (has_digits && (is_am || is_pm) && has_colon_before_current && has_hour_before_colon) {
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

                    pv_normalizer_token_t *am_pm_token = token->next;

                    token->tag = PV_NORMALIZER_TAG_JA_TIME_MINUTES;
                    am_pm_token->tag = PV_NORMALIZER_TAG_JA_TIME_AM_PM;
                    token->previous->tag = PV_NORMALIZER_TAG_JA_TIME_COLON;
                    token->previous->previous->tag = PV_NORMALIZER_TAG_JA_TIME_HOURS;

                    return PV_STATUS_SUCCESS;
                }
            }
        }
    }

    // TAG: " 7am ", " 12am " (tokenized to [9am])
    if ((strlen(token->string) >= 3) && pv_normalizer_token_is_space_or_null(token->previous)) {
        const char *string = token->string;

        bool is_pm = false;
        bool found_time = false;
        if (strlen(token->string) == 3) {
            bool has_digits = isdigit(string[0]);
            bool has_m = (string[2] == 'm') || (string[2] == 'M');
            is_pm = has_m && ((string[1] == 'p') || (string[1] == 'P'));
            bool is_am = has_m && ((string[1] == 'a') || (string[1] == 'A'));
            if (has_digits && (is_am || is_pm)) {
                pv_status_t status = pv_normalizer_token_list_unroll_token(1, token, token_list);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_token_list_unroll_token,
                            pv_status_to_string(status));
                    return status;
                }
                found_time = true;
            }
        } else if (strlen(token->string) == 4) {
            bool has_digits = isdigit(string[0]) && isdigit(string[1]);
            bool has_m = (string[3] == 'm') || (string[3] == 'M');
            is_pm = has_m && ((string[2] == 'p') || (string[2] == 'P'));
            bool is_am = has_m && ((string[2] == 'a') || (string[2] == 'A'));
            if (has_digits && (is_am || is_pm)) {
                char hour_string[3] = {0};
                hour_string[0] = string[0];
                hour_string[1] = string[1];

                if (!pv_normalizer_util_string_number_greater_than_int(hour_string, 12)) {
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
            token->tag = PV_NORMALIZER_TAG_JA_TIME_HOURS;
            pv_normalizer_token_t *am_pm_token = token->next;
            am_pm_token->tag = PV_NORMALIZER_TAG_JA_TIME_AM_PM;

            char *am_pm_string = is_pm ? "PM" : "AM";
            free(am_pm_token->string);
            am_pm_token->string = calloc(strlen(am_pm_string) + 1, sizeof(char));
            if (!am_pm_token->string) {
                PV_ERROR_REPORT(
                        &pv_error_msg_alloc,
                        PV_ERROR_ARGS_PUBLIC_EMPTY(),
                        PV_ERROR_ARGS_PRIVATE("am_pm_token->string"));
                return PV_STATUS_OUT_OF_MEMORY;
            }
            strcpy(am_pm_token->string, am_pm_string);

            return PV_STATUS_SUCCESS;
        }
    }

    // TAG: "7:00a.m." (tokenized to [7, :, 00a, ., m, .])
    if ((num_tokens_before > 4) && is_dot) {
        pv_normalizer_token_t *three_previous = pv_normalizer_token_get_nth_token_before(token, 3, false);
        const char *string = three_previous->string;
        bool no_spaces =
                !pv_normalizer_token_is_space_or_null(pv_normalizer_token_get_nth_token_before(token, 1, false)) &&
                !pv_normalizer_token_is_space_or_null(pv_normalizer_token_get_nth_token_before(token, 2, false)) &&
                !pv_normalizer_token_is_space_or_null(pv_normalizer_token_get_nth_token_before(token, 3, false)) &&
                !pv_normalizer_token_is_space_or_null(pv_normalizer_token_get_nth_token_before(token, 4, false)) &&
                !pv_normalizer_token_is_space_or_null(pv_normalizer_token_get_nth_token_before(token, 5, false));

        bool three_previous_can_be_minutes_and_am_pm = (strlen(three_previous->string) == 3);
        bool previous_is_m =
                (strcmp(token->previous->string, "m") == 0) || (strcmp(token->previous->string, "M") == 0);
        bool two_previous_is_dot = (strcmp(token->previous->previous->string, ".") == 0);
        if (three_previous_can_be_minutes_and_am_pm && previous_is_m && two_previous_is_dot && no_spaces) {
            bool has_digits =
                    isdigit(string[0]) &&
                    isdigit(string[1]);
            bool is_pm = ((string[2] == 'p') || (string[2] == 'P'));
            bool is_am = ((string[2] == 'a') || (string[2] == 'A'));

            if (has_digits && (is_am || is_pm)) {
                pv_normalizer_token_t *four_previous = pv_normalizer_token_get_nth_token_before(token, 4, false);

                bool has_hour_four_before_colon =
                        (strcmp(four_previous->string, ":") == 0) &&
                        (four_previous->previous->tag == PV_NORMALIZER_TAG_JA_CARDINAL) &&
                        !pv_normalizer_util_string_number_greater_than_int(four_previous->previous->string, 12);
                if (has_hour_four_before_colon) {
                    char minute_string[3] = {0};
                    minute_string[0] = string[0];
                    minute_string[1] = string[1];

                    if (!pv_normalizer_util_string_number_greater_than_int(minute_string, 59)) {
                        pv_status_t status = pv_normalizer_token_list_unroll_token(
                                2,
                                three_previous,
                                token_list);
                        if (status != PV_STATUS_SUCCESS) {
                            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                    pv_normalizer_token_list_unroll_token,
                                    pv_status_to_string(status));
                            return status;
                        }

                        pv_normalizer_token_t *am_pm_token = three_previous->next;
                        pv_normalizer_token_t *minute_token = three_previous;
                        pv_normalizer_token_t *time_colon_token = three_previous->previous;
                        pv_normalizer_token_t *hour_token = three_previous->previous->previous;
                        minute_token->tag = PV_NORMALIZER_TAG_JA_TIME_MINUTES;
                        am_pm_token->tag = PV_NORMALIZER_TAG_JA_TIME_AM_PM;
                        time_colon_token->tag = PV_NORMALIZER_TAG_JA_TIME_COLON;
                        hour_token->tag = PV_NORMALIZER_TAG_JA_TIME_HOURS;

                        char *original_string = calloc(strlen(am_pm_token->original_string) + strlen(token->original_string) + 1, sizeof(char));
                        if (!original_string) {
                            PV_ERROR_REPORT(
                                    &pv_error_msg_alloc,
                                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                                    PV_ERROR_ARGS_PRIVATE("original_string"));
                            return PV_STATUS_OUT_OF_MEMORY;
                        }
                        strcpy(original_string, am_pm_token->original_string);
                        strcat(original_string, token->original_string);


                        char *collapsed_token_string = is_pm ? "PM" : "AM";
                        pv_normalizer_token_t *collapsed_token = NULL;
                        status = pv_normalizer_token_list_merge_tokens(
                                token_list,
                                collapsed_token_string,
                                original_string,
                                am_pm_token,
                                token,
                                &collapsed_token);
                        if (status != PV_STATUS_SUCCESS) {
                            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                    pv_normalizer_token_list_merge_tokens,
                                    pv_status_to_string(status));
                            free(original_string);
                            return status;
                        }

                        free(minute_token->original_string);
                        minute_token->original_string = calloc(strlen(original_string) + 1, sizeof(char));
                        if (!minute_token->original_string) {
                            PV_ERROR_REPORT(
                                    &pv_error_msg_alloc,
                                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                                    PV_ERROR_ARGS_PRIVATE("minute_token->original_string"));
                            free(original_string);
                            return PV_STATUS_OUT_OF_MEMORY;
                        }
                        strcpy(minute_token->original_string, original_string);
                        minute_token->length_future_context -= 2;

                        free(time_colon_token->original_string);
                        time_colon_token->original_string = calloc(strlen(original_string) + 1, sizeof(char));
                        if (!time_colon_token->original_string) {
                            PV_ERROR_REPORT(
                                    &pv_error_msg_alloc,
                                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                                    PV_ERROR_ARGS_PRIVATE("minute_token->original_string"));
                            free(original_string);
                            return PV_STATUS_OUT_OF_MEMORY;
                        }
                        strcpy(time_colon_token->original_string, original_string);
                        time_colon_token->length_future_context -= 2;

                        free(hour_token->original_string);
                        hour_token->original_string = calloc(strlen(original_string) + 1, sizeof(char));
                        if (!hour_token->original_string) {
                            PV_ERROR_REPORT(
                                    &pv_error_msg_alloc,
                                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                                    PV_ERROR_ARGS_PRIVATE("minute_token->original_string"));
                            free(original_string);
                            return PV_STATUS_OUT_OF_MEMORY;
                        }
                        strcpy(hour_token->original_string, original_string);
                        free(original_string);
                        hour_token->length_future_context -= 2;

                        token = collapsed_token;
                        token->tag = PV_NORMALIZER_TAG_JA_TIME_AM_PM;
                        token->length_past_context += 3;

                        return PV_STATUS_SUCCESS;
                    }
                }
            }
        }
    }

    // TAG: "7a.m." (tokenized to [7a, ., m, .])
    if ((num_tokens_before > 2) && is_dot) {
        pv_normalizer_token_t *three_previous = pv_normalizer_token_get_nth_token_before(token, 3, false);
        const char *string = three_previous->string;
        bool no_spaces =
                !pv_normalizer_token_is_space_or_null(pv_normalizer_token_get_nth_token_before(token, 1, false)) &&
                !pv_normalizer_token_is_space_or_null(pv_normalizer_token_get_nth_token_before(token, 2, false)) &&
                !pv_normalizer_token_is_space_or_null(pv_normalizer_token_get_nth_token_before(token, 3, false));

        bool three_previous_can_be_minutes_and_am_pm =
                (strlen(string) == 2) || (strlen(string) == 3);
        bool previous_is_m =
                (strcmp(token->previous->string, "m") == 0) || (strcmp(token->previous->string, "M") == 0);
        bool two_previous_is_dot = (strcmp(token->previous->previous->string, ".") == 0);
        if (three_previous_can_be_minutes_and_am_pm && previous_is_m && two_previous_is_dot && no_spaces) {
            bool has_digits = false;
            bool is_pm = false;
            bool is_am = false;
            if (strlen(string) == 2) {
                has_digits = isdigit(string[0]);
                is_pm = ((string[1] == 'p') || (string[1] == 'P'));
                is_am = ((string[1] == 'a') || (string[1] == 'A'));
            } else {
                has_digits = isdigit(string[0]) && isdigit(string[1]);
                is_pm = ((string[2] == 'p') || (string[2] == 'P'));
                is_am = ((string[2] == 'a') || (string[2] == 'A'));
            }

            if (has_digits && (is_am || is_pm)) {
                char hour_string[3] = {0};
                hour_string[0] = string[0];
                if (strlen(three_previous->string) == 3) {
                    hour_string[1] = string[1];
                }

                if (!pv_normalizer_util_string_number_greater_than_int(hour_string, 12)) {
                    pv_status_t status = pv_normalizer_token_list_unroll_token(
                            (strlen(three_previous->string) == 2) ? 1 : 2,
                            three_previous,
                            token_list);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_normalizer_token_list_unroll_token,
                                pv_status_to_string(status));
                        return status;
                    }

                    pv_normalizer_token_t *am_pm_token = three_previous->next;
                    three_previous->tag = PV_NORMALIZER_TAG_JA_TIME_HOURS;
                    am_pm_token->tag = PV_NORMALIZER_TAG_JA_TIME_AM_PM;

                    char *original_string = calloc(strlen(am_pm_token->original_string) + strlen(token->original_string) + 1, sizeof(char));
                    if (!original_string) {
                        PV_ERROR_REPORT(
                                &pv_error_msg_alloc,
                                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                                PV_ERROR_ARGS_PRIVATE("original_string"));
                        return PV_STATUS_OUT_OF_MEMORY;
                    }
                    strcpy(original_string, am_pm_token->original_string);
                    strcat(original_string, token->original_string);

                    char *collapsed_token_string = is_pm ? "PM" : "AM";
                    pv_normalizer_token_t *collapsed_token = NULL;
                    status = pv_normalizer_token_list_merge_tokens(
                            token_list,
                            collapsed_token_string,
                            original_string,
                            am_pm_token,
                            token,
                            &collapsed_token);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_normalizer_token_list_merge_tokens,
                                pv_status_to_string(status));
                        free(original_string);
                        return status;
                    }

                    free(three_previous->original_string);
                    three_previous->original_string = calloc(strlen(original_string) + 1, sizeof(char));
                    if (!three_previous->original_string) {
                        PV_ERROR_REPORT(
                                &pv_error_msg_alloc,
                                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                                PV_ERROR_ARGS_PRIVATE("minute_token->original_string"));
                        free(original_string);
                        return PV_STATUS_OUT_OF_MEMORY;
                    }
                    strcpy(three_previous->original_string, original_string);
                    free(original_string);
                    three_previous->length_future_context -= 2;

                    token = collapsed_token;
                    token->tag = PV_NORMALIZER_TAG_JA_TIME_AM_PM;
                    token->length_past_context += 1;

                    return PV_STATUS_SUCCESS;
                }
            }
        }
    }
    return PV_STATUS_SUCCESS;
}

pv_status_t pv_normalizer_tagger_tag_url(pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[PV_NORMALIZER_TAG_JA_DOT]) {
        const char *string = token->string;

        bool previous_is_non_space_character =
                (token->previous != NULL) &&
                (strcmp(token->previous->string, " ") != 0) &&
                (token->previous->tag != PV_NORMALIZER_TAG_JA_WORD) &&
                !token->previous->next_character_is_space;
        bool previous_is_not_dot =
                (token->previous != NULL) &&
                (strcmp(token->previous->string, ".") != 0);
        bool next_is_non_space_character =
                (token->next != NULL) &&
                (strcmp(token->next->string, " ") != 0) &&
                (token->next->tag != PV_NORMALIZER_TAG_JA_WORD) &&
                !token->next_character_is_space &&
                (strcmp(token->next->string, "\n") != 0) &&
                (strcmp(token->next->string, "\"") != 0);
        bool next_is_not_dot =
                (token->next != NULL) &&
                (strcmp(token->next->string, ".") != 0);

        if ((strcmp(string, ".") == 0) &&
            previous_is_non_space_character &&
            next_is_non_space_character &&
            previous_is_not_dot &&
            next_is_not_dot) {
            token->tag = PV_NORMALIZER_TAG_JA_DOT;
        }
    }

    if (PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[PV_NORMALIZER_TAG_JA_COLON]) {
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
            token->tag = PV_NORMALIZER_TAG_JA_COLON;
        }
    }

    if (PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[PV_NORMALIZER_TAG_JA_ACRONYM]) {
        char *upper_string = NULL;
        pv_status_t status = pv_normalizer_util_upper(token->string, &upper_string);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_util_upper,
                    pv_status_to_string(status));
            return status;
        }

        for (int32_t i = 0; i < PV_NORMALIZER_NUM_ACRONYMS_JA; i++) {
            if (strcmp(upper_string, PV_NORMALIZER_ACRONYMS_JA[i]) == 0) {
                token->tag = PV_NORMALIZER_TAG_JA_ACRONYM;
                break;
            }
        }

        free(upper_string);
    }

    if (PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[PV_NORMALIZER_TAG_JA_TOP_LEVEL_DOMAIN]) {
        if ((token->previous != NULL) &&
            !token->previous->next_character_is_space &&
            (token->previous->tag == PV_NORMALIZER_TAG_JA_DOT || strcmp(token->previous->string, ".") == 0)) {
            char *upper_string = NULL;
            pv_status_t status = pv_normalizer_util_upper(token->string, &upper_string);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_normalizer_util_upper,
                        pv_status_to_string(status));
                return status;
            }

            for (int32_t i = 0; i < PV_NORMALIZER_NUM_TOP_LEVEL_DOMAINS_JA; i++) {
                if (strcmp(upper_string, PV_NORMALIZER_TOP_LEVEL_DOMAINS_JA[i]) == 0) {
                    token->tag = PV_NORMALIZER_TAG_JA_TOP_LEVEL_DOMAIN;
                    token->tag_data_index = i;
                    if (token->previous->tag != PV_NORMALIZER_TAG_JA_DOT) {
                        token->previous->tag = PV_NORMALIZER_TAG_JA_DOT;
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
        const pv_normalizer_tagger_ja_t *object,
        pv_normalizer_token_t *token,
        pv_normalizer_token_list_t *token_list) {
    PV_ASSERT(object);
    PV_ASSERT(token);
    PV_ASSERT(token_list);

    if (PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[PV_NORMALIZER_TAG_JA_CURRENCY]) {
        const char *string = token->string;
        int32_t length = (int32_t) strlen(string);

        int32_t currency_index = -1;
        int32_t num_bytes_currency = 1;
        bool starts_with_currency = false;
        for (int32_t i = 0; i < PV_NORMALIZER_NUM_CURRENCIES_JA; i++) {
            num_bytes_currency = (int32_t) strlen(PV_NORMALIZER_CURRENCIES_JA[i]);
            if (strncmp(string, PV_NORMALIZER_CURRENCIES_JA[i], num_bytes_currency) == 0) {
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
                token->tag = PV_NORMALIZER_TAG_JA_CURRENCY;
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
                    for (int32_t j = 0; j < PV_NORMALIZER_NUM_CURRENCIES_JA; j++) {
                        num_bytes_currency = (int32_t) strlen(PV_NORMALIZER_CURRENCIES_JA[j]);
                        if (strncmp(string + i, PV_NORMALIZER_CURRENCIES_JA[j], num_bytes_currency) == 0) {
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
                        (strcmp(token->previous->string, ".") == 0) &&
                        (length_digits != 2);

                if (invalid_digits) {
                    return PV_STATUS_SUCCESS;
                }

                token->tag = PV_NORMALIZER_TAG_JA_CURRENCY;
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
                         (strcmp(token->next->string, ".") == 0) &&
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
                    is_before_punctuation = next_is_punctuation &&
                                            ((token->next->next_character_is_space) ||
                                             ((token->next->next != NULL) && (token->next->next->tag == PV_NORMALIZER_TAG_JA_WORD)) ||
                                             (token->next->next == NULL));
                }

                // Possible end of comma number e.g. `1,000$`
                bool is_before_word_or_parenthesis =
                        (token->next != NULL) &&
                        ((token->next->tag == PV_NORMALIZER_TAG_JA_WORD) || (strcmp(token->next->string, ")") == 0));
                if ((token->next_character_is_space ||
                     (token->next == NULL) ||
                     is_before_word_or_parenthesis ||
                     is_before_hyphen ||
                     is_before_decimal || is_before_punctuation) &&
                    ((strlen(string) - num_bytes_character) == 3)) {
                    pv_status_t status = pv_normalizer_tagger_tag_comma_number(token, token_list);
                    if (status != PV_STATUS_SUCCESS) {
                        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                pv_normalizer_tagger_tag_comma_number,
                                pv_status_to_string(status));
                        return status;
                    }
                    token->tag_data_index = currency_index;

                    return PV_STATUS_SUCCESS;
                }

                // `5.25$` - tokens: ['5', '.', '25$'] and -5.25$ - token: ['-5', '.', '25$']
                if ((token->previous != NULL) && token->previous->previous != NULL) {
                    pv_normalizer_token_t *previous = token->previous;
                    pv_normalizer_token_t *two_previous = token->previous->previous;

                    bool is_currency_decimal =
                            (strcmp(previous->string, ".") == 0) &&
                            (!previous->next_character_is_space) &&
                            ((two_previous->tag == PV_NORMALIZER_TAG_JA_CARDINAL) ||
                             (two_previous->tag == PV_NORMALIZER_TAG_JA_NEGATIVE_CARDINAL)) &&
                            (!two_previous->next_character_is_space);

                    if (is_currency_decimal) {
                        pv_normalizer_token_tag_ja_t two_previous_tag = two_previous->tag;

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

                        char *original_string = calloc(strlen(token->original_string) + 1, sizeof(char));
                        if (!original_string) {
                            PV_ERROR_REPORT(
                                    &pv_error_msg_alloc,
                                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                                    PV_ERROR_ARGS_PRIVATE("original_string"));
                            free(merged_string);
                            return PV_STATUS_OUT_OF_MEMORY;
                        }
                        strcpy(original_string, token->original_string);

                        pv_normalizer_token_t *merged_token = NULL;
                        pv_status_t status = pv_normalizer_token_list_merge_tokens(
                                token_list,
                                merged_string,
                                original_string,
                                two_previous,
                                token,
                                &merged_token);
                        free(merged_string);
                        free(original_string);
                        if (status != PV_STATUS_SUCCESS) {
                            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                                    pv_normalizer_token_list_merge_tokens,
                                    pv_status_to_string(status));
                            return status;
                        }

                        if (two_previous_tag == PV_NORMALIZER_TAG_JA_CARDINAL) {
                            merged_token->tag = PV_NORMALIZER_TAG_JA_CURRENCY;
                        } else {
                            merged_token->tag = PV_NORMALIZER_TAG_JA_NEGATIVE_CURRENCY;
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

        // `$5.25` - tokens: ['$5', '.', '25']
        if (only_digits && (token->previous != NULL) && (token->previous->previous != NULL)) {
            pv_normalizer_token_t *previous = token->previous;
            pv_normalizer_token_t *two_previous = token->previous->previous;

            pv_normalizer_token_tag_ja_t two_previous_tag = two_previous->tag;
            int32_t two_previous_currency_index = two_previous->tag_data_index;

            bool is_currency_decimal =
                    (strcmp(previous->string, ".") == 0) &&
                    (!previous->next_character_is_space) &&
                    ((two_previous->tag == PV_NORMALIZER_TAG_JA_CURRENCY) ||
                     (two_previous->tag == PV_NORMALIZER_TAG_JA_NEGATIVE_CURRENCY)) &&
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

                char *original_string = calloc(strlen(token->original_string) + 1, sizeof(char));
                if (!original_string) {
                    PV_ERROR_REPORT(
                            &pv_error_msg_alloc,
                            PV_ERROR_ARGS_PUBLIC_EMPTY(),
                            PV_ERROR_ARGS_PRIVATE("original_string"));
                    free(merged_string);
                    return PV_STATUS_OUT_OF_MEMORY;
                }
                strcpy(original_string, token->original_string);

                pv_normalizer_token_t *merged_token = NULL;
                pv_status_t status = pv_normalizer_token_list_merge_tokens(
                        token_list,
                        merged_string,
                        original_string,
                        two_previous,
                        token,
                        &merged_token);
                free(merged_string);
                free(original_string);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_token_list_merge_tokens,
                            pv_status_to_string(status));
                    return status;
                }

                merged_token->tag = two_previous_tag;
                merged_token->tag_data_index = two_previous_currency_index;
                return PV_STATUS_SUCCESS;
            }
        }

        if (string[0] == '-') {
            bool second_character_is_currency = false;
            for (int32_t i = 0; i < PV_NORMALIZER_NUM_CURRENCIES_JA; i++) {
                num_bytes_currency = (int32_t) strlen(PV_NORMALIZER_CURRENCIES_JA[i]);
                if (strncmp(string + 1, PV_NORMALIZER_CURRENCIES_JA[i], num_bytes_currency) == 0) {
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
                    token->tag = PV_NORMALIZER_TAG_JA_NEGATIVE_CURRENCY;
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
                        for (int32_t j = 0; j < PV_NORMALIZER_NUM_CURRENCIES_JA; j++) {
                            num_bytes_currency = (int32_t) strlen(PV_NORMALIZER_CURRENCIES_JA[j]);
                            if (strncmp(string + i, PV_NORMALIZER_CURRENCIES_JA[j], num_bytes_currency) == 0) {
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
                    token->tag = PV_NORMALIZER_TAG_JA_NEGATIVE_CURRENCY;
                    token->tag_data_index = currency_index;
                    return PV_STATUS_SUCCESS;
                }
            }
        }

        bool is_currency = false;
        for (int32_t i = 0; i < PV_NORMALIZER_NUM_CURRENCIES_JA; i++) {
            if (strcmp(string, PV_NORMALIZER_CURRENCIES_JA[i]) == 0) {
                currency_index = i;
                is_currency = true;
                break;
            }
        }

        pv_normalizer_token_t *previous = pv_normalizer_token_get_nth_token_before(token, 1, true);
        bool previous_is_cardinal =
                (previous != NULL) &&
                ((previous->tag == PV_NORMALIZER_TAG_JA_CARDINAL) ||
                 (previous->tag == PV_NORMALIZER_TAG_JA_NEGATIVE_CARDINAL));
        bool previous_is_decimal =
                (previous != NULL) &&
                (previous->tag == PV_NORMALIZER_TAG_JA_DECIMAL_DIGITS);

        // `1 $` - tokens: ['1', ' ', '$']
        if (is_currency && (previous_is_cardinal || previous_is_decimal)) {
            token->tag = PV_NORMALIZER_TAG_JA_CURRENCY_SYMBOL;
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

    if (PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[PV_NORMALIZER_TAG_JA_ABBREVIATION]) {
        bool is_dot = (strcmp(token->string, ".") == 0);
        if (is_dot) {
            pv_normalizer_token_t *start_token = token;
            while ((start_token->previous != NULL) &&
                   ((start_token->previous->tag == PV_NORMALIZER_TAG_JA_ALPHANUM_SPELL_OUT) ||
                    (start_token->previous->tag == PV_NORMALIZER_TAG_JA_DOT))) {
                start_token = start_token->previous;
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

            for (int32_t i = 0; i < PV_NORMALIZER_NUM_ABBREVIATIONS_JA; i++) {
                if (strcmp(potential_abbreviation, PV_NORMALIZER_ABBREVIATIONS_JA[i]) == 0) {
                    token->tag = PV_NORMALIZER_TAG_JA_ABBREVIATION;
                    token->tag_data_index = i;
                    break;
                }
            }

            if (token->tag == PV_NORMALIZER_TAG_JA_ABBREVIATION) {
                char *abbreviation_original_string = NULL;
                if (token->next == NULL || token->next_character_is_space) {
                    abbreviation_original_string = calloc(strlen(start_token->original_string) +
                                                                  strlen(token->original_string) + 1,
                                                          sizeof(char));
                    if (!abbreviation_original_string) {
                        PV_ERROR_REPORT(
                                &pv_error_msg_alloc,
                                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                                PV_ERROR_ARGS_PRIVATE("abbreviation_original_string"));
                        return PV_STATUS_OUT_OF_MEMORY;
                    }
                    strcpy(abbreviation_original_string, start_token->original_string);
                    strcat(abbreviation_original_string, token->original_string);
                } else {
                    abbreviation_original_string = calloc(strlen(token->original_string) + 1, sizeof(char));
                    if (!abbreviation_original_string) {
                        PV_ERROR_REPORT(
                                &pv_error_msg_alloc,
                                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                                PV_ERROR_ARGS_PRIVATE("abbreviation_original_string"));
                        return PV_STATUS_OUT_OF_MEMORY;
                    }
                    strcpy(abbreviation_original_string, token->original_string);
                }

                pv_normalizer_token_t *collapsed_token = NULL;
                status = pv_normalizer_token_list_merge_tokens(
                        token_list,
                        potential_abbreviation,
                        abbreviation_original_string,
                        start_token,
                        token,
                        &collapsed_token);
                free(potential_abbreviation);
                potential_abbreviation = NULL;
                free(abbreviation_original_string);
                abbreviation_original_string = NULL;
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_token_list_merge_tokens,
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


static bool pv_normalizer_tagger_is_date_separator(const char *string) {
    PV_ASSERT(string);

    return (strcmp(string, "-") == 0) ||
           (strcmp(string, "/") == 0) ||
           (strcmp(string, ".") == 0);
}

static bool pv_normalizer_tagger_is_date_separator_character(char character) {
    PV_ASSERT(character);

    return character == '-' || character == '/' || character == '.';
}

static int32_t pv_normalizer_tagger_get_month_index(const char *string) {
    PV_ASSERT(string);

    for (int32_t i = 0; i < PV_NORMALIZER_NUM_MONTH_NAMES_JA; i++) {
        if (strcmp(string, PV_NORMALIZER_MONTH_NAMES_JA[i]) == 0) {
            return i;
        }
    }

    return -1;
}

static int32_t pv_normalizer_tagger_get_day_index(const char *string) {
    PV_ASSERT(string);

    for (int32_t i = 0; i < PV_NORMALIZER_NUM_DAY_NAMES_JA; i++) {
        if (strcmp(string, PV_NORMALIZER_DAY_NAMES_JA[i]) == 0) {
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

    if (PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[PV_NORMALIZER_TAG_JA_DATE_MONTH]) {

        if (strcmp(token->original_string, "月") == 0) {
            pv_normalizer_token_t *previous = pv_normalizer_token_get_nth_token_before(token, 1, true);
            if ((previous != NULL) &&
                (previous->tag == PV_NORMALIZER_TAG_JA_CARDINAL)) {
                int32_t month_idx = pv_normalizer_tagger_get_month_index(previous->string);
                if (month_idx >= 0) {
                    token->tag = PV_NORMALIZER_TAG_JA_DATE_MONTH;
                    previous->tag = PV_NORMALIZER_TAG_JA_DATE_MONTH;
                    previous->tag_data_index = month_idx;
                }
            }
        }

        const char *string = token->string;
        int32_t length = (int32_t) strlen(string);

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

        if (is_valid_date && (num_separators == 2) && is_valid_num_digits) {
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
            token->tag = PV_NORMALIZER_TAG_JA_CARDINAL;
            token->length_future_context = token_list_date->size - 1;
            token->length_past_context = 0;

            pv_normalizer_token_t *previous = token;
            pv_normalizer_token_t *current = token_list_date->head->next;
            int32_t length_past_context = 1;
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

                current->length_future_context = token_list_date->size - length_past_context - 1;
                current->length_past_context = length_past_context;
                length_past_context += 1;

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
            bool previous_is_date_separator = pv_normalizer_tagger_is_date_separator(previous->string);
            bool two_previous_is_day =
                    pv_normalizer_util_only_contains_digits(two_previous->string) &&
                    !pv_normalizer_util_string_number_greater_than_int(two_previous->string, 31);
            bool three_previous_is_date_separator =
                    pv_normalizer_tagger_is_date_separator(three_previous->string) &&
                    (strcmp(three_previous->string, previous->string) == 0);
            bool four_previous_is_month =
                    pv_normalizer_util_only_contains_digits(four_previous->string) &&
                    !pv_normalizer_util_string_number_greater_than_int(four_previous->string, 12);

            // Fallback to DAY-MONTH-YEAR format
            bool four_previous_is_day =
                    pv_normalizer_util_only_contains_digits(four_previous->string) &&
                    pv_normalizer_util_string_number_greater_than_int(four_previous->string, 12) &&
                    !pv_normalizer_util_string_number_greater_than_int(four_previous->string, 31);
            bool two_previous_is_month = false;
            if (four_previous_is_day) {
                two_previous_is_month =
                        pv_normalizer_util_only_contains_digits(two_previous->string) &&
                        !pv_normalizer_util_string_number_greater_than_int(two_previous->string, 12);
            }

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
                token->tag = PV_NORMALIZER_TAG_JA_DATE_YEAR;
                previous->tag = PV_NORMALIZER_TAG_JA_DATE_SEPARATOR;
                three_previous->tag = PV_NORMALIZER_TAG_JA_DATE_SEPARATOR;

                if (two_previous_is_month && four_previous_is_day) {
                    two_previous->tag = PV_NORMALIZER_TAG_JA_DATE_MONTH;
                    two_previous->tag_data_index = pv_normalizer_tagger_get_month_index(two_previous->string);
                    four_previous->tag = PV_NORMALIZER_TAG_JA_DATE_DAY;
                    four_previous->tag_data_index = pv_normalizer_tagger_get_day_index(four_previous->string);
                } else {
                    two_previous->tag = PV_NORMALIZER_TAG_JA_DATE_DAY;
                    two_previous->tag_data_index = pv_normalizer_tagger_get_day_index(two_previous->string);
                    four_previous->tag = PV_NORMALIZER_TAG_JA_DATE_MONTH;
                    four_previous->tag_data_index = pv_normalizer_tagger_get_month_index(four_previous->string);
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
            bool valid_space = !(three_previous->next_character_is_space ^ previous->next_character_is_space);
            bool is_date =
                    three_previous_is_date_separator &&
                    two_previous_is_month &&
                    previous_is_date_separator &&
                    current_is_day &&
                    valid_space;
            if (is_date) {
                four_previous->tag = PV_NORMALIZER_TAG_JA_DATE_YEAR;
                three_previous->tag = PV_NORMALIZER_TAG_JA_DATE_SEPARATOR;
                two_previous->tag = PV_NORMALIZER_TAG_JA_DATE_MONTH;
                two_previous->tag_data_index = pv_normalizer_tagger_get_month_index(two_previous->string);
                previous->tag = PV_NORMALIZER_TAG_JA_DATE_SEPARATOR;
                token->tag = PV_NORMALIZER_TAG_JA_DATE_DAY;
                token->tag_data_index = pv_normalizer_tagger_get_day_index(token->string);

                if (previous->next_character_is_space && three_previous->next_character_is_space) {
                    size_t original_length = strlen(four_previous->string) +
                                             strlen(three_previous->string) +
                                             strlen(two_previous->string) +
                                             strlen(previous->string) +
                                             strlen(token->string) + 4;
                    char *original_string = calloc(original_length, sizeof(char));
                    if (!original_string) {
                        PV_ERROR_REPORT(
                                &pv_error_msg_alloc,
                                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                                PV_ERROR_ARGS_PRIVATE("original_string"));
                        return PV_STATUS_OUT_OF_MEMORY;
                    }
                    strcpy(original_string, four_previous->original_string);
                    strcat(original_string, three_previous->original_string);
                    strcat(original_string, three_previous->next->original_string);
                    strcat(original_string, two_previous->original_string);
                    strcat(original_string, previous->original_string);
                    strcat(original_string, previous->next->original_string);
                    strcat(original_string, token->original_string);

                    pv_normalizer_token_t *tokens[] = {
                            four_previous,
                            three_previous,
                            three_previous->next,
                            two_previous,
                            previous,
                            previous->next,
                            token};
                    int32_t num_tokens = PV_ARRAY_LEN(tokens);

                    for (int32_t j = 0; j < num_tokens; j++) {
                        char *token_string = calloc(original_length, sizeof(char));
                        if (!token_string) {
                            PV_ERROR_REPORT(
                                    &pv_error_msg_alloc,
                                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                                    PV_ERROR_ARGS_PRIVATE("token_string"));
                            return PV_STATUS_OUT_OF_MEMORY;
                        }

                        free(tokens[j]->original_string);
                        strcpy(token_string, original_string);
                        tokens[j]->original_string = token_string;

                        tokens[j]->length_future_context = num_tokens - 1 - j;
                        tokens[j]->length_past_context = j;
                    }
                }
                return PV_STATUS_SUCCESS;
            }
        }
    }

    if (PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[PV_NORMALIZER_TAG_JA_DATE_YEAR]) {
        if (strcmp(token->original_string, "年") == 0) {
            pv_normalizer_token_t *previous = pv_normalizer_token_get_nth_token_before(token, 1, true);
            if ((previous != NULL) &&
                (previous->tag == PV_NORMALIZER_TAG_JA_CARDINAL)) {
                const char *string = previous->string;
                int32_t length = (int32_t) strlen(string);
                bool current_can_be_year =
                        pv_normalizer_util_only_contains_digits(string) &&
                        (length == 4) && (string[0] != '0');

                if (current_can_be_year) {
                    token->tag = PV_NORMALIZER_TAG_JA_DATE_YEAR;
                    previous->tag = PV_NORMALIZER_TAG_JA_DATE_YEAR;
                }
            }
        }
    }

    if (PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[PV_NORMALIZER_TAG_JA_DATE_DAY]) {
        if (strcmp(token->original_string, "日") == 0) {
            pv_normalizer_token_t *previous = pv_normalizer_token_get_nth_token_before(token, 1, true);
            if ((previous != NULL) &&
                (previous->tag == PV_NORMALIZER_TAG_JA_CARDINAL)) {
                int32_t day_idx = pv_normalizer_tagger_get_day_index(previous->string);
                if (day_idx >= 0) {
                    token->tag = PV_NORMALIZER_TAG_JA_DATE_DAY;
                    previous->tag = PV_NORMALIZER_TAG_JA_DATE_DAY;
                    previous->tag_data_index = day_idx;
                }
            }
        }
    }

    return PV_STATUS_SUCCESS;
}

static bool pv_normalizer_tagger_is_digit_separator(char string) {
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

    if (PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[PV_NORMALIZER_TAG_JA_DIGITS_WITH_PARENTHESES]) {

        // TAG: `(XXX)` e.g. in phone number `(XXX) XXX-XXXX` tokenized to [(XXX), XXX-XXXX]
        if ((length == 5) && (string[0] == '(') && (string[4] == ')')) {
            if (isdigit(string[1]) && isdigit(string[2]) && isdigit(string[3])) {
                token->tag = PV_NORMALIZER_TAG_JA_DIGITS_WITH_PARENTHESES;
            }
        }
    }

    if (PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[PV_NORMALIZER_TAG_JA_DIGITS]) {

        // TAG: `+1`
        pv_normalizer_token_t *previous_no_skip_space = pv_normalizer_token_get_nth_token_before(token, 1, false);
        if ((previous_no_skip_space != NULL) && (strcmp(previous_no_skip_space->string, "+") == 0)) {
            if (pv_normalizer_util_only_contains_digits(string)) {
                token->tag = PV_NORMALIZER_TAG_JA_DIGITS;
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
            token->tag = PV_NORMALIZER_TAG_JA_DIGITS;
            token->length_future_context = token_list_digits->size - 1;
            token->length_past_context = 0;

            pv_normalizer_token_t *previous = token;
            pv_normalizer_token_t *current = head->next;
            int32_t length_past_context = 1;
            while (current != NULL) {
                pv_normalizer_token_t *next = current->next;

                if ((strlen(current->string) == 1) && pv_normalizer_tagger_is_digit_separator(current->string[0])) {
                    current->tag = PV_NORMALIZER_TAG_JA_DIGITS_SEPARATOR;
                } else {
                    current->tag = PV_NORMALIZER_TAG_JA_DIGITS;
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
                (previous_skip_space->tag == PV_NORMALIZER_TAG_JA_DIGITS_WITH_PARENTHESES);
        pv_normalizer_token_t *two_previous = pv_normalizer_token_get_nth_token_before(token, 2, false);
        pv_normalizer_token_t *four_previous = pv_normalizer_token_get_nth_token_before(token, 4, false);
        bool is_third_number_in_digits_sequence =
                (four_previous != NULL) &&
                (four_previous->tag == PV_NORMALIZER_TAG_JA_DIGITS_WITH_PARENTHESES) &&
                (two_previous->tag == PV_NORMALIZER_TAG_JA_DIGITS);

        bool is_part_of_digits_sequence =
                (is_second_number_in_digits_sequence || is_third_number_in_digits_sequence) &&
                !has_double_dash &&
                only_digits_and_dashes;
        if (is_part_of_digits_sequence) {
            pv_normalizer_token_t *current = token;
            while (current != NULL) {
                if (pv_normalizer_util_only_contains_digits(current->string)) {
                    current->tag = PV_NORMALIZER_TAG_JA_DIGITS;
                } else if (strcmp(current->string, "-") == 0) {
                    current->tag = PV_NORMALIZER_TAG_JA_DIGITS_SEPARATOR;
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
                if ((current->tag == PV_NORMALIZER_TAG_JA_CARDINAL) ||
                    pv_normalizer_util_only_contains_digits(current->string)) {
                    current->tag = PV_NORMALIZER_TAG_JA_DIGITS;
                } else if (strcmp(current->string, ".") == 0) {
                    current->tag = PV_NORMALIZER_TAG_JA_DIGITS_SEPARATOR;
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

pv_status_t pv_normalizer_tagger_tag_name(
        pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    if (PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[token->tag] <
        PV_NORMALIZER_TOKEN_TAG_JA_WEIGHTS[PV_NORMALIZER_TAG_JA_NAME_INITIAL_LETTER]) {
        const char *string = token->string;

        bool is_dot = (strcmp(string, ".") == 0);
        if (is_dot) {
            pv_normalizer_token_t *previous_token = pv_normalizer_token_get_nth_token_before(token, 1, false);
            if (previous_token != NULL) {
                bool is_capitalized = false;
                pv_status_t status = pv_normalizer_util_ja_is_capitalized_word(
                        previous_token->string,
                        &is_capitalized);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_normalizer_util_ja_is_capitalized_word,
                            pv_status_to_string(status));
                    return status;
                }
                pv_normalizer_token_t *two_previous = pv_normalizer_token_get_nth_token_before(token, 2, false);
                if (is_capitalized && (strlen(previous_token->string) == 1) &&
                    (two_previous == NULL || two_previous->tag != PV_NORMALIZER_TAG_JA_LETTER_SPELL_OUT)) {
                    previous_token->tag = PV_NORMALIZER_TAG_JA_NAME_INITIAL_LETTER;
                    token->tag = PV_NORMALIZER_TAG_JA_NAME_INITIAL_DOT;
                }
            }
        }
    }

    return PV_STATUS_SUCCESS;
}
