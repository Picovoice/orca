#include <ctype.h>
#include <string.h>

#include "core/pv_assert.h"
#include "core/pv_error_messages.h"
#include "orca/normalizer/pv_normalizer_token.h"
#include "orca/normalizer/pv_normalizer_use_cases.h"
#include "orca/normalizer/pv_normalizer_util.h"

#include "orca/normalizer/fr/pv_normalizer_stream_context_scanner_fr.h"
#include "orca/normalizer/fr/pv_normalizer_tags_fr.h"
#include "orca/normalizer/pv_normalizer_data/pv_normalizer_shared_data.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_normalizer_mock.h"

#endif

static pv_status_t pv_normalizer_stream_context_can_be_time(const pv_normalizer_token_t *token);

static bool pv_normalizer_stream_context_can_be_phone_number(const pv_normalizer_token_t *token);

static bool pv_normalizer_stream_context_can_be_fraction_or_per_slash(const pv_normalizer_token_t *token);

static bool pv_normalizer_stream_context_is_number(const pv_normalizer_token_t *token);

static bool pv_normalizer_stream_context_is_measurement(const pv_normalizer_token_t *token);

static bool pv_normalizer_stream_context_is_punctuation_part_of_time(const pv_normalizer_token_t *token);

static bool pv_normalizer_stream_context_is_punctuation_part_of_cardinal(const pv_normalizer_token_t *token);

static bool pv_normalizer_stream_context_is_punctuation_part_of_decimal(const pv_normalizer_token_t *token);

static bool pv_normalizer_stream_context_is_punctuation_part_of_url_or_abbreviation(const pv_normalizer_token_t *token);

bool PV_MOCKABLE(pv_normalizer_stream_context_scanner_fr_is_verbalizable)(
        const pv_normalizer_token_t *token,
        int32_t num_use_cases,
        const pv_normalizer_use_cases_fr_t *use_cases) {
    PV_ASSERT(token);
    PV_ASSERT(num_use_cases >= 0);
    PV_ASSERT(use_cases);

    if (token->tag == PV_NORMALIZER_TAG_FR_SPECIAL_CHARACTER) {
        for (int32_t j = 0; j < num_use_cases; j++) {
            if (use_cases[j] == PV_NORMALIZER_USE_DIGITS_SEQUENCE_NORMALIZER_FR) {
                if (pv_normalizer_stream_context_can_be_phone_number(token)) {
                    return false;
                }
            } else if (use_cases[j] == PV_NORMALIZER_USE_FRACTION_NORMALIZER_FR) {
                if (pv_normalizer_stream_context_can_be_fraction_or_per_slash(token)) {
                    return false;
                }
            }
        }

        return true;
    }

    if (token->tag == PV_NORMALIZER_TAG_FR_CURRENCY_SYMBOL) {
        return false;
    }

    if (token->tag == PV_NORMALIZER_TAG_FR_CUSTOM_PRONUNCIATION) {
        return true;
    }

    if (token->tag == PV_NORMALIZER_TAG_FR_ABBREVIATION) {
        return true;
    }

    if (token->tag == PV_NORMALIZER_TAG_FR_SPACE) {
        pv_normalizer_token_t *previous = pv_normalizer_token_get_nth_token_before(token, 1, true);
        if (previous != NULL) {
            if (pv_normalizer_stream_context_is_number(previous)) {
                return false;
            }
            if (pv_normalizer_stream_context_is_measurement(previous)) {
                return false;
            }
        }
        pv_normalizer_token_t *two_previous = pv_normalizer_token_get_nth_token_before(token, 2, true);
        if (two_previous != NULL) {
            if (pv_normalizer_stream_context_is_number(two_previous)) {
                return false;
            }
            if (pv_normalizer_stream_context_is_measurement(two_previous)) {
                return false;
            }
        }
        return true;
    }

    if (token->tag == PV_NORMALIZER_TAG_FR_PUNCTUATION) {
        if (strcmp(token->string, ",") == 0) {
            pv_normalizer_token_t *previous = pv_normalizer_token_get_nth_token_before(token, 1, false);
            if (previous != NULL) {
                if (pv_normalizer_stream_context_is_number(previous)) {
                    return false;
                }
            }
        }
    }

    bool is_verbalizable = false;

    bool is_after_dot = (token->previous != NULL) && (strcmp(token->previous->string, ".") == 0);
    if (is_after_dot) {
        if (token->tag == PV_NORMALIZER_TAG_FR_SPACE) {
            if (!pv_normalizer_stream_context_is_number(pv_normalizer_token_get_nth_token_before(token, 2, false))) {
                is_verbalizable = true;
            }
        }
    }

    if (token->tag == PV_NORMALIZER_TAG_FR_PUNCTUATION) {
        is_verbalizable = true;

        if (strcmp(token->string, ":") == 0) {
            return false;
        }

        for (int32_t j = 0; j < num_use_cases; j++) {
            if ((use_cases[j] == PV_NORMALIZER_USE_URL_NORMALIZER_FR) ||
                    use_cases[j] == PV_NORMALIZER_USE_ABBREVIATION_NORMALIZER_FR) {
                if (pv_normalizer_stream_context_is_punctuation_part_of_url_or_abbreviation(token)) {
                    return false;
                }
            } else if (use_cases[j] == PV_NORMALIZER_USE_TIME_NORMALIZER_FR) {
                if (pv_normalizer_stream_context_is_punctuation_part_of_time(token)) {
                    return false;
                }
            } else if ((use_cases[j] == PV_NORMALIZER_USE_CARDINAL_NORMALIZER_FR) ||
                    use_cases[j] == PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_FR) {
                if (pv_normalizer_stream_context_is_punctuation_part_of_cardinal(token)) {
                    return false;
                }
            } else if (use_cases[j] == PV_NORMALIZER_USE_DECIMAL_NORMALIZER_FR) {
                if (pv_normalizer_stream_context_is_punctuation_part_of_decimal(token)) {
                    return false;
                }
            }
        }
    }

    return is_verbalizable;
}

void PV_MOCKABLE(pv_normalizer_stream_context_scanner_fr_remove_hyphen_only_tokens)(
        pv_normalizer_token_list_t *token_list) {
    PV_ASSERT(token_list);

    pv_normalizer_token_t *current = token_list->head;
    while (current) {
        pv_normalizer_token_t *next = current->next;

        if (current->tag != PV_NORMALIZER_TAG_FR_NUMBER_RANGE_TO) {
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

    return (((token->tag == PV_NORMALIZER_TAG_FR_CARDINAL) &&
             (!pv_normalizer_util_string_number_greater_than_int(token->string, 24))) ||
            (token->tag == PV_NORMALIZER_TAG_FR_TIME_MINUTES) ||
            (token->tag == PV_NORMALIZER_TAG_FR_TIME_HOURS) ||
            ((token->tag == PV_NORMALIZER_TAG_FR_ALPHANUM_SPELL_OUT) && (strlen(token->string) <= 3)));
}

bool pv_normalizer_stream_context_is_punctuation_part_of_time(const pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    bool can_be_time =
            (strcmp(token->string, ":") == 0 || strcmp(token->string, "h") == 0) &&
            (token->previous != NULL) &&
            (token->previous->tag == PV_NORMALIZER_TAG_FR_CARDINAL) &&
            !pv_normalizer_util_string_number_greater_than_int(token->previous->string, 24);
    if (can_be_time) {
        return true;
    }

    bool is_valid_end = (strcmp(token->string, ".") == 0) ||
            (strcmp(token->string, APOSTROPHE) == 0) ||
            (strcmp(token->string, "m") == 0);

    if (is_valid_end) {
        pv_normalizer_token_t *previous = pv_normalizer_token_get_nth_token_before(token, 1, true);
        if (previous && pv_normalizer_stream_context_can_be_time(previous)) {
            return true;
        }
    }

    return false;
}

bool pv_normalizer_stream_context_is_punctuation_part_of_cardinal(const pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    return
            (strcmp(token->string, ".") == 0) &&
            ((token->previous != NULL) &&
             ((token->previous->tag == PV_NORMALIZER_TAG_FR_CARDINAL) ||
              (token->previous->tag == PV_NORMALIZER_TAG_FR_NEGATIVE_CARDINAL)));
}

bool pv_normalizer_stream_context_is_punctuation_part_of_decimal(const pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    pv_normalizer_token_t *previous = pv_normalizer_token_get_nth_token_before(token, 1, false);
    pv_normalizer_token_t *two_previous = pv_normalizer_token_get_nth_token_before(token, 2, false);

    bool is_comma = strcmp(token->string, ",") == 0;
    bool is_previous_correct_tag =
            (previous != NULL) &&
            ((previous->tag == PV_NORMALIZER_TAG_FR_CARDINAL) ||
             (previous->tag == PV_NORMALIZER_TAG_FR_NEGATIVE_CARDINAL));
    bool is_two_previous_hyphen = (two_previous != NULL) && (strcmp(two_previous->string, "-") == 0);

    return !is_two_previous_hyphen && is_comma && is_previous_correct_tag;
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
    bool previous_is_punctuation = (token->previous != NULL) && (token->previous->tag == PV_NORMALIZER_TAG_FR_PUNCTUATION);
    if (is_colon && !previous_is_punctuation) {
        return true;
    }

    return false;
}

bool pv_normalizer_stream_context_is_number(const pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    return
            (token != NULL) &&
            ((token->tag == PV_NORMALIZER_TAG_FR_CARDINAL) ||
             (token->tag == PV_NORMALIZER_TAG_FR_NEGATIVE_CARDINAL) ||
             ((strlen(token->string) > 0) && isdigit(token->string[0])));
}

bool pv_normalizer_stream_context_is_measurement(const pv_normalizer_token_t *token) {
    PV_ASSERT(token);

    return
            (token != NULL) &&
            (token->tag == PV_NORMALIZER_TAG_FR_MEASUREMENT);
}
