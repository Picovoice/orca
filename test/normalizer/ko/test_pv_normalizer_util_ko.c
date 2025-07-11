#include "test/pv_test.h"

#include "orca/normalizer/ko/pv_normalizer_util_ko.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_normalizer_mock.h"

#endif

static void test_pv_normalizer_util_is_special_character() {
    int32_t tmp_length = 0;

    const char *special_character = "@";
    const char *non_special_character = "a";
    bool result_1 = pv_normalizer_util_ko_is_special_character(special_character, &tmp_length);
    bool result_2 = pv_normalizer_util_ko_is_special_character(non_special_character, &tmp_length);
    pv_test_true(result_1 == true, "failed to recognize the Korean special characer: `%s`", special_character);
    pv_test_true(result_2 == false, "failed to recognize the Korean non-special characer: `%s`", non_special_character);
}

static void test_pv_normalizer_util_is_punctuation() {
    const char *punctuation = ".";
    const char *non_punctuation = "@";
    bool result_1 = pv_normalizer_util_ko_is_punctuation(punctuation);
    bool result_2 = pv_normalizer_util_ko_is_punctuation(non_punctuation);
    pv_test_true(result_1 == true, "failed to recognize the Korean punctuation '%s'", punctuation);
    pv_test_true(result_2 == false, "failed to recognize the Korean non-punctuation '%s'", non_punctuation);
}

static void test_pv_normalizer_util_is_normalizable_character() {
    const char *normalizable_character = "a";
    const char *non_normalizable_character = "文";
    bool result_1 = pv_normalizer_util_ko_is_normalizable_character(normalizable_character);
    bool result_2 = pv_normalizer_util_ko_is_normalizable_character(non_normalizable_character);
    pv_test_true(result_1 == true, "failed to recognize the Korean normalizable character '%s'", normalizable_character);
    pv_test_true(result_2 == false, "failed to recognize the Korean non-normalizable character '%s'", non_normalizable_character);
}

static const pv_test_case_t PV_NORMALIZER_UTIL_KO_TEST_CASES[] = {
        {"is_special_character", test_pv_normalizer_util_is_special_character},
        {"is_punctuation", test_pv_normalizer_util_is_punctuation},
        {"is_normalizable_character", test_pv_normalizer_util_is_normalizable_character},
};

const pv_test_suite_t PV_NORMALIZER_UTIL_KO_TEST_SUITE = {
        .name = "normalizer_util_ko",
        .setup = NULL,
        .teardown = NULL,
        .num_test_cases = PV_ARRAY_LEN(PV_NORMALIZER_UTIL_KO_TEST_CASES),
        .test_cases = PV_NORMALIZER_UTIL_KO_TEST_CASES,
};
