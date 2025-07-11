#include <ctype.h>
#include <string.h>

#include "core/pv_assert.h"
#include "core/pv_error_messages.h"
#include "orca/normalizer/pv_normalizer_token.h"
#include "orca/normalizer/pv_normalizer_use_cases.h"
#include "orca/normalizer/pv_normalizer_util.h"

#include "orca/normalizer/it/pv_normalizer_stream_context_scanner_it.h"
#include "orca/normalizer/it/pv_normalizer_tags_it.h"
#include "orca/normalizer/it/pv_normalizer_util_it.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_normalizer_mock.h"

#endif

static bool pv_normalizer_stream_context_can_be_phone_number(const pv_normalizer_token_t *token);

static bool pv_normalizer_stream_context_can_be_fraction_or_per_slash(const pv_normalizer_token_t *token);

static bool pv_normalizer_stream_context_is_punctuation_part_of_time(const pv_normalizer_token_t *token);

static bool pv_normalizer_stream_context_is_punctuation_part_of_cardinal(const pv_normalizer_token_t *token);

static bool pv_normalizer_stream_context_is_punctuation_part_of_decimal(const pv_normalizer_token_t *token);

static bool pv_normalizer_stream_context_is_punctuation_part_of_url_or_abbreviation(const pv_normalizer_token_t *token);

bool PV_MOCKABLE(pv_normalizer_stream_context_scanner_it_is_verbalizable)(
        const pv_normalizer_token_t *token,
        int32_t num_use_cases,
        const pv_normalizer_use_cases_it_t *use_cases) {
    PV_ASSERT(token);
    PV_ASSERT(num_use_cases >= 0);
    PV_ASSERT(use_cases);

    if (token->tag == PV_NORMALIZER_TAG_IT_SPECIAL_CHARACTER) {
        for (int32_t j = 0; j < num_use_cases; j++) {
            if (use_cases[j] == PV_NORMALIZER_USE_DIGITS_SEQUENCE_NORMALIZER_IT) {
                if (pv_normalizer_stream_context_can_be_phone_number(token)) {
                    return false;
                }
            } else if (use_cases[j] == PV_NORMALIZER_USE_FRACTION_NORMALIZER_IT) {
                if (pv_normalizer_stream_context_can_be_fraction_or_per_slash(token)) {
                    return false;
                }
            }
        }

        return true;
    }

    if (token->tag == PV_NORMALIZER_TAG_IT_CURRENCY_SYMBOL) {
        // If it is "1 $" then conceptually can return, but now we are verbalizing stand-alone currency symbol without cardinal in front of it, hence can't be verbalizable immediately because the token sequence might be "$"->"1".
        return false;
    }

    if (token->tag == PV_NORMALIZER_TAG_IT_CUSTOM_PRONUNCIATION) {
        return true;
    }

    if (token->tag == PV_NORMALIZER_TAG_IT_ABBREVIATION) {
        return true;
    }

    if (token->next != NULL) {
        if (token->next_character_is_space) {
            return true;
        }
    }

    if (token->tag == PV_NORMALIZER_TAG_IT_SPACE) {
        // TODO (Ted): It's impossible to use this streaming interface to both achieve correct word alignment and verbalize the string "1 1 1 ..." in a streaming fashion (for other languages might be "1st 2nd 3rd ..." or "1 2 3 4 ..."). After an offline discussion, everyone else suggests to put this into tracking and accept the situation where we have to wait till the end to verbalize everything in the example string.
        if (pv_normalizer_util_it_found_previous_one(token) || pv_normalizer_util_it_found_previous_negative_one(token)) {
            return false;
        }

        if ((token->previous) && (token->previous->tag == PV_NORMALIZER_TAG_IT_MEASUREMENT)) {
            return false;
        }

        return true;
    }

    bool is_verbalizable = false;

    if (token->tag == PV_NORMALIZER_TAG_IT_PUNCTUATION) {
        is_verbalizable = true;

        for (int32_t j = 0; j < num_use_cases; j++) {
            if (
                    (use_cases[j] == PV_NORMALIZER_USE_URL_NORMALIZER_IT) ||
                    use_cases[j] == PV_NORMALIZER_USE_ABBREVIATION_NORMALIZER_IT) {
                if (pv_normalizer_stream_context_is_punctuation_part_of_url_or_abbreviation(token)) {
                    return false;
                }
            } else if (use_cases[j] == PV_NORMALIZER_USE_TIME_NORMALIZER_IT) {
                if (pv_normalizer_stream_context_is_punctuation_part_of_time(token)) {
                    return false;
                }
            } else if (
                    (use_cases[j] == PV_NORMALIZER_USE_CARDINAL_NORMALIZER_IT) ||
                    use_cases[j] == PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_IT) {
                if (pv_normalizer_stream_context_is_punctuation_part_of_cardinal(token)) {
                    return false;
                }
            } else if (use_cases[j] == PV_NORMALIZER_USE_DECIMAL_NORMALIZER_IT) {
                if (pv_normalizer_stream_context_is_punctuation_part_of_decimal(token)) {
                    return false;
                }
            }
        }
    }

    return is_verbalizable;
}

void PV_MOCKABLE(pv_normalizer_stream_context_scanner_it_remove_hyphen_only_tokens)(
        pv_normalizer_token_list_t *token_list) {
    PV_ASSERT(token_list);

    pv_normalizer_token_t *current = token_list->head;
    while (current) {
        pv_normalizer_token_t *next = current->next;

        if (current->tag != PV_NORMALIZER_TAG_IT_NUMBER_RANGE_TO) {
            bool only_dashes = false;
            for (int32_t i = 0; i < (int32_t) strlen(current->string); i++) {
                if (current->string[i] != '-') {
                    only_dashes = false;
                    break;
                } else {
                    only_dashes = true;
                }
            }
            if (only_dashes) {
                pv_normalizer_token_list_remove_token(token_list, current);
            }
        }

        current = next;
    }
}

bool pv_normalizer_stream_context_is_punctuation_part_of_time(const pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    bool can_be_time =
            (strcmp(token->string, ":") == 0) &&
            (token->previous != NULL) &&
            (token->previous->tag == PV_NORMALIZER_TAG_IT_CARDINAL ||
             token->previous->tag == PV_NORMALIZER_TAG_IT_CARDINAL_ONE) &&
            !pv_normalizer_util_string_number_greater_than_int(token->previous->string, 24);
    if (can_be_time) {
        return true;
    }

    return false;
}

bool pv_normalizer_stream_context_is_punctuation_part_of_cardinal(const pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    return (strcmp(token->string, ".") == 0) &&
           ((token->previous != NULL) &&
            ((token->previous->tag == PV_NORMALIZER_TAG_IT_CARDINAL) ||
             (token->previous->tag == PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL) ||
             (token->previous->tag == PV_NORMALIZER_TAG_IT_CARDINAL_ONE) ||
             (token->previous->tag == PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL_ONE)));
}

bool pv_normalizer_stream_context_is_punctuation_part_of_decimal(const pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    return (strcmp(token->string, ",") == 0) &&
           ((token->previous != NULL) &&
            ((token->previous->tag == PV_NORMALIZER_TAG_IT_CARDINAL) ||
             (token->previous->tag == PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL) ||
             (token->previous->tag == PV_NORMALIZER_TAG_IT_CARDINAL_ONE) ||
             (token->previous->tag == PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL_ONE)));
}

bool pv_normalizer_stream_context_can_be_phone_number(const pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    return (strcmp(token->string, "(") == 0);
}

bool pv_normalizer_stream_context_can_be_fraction_or_per_slash(const pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    return (strcmp(token->string, "/") == 0);
}

bool pv_normalizer_stream_context_is_punctuation_part_of_url_or_abbreviation(const pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    bool is_dot = (strcmp(token->string, ".") == 0);
    bool previous_is_dot = (token->previous != NULL) && (strcmp(token->previous->string, ".") == 0);
    if (is_dot && !previous_is_dot) {
        return true;
    }

    bool is_colon = (strcmp(token->string, ":") == 0);
    bool previous_is_punctuation = (token->previous != NULL) && (token->previous->tag == PV_NORMALIZER_TAG_IT_PUNCTUATION);
    if (is_colon && !previous_is_punctuation) {
        return true;
    }

    return false;
}
