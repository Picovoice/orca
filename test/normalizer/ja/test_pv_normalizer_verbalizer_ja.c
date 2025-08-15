#include <stdlib.h>
#include <string.h>

#include "core/pv_language.h"
#include "core/pv_language_json.h"
#include "mock/pv_mock.h"
#include "orca/normalizer/ja/pv_normalizer_tags_ja.h"
#include "orca/normalizer/ja/pv_normalizer_verbalizer_ja.h"
#include "orca/normalizer/pv_normalizer_tagger.h"
#include "orca/normalizer/pv_normalizer_tokenizer.h"
#include "orca/normalizer/pv_normalizer_verbalizer.h"
#include "test/pv_test.h"
#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "core/mock/pv_core_runtime_mock.h"
#include "orca/mock/pv_normalizer_mock.h"

#endif

static pv_normalizer_verbalizer_t *normalizer_verbalizer_object = NULL;
static pv_normalizer_use_cases_ja_t ALL_TEST_USE_CASES[] = {
        PV_NORMALIZER_USE_WORD_NORMALIZER_JA,
        PV_NORMALIZER_USE_PUNCTUATION_NORMALIZER_JA,
        PV_NORMALIZER_USE_CARDINAL_NORMALIZER_JA,
        PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_JA,
        PV_NORMALIZER_USE_CUSTOM_PRONUNCIATION_NORMALIZER_JA,
        PV_NORMALIZER_USE_NUMBER_RANGE_NORMALIZER_JA,
        PV_NORMALIZER_USE_SPECIAL_CHARACTER_NORMALIZER_JA,
        PV_NORMALIZER_USE_DECIMAL_NORMALIZER_JA,
        PV_NORMALIZER_USE_ALPHANUM_SPELL_OUT_NORMALIZER_JA,
        PV_NORMALIZER_USE_MEASUREMENT_NORMALIZER_JA,
        PV_NORMALIZER_USE_TIME_NORMALIZER_JA,
        PV_NORMALIZER_USE_FRACTION_NORMALIZER_JA,
        PV_NORMALIZER_USE_URL_NORMALIZER_JA,
        PV_NORMALIZER_USE_CURRENCY_NORMALIZER_JA,
        PV_NORMALIZER_USE_ABBREVIATION_NORMALIZER_JA,
        PV_NORMALIZER_USE_DIGITS_SEQUENCE_NORMALIZER_JA,
        PV_NORMALIZER_USE_DATE_NORMALIZER_JA,
        PV_NORMALIZER_USE_NAME_NORMALIZER_JA,
};

static pv_normalizer_tagger_t *normalizer_tagger_object = NULL;
static pv_normalizer_tokenizer_t *normalizer_tokenizer_object = NULL;
static pv_language_info_t *language_info_object = NULL;

static const char LANGUAGE_INFO_PATH[] = "normalizer/ref/pv_language_info_normalizer_ja.json";
static const char TOKENIZER_DATA_PATH[] = "normalizer/tokenizer_data/ipadic.bin";

static const char VERBALIZER_TEST_SENTENCE[] = "私はビタミンB-12、C、その他15種類を摂取しています。";
static const char *VERBALIZER_TEST_SENTENCE_VERBALIZED[] = {
        "私",
        "は",
        "ビタミン",
        "ビー",
        "ジューニ",
        "、",
        "シー",
        "、",
        "その他",
        "ジューゴ",
        "種類",
        "を",
        "摂取",
        "し",
        "て",
        "い",
        "ます",
        "。",
};

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

    char *tokenizer_data_path = pv_test_module_res_path(TOKENIZER_DATA_PATH);
    pv_test_true(tokenizer_data_path != NULL, "Failed to get tokenizer_data_path");
    if (!tokenizer_data_path) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    int32_t num_content_bytes = 0;
    void *tokenizer_data = NULL;
    status = pv_file_load(tokenizer_data_path, &num_content_bytes, &tokenizer_data);
    free(tokenizer_data_path);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    const void *shadow = tokenizer_data;
    status = pv_normalizer_tokenizer_init(
            language_info_object,
            &shadow,
            &normalizer_tokenizer_object);
    free(tokenizer_data);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_normalizer_tagger_init(
            language_info_object,
            PV_ARRAY_LEN(ALL_TEST_USE_CASES),
            (const int32_t *) ALL_TEST_USE_CASES,
            normalizer_tokenizer_object,
            NULL,
            &normalizer_tagger_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_normalizer_verbalizer_init(
            PV_NORMALIZER_LANGUAGE_JA,
            PV_ARRAY_LEN(ALL_TEST_USE_CASES),
            (const int32_t *) ALL_TEST_USE_CASES,
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
        pv_normalizer_use_cases_ja_t *use_cases,
        pv_normalizer_tagger_t **tagger,
        pv_normalizer_verbalizer_t **verbalizer) {
    *tagger = NULL;
    *verbalizer = NULL;

    pv_status_t status = pv_normalizer_tagger_init(
            language_info_object,
            num_use_cases,
            (const int32_t *) use_cases,
            normalizer_tokenizer_object,
            NULL,
            tagger);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to initialize tagger");

    status = pv_normalizer_verbalizer_init(
            PV_NORMALIZER_LANGUAGE_JA,
            num_use_cases,
            (const int32_t *) use_cases,
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
            false,
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
    pv_normalizer_use_cases_ja_t use_cases[] = {PV_NORMALIZER_USE_WORD_NORMALIZER_JA};

    static const char sentence[] = "私はビタミンB-12、C、その他15種類を摂取しています。";

    const char *sentence_verbalized[] = {
            "私",
            "は",
            "ビタミン",
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            "その他",
            NULL,
            "種類",
            "を",
            "摂取",
            "し",
            "て",
            "い",
            "ます",
            NULL,
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
    pv_normalizer_use_cases_ja_t use_cases[] = {PV_NORMALIZER_USE_PUNCTUATION_NORMALIZER_JA};

    const char *sentence_verbalized[] = {
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            "、",
            NULL,
            "、",
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            "。"};

    pv_normalizer_tagger_t *tagger = NULL;
    pv_normalizer_verbalizer_t *verbalizer = NULL;
    test_pv_normalizer_verbalizer_init_helper(PV_ARRAY_LEN(use_cases), use_cases, &tagger, &verbalizer);

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

static void test_pv_normalizer_verbalizer_verbalize_cardinal(void) {
    pv_normalizer_use_cases_ja_t use_cases[] = {PV_NORMALIZER_USE_CARDINAL_NORMALIZER_JA};

    const char *sentence_verbalized[] = {
            NULL,
            NULL,
            NULL,
            NULL,
            "ジューニ",
            NULL,
            NULL,
            NULL,
            NULL,
            "ジューゴ",
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
    };

    pv_normalizer_tagger_t *tagger = NULL;
    pv_normalizer_verbalizer_t *verbalizer = NULL;
    test_pv_normalizer_verbalizer_init_helper(PV_ARRAY_LEN(use_cases), use_cases, &tagger, &verbalizer);

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

static void test_pv_normalizer_verbalizer_verbalize_negative_cardinal(void) {
    pv_normalizer_use_cases_ja_t use_cases[] = {PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_JA};

    const char sentence[] = "123 -123";
    const char *sentence_verbalized[] = {NULL, "マイナスヒャクニジューサン"};

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

static void test_pv_normalizer_verbalizer_verbalize_custom_pronunciation(void) {
    pv_normalizer_use_cases_ja_t use_cases[] = {PV_NORMALIZER_USE_CUSTOM_PRONUNCIATION_NORMALIZER_JA};

    const char sentence[] = "これは{カスタム|k a s t a m ɕ}発音です";
    const char *sentence_verbalized[] = {NULL, NULL, "{k a s t a m ɕ}", NULL, NULL};

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


static void test_pv_normalizer_verbalizer_number_to_string(void) {
    const char *sentence = "36 130 0 003 123456789 5242 999999999999999 1200000000000009 11111111111111111";
    const char *sentence_verbalized[] = {
            "サンジューロク",
            "ヒャクサンジュー",
            "ゼロ",
            "ゼロゼロサン",
            "イチオクニセンサンヒャクヨンジューゴマンロクセンナナヒャクハチジューキュー",
            "ゴセンニヒャクヨンジューニ",
            "キューヒャクキュージューキューチョーキューセンキューヒャクキュージューキューオクキューセンキューヒャクキュージューキューマンキューセンキューヒャクキュージューキュー",
            "イチニゼロゼロゼロゼロゼロゼロゼロゼロゼロゼロゼロゼロゼロキュー",
            "イチイチイチイチイチイチイチイチイチイチイチイチイチイチイチイチイチ"};

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            false,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_number_range(void) {
    pv_normalizer_use_cases_ja_t use_cases[] = {
            PV_NORMALIZER_USE_NUMBER_RANGE_NORMALIZER_JA,
            PV_NORMALIZER_USE_CARDINAL_NORMALIZER_JA,
            PV_NORMALIZER_USE_DECIMAL_NORMALIZER_JA};

    const char sentence[] = "12-5 1-44 1.1-1.3";
    const char *sentence_verbalized[] = {
            "ジューニ",
            "カラ",
            "ゴ",
            "イチ",
            "カラ",
            "ヨンジューヨン",
            "イチ",
            "テン",
            "イチ",
            "カラ",
            "イチ",
            "テン",
            "サン",
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
    pv_normalizer_use_cases_ja_t use_cases[] = {99};

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

static void test_pv_normalizer_verbalizer_verbalize_special_characters(void) {
    pv_normalizer_use_cases_ja_t use_cases[] = {PV_NORMALIZER_USE_SPECIAL_CHARACTER_NORMALIZER_JA};

    const char *sentence = "& % @ \n _ ( ) 『 』 °C RPM";
    const char *sentence_verbalized[] = {
            "アンド",
            "パーセント",
            "アット",
            "。",
            "アンダーバー",
            "、",
            "、",
            "、",
            "、",
            "ド",
            "アールピーエム"};

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
            "イチ",
            "テン",
            "ニサン",
            "テン",
            "ゴ",
            "ジュー",
            "テン",
            "イチ",
            "パーセント",
            "マイナスニ",
            "テン",
            "ニ"};

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
    const char sentence[] = "5g 1 ml 7.3L 3 ft 10.1km 5°C 1,000,000g 25ft.-lb.";
    const char *sentence_verbalized[] = {
            "ゴ",
            "グラム",
            "イチ",
            "ミリリットル",
            "ナナ",
            "テン",
            "サン",
            "リットル",
            "サン",
            "フィート",
            "ジュー",
            "テン",
            "イチ",
            "キロメートル",
            "ゴ",
            "ド",
            "ヒャクマン",
            "グラム",
            "ニジューゴ",
            "フィート",
            NULL,
            "ポンド",
            "。",
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
    const char sentence[] = "5 km/m 10 oz/km 1.1m/l -5.41kg/ft tb/hz 1,005m/s";
    const char *sentence_verbalized[] = {
            "ゴ",
            "キロメートル",
            "マイ",
            "メートル",
            "ジュー",
            "オンス",
            "マイ",
            "キロメートル",
            "イチ",
            "テン",
            "イチ",
            "メートル",
            "マイ",
            "リットル",
            "マイナスゴ",
            "テン",
            "ヨンイチ",
            "キログラム",
            "マイ",
            "フィート",
            "テラバイト",
            "マイ",
            "ヘルツ",
            "センゴ",
            "メートル",
            "マイ",
            "ビョウ",
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
            "エイチ", "ティー", "エム", "エル", "ゴ",
            NULL,
            "シー", "ゴ", "ビー",
            NULL,
            "イチ", "ニ", "エー",
            NULL,
            "ゼット", "ナナ", "ハチ",
            NULL,
            "ヨンヒャク", "ビー", "シー"};

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
    const char *sentence = "午後7:07 8:00 89:00 午前2:01 11:15 a.m. 03:00 03:14 9";
    const char *sentence_verbalized[] = {
            "午後", NULL, "ナナジ", NULL, "ナナプン", NULL,
            "ハチジ", NULL, NULL, NULL,
            "ハチジューキュー", "コロン", "ゼロゼロ", NULL,
            "午前", NULL, "ニジ", NULL, "イチプン", NULL,
            "ゴゼンジューイチジ", NULL, "ジューゴプン", NULL, NULL, NULL,
            "サンジ", NULL, NULL, NULL,
            "サンジ", NULL, "ジューヨンプン", NULL,
            "キュー"};

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
            "センゴ",
            "イチマンセンゴ",
            "ジューイチマンセンゴ",
            "マイナスゴジューオク",
            "センヒャクジューイチ",
            "、",
            "ゼロゼロゼロ",
            "イチ",
            "、",
            "イチ",
            "、",
            "ゼロゼロゼロ",
            "イチ",
            "、",
            "ゼロゼロゼロ",
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
            "セン",
            "テン",
            "ニ",
            "マイナスサンジューマン",
            "テン",
            "ロクロクロクロク",
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
            "セン",
            "カラ",
            "ニセン",
            "セン",
            "テン",
            "ニ",
            "カラ",
            "セン",
            "テン",
            "ゴ",
            "イチマン",
            "カラ",
            "イチ",
            "テン",
            "サン",
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
            "イチ",
            "ワル",
            "ニ",
            "キュー",
            "ワル",
            "サン",
            "テン",
            "ニ",
            "イチ",
            "ワル",
            "セン",
            "イチ",
            "ワル",
            "マイナスヨン",
            "マイナスヨン",
            "ワル",
            "サン"};

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
            "ダブリュー",
            "ダブリュー",
            "ダブリュー",
            "ドット",
            "イー",
            "エックス",
            "エー",
            "エム",
            "ピー",
            "エル",
            "イー",
            "ドット",
            "コム",
            "エイチ",
            "ティー",
            "ティー",
            "ピー",
            "エス",
            "コロン",
            "スラッシュ",
            "スラッシュ",
            "エイチ",
            "イー",
            "エル",
            "エル",
            "オー",
            "ドット",
            "シー",
            "エー",
            "スラッシュ",
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
            "ゴドル",
            NULL,
            "ジューユーロ",
            NULL,
            "ジュードル",
            NULL,
            "ジューユーロ",
            NULL,
            "イチドルニジューゴセント",
            NULL,
            "イチドルニジューゴセント",
            NULL,
            "マイナスゴヒャクドル",
            NULL,
            "マイナスゴヒャクドル",
            NULL,
            "マイナスイチドルニジューゴセント",
            NULL,
            "マイナスイチドルニジューゴセント",
            NULL,
            "センユーロ",
            NULL,
            "マイナスセンユーロ",
            NULL,
            "センユーロ",
            NULL,
            "マイナスセンユーロ",
            NULL,
            "センユーロニジューゴセント",
            NULL,
            "センユーロニジューゴセント",
            NULL,
            "ニエン",
            NULL,
            "ニシェケル",
            NULL,
            "ニポンド",
            NULL,
            "ニウォン",
            NULL,
            "ニリラ",
            NULL,
            "ニペソ",
            NULL,
            "ニルーブル",
            NULL,
            "ニバーツ",
            NULL,
            "ニフリヴニャ",
            NULL,
            "ニルピー",
            NULL,
            "ニセント",
            NULL,
            "イチドル",
            NULL,
            "イチドルイチセント",
            NULL,
            "ゴ",
            NULL,
            "ユーロ",
            NULL,
            "イチ",
            "テン",
            "ニゴ",
            NULL,
            "ポンド",
            NULL,
            "マイナスサン",
            NULL,
            "ユーロ",
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
    const char *sentence = "e.g. は Prof. Dr. Ave. St. vs. etc.";
    const char *sentence_verbalized[] = {
            "タトエバ",
            NULL,
            "は",
            NULL,
            "キョージュ",
            NULL,
            "ハカセ",
            NULL,
            "オオドオリー",
            NULL,
            "トオリー",
            NULL,
            "タイ",
            NULL,
            "ソノタ",
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
    const char *sentence = "(772) 778-1923 1-800-123-4567 123-99456-7890 893.456.7890 +1 (381) 102-129";
    const char *sentence_verbalized[] = {
            "、 ナナナナニ、",
            NULL,
            "ナナナナハチ",
            "、",
            "イチキューニサン",
            NULL,
            "イチ",
            "、",
            "ハチゼロゼロ",
            "、",
            "イチニサン",
            "、",
            "ヨンゴロクナナ",
            NULL,
            "イチニサン",
            "、",
            "キューキューヨンゴロク",
            "、",
            "ナナハチキューゼロ",
            NULL,
            "ハチキューサン",
            "、",
            "ヨンゴロク",
            "、",
            "ナナハチキューゼロ",
            NULL,
            "プラス",
            "イチ",
            NULL,
            "、 サンハチイチ、",
            NULL,
            "イチゼロニ",
            "、",
            "イチニキュー",
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
    const char *sentence = "06-03-2020 31-02-1991 06/03/2020 6/3/2020 1991/03/13 "
                           "2020年6月3日 6月3日2024年 20日2024年10月 8月1日 2024年";
    const char *sentence_verbalized[] = {
            "ロクガツ", NULL, "ミッカ", NULL, "ニセンニジューネン",
            NULL,
            "サンジューイチニチ", NULL, "ニガツ", NULL, "センキューヒャクキュージューイチネン",
            NULL,
            "ロクガツ", NULL, "ミッカ", NULL, "ニセンニジューネン",
            NULL,
            "ロクガツ", NULL, "ミッカ", NULL, "ニセンニジュー",
            NULL,
            "センキューヒャクキュージューイチネン", NULL, "サンガツ", NULL, "ジューサンニチ",
            NULL,
            "ニセンニジュー", NULL, "ネン", NULL, "ロク", NULL, "ガツ", NULL, "ミッ", NULL, "カ",
            NULL,
            "ロク", NULL, "ガツ", NULL, "ミッ", NULL, "カ", NULL, "ニセンニジューヨン", NULL, "ネン",
            NULL,
            "ハツ", NULL, "カ", NULL, "ニセンニジューヨン", NULL, "ネン", NULL, "ジュー", NULL, "ガツ",
            NULL,
            "ハチ", NULL, "ガツ", NULL, "ツイ", NULL, "タチ",
            NULL,
            "ニセンニジューヨン", NULL, "ネン"};

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
            "ジェイ",
            NULL,
            NULL,
            "ケー",
            NULL,
            NULL,
            "アール",
            "オー",
            "ダブリュー",
            "エル",
            "アイ",
            "エヌ",
            "ジー",
            "、",
            NULL,
            "ジェイ",
            "オー",
            "エイチ",
            "エヌ",
            NULL,
            "エフ",
            NULL,
            NULL,
            "ケー",
            "イー",
            "エヌ",
            "エヌ",
            "イー",
            "ディー",
            "ワイ",
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

#ifdef __PV_MOCKS__

static void test_pv_normalizer_verbalizer_init_failure_helper(
        pv_status_t expected_status,
        const char *expected_public_message_regex,
        const char *expected_private_message_regex) {
    pv_normalizer_verbalizer_t *normalizer_verbalizer = NULL;
    pv_status_t status = pv_normalizer_verbalizer_init(
            PV_NORMALIZER_LANGUAGE_JA,
            PV_ARRAY_LEN(ALL_TEST_USE_CASES),
            (int32_t *) ALL_TEST_USE_CASES,
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
        pv_normalizer_token_tag_ja_t tag) {
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
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_verbalizer_ja_init, PV_STATUS_INVALID_ARGUMENT)

    test_pv_normalizer_verbalizer_init_failure_helper(PV_STATUS_INVALID_ARGUMENT, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_ja_init` failed with status `INVALID_ARGUMENT`\\.");
}

static void test_pv_normalizer_verbalizer_verbalize_punctuation_fail_calloc(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "、";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_JA_PUNCTUATION);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_verbalize_punctuation` failed with status `OUT_OF_MEMORY`.");
}

static void test_pv_normalizer_verbalizer_verbalize_cardinal_fail_calloc(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "5";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_JA_CARDINAL);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_verbalize_cardinal` failed with status `OUT_OF_MEMORY`.");
}

static void test_pv_normalizer_verbalizer_verbalize_negative_cardinal_fail_calloc(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "-5";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_JA_NEGATIVE_CARDINAL);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_verbalize_negative_cardinal` failed with status `OUT_OF_MEMORY`.");
}

static void test_pv_normalizer_verbalizer_verbalize_negative_cardinal_fail_calloc_2(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "-5";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_JA_NEGATIVE_CARDINAL);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_real,
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_verbalize_negative_cardinal` failed with status `OUT_OF_MEMORY`.");
}

static void test_pv_normalizer_verbalizer_verbalize_number_range_fail_calloc_1(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "1-5";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_JA_NUMBER_RANGE_TO);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_verbalize_number_range` failed with status `OUT_OF_MEMORY`.");
}

static void test_pv_normalizer_verbalizer_verbalize_currency_fail_calloc_1(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "$1";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_JA_CURRENCY);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `number_string`.");
}

static void test_pv_normalizer_verbalizer_verbalize_currency_fail_calloc_2(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "$1";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_JA_CURRENCY);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_real,
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `result`.");
}

static void test_pv_normalizer_verbalizer_verbalize_negative_currency_fail_calloc_1(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "-$1";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_JA_NEGATIVE_CURRENCY);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `number_string`.");
}

static void test_pv_normalizer_verbalizer_verbalize_negative_currency_fail_calloc_2(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "-$1";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_JA_NEGATIVE_CURRENCY);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_real,
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `result`.");
}

#endif

static const pv_test_case_t PV_NORMALIZER_VERBALIZER_JA_TEST_CASES[] = {
        {"verbalize_all", test_pv_normalizer_verbalizer_verbalize_all},
        {"verbalize_word", test_pv_normalizer_verbalizer_verbalize_word},
        {"verbalize_punctuation", test_pv_normalizer_verbalizer_verbalize_punctuation},
        {"verbalize_cardinal", test_pv_normalizer_verbalizer_verbalize_cardinal},
        {"verbalize_negative_cardinal", test_pv_normalizer_verbalizer_verbalize_negative_cardinal},
        {"verbalize_custom_pronunciation", test_pv_normalizer_verbalizer_verbalize_custom_pronunciation},
        {"number_to_string", test_pv_normalizer_verbalizer_number_to_string},
        {"verbalize_number_range", test_pv_normalizer_verbalizer_verbalize_number_range},
        {"verbalize_invalid_use_case", test_pv_normalizer_verbalizer_verbalize_invalid_use_case},
        {"verbalize_special_characters", test_pv_normalizer_verbalizer_verbalize_special_characters},
        {"verbalize_decimal", test_pv_normalizer_verbalizer_verbalize_decimal},
        {"verbalize_measurement", test_pv_normalizer_verbalizer_verbalize_measurement},
        {"verbalize_per_measurement", test_pv_normalizer_verbalizer_verbalize_per_measurement},
        {"verbalize_alphanum_spell_out", test_pv_normalizer_verbalizer_verbalize_alphanum_spell_out},
        {"verbalize_time", test_pv_normalizer_verbalizer_verbalize_time},
        {"verbalize_comma_cardinal", test_pv_normalizer_verbalizer_verbalize_comma_cardinal},
        {"verbalize_comma_decimal", test_pv_normalizer_verbalizer_verbalize_comma_decimal},
        {"verbalize_comma_number_range", test_pv_normalizer_verbalizer_verbalize_comma_number_range},
        {"verbalize_fraction", test_pv_normalizer_verbalizer_verbalize_fraction},
        {"verbalize_url", test_pv_normalizer_verbalizer_verbalize_url},
        {"verbalize_currency", test_pv_normalizer_verbalizer_verbalize_currency},
        {"verbalize_abbreviation", test_pv_normalizer_verbalizer_verbalize_abbreviation},
        {"verbalize_digits_sequence", test_pv_normalizer_verbalizer_verbalize_digits_sequence},
        {"verbalize_date", test_pv_normalizer_verbalizer_verbalize_date},
        {"verbalize_name", test_pv_normalizer_verbalizer_verbalize_name},

#ifdef __PV_MOCKS__

        {"verbalizer_init_calloc_failure", test_pv_normalizer_verbalizer_init_calloc_failure},
        {"verbalizer_init_internal_failure", test_pv_normalizer_verbalizer_init_internal_failure},
        {"verbalize_punctuation_fail_calloc", test_pv_normalizer_verbalizer_verbalize_punctuation_fail_calloc},
        {"verbalize_cardinal_fail_calloc", test_pv_normalizer_verbalizer_verbalize_cardinal_fail_calloc},
        {"verbalize_negative_cardinal_fail_calloc",
         test_pv_normalizer_verbalizer_verbalize_negative_cardinal_fail_calloc},
        {"verbalize_negative_cardinal_fail_calloc_2",
         test_pv_normalizer_verbalizer_verbalize_negative_cardinal_fail_calloc_2},
        {"verbalize_number_range_fail_calloc_1",
         test_pv_normalizer_verbalizer_verbalize_number_range_fail_calloc_1},
        {"verbalize_currency_fail_calloc_1", test_pv_normalizer_verbalizer_verbalize_currency_fail_calloc_1},
        {"verbalize_currency_fail_calloc_2", test_pv_normalizer_verbalizer_verbalize_currency_fail_calloc_2},
        {"verbalize_negative_currency_fail_calloc_1",
         test_pv_normalizer_verbalizer_verbalize_negative_currency_fail_calloc_1},
        {"verbalize_negative_currency_fail_calloc_2",
         test_pv_normalizer_verbalizer_verbalize_negative_currency_fail_calloc_2},

#endif

};

const pv_test_suite_t PV_NORMALIZER_VERBALIZER_JA_TEST_SUITE = {
        .name = "normalizer_verbalizer_ja",
        .setup = test_pv_normalizer_verbalizer_setup,
        .teardown = test_pv_normalizer_verbalizer_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_NORMALIZER_VERBALIZER_JA_TEST_CASES),
        .test_cases = PV_NORMALIZER_VERBALIZER_JA_TEST_CASES,
};
