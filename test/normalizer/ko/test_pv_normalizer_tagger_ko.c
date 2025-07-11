#include <stdlib.h>
#include <string.h>

#include "core/pv_language.h"
#include "core/pv_language_json.h"
#include "orca/normalizer/pv_normalizer_tagger.h"
#include "orca/normalizer/pv_normalizer_tokenizer.h"
#include "orca/normalizer/pv_normalizer_use_cases.h"
#include "test/pv_test.h"

#include "orca/normalizer/ko/pv_normalizer_tags_ko.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_normalizer_mock.h"

#endif

static pv_normalizer_tagger_t *normalizer_tagger_object = NULL;
static pv_normalizer_use_cases_ko_t ALL_TEST_USE_CASES[] = {
        PV_NORMALIZER_USE_WORD_NORMALIZER_KO,
        PV_NORMALIZER_USE_CUSTOM_PRONUNCIATION_NORMALIZER_KO,
        PV_NORMALIZER_USE_PUNCTUATION_NORMALIZER_KO,
        PV_NORMALIZER_USE_CARDINAL_NORMALIZER_KO,
        PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_KO,
        PV_NORMALIZER_USE_NUMBER_RANGE_NORMALIZER_KO,
        PV_NORMALIZER_USE_SPECIAL_CHARACTER_NORMALIZER_KO,
        PV_NORMALIZER_USE_DECIMAL_NORMALIZER_KO,
        PV_NORMALIZER_USE_MEASUREMENT_NORMALIZER_KO,
        PV_NORMALIZER_USE_ALPHANUM_SPELL_OUT_NORMALIZER_KO,
        PV_NORMALIZER_USE_TIME_NORMALIZER_KO,
        PV_NORMALIZER_USE_FRACTION_NORMALIZER_KO,
        PV_NORMALIZER_USE_URL_NORMALIZER_KO,
        PV_NORMALIZER_USE_CURRENCY_NORMALIZER_KO,
        PV_NORMALIZER_USE_DIGITS_SEQUENCE_NORMALIZER_KO,
        PV_NORMALIZER_USE_DATE_NORMALIZER_KO,
        PV_NORMALIZER_USE_NAME_NORMALIZER_KO,
};

static const char TAGGER_TEST_SENTENCE[] = "나는 비타민 B-12, C, 그리고 15개를 더 먹고 있어요!";

static const pv_normalizer_token_tag_ko_t TAGGER_TEST_SENTENCE_TAGS[] = {
        PV_NORMALIZER_TAG_KO_WORD,
        PV_NORMALIZER_TAG_KO_WORD,
        PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT,
        PV_NORMALIZER_TAG_KO_CARDINAL,
        PV_NORMALIZER_TAG_KO_PUNCTUATION,
        PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT,
        PV_NORMALIZER_TAG_KO_PUNCTUATION,
        PV_NORMALIZER_TAG_KO_WORD,
        PV_NORMALIZER_TAG_KO_CARDINAL,
        PV_NORMALIZER_TAG_KO_WORD,
        PV_NORMALIZER_TAG_KO_WORD,
        PV_NORMALIZER_TAG_KO_WORD,
        PV_NORMALIZER_TAG_KO_WORD,
        PV_NORMALIZER_TAG_KO_PUNCTUATION,
};

static pv_normalizer_tokenizer_t *normalizer_tokenizer_object = NULL;
static pv_language_info_t *language_info_object = NULL;

static const char LANGUAGE_INFO_PATH[] = "normalizer/ref/pv_language_info_normalizer_ko.json";

static pv_status_t test_pv_normalizer_tagger_setup(void) {
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
            (const int32_t *) ALL_TEST_USE_CASES,
            normalizer_tokenizer_object,
            NULL,
            &normalizer_tagger_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_normalizer_tagger_teardown(void) {
    pv_normalizer_tagger_delete(normalizer_tagger_object);
    pv_normalizer_tokenizer_delete(normalizer_tokenizer_object);
    pv_language_info_delete(language_info_object);
}

static void test_pv_normalizer_tagger_init_helper(
        int32_t num_use_cases,
        const pv_normalizer_use_cases_ko_t *use_cases,
        pv_normalizer_tagger_t **tagger) {
    *tagger = NULL;

    pv_status_t status = pv_normalizer_tagger_init(
            language_info_object,
            num_use_cases,
            (const int32_t *) use_cases,
            normalizer_tokenizer_object,
            NULL,
            tagger);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to initialize tagger");
}

static void test_pv_normalizer_tagger_tokenize_tag_helper(
        pv_normalizer_tokenizer_t *tokenizer,
        pv_normalizer_tagger_t *tagger,
        const char *sentence,
        int32_t target_num_tokens,
        bool preserve_word_boundary,
        pv_normalizer_token_list_t **token_list) {
    *token_list = NULL;

    pv_normalizer_token_list_t *token_list_internal = NULL;
    pv_status_t status = pv_normalizer_tokenizer_tokenize(
            tokenizer,
            sentence,
            pv_normalizer_tokenizer_default_word_boundary_character(tokenizer),
            preserve_word_boundary,
            false,
            false,
            false,
            &token_list_internal);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to tokenize sentence: `%s`", sentence);

    status = pv_normalizer_tagger_tag(tagger, token_list_internal, 0, true);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to tag sentence: `%s`", sentence);
    pv_test_true(
            token_list_internal->size == target_num_tokens,
            "incorrect number of tokens. got `%d`, expected `%d`",
            token_list_internal->size,
            target_num_tokens);

    *token_list = token_list_internal;
}

static void test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
        pv_normalizer_tokenizer_t *tokenizer,
        pv_normalizer_tagger_t *tagger,
        const char *sentence,
        int32_t num_target_tokens,
        bool preserve_word_boundary,
        const pv_normalizer_token_tag_ko_t *target_tags) {
    pv_normalizer_token_list_t *token_list = NULL;
    test_pv_normalizer_tagger_tokenize_tag_helper(
            tokenizer,
            tagger,
            sentence,
            num_target_tokens,
            preserve_word_boundary,
            &token_list);

    pv_normalizer_token_t *current = token_list->head;
    for (int32_t i = 0; i < token_list->size; i++) {
        pv_test_true(current != NULL, "token is empty");
        pv_test_true(
                current->tag == (int32_t) target_tags[i],
                "(%d) incorrect tag for `%s`: got `%d`, expected `%d`",
                i,
                current->string,
                current->tag,
                target_tags[i]);

        current = current->next;
    }

    pv_normalizer_token_list_delete(token_list);
}

static void test_pv_normalizer_tagger_tokenize_tag_and_check_strings_tags_helper(
        pv_normalizer_tokenizer_t *tokenizer,
        pv_normalizer_tagger_t *tagger,
        const char *sentence,
        int32_t num_target_tokens,
        const pv_normalizer_token_tag_ko_t *target_tags,
        bool preserve_word_boundary,
        const char **target_strings) {
    pv_normalizer_token_list_t *token_list = NULL;
    test_pv_normalizer_tagger_tokenize_tag_helper(
            tokenizer,
            tagger,
            sentence,
            num_target_tokens,
            preserve_word_boundary,
            &token_list);

    pv_normalizer_token_t *current = token_list->head;
    for (int32_t i = 0; i < token_list->size; i++) {
        pv_test_true(current != NULL, "token is empty");
        pv_test_true(
                strcmp(current->string, target_strings[i]) == 0,
                "incorrect string: got `%s`, expected `%s`",
                current->string,
                target_strings[i]);
        pv_test_true(
                current->tag == (int32_t) target_tags[i],
                "incorrect tag for `%s`: got %d, expected `%d`",
                current->string,
                current->tag,
                target_tags[i]);

        current = current->next;
    }

    pv_normalizer_token_list_delete(token_list);
}

static void test_pv_normalizer_tagger_tokenize_tag_and_check_status_helper(
        pv_normalizer_tokenizer_t *tokenizer,
        pv_normalizer_tagger_t *tagger,
        const char *sentence,
        const pv_status_t target_status) {
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
            status == target_status,
            "raised `%s` instead of `%s`",
            pv_status_to_string(status),
            pv_status_to_string(target_status));

    pv_normalizer_token_list_delete(token_list);
}

static void test_pv_normalizer_tagger_tokenize_tag_and_check_length_future_past_context_tags_helper(
        pv_normalizer_tokenizer_t *tokenizer,
        pv_normalizer_tagger_t *tagger,
        const char *sentence,
        int32_t num_target_tokens,
        bool preserve_word_boundary,
        const pv_normalizer_token_tag_ko_t *target_tags,
        const char **target_original_strings,
        const int32_t *target_length_future_context_list,
        const int32_t *target_length_past_context_list) {
    pv_normalizer_token_list_t *token_list = NULL;
    test_pv_normalizer_tagger_tokenize_tag_helper(
            tokenizer,
            tagger,
            sentence,
            num_target_tokens,
            preserve_word_boundary,
            &token_list);

    pv_normalizer_token_t *current = token_list->head;
    for (int32_t i = 0; i < token_list->size; i++) {
        pv_test_true(current != NULL, "token is empty");
        pv_test_true(
                current->tag == (int32_t) target_tags[i],
                "incorrect tag for `%s`: got `%d`, expected `%d`",
                current->string,
                current->tag,
                target_tags[i]);
        pv_test_true(
                strcmp(current->original_string, target_original_strings[i]) == 0,
                "incorrect string: got `%s`, expected `%s`",
                current->original_string,
                target_original_strings[i]);
        pv_test_true(
                current->length_future_context == target_length_future_context_list[i],
                "incorrect length_future_context (%d): got `%d`, expected `%d`",
                i,
                current->length_future_context,
                target_length_future_context_list[i]);
        pv_test_true(
                current->length_past_context == target_length_past_context_list[i],
                "incorrect length_past_context (%d): got `%d`, expected `%d`",
                i,
                current->length_past_context,
                target_length_past_context_list[i]);

        current = current->next;
    }

    pv_normalizer_token_list_delete(token_list);
}

static void test_pv_normalizer_tagger_tag_all(void) {
    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            TAGGER_TEST_SENTENCE,
            PV_ARRAY_LEN(TAGGER_TEST_SENTENCE_TAGS),
            false,
            TAGGER_TEST_SENTENCE_TAGS);
}

static void test_pv_normalizer_tagger_tag_word_preserve_boundary(void) {
    static pv_normalizer_use_cases_ko_t use_cases[] = {PV_NORMALIZER_USE_WORD_NORMALIZER_KO};

    const pv_normalizer_token_tag_ko_t sentence_tags[] = {
            PV_NORMALIZER_TAG_KO_WORD,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_WORD,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_NONE,
            PV_NORMALIZER_TAG_KO_NONE,
            PV_NORMALIZER_TAG_KO_PUNCTUATION,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_NONE,
            PV_NORMALIZER_TAG_KO_PUNCTUATION,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_WORD,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_NONE,
            PV_NORMALIZER_TAG_KO_WORD,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_WORD,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_WORD,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_WORD,
            PV_NORMALIZER_TAG_KO_PUNCTUATION,
    };

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(use_cases), use_cases, &tagger);

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            tagger,
            TAGGER_TEST_SENTENCE,
            PV_ARRAY_LEN(sentence_tags),
            true,
            sentence_tags);

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_tag_word(void) {
    static pv_normalizer_use_cases_ko_t use_cases[] = {PV_NORMALIZER_USE_WORD_NORMALIZER_KO};

    const pv_normalizer_token_tag_ko_t sentence_tags[] = {
            PV_NORMALIZER_TAG_KO_WORD,
            PV_NORMALIZER_TAG_KO_WORD,
            PV_NORMALIZER_TAG_KO_NONE,
            PV_NORMALIZER_TAG_KO_NONE,
            PV_NORMALIZER_TAG_KO_PUNCTUATION,
            PV_NORMALIZER_TAG_KO_NONE,
            PV_NORMALIZER_TAG_KO_PUNCTUATION,
            PV_NORMALIZER_TAG_KO_WORD,
            PV_NORMALIZER_TAG_KO_NONE,
            PV_NORMALIZER_TAG_KO_WORD,
            PV_NORMALIZER_TAG_KO_WORD,
            PV_NORMALIZER_TAG_KO_WORD,
            PV_NORMALIZER_TAG_KO_WORD,
            PV_NORMALIZER_TAG_KO_PUNCTUATION,
    };

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(use_cases), use_cases, &tagger);

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            tagger,
            TAGGER_TEST_SENTENCE,
            PV_ARRAY_LEN(sentence_tags),
            false,
            sentence_tags);

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_tag_punctuation(void) {
    static pv_normalizer_use_cases_ko_t use_cases[] = {PV_NORMALIZER_USE_PUNCTUATION_NORMALIZER_KO};

    const pv_normalizer_token_tag_ko_t sentence_tags[] = {
            PV_NORMALIZER_TAG_KO_NONE,
            PV_NORMALIZER_TAG_KO_NONE,
            PV_NORMALIZER_TAG_KO_NONE,
            PV_NORMALIZER_TAG_KO_NONE,
            PV_NORMALIZER_TAG_KO_PUNCTUATION,
            PV_NORMALIZER_TAG_KO_NONE,
            PV_NORMALIZER_TAG_KO_PUNCTUATION,
            PV_NORMALIZER_TAG_KO_NONE,
            PV_NORMALIZER_TAG_KO_NONE,
            PV_NORMALIZER_TAG_KO_NONE,
            PV_NORMALIZER_TAG_KO_NONE,
            PV_NORMALIZER_TAG_KO_NONE,
            PV_NORMALIZER_TAG_KO_NONE,
            PV_NORMALIZER_TAG_KO_PUNCTUATION,
    };

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(use_cases), use_cases, &tagger);

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            tagger,
            TAGGER_TEST_SENTENCE,
            PV_ARRAY_LEN(sentence_tags),
            false,
            sentence_tags);

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_tag_cardinal(void) {
    static pv_normalizer_use_cases_ko_t use_cases[] = {PV_NORMALIZER_USE_CARDINAL_NORMALIZER_KO};

    const pv_normalizer_token_tag_ko_t sentence_tags[] = {
            PV_NORMALIZER_TAG_KO_NONE,
            PV_NORMALIZER_TAG_KO_NONE,
            PV_NORMALIZER_TAG_KO_NONE,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_PUNCTUATION,
            PV_NORMALIZER_TAG_KO_NONE,
            PV_NORMALIZER_TAG_KO_PUNCTUATION,
            PV_NORMALIZER_TAG_KO_NONE,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_NONE,
            PV_NORMALIZER_TAG_KO_NONE,
            PV_NORMALIZER_TAG_KO_NONE,
            PV_NORMALIZER_TAG_KO_NONE,
            PV_NORMALIZER_TAG_KO_PUNCTUATION,
    };

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(use_cases), use_cases, &tagger);

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            tagger,
            TAGGER_TEST_SENTENCE,
            PV_ARRAY_LEN(sentence_tags),
            false,
            sentence_tags);

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_tag_negative_cardinal(void) {
    static pv_normalizer_use_cases_ko_t use_cases[] = {PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_KO};

    const char sentence[] = "123 -563";
    const pv_normalizer_token_tag_ko_t sentence_tags[] = {
            PV_NORMALIZER_TAG_KO_NONE,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_NEGATIVE_CARDINAL};

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(use_cases), use_cases, &tagger);

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            tagger,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            true,
            sentence_tags);

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_tag_custom_pronunciation(void) {
    static pv_normalizer_use_cases_ko_t use_cases[] = {PV_NORMALIZER_USE_CUSTOM_PRONUNCIATION_NORMALIZER_KO};

    const char sentence[] = "맞춤형 {발음|p a r ɯ m}";
    const pv_normalizer_token_tag_ko_t sentence_tags[] = {
            PV_NORMALIZER_TAG_KO_NONE,
            PV_NORMALIZER_TAG_KO_CUSTOM_PRONUNCIATION};

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(use_cases), use_cases, &tagger);

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            tagger,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            false,
            sentence_tags);

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_tag_invalid_use_case(void) {
    static pv_normalizer_use_cases_ko_t invalid_use_cases[] = {99};

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(invalid_use_cases), invalid_use_cases, &tagger);

    test_pv_normalizer_tagger_tokenize_tag_and_check_status_helper(
            normalizer_tokenizer_object,
            tagger,
            TAGGER_TEST_SENTENCE,
            PV_STATUS_INVALID_ARGUMENT);

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_tag_number_range(void) {
    const char sentence[] = "12-5 -4-2 안녕 43- -9 5-a a-5 6-33 50-50-50 1--3";
    const pv_normalizer_token_tag_ko_t sentence_tags[] = {
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_NUMBER_RANGE_TO,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_WORD,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_NUMBER_RANGE_TO,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_DIGITS,
            PV_NORMALIZER_TAG_KO_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_KO_DIGITS,
            PV_NORMALIZER_TAG_KO_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_KO_DIGITS,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_CARDINAL,
    };

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(ALL_TEST_USE_CASES), ALL_TEST_USE_CASES, &tagger);

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            tagger,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            true,
            sentence_tags);

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_tag_hyphenated_string(void) {
    static pv_normalizer_use_cases_ko_t use_cases[] = {PV_NORMALIZER_USE_WORD_NORMALIZER_KO};

    const char sentence[] = "4-분 안녕 -563 어디-있어 2-2 안녕-";
    const char *sentence_strings[] = {
            "4",
            "분",
            "안녕",
            "563",
            "어디",
            "있어",
            "2",
            "2",
            "안녕"};
    const pv_normalizer_token_tag_ko_t sentence_tags[] = {
            PV_NORMALIZER_TAG_KO_NONE,
            PV_NORMALIZER_TAG_KO_WORD,
            PV_NORMALIZER_TAG_KO_WORD,
            PV_NORMALIZER_TAG_KO_NONE,
            PV_NORMALIZER_TAG_KO_WORD,
            PV_NORMALIZER_TAG_KO_WORD,
            PV_NORMALIZER_TAG_KO_NONE,
            PV_NORMALIZER_TAG_KO_NONE,
            PV_NORMALIZER_TAG_KO_WORD,
    };

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(use_cases), use_cases, &tagger);

    test_pv_normalizer_tagger_tokenize_tag_and_check_strings_tags_helper(
            normalizer_tokenizer_object,
            tagger,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            sentence_tags,
            false,
            sentence_strings);

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_tag_invalid_hyphens_only(void) {
    static pv_normalizer_use_cases_ko_t use_cases[] = {PV_NORMALIZER_USE_WORD_NORMALIZER_KO};

    const char sentence[] = "- ---";

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(use_cases), use_cases, &tagger);

    test_pv_normalizer_tagger_tokenize_tag_and_check_status_helper(
            normalizer_tokenizer_object,
            tagger,
            sentence,
            PV_STATUS_SUCCESS);

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_correct_num_tag_weights(void) {
    pv_test_true(
            PV_ARRAY_LEN(PV_NORMALIZER_TOKEN_TAG_KO_WEIGHTS) == PV_NORMALIZER_TAG_KO_NUM_TAGS,
            "Number of tags and tag weights do not match. Have %d tags and %d weights",
            PV_ARRAY_LEN(PV_NORMALIZER_TOKEN_TAG_KO_WEIGHTS),
            PV_NORMALIZER_TAG_KO_NUM_TAGS);
}

static void test_pv_normalizer_tagger_tag_special_characters(void) {
    static pv_normalizer_use_cases_ko_t use_cases[] = {PV_NORMALIZER_USE_SPECIAL_CHARACTER_NORMALIZER_KO};

    const char sentence[] = "안녕 5% @@ °C RPM 90 CM²";
    const char *sentence_strings[] = {"안녕", "5", "%", "@", "@", "°C", "RPM", "90", "CM²"};
    const pv_normalizer_token_tag_ko_t sentence_tags[] = {
            PV_NORMALIZER_TAG_KO_NONE,
            PV_NORMALIZER_TAG_KO_NONE,
            PV_NORMALIZER_TAG_KO_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_KO_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_KO_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_KO_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_KO_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_KO_NONE,
            PV_NORMALIZER_TAG_KO_SPECIAL_CHARACTER,
    };

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(use_cases), use_cases, &tagger);

    test_pv_normalizer_tagger_tokenize_tag_and_check_strings_tags_helper(
            normalizer_tokenizer_object,
            tagger,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            sentence_tags,
            false,
            sentence_strings);

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_tag_negative_percent(void) {
    const char sentence[] = "-5%";
    const char *sentence_strings[] = {"-5", "%"};
    const pv_normalizer_token_tag_ko_t sentence_tags[] = {
            PV_NORMALIZER_TAG_KO_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_KO_SPECIAL_CHARACTER,
    };

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(ALL_TEST_USE_CASES), ALL_TEST_USE_CASES, &tagger);

    test_pv_normalizer_tagger_tokenize_tag_and_check_strings_tags_helper(
            normalizer_tokenizer_object,
            tagger,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            sentence_tags,
            false,
            sentence_strings);

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_tag_empty_custom_pron(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    pv_status_t status = pv_normalizer_token_list_init(&token_list);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to initialize token list");

    pv_normalizer_token_t *empty_token = NULL;
    status = pv_normalizer_token_init(0, 4, "{|AY}", false, true, false, &empty_token);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to initialize token");

    pv_normalizer_token_list_append_token(token_list, empty_token);

    status = pv_normalizer_tagger_tag(normalizer_tagger_object, token_list, 0, true);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to tag sentence: `%s`", TAGGER_TEST_SENTENCE);

    pv_normalizer_token_t *current = token_list->head;
    pv_test_true(current != NULL, "token is empty");
    pv_test_true(
            current->tag == PV_NORMALIZER_TAG_KO_CUSTOM_PRONUNCIATION,
            "incorrect tag for `%s`: got `%d`, expected `%d`",
            current->string,
            current->tag,
            PV_NORMALIZER_TAG_KO_CUSTOM_PRONUNCIATION);

    pv_normalizer_token_list_delete(token_list);
}

static void test_pv_normalizer_tagger_tag_weights_order(void) {
    pv_normalizer_use_cases_ko_t use_cases[] = {PV_NORMALIZER_USE_WORD_NORMALIZER_KO};

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(use_cases), use_cases, &tagger);

    pv_normalizer_token_list_t *token_list = NULL;
    pv_status_t status = pv_normalizer_token_list_init(&token_list);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to initialize token list");

    pv_normalizer_token_t *empty_token = NULL;
    status = pv_normalizer_token_init(0, 4, "{|AY}", false, true, false, &empty_token);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to initialize token");
    empty_token->tag = PV_NORMALIZER_TAG_KO_WORD;

    pv_normalizer_token_list_append_token(token_list, empty_token);

    status = pv_normalizer_tagger_tag(tagger, token_list, 0, true);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to tag sentence: `%s`", TAGGER_TEST_SENTENCE);

    pv_normalizer_token_t *current = token_list->head;
    pv_test_true(current != NULL, "token is empty");
    pv_test_true(
            current->tag == PV_NORMALIZER_TAG_KO_WORD,
            "incorrect tag for `%s`: got `%d`, expected `%d`",
            current->string,
            current->tag,
            PV_NORMALIZER_TAG_KO_WORD);

    pv_normalizer_tagger_delete(tagger);

    pv_normalizer_use_cases_ko_t use_cases2[] = {
            PV_NORMALIZER_USE_WORD_NORMALIZER_KO,
            PV_NORMALIZER_USE_CUSTOM_PRONUNCIATION_NORMALIZER_KO};

    tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(use_cases2), use_cases2, &tagger);

    status = pv_normalizer_tagger_tag(normalizer_tagger_object, token_list, 0, true);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to tag sentence: `%s`", TAGGER_TEST_SENTENCE);

    current = token_list->head;
    pv_test_true(current != NULL, "token is empty");
    pv_test_true(
            current->tag == PV_NORMALIZER_TAG_KO_CUSTOM_PRONUNCIATION,
            "incorrect tag for `%s`: got `%d`, expected `%d`",
            current->string,
            current->tag,
            PV_NORMALIZER_TAG_KO_CUSTOM_PRONUNCIATION);

    pv_normalizer_token_list_delete(token_list);
    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_next_char_is_space(void) {
    static pv_normalizer_use_cases_ko_t use_cases[] = {PV_NORMALIZER_USE_WORD_NORMALIZER_KO};

    const char sentence[] = "4-분, 안녕 -563 어디-있어 2-2 안녕-";
    const bool sentence_next_character_is_space[] = {
            true,
            false,
            true,
            true,
            true,
            true,
            true,
            true,
            true,
            false,
    };
    int32_t num_tokens = 10;

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(use_cases), use_cases, &tagger);

    pv_normalizer_token_list_t *token_list = NULL;
    test_pv_normalizer_tagger_tokenize_tag_helper(
            normalizer_tokenizer_object,
            tagger,
            sentence,
            num_tokens,
            false,
            &token_list);

    pv_normalizer_token_t *current = token_list->head;
    for (int32_t i = 0; i < token_list->size; i++) {
        pv_test_true(current != NULL, "token is empty");
        pv_test_true(
                current->next_character_is_space == sentence_next_character_is_space[i],
                "incorrect next_character_is_space for %s: got %d, expected %d",
                current->string,
                current->tag,
                sentence_next_character_is_space[i]);

        current = current->next;
    }
    pv_normalizer_token_list_delete(token_list);
    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_tag_decimal(void) {
    const char sentence[] = ".1 1.2 .456 1..2 a.6 .5% -1.1";
    const pv_normalizer_token_tag_ko_t sentence_tags[] = {
            PV_NORMALIZER_TAG_KO_DECIMAL_POINT,
            PV_NORMALIZER_TAG_KO_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_DECIMAL_POINT,
            PV_NORMALIZER_TAG_KO_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_DECIMAL_POINT,
            PV_NORMALIZER_TAG_KO_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_DOT,
            PV_NORMALIZER_TAG_KO_DOT,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_DOT,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_DECIMAL_POINT,
            PV_NORMALIZER_TAG_KO_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_KO_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_KO_DECIMAL_POINT,
            PV_NORMALIZER_TAG_KO_DECIMAL_DIGITS,
    };

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(ALL_TEST_USE_CASES), ALL_TEST_USE_CASES, &tagger);

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            tagger,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            true,
            sentence_tags);

    const char sentence2[] = ". 5";
    const pv_normalizer_token_tag_ko_t sentence_tags2[] = {
            PV_NORMALIZER_TAG_KO_PUNCTUATION,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL};

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            tagger,
            sentence2,
            PV_ARRAY_LEN(sentence_tags2),
            true,
            sentence_tags2);

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_tag_alphanum_spell_out(void) {
    const char sentence[] = "HTML5 A87B 12A 12 하이B4";
    const pv_normalizer_token_tag_ko_t sentence_tags[] = {
            PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_WORD,
            PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_CARDINAL_SPELL_OUT,
    };

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(ALL_TEST_USE_CASES), ALL_TEST_USE_CASES, &tagger);

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            tagger,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            true,
            sentence_tags);

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_tag_time(void) {
    const char sentence[] =
            "7:00 p.m. 오전 09:18 18:23 21:89 25:09 p.m. am 8 a.m.. 04:21am 12:00p.m. 2AM 25PM 2a.m. 12p.m.";
    const pv_normalizer_token_tag_ko_t sentence_tags[] = {
            PV_NORMALIZER_TAG_KO_TIME_HOURS,
            PV_NORMALIZER_TAG_KO_TIME_COLON,
            PV_NORMALIZER_TAG_KO_TIME_MINUTES,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_TIME_AM_PM,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_WORD,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_TIME_HOURS,
            PV_NORMALIZER_TAG_KO_TIME_COLON,
            PV_NORMALIZER_TAG_KO_TIME_MINUTES,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_TIME_HOURS,
            PV_NORMALIZER_TAG_KO_TIME_COLON,
            PV_NORMALIZER_TAG_KO_TIME_MINUTES,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_COLON,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_COLON,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_TIME_AM_PM,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_TIME_HOURS,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_TIME_AM_PM,
            PV_NORMALIZER_TAG_KO_PUNCTUATION,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_TIME_HOURS,
            PV_NORMALIZER_TAG_KO_TIME_COLON,
            PV_NORMALIZER_TAG_KO_TIME_MINUTES,
            PV_NORMALIZER_TAG_KO_TIME_AM_PM,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_TIME_HOURS,
            PV_NORMALIZER_TAG_KO_TIME_COLON,
            PV_NORMALIZER_TAG_KO_TIME_MINUTES,
            PV_NORMALIZER_TAG_KO_TIME_AM_PM,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_TIME_HOURS,
            PV_NORMALIZER_TAG_KO_TIME_AM_PM,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_TIME_HOURS,
            PV_NORMALIZER_TAG_KO_TIME_AM_PM,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_TIME_HOURS,
            PV_NORMALIZER_TAG_KO_TIME_AM_PM,
    };

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(ALL_TEST_USE_CASES), ALL_TEST_USE_CASES, &tagger);

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            tagger,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            true,
            sentence_tags);

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_tag_measurement(void) {
    const char sentence[] = "5 g 1ml -7.3l 안녕 ML 10.1km 5°C 1,000g -1,000,111g 25ft.-lb. 명확한 맥락 ml-g 맥 락 h-l스코어";
    const pv_normalizer_token_tag_ko_t sentence_tags[] = {
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_MEASUREMENT,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_MEASUREMENT,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_KO_DECIMAL_POINT,
            PV_NORMALIZER_TAG_KO_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_KO_MEASUREMENT,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_WORD,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_MEASUREMENT,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_DECIMAL_POINT,
            PV_NORMALIZER_TAG_KO_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_KO_MEASUREMENT,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_MEASUREMENT,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_MEASUREMENT,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_KO_MEASUREMENT,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_MEASUREMENT,
            PV_NORMALIZER_TAG_KO_DOT,
            PV_NORMALIZER_TAG_KO_MEASUREMENT,
            PV_NORMALIZER_TAG_KO_PUNCTUATION,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_WORD,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_WORD,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_MEASUREMENT,
            PV_NORMALIZER_TAG_KO_MEASUREMENT,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_WORD,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_WORD,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_WORD
    };

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(ALL_TEST_USE_CASES), ALL_TEST_USE_CASES, &tagger);

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            tagger,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            true,
            sentence_tags);

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_tag_per_measurement(void) {
    const char sentence[] = "5 km/m 10 oz/km 1.1m/l -5.41kg/ft m/s 7 마일/시간 1,000,111m/s";
    const pv_normalizer_token_tag_ko_t sentence_tags[] = {
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_MEASUREMENT,
            PV_NORMALIZER_TAG_KO_PER_SLASH,
            PV_NORMALIZER_TAG_KO_MEASUREMENT,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_MEASUREMENT,
            PV_NORMALIZER_TAG_KO_PER_SLASH,
            PV_NORMALIZER_TAG_KO_MEASUREMENT,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_DECIMAL_POINT,
            PV_NORMALIZER_TAG_KO_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_KO_MEASUREMENT,
            PV_NORMALIZER_TAG_KO_PER_SLASH,
            PV_NORMALIZER_TAG_KO_MEASUREMENT,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_KO_DECIMAL_POINT,
            PV_NORMALIZER_TAG_KO_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_KO_MEASUREMENT,
            PV_NORMALIZER_TAG_KO_PER_SLASH,
            PV_NORMALIZER_TAG_KO_MEASUREMENT,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_MEASUREMENT,
            PV_NORMALIZER_TAG_KO_PER_SLASH,
            PV_NORMALIZER_TAG_KO_MEASUREMENT,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_WORD,
            PV_NORMALIZER_TAG_KO_PER_SLASH,
            PV_NORMALIZER_TAG_KO_WORD,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_MEASUREMENT,
            PV_NORMALIZER_TAG_KO_PER_SLASH,
            PV_NORMALIZER_TAG_KO_MEASUREMENT,
    };

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(ALL_TEST_USE_CASES), ALL_TEST_USE_CASES, &tagger);

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            tagger,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            true,
            sentence_tags);

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_tag_comma_cardinal(void) {
    const char *sentence = "1,005 11,005 111,005 -5,123,123,123 1111,000 1,1,000 1, 000";

    const pv_normalizer_token_tag_ko_t sentence_tags[] = {
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_PUNCTUATION,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_PUNCTUATION,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_PUNCTUATION,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_PUNCTUATION,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL,
    };

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(ALL_TEST_USE_CASES), ALL_TEST_USE_CASES, &tagger);

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            tagger,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            true,
            sentence_tags);

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_tag_comma_number_range(void) {
    const char *sentence = "1,000-2,000 1,000.2-1,000.5 1,000~2,000 1,000〜2,000 1,000～2,000";

    const pv_normalizer_token_tag_ko_t sentence_tags[] = {
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_NUMBER_RANGE_TO,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_DECIMAL_POINT,
            PV_NORMALIZER_TAG_KO_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_KO_NUMBER_RANGE_TO,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_DECIMAL_POINT,
            PV_NORMALIZER_TAG_KO_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_NUMBER_RANGE_TO,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_NUMBER_RANGE_TO,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_NUMBER_RANGE_TO,
            PV_NORMALIZER_TAG_KO_CARDINAL,
    };

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(ALL_TEST_USE_CASES), ALL_TEST_USE_CASES, &tagger);

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            tagger,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            true,
            sentence_tags);

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_tag_comma_decimal(void) {
    const char *sentence = "1,000.2 -300,000.6666";

    const pv_normalizer_token_tag_ko_t sentence_tags[] = {
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_DECIMAL_POINT,
            PV_NORMALIZER_TAG_KO_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_KO_DECIMAL_POINT,
            PV_NORMALIZER_TAG_KO_DECIMAL_DIGITS};

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(ALL_TEST_USE_CASES), ALL_TEST_USE_CASES, &tagger);

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            tagger,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            true,
            sentence_tags);

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_tag_fraction(void) {
    const char *sentence = "1/2 1/3.2 1/1,000 1/-4 -4/1";

    const pv_normalizer_token_tag_ko_t sentence_tags[] = {
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_FRACTION_SLASH,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_FRACTION_SLASH,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_DECIMAL_POINT,
            PV_NORMALIZER_TAG_KO_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_FRACTION_SLASH,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_FRACTION_SLASH,
            PV_NORMALIZER_TAG_KO_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_KO_FRACTION_SLASH,
            PV_NORMALIZER_TAG_KO_CARDINAL,
    };

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(ALL_TEST_USE_CASES), ALL_TEST_USE_CASES, &tagger);

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            tagger,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            true,
            sentence_tags);

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_tag_url(void) {
    const char *sentence = "www.example.com https://hi.xyz/";

    const pv_normalizer_token_tag_ko_t sentence_tags[] = {
            PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_DOT,
            PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_DOT,
            PV_NORMALIZER_TAG_KO_TOP_LEVEL_DOMAIN,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_COLON,
            PV_NORMALIZER_TAG_KO_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_KO_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_DOT,
            PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_SPECIAL_CHARACTER,
    };

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(ALL_TEST_USE_CASES), ALL_TEST_USE_CASES, &tagger);

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            tagger,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            true,
            sentence_tags);

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_tag_currency(void) {
    const char *sentence =
            "$5 €10 10$ 10€ $1.25 1.25$ -$500 -500$ -1.25$ -$1.25 €1,000 -€1,000 1,000€ -1,000€ €1,000.25 1,000.25€ "
            "1.2$ $1.222 1USD USD1.25 1.25USD -1USD -1.25USD 1 $ -1 $ 1.25 $ -1.25 $";

    const pv_normalizer_token_tag_ko_t sentence_tags[] = {
            PV_NORMALIZER_TAG_KO_CURRENCY,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CURRENCY,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CURRENCY,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CURRENCY,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CURRENCY,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CURRENCY,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CURRENCY,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CURRENCY,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CURRENCY,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CURRENCY,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_DOT,
            PV_NORMALIZER_TAG_KO_NONE,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CURRENCY,
            PV_NORMALIZER_TAG_KO_DOT,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CURRENCY,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CURRENCY,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CURRENCY,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CURRENCY_SYMBOL,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CURRENCY_SYMBOL,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_DECIMAL_POINT,
            PV_NORMALIZER_TAG_KO_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CURRENCY_SYMBOL,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_KO_DECIMAL_POINT,
            PV_NORMALIZER_TAG_KO_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CURRENCY_SYMBOL,
    };

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(ALL_TEST_USE_CASES), ALL_TEST_USE_CASES, &tagger);

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            tagger,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            true,
            sentence_tags);

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_tag_digits_sequence(void) {
    const char *sentence = "(778) 239-1823 102-291091-4920 921.212.1203";
    const pv_normalizer_token_tag_ko_t sentence_tags[] = {
            PV_NORMALIZER_TAG_KO_DIGITS_WITH_PARENTHESES,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_DIGITS,
            PV_NORMALIZER_TAG_KO_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_KO_DIGITS,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_DIGITS,
            PV_NORMALIZER_TAG_KO_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_KO_DIGITS,
            PV_NORMALIZER_TAG_KO_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_KO_DIGITS,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_DIGITS,
            PV_NORMALIZER_TAG_KO_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_KO_DIGITS,
            PV_NORMALIZER_TAG_KO_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_KO_DIGITS,
    };

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(ALL_TEST_USE_CASES), ALL_TEST_USE_CASES, &tagger);

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            tagger,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            true,
            sentence_tags);

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_tag_date(void) {
    const char *sentence = "2020-03-06 31-02-1991 1975년 7월 14일 1975.7.14 1988. 08. 02 02.08.1972 1988.08. 02";
    const pv_normalizer_token_tag_ko_t sentence_tags[] = {
            PV_NORMALIZER_TAG_KO_DATE_YEAR,
            PV_NORMALIZER_TAG_KO_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_KO_DATE_MONTH,
            PV_NORMALIZER_TAG_KO_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_KO_DATE_DAY,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_DIGITS,
            PV_NORMALIZER_TAG_KO_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_KO_DIGITS,
            PV_NORMALIZER_TAG_KO_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_KO_DIGITS,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_WORD,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_WORD,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_WORD,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_DATE_YEAR,
            PV_NORMALIZER_TAG_KO_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_KO_DATE_MONTH,
            PV_NORMALIZER_TAG_KO_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_KO_DATE_DAY,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_DATE_YEAR,
            PV_NORMALIZER_TAG_KO_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_DATE_MONTH,
            PV_NORMALIZER_TAG_KO_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_DATE_DAY,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_DIGITS,
            PV_NORMALIZER_TAG_KO_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_KO_DIGITS,
            PV_NORMALIZER_TAG_KO_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_KO_DIGITS,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_DECIMAL_POINT,
            PV_NORMALIZER_TAG_KO_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_KO_PUNCTUATION,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL,
    };

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(ALL_TEST_USE_CASES), ALL_TEST_USE_CASES, &tagger);

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            tagger,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            true,
            sentence_tags);

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_length_future_past_context_after_tag(void) {
    const char *sentence = "3-5 일 이내에. @AB12 www.ex.com 9:30 1/2 1/1,000 2ml 2m/h -1,900,876";
    const pv_normalizer_token_tag_ko_t sentence_tags[] = {
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_NUMBER_RANGE_TO,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_WORD,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_WORD,
            PV_NORMALIZER_TAG_KO_PUNCTUATION,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_DOT,
            PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_KO_DOT,
            PV_NORMALIZER_TAG_KO_TOP_LEVEL_DOMAIN,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_TIME_HOURS,
            PV_NORMALIZER_TAG_KO_TIME_COLON,
            PV_NORMALIZER_TAG_KO_TIME_MINUTES,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_FRACTION_SLASH,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_FRACTION_SLASH,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_MEASUREMENT,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_CARDINAL,
            PV_NORMALIZER_TAG_KO_MEASUREMENT,
            PV_NORMALIZER_TAG_KO_PER_SLASH,
            PV_NORMALIZER_TAG_KO_MEASUREMENT,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_NEGATIVE_CARDINAL,
    };
    const char *sentence_original_strings[] = {
            "3-5",
            "3-5",
            "3-5",
            " ",
            "일",
            " ",
            "이내에",
            ".",
            " ",
            "@AB12",
            "@AB12",
            "@AB12",
            "@AB12",
            "@AB12",
            " ",
            "www.ex.com",
            "www.ex.com",
            "www.ex.com",
            "www.ex.com",
            "www.ex.com",
            "www.ex.com",
            "www.ex.com",
            "www.ex.com",
            " ",
            "9:30",
            "9:30",
            "9:30",
            " ",
            "1/2",
            "1/2",
            "1/2",
            " ",
            "1/1,000",
            "1/1,000",
            "1/1,000",
            " ",
            "2ml",
            "2ml",
            " ",
            "2m/h",
            "2m/h",
            "2m/h",
            "2m/h",
            " ",
            "-1,900,876",
    };
    const int32_t sentence_futures[] = {
            2,
            1,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            4,
            3,
            2,
            1,
            0,
            0,
            7,
            6,
            5,
            4,
            3,
            2,
            1,
            0,
            0,
            2,
            1,
            0,
            0,
            2,
            1,
            0,
            0,
            2,
            1,
            0,
            0,
            1,
            0,
            0,
            3,
            2,
            1,
            0,
            0,
            0,
    };
    const int32_t sentence_pasts[] = {
            0,
            1,
            2,
            0,
            0,
            0,
            0,
            0,
            0,
            0,
            1,
            2,
            3,
            4,
            0,
            0,
            1,
            2,
            3,
            4,
            5,
            6,
            7,
            0,
            0,
            1,
            2,
            0,
            0,
            1,
            2,
            0,
            0,
            1,
            2,
            0,
            0,
            1,
            0,
            0,
            1,
            2,
            3,
            0,
            0,
    };

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(ALL_TEST_USE_CASES), ALL_TEST_USE_CASES, &tagger);

    test_pv_normalizer_tagger_tokenize_tag_and_check_length_future_past_context_tags_helper(
            normalizer_tokenizer_object,
            tagger,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            true,
            sentence_tags,
            sentence_original_strings,
            sentence_futures,
            sentence_pasts);

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_length_future_past_context_after_tag_date_digits(void) {
    const char *sentence = "2023.12.25 2023. 12. 25 1829-06-03 (778) 239-1823 102-291-4920 123.456.789";
    const pv_normalizer_token_tag_ko_t sentence_tags[] = {
            PV_NORMALIZER_TAG_KO_DATE_YEAR,
            PV_NORMALIZER_TAG_KO_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_KO_DATE_MONTH,
            PV_NORMALIZER_TAG_KO_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_KO_DATE_DAY,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_DATE_YEAR,
            PV_NORMALIZER_TAG_KO_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_DATE_MONTH,
            PV_NORMALIZER_TAG_KO_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_DATE_DAY,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_DATE_YEAR,
            PV_NORMALIZER_TAG_KO_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_KO_DATE_MONTH,
            PV_NORMALIZER_TAG_KO_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_KO_DATE_DAY,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_DIGITS_WITH_PARENTHESES,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_DIGITS,
            PV_NORMALIZER_TAG_KO_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_KO_DIGITS,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_DIGITS,
            PV_NORMALIZER_TAG_KO_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_KO_DIGITS,
            PV_NORMALIZER_TAG_KO_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_KO_DIGITS,
            PV_NORMALIZER_TAG_KO_SPACE,
            PV_NORMALIZER_TAG_KO_DIGITS,
            PV_NORMALIZER_TAG_KO_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_KO_DIGITS,
            PV_NORMALIZER_TAG_KO_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_KO_DIGITS,
    };
    const char *sentence_original_strings[] = {
            "2023.12.25",
            "2023.12.25",
            "2023.12.25",
            "2023.12.25",
            "2023.12.25",
            " ",
            "2023. 12. 25",
            "2023. 12. 25",
            "2023. 12. 25",
            "2023. 12. 25",
            "2023. 12. 25",
            "2023. 12. 25",
            "2023. 12. 25",
            " ",
            "1829-06-03",
            "1829-06-03",
            "1829-06-03",
            "1829-06-03",
            "1829-06-03",
            " ",
            "(778)",
            " ",
            "239-1823",
            "239-1823",
            "239-1823",
            " ",
            "102-291-4920",
            "102-291-4920",
            "102-291-4920",
            "102-291-4920",
            "102-291-4920",
            " ",
            "123.456.789",
            "123.456.789",
            "123.456.789",
            "123.456.789",
            "123.456.789",
    };
    const int32_t sentence_futures[] = {
            4,
            3,
            2,
            1,
            0,
            0,
            6,
            5,
            4,
            3,
            2,
            1,
            0,
            0,
            4,
            3,
            2,
            1,
            0,
            0,
            0,
            0,
            2,
            1,
            0,
            0,
            4,
            3,
            2,
            1,
            0,
            0,
            4,
            3,
            2,
            1,
            0,
            0,
    };
    const int32_t sentence_pasts[] = {
            0,
            1,
            2,
            3,
            4,
            0,
            0,
            1,
            2,
            3,
            4,
            5,
            6,
            0,
            0,
            1,
            2,
            3,
            4,
            0,
            0,
            0,
            0,
            1,
            2,
            0,
            0,
            1,
            2,
            3,
            4,
            0,
            0,
            1,
            2,
            3,
            4,
            0,
    };

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(ALL_TEST_USE_CASES), ALL_TEST_USE_CASES, &tagger);

    test_pv_normalizer_tagger_tokenize_tag_and_check_length_future_past_context_tags_helper(
            normalizer_tokenizer_object,
            tagger,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            true,
            sentence_tags,
            sentence_original_strings,
            sentence_futures,
            sentence_pasts);

    pv_normalizer_tagger_delete(tagger);
}


#ifdef __PV_MOCKS__

static void test_pv_normalizer_tagger_init_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_tagger_ko_init, PV_STATUS_OUT_OF_MEMORY);

    static pv_normalizer_use_cases_ko_t use_cases[] = {PV_NORMALIZER_USE_WORD_NORMALIZER_KO};
    pv_normalizer_tagger_t *tagger = NULL;

    pv_status_t status = pv_normalizer_tagger_init(
            language_info_object,
            PV_ARRAY_LEN(use_cases),
            (const int32_t *) use_cases,
            normalizer_tokenizer_object,
            NULL,
            &tagger);
    pv_test_true(status == PV_STATUS_OUT_OF_MEMORY, "failed to fail with `PV_STATUS_OUT_OF_MEMORY`, got status `%s`", pv_status_to_string(status));
}


static void test_pv_normalizer_tagger_tag_cardinal_helper_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_util_check_token_is_before_character, PV_STATUS_INVALID_ARGUMENT);

    const char sentence[] = "123";
    static pv_normalizer_use_cases_ko_t use_cases[] = {PV_NORMALIZER_USE_CARDINAL_NORMALIZER_KO};
    pv_normalizer_tagger_t *tagger = NULL;
    pv_normalizer_token_list_t *token_list = NULL;

    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(use_cases), use_cases, &tagger);

    pv_status_t status = pv_normalizer_tokenizer_tokenize(
            normalizer_tokenizer_object,
            sentence,
            pv_normalizer_tokenizer_default_word_boundary_character(normalizer_tokenizer_object),
            true,
            false,
            false,
            false,
            &token_list);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to tokenize sentence: `%s`", sentence);
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    status = pv_normalizer_tagger_tag(tagger, token_list, 0, true);
    pv_test_true(
            status == PV_STATUS_INVALID_ARGUMENT,
            "mock error, expected status `%s`, got status `%s`",
            pv_status_to_string(PV_STATUS_INVALID_ARGUMENT),
            pv_status_to_string(status));

    pv_normalizer_tagger_delete(tagger);
    pv_normalizer_token_list_delete(token_list);
}


static void test_pv_normalizer_tagger_tag_currency_helper_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_util_check_token_is_before_character, PV_STATUS_INVALID_ARGUMENT);

    const char sentence[] = "5$";
    static pv_normalizer_use_cases_ko_t use_cases[] = {PV_NORMALIZER_USE_CURRENCY_NORMALIZER_KO};
    pv_normalizer_tagger_t *tagger = NULL;
    pv_normalizer_token_list_t *token_list = NULL;

    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(use_cases), use_cases, &tagger);

    pv_status_t status = pv_normalizer_tokenizer_tokenize(
            normalizer_tokenizer_object,
            sentence,
            pv_normalizer_tokenizer_default_word_boundary_character(normalizer_tokenizer_object),
            true,
            false,
            false,
            false,
            &token_list);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to tokenize sentence: `%s`", sentence);
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    status = pv_normalizer_tagger_tag(tagger, token_list, 0, true);
    pv_test_true(
            status == PV_STATUS_INVALID_ARGUMENT,
            "mock error, expected status `%s`, got status `%s`",
            pv_status_to_string(PV_STATUS_INVALID_ARGUMENT),
            pv_status_to_string(status));

    pv_normalizer_tagger_delete(tagger);
    pv_normalizer_token_list_delete(token_list);
}

#endif


static const pv_test_case_t PV_NORMALIZER_TAGGER_TEST_CASES[] = {
        {"tag_word_preserve_boundary", test_pv_normalizer_tagger_tag_word_preserve_boundary},
        {"tag_word", test_pv_normalizer_tagger_tag_word},
        {"tag_punctuation", test_pv_normalizer_tagger_tag_punctuation},
        {"tag_cardinal", test_pv_normalizer_tagger_tag_cardinal},
        {"tag_negative_cardinal", test_pv_normalizer_tagger_tag_negative_cardinal},
        {"tag_custom_pronunciation", test_pv_normalizer_tagger_tag_custom_pronunciation},
        {"tag_invalid_use_case", test_pv_normalizer_tagger_tag_invalid_use_case},
        {"tag_hyphenated_string", test_pv_normalizer_tagger_tag_hyphenated_string},
        {"tag_invalid_hyphens_only", test_pv_normalizer_tagger_tag_invalid_hyphens_only},
        {"tag_number_range", test_pv_normalizer_tagger_tag_number_range},
        {"test_correct_num_tag_weights", test_pv_normalizer_tagger_correct_num_tag_weights},

        {"tag_special_character", test_pv_normalizer_tagger_tag_special_characters},
        {"tag_negative_percent", test_pv_normalizer_tagger_tag_negative_percent},
        {"tag_all", test_pv_normalizer_tagger_tag_all},
        {"tag_empty_custom_pron", test_pv_normalizer_tagger_tag_empty_custom_pron},
        {"tag_weights_order", test_pv_normalizer_tagger_tag_weights_order},
        {"next_character_is_space", test_pv_normalizer_tagger_next_char_is_space},
        {"tag_decimal", test_pv_normalizer_tagger_tag_decimal},
        {"tag_measurement", test_pv_normalizer_tagger_tag_measurement},
        {"tag_per_measurement", test_pv_normalizer_tagger_tag_per_measurement},
        {"tag_alphanum_spell_out", test_pv_normalizer_tagger_tag_alphanum_spell_out},
        {"tag_time", test_pv_normalizer_tagger_tag_time},
        {"tag_comma_cardinal", test_pv_normalizer_tagger_tag_comma_cardinal},
        {"tag_comma_number_range", test_pv_normalizer_tagger_tag_comma_number_range},
        {"tag_comma_decimal", test_pv_normalizer_tagger_tag_comma_decimal},
        {"tag_fraction", test_pv_normalizer_tagger_tag_fraction},
        {"tag_url", test_pv_normalizer_tagger_tag_url},
        {"tag_currency", test_pv_normalizer_tagger_tag_currency},
        {"tag_digits_sequence", test_pv_normalizer_tagger_tag_digits_sequence},
        {"tag_date", test_pv_normalizer_tagger_tag_date},
        {"length_future_past_context", test_pv_normalizer_tagger_length_future_past_context_after_tag},
        {"length_future_past_date_digits", test_pv_normalizer_tagger_length_future_past_context_after_tag_date_digits},

#ifdef __PV_MOCKS__

        {"tagger_init_failure", test_pv_normalizer_tagger_init_failure},
        {"tagger_tag_cardinal_helper_failure", test_pv_normalizer_tagger_tag_cardinal_helper_failure},
        {"tagger_tag_currency_helper_failure", test_pv_normalizer_tagger_tag_currency_helper_failure},

#endif

};

const pv_test_suite_t PV_NORMALIZER_TAGGER_KO_TEST_SUITE = {
        .name = "normalizer_tagger_ko",
        .setup = test_pv_normalizer_tagger_setup,
        .teardown = test_pv_normalizer_tagger_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_NORMALIZER_TAGGER_TEST_CASES),
        .test_cases = PV_NORMALIZER_TAGGER_TEST_CASES,
};
