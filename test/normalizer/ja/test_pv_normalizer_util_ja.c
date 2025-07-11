#include <stdlib.h>
#include <string.h>

#include "test/pv_test.h"

#include "orca/normalizer/ja/pv_normalizer_util_ja.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_normalizer_mock.h"

#endif

static void test_pv_normalizer_util_ja_is_special_character() {
    int32_t tmp_length = 0;

    const char *special_character = "@";
    const char *non_special_character = "a";
    const char *invalid_character = "\xF0\x90\x84\x87\x87";
    bool result_1 = pv_normalizer_util_ja_is_special_character(special_character, &tmp_length);
    bool result_2 = pv_normalizer_util_ja_is_special_character(non_special_character, &tmp_length);
    bool result_3 = pv_normalizer_util_ja_is_special_character(invalid_character, &tmp_length);
    pv_test_true(result_1 == true, "failed to recognize the Japanese special character: `%s`", special_character);
    pv_test_true(result_2 == false, "failed to recognize the Japanese non-special character: `%s`", non_special_character);
    pv_test_true(result_3 == false, "should have returned false for an invalid character");
}

static void test_pv_normalizer_util_ja_is_punctuation() {
    const char *punctuation = "。";
    const char *non_punctuation = "@";
    const char *invalid_character = "\xF0\x90\x84\x87\x87";
    bool result_1 = pv_normalizer_util_ja_is_punctuation(punctuation);
    bool result_2 = pv_normalizer_util_ja_is_punctuation(non_punctuation);
    bool result_3 = pv_normalizer_util_ja_is_punctuation(invalid_character);
    pv_test_true(result_1 == true, "failed to recognize the Japanese punctuation '%s'", punctuation);
    pv_test_true(result_2 == false, "failed to recognize the Japanese non-punctuation '%s'", non_punctuation);
    pv_test_true(result_3 == false, "should have returned false for an invalid character");
}

static void test_pv_normalizer_util_ja_is_normalizable_character() {
    const char *normalizable_character = "音";
    const char *non_normalizable_character = "ę";
    const char *invalid_character = "\x8C\xEA";
    bool result_1 = pv_normalizer_util_ja_is_normalizable_character(normalizable_character);
    bool result_2 = pv_normalizer_util_ja_is_normalizable_character(non_normalizable_character);
    bool result_3 = pv_normalizer_util_ja_is_normalizable_character(invalid_character);
    pv_test_true(result_1 == true, "failed to recognize the Japanese normalizable character '%s'", normalizable_character);
    pv_test_true(result_2 == false, "failed to recognize the Japanese non-normalizable character '%s'", non_normalizable_character);
    pv_test_true(result_3 == false, "should have returned false for an invalid character");
}

static void test_pv_normalizer_util_ja_is_word_character() {
    const char *word_character = "音";
    const char *non_word_character = "ㅀ";
    const char *non_three_byte_character = "ę";
    const char *invalid_character = "\x8C\xEA";
    bool result_1 = pv_normalizer_util_ja_is_word_character(word_character);
    bool result_2 = pv_normalizer_util_ja_is_word_character(non_word_character);
    bool result_3 = pv_normalizer_util_ja_is_word_character(non_three_byte_character);
    bool result_4 = pv_normalizer_util_ja_is_word_character(invalid_character);
    pv_test_true(result_1 == true, "failed to recognize the Japanese word character '%s'", word_character);
    pv_test_true(result_2 == false, "failed to recognize the Japanese non-word character '%s'", non_word_character);
    pv_test_true(result_3 == false, "failed to recognize the Japanese non-3-byte character '%s'", non_three_byte_character);
    pv_test_true(result_4 == false, "should have returned false for an invalid character");
}

static void test_pv_normalizer_util_ja_is_capitalized_word() {
    const char *cap = "Word";
    const char *no_cap = "word";
    const char *empty = "";
    const char *cant_cap = "音";
    const char *invalid_character = "\x8C\xEA";
    bool result_1, result_2, result_3, result_4, result_5;
    pv_status_t status = pv_normalizer_util_ja_is_capitalized_word(cap, &result_1);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "expected `%s`, got `%s`",
            pv_status_to_string(PV_STATUS_SUCCESS),
            pv_status_to_string(status));
    status = pv_normalizer_util_ja_is_capitalized_word(no_cap, &result_2);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "expected `%s`, got `%s`",
            pv_status_to_string(PV_STATUS_SUCCESS),
            pv_status_to_string(status));
    status = pv_normalizer_util_ja_is_capitalized_word(empty, &result_3);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "expected `%s`, got `%s`",
            pv_status_to_string(PV_STATUS_SUCCESS),
            pv_status_to_string(status));
    status = pv_normalizer_util_ja_is_capitalized_word(cant_cap, &result_4);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "expected `%s`, got `%s`",
            pv_status_to_string(PV_STATUS_SUCCESS),
            pv_status_to_string(status));
    status = pv_normalizer_util_ja_is_capitalized_word(invalid_character, &result_5);
    pv_test_true(
            status == PV_STATUS_INVALID_ARGUMENT,
            "expected `%s`, got `%s`",
            pv_status_to_string(PV_STATUS_INVALID_ARGUMENT),
            pv_status_to_string(status));
    pv_test_true(result_1 == true, "failed to recognize the capitalized word '%s'", cap);
    pv_test_true(result_2 == false, "failed to recognize the non-capitalized word '%s'", no_cap);
    pv_test_true(result_3 == false, "failed to recognize an empty string");
    pv_test_true(result_4 == false, "failed to recognize the can't capitalize word '%s'", cant_cap);
    pv_test_true(result_5 == false, "should have stayed false for an invalid character");
}

static void test_pv_normalizer_util_ja_normalize_full_width_text() {
    const char *full_width = "１つ";
    const char *expected_norm = "1つ";

    char *norm = NULL;
    pv_status_t status = pv_normalizer_util_ja_normalize_full_width_text(full_width, &norm);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "expected `%s`, got `%s`",
            pv_status_to_string(PV_STATUS_SUCCESS),
            pv_status_to_string(status));
    pv_test_true(strcmp(expected_norm, norm) == 0, "expected `%s`, got `%s`", expected_norm, full_width);
    free(norm);
}

static void test_pv_normalizer_util_ja_normalize_full_width_text_four_byte() {
    const char *full_width = "\xF0\x90\x84\x87";

    char *norm = NULL;
    pv_status_t status = pv_normalizer_util_ja_normalize_full_width_text(full_width, &norm);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "expected `%s`, got `%s`",
            pv_status_to_string(PV_STATUS_SUCCESS),
            pv_status_to_string(status));
    free(norm);
}

static void test_pv_normalizer_util_ja_normalize_full_width_text_invalid() {
    const char *full_width = "\xF0\x90\x84\x87\x87";

    char *norm = NULL;
    pv_status_t status = pv_normalizer_util_ja_normalize_full_width_text(full_width, &norm);
    pv_test_true(
            status == PV_STATUS_INVALID_ARGUMENT,
            "expected `%s`, got `%s`",
            pv_status_to_string(PV_STATUS_INVALID_ARGUMENT),
            pv_status_to_string(status));
}

#ifdef __PV_MOCKS__

static void test_pv_normalizer_util_ja_normalize_full_width_text_calloc_fail() {
    PV_SET_MOCK_RETURN_VAL(calloc, NULL);

    const char *full_width = "１つ";

    char *norm = NULL;
    pv_status_t status = pv_normalizer_util_ja_normalize_full_width_text(full_width, &norm);
    pv_test_true(
            status == PV_STATUS_OUT_OF_MEMORY,
            "expected `%s`, got `%s`",
            pv_status_to_string(PV_STATUS_OUT_OF_MEMORY),
            pv_status_to_string(status));
}

#endif

static const pv_test_case_t PV_NORMALIZER_UTIL_JA_TEST_CASES[] = {
        {"is special character", test_pv_normalizer_util_ja_is_special_character},
        {"is punctuation", test_pv_normalizer_util_ja_is_punctuation},
        {"is normalizable character", test_pv_normalizer_util_ja_is_normalizable_character},
        {"is word character", test_pv_normalizer_util_ja_is_word_character},
        {"is capitalized word", test_pv_normalizer_util_ja_is_capitalized_word},
        {"normalize full width", test_pv_normalizer_util_ja_normalize_full_width_text},
        {"normalize full width four byte", test_pv_normalizer_util_ja_normalize_full_width_text_four_byte},
        {"normalize full width invalid", test_pv_normalizer_util_ja_normalize_full_width_text_invalid},

#ifdef __PV_MOCKS__

        {"normalize full width calloc fail", test_pv_normalizer_util_ja_normalize_full_width_text_calloc_fail},

#endif

};

const pv_test_suite_t PV_NORMALIZER_UTIL_JA_TEST_SUITE = {
        .name = "normalizer_util_ja",
        .setup = NULL,
        .teardown = NULL,
        .num_test_cases = PV_ARRAY_LEN(PV_NORMALIZER_UTIL_JA_TEST_CASES),
        .test_cases = PV_NORMALIZER_UTIL_JA_TEST_CASES,
};
