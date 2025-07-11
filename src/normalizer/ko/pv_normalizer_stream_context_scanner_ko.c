#include <ctype.h>
#include <string.h>

#include "core/pv_assert.h"
#include "core/pv_error_messages.h"
#include "orca/normalizer/pv_normalizer_token.h"
#include "orca/normalizer/pv_normalizer_use_cases.h"
#include "orca/normalizer/pv_normalizer_util.h"

#include "orca/normalizer/ko/pv_normalizer_stream_context_scanner_ko.h"
#include "orca/normalizer/ko/pv_normalizer_tags_ko.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_normalizer_mock.h"

#endif

static pv_status_t pv_normalizer_stream_context_can_be_time(const pv_normalizer_token_t *token);

static bool pv_normalizer_stream_context_can_be_phone_number(const pv_normalizer_token_t *token);

static bool pv_normalizer_stream_context_can_be_fraction_or_per_slash(const pv_normalizer_token_t *token);

static bool pv_normalizer_stream_context_is_number(const pv_normalizer_token_t *token);

static bool pv_normalizer_stream_context_is_punctuation_part_of_time(const pv_normalizer_token_t *token);

static bool pv_normalizer_stream_context_is_punctuation_part_of_cardinal(const pv_normalizer_token_t *token);

static bool pv_normalizer_stream_context_is_punctuation_part_of_decimal(const pv_normalizer_token_t *token);

static bool pv_normalizer_stream_context_is_punctuation_part_of_url(const pv_normalizer_token_t *token);

static bool pv_normalizer_stream_context_is_punctuation_part_of_date(const pv_normalizer_token_t *token);

bool PV_MOCKABLE(pv_normalizer_stream_context_scanner_ko_is_verbalizable)(
        const pv_normalizer_token_t *token,
        int32_t num_use_cases,
        const pv_normalizer_use_cases_ko_t *use_cases) {
    PV_ASSERT(token);
    PV_ASSERT(num_use_cases >= 0);
    PV_ASSERT(use_cases);

    if (token->tag == PV_NORMALIZER_TAG_KO_SPECIAL_CHARACTER) {
        for (int32_t j = 0; j < num_use_cases; j++) {
            if (use_cases[j] == PV_NORMALIZER_USE_DIGITS_SEQUENCE_NORMALIZER_KO) {
                if (pv_normalizer_stream_context_can_be_phone_number(token)) {
                    return false;
                }
            } else if (use_cases[j] == PV_NORMALIZER_USE_FRACTION_NORMALIZER_KO) {
                if (pv_normalizer_stream_context_can_be_fraction_or_per_slash(token)) {
                    return false;
                }
            }
        }

        return true;
    }

    if (token->tag == PV_NORMALIZER_TAG_KO_CURRENCY_SYMBOL) {
        return true;
    }

    if (token->tag == PV_NORMALIZER_TAG_KO_CUSTOM_PRONUNCIATION) {
        return true;
    }

    if (token->tag == PV_NORMALIZER_TAG_KO_ABBREVIATION) {
        return true;
    }

    if (token->tag == PV_NORMALIZER_TAG_KO_TIME_HOURS || token->tag == PV_NORMALIZER_TAG_KO_TIME_MINUTES) {
        return false;
    }

    if (token->tag == PV_NORMALIZER_TAG_KO_TIME_AM_PM) {
        return true;
    }

    if (token->tag == PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT) {
        return false;
    }

    if (token->next != NULL) {
        if (token->tag != PV_NORMALIZER_TAG_KO_CARDINAL && token->next_character_is_space) {
            return true;
        }
    }

    if (token->tag == PV_NORMALIZER_TAG_KO_SPACE) {
        if (token->previous != NULL) {
            if (token->previous->tag == PV_NORMALIZER_TAG_KO_TIME_HOURS ||
                token->previous->tag == PV_NORMALIZER_TAG_KO_TIME_MINUTES ||
                token->previous->tag == PV_NORMALIZER_TAG_KO_CARDINAL ||
                token->previous->tag == PV_NORMALIZER_TAG_KO_MEASUREMENT) {
                return false;
            } else if (pv_normalizer_stream_context_is_punctuation_part_of_date(token->previous)) {
                return false;
            }
        }
        return true;
    }

    bool is_verbalizable = false;

    bool is_after_dot = (token->previous != NULL) && (strcmp(token->previous->string, ".") == 0);
    if (is_after_dot) {
        if (token->tag == PV_NORMALIZER_TAG_KO_SPACE) {
            if (!pv_normalizer_stream_context_is_number(pv_normalizer_token_get_nth_token_before(token, 2, false))) {
                is_verbalizable = true;
            }
        }
    }

    if (token->tag == PV_NORMALIZER_TAG_KO_PUNCTUATION) {
        is_verbalizable = true;

        for (int32_t j = 0; j < num_use_cases; j++) {
            if (use_cases[j] == PV_NORMALIZER_USE_URL_NORMALIZER_KO) {
                if (pv_normalizer_stream_context_is_punctuation_part_of_url(token)) {
                    return false;
                }
            } else if (use_cases[j] == PV_NORMALIZER_USE_TIME_NORMALIZER_KO) {
                if (pv_normalizer_stream_context_is_punctuation_part_of_time(token)) {
                    return false;
                }
            } else if (
                    (use_cases[j] == PV_NORMALIZER_USE_CARDINAL_NORMALIZER_KO) ||
                    use_cases[j] == PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_KO) {
                if (pv_normalizer_stream_context_is_punctuation_part_of_cardinal(token)) {
                    return false;
                }
            } else if (use_cases[j] == PV_NORMALIZER_USE_DECIMAL_NORMALIZER_KO) {
                if (pv_normalizer_stream_context_is_punctuation_part_of_decimal(token)) {
                    return false;
                }
            }
        }
    }

    return is_verbalizable;
}

void PV_MOCKABLE(pv_normalizer_stream_context_scanner_ko_remove_hyphen_only_tokens)(
        pv_normalizer_token_list_t *token_list) {
    PV_ASSERT(token_list);

    pv_normalizer_token_t *current = token_list->head;
    while (current) {
        pv_normalizer_token_t *next = current->next;

        if (current->tag != PV_NORMALIZER_TAG_KO_NUMBER_RANGE_TO) {
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

pv_status_t pv_normalizer_stream_context_can_be_time(const pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    return (((token->tag == PV_NORMALIZER_TAG_KO_CARDINAL) &&
             (!pv_normalizer_util_string_number_greater_than_int(token->string, 24))) ||
            (token->tag == PV_NORMALIZER_TAG_KO_TIME_MINUTES) ||
            (token->tag == PV_NORMALIZER_TAG_KO_TIME_HOURS) ||
            ((token->tag == PV_NORMALIZER_TAG_KO_ALPHANUM_SPELL_OUT) && (strlen(token->string) <= 3)));
}

bool pv_normalizer_stream_context_is_punctuation_part_of_time(const pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    bool can_be_time =
            (strcmp(token->string, ":") == 0) &&
            (token->previous != NULL) &&
            (token->previous->tag == PV_NORMALIZER_TAG_KO_CARDINAL) &&
            !pv_normalizer_util_string_number_greater_than_int(token->previous->string, 24);
    if (can_be_time) {
        return true;
    }

    bool is_dot = (strcmp(token->string, ".") == 0);
    bool can_be_first_dot_am_pm =
            ((token->previous != NULL) &&
             ((strcmp(token->previous->string, "a") == 0) ||
              (strcmp(token->previous->string, "A") == 0) ||
              (strcmp(token->previous->string, "p") == 0) ||
              (strcmp(token->previous->string, "P") == 0)));
    if (is_dot && can_be_first_dot_am_pm) {
        pv_normalizer_token_t *previous = pv_normalizer_token_get_nth_token_before(token, 1, true);
        if (previous && pv_normalizer_stream_context_can_be_time(previous)) {
            return true;
        }

        pv_normalizer_token_t *two_previous = pv_normalizer_token_get_nth_token_before(token, 2, true);
        if (two_previous && pv_normalizer_stream_context_can_be_time(two_previous)) {
            return true;
        }
    }

    return false;
}

bool pv_normalizer_stream_context_is_punctuation_part_of_cardinal(const pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    return
            (strcmp(token->string, ",") == 0) &&
            ((token->previous != NULL) &&
             ((token->previous->tag == PV_NORMALIZER_TAG_KO_CARDINAL) ||
              (token->previous->tag == PV_NORMALIZER_TAG_KO_NEGATIVE_CARDINAL)));
}

bool pv_normalizer_stream_context_is_punctuation_part_of_decimal(const pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    return
            (strcmp(token->string, ".") == 0) &&
            ((token->previous != NULL) &&
             ((token->previous->tag == PV_NORMALIZER_TAG_KO_CARDINAL) ||
              (token->previous->tag == PV_NORMALIZER_TAG_KO_NEGATIVE_CARDINAL))) &&
            !((token->previous->previous != NULL) &&
              (strcmp(token->previous->previous->string, "-") == 0));
}

bool pv_normalizer_stream_context_is_punctuation_part_of_date(const pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    bool can_be_date =
            (strcmp(token->string, ".") == 0) &&
            (token->previous != NULL) &&
            (token->previous->tag == PV_NORMALIZER_TAG_KO_CARDINAL);
    if (can_be_date) {
        return true;
    }

    return false;
}

bool pv_normalizer_stream_context_can_be_phone_number(const pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    return (strcmp(token->string, "(") == 0);
}

bool pv_normalizer_stream_context_can_be_fraction_or_per_slash(const pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    return (strcmp(token->string, "/") == 0);
}

bool pv_normalizer_stream_context_is_punctuation_part_of_url(const pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    bool is_dot = (strcmp(token->string, ".") == 0);
    bool previous_is_dot = (token->previous != NULL) && (strcmp(token->previous->string, ".") == 0);
    if (is_dot && !previous_is_dot) {
        return true;
    }

    bool is_colon = (strcmp(token->string, ":") == 0);
    bool previous_is_punctuation = (token->previous != NULL) && (token->previous->tag == PV_NORMALIZER_TAG_KO_PUNCTUATION);
    if (is_colon && !previous_is_punctuation) {
        return true;
    }

    return false;
}

bool pv_normalizer_stream_context_is_number(const pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    return
            (token != NULL) &&
            ((token->tag == PV_NORMALIZER_TAG_KO_CARDINAL) ||
             (token->tag == PV_NORMALIZER_TAG_KO_NEGATIVE_CARDINAL) ||
             ((strlen(token->string) > 0) && isdigit(token->string[0])));
}
