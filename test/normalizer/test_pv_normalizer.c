#include <stdlib.h>
#include <string.h>

#include "core/picovoice.h"
#include "core/pv_language.h"
#include "core/pv_language_json.h"
#include "mock/pv_mock.h"
#include "orca/normalizer/pv_normalizer.h"
#include "orca/normalizer/pv_normalizer_tagger.h"
#include "orca/normalizer/pv_normalizer_token.h"
#include "orca/normalizer/pv_normalizer_tokenizer.h"
#include "orca/normalizer/pv_normalizer_verbalizer.h"
#include "test/pv_test.h"

#ifdef __PV_MOCKS__

#include "core/mock/pv_core_runtime_mock.h"
#include "orca/mock/pv_normalizer_mock.h"

#endif

static pv_normalizer_t *normalizer_object = NULL;

static pv_language_info_t *language_info_object = NULL;
static pv_noun_gender_dict_t *noun_gender_dict_object = NULL;

static const char LANGUAGE_INFO_PATH[] = "normalizer/ref/pv_language_info_normalizer_en.json";
static const char NOUN_GENDER_DICT_PATH[] = "noun_gender_dict/empty.txt";

static const char NORMALIZER_TEST_SENTENCE[] = "123 Neural networks \"work\", sometimes! -5 b-4";
static const char *NORMALIZER_TEST_SENTENCE_NORMALIZED =
        "ONE HUNDRED TWENTY THREE NEURAL NETWORKS \" WORK \" , SOMETIMES ! NEGATIVE FIVE B FOUR";

static void *calloc_return_null(size_t arg0, size_t arg1) {
    (void) arg0;
    (void) arg1;
    return NULL;
}

static pv_status_t test_pv_normalizer_setup(void) {
    char *language_info_path = pv_test_module_res_path(LANGUAGE_INFO_PATH);
    pv_test_true(language_info_path != NULL, "Failed to get language_info_path");
    if (!language_info_path) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_status_t status = pv_language_info_load_json(language_info_path, &language_info_object, true, true);
    free(language_info_path);
    pv_test_true(status == PV_STATUS_SUCCESS, "Failed to load language info from path: '%s'", LANGUAGE_INFO_PATH);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    char *noun_gender_dict_path = pv_test_module_res_path(NOUN_GENDER_DICT_PATH);
    pv_test_true(noun_gender_dict_path != NULL, "Failed to get noun_gender_dict_path");
    if (!noun_gender_dict_path) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    status = pv_noun_gender_dict_init(noun_gender_dict_path, &noun_gender_dict_object);
    free(noun_gender_dict_path);
    pv_test_true(status == PV_STATUS_SUCCESS, "Failed to load noun gender dict from path: '%s'", NOUN_GENDER_DICT_PATH);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_normalizer_init(language_info_object, noun_gender_dict_object, NULL, &normalizer_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_normalizer_teardown(void) {
    pv_noun_gender_dict_delete(noun_gender_dict_object);
    noun_gender_dict_object = NULL;

    pv_normalizer_delete(normalizer_object);
    normalizer_object = NULL;
}

static void test_pv_normalizer_normalize_and_check_normalized_helper(
        pv_normalizer_t *normalizer,
        const char *sentence,
        const char *sentence_normalized,
        bool preserve_word_boundary,
        bool remove_unknown_characters) {
    char *normalized = NULL;
    pv_status_t status = pv_normalizer_normalize(
            normalizer,
            sentence,
            preserve_word_boundary,
            remove_unknown_characters,
            &normalized,
            NULL);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to normalize sentence: `%s`", sentence);

    pv_test_true(
            strcmp(normalized, sentence_normalized) == 0,
            "incorrect normalized string: got `%s`, expected `%s`",
            normalized,
            sentence_normalized);

    free(normalized);
}

static void test_pv_normalizer_normalize_and_check_normalized_status_helper(
        pv_normalizer_t *normalizer,
        const char *sentence,
        bool preserve_word_boundary,
        bool remove_unknown_characters,
        pv_status_t expected) {
    char *normalized = NULL;
    pv_status_t status = pv_normalizer_normalize(
            normalizer,
            sentence,
            preserve_word_boundary,
            remove_unknown_characters,
            &normalized,
            NULL);
    pv_test_true(status == expected,
                 "failed to normalize: expected `%s`, got `%s`",
                 pv_status_to_string(expected),
                 pv_status_to_string(status));
    if (status == PV_STATUS_SUCCESS) {
        free(normalized);
    }
}

static void test_pv_normalizer_normalize(void) {
    test_pv_normalizer_normalize_and_check_normalized_helper(
            normalizer_object,
            NORMALIZER_TEST_SENTENCE,
            NORMALIZER_TEST_SENTENCE_NORMALIZED,
            false,
            false);
}

static void test_pv_normalizer_normalize_custom_pronunciation(void) {
    const char *sentence = "A {custom|K AH S T AH M} pronunciation {|V AE L}";
    const char *sentence_normalized = "A {K AH S T AH M} PRONUNCIATION {V AE L}";

    test_pv_normalizer_normalize_and_check_normalized_helper(
            normalizer_object,
            sentence,
            sentence_normalized,
            false,
            false);
}

static void test_pv_normalizer_normalize_number_range(void) {
    const char *sentence = "i got 20-30 apples";
    const char *sentence_normalized = "I GOT TWENTY TO THIRTY APPLES";

    test_pv_normalizer_normalize_and_check_normalized_helper(
            normalizer_object,
            sentence,
            sentence_normalized,
            false,
            false);
}

static void test_pv_normalizer_normalize_ordinal(void) {
    const char *sentence = "This is the 1st event in 5 years!";
    const char *sentence_normalized = "THIS IS THE FIRST EVENT IN FIVE YEARS !";

    test_pv_normalizer_normalize_and_check_normalized_helper(
            normalizer_object,
            sentence,
            sentence_normalized,
            false,
            false);
}

static void test_pv_normalizer_normalize_negative_ordinal(void) {
    const char *sentence = "a -2nd order equation";
    const char *sentence_normalized = "A NEGATIVE SECOND ORDER EQUATION";

    test_pv_normalizer_normalize_and_check_normalized_helper(
            normalizer_object,
            sentence,
            sentence_normalized,
            false,
            false);
}

static void test_pv_normalizer_normalize_token_array(void) {
    const char *sentence = "hello 55";
    const char *target_verbalized[] = {"HELLO", "FIFTY", " ", "FIVE"};

    pv_normalizer_token_list_t *token_list = NULL;
    pv_status_t status = pv_normalizer_normalize(
            normalizer_object,
            sentence,
            false,
            false,
            NULL,
            &token_list);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to normalize sentence: `%s`", NORMALIZER_TEST_SENTENCE);

    pv_normalizer_token_t *current = token_list->head;
    int32_t i = 0;
    while (current) {
        if (target_verbalized[i] == NULL) {
            pv_test_true(
                    current->verbalized == NULL,
                    "verbalized token expected to be NULL, found `%s`",
                    current->verbalized);
        } else {
            pv_test_true(
                    strcmp(current->verbalized, target_verbalized[i]) == 0,
                    "incorrect normalized string: got `%s`, expected `%s`",
                    current->verbalized,
                    target_verbalized[i]);
        }
        current = current->next;
        i++;
    }

    pv_normalizer_token_list_delete(token_list);
}

static void test_pv_normalizer_normalize_special_character(void) {
    const char *sentence = "a&w 25% -15% jim@hotmail";
    const char *sentence_normalized = "A AND W TWENTY FIVE PERCENT NEGATIVE FIFTEEN PERCENT JIM AT HOTMAIL";

    test_pv_normalizer_normalize_and_check_normalized_helper(
            normalizer_object,
            sentence,
            sentence_normalized,
            false,
            false);
}

static void test_pv_normalizer_normalize_decimal(void) {
    const char *sentence = "9.99 .7 -0.6";
    const char *sentence_normalized = "NINE POINT NINE NINE POINT SEVEN NEGATIVE ZERO POINT SIX";

    test_pv_normalizer_normalize_and_check_normalized_helper(
            normalizer_object,
            sentence,
            sentence_normalized,
            false,
            false);
}

static void test_pv_normalizer_normalize_measurement(void) {
    const char *sentence = "the route was 24.7km with 1200 m of vert. the speed was 15km/h";
    const char *sentence_normalized =
            "THE ROUTE WAS TWENTY FOUR POINT SEVEN KILOMETERS WITH ONE THOUSAND TWO HUNDRED METERS OF VERT . "
            "THE SPEED WAS FIFTEEN KILOMETERS PER HOUR";

    test_pv_normalizer_normalize_and_check_normalized_helper(
            normalizer_object,
            sentence,
            sentence_normalized,
            false,
            false);
}

static void test_pv_normalizer_normalize_fraction(void) {
    const char *sentence = "1/2 9/3.2 -4/3";
    const char *sentence_normalized = "ONE HALF NINE OVER THREE POINT TWO NEGATIVE FOUR THIRDS";

    test_pv_normalizer_normalize_and_check_normalized_helper(
            normalizer_object,
            sentence,
            sentence_normalized,
            false,
            false);
}

static void test_pv_normalizer_normalize_clean_text(void) {
    const char *sentence = "*~本  語 Hello  ल्लो テスト  هي";
    const char *sentence_normalized = "HELLO";

    test_pv_normalizer_normalize_and_check_normalized_helper(
            normalizer_object,
            sentence,
            sentence_normalized,
            true,
            true);
}

static void test_pv_normalizer_normalize_invalid_custom_pron(void) {
    const char *sentences[] = {
            "A {|AY} {custom|K AH S pron {valid|V AE L AH D} test!{valid|IH}% {{{|hi}was}||}",
            "}",
            "}|{|hi}",
            "}}}}|{|WHAT}",
            "hi{you_r|YAOR}{**your|}",
            "{w. you",
    };

    for (int32_t i = 0; i < PV_ARRAY_LEN(sentences); i++) {
        test_pv_normalizer_normalize_and_check_normalized_status_helper(
                normalizer_object,
                sentences[i],
                true,
                true,
                PV_STATUS_INVALID_ARGUMENT);
    }
}

static void test_pv_normalizer_normalize_robustness_tests(void) {
    const char *sentences[] = {
            "hi%<|end|>",
            "hi^<*end>",
            "hi\n<*|end>",
            "ONE\n1 HI",
            "1 \n hi",
            " \n-\n|~.",
            " %\n-~.",
            "**BEGIN**\n1. This",
            "__\n|       |",
            ")\n",
    };
    const char *targets[] = {
            "HI PERCENT END",
            "HI END",
            "HI . END",
            "ONE . ONE HI",
            "ONE . HI",
            ". . .",
            "PERCENT . .",
            "BEGIN . ONE . THIS",
            "UNDERSCORE UNDERSCORE .",
            ", .",
    };

    for (int32_t i = 0; i < PV_ARRAY_LEN(sentences); i++) {
        test_pv_normalizer_normalize_and_check_normalized_helper(
                normalizer_object,
                sentences[i],
                targets[i],
                true,
                true);
    }
}

#ifdef __PV_MOCKS__

static void test_pv_normalizer_init_helper(
        pv_status_t expected_status,
        const char *expected_public_message_regex,
        const char *expected_private_message_regex) {
    char *language_info_path = pv_test_module_res_path(LANGUAGE_INFO_PATH);
    pv_language_info_load_json(language_info_path, &language_info_object, true, true);

    pv_normalizer_t *object = NULL;
    pv_status_t status = pv_normalizer_init(language_info_object, noun_gender_dict_object, NULL, &object);
    reset_mocks();
    pv_test_true(
            status == expected_status,
            "init normalizer error, expected '%s' got '%s'",
            pv_status_to_string(expected_status),
            pv_status_to_string(status));
    if (expected_status != PV_STATUS_SUCCESS) {
        const char *expected_message = expected_public_message_regex;

        #ifdef __PV_ERROR_SHOW_PRIVATE_MSGS__

        if (expected_private_message_regex) {
            expected_message = expected_private_message_regex;
        }

        #endif

        pv_test_error_message(
                expected_public_message_regex,
                expected_private_message_regex,
                true,
                "error message mismatch, expected '%s'",
                expected_message);
    }

    if (status == PV_STATUS_SUCCESS) {
        pv_normalizer_delete(object);
    }
}

static void test_pv_normalizer_normalize_helper(
        pv_status_t expected_status,
        const char *expected_public_message_regex,
        const char *expected_private_message_regex) {
    char *normalized = NULL;
    pv_normalizer_token_list_t *token_list = NULL;
    pv_status_t status = pv_normalizer_normalize(
            normalizer_object,
            NORMALIZER_TEST_SENTENCE,
            false,
            false,
            &normalized,
            &token_list);
    reset_mocks();
    pv_test_true(
            status == expected_status,
            "normalizer normalize error, expected '%s' got '%s'",
            pv_status_to_string(expected_status),
            pv_status_to_string(status));
    if (expected_status != PV_STATUS_SUCCESS) {
        const char *expected_message = expected_public_message_regex;

        #ifdef __PV_ERROR_SHOW_PRIVATE_MSGS__

        if (expected_private_message_regex) {
            expected_message = expected_private_message_regex;
        }

        #endif

        pv_test_error_message(
                expected_public_message_regex,
                expected_private_message_regex,
                true,
                "error message mismatch, expected '%s'",
                expected_message);
    }

    if (status == PV_STATUS_SUCCESS) {
        free(normalized);
    }
}

static void test_pv_normalizer_init_failure(void) {
    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_init_helper(PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `o`\\.");
}

static void test_pv_normalizer_tokenizer_init_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_language_info_load_json, PV_STATUS_SUCCESS);
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_tokenizer_init, PV_STATUS_OUT_OF_MEMORY);
    test_pv_normalizer_init_helper(PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_tokenizer_init` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_normalizer_tagger_init_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_tagger_init, PV_STATUS_OUT_OF_MEMORY);
test_pv_normalizer_init_helper(PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_tagger_init` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_normalizer_verbalizer_init_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_verbalizer_en_init, PV_STATUS_OUT_OF_MEMORY);
    test_pv_normalizer_init_helper(PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_init` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_normalizer_use_cases_init_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_get_use_cases_from_language, PV_STATUS_OUT_OF_MEMORY);
    test_pv_normalizer_init_helper(PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_get_use_cases_from_language` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_normalizer_normalize_tokenize_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_tokenizer_tokenize, PV_STATUS_OUT_OF_MEMORY);
    test_pv_normalizer_normalize_helper(PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_tokenizer_tokenize` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_normalizer_normalize_tag_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_tagger_tag, PV_STATUS_OUT_OF_MEMORY);
    test_pv_normalizer_normalize_helper(PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_tagger_tag` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_normalizer_normalize_verbalize_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_verbalizer_en_verbalize, PV_STATUS_OUT_OF_MEMORY);
    test_pv_normalizer_normalize_helper(PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_verbalize` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_normalizer_normalize_token_num_bytes_character_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_tokenizer_tokenize, PV_STATUS_INVALID_ARGUMENT);
    test_pv_normalizer_normalize_helper(PV_STATUS_INVALID_ARGUMENT, pv_test_function_hash_regex(), "`pv_normalizer_tokenizer_tokenize` failed with status `INVALID_ARGUMENT`\\.");
}

#endif

static const pv_test_case_t PV_NORMALIZER_TEST_CASES[] = {
        {"normalize", test_pv_normalizer_normalize},
        {"normalize_custom_pronunciation", test_pv_normalizer_normalize_custom_pronunciation},
        {"normalize_number_range", test_pv_normalizer_normalize_number_range},
        {"normalize_ordinal", test_pv_normalizer_normalize_ordinal},
        {"normalize_negative_ordinal", test_pv_normalizer_normalize_negative_ordinal},
        {"normalize_token_array", test_pv_normalizer_normalize_token_array},
        {"normalize_special_character", test_pv_normalizer_normalize_special_character},
        {"normalize_clean_text", test_pv_normalizer_normalize_clean_text},
        {"normalize_invalid_custom_pron", test_pv_normalizer_normalize_invalid_custom_pron},
        {"normalize_robustness_tests", test_pv_normalizer_normalize_robustness_tests},
        {"normalize_decimal", test_pv_normalizer_normalize_decimal},
        {"normalize_measurement", test_pv_normalizer_normalize_measurement},
        {"normalize_fraction", test_pv_normalizer_normalize_fraction},

#ifdef __PV_MOCKS__

        {"normalize_init_failure", test_pv_normalizer_init_failure},
        {"normalize_tokenizer_init_failure", test_pv_normalizer_tokenizer_init_failure},
        {"normalize_tagger_init_failure", test_pv_normalizer_tagger_init_failure},
        {"normalize_verbalizer_init_failure", test_pv_normalizer_verbalizer_init_failure},
        {"normalize_use_cases_init_failure", test_pv_normalizer_use_cases_init_failure},
        {"normalize_tokenize_failure", test_pv_normalizer_normalize_tokenize_failure},
        {"normalize_tag_failure", test_pv_normalizer_normalize_tag_failure},
        {"normalize_verbalize_failure", test_pv_normalizer_normalize_verbalize_failure},
        {"normalize_token_num_bytes_character_failure", test_pv_normalizer_normalize_token_num_bytes_character_failure},

#endif

};

const pv_test_suite_t PV_NORMALIZER_TEST_SUITE = {
        .name = "normalizer",
        .setup = test_pv_normalizer_setup,
        .teardown = test_pv_normalizer_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_NORMALIZER_TEST_CASES),
        .test_cases = PV_NORMALIZER_TEST_CASES,
};
