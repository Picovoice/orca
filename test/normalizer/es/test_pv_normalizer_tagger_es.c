#include <stdlib.h>
#include <string.h>

#include "core/pv_language.h"
#include "core/pv_language_json.h"
#include "orca/normalizer/pv_normalizer_tagger.h"
#include "orca/normalizer/pv_normalizer_tokenizer.h"
#include "orca/normalizer/pv_normalizer_use_cases.h"
#include "test/pv_test.h"

#include "orca/normalizer/es/pv_normalizer_tags_es.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_normalizer_mock.h"

#endif

static pv_normalizer_tagger_t *normalizer_tagger_object = NULL;
static pv_normalizer_use_cases_es_t ALL_TEST_USE_CASES[] = {
        PV_NORMALIZER_USE_WORD_NORMALIZER_ES,
        PV_NORMALIZER_USE_CUSTOM_PRONUNCIATION_NORMALIZER_ES,
        PV_NORMALIZER_USE_PUNCTUATION_NORMALIZER_ES,
        PV_NORMALIZER_USE_CARDINAL_NORMALIZER_ES,
        PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_ES,
        PV_NORMALIZER_USE_NUMBER_RANGE_NORMALIZER_ES,
        PV_NORMALIZER_USE_ORDINAL_NORMALIZER_ES,
        PV_NORMALIZER_USE_NEGATIVE_ORDINAL_NORMALIZER_ES,
        PV_NORMALIZER_USE_SPECIAL_CHARACTER_NORMALIZER_ES,
        PV_NORMALIZER_USE_DECIMAL_NORMALIZER_ES,
        PV_NORMALIZER_USE_ALPHANUM_SPELL_OUT_NORMALIZER_ES,
        PV_NORMALIZER_USE_MEASUREMENT_NORMALIZER_ES,
        PV_NORMALIZER_USE_TIME_NORMALIZER_ES,
        PV_NORMALIZER_USE_FRACTION_NORMALIZER_ES,
        PV_NORMALIZER_USE_URL_NORMALIZER_ES,
        PV_NORMALIZER_USE_CURRENCY_NORMALIZER_ES,
        PV_NORMALIZER_USE_ABBREVIATION_NORMALIZER_ES,
        PV_NORMALIZER_USE_DIGITS_SEQUENCE_NORMALIZER_ES,
        PV_NORMALIZER_USE_DATE_NORMALIZER_ES,
        PV_NORMALIZER_USE_NAME_NORMALIZER_ES,
};

static const char TAGGER_TEST_SENTENCE[] = "¡mañana tomaré vitamina b-12, c, y 15 más!";

static const pv_normalizer_token_tag_es_t TAGGER_TEST_SENTENCE_TAGS[] = {
        PV_NORMALIZER_TAG_ES_PUNCTUATION,
        PV_NORMALIZER_TAG_ES_WORD,
        PV_NORMALIZER_TAG_ES_WORD,
        PV_NORMALIZER_TAG_ES_WORD,
        PV_NORMALIZER_TAG_ES_WORD,
        PV_NORMALIZER_TAG_ES_CARDINAL,
        PV_NORMALIZER_TAG_ES_PUNCTUATION,
        PV_NORMALIZER_TAG_ES_WORD,
        PV_NORMALIZER_TAG_ES_PUNCTUATION,
        PV_NORMALIZER_TAG_ES_WORD,
        PV_NORMALIZER_TAG_ES_CARDINAL,
        PV_NORMALIZER_TAG_ES_WORD,
        PV_NORMALIZER_TAG_ES_PUNCTUATION,
};

static pv_normalizer_tokenizer_t *normalizer_tokenizer_object = NULL;
static pv_language_info_t *language_info_object = NULL;
static pv_noun_gender_dict_t *noun_gender_dict_object = NULL;

static const char LANGUAGE_INFO_PATH[] = "normalizer/ref/pv_language_info_normalizer_es.json";
static const char NOUN_GENDER_DICT_PATH[] = "test_data/noun_gender_dict/noun_gender_dict_es.txt";

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

    char *noun_gender_dict_path = pv_test_resource_path(NOUN_GENDER_DICT_PATH);
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

    status = pv_normalizer_tagger_init(
            language_info_object,
            PV_ARRAY_LEN(ALL_TEST_USE_CASES),
            (const int32_t *) ALL_TEST_USE_CASES,
            normalizer_tokenizer_object,
            noun_gender_dict_object,
            &normalizer_tagger_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_normalizer_tagger_teardown(void) {
    pv_noun_gender_dict_delete(noun_gender_dict_object);
    pv_normalizer_tagger_delete(normalizer_tagger_object);
    pv_normalizer_tokenizer_delete(normalizer_tokenizer_object);
    pv_language_info_delete(language_info_object);
}

static void test_pv_normalizer_tagger_init_helper(
        int32_t num_use_cases,
        const pv_normalizer_use_cases_es_t *use_cases,
        pv_normalizer_tagger_t **tagger) {
    *tagger = NULL;

    pv_status_t status = pv_normalizer_tagger_init(
            language_info_object,
            num_use_cases,
            (const int32_t *) use_cases,
            normalizer_tokenizer_object,
            noun_gender_dict_object,
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
            pv_normalizer_tokenizer_default_word_boundary_character(normalizer_tokenizer_object),
            preserve_word_boundary,
            false,
            false,
            true,
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
        const pv_normalizer_token_tag_es_t *target_tags) {
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
                "incorrect tag for token string `%s` at position `%d`: got tag index `%d`, expected tag index `%d`",
                current->string,
                i,
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
        const pv_normalizer_token_tag_es_t *target_tags,
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
            pv_normalizer_tokenizer_default_word_boundary_character(normalizer_tokenizer_object),
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
        const pv_normalizer_token_tag_es_t *target_tags,
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
                "incorrect length_future_context: got `%d`, expected `%d`",
                current->length_future_context,
                target_length_future_context_list[i]);
        pv_test_true(
                current->length_past_context == target_length_past_context_list[i],
                "incorrect length_past_context: got `%d`, expected `%d`",
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
    static pv_normalizer_use_cases_es_t use_cases[] = {PV_NORMALIZER_USE_WORD_NORMALIZER_ES};

    const pv_normalizer_token_tag_es_t sentence_tags[] = {
            PV_NORMALIZER_TAG_ES_PUNCTUATION,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_NONE,
            PV_NORMALIZER_TAG_ES_PUNCTUATION,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_PUNCTUATION,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_NONE,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_PUNCTUATION,
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
    static pv_normalizer_use_cases_es_t use_cases[] = {PV_NORMALIZER_USE_WORD_NORMALIZER_ES};

    const pv_normalizer_token_tag_es_t sentence_tags[] = {
            PV_NORMALIZER_TAG_ES_PUNCTUATION,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_NONE,
            PV_NORMALIZER_TAG_ES_PUNCTUATION,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_PUNCTUATION,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_NONE,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_PUNCTUATION,
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
    static pv_normalizer_use_cases_es_t use_cases[] = {PV_NORMALIZER_USE_PUNCTUATION_NORMALIZER_ES};

    const pv_normalizer_token_tag_es_t sentence_tags[] = {
            PV_NORMALIZER_TAG_ES_PUNCTUATION,
            PV_NORMALIZER_TAG_ES_NONE,
            PV_NORMALIZER_TAG_ES_NONE,
            PV_NORMALIZER_TAG_ES_NONE,
            PV_NORMALIZER_TAG_ES_NONE,
            PV_NORMALIZER_TAG_ES_NONE,
            PV_NORMALIZER_TAG_ES_PUNCTUATION,
            PV_NORMALIZER_TAG_ES_NONE,
            PV_NORMALIZER_TAG_ES_PUNCTUATION,
            PV_NORMALIZER_TAG_ES_NONE,
            PV_NORMALIZER_TAG_ES_NONE,
            PV_NORMALIZER_TAG_ES_NONE,
            PV_NORMALIZER_TAG_ES_PUNCTUATION,
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
    static pv_normalizer_use_cases_es_t use_cases[] = {PV_NORMALIZER_USE_CARDINAL_NORMALIZER_ES};

    const pv_normalizer_token_tag_es_t sentence_tags[] = {
            PV_NORMALIZER_TAG_ES_PUNCTUATION,
            PV_NORMALIZER_TAG_ES_NONE,
            PV_NORMALIZER_TAG_ES_NONE,
            PV_NORMALIZER_TAG_ES_NONE,
            PV_NORMALIZER_TAG_ES_NONE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_PUNCTUATION,
            PV_NORMALIZER_TAG_ES_NONE,
            PV_NORMALIZER_TAG_ES_PUNCTUATION,
            PV_NORMALIZER_TAG_ES_NONE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_NONE,
            PV_NORMALIZER_TAG_ES_PUNCTUATION};

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
    static pv_normalizer_use_cases_es_t use_cases[] = {PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_ES};

    const char sentence[] = "123 -563 −123";
    const pv_normalizer_token_tag_es_t sentence_tags[] = {
            PV_NORMALIZER_TAG_ES_NONE,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_NEGATIVE_CARDINAL,
    };

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
    static pv_normalizer_use_cases_es_t use_cases[] = {PV_NORMALIZER_USE_CUSTOM_PRONUNCIATION_NORMALIZER_ES};

    const char sentence[] = "La pronunciación {personalizada|p e ɾ s o n a l i θ a ð a}";
    const pv_normalizer_token_tag_es_t sentence_tags[] = {
            PV_NORMALIZER_TAG_ES_NONE,
            PV_NORMALIZER_TAG_ES_NONE,
            PV_NORMALIZER_TAG_ES_CUSTOM_PRONUNCIATION};

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

static void test_pv_normalizer_tagger_tag_custom_pronunciation_all_phonemes(void) {
    static pv_normalizer_use_cases_es_t use_cases[] = {PV_NORMALIZER_USE_CUSTOM_PRONUNCIATION_NORMALIZER_ES};

    const char sentence[] = "{hola|a b d e f g i j k l m n o p r s t u w x z ð ŋ ɣ ɱ ɲ ɾ ʎ ʝ ʧ β θ}";
    const pv_normalizer_token_tag_es_t sentence_tags[] = {
            PV_NORMALIZER_TAG_ES_CUSTOM_PRONUNCIATION};

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
    static pv_normalizer_use_cases_es_t invalid_use_cases[] = {99};

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
    const char sentence[] = "12-5 -4-2 hola 43- -9 5-a a-5 6-33 50-50-50 1--3 123–12";
    const pv_normalizer_token_tag_es_t sentence_tags[] = {
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_NUMBER_RANGE_TO,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_NUMBER_RANGE_TO,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_DIGITS,
            PV_NORMALIZER_TAG_ES_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_ES_DIGITS,
            PV_NORMALIZER_TAG_ES_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_ES_DIGITS,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_NUMBER_RANGE_TO,
            PV_NORMALIZER_TAG_ES_CARDINAL,
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
    static pv_normalizer_use_cases_es_t use_cases[] = {PV_NORMALIZER_USE_WORD_NORMALIZER_ES};

    const char sentence[] = "4-minutos hola -563 donde-estas 2-2 hola- some–where";
    const char *sentence_strings[] = {
            "4",
            "minutos",
            "hola",
            "563",
            "donde",
            "estas",
            "2",
            "2",
            "hola",
            "some",
            "where",
    };
    const pv_normalizer_token_tag_es_t sentence_tags[] = {
            PV_NORMALIZER_TAG_ES_NONE,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_NONE,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_NONE,
            PV_NORMALIZER_TAG_ES_NONE,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_WORD,
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
    static pv_normalizer_use_cases_es_t use_cases[] = {PV_NORMALIZER_USE_WORD_NORMALIZER_ES};

    const char sentence[] = "- --- − – ‒";

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(use_cases), use_cases, &tagger);

    test_pv_normalizer_tagger_tokenize_tag_and_check_status_helper(
            normalizer_tokenizer_object,
            tagger,
            sentence,
            PV_STATUS_SUCCESS);

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_tag_ordinal(void) {
    static pv_normalizer_use_cases_es_t use_cases[] = {
            PV_NORMALIZER_USE_ORDINAL_NORMALIZER_ES,
            PV_NORMALIZER_USE_CARDINAL_NORMALIZER_ES,
            PV_NORMALIZER_USE_ALPHANUM_SPELL_OUT_NORMALIZER_ES,
            PV_NORMALIZER_USE_WORD_NORMALIZER_ES,
    };

    const char *sentence =
            "hola 1ro 2da 3ro 6to 7mo 8vo 9no 10ma 11mo 12mo 13ro 21ro 52do 143ro 2000000000000mo 10000000000000000000mo";
    const pv_normalizer_token_tag_es_t sentence_tags[] = {
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_ORDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_ORDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_ORDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_ORDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_ORDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_ORDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_ORDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_ORDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_ORDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_ORDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_ORDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_ORDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_ORDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_ORDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_ORDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_LETTER_SPELL_OUT,
    };

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

static void test_pv_normalizer_tagger_tag_invalid_ordinal(void) {
    static pv_normalizer_use_cases_es_t use_cases[] = {PV_NORMALIZER_USE_ORDINAL_NORMALIZER_ES};

    const char *sentences[] = {"1do", "1mo", "1vo", "4do", "11ro", "12do", "13do", "35no"};

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(use_cases), use_cases, &tagger);

    for (int32_t i = 0; i < PV_ARRAY_LEN(sentences); i++) {
        const char *sentence = sentences[i];

        pv_normalizer_token_list_t *token_list = NULL;

        pv_status_t status = pv_normalizer_tokenizer_tokenize(
                normalizer_tokenizer_object,
                sentence,
                pv_normalizer_tokenizer_default_word_boundary_character(normalizer_tokenizer_object),
                false,
                false,
                false,
                false,
                &token_list);
        pv_test_true(status == PV_STATUS_SUCCESS, "failed to tokenize sentence: `%s`", sentence);

        status = pv_normalizer_tagger_tag(tagger, token_list, 0, true);
        pv_test_true(status == PV_STATUS_SUCCESS, "failed to tag sentence: `%s`", sentence);
        pv_test_true(
                token_list->head->tag != PV_NORMALIZER_TAG_ES_ORDINAL,
                "`%s` was incorrectly tagged as PV_NORMALIZER_TAG_ORDINAL",
                sentence);

        pv_normalizer_token_list_delete(token_list);
    }

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_tag_negative_ordinal(void) {
    static pv_normalizer_use_cases_es_t use_cases[] = {
            PV_NORMALIZER_USE_NEGATIVE_ORDINAL_NORMALIZER_ES,
            PV_NORMALIZER_USE_ALPHANUM_SPELL_OUT_NORMALIZER_ES,
    };

    const char sentence[] =
            "hola -1ro -2da -3ro -6to -10mo -11mo -12mo -13ro -21ro -52do -143ra -2000000000000mo "
            "-10000000000000000000mo −1ro";
    const pv_normalizer_token_tag_es_t sentence_tags[] = {
            PV_NORMALIZER_TAG_ES_NONE,
            PV_NORMALIZER_TAG_ES_NEGATIVE_ORDINAL,
            PV_NORMALIZER_TAG_ES_NEGATIVE_ORDINAL,
            PV_NORMALIZER_TAG_ES_NEGATIVE_ORDINAL,
            PV_NORMALIZER_TAG_ES_NEGATIVE_ORDINAL,
            PV_NORMALIZER_TAG_ES_NEGATIVE_ORDINAL,
            PV_NORMALIZER_TAG_ES_NEGATIVE_ORDINAL,
            PV_NORMALIZER_TAG_ES_NEGATIVE_ORDINAL,
            PV_NORMALIZER_TAG_ES_NEGATIVE_ORDINAL,
            PV_NORMALIZER_TAG_ES_NEGATIVE_ORDINAL,
            PV_NORMALIZER_TAG_ES_NEGATIVE_ORDINAL,
            PV_NORMALIZER_TAG_ES_NEGATIVE_ORDINAL,
            PV_NORMALIZER_TAG_ES_NEGATIVE_ORDINAL,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_NEGATIVE_ORDINAL,
    };

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

static void test_pv_normalizer_tagger_tag_invalid_negative_ordinal(void) {
    static pv_normalizer_use_cases_es_t use_cases[] = {PV_NORMALIZER_USE_ORDINAL_NORMALIZER_ES};

    const char *sentences[] = {"-1ro", "-1do", "-1ro", "-4to", "-11ro", "-12mo", "-13ro", "-35ro"};

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(use_cases), use_cases, &tagger);

    for (int32_t i = 0; i < PV_ARRAY_LEN(sentences); i++) {
        const char *sentence = sentences[i];

        pv_normalizer_token_list_t *token_list = NULL;

        pv_status_t status = pv_normalizer_tokenizer_tokenize(
                normalizer_tokenizer_object,
                sentence,
                pv_normalizer_tokenizer_default_word_boundary_character(normalizer_tokenizer_object),
                false,
                false,
                false,
                false,
                &token_list);
        pv_test_true(status == PV_STATUS_SUCCESS, "failed to tokenize sentence: `%s`", sentence);

        status = pv_normalizer_tagger_tag(tagger, token_list, 0, true);
        pv_test_true(status == PV_STATUS_SUCCESS, "failed to tag sentence: `%s`", sentence);
        pv_test_true(
                token_list->head->tag != PV_NORMALIZER_TAG_ES_NEGATIVE_ORDINAL,
                "%s was incorrectly tagged as PV_NORMALIZER_TAG_NEGATIVE_ORDINAL",
                pv_status_to_string(status),
                sentence);

        pv_normalizer_token_list_delete(token_list);
    }

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_correct_num_tag_weights(void) {
    pv_test_true(
            PV_ARRAY_LEN(PV_NORMALIZER_TOKEN_TAG_ES_WEIGHTS) == PV_NORMALIZER_TAG_ES_NUM_TAGS,
            "Number of tags and tag weights do not match. Have %d tags and %d weights",
            PV_ARRAY_LEN(PV_NORMALIZER_TOKEN_TAG_ES_WEIGHTS),
            PV_NORMALIZER_TAG_ES_NUM_TAGS);
}

static void test_pv_normalizer_tagger_tag_special_characters(void) {
    static pv_normalizer_use_cases_es_t use_cases[] = {PV_NORMALIZER_USE_SPECIAL_CHARACTER_NORMALIZER_ES};

    const char sentence[] = "hola 5% @@ °C RPM 90 CM²";
    const char *sentence_strings[] = {"hola", "5", "%", "@", "@", "°C", "RPM", "90", "CM²"};
    const pv_normalizer_token_tag_es_t sentence_tags[] = {
            PV_NORMALIZER_TAG_ES_NONE,
            PV_NORMALIZER_TAG_ES_NONE,
            PV_NORMALIZER_TAG_ES_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_ES_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_ES_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_ES_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_ES_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_ES_NONE,
            PV_NORMALIZER_TAG_ES_SPECIAL_CHARACTER,
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
    const char sentence[] = "-5% −5%";
    const char *sentence_strings[] = {
            "-5",
            "%",
            "-5",
            "%",
    };
    const pv_normalizer_token_tag_es_t sentence_tags[] = {
            PV_NORMALIZER_TAG_ES_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_ES_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_ES_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_ES_SPECIAL_CHARACTER,
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
            current->tag == PV_NORMALIZER_TAG_ES_CUSTOM_PRONUNCIATION,
            "incorrect tag for `%s`: got `%d`, expected `%d`",
            current->string,
            current->tag,
            PV_NORMALIZER_TAG_ES_CUSTOM_PRONUNCIATION);

    pv_normalizer_token_list_delete(token_list);
}

static void test_pv_normalizer_tagger_tag_weights_order(void) {
    pv_normalizer_use_cases_es_t use_cases[] = {PV_NORMALIZER_USE_WORD_NORMALIZER_ES};

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(use_cases), use_cases, &tagger);

    pv_normalizer_token_list_t *token_list = NULL;
    pv_status_t status = pv_normalizer_token_list_init(&token_list);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to initialize token list");

    pv_normalizer_token_t *empty_token = NULL;
    status = pv_normalizer_token_init(0, 4, "{|AY}", false, true, false, &empty_token);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to initialize token");
    empty_token->tag = PV_NORMALIZER_TAG_ES_WORD;

    pv_normalizer_token_list_append_token(token_list, empty_token);

    status = pv_normalizer_tagger_tag(tagger, token_list, 0, true);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to tag sentence: `%s`", TAGGER_TEST_SENTENCE);

    pv_normalizer_token_t *current = token_list->head;
    pv_test_true(current != NULL, "token is empty");
    pv_test_true(
            current->tag == PV_NORMALIZER_TAG_ES_WORD,
            "incorrect tag for `%s`: got `%d`, expected `%d`",
            current->string,
            current->tag,
            PV_NORMALIZER_TAG_ES_WORD);

    pv_normalizer_tagger_delete(tagger);

    pv_normalizer_use_cases_es_t use_cases2[] = {
            PV_NORMALIZER_USE_WORD_NORMALIZER_ES,
            PV_NORMALIZER_USE_CUSTOM_PRONUNCIATION_NORMALIZER_ES,
    };

    tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(use_cases2), use_cases2, &tagger);

    status = pv_normalizer_tagger_tag(normalizer_tagger_object, token_list, 0, true);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to tag sentence: `%s`", TAGGER_TEST_SENTENCE);

    current = token_list->head;
    pv_test_true(current != NULL, "token is empty");
    pv_test_true(
            current->tag == PV_NORMALIZER_TAG_ES_CUSTOM_PRONUNCIATION,
            "incorrect tag for `%s`: got `%d`, expected `%d`",
            current->string,
            current->tag,
            PV_NORMALIZER_TAG_ES_CUSTOM_PRONUNCIATION);

    pv_normalizer_token_list_delete(token_list);
    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_next_char_is_space(void) {
    static pv_normalizer_use_cases_es_t use_cases[] = {PV_NORMALIZER_USE_WORD_NORMALIZER_ES};

    const char sentence[] = "4-minutos, hola -563 donde-estas 2-2 hola-";
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
    const char sentence[] = ",1 1,2 ,456 1,,2 a,6 ,5% -1,1";
    const pv_normalizer_token_tag_es_t sentence_tags[] = {
            PV_NORMALIZER_TAG_ES_DECIMAL_POINT,
            PV_NORMALIZER_TAG_ES_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_DECIMAL_POINT,
            PV_NORMALIZER_TAG_ES_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_DECIMAL_POINT,
            PV_NORMALIZER_TAG_ES_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_PUNCTUATION,
            PV_NORMALIZER_TAG_ES_PUNCTUATION,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_PUNCTUATION,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_DECIMAL_POINT,
            PV_NORMALIZER_TAG_ES_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_ES_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_ES_DECIMAL_POINT,
            PV_NORMALIZER_TAG_ES_DECIMAL_DIGITS,
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
    const pv_normalizer_token_tag_es_t sentence_tags2[] = {
            PV_NORMALIZER_TAG_ES_PUNCTUATION,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL};

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
    const char sentence[] = "HTML5 A87B 12A 12 ÓL1";
    const pv_normalizer_token_tag_es_t sentence_tags[] = {
            PV_NORMALIZER_TAG_ES_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
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
            "7:00 0h31 12h59' 9h11m 7:00 p.m. am 09:18 pm 18:23 21:89 25:09 PM p.m. am 8 a.m.. 04:21am 12:00p.m. 2AM 25PM 2a.m. 12p.m.";
    const pv_normalizer_token_tag_es_t sentence_tags[] = {
            PV_NORMALIZER_TAG_ES_TIME_HOURS,
            PV_NORMALIZER_TAG_ES_TIME_COLON,
            PV_NORMALIZER_TAG_ES_TIME_MINUTES,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_TIME_HOURS,
            PV_NORMALIZER_TAG_ES_TIME_H,
            PV_NORMALIZER_TAG_ES_TIME_MINUTES,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_TIME_HOURS,
            PV_NORMALIZER_TAG_ES_TIME_H,
            PV_NORMALIZER_TAG_ES_TIME_MINUTES,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_TIME_HOURS,
            PV_NORMALIZER_TAG_ES_TIME_H,
            PV_NORMALIZER_TAG_ES_TIME_MINUTES,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_TIME_HOURS,
            PV_NORMALIZER_TAG_ES_TIME_COLON,
            PV_NORMALIZER_TAG_ES_TIME_MINUTES,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_TIME_AM_PM,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_TIME_HOURS,
            PV_NORMALIZER_TAG_ES_TIME_COLON,
            PV_NORMALIZER_TAG_ES_TIME_MINUTES,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_TIME_AM_PM,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_TIME_HOURS,
            PV_NORMALIZER_TAG_ES_TIME_COLON,
            PV_NORMALIZER_TAG_ES_TIME_MINUTES,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_COLON,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_COLON,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_DOT,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_PUNCTUATION,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_TIME_AM_PM,
            PV_NORMALIZER_TAG_ES_PUNCTUATION,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_TIME_HOURS,
            PV_NORMALIZER_TAG_ES_TIME_COLON,
            PV_NORMALIZER_TAG_ES_TIME_MINUTES,
            PV_NORMALIZER_TAG_ES_TIME_AM_PM,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_TIME_HOURS,
            PV_NORMALIZER_TAG_ES_TIME_COLON,
            PV_NORMALIZER_TAG_ES_TIME_MINUTES,
            PV_NORMALIZER_TAG_ES_TIME_AM_PM,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_TIME_HOURS,
            PV_NORMALIZER_TAG_ES_TIME_AM_PM,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_TIME_HOURS,
            PV_NORMALIZER_TAG_ES_TIME_AM_PM,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_TIME_HOURS,
            PV_NORMALIZER_TAG_ES_TIME_AM_PM,
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
    const char sentence[] = "5 g 1ml -7,3l hola ml 10,1km 5°C 1.500km -1.000.111g 25ft.-lb. contexto claro ml-g";
    const pv_normalizer_token_tag_es_t sentence_tags[] = {
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_MEASUREMENT,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_MEASUREMENT,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_ES_DECIMAL_POINT,
            PV_NORMALIZER_TAG_ES_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_ES_MEASUREMENT,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_MEASUREMENT,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_DECIMAL_POINT,
            PV_NORMALIZER_TAG_ES_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_ES_MEASUREMENT,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_MEASUREMENT,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_MEASUREMENT,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_ES_MEASUREMENT,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_MEASUREMENT,
            PV_NORMALIZER_TAG_ES_DOT,
            PV_NORMALIZER_TAG_ES_MEASUREMENT,
            PV_NORMALIZER_TAG_ES_PUNCTUATION,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_MEASUREMENT,
            PV_NORMALIZER_TAG_ES_MEASUREMENT,
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
    const char sentence[] = "5 km/m 10 oz/km 1,1m/l -5,41kg/ft m/s 7 millas/hora 100Km/hora 1.000.111m/s";
    const pv_normalizer_token_tag_es_t sentence_tags[] = {
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_MEASUREMENT,
            PV_NORMALIZER_TAG_ES_POR_SLASH,
            PV_NORMALIZER_TAG_ES_MEASUREMENT,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_MEASUREMENT,
            PV_NORMALIZER_TAG_ES_POR_SLASH,
            PV_NORMALIZER_TAG_ES_MEASUREMENT,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_DECIMAL_POINT,
            PV_NORMALIZER_TAG_ES_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_ES_MEASUREMENT,
            PV_NORMALIZER_TAG_ES_POR_SLASH,
            PV_NORMALIZER_TAG_ES_MEASUREMENT,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_ES_DECIMAL_POINT,
            PV_NORMALIZER_TAG_ES_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_ES_MEASUREMENT,
            PV_NORMALIZER_TAG_ES_POR_SLASH,
            PV_NORMALIZER_TAG_ES_MEASUREMENT,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_MEASUREMENT,
            PV_NORMALIZER_TAG_ES_POR_SLASH,
            PV_NORMALIZER_TAG_ES_MEASUREMENT,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_POR_SLASH,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_MEASUREMENT,
            PV_NORMALIZER_TAG_ES_POR_SLASH,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_MEASUREMENT,
            PV_NORMALIZER_TAG_ES_POR_SLASH,
            PV_NORMALIZER_TAG_ES_MEASUREMENT,
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

static void test_pv_normalizer_tagger_tag_dot_cardinal(void) {
    const char *sentence = "1.005 11.005 111.005 -5.123.123.123 1111.000 1.1.000 1. 000";

    const pv_normalizer_token_tag_es_t sentence_tags[] = {
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_DOT,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_DOT,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_DOT,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_PUNCTUATION,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
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

static void test_pv_normalizer_tagger_tag_dot_ordinal(void) {
    const char *sentence = "1.005to 11.005ta 111.005to -5.123.123.123ro 1111.000mo 1.1.000mo 1. 000mo";

    const pv_normalizer_token_tag_es_t sentence_tags[] = {
            PV_NORMALIZER_TAG_ES_ORDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_ORDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_ORDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_NEGATIVE_ORDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_DOT,
            PV_NORMALIZER_TAG_ES_ORDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_DOT,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_DOT,
            PV_NORMALIZER_TAG_ES_ORDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_PUNCTUATION,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_ORDINAL,
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

static void test_pv_normalizer_tagger_tag_dot_number_range(void) {
    const char *sentence = "1.000-2.000 1.000,2-1.000,5 1.000–2.000";

    const pv_normalizer_token_tag_es_t sentence_tags[] = {
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_NUMBER_RANGE_TO,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_DECIMAL_POINT,
            PV_NORMALIZER_TAG_ES_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_ES_NUMBER_RANGE_TO,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_DECIMAL_POINT,
            PV_NORMALIZER_TAG_ES_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_NUMBER_RANGE_TO,
            PV_NORMALIZER_TAG_ES_CARDINAL,
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

static void test_pv_normalizer_tagger_tag_dot_decimal(void) {
    const char *sentence = "1.000,2 -300.000,6666";

    const pv_normalizer_token_tag_es_t sentence_tags[] = {
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_DECIMAL_POINT,
            PV_NORMALIZER_TAG_ES_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_ES_DECIMAL_POINT,
            PV_NORMALIZER_TAG_ES_DECIMAL_DIGITS,
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

static void test_pv_normalizer_tagger_tag_fraction(void) {
    const char *sentence = "1/2 1/3,2 1/1.000 1/-4 -4/1";

    const pv_normalizer_token_tag_es_t sentence_tags[] = {
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_FRACTION_SLASH,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_FRACTION_SLASH,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_DECIMAL_POINT,
            PV_NORMALIZER_TAG_ES_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_FRACTION_SLASH,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_FRACTION_SLASH,
            PV_NORMALIZER_TAG_ES_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_ES_FRACTION_SLASH,
            PV_NORMALIZER_TAG_ES_CARDINAL,
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

static void test_pv_normalizer_tagger_tag_fraction_feminine(void) {
    const char *sentence = "1/3 parte de la torta";

    const pv_normalizer_token_tag_es_t sentence_tags[] = {
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_FRACTION_SLASH,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_WORD,
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
    const char *sentence = "www.example.com https://hola.xyz/";

    const pv_normalizer_token_tag_es_t sentence_tags[] = {
            PV_NORMALIZER_TAG_ES_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_DOT,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_DOT,
            PV_NORMALIZER_TAG_ES_TOP_LEVEL_DOMAIN,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_COLON,
            PV_NORMALIZER_TAG_ES_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_ES_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_DOT,
            PV_NORMALIZER_TAG_ES_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_SPECIAL_CHARACTER,
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
            "$5 €10 10$ 10€ $1,25 1,25$ -$500 -500$ -1,25$ -$1,25 €1.000 -€1.000 1.000€ -1.000€ €1.000,25 1.000,25€ "
            "1,2$ $1,222 1USD USD1,25 1,25USD -1USD -1,25USD 1 $ -1 $ 1,25 $ -1,25 $";

    const pv_normalizer_token_tag_es_t sentence_tags[] = {
            PV_NORMALIZER_TAG_ES_CURRENCY,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CURRENCY,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CURRENCY,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CURRENCY,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CURRENCY,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CURRENCY,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CURRENCY,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CURRENCY,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CURRENCY,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CURRENCY,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_PUNCTUATION,
            PV_NORMALIZER_TAG_ES_NONE,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CURRENCY,
            PV_NORMALIZER_TAG_ES_PUNCTUATION,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CURRENCY,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CURRENCY,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CURRENCY,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CURRENCY_SYMBOL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CURRENCY_SYMBOL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_DECIMAL_POINT,
            PV_NORMALIZER_TAG_ES_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CURRENCY_SYMBOL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_ES_DECIMAL_POINT,
            PV_NORMALIZER_TAG_ES_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CURRENCY_SYMBOL,
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

static void test_pv_normalizer_tagger_tag_abbreviations(void) {
    const char *sentence = "p.ej. este etc. Sr. Av. C/ Dr. Sra. Prof. Gob.";
    const pv_normalizer_token_tag_es_t sentence_tags[] = {
            PV_NORMALIZER_TAG_ES_ABBREVIATION,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_ABBREVIATION,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_ABBREVIATION,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_ABBREVIATION,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_ABBREVIATION,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_ABBREVIATION,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_ABBREVIATION,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_ABBREVIATION,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_ABBREVIATION,
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
    const char *sentence = "(778) 239-1823 102-291091-4920 123‒456‒7890";
    const pv_normalizer_token_tag_es_t sentence_tags[] = {
            PV_NORMALIZER_TAG_ES_DIGITS_WITH_PARENTHESES,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_DIGITS,
            PV_NORMALIZER_TAG_ES_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_ES_DIGITS,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_DIGITS,
            PV_NORMALIZER_TAG_ES_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_ES_DIGITS,
            PV_NORMALIZER_TAG_ES_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_ES_DIGITS,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_DIGITS,
            PV_NORMALIZER_TAG_ES_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_ES_DIGITS,
            PV_NORMALIZER_TAG_ES_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_ES_DIGITS,
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
    const char *sentence = "06-03-2020 31-02-1991 06/03/2020 6/3/2020 1991/03/13 1991/13/13 07/13/182 2024-05-06";
    const pv_normalizer_token_tag_es_t sentence_tags[] = {
            PV_NORMALIZER_TAG_ES_DATE_DAY,
            PV_NORMALIZER_TAG_ES_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_ES_DATE_MONTH,
            PV_NORMALIZER_TAG_ES_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_ES_DATE_YEAR,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_DATE_DAY,
            PV_NORMALIZER_TAG_ES_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_ES_DATE_MONTH,
            PV_NORMALIZER_TAG_ES_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_ES_DATE_YEAR,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_DATE_DAY,
            PV_NORMALIZER_TAG_ES_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_ES_DATE_MONTH,
            PV_NORMALIZER_TAG_ES_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_ES_DATE_YEAR,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_DATE_DAY,
            PV_NORMALIZER_TAG_ES_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_ES_DATE_MONTH,
            PV_NORMALIZER_TAG_ES_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_ES_DATE_YEAR,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_DATE_YEAR,
            PV_NORMALIZER_TAG_ES_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_ES_DATE_MONTH,
            PV_NORMALIZER_TAG_ES_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_ES_DATE_DAY,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_DIGITS,
            PV_NORMALIZER_TAG_ES_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_ES_DIGITS,
            PV_NORMALIZER_TAG_ES_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_ES_DIGITS,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_DIGITS,
            PV_NORMALIZER_TAG_ES_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_ES_DIGITS,
            PV_NORMALIZER_TAG_ES_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_ES_DIGITS,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_DATE_YEAR,
            PV_NORMALIZER_TAG_ES_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_ES_DATE_MONTH,
            PV_NORMALIZER_TAG_ES_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_ES_DATE_DAY,
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

    const char *sentence_two = "3 Junio 2020 8 Sep 1991 13-Jun-2024 32-10-2024 1 Ago";
    const pv_normalizer_token_tag_es_t sentence_tags_two[] = {
            PV_NORMALIZER_TAG_ES_DATE_DAY,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_DATE_MONTH,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_DATE_YEAR,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_DATE_DAY,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_DATE_MONTH,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_DATE_YEAR,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_DATE_DAY,
            PV_NORMALIZER_TAG_ES_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_ES_DATE_MONTH,
            PV_NORMALIZER_TAG_ES_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_ES_DATE_YEAR,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_DIGITS,
            PV_NORMALIZER_TAG_ES_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_ES_DIGITS,
            PV_NORMALIZER_TAG_ES_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_ES_DIGITS,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_DATE_DAY,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_DATE_MONTH,
    };

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            tagger,
            sentence_two,
            PV_ARRAY_LEN(sentence_tags_two),
            true,
            sentence_tags_two);

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_tag_name(void) {
    const char *sentence = "Este es J. R. R. Tolkien John F. Kennedy 8:00";
    const pv_normalizer_token_tag_es_t sentence_tags[] = {
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_NAME_INITIAL_DOT,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_NAME_INITIAL_DOT,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_NAME_INITIAL_DOT,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_NAME_INITIAL_DOT,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_TIME_HOURS,
            PV_NORMALIZER_TAG_ES_TIME_COLON,
            PV_NORMALIZER_TAG_ES_TIME_MINUTES,
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

static void test_pv_normalizer_tagger_tag_single_quote(void) {
    const char *sentence = "Diga, 'asegúrate de llegar a tiempo.' 'Hola&' ‘1.000$’";
    const pv_normalizer_token_tag_es_t sentence_tags[] = {
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_PUNCTUATION,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_PUNCTUATION,
            PV_NORMALIZER_TAG_ES_SINGLE_QUOTE,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_ES_SINGLE_QUOTE,
            PV_NORMALIZER_TAG_ES_SINGLE_QUOTE,
            PV_NORMALIZER_TAG_ES_CURRENCY,
            PV_NORMALIZER_TAG_ES_SINGLE_QUOTE,
    };

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(ALL_TEST_USE_CASES), ALL_TEST_USE_CASES, &tagger);

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            tagger,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            false,
            sentence_tags);

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_length_future_past_context_after_tag(void) {
    const char *sentence = "entre 3-5 dias. #AB12 www.ejemplo.com 9:30 1/2 1/1.000 1.123ro 2ml 2m/h -1.900.876 $10.000,00 Dr.A.B Dr. p.ej.";
    const pv_normalizer_token_tag_es_t sentence_tags[] = {
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_NUMBER_RANGE_TO,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_PUNCTUATION,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_ES_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_DOT,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_DOT,
            PV_NORMALIZER_TAG_ES_TOP_LEVEL_DOMAIN,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_TIME_HOURS,
            PV_NORMALIZER_TAG_ES_TIME_COLON,
            PV_NORMALIZER_TAG_ES_TIME_MINUTES,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_FRACTION_SLASH,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_FRACTION_SLASH,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_ORDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_MEASUREMENT,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_MEASUREMENT,
            PV_NORMALIZER_TAG_ES_POR_SLASH,
            PV_NORMALIZER_TAG_ES_MEASUREMENT,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CURRENCY,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_ABBREVIATION,
            PV_NORMALIZER_TAG_ES_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_ES_NAME_INITIAL_DOT,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_ABBREVIATION,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_ABBREVIATION,
    };
    const char *sentence_original_strings[] = {
            "entre",
            " ",
            "3-5",
            "3-5",
            "3-5",
            " ",
            "dias",
            ".",
            " ",
            "#AB12",
            "#AB12",
            "#AB12",
            "#AB12",
            "#AB12",
            " ",
            "www.ejemplo.com",
            "www.ejemplo.com",
            "www.ejemplo.com",
            "www.ejemplo.com",
            "www.ejemplo.com",
            "www.ejemplo.com",
            "www.ejemplo.com",
            " ",
            "9:30",
            "9:30",
            "9:30",
            " ",
            "1/2",
            "1/2",
            "1/2",
            " ",
            "1/1.000",
            "1/1.000",
            "1/1.000",
            " ",
            "1.123ro",
            " ",
            "2ml",
            "2ml",
            " ",
            "2m/h",
            "2m/h",
            "2m/h",
            "2m/h",
            " ",
            "-1.900.876",
            " ",
            "$10.000,00",
            " ",
            "Dr.A.B",
            "Dr.A.B",
            "Dr.A.B",
            "Dr.A.B",
            " ",
            "Dr.",
            " ",
            "p.ej.",
    };
    const int32_t sentence_futures[] = {
            0,
            0,
            2,
            1,
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
            0,
            0,
            0,
            3,
            2,
            1,
            0,
            0,
            0,
            0,
            0,
    };
    const int32_t sentence_pasts[] = {
            0,
            0,
            0,
            1,
            2,
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
            0,
            0,
            0,
            0,
            1,
            2,
            3,
            0,
            0,
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
    const char *sentence = "12/25/2023 03-Jun-1829 (778) 239-1823 102-291-4920 123.456.789";
    const pv_normalizer_token_tag_es_t sentence_tags[] = {
            PV_NORMALIZER_TAG_ES_DATE_MONTH,
            PV_NORMALIZER_TAG_ES_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_ES_DATE_DAY,
            PV_NORMALIZER_TAG_ES_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_ES_DATE_YEAR,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_DATE_DAY,
            PV_NORMALIZER_TAG_ES_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_ES_DATE_MONTH,
            PV_NORMALIZER_TAG_ES_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_ES_DATE_YEAR,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_DIGITS_WITH_PARENTHESES,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_DIGITS,
            PV_NORMALIZER_TAG_ES_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_ES_DIGITS,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_DIGITS,
            PV_NORMALIZER_TAG_ES_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_ES_DIGITS,
            PV_NORMALIZER_TAG_ES_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_ES_DIGITS,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_DOT,
            PV_NORMALIZER_TAG_ES_CARDINAL,
            PV_NORMALIZER_TAG_ES_DOT,
            PV_NORMALIZER_TAG_ES_CARDINAL,
    };
    const char *sentence_original_strings[] = {
            "12/25/2023",
            "12/25/2023",
            "12/25/2023",
            "12/25/2023",
            "12/25/2023",
            " ",
            "03-Jun-1829",
            "03-Jun-1829",
            "03-Jun-1829",
            "03-Jun-1829",
            "03-Jun-1829",
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

static void test_pv_normalizer_tagger_length_future_past_context_single_quote(void) {
    const char *sentence = "consumir 'bienes y servicios' aumentando.";
    const pv_normalizer_token_tag_es_t sentence_tags[] = {
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_SPACE,
            PV_NORMALIZER_TAG_ES_WORD,
            PV_NORMALIZER_TAG_ES_PUNCTUATION,
    };
    const char *sentence_original_strings[] = {
            "consumir", " ",
            "'bienes", " ",
            "y", " ",
            "servicios'", " ",
            "aumentando", ".",
    };
    const int32_t sentence_futures[10] = {0};
    const int32_t sentence_pasts[10] = {0};

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
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_tagger_es_init, PV_STATUS_OUT_OF_MEMORY);

    static pv_normalizer_use_cases_es_t use_cases[] = {PV_NORMALIZER_USE_WORD_NORMALIZER_ES};
    pv_normalizer_tagger_t *tagger = NULL;

    pv_status_t status = pv_normalizer_tagger_init(
            language_info_object,
            PV_ARRAY_LEN(use_cases),
            (const int32_t *) use_cases,
            normalizer_tokenizer_object,
            noun_gender_dict_object,
            &tagger);
    pv_test_true(status == PV_STATUS_OUT_OF_MEMORY, "failed to fail with `PV_STATUS_OUT_OF_MEMORY`, got status `%s`", pv_status_to_string(status));
    pv_test_error_message(
            pv_test_function_hash_regex(),
            "`pv_normalizer_tagger_es_init` failed with status `OUT_OF_MEMORY`.",
            true,
            "Error message mismatch"
    );
}


static void test_pv_normalizer_tagger_tag_cardinal_helper_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_util_check_token_is_before_character, PV_STATUS_INVALID_ARGUMENT);

    const char sentence[] = "123";
    static pv_normalizer_use_cases_es_t use_cases[] = {PV_NORMALIZER_USE_CARDINAL_NORMALIZER_ES};
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
    pv_test_error_message(
            pv_test_function_hash_regex(),
            "`pv_normalizer_util_check_token_is_before_character` failed with status `INVALID_ARGUMENT`.",
            false,
            "Error message mismatch"
    );

    pv_normalizer_tagger_delete(tagger);
    pv_normalizer_token_list_delete(token_list);
}


static void test_pv_normalizer_tagger_tag_currency_helper_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_util_check_token_is_before_character, PV_STATUS_INVALID_ARGUMENT);

    const char sentence[] = "5$";
    static pv_normalizer_use_cases_es_t use_cases[] = {PV_NORMALIZER_USE_CURRENCY_NORMALIZER_ES};
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
    pv_test_error_message(
            pv_test_function_hash_regex(),
            "`pv_normalizer_util_check_token_is_before_character` failed with status `INVALID_ARGUMENT`.",
            false,
            "Error message mismatch"
    );

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
        {"tag_custom_pronunciation_all_phonemes", test_pv_normalizer_tagger_tag_custom_pronunciation_all_phonemes},
        {"tag_invalid_use_case", test_pv_normalizer_tagger_tag_invalid_use_case},
        {"tag_hyphenated_string", test_pv_normalizer_tagger_tag_hyphenated_string},
        {"tag_invalid_hyphens_only", test_pv_normalizer_tagger_tag_invalid_hyphens_only},
        {"tag_number_range", test_pv_normalizer_tagger_tag_number_range},
        {"tag_ordinal", test_pv_normalizer_tagger_tag_ordinal},
        {"tag_invalid_ordinal", test_pv_normalizer_tagger_tag_invalid_ordinal},
        {"tag_negative_ordinal", test_pv_normalizer_tagger_tag_negative_ordinal},
        {"tag_invalid_negative_ordinal", test_pv_normalizer_tagger_tag_invalid_negative_ordinal},
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
        {"tag_dot_cardinal", test_pv_normalizer_tagger_tag_dot_cardinal},
        {"tag_dot_ordinal", test_pv_normalizer_tagger_tag_dot_ordinal},
        {"tag_dot_number_range", test_pv_normalizer_tagger_tag_dot_number_range},
        {"tag_dot_decimal", test_pv_normalizer_tagger_tag_dot_decimal},
        {"tag_fraction", test_pv_normalizer_tagger_tag_fraction},
        {"tag_fraction_feminine", test_pv_normalizer_tagger_tag_fraction_feminine},
        {"tag_url", test_pv_normalizer_tagger_tag_url},
        {"tag_currency", test_pv_normalizer_tagger_tag_currency},
        {"tag_abbreviations", test_pv_normalizer_tagger_tag_abbreviations},
        {"tag_digits_sequence", test_pv_normalizer_tagger_tag_digits_sequence},
        {"tag_date", test_pv_normalizer_tagger_tag_date},
        {"tag_name", test_pv_normalizer_tagger_tag_name},
        {"length_future_past_context", test_pv_normalizer_tagger_length_future_past_context_after_tag},
        {"length_future_past_date_digits", test_pv_normalizer_tagger_length_future_past_context_after_tag_date_digits},
        {"tag_single_quote", test_pv_normalizer_tagger_tag_single_quote},
        {"length_future_past_single_quote", test_pv_normalizer_tagger_length_future_past_context_single_quote},

#ifdef __PV_MOCKS__

        {"tagger_init_failure", test_pv_normalizer_tagger_init_failure},
        {"tagger_tag_cardinal_helper_failure", test_pv_normalizer_tagger_tag_cardinal_helper_failure},
        {"tagger_tag_currency_helper_failure", test_pv_normalizer_tagger_tag_currency_helper_failure},

#endif

};

const pv_test_suite_t PV_NORMALIZER_TAGGER_ES_TEST_SUITE = {
        .name = "normalizer_tagger_es",
        .setup = test_pv_normalizer_tagger_setup,
        .teardown = test_pv_normalizer_tagger_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_NORMALIZER_TAGGER_TEST_CASES),
        .test_cases = PV_NORMALIZER_TAGGER_TEST_CASES,
};
