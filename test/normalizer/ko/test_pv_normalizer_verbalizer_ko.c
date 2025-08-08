#include <stdlib.h>
#include <string.h>

#include "core/pv_language.h"
#include "core/pv_language_json.h"
#include "mock/pv_mock.h"
#include "orca/normalizer/ko/pv_normalizer_tags_ko.h"
#include "orca/normalizer/ko/pv_normalizer_verbalizer_ko.h"
#include "orca/normalizer/pv_normalizer_tagger.h"
#include "orca/normalizer/pv_normalizer_tokenizer.h"
#include "orca/normalizer/pv_normalizer_verbalizer.h"
#include "test/pv_test.h"

#ifdef __PV_MOCKS__

#include "core/mock/pv_core_runtime_mock.h"
#include "orca/mock/pv_normalizer_mock.h"

#endif

static pv_normalizer_verbalizer_t *normalizer_verbalizer_object = NULL;
static pv_normalizer_language_t LANGUAGE = PV_NORMALIZER_LANGUAGE_KO;
static int32_t ALL_TEST_USE_CASES[] = {
        PV_NORMALIZER_USE_WORD_NORMALIZER_KO,
        PV_NORMALIZER_USE_PUNCTUATION_NORMALIZER_KO,
        PV_NORMALIZER_USE_CARDINAL_NORMALIZER_KO,
        PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_KO,
        PV_NORMALIZER_USE_CUSTOM_PRONUNCIATION_NORMALIZER_KO,
        PV_NORMALIZER_USE_NUMBER_RANGE_NORMALIZER_KO,
        PV_NORMALIZER_USE_SPECIAL_CHARACTER_NORMALIZER_KO,
        PV_NORMALIZER_USE_DECIMAL_NORMALIZER_KO,
        PV_NORMALIZER_USE_ALPHANUM_SPELL_OUT_NORMALIZER_KO,
        PV_NORMALIZER_USE_MEASUREMENT_NORMALIZER_KO,
        PV_NORMALIZER_USE_TIME_NORMALIZER_KO,
        PV_NORMALIZER_USE_FRACTION_NORMALIZER_KO,
        PV_NORMALIZER_USE_URL_NORMALIZER_KO,
        PV_NORMALIZER_USE_CURRENCY_NORMALIZER_KO,
        PV_NORMALIZER_USE_DIGITS_SEQUENCE_NORMALIZER_KO,
        PV_NORMALIZER_USE_DATE_NORMALIZER_KO,
        PV_NORMALIZER_USE_NAME_NORMALIZER_KO,
};

static pv_normalizer_tagger_t *normalizer_tagger_object = NULL;
static pv_normalizer_tokenizer_t *normalizer_tokenizer_object = NULL;
static pv_language_info_t *language_info_object = NULL;

static const char LANGUAGE_INFO_PATH[] = "normalizer/ref/pv_language_info_normalizer_ko.json";

static const char VERBALIZER_TEST_SENTENCE[] = "나는 비타민 B-12, C, 그리고 15개를 더 먹고 있어요!";
static const char *VERBALIZER_TEST_SENTENCE_VERBALIZED[] = {
        "나는",
        "비타민",
        "비",
        "십이",
        ",",
        "시",
        ",",
        "그리고",
        "열다섯",
        "개를",
        "더",
        "먹고",
        "있어요",
        "!"};

static void *calloc_return_null(size_t arg0, size_t arg1) {
    (void) arg0;
    (void) arg1;
    return NULL;
}

static pv_status_t test_pv_normalizer_verbalizer_setup(void) {
    char *language_info_path = pv_test_resource_path(LANGUAGE_INFO_PATH);
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
            LANGUAGE,
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
            LANGUAGE,
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
            pv_normalizer_tokenizer_default_word_boundary_character(tokenizer),
            preserve_word_boundary,
            false,
            false,
            false,
            &token_list);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to tokenize sentence: `%s`", sentence);

    status = pv_normalizer_tagger_tag(tagger, token_list, 0, true);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to tag sentence: `%s`", sentence);

    status = pv_normalizer_verbalizer_verbalize(verbalizer, token_list, 0);

    pv_test_true(
            token_list->size == target_num_tokens,
            "incorrect number of tokens. got `%d`, expected `%d`",
            token_list->size,
            target_num_tokens);

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
            pv_normalizer_tokenizer_default_word_boundary_character(tokenizer),
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
    int32_t use_cases[] = {PV_NORMALIZER_USE_WORD_NORMALIZER_KO};

    static const char sentence[] = "나는 비타민 B-12, C, 그리고 15개를 더 먹고 있어요!";

    const char *sentence_verbalized[] = {
            "나는",
            "비타민",
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            "그리고",
            NULL,
            "개를",
            "더",
            "먹고",
            "있어요",
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
    int32_t use_cases[] = {PV_NORMALIZER_USE_PUNCTUATION_NORMALIZER_KO};

    const char *sentence_verbalized[] = {
            NULL,
            NULL,
            NULL,
            NULL,
            ",",
            NULL,
            ",",
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            "!"};

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

static void test_pv_normalizer_verbalizer_verbalize_english_punctuation(void) {
    const char *sentence = ": \" ' ?";
    const char *sentence_verbalized[] = {
            NULL,
            NULL,
            NULL,
            "?"
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

static void test_pv_normalizer_verbalizer_verbalize_custom_pronunciation(void) {
    int32_t use_cases[] = {PV_NORMALIZER_USE_CUSTOM_PRONUNCIATION_NORMALIZER_KO};

    const char sentence[] = "맞춤형 {발음|p a r ɯ m}";
    const char *sentence_verbalized[] = {NULL, "{p a r ɯ m}"};

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
    int32_t use_cases[] = {PV_NORMALIZER_USE_CARDINAL_NORMALIZER_KO};

    const char *sentence_verbalized[] = {
            NULL,
            NULL,
            NULL,
            "십이",
            NULL,
            NULL,
            NULL,
            NULL,
            "십오",
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

static void test_pv_normalizer_verbalizer_number_to_string(void) {
    const char *sentence = "36 130 0 003 456 123456789 5242 999999999999999 1200000000000009 11111111111111111 1000000000000000000";
    const char *sentence_verbalized[] = {
            "삼십육",
            "백삼십",
            "영",
            "영 영 삼",
            "사백오십육",
            "일억이천삼백사십오만육천칠백팔십구",
            "오천이백사십이",
            "구백구십구조구천구백구십구억구천구백구십구만구천구백구십구",
            "천이백조구",
            "일경천백십일조천백십일억천백십일만천백십일",
            "일 영 영 영 영 영 영 영 영 영 영 영 영 영 영 영 영 영 영"};

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
    int32_t use_cases[] = {PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_KO};

    const char sentence[] = "123 -123";
    const char *sentence_verbalized[] = {NULL, "마이너스 백이십삼"};

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
            PV_NORMALIZER_USE_NUMBER_RANGE_NORMALIZER_KO,
            PV_NORMALIZER_USE_CARDINAL_NORMALIZER_KO,
            PV_NORMALIZER_USE_DECIMAL_NORMALIZER_KO};

    const char sentence[] = "12-5 1-44 1.1-1.3";
    const char *sentence_verbalized[] = {
            "십이",
            "에서",
            "오",
            "일",
            "에서",
            "사십사",
            "일",
            "점",
            "일",
            "에서",
            "일",
            "점",
            "삼",
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

static void test_pv_normalizer_verbalizer_verbalize_special_characters(void) {
    int32_t use_cases[] = {PV_NORMALIZER_USE_SPECIAL_CHARACTER_NORMALIZER_KO};

    const char *sentence = "% @ \n _ ( ) °C RPM";
    const char *sentence_verbalized[] = {
            "퍼센트",
            "골뱅이",
            ".",
            "언더바",
            ",",
            ",",
            "섭씨 도",
            "분당 회전수"};

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
            "일",
            "점",
            "이십삼",
            "점",
            "오",
            "십",
            "점",
            "일",
            "퍼센트",
            "마이너스 이",
            "점",
            "이"};

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
    const char sentence[] = "5g 1 ml 7.3L 3 ft 10.1km 5°C 8.1 RPM 1,000,000g 25ft.-lb.";
    const char *sentence_verbalized[] = {
            "오",
            "그램",
            "일",
            "밀리리터",
            "칠",
            "점",
            "삼",
            "리터",
            "삼",
            "피트",
            "십",
            "점",
            "일",
            "킬로미터",
            "섭씨 오도",
            "분당 팔점일회전수",
            "백만",
            "그램",
            "이십오",
            "피트",
            NULL,
            "파운드",
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
    const char sentence[] = "5 km/m 10 oz/km 1.1m/l -5.41kg/ft tb/hz 1,000m/s";
    const char *sentence_verbalized[] = {
            "미터당 오킬로미터",
            "킬로미터당 십온스",
            "리터당 일점일미터",
            "피트당 마이너스 오점사십일킬로그램",
            "헤르츠당 테라바이트",
            "초당 천미터",
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
    const char *sentence = "HTML5 C5B 12A Z78 헤이B";
    const char *sentence_verbalized[] = {
            "에이치", "티", "엠", "엘", "오",
            NULL,
            "시", "오", "비",
            NULL,
            "일", "이", "에이",
            NULL,
            "제트", "칠", "팔",
            NULL,
            "헤이", "비"};

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
    const char *sentence = "오후 7:00 09:18 a.m. 18:23 21:89 오후 01:30";
    const char *sentence_verbalized[] = {
            "오후",
            NULL,
            "일곱시",
            NULL,
            NULL,
            NULL,
            "오전 아홉시",
            NULL,
            "십팔분",
            NULL,
            NULL,
            NULL,
            "십팔시",
            NULL,
            "이십삼분",
            NULL,
            "이십일",
            "쌍점",
            "팔십구",
            NULL,
            "오후",
            NULL,
            "한시",
            NULL,
            "삼십분",
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
            "천오",
            "만천오",
            "십일만천오",
            "마이너스 오십억",
            "천백십일",
            ",",
            "영 영 영",
            "일",
            ",",
            "일",
            ",",
            "영 영 영",
            "일",
            ",",
            "영 영 영",
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
            "천",
            "점",
            "이",
            "마이너스 삼십만",
            "점",
            "육천육백육십육",
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
            "천",
            "에서",
            "이천",
            "천",
            "점",
            "이",
            "에서",
            "천",
            "점",
            "오",
            "만",
            "에서",
            "일",
            "점",
            "삼",
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
    const char *sentence = "1/2 9/3.2 1/1,000 1.2/3 1/-4 -4/3 -2.3/-4.5";
    const char *sentence_verbalized[] = {
            NULL,
            NULL,
            "이분의 일",
            NULL,
            NULL,
            NULL,
            NULL,
            "삼점이분의 구",
            NULL,
            NULL,
            "천분의 일",
            NULL,
            NULL,
            NULL,
            NULL,
            "삼분의 일점이",
            NULL,
            NULL,
            "마이너스 사분의 일",
            NULL,
            NULL,
            "삼분의 마이너스 사",
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            "마이너스 사점오분의 마이너스 이점삼"};

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
    const char *sentence = "www.ejemplo.com https://hola.ca/";
    const char *sentence_verbalized[] = {
            "더블유",
            "더블유",
            "더블유",
            "점",
            "이",
            "제이",
            "이",
            "엠",
            "피",
            "엘",
            "오",
            "점",
            "컴",
            "에이치",
            "티",
            "티",
            "피",
            "에스",
            "쌍점",
            "슬래시",
            "슬래시",
            "에이치",
            "오",
            "엘",
            "에이",
            "점",
            "시",
            "에이",
            "슬래시",
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
            "오 달러",
            NULL,
            "십 유로",
            NULL,
            "십 달러",
            NULL,
            "십 유로",
            NULL,
            "일 달러 이십오 센트",
            NULL,
            "일 달러 이십오 센트",
            NULL,
            "마이너스 오백 달러",
            NULL,
            "마이너스 오백 달러",
            NULL,
            "마이너스 일 달러 이십오 센트",
            NULL,
            "마이너스 일 달러 이십오 센트",
            NULL,
            "천 유로",
            NULL,
            "마이너스 천 유로",
            NULL,
            "천 유로",
            NULL,
            "마이너스 천 유로",
            NULL,
            "천 유로 이십오 센트",
            NULL,
            "천 유로 이십오 센트",
            NULL,
            "이 위안",
            NULL,
            "이 세켈",
            NULL,
            "이 파운드",
            NULL,
            "이 원",
            NULL,
            "이 리라",
            NULL,
            "이 페소",
            NULL,
            "이 루블",
            NULL,
            "이 바트",
            NULL,
            "이 흐리브냐",
            NULL,
            "이 루피",
            NULL,
            "이 센트",
            NULL,
            "일 달러",
            NULL,
            "일 달러 일 센트",
            NULL,
            "오",
            NULL,
            "유로",
            NULL,
            "일",
            "점",
            "이십오",
            NULL,
            "파운드",
            NULL,
            "마이너스 삼",
            NULL,
            "유로",
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
            ", 칠 칠 이,",
            NULL,
            "칠 칠 팔",
            ",",
            "일 구 이 삼",
            NULL,
            "일",
            ",",
            "팔 영 영",
            ",",
            "일 이 삼",
            ",",
            "사 오 육 칠",
            NULL,
            "일 이 삼",
            ",",
            "구 구 사 오 육",
            ",",
            "칠 팔 구 영",
            NULL,
            "팔 구 삼",
            ",",
            "사 오 육",
            ",",
            "칠 팔 구 영",
            NULL,
            "더하기",
            "일",
            NULL,
            ", 삼 팔 일,",
            NULL,
            "일 영 이",
            ",",
            "일 이 구",
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
    const char *sentence = "2020-03-06 31-02-1991 1975년 7월 14일 1975.7.14 1988. 08. 02";
    const char *sentence_verbalized[] = {
            "이천이십년",
            NULL,
            "삼월",
            NULL,
            "육일",
            NULL,
            "삼 일",
            ",",
            "영 이",
            ",",
            "일 구 구 일",
            NULL,
            "천구백칠십오",
            "년",
            NULL,
            "칠",
            "월",
            NULL,
            "십사",
            "일",
            NULL,
            "천구백칠십오년",
            NULL,
            "칠월",
            NULL,
            "십사일",
            NULL,
            "천구백팔십팔년",
            NULL,
            NULL,
            "팔월",
            NULL,
            NULL,
            "이일",
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
            PV_NORMALIZER_LANGUAGE_KO,
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
        pv_normalizer_token_tag_ko_t tag) {
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
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_verbalizer_ko_init, PV_STATUS_INVALID_ARGUMENT)

    test_pv_normalizer_verbalizer_init_failure_helper(PV_STATUS_INVALID_ARGUMENT, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_ko_init` failed with status `INVALID_ARGUMENT`\\.");
}

static void test_pv_normalizer_verbalizer_verbalize_word_fail_calloc(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "hello";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_KO_WORD);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_verbalize_word` failed with status `OUT_OF_MEMORY`.");
}

static void test_pv_normalizer_verbalizer_verbalize_punctuation_fail_calloc(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = ",";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_KO_PUNCTUATION);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_verbalize_punctuation` failed with status `OUT_OF_MEMORY`.");
}

static void test_pv_normalizer_verbalizer_verbalize_cardinal_fail_calloc(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "5";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_KO_CARDINAL);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_verbalize_cardinal` failed with status `OUT_OF_MEMORY`.");
}

static void test_pv_normalizer_verbalizer_verbalize_negative_cardinal_fail_calloc(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "-5";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_KO_NEGATIVE_CARDINAL);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_verbalize_negative_cardinal` failed with status `OUT_OF_MEMORY`.");
}

static void test_pv_normalizer_verbalizer_verbalize_negative_cardinal_fail_calloc_2(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "-5";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_KO_NEGATIVE_CARDINAL);

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
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_KO_NUMBER_RANGE_TO);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_verbalize_number_range` failed with status `OUT_OF_MEMORY`.");
}

static void test_pv_normalizer_verbalizer_verbalize_currency_fail_calloc_1(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "$1";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_KO_CURRENCY);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `number_string`.");
}

static void test_pv_normalizer_verbalizer_verbalize_currency_fail_calloc_2(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "$1";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_KO_CURRENCY);

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
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_KO_NEGATIVE_CURRENCY);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `number_string`.");
}

static void test_pv_normalizer_verbalizer_verbalize_negative_currency_fail_calloc_2(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "-$1";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_KO_NEGATIVE_CURRENCY);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_real,
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `result`.");
}

#endif

static const pv_test_case_t PV_NORMALIZER_VERBALIZER_TEST_CASES[] = {
        {"verbalize_word", test_pv_normalizer_verbalizer_verbalize_word},
        {"verbalize_punctuation", test_pv_normalizer_verbalizer_verbalize_punctuation},
        {"verbalize_english_punctuation", test_pv_normalizer_verbalizer_verbalize_english_punctuation},
        {"verbalize_cardinal", test_pv_normalizer_verbalizer_verbalize_cardinal},
        {"verbalize_negative_cardinal", test_pv_normalizer_verbalizer_verbalize_negative_cardinal},
        {"verbalize_custom_pronunciation", test_pv_normalizer_verbalizer_verbalize_custom_pronunciation},
        {"number_to_string", test_pv_normalizer_verbalizer_number_to_string},
        {"verbalize_invalid_use_case", test_pv_normalizer_verbalizer_verbalize_invalid_use_case},
        {"verbalize_all", test_pv_normalizer_verbalizer_verbalize_all},
        {"verbalize_number_range", test_pv_normalizer_verbalizer_verbalize_number_range},
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
        {"verbalize_digits_sequence", test_pv_normalizer_verbalizer_verbalize_digits_sequence},
        {"verbalize_date", test_pv_normalizer_verbalizer_verbalize_date},

#ifdef __PV_MOCKS__

        {"verbalizer_init_calloc_failure", test_pv_normalizer_verbalizer_init_calloc_failure},
        {"verbalizer_init_internal_failure", test_pv_normalizer_verbalizer_init_internal_failure},
        {"verbalize_word_fail_calloc", test_pv_normalizer_verbalizer_verbalize_word_fail_calloc},
        {"verbalize_punctuation_fail_calloc", test_pv_normalizer_verbalizer_verbalize_punctuation_fail_calloc},
        {"verbalize_cardinal_fail_calloc", test_pv_normalizer_verbalizer_verbalize_cardinal_fail_calloc},
        {"verbalize_negative_cardinal_fail_calloc", test_pv_normalizer_verbalizer_verbalize_negative_cardinal_fail_calloc},
        {"verbalize_negative_cardinal_fail_calloc_2", test_pv_normalizer_verbalizer_verbalize_negative_cardinal_fail_calloc_2},
        {"verbalize_number_range_fail_calloc_1", test_pv_normalizer_verbalizer_verbalize_number_range_fail_calloc_1},
        {"verbalize_currency_fail_calloc_1", test_pv_normalizer_verbalizer_verbalize_currency_fail_calloc_1},
        {"verbalize_currency_fail_calloc_2", test_pv_normalizer_verbalizer_verbalize_currency_fail_calloc_2},
        {"verbalize_negative_currency_fail_calloc_1", test_pv_normalizer_verbalizer_verbalize_negative_currency_fail_calloc_1},
        {"verbalize_negative_currency_fail_calloc_2", test_pv_normalizer_verbalizer_verbalize_negative_currency_fail_calloc_2},

#endif
};

const pv_test_suite_t PV_NORMALIZER_VERBALIZER_KO_TEST_SUITE = {
        .name = "normalizer_verbalizer_ko",
        .setup = test_pv_normalizer_verbalizer_setup,
        .teardown = test_pv_normalizer_verbalizer_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_NORMALIZER_VERBALIZER_TEST_CASES),
        .test_cases = PV_NORMALIZER_VERBALIZER_TEST_CASES,
};
