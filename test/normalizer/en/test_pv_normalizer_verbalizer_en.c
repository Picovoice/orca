#include <stdlib.h>
#include <string.h>

#include "core/pv_language.h"
#include "core/pv_language_json.h"
#include "mock/pv_mock.h"
#include "orca/normalizer/en/pv_normalizer_tags_en.h"
#include "orca/normalizer/en/pv_normalizer_verbalizer_en.h"
#include "orca/normalizer/pv_normalizer_tagger.h"
#include "orca/normalizer/pv_normalizer_tokenizer.h"
#include "orca/normalizer/pv_normalizer_verbalizer.h"
#include "test/pv_test.h"

#ifdef __PV_MOCKS__

#include "core/mock/pv_core_runtime_mock.h"
#include "orca/mock/pv_normalizer_mock.h"

#endif

static pv_normalizer_verbalizer_t *normalizer_verbalizer_object = NULL;
static int32_t ALL_TEST_USE_CASES[] = {
        PV_NORMALIZER_USE_WORD_NORMALIZER,
        PV_NORMALIZER_USE_PUNCTUATION_NORMALIZER,
        PV_NORMALIZER_USE_CARDINAL_NORMALIZER,
        PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER,
        PV_NORMALIZER_USE_CUSTOM_PRONUNCIATION_NORMALIZER,
        PV_NORMALIZER_USE_NUMBER_RANGE_NORMALIZER,
        PV_NORMALIZER_USE_ORDINAL_NORMALIZER,
        PV_NORMALIZER_USE_NEGATIVE_ORDINAL_NORMALIZER,
        PV_NORMALIZER_USE_SPECIAL_CHARACTER_NORMALIZER,
        PV_NORMALIZER_USE_DECIMAL_NORMALIZER,
        PV_NORMALIZER_USE_ALPHANUM_SPELL_OUT_NORMALIZER,
        PV_NORMALIZER_USE_MEASUREMENT_NORMALIZER,
        PV_NORMALIZER_USE_TIME_NORMALIZER,
        PV_NORMALIZER_USE_FRACTION_NORMALIZER,
        PV_NORMALIZER_USE_URL_NORMALIZER,
        PV_NORMALIZER_USE_CURRENCY_NORMALIZER,
        PV_NORMALIZER_USE_ABBREVIATION_NORMALIZER,
        PV_NORMALIZER_USE_DIGITS_SEQUENCE_NORMALIZER,
        PV_NORMALIZER_USE_DATE_NORMALIZER,
        PV_NORMALIZER_USE_NAME_NORMALIZER,
};

static pv_normalizer_tagger_t *normalizer_tagger_object = NULL;
static pv_normalizer_tokenizer_t *normalizer_tokenizer_object = NULL;
static pv_language_info_t *language_info_object = NULL;

static const char LANGUAGE_INFO_PATH[] = "normalizer/ref/pv_language_info_normalizer_en.json";

static const char VERBALIZER_TEST_SENTENCE[] = "i'm taking vitamin b-12, c, and 15 others!";
static const char *VERBALIZER_TEST_SENTENCE_VERBALIZED[] = {
        "I'M",
        "TAKING",
        "VITAMIN",
        "B",
        "TWELVE",
        ",",
        "C",
        ",",
        "AND",
        "FIFTEEN",
        "OTHERS",
        "!"};

static void *calloc_return_null(size_t arg0, size_t arg1) {
    (void) arg0;
    (void) arg1;
    return NULL;
}

static pv_status_t test_pv_normalizer_verbalizer_setup(void) {
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

    status = pv_normalizer_tokenizer_init(language_info_object, NULL, &normalizer_tokenizer_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_normalizer_tagger_init(
            language_info_object,
            PV_ARRAY_LEN(ALL_TEST_USE_CASES),
            ALL_TEST_USE_CASES,
            normalizer_tokenizer_object,
            NULL,
            &normalizer_tagger_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_normalizer_verbalizer_init(
            PV_NORMALIZER_LANGUAGE_EN,
            PV_ARRAY_LEN(ALL_TEST_USE_CASES),
            ALL_TEST_USE_CASES,
            NULL,
            &normalizer_verbalizer_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_normalizer_verbalizer_teardown(void) {
    pv_normalizer_tokenizer_delete(normalizer_tokenizer_object);
    normalizer_tokenizer_object = NULL;

    pv_normalizer_tagger_delete(normalizer_tagger_object);
    normalizer_tagger_object = NULL;

    pv_normalizer_verbalizer_delete(normalizer_verbalizer_object);
    normalizer_verbalizer_object = NULL;
}

static void test_pv_normalizer_verbalizer_init_helper(
        int32_t num_use_cases,
        int32_t *use_cases,
        pv_normalizer_tagger_t **tagger,
        pv_normalizer_verbalizer_t **verbalizer) {
    *tagger = NULL;
    *verbalizer = NULL;

    pv_status_t status = pv_normalizer_tagger_init(
            language_info_object,
            num_use_cases,
            use_cases,
            normalizer_tokenizer_object,
            NULL,
            tagger);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to initialize tagger");

    status = pv_normalizer_verbalizer_init(
            PV_NORMALIZER_LANGUAGE_EN,
            num_use_cases,
            use_cases,
            NULL,
            verbalizer);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to initialize verbalizer");
}

static void test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
        pv_normalizer_tokenizer_t *tokenizer,
        pv_normalizer_tagger_t *tagger,
        pv_normalizer_verbalizer_t *verbalizer,
        const char *sentence,
        int32_t target_num_tokens,
        bool preserve_word_boundary,
        const char **sentence_verbalize) {
    pv_normalizer_token_list_t *token_list = NULL;

    pv_status_t status = pv_normalizer_tokenizer_tokenize(
            tokenizer,
            sentence,
            pv_normalizer_tokenizer_default_word_boundary_character(normalizer_tokenizer_object),
            preserve_word_boundary,
            false,
            false,
            true,
            &token_list);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to tokenize sentence: `%s`", sentence);

    status = pv_normalizer_tagger_tag(tagger, token_list, 0, true);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to tag sentence: `%s`", sentence);
    pv_test_true(
            token_list->size == target_num_tokens,
            "incorrect number of tokens. got `%d`, expected `%d`",
            token_list->size,
            target_num_tokens);

    status = pv_normalizer_verbalizer_verbalize(verbalizer, token_list, 0);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to verbalize sentence: `%s`", sentence);
    pv_normalizer_token_t *current = token_list->head;
    for (int32_t i = 0; i < token_list->size; i++) {
        pv_test_true(current != NULL, "token is empty");
        if (sentence_verbalize[i] == NULL) {
            pv_test_true(
                    current->verbalized == sentence_verbalize[i],
                    "incorrect verbalized: got `%s`, expected `%s`",
                    current->verbalized,
                    sentence_verbalize[i]);
        } else {
            if (current->verbalized == NULL) {
                pv_test_true(false, "verbalized is empty but expected to be `%s`", sentence_verbalize[i]);
            } else {
                pv_test_true(
                        strcmp(current->verbalized, sentence_verbalize[i]) == 0,
                        "incorrect verbalized: got `%s`, expected `%s`, at index `%d`",
                        current->verbalized,
                        sentence_verbalize[i],
                        i);
            }
        }
        current = current->next;
    }
    pv_normalizer_token_list_delete(token_list);
}

static void test_pv_normalizer_verbalizer_verbalize_and_check_status_helper(
        pv_normalizer_tokenizer_t *tokenizer,
        pv_normalizer_tagger_t *tagger,
        pv_normalizer_verbalizer_t *verbalizer,
        const char *sentence,
        pv_status_t target_tagger_status,
        pv_status_t target_verbalizer_status) {
    pv_normalizer_token_list_t *token_list = NULL;

    pv_status_t status = pv_normalizer_tokenizer_tokenize(
            tokenizer,
            sentence,
            pv_normalizer_tokenizer_default_word_boundary_character(normalizer_tokenizer_object),
            false,
            false,
            false,
            false,
            &token_list);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to tokenize sentence: `%s`", sentence);

    status = pv_normalizer_tagger_tag(tagger, token_list, 0, true);
    pv_test_true(
            status == target_tagger_status,
            "raised %s instead of %s",
            pv_status_to_string(status),
            pv_status_to_string(target_tagger_status));

    status = pv_normalizer_verbalizer_verbalize(verbalizer, token_list, 0);
    pv_test_true(
            status == target_verbalizer_status,
            "raised %s instead of %s",
            pv_status_to_string(status),
            pv_status_to_string(target_verbalizer_status));

    pv_normalizer_token_list_delete(token_list);
}


static void test_pv_normalizer_verbalizer_verbalize_all(void) {
    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            VERBALIZER_TEST_SENTENCE,
            PV_ARRAY_LEN(VERBALIZER_TEST_SENTENCE_VERBALIZED),
            false,
            VERBALIZER_TEST_SENTENCE_VERBALIZED);
}

static void test_pv_normalizer_verbalizer_verbalize_word(void) {
    int32_t use_cases[] = {PV_NORMALIZER_USE_WORD_NORMALIZER};
    static const char sentence[] = "i'm taking vitamin b-12, c, and 15 others! Don’t";

    const char *sentence_verbalized[] = {
            "I'M",
            "TAKING",
            "VITAMIN",
            "B",
            NULL,
            NULL,
            "C",
            NULL,
            "AND",
            NULL,
            "OTHERS",
            NULL,
            "DON'T",
    };

    pv_normalizer_tagger_t *tagger = NULL;
    pv_normalizer_verbalizer_t *verbalizer = NULL;
    test_pv_normalizer_verbalizer_init_helper(PV_ARRAY_LEN(use_cases), use_cases, &tagger, &verbalizer);

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            tagger,
            verbalizer,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            false,
            sentence_verbalized);

    pv_normalizer_tagger_delete(tagger);
    pv_normalizer_verbalizer_delete(verbalizer);
}

static void test_pv_normalizer_verbalizer_verbalize_punctuation(void) {
    const char *sentence_verbalized[] = {
            "I'M",
            "TAKING",
            "VITAMIN",
            "B",
            "TWELVE",
            ",",
            "C",
            ",",
            "AND",
            "FIFTEEN",
            "OTHERS",
            "!"
    };

    pv_normalizer_tagger_t *tagger = NULL;
    pv_normalizer_verbalizer_t *verbalizer = NULL;
    test_pv_normalizer_verbalizer_init_helper(PV_ARRAY_LEN(ALL_TEST_USE_CASES), ALL_TEST_USE_CASES, &tagger, &verbalizer);

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            tagger,
            verbalizer,
            VERBALIZER_TEST_SENTENCE,
            PV_ARRAY_LEN(sentence_verbalized),
            false,
            sentence_verbalized);

    pv_normalizer_tagger_delete(tagger);
    pv_normalizer_verbalizer_delete(verbalizer);
}

static void test_pv_normalizer_verbalizer_verbalize_custom_pronunciation(void) {
    int32_t use_cases[] = {PV_NORMALIZER_USE_CUSTOM_PRONUNCIATION_NORMALIZER};

    const char sentence[] = "A {custom|K AH S T AH M} pronunciation";
    const char *sentence_verbalized[] = {NULL, "{K AH S T AH M}", NULL};

    pv_normalizer_tagger_t *tagger = NULL;
    pv_normalizer_verbalizer_t *verbalizer = NULL;
    test_pv_normalizer_verbalizer_init_helper(PV_ARRAY_LEN(use_cases), use_cases, &tagger, &verbalizer);

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            tagger,
            verbalizer,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            false,
            sentence_verbalized);

    pv_normalizer_tagger_delete(tagger);
    pv_normalizer_verbalizer_delete(verbalizer);
}

static void test_pv_normalizer_verbalizer_verbalize_cardinal(void) {
    const char *sentence_verbalized[] = {
            "I'M",
            "TAKING",
            "VITAMIN",
            "B",
            "TWELVE",
            ",",
            "C",
            ",",
            "AND",
            "FIFTEEN",
            "OTHERS",
            "!"
    };

    pv_normalizer_tagger_t *tagger = NULL;
    pv_normalizer_verbalizer_t *verbalizer = NULL;
    test_pv_normalizer_verbalizer_init_helper(PV_ARRAY_LEN(ALL_TEST_USE_CASES), ALL_TEST_USE_CASES, &tagger, &verbalizer);

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            tagger,
            verbalizer,
            VERBALIZER_TEST_SENTENCE,
            PV_ARRAY_LEN(sentence_verbalized),
            false,
            sentence_verbalized);

    pv_normalizer_tagger_delete(tagger);
    pv_normalizer_verbalizer_delete(verbalizer);
}

static void test_pv_normalizer_verbalizer_number_to_string(void) {
    const char *sentence = "36 130 0 003 123456789 5242 999999999999999 1200000000000009 11111111111111111";
    const char *sentence_verbalized[] = {
            "THIRTY SIX",
            "ONE HUNDRED THIRTY",
            "ZERO",
            "ZERO ZERO THREE",
            "ONE HUNDRED TWENTY THREE MILLION FOUR HUNDRED FIFTY SIX THOUSAND SEVEN HUNDRED EIGHTY NINE",
            "FIVE THOUSAND TWO HUNDRED FORTY TWO",
            "NINE HUNDRED NINETY NINE TRILLION NINE HUNDRED NINETY NINE BILLION NINE HUNDRED NINETY NINE MILLION NINE HUNDRED NINETY NINE THOUSAND NINE HUNDRED NINETY NINE",
            "ONE TWO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO NINE",
            "ONE ONE ONE ONE ONE ONE ONE ONE ONE ONE ONE ONE ONE ONE ONE ONE ONE"};

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            false,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_negative_cardinal(void) {
    int32_t use_cases[] = {PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER};

    const char sentence[] = "123 -123";
    const char *sentence_verbalized[] = {NULL, "NEGATIVE ONE HUNDRED TWENTY THREE"};

    pv_normalizer_tagger_t *tagger = NULL;
    pv_normalizer_verbalizer_t *verbalizer = NULL;
    test_pv_normalizer_verbalizer_init_helper(PV_ARRAY_LEN(use_cases), use_cases, &tagger, &verbalizer);

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            tagger,
            verbalizer,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            false,
            sentence_verbalized);

    pv_normalizer_tagger_delete(tagger);
    pv_normalizer_verbalizer_delete(verbalizer);
}

static void test_pv_normalizer_verbalizer_verbalize_number_range(void) {
    int32_t use_cases[] = {
            PV_NORMALIZER_USE_NUMBER_RANGE_NORMALIZER,
            PV_NORMALIZER_USE_CARDINAL_NORMALIZER,
            PV_NORMALIZER_USE_DECIMAL_NORMALIZER};

    const char sentence[] = "12-5 1-44 1.1-1.3";
    const char *sentence_verbalized[] = {
            "TWELVE",
            "TO",
            "FIVE",
            "ONE",
            "TO",
            "FORTY FOUR",
            "ONE",
            "POINT",
            "ONE",
            "TO",
            "ONE",
            "POINT",
            "THREE",
    };

    pv_normalizer_tagger_t *tagger = NULL;
    pv_normalizer_verbalizer_t *verbalizer = NULL;
    test_pv_normalizer_verbalizer_init_helper(PV_ARRAY_LEN(use_cases), use_cases, &tagger, &verbalizer);

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            tagger,
            verbalizer,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            false,
            sentence_verbalized);

    pv_normalizer_tagger_delete(tagger);
    pv_normalizer_verbalizer_delete(verbalizer);
}

static void test_pv_normalizer_verbalizer_verbalize_invalid_use_case(void) {
    int32_t use_cases[] = {99};

    pv_normalizer_tagger_t *tagger = NULL;
    pv_normalizer_verbalizer_t *verbalizer = NULL;
    test_pv_normalizer_verbalizer_init_helper(PV_ARRAY_LEN(use_cases), use_cases, &tagger, &verbalizer);

    test_pv_normalizer_verbalizer_verbalize_and_check_status_helper(
            normalizer_tokenizer_object,
            tagger,
            verbalizer,
            VERBALIZER_TEST_SENTENCE,
            PV_STATUS_INVALID_ARGUMENT,
            PV_STATUS_INVALID_ARGUMENT);

    pv_normalizer_tagger_delete(tagger);
    pv_normalizer_verbalizer_delete(verbalizer);
}

static void test_pv_normalizer_verbalizer_verbalize_ordinal(void) {
    int32_t use_cases[] = {
            PV_NORMALIZER_USE_ORDINAL_NORMALIZER,
            PV_NORMALIZER_USE_WORD_NORMALIZER,
            PV_NORMALIZER_USE_CARDINAL_NORMALIZER,
            PV_NORMALIZER_USE_PUNCTUATION_NORMALIZER,
    };

    const char *sentence =
            "1st 22nd 19th 200th 4000th 1000000th 6000000000th 5000000000000th 123456789th 0th";
    const char *sentence_verbalized[] = {
            "FIRST",
            NULL,
            "TWENTY SECOND",
            NULL,
            "NINETEENTH",
            NULL,
            "TWO HUNDREDTH",
            NULL,
            "FOUR THOUSANDTH",
            NULL,
            "ONE MILLIONTH",
            NULL,
            "SIX BILLIONTH",
            NULL,
            "FIVE TRILLIONTH",
            NULL,
            "ONE HUNDRED TWENTY THREE MILLION FOUR HUNDRED FIFTY SIX THOUSAND SEVEN HUNDRED EIGHTY NINTH",
            NULL,
            "ZEROTH",
    };

    pv_normalizer_tagger_t *tagger = NULL;
    pv_normalizer_verbalizer_t *verbalizer = NULL;
    test_pv_normalizer_verbalizer_init_helper(PV_ARRAY_LEN(use_cases), use_cases, &tagger, &verbalizer);

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            tagger,
            verbalizer,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            true,
            sentence_verbalized);

    pv_normalizer_tagger_delete(tagger);
    pv_normalizer_verbalizer_delete(verbalizer);
}

static void test_pv_normalizer_verbalizer_verbalize_invalid_ordinal(void) {
    static int32_t use_cases[] = {PV_NORMALIZER_USE_ORDINAL_NORMALIZER};

    char *sentences[] = {"01st", "1200000000000009th"};

    pv_normalizer_tagger_t *tagger = NULL;
    pv_normalizer_verbalizer_t *verbalizer = NULL;
    test_pv_normalizer_verbalizer_init_helper(PV_ARRAY_LEN(use_cases), use_cases, &tagger, &verbalizer);

    for (int32_t i = 0; i < PV_ARRAY_LEN(sentences); i++) {
        test_pv_normalizer_verbalizer_verbalize_and_check_status_helper(
                normalizer_tokenizer_object,
                tagger,
                verbalizer,
                sentences[i],
                PV_STATUS_SUCCESS,
                PV_STATUS_SUCCESS);
    }

    pv_normalizer_tagger_delete(tagger);
    pv_normalizer_verbalizer_delete(verbalizer);
}

static void test_pv_normalizer_verbalizer_verbalize_negative_ordinal(void) {
    int32_t use_cases[] = {PV_NORMALIZER_USE_NEGATIVE_ORDINAL_NORMALIZER};

    const char *sentence = "-1st -22nd  -5000000000000th -123456789th";
    const char *sentence_verbalized[] = {
            "NEGATIVE FIRST",
            "NEGATIVE TWENTY SECOND",
            "NEGATIVE FIVE TRILLIONTH",
            "NEGATIVE ONE HUNDRED TWENTY THREE MILLION FOUR HUNDRED FIFTY SIX THOUSAND SEVEN HUNDRED EIGHTY NINTH",
    };

    pv_normalizer_tagger_t *tagger = NULL;
    pv_normalizer_verbalizer_t *verbalizer = NULL;
    test_pv_normalizer_verbalizer_init_helper(PV_ARRAY_LEN(use_cases), use_cases, &tagger, &verbalizer);

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            tagger,
            verbalizer,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            false,
            sentence_verbalized);

    pv_normalizer_tagger_delete(tagger);
    pv_normalizer_verbalizer_delete(verbalizer);
}

static void test_pv_normalizer_verbalizer_verbalize_special_characters(void) {
    int32_t use_cases[] = {PV_NORMALIZER_USE_SPECIAL_CHARACTER_NORMALIZER};

    const char *sentence = "& % @ \n _ ( ) °C RPM —";
    const char *sentence_verbalized[] = {
            "AND",
            "PERCENT",
            "AT",
            ".",
            "UNDERSCORE",
            ",",
            ",",
            "DEGREES CELSIUS",
            "REVOLUTIONS PER MINUTE",
            ",",
    };

    pv_normalizer_tagger_t *tagger = NULL;
    pv_normalizer_verbalizer_t *verbalizer = NULL;
    test_pv_normalizer_verbalizer_init_helper(PV_ARRAY_LEN(use_cases), use_cases, &tagger, &verbalizer);

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            tagger,
            verbalizer,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            false,
            sentence_verbalized);

    pv_normalizer_tagger_delete(tagger);
    pv_normalizer_verbalizer_delete(verbalizer);
}

static void test_pv_normalizer_verbalizer_verbalize_decimal(void) {
    const char *sentence = "1.23 .5 10.1% -2.2";
    const char *sentence_verbalized[] = {
            "ONE",
            "POINT",
            "TWO THREE",
            "POINT",
            "FIVE",
            "TEN",
            "POINT",
            "ONE",
            "PERCENT",
            "NEGATIVE TWO",
            "POINT",
            "TWO"};

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            false,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_measurement(void) {
    const char sentence[] = "5g 1 ml 7.3L 3 ft 10.1km 5°C 800 ml 1,000g 25ft.-lb.";
    const char *sentence_verbalized[] = {
            "FIVE",
            "GRAMS",
            "ONE",
            "MILLILITER",
            "SEVEN",
            "POINT",
            "THREE",
            "LITERS",
            "THREE",
            "FEET",
            "TEN",
            "POINT",
            "ONE",
            "KILOMETERS",
            "FIVE",
            "DEGREES CELSIUS",
            "EIGHT HUNDRED",
            "MILLILITERS",
            "ONE THOUSAND",
            "GRAMS",
            "TWENTY FIVE",
            "FOOT",
            NULL,
            "POUNDS",
            ".",
    };

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            false,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_per_measurement(void) {
    const char sentence[] = "5 km/m 10 oz/km 1.1m/l -5.41kg/ft tb/hz 1,000,000m/s";
    const char *sentence_verbalized[] = {
            "FIVE",
            "KILOMETERS",
            "PER",
            "METER",
            "TEN",
            "OUNCES",
            "PER",
            "KILOMETER",
            "ONE",
            "POINT",
            "ONE",
            "METERS",
            "PER",
            "LITER",
            "NEGATIVE FIVE",
            "POINT",
            "FOUR ONE",
            "KILOGRAMS",
            "PER",
            "FOOT",
            "TERABYTES",
            "PER",
            "HERTZ",
            "ONE MILLION",
            "METERS",
            "PER",
            "SECOND",
    };

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            false,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_alphanum_spell_out(void) {
    const char *sentence = "HTML5 C5B 12A Z78 400BC";
    const char *sentence_verbalized[] = {
            "H", "T", "M", "L", "FIVE",
            NULL,
            "C", "FIVE", "B",
            NULL,
            "ONE", "TWO", "A",
            NULL,
            "Z", "SEVEN", "EIGHT",
            NULL,
            "FOUR HUNDRED", "B", "C"};

    pv_normalizer_tagger_t *tagger = NULL;
    pv_normalizer_verbalizer_t *verbalizer = NULL;
    test_pv_normalizer_verbalizer_init_helper(
            PV_ARRAY_LEN(ALL_TEST_USE_CASES),
            ALL_TEST_USE_CASES,
            &tagger,
            &verbalizer);
    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            tagger,
            verbalizer,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            true,
            sentence_verbalized);

    pv_normalizer_tagger_delete(tagger);
    pv_normalizer_verbalizer_delete(verbalizer);
}

static void test_pv_normalizer_verbalizer_verbalize_time(void) {
    const char *sentence = "7:07 a.m. 8 PM 89 P.M. 2:01 P.M. 11:15 03:00 p.m. 03:14PM 9am 11:30pm";
    const char *sentence_verbalized[] = {
            "SEVEN",
            NULL,
            "OH SEVEN",
            NULL,
            "AM",
            NULL,
            "EIGHT",
            NULL,
            "PM",
            NULL,
            "EIGHTY NINE",
            NULL,
            "P",
            NULL,
            "M",
            NULL,
            NULL,
            "TWO",
            NULL,
            "OH ONE",
            NULL,
            "PM",
            NULL,
            "ELEVEN",
            NULL,
            "FIFTEEN",
            NULL,
            "THREE",
            NULL,
            NULL,
            NULL,
            "PM",
            NULL,
            "THREE",
            NULL,
            "FOURTEEN",
            "PM",
            NULL,
            "NINE",
            "AM",
            NULL,
            "ELEVEN",
            NULL,
            "THIRTY",
            "PM",
    };

    pv_normalizer_tagger_t *tagger = NULL;
    pv_normalizer_verbalizer_t *verbalizer = NULL;
    test_pv_normalizer_verbalizer_init_helper(
            PV_ARRAY_LEN(ALL_TEST_USE_CASES),
            ALL_TEST_USE_CASES,
            &tagger,
            &verbalizer);
    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            tagger,
            verbalizer,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            true,
            sentence_verbalized);

    pv_normalizer_tagger_delete(tagger);
    pv_normalizer_verbalizer_delete(verbalizer);
}

static void test_pv_normalizer_verbalizer_verbalize_comma_cardinal(void) {
    const char *sentence = "1,005 11,005 111,005 -5,000,000,000 1111,000 1,1,000 1, 000";
    const char *sentence_verbalized[] = {
            "ONE THOUSAND FIVE",
            "ELEVEN THOUSAND FIVE",
            "ONE HUNDRED ELEVEN THOUSAND FIVE",
            "NEGATIVE FIVE BILLION",
            "ONE THOUSAND ONE HUNDRED ELEVEN",
            ",",
            "ZERO ZERO ZERO",
            "ONE",
            ",",
            "ONE",
            ",",
            "ZERO ZERO ZERO",
            "ONE",
            ",",
            "ZERO ZERO ZERO",
    };

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            false,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_comma_ordinal(void) {
    const char *sentence = "1,005th 11,005th 111,005th -5,000,000,003rd 1111,100th 1,1,100th 1, 100th";
    const char *sentence_verbalized[] = {
            "ONE THOUSAND FIFTH",
            "ELEVEN THOUSAND FIFTH",
            "ONE HUNDRED ELEVEN THOUSAND FIFTH",
            "NEGATIVE FIVE BILLION THIRD",
            "ONE THOUSAND ONE HUNDRED ELEVEN",
            ",",
            "ONE HUNDREDTH",
            "ONE",
            ",",
            "ONE",
            ",",
            "ONE HUNDREDTH",
            "ONE",
            ",",
            "ONE HUNDREDTH",
    };

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            false,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_comma_decimal(void) {
    const char *sentence = "1,000.2 -300,000.6666";
    const char *sentence_verbalized[] = {
            "ONE THOUSAND",
            "POINT",
            "TWO",
            "NEGATIVE THREE HUNDRED THOUSAND",
            "POINT",
            "SIX SIX SIX SIX",
    };

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            false,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_comma_number_range(void) {
    const char *sentence = "1,000-2,000 1,000.2-1,000.5 10,000-1.3";
    const char *sentence_verbalized[] = {
            "ONE THOUSAND",
            "TO",
            "TWO THOUSAND",
            "ONE THOUSAND",
            "POINT",
            "TWO",
            "TO",
            "ONE THOUSAND",
            "POINT",
            "FIVE",
            "TEN THOUSAND",
            "TO",
            "ONE",
            "POINT",
            "THREE",
    };

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            false,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_fraction(void) {
    const char *sentence = "1/2 9/3.2 1/1,000 1/-4 -4/3";
    const char *sentence_verbalized[] = {
            "ONE",
            NULL,
            "HALF",
            "NINE",
            "OVER",
            "THREE",
            "POINT",
            "TWO",
            "ONE",
            NULL,
            "THOUSANDTH",
            "ONE",
            "OVER",
            "NEGATIVE FOUR",
            "NEGATIVE FOUR",
            NULL,
            "THIRDS"};

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            false,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_url(void) {
    const char *sentence = "www.example.com https://hello.ca/";
    const char *sentence_verbalized[] = {
            "W",
            "W",
            "W",
            "DOT",
            "EXAMPLE",
            "DOT",
            "COM",
            "H",
            "T",
            "T",
            "P",
            "S",
            "COLON",
            "SLASH",
            "SLASH",
            "HELLO",
            "DOT",
            "C",
            "A",
            "SLASH",
    };

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            false,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_currency(void) {
    const char *sentence =
            "$5 €10 10$ 10€ $1.25 1.25$ -$500 -500$ -1.25$ -$1.25 €1,000 -€1,000 1,000€ -1,000€ €1,000.25 1,000.25€ "
            "¥2 ₪2 £2 ₩2 ₺2 ₱2 ₽2 ฿2 ₴2 ₹2 ¢2 $1.00 $1.01 5 EUR 1.25 £ -3 €";
    const char *sentence_verbalized[] = {
            "FIVE DOLLARS",
            NULL,
            "TEN EUROS",
            NULL,
            "TEN DOLLARS",
            NULL,
            "TEN EUROS",
            NULL,
            "ONE DOLLAR AND TWENTY FIVE CENTS",
            NULL,
            "ONE DOLLAR AND TWENTY FIVE CENTS",
            NULL,
            "NEGATIVE FIVE HUNDRED DOLLARS",
            NULL,
            "NEGATIVE FIVE HUNDRED DOLLARS",
            NULL,
            "NEGATIVE ONE DOLLAR AND TWENTY FIVE CENTS",
            NULL,
            "NEGATIVE ONE DOLLAR AND TWENTY FIVE CENTS",
            NULL,
            "ONE THOUSAND EUROS",
            NULL,
            "NEGATIVE ONE THOUSAND EUROS",
            NULL,
            "ONE THOUSAND EUROS",
            NULL,
            "NEGATIVE ONE THOUSAND EUROS",
            NULL,
            "ONE THOUSAND EUROS AND TWENTY FIVE CENTS",
            NULL,
            "ONE THOUSAND EUROS AND TWENTY FIVE CENTS",
            NULL,
            "TWO YUAN",
            NULL,
            "TWO SHEKELS",
            NULL,
            "TWO POUNDS",
            NULL,
            "TWO WON",
            NULL,
            "TWO LIRA",
            NULL,
            "TWO PESOS",
            NULL,
            "TWO RUBLES",
            NULL,
            "TWO BAHT",
            NULL,
            "TWO HRYVNIAS",
            NULL,
            "TWO RUPEES",
            NULL,
            "TWO CENTS",
            NULL,
            "ONE DOLLAR",
            NULL,
            "ONE DOLLAR AND ONE CENT",
            NULL,
            "FIVE",
            NULL,
            "EUROS",
            NULL,
            "ONE",
            "POINT",
            "TWO FIVE",
            NULL,
            "POUNDS",
            NULL,
            "NEGATIVE THREE",
            NULL,
            "EUROS",
    };

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            true,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_abbreviation(void) {
    const char *sentence = " e.g. is Prof. Dr. Doe Ave. St. vs. etc.";
    const char *sentence_verbalized[] = {
            NULL,
            "FOR EXAMPLE",
            NULL,
            "IS",
            NULL,
            "PROFESSOR",
            NULL,
            "DOCTOR",
            NULL,
            "DOE",
            NULL,
            "AVENUE",
            NULL,
            "STREET",
            NULL,
            "VERSUS",
            NULL,
            "ETCETERA",
    };

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            true,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_digits_sequence(void) {
    const char *sentence = " (772) 778-1923 1-800-123-4567 123-99456-7890 893.456.7890 +1 (381) 102-129";
    const char *sentence_verbalized[] = {
            NULL,
            ", SEVEN SEVEN TWO,",
            NULL,
            "SEVEN SEVEN EIGHT",
            ",",
            "ONE NINE TWO THREE",
            NULL,
            "ONE",
            ",",
            "EIGHT ZERO ZERO",
            ",",
            "ONE TWO THREE",
            ",",
            "FOUR FIVE SIX SEVEN",
            NULL,
            "ONE TWO THREE",
            ",",
            "NINE NINE FOUR FIVE SIX",
            ",",
            "SEVEN EIGHT NINE ZERO",
            NULL,
            "EIGHT NINE THREE",
            ",",
            "FOUR FIVE SIX",
            ",",
            "SEVEN EIGHT NINE ZERO",
            NULL,
            "PLUS",
            "ONE",
            NULL,
            ", THREE EIGHT ONE,",
            NULL,
            "ONE ZERO TWO",
            ",",
            "ONE TWO NINE",
    };

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            true,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_date(void) {
    const char *sentence = "03-Jun-2020 08/02/1877 2013-11-28 13/01/1992 Aug 1 32-10-2024 Oct 2, 1800 09-21-1907";
    const char *sentence_verbalized[] = {
            "THE THIRD OF",
            NULL,
            "JUNE ,",
            NULL,
            "TWENTY TWENTY",
            NULL,
            "AUGUST",
            NULL,
            "SECOND ,",
            NULL,
            "EIGHTEEN SEVENTY SEVEN",
            NULL,
            NULL,
            NULL,
            "NOVEMBER",
            NULL,
            "TWENTY EIGHTH , TWENTY THIRTEEN",
            NULL,
            "THE THIRTEENTH OF",
            NULL,
            "JANUARY ,",
            NULL,
            "NINETEEN NINETY TWO",
            NULL,
            "AUGUST",
            NULL,
            "FIRST",
            NULL,
            "THREE TWO",
            ",",
            "ONE ZERO",
            ",",
            "TWO ZERO TWO FOUR",
            NULL,
            "OCTOBER",
            NULL,
            "SECOND",
            ",",
            NULL,
            "EIGHTEEN HUNDRED",
            NULL,
            "SEPTEMBER",
            NULL,
            "TWENTY FIRST ,",
            NULL,
            "NINETEEN OH SEVEN",
    };

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            true,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_name(void) {
    const char *sentence = "J. K. Rowling, John F. Kennedy";
    const char *sentence_verbalized[] = {
            "J",
            NULL,
            NULL,
            "K",
            NULL,
            NULL,
            "ROWLING",
            ",",
            NULL,
            "JOHN",
            NULL,
            "F",
            NULL,
            NULL,
            "KENNEDY",
    };

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            true,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_comma_cardinal_punctuation(void) {
    const char *sentence = "1,005, 11,002: 111,005.";
    const char *sentence_verbalized[] = {
            "ONE THOUSAND FIVE", ",",
            "ELEVEN THOUSAND TWO", ":",
            "ONE HUNDRED ELEVEN THOUSAND FIVE", "."};

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            false,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_tag_additional_date(void) {
    const char *sentence = "June 2020, September 1817. March 15th, 2023, October 5th, 2023. The 8 May 2023, 31 January 2020.";
    const char *sentence_verbalized[] = {
            "JUNE",
            "TWENTY TWENTY",
            ",",
            "SEPTEMBER",
            "EIGHTEEN SEVENTEEN",
            ".",
            "MARCH",
            "FIFTEENTH",
            ",",
            "TWENTY TWENTY THREE",
            ",",
            "OCTOBER",
            "FIFTH",
            ",",
            "TWENTY TWENTY THREE",
            ".",
            "THE",
            "EIGHTH OF",
            "MAY ,",
            "TWENTY TWENTY THREE",
            ",",
            "THE THIRTY FIRST OF",
            "JANUARY ,",
            "TWENTY TWENTY",
            ".",
    };

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            false,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_domain_extensions_only(void) {
    const char *sentence = "such as .com, .net, and .org";
    const char *sentence_verbalized[] = {
            "SUCH",
            "AS",
            "DOT",
            "COM",
            ",",
            "DOT",
            "NET",
            ",",
            "AND",
            "DOT",
            "ORG",
    };

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            false,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_temperature_punctuation(void) {
    const char *sentence = "about 10-15°C. (or 22°C).";
    const char *sentence_verbalized[] = {
            "ABOUT",
            NULL,
            "TEN",
            "TO",
            "FIFTEEN",
            "DEGREES CELSIUS",
            ".",
            NULL,
            ",",
            "OR",
            NULL,
            "TWENTY TWO",
            "DEGREES CELSIUS",
            ",",
            ".",
    };

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            true,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_cardinal_in_parentheses(void) {
    const char *sentence = "feet (8,848 meters).";
    const char *sentence_verbalized[] = {
            "FEET",
            ",",
            "EIGHT THOUSAND EIGHT HUNDRED FORTY EIGHT",
            "METERS",
            ",",
            ".",
    };

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            false,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_single_quote(void) {
    const char *sentence = "time to ask, 'Where you at?' '(hello)'";
    const char *sentence_verbalized[] = {
            "TIME",
            "TO",
            "ASK",
            ",",
            "'WHERE",
            "YOU",
            "AT",
            "?",
            "\"",
            "\"",
            ",",
            "HELLO",
            ",",
            "\"",
    };

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            false,
            sentence_verbalized);
}

#ifdef __PV_MOCKS__

static void test_pv_normalizer_verbalizer_init_failure_helper(
        pv_status_t expected_status,
        const char *expected_public_message_regex,
        const char *expected_private_message_regex) {
    pv_normalizer_verbalizer_t *normalizer_verbalizer = NULL;
    pv_status_t status = pv_normalizer_verbalizer_init(
            PV_NORMALIZER_LANGUAGE_EN,
            PV_ARRAY_LEN(ALL_TEST_USE_CASES),
            ALL_TEST_USE_CASES,
            NULL,
            &normalizer_verbalizer);
    reset_mocks();
    pv_test_true(
            status == expected_status,
            "init internal error, expected `%s` got `%s`",
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
                false,
                "error message mismatch, expected '%s'",
                expected_message);
    }
}

static void test_pv_normalizer_verbalizer_input_setup_helper(
        pv_normalizer_token_list_t **token_list,
        char *string,
        pv_normalizer_token_tag_en_t tag) {
    *token_list = NULL;
    pv_status_t status = pv_normalizer_token_list_init(token_list);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to initialize token list");

    pv_normalizer_token_t *token = NULL;
    status = pv_normalizer_token_init(
            0,
            5,
            string,
            false,
            false,
            true,
            &token);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to initialize token");

    token->tag = tag;

    pv_normalizer_token_list_append_token(*token_list, token);
}

static void test_pv_normalizer_verbalizer_verbalize_helper(
        pv_normalizer_token_list_t *token_list,
        pv_status_t expected_status,
        const char *expected_public_message_regex,
        const char *expected_private_message_regex) {

    pv_status_t status = pv_normalizer_verbalizer_verbalize(normalizer_verbalizer_object, token_list, 0);
    reset_mocks();
    pv_test_true(
            status == expected_status,
            "verbalize error, expected '%s' got '%s'",
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
                false,
                "error message mismatch, expected '%s'",
                expected_message);
    }

    pv_normalizer_token_list_delete(token_list);
}

static void test_pv_normalizer_verbalizer_init_calloc_failure(void) {
    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);

    test_pv_normalizer_verbalizer_init_failure_helper(PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `o`\\.");
}

static void test_pv_normalizer_verbalizer_init_internal_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_verbalizer_en_init, PV_STATUS_INVALID_ARGUMENT)

    test_pv_normalizer_verbalizer_init_failure_helper(PV_STATUS_INVALID_ARGUMENT, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_en_init` failed with status `INVALID_ARGUMENT`\\.");
}

static void test_pv_normalizer_verbalizer_verbalize_word_fail_calloc(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "hello";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_EN_WORD);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_verbalize_word_common` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_normalizer_verbalizer_verbalize_punctuation_fail_calloc(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = ",";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_EN_PUNCTUATION);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `token_verbalized`\\.");
}

static void test_pv_normalizer_verbalizer_verbalize_cardinal_fail_calloc(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "5";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_EN_CARDINAL);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_cardinal_to_string` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_normalizer_verbalizer_verbalize_negative_cardinal_fail_calloc(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "-5";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_EN_NEGATIVE_CARDINAL);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_cardinal_to_string` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_normalizer_verbalizer_verbalize_negative_cardinal_fail_calloc_2(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "-5";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_EN_NEGATIVE_CARDINAL);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_real,
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `token_verbalized`\\.");
}

static void test_pv_normalizer_verbalizer_verbalize_number_range_fail_calloc_1(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "1-5";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_EN_NUMBER_RANGE_TO);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `token_verbalized`\\.");
}

static void test_pv_normalizer_verbalizer_verbalize_ordinal_fail_calloc_1(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "1st";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_EN_ORDINAL);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_cardinal_to_string` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_normalizer_verbalizer_verbalize_ordinal_fail_calloc_2(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "1st";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_EN_ORDINAL);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_real,
            calloc_real,
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `last_cardinal`\\.");
}

static void test_pv_normalizer_verbalizer_verbalize_negative_ordinal_fail_calloc_1(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "-1st";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_EN_NEGATIVE_ORDINAL);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_cardinal_to_string` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_normalizer_verbalizer_verbalize_negative_ordinal_fail_calloc_2(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "-1st";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_EN_NEGATIVE_ORDINAL);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_real,
            calloc_real,
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `last_cardinal`\\.");
}

static void test_pv_normalizer_verbalizer_verbalize_negative_ordinal_fail_calloc_3(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "-1st";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_EN_NEGATIVE_ORDINAL);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_real,
            calloc_real,
            calloc_real,
            calloc_real,
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `token_verbalized`\\.");
}

static void test_pv_normalizer_verbalizer_verbalize_currency_fail_calloc_1(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "$1";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_EN_CURRENCY);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `number_string`\\.");
}

static void test_pv_normalizer_verbalizer_verbalize_currency_fail_calloc_2(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "$1";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_EN_CURRENCY);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_real,
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_cardinal_to_string` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_normalizer_verbalizer_verbalize_negative_currency_fail_calloc_1(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "-$1";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_EN_NEGATIVE_CURRENCY);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `number_string`\\.");
}

static void test_pv_normalizer_verbalizer_verbalize_negative_currency_fail_calloc_2(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "-$1";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_EN_NEGATIVE_CURRENCY);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_real,
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_cardinal_to_string` failed with status `OUT_OF_MEMORY`\\.");
}

#endif

static const pv_test_case_t PV_NORMALIZER_VERBALIZER_TEST_CASES[] = {
        {"verbalize_word", test_pv_normalizer_verbalizer_verbalize_word},
        {"verbalize_punctuation", test_pv_normalizer_verbalizer_verbalize_punctuation},
        {"verbalize_cardinal", test_pv_normalizer_verbalizer_verbalize_cardinal},
        {"verbalize_negative_cardinal", test_pv_normalizer_verbalizer_verbalize_negative_cardinal},
        {"verbalize_custom_pronunciation", test_pv_normalizer_verbalizer_verbalize_custom_pronunciation},
        {"number_to_string", test_pv_normalizer_verbalizer_number_to_string},
        {"verbalize_invalid_use_case", test_pv_normalizer_verbalizer_verbalize_invalid_use_case},
        {"verbalize_all", test_pv_normalizer_verbalizer_verbalize_all},
        {"verbalize_number_range", test_pv_normalizer_verbalizer_verbalize_number_range},
        {"verbalize_ordinal", test_pv_normalizer_verbalizer_verbalize_ordinal},
        {"verbalize_invalid_ordinal", test_pv_normalizer_verbalizer_verbalize_invalid_ordinal},
        {"verbalize_negative_ordinal", test_pv_normalizer_verbalizer_verbalize_negative_ordinal},
        {"verbalize_special_characters", test_pv_normalizer_verbalizer_verbalize_special_characters},
        {"verbalize_decimal", test_pv_normalizer_verbalizer_verbalize_decimal},
        {"verbalize_measurement", test_pv_normalizer_verbalizer_verbalize_measurement},
        {"verbalize_per_measurement", test_pv_normalizer_verbalizer_verbalize_per_measurement},
        {"verbalize_alphanum_spell_out", test_pv_normalizer_verbalizer_verbalize_alphanum_spell_out},
        {"verbalize_time", test_pv_normalizer_verbalizer_verbalize_time},
        {"verbalize_comma_cardinal", test_pv_normalizer_verbalizer_verbalize_comma_cardinal},
        {"verbalize_comma_ordinal", test_pv_normalizer_verbalizer_verbalize_comma_ordinal},
        {"verbalize_comma_decimal", test_pv_normalizer_verbalizer_verbalize_comma_decimal},
        {"verbalize_comma_number_range", test_pv_normalizer_verbalizer_verbalize_comma_number_range},
        {"verbalize_fraction", test_pv_normalizer_verbalizer_verbalize_fraction},
        {"verbalize_url", test_pv_normalizer_verbalizer_verbalize_url},
        {"verbalize_currency", test_pv_normalizer_verbalizer_verbalize_currency},
        {"verbalize_abbreviation", test_pv_normalizer_verbalizer_verbalize_abbreviation},
        {"verbalize_digits_sequence", test_pv_normalizer_verbalizer_verbalize_digits_sequence},
        {"verbalize_date", test_pv_normalizer_verbalizer_verbalize_date},
        {"verbalize_name", test_pv_normalizer_verbalizer_verbalize_name},
        {"verbalize_comma_cardinal_punctuation", test_pv_normalizer_verbalizer_verbalize_comma_cardinal_punctuation},
        {"verbalize_additional_date", test_pv_normalizer_verbalizer_verbalize_tag_additional_date},
        {"verbalize_domain_extensions_only", test_pv_normalizer_verbalizer_verbalize_domain_extensions_only},
        {"verbalize_temperature_punctuation", test_pv_normalizer_verbalizer_verbalize_temperature_punctuation},
        {"verbalize_cardinal_in_parentheses", test_pv_normalizer_verbalizer_verbalize_cardinal_in_parentheses},
        {"verbalize_single_quote", test_pv_normalizer_verbalizer_verbalize_single_quote},


#ifdef __PV_MOCKS__

        {"verbalizer_init_calloc_failure", test_pv_normalizer_verbalizer_init_calloc_failure},
        {"verbalizer_init_internal_failure", test_pv_normalizer_verbalizer_init_internal_failure},
        {"verbalize_word_fail_calloc", test_pv_normalizer_verbalizer_verbalize_word_fail_calloc},
        {"verbalize_punctuation_fail_calloc", test_pv_normalizer_verbalizer_verbalize_punctuation_fail_calloc},
        {"verbalize_cardinal_fail_calloc", test_pv_normalizer_verbalizer_verbalize_cardinal_fail_calloc},
        {"verbalize_negative_cardinal_fail_calloc",
         test_pv_normalizer_verbalizer_verbalize_negative_cardinal_fail_calloc},
        {"verbalize_negative_cardinal_fail_calloc_2",
         test_pv_normalizer_verbalizer_verbalize_negative_cardinal_fail_calloc_2},
        {"verbalize_number_range_fail_calloc_1",
         test_pv_normalizer_verbalizer_verbalize_number_range_fail_calloc_1},
        {"verbalize_ordinal_fail_calloc_1", test_pv_normalizer_verbalizer_verbalize_ordinal_fail_calloc_1},
        {"verbalize_ordinal_fail_calloc_2", test_pv_normalizer_verbalizer_verbalize_ordinal_fail_calloc_2},
        {"verbalize_negative_ordinal_fail_calloc_1",
         test_pv_normalizer_verbalizer_verbalize_negative_ordinal_fail_calloc_1},
        {"verbalize_negative_ordinal_fail_calloc_2",
         test_pv_normalizer_verbalizer_verbalize_negative_ordinal_fail_calloc_2},
        {"verbalize_negative_ordinal_fail_calloc_3",
         test_pv_normalizer_verbalizer_verbalize_negative_ordinal_fail_calloc_3},
        {"verbalize_currency_fail_calloc_1", test_pv_normalizer_verbalizer_verbalize_currency_fail_calloc_1},
        {"verbalize_currency_fail_calloc_2", test_pv_normalizer_verbalizer_verbalize_currency_fail_calloc_2},
        {"verbalize_negative_currency_fail_calloc_1",
         test_pv_normalizer_verbalizer_verbalize_negative_currency_fail_calloc_1},
        {"verbalize_negative_currency_fail_calloc_2",
         test_pv_normalizer_verbalizer_verbalize_negative_currency_fail_calloc_2},

#endif

};

const pv_test_suite_t PV_NORMALIZER_VERBALIZER_EN_TEST_SUITE = {
        .name = "normalizer_verbalizer_en",
        .setup = test_pv_normalizer_verbalizer_setup,
        .teardown = test_pv_normalizer_verbalizer_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_NORMALIZER_VERBALIZER_TEST_CASES),
        .test_cases = PV_NORMALIZER_VERBALIZER_TEST_CASES,
};
