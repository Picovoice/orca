#include <stdlib.h>
#include <string.h>

#include "core/pv_language.h"
#include "core/pv_language_json.h"
#include "orca/normalizer/pv_normalizer_tagger.h"
#include "orca/normalizer/pv_normalizer_tokenizer.h"
#include "orca/normalizer/pv_normalizer_use_cases.h"
#include "test/pv_test.h"

#include "orca/normalizer/de/pv_normalizer_tags_de.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_normalizer_mock.h"

#endif

static pv_normalizer_tagger_t *normalizer_tagger_object = NULL;
static pv_normalizer_use_cases_de_t ALL_TEST_USE_CASES[] = {
        PV_NORMALIZER_USE_WORD_NORMALIZER_DE,
        PV_NORMALIZER_USE_CUSTOM_PRONUNCIATION_NORMALIZER_DE,
        PV_NORMALIZER_USE_PUNCTUATION_NORMALIZER_DE,
        PV_NORMALIZER_USE_CARDINAL_NORMALIZER_DE,
        PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_DE,
        PV_NORMALIZER_USE_NUMBER_RANGE_NORMALIZER_DE,
        PV_NORMALIZER_USE_ORDINAL_NORMALIZER_DE,
        PV_NORMALIZER_USE_SPECIAL_CHARACTER_NORMALIZER_DE,
        PV_NORMALIZER_USE_DECIMAL_NORMALIZER_DE,
        PV_NORMALIZER_USE_MEASUREMENT_NORMALIZER_DE,
        PV_NORMALIZER_USE_ALPHANUM_SPELL_OUT_NORMALIZER_DE,
        PV_NORMALIZER_USE_TIME_NORMALIZER_DE,
        PV_NORMALIZER_USE_FRACTION_NORMALIZER_DE,
        PV_NORMALIZER_USE_URL_NORMALIZER_DE,
        PV_NORMALIZER_USE_CURRENCY_NORMALIZER_DE,
        PV_NORMALIZER_USE_ABBREVIATION_NORMALIZER_DE,
        PV_NORMALIZER_USE_DIGITS_SEQUENCE_NORMALIZER_DE,
        PV_NORMALIZER_USE_DATE_NORMALIZER_DE,
        PV_NORMALIZER_USE_NAME_NORMALIZER_DE,
};

static pv_normalizer_tokenizer_t *normalizer_tokenizer_object = NULL;
static pv_language_info_t *language_info_object = NULL;
static pv_noun_gender_dict_t *noun_gender_dict_object = NULL;

static const char LANGUAGE_INFO_PATH[] = "normalizer/ref/pv_language_info_normalizer_de.json";
static const char NOUN_GENDER_DICT_PATH[] = "test_data/noun_gender_dict/noun_gender_dict_de.txt";

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
        const pv_normalizer_use_cases_de_t *use_cases,
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
        const pv_normalizer_token_tag_de_t *target_tags) {
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
        const pv_normalizer_token_tag_de_t *target_tags,
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
        const pv_normalizer_token_tag_de_t *target_tags,
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

static void test_pv_normalizer_tagger_tag_word(void) {
    const char sentence[] = "Ich träume von Urlaub";
    const pv_normalizer_token_tag_de_t sentence_tags[] = {
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
    };

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            true,
            sentence_tags);
}

static void test_pv_normalizer_tagger_tag_punctuation(void) {
    const char sentence[] = "\"Was, du gehst jetzt: wirklich?\"";
    const pv_normalizer_token_tag_de_t sentence_tags[] = {
            PV_NORMALIZER_TAG_DE_PUNCTUATION,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_PUNCTUATION,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_PUNCTUATION,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_PUNCTUATION,
            PV_NORMALIZER_TAG_DE_PUNCTUATION,
    };

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            true,
            sentence_tags);
}

static void test_pv_normalizer_tagger_tag_cardinal(void) {
    const char sentence[] = "1 2 3 10 12 56 123 5630 999999999999999";
    const pv_normalizer_token_tag_de_t sentence_tags[] = {
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
    };

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            true,
            sentence_tags);
}

static void test_pv_normalizer_tagger_tag_negative_cardinal(void) {
    const char sentence[] = "-1 -2 -3 -10 -12 -56 -123 -5630 -999999999999999 −123";
    const pv_normalizer_token_tag_de_t sentence_tags[] = {
            PV_NORMALIZER_TAG_DE_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_NEGATIVE_CARDINAL,
    };

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            true,
            sentence_tags);
}

static void test_pv_normalizer_tagger_tag_custom_pronunciation(void) {
    const char sentence[] = "Eine {ein|aɪ ŋ ɛ̃} Aussprache";
    const pv_normalizer_token_tag_de_t sentence_tags[] = {
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CUSTOM_PRONUNCIATION,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
    };

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            true,
            sentence_tags);
}

static void test_pv_normalizer_tagger_tag_invalid_use_case(void) {
    static pv_normalizer_use_cases_de_t invalid_use_cases[] = {99};
    const char sentence[] = "Hallo";

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(invalid_use_cases), invalid_use_cases, &tagger);

    test_pv_normalizer_tagger_tokenize_tag_and_check_status_helper(
            normalizer_tokenizer_object,
            tagger,
            sentence,
            PV_STATUS_INVALID_ARGUMENT);

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_tag_number_range(void) {
    const char sentence[] = "12-5 -4-2 Hallo 43- -9 5-a a-5 6-33 50-50-50 1--3 123–12";
    const pv_normalizer_token_tag_de_t sentence_tags[] = {
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_NUMBER_RANGE_TO,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_NUMBER_RANGE_TO,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_DIGITS,
            PV_NORMALIZER_TAG_DE_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_DE_DIGITS,
            PV_NORMALIZER_TAG_DE_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_DE_DIGITS,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_NUMBER_RANGE_TO,
            PV_NORMALIZER_TAG_DE_CARDINAL,
    };

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            true,
            sentence_tags);
}

static void test_pv_normalizer_tagger_tag_hyphenated_string(void) {
    const char sentence[] = "4-Minuten Hallo -563 irgend-wo 2-2 Hallo- some–where";
    const char *sentence_strings[] = {
            "4",
            "Minuten",
            "Hallo",
            "-563",
            "irgend",
            "wo",
            "2",
            "-",
            "2",
            "Hallo",
            "some",
            "where",
    };
    const pv_normalizer_token_tag_de_t sentence_tags[] = {
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_NUMBER_RANGE_TO,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_WORD,
    };

    test_pv_normalizer_tagger_tokenize_tag_and_check_strings_tags_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            sentence_tags,
            false,
            sentence_strings);
}

static void test_pv_normalizer_tagger_next_char_is_space(void) {
    static pv_normalizer_use_cases_de_t use_cases[] = {PV_NORMALIZER_USE_WORD_NORMALIZER_DE};

    const char sentence[] = "4-Minuten, Hallo -563 irgend-wo 2-2 Hallo-";
    const bool sentence_next_character_is_space[] = {
            false,
            false,
            true,
            true,
            true,
            false,
            true,
            false,
            true,
            false};
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
                current->next_character_is_space,
                sentence_next_character_is_space[i]);

        current = current->next;
    }
    pv_normalizer_token_list_delete(token_list);
    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_tag_invalid_hyphens_only(void) {
    const char sentence[] = "- --- − – ‒";

    test_pv_normalizer_tagger_tokenize_tag_and_check_status_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            sentence,
            PV_STATUS_SUCCESS);
}

static void test_pv_normalizer_tagger_tag_ordinal(void) {
    const char *sentence = "1. Sommer 2. Aakerbeere 13. Aalbaum 10. Frühling 29. Februar 331. Aalwanderung 12345. Abandonnement "
                           "666666. Aasvogel 2000000000000. Aalstrich 351. Aare 23. Aalenium 12. Der Mann";
    const pv_normalizer_token_tag_de_t sentence_tags[] = {
            PV_NORMALIZER_TAG_DE_ORDINAL_MASCULINE,
            PV_NORMALIZER_TAG_DE_ORDINAL_DOT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_ORDINAL_FEMININE,
            PV_NORMALIZER_TAG_DE_ORDINAL_DOT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_ORDINAL_MASCULINE,
            PV_NORMALIZER_TAG_DE_ORDINAL_DOT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_ORDINAL_NEUTER,
            PV_NORMALIZER_TAG_DE_ORDINAL_DOT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_ORDINAL_MASCULINE,
            PV_NORMALIZER_TAG_DE_ORDINAL_DOT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_DATE_MONTH,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_ORDINAL_FEMININE,
            PV_NORMALIZER_TAG_DE_ORDINAL_DOT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_ORDINAL_NEUTER,
            PV_NORMALIZER_TAG_DE_ORDINAL_DOT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_ORDINAL_MASCULINE,
            PV_NORMALIZER_TAG_DE_ORDINAL_DOT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_ORDINAL_MASCULINE,
            PV_NORMALIZER_TAG_DE_ORDINAL_DOT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_ORDINAL_FEMININE,
            PV_NORMALIZER_TAG_DE_ORDINAL_DOT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_ORDINAL_NEUTER,
            PV_NORMALIZER_TAG_DE_ORDINAL_DOT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_PUNCTUATION,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
    };

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            true,
            sentence_tags);
}

static void test_pv_normalizer_tagger_tag_negative_ordinal(void) {
    const char *sentence = "−1. Sommer -1. Sommer -2. Aakerbeere -13. Aalbaum -10. Frühling -29. Februar -331. Aalwanderung -12345. Abandonnement "
                           "-666666. Aasvogel -2000000000000. Aalstrich -351. Aare -23. Aalenium -12. Der Mann";
    const pv_normalizer_token_tag_de_t sentence_tags[] = {
            PV_NORMALIZER_TAG_DE_NEGATIVE_ORDINAL_MASCULINE,
            PV_NORMALIZER_TAG_DE_ORDINAL_DOT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_NEGATIVE_ORDINAL_MASCULINE,
            PV_NORMALIZER_TAG_DE_ORDINAL_DOT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_NEGATIVE_ORDINAL_FEMININE,
            PV_NORMALIZER_TAG_DE_ORDINAL_DOT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_NEGATIVE_ORDINAL_MASCULINE,
            PV_NORMALIZER_TAG_DE_ORDINAL_DOT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_NEGATIVE_ORDINAL_NEUTER,
            PV_NORMALIZER_TAG_DE_ORDINAL_DOT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_NEGATIVE_ORDINAL_MASCULINE,
            PV_NORMALIZER_TAG_DE_ORDINAL_DOT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_DATE_MONTH,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_NEGATIVE_ORDINAL_FEMININE,
            PV_NORMALIZER_TAG_DE_ORDINAL_DOT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_NEGATIVE_ORDINAL_NEUTER,
            PV_NORMALIZER_TAG_DE_ORDINAL_DOT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_NEGATIVE_ORDINAL_MASCULINE,
            PV_NORMALIZER_TAG_DE_ORDINAL_DOT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_NEGATIVE_ORDINAL_MASCULINE,
            PV_NORMALIZER_TAG_DE_ORDINAL_DOT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_NEGATIVE_ORDINAL_FEMININE,
            PV_NORMALIZER_TAG_DE_ORDINAL_DOT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_NEGATIVE_ORDINAL_NEUTER,
            PV_NORMALIZER_TAG_DE_ORDINAL_DOT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_DE_PUNCTUATION,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
    };

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            true,
            sentence_tags);
}

static void test_pv_normalizer_tagger_correct_num_tag_weights(void) {
    pv_test_true(
            PV_ARRAY_LEN(PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS) == PV_NORMALIZER_TAG_DE_NUM_TAGS,
            "Number of tags and tag weights do not match. Have %d tags and %d weights",
            PV_ARRAY_LEN(PV_NORMALIZER_TOKEN_TAG_DE_WEIGHTS),
            PV_NORMALIZER_TAG_DE_NUM_TAGS);
}

static void test_pv_normalizer_tagger_tag_special_characters(void) {
    const char sentence[] = "Hallo 5% A&W @@ °C RPM";
    const char *sentence_strings[] = {"Hallo", "5", "%", "A", "&", "W", "@", "@", "°C", "RPM"};
    const pv_normalizer_token_tag_de_t sentence_tags[] = {
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_DE_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_DE_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_DE_SPECIAL_CHARACTER,
    };

    test_pv_normalizer_tagger_tokenize_tag_and_check_strings_tags_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            sentence_tags,
            false,
            sentence_strings);
}

static void test_pv_normalizer_tagger_tag_negative_percent(void) {
    const char sentence[] = "-5% −5%";
    const char *sentence_strings[] = {
            "-5",
            "%",
            "-5",
            "%",
    };
    const pv_normalizer_token_tag_de_t sentence_tags[] = {
            PV_NORMALIZER_TAG_DE_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_DE_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPECIAL_CHARACTER,
    };

    test_pv_normalizer_tagger_tokenize_tag_and_check_strings_tags_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            sentence_tags,
            false,
            sentence_strings);
}

static void test_pv_normalizer_tagger_tag_empty_custom_pron(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    pv_status_t status = pv_normalizer_token_list_init(&token_list);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to initialize token list");

    pv_normalizer_token_t *empty_token = NULL;
    // TODO(Albert): Use IPA?
    status = pv_normalizer_token_init(0, 4, "{|AY}", false, true, false, &empty_token);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to initialize token");

    pv_normalizer_token_list_append_token(token_list, empty_token);

    const char sentence[] = "Hallo";
    status = pv_normalizer_tagger_tag(normalizer_tagger_object, token_list, 0, true);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to tag sentence: `%s`", sentence);

    pv_normalizer_token_t *current = token_list->head;
    pv_test_true(current != NULL, "token is empty");
    pv_test_true(
            current->tag == PV_NORMALIZER_TAG_DE_CUSTOM_PRONUNCIATION,
            "incorrect tag for `%s`: got `%d`, expected `%d`",
            current->string,
            current->tag,
            PV_NORMALIZER_TAG_DE_CUSTOM_PRONUNCIATION);

    pv_normalizer_token_list_delete(token_list);
}

static void test_pv_normalizer_tagger_tag_weights_order(void) {
    pv_normalizer_use_cases_de_t use_cases[] = {PV_NORMALIZER_USE_WORD_NORMALIZER_DE};

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(use_cases), use_cases, &tagger);

    pv_normalizer_token_list_t *token_list = NULL;
    pv_status_t status = pv_normalizer_token_list_init(&token_list);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to initialize token list");

    pv_normalizer_token_t *empty_token = NULL;
    // TODO(Albert): Use IPA
    status = pv_normalizer_token_init(0, 4, "{|AY}", false, true, false, &empty_token);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to initialize token");
    empty_token->tag = PV_NORMALIZER_TAG_DE_WORD;

    pv_normalizer_token_list_append_token(token_list, empty_token);

    const char sentence[] = "Hallo";
    status = pv_normalizer_tagger_tag(tagger, token_list, 0, true);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to tag sentence: `%s`", sentence);

    pv_normalizer_token_t *current = token_list->head;
    pv_test_true(current != NULL, "token is empty");
    pv_test_true(
            current->tag == PV_NORMALIZER_TAG_DE_WORD,
            "incorrect tag for `%s`: got `%d`, expected `%d`",
            current->string,
            current->tag,
            PV_NORMALIZER_TAG_DE_WORD);

    pv_normalizer_tagger_delete(tagger);

    pv_normalizer_use_cases_de_t use_cases2[] = {
            PV_NORMALIZER_USE_WORD_NORMALIZER_DE,
            PV_NORMALIZER_USE_CUSTOM_PRONUNCIATION_NORMALIZER_DE,
    };

    tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(use_cases2), use_cases2, &tagger);

    status = pv_normalizer_tagger_tag(normalizer_tagger_object, token_list, 0, true);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to tag sentence: `%s`", sentence);

    current = token_list->head;
    pv_test_true(current != NULL, "token is empty");
    pv_test_true(
            current->tag == PV_NORMALIZER_TAG_DE_CUSTOM_PRONUNCIATION,
            "incorrect tag for `%s`: got `%d`, expected `%d`",
            current->string,
            current->tag,
            PV_NORMALIZER_TAG_DE_CUSTOM_PRONUNCIATION);

    pv_normalizer_token_list_delete(token_list);
    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_tag_decimal(void) {
    const char sentence[] = ",1 1,2 ,456 1,,2 a,6 ,5% -1,1";
    const pv_normalizer_token_tag_de_t sentence_tags[] = {
            PV_NORMALIZER_TAG_DE_DECIMAL_POINT,
            PV_NORMALIZER_TAG_DE_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_DECIMAL_POINT,
            PV_NORMALIZER_TAG_DE_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_DECIMAL_POINT,
            PV_NORMALIZER_TAG_DE_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_PUNCTUATION,
            PV_NORMALIZER_TAG_DE_PUNCTUATION,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_PUNCTUATION,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_DECIMAL_POINT,
            PV_NORMALIZER_TAG_DE_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_DE_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_DE_DECIMAL_POINT,
            PV_NORMALIZER_TAG_DE_DECIMAL_DIGITS,
    };

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            true,
            sentence_tags);

    const char sentence2[] = ", 5";
    const pv_normalizer_token_tag_de_t sentence_tags2[] = {
            PV_NORMALIZER_TAG_DE_PUNCTUATION,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL};

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            sentence2,
            PV_ARRAY_LEN(sentence_tags2),
            true,
            sentence_tags2);
}

static void test_pv_normalizer_tagger_tag_alphanum_spell_out(void) {
    const char sentence[] = "HTML5 A87B 12A 12 400AD 1000BCE";
    const pv_normalizer_token_tag_de_t sentence_tags[] = {
            PV_NORMALIZER_TAG_DE_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_DE_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_DE_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_DE_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_DE_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_DE_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_DE_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_DE_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_DE_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_DE_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_DE_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_DE_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_DE_LETTER_SPELL_OUT,
    };

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            true,
            sentence_tags);
}

static void test_pv_normalizer_tagger_tag_time(void) {
    const char sentence[] =
            "7:00 Uhr uhr 09:18 uhr 18:23 21:89 25:09 Uhr 8 Uhr. 04:21uhr 12:00uhr. 2UHR 25UHR 2Uhr 12uhr";
    const pv_normalizer_token_tag_de_t sentence_tags[] = {
            PV_NORMALIZER_TAG_DE_TIME_HOURS,
            PV_NORMALIZER_TAG_DE_TIME_COLON,
            PV_NORMALIZER_TAG_DE_TIME_MINUTES,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_TIME_UHR,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_TIME_HOURS,
            PV_NORMALIZER_TAG_DE_TIME_COLON,
            PV_NORMALIZER_TAG_DE_TIME_MINUTES,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_TIME_UHR,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_TIME_HOURS,
            PV_NORMALIZER_TAG_DE_TIME_COLON,
            PV_NORMALIZER_TAG_DE_TIME_MINUTES,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_COLON,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_COLON,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_TIME_UHR,
            PV_NORMALIZER_TAG_DE_PUNCTUATION,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_TIME_HOURS,
            PV_NORMALIZER_TAG_DE_TIME_COLON,
            PV_NORMALIZER_TAG_DE_TIME_MINUTES,
            PV_NORMALIZER_TAG_DE_TIME_UHR,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_TIME_HOURS,
            PV_NORMALIZER_TAG_DE_TIME_COLON,
            PV_NORMALIZER_TAG_DE_TIME_MINUTES,
            PV_NORMALIZER_TAG_DE_TIME_UHR,
            PV_NORMALIZER_TAG_DE_PUNCTUATION,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_TIME_HOURS,
            PV_NORMALIZER_TAG_DE_TIME_UHR,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_DE_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_DE_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_DE_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_DE_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_TIME_HOURS,
            PV_NORMALIZER_TAG_DE_TIME_UHR,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_TIME_HOURS,
            PV_NORMALIZER_TAG_DE_TIME_UHR,
    };

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            true,
            sentence_tags);
}

static void test_pv_normalizer_tagger_tag_measurement(void) {
    const char sentence[] = "5 ml 1g -7,3l hello ml 10,1km 5°C 1.000g -1.000.111g 1.000.111g 25ft.-lb. klarer Kontext ml-g";
    const pv_normalizer_token_tag_de_t sentence_tags[] = {
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_MEASUREMENT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_MEASUREMENT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_DE_DECIMAL_POINT,
            PV_NORMALIZER_TAG_DE_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_DE_MEASUREMENT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_MEASUREMENT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_DECIMAL_POINT,
            PV_NORMALIZER_TAG_DE_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_DE_MEASUREMENT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_MEASUREMENT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_MEASUREMENT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_DE_MEASUREMENT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_MEASUREMENT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_MEASUREMENT,
            PV_NORMALIZER_TAG_DE_DOT,
            PV_NORMALIZER_TAG_DE_MEASUREMENT,
            PV_NORMALIZER_TAG_DE_PUNCTUATION,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_MEASUREMENT,
            PV_NORMALIZER_TAG_DE_MEASUREMENT,
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
    const char sentence[] = "5 km/m 10 oz/km 1,1m/l -5,41kg/ft m/s 7 Meilen/Stunde 1.000m/s";
    const pv_normalizer_token_tag_de_t sentence_tags[] = {
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_MEASUREMENT,
            PV_NORMALIZER_TAG_DE_PER_SLASH,
            PV_NORMALIZER_TAG_DE_MEASUREMENT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_MEASUREMENT,
            PV_NORMALIZER_TAG_DE_PER_SLASH,
            PV_NORMALIZER_TAG_DE_MEASUREMENT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_DECIMAL_POINT,
            PV_NORMALIZER_TAG_DE_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_DE_MEASUREMENT,
            PV_NORMALIZER_TAG_DE_PER_SLASH,
            PV_NORMALIZER_TAG_DE_MEASUREMENT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_DE_DECIMAL_POINT,
            PV_NORMALIZER_TAG_DE_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_DE_MEASUREMENT,
            PV_NORMALIZER_TAG_DE_PER_SLASH,
            PV_NORMALIZER_TAG_DE_MEASUREMENT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_MEASUREMENT,
            PV_NORMALIZER_TAG_DE_PER_SLASH,
            PV_NORMALIZER_TAG_DE_MEASUREMENT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_MEASUREMENT,
            PV_NORMALIZER_TAG_DE_PER_SLASH,
            PV_NORMALIZER_TAG_DE_MEASUREMENT,
    };

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            true,
            sentence_tags);
}

static void test_pv_normalizer_tagger_tag_dot_cardinal(void) {
    const char *sentence = "1.005 11.005 111.005 -5.123.123.123 1111.000 1.1.000 1. 000";
    const pv_normalizer_token_tag_de_t sentence_tags[] = {
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_DOT,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_DIGITS,
            PV_NORMALIZER_TAG_DE_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_DE_DIGITS,
            PV_NORMALIZER_TAG_DE_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_DE_DIGITS,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_PUNCTUATION,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
    };

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            true,
            sentence_tags);
}

static void test_pv_normalizer_tagger_tag_dot_ordinal(void) {
    const char *sentence = "1.005. Sommer 22.000. Aake 123.456. Aalfang 1.123.123. Frühling 2.000.000.000.000. "
                           "Abandon 1.0005. Sommer 1234.000. Aake 3.12. Frühling";
    const pv_normalizer_token_tag_de_t sentence_tags[] = {
            PV_NORMALIZER_TAG_DE_ORDINAL_MASCULINE,
            PV_NORMALIZER_TAG_DE_ORDINAL_DOT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_ORDINAL_FEMININE,
            PV_NORMALIZER_TAG_DE_ORDINAL_DOT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_ORDINAL_MASCULINE,
            PV_NORMALIZER_TAG_DE_ORDINAL_DOT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_ORDINAL_NEUTER,
            PV_NORMALIZER_TAG_DE_ORDINAL_DOT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_ORDINAL_MASCULINE,
            PV_NORMALIZER_TAG_DE_ORDINAL_DOT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_DOT,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_PUNCTUATION,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_DOT,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_PUNCTUATION,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_DOT,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_PUNCTUATION,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
    };

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            true,
            sentence_tags);
}

static void test_pv_normalizer_tagger_tag_negative_dot_ordinal(void) {
    const char *sentence = "-1.005. Sommer -22.000. Aake -123.456. Aalfang -1.123.123. Frühling -2.000.000.000.000. "
                           "Abandon -1.0005. Sommer -1234.000. Aake -3.12. Frühling";
    const pv_normalizer_token_tag_de_t sentence_tags[] = {
            PV_NORMALIZER_TAG_DE_NEGATIVE_ORDINAL_MASCULINE,
            PV_NORMALIZER_TAG_DE_ORDINAL_DOT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_NEGATIVE_ORDINAL_FEMININE,
            PV_NORMALIZER_TAG_DE_ORDINAL_DOT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_NEGATIVE_ORDINAL_MASCULINE,
            PV_NORMALIZER_TAG_DE_ORDINAL_DOT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_NEGATIVE_ORDINAL_NEUTER,
            PV_NORMALIZER_TAG_DE_ORDINAL_DOT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_NEGATIVE_ORDINAL_MASCULINE,
            PV_NORMALIZER_TAG_DE_ORDINAL_DOT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_DE_DOT,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_PUNCTUATION,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_DE_DOT,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_PUNCTUATION,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_DE_DOT,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_PUNCTUATION,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
    };

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            true,
            sentence_tags);
}

static void test_pv_normalizer_tagger_tag_dot_number_range(void) {
    const char *sentence = "1.000-2.000 1.000,2-1.000,5 1.000–2.000";

    const pv_normalizer_token_tag_de_t sentence_tags[] = {
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_NUMBER_RANGE_TO,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_DECIMAL_POINT,
            PV_NORMALIZER_TAG_DE_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_DE_NUMBER_RANGE_TO,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_DECIMAL_POINT,
            PV_NORMALIZER_TAG_DE_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_NUMBER_RANGE_TO,
            PV_NORMALIZER_TAG_DE_CARDINAL,
    };

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            true,
            sentence_tags);
}

static void test_pv_normalizer_tagger_tag_dot_decimal(void) {
    const char *sentence = "1.000,2 -300.000,6666";

    const pv_normalizer_token_tag_de_t sentence_tags[] = {
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_DECIMAL_POINT,
            PV_NORMALIZER_TAG_DE_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_DE_DECIMAL_POINT,
            PV_NORMALIZER_TAG_DE_DECIMAL_DIGITS};

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            true,
            sentence_tags);
}

static void test_pv_normalizer_tagger_tag_fraction(void) {
    const char *sentence = "1/2 1/3,2 1/1.000 1/-4 -4/1";

    const pv_normalizer_token_tag_de_t sentence_tags[] = {
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_FRACTION_SLASH,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_FRACTION_SLASH,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_DECIMAL_POINT,
            PV_NORMALIZER_TAG_DE_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_FRACTION_SLASH,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_FRACTION_SLASH,
            PV_NORMALIZER_TAG_DE_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_DE_FRACTION_SLASH,
            PV_NORMALIZER_TAG_DE_CARDINAL,
    };

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            true,
            sentence_tags);
}

static void test_pv_normalizer_tagger_tag_url(void) {
    const char *sentence = "www.example.com https://hello.xyz/";
    const pv_normalizer_token_tag_de_t sentence_tags[] = {
            PV_NORMALIZER_TAG_DE_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_DE_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_DE_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_DE_DOT,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_DOT,
            PV_NORMALIZER_TAG_DE_TOP_LEVEL_DOMAIN,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_DE_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_DE_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_DE_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_DE_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_DE_COLON,
            PV_NORMALIZER_TAG_DE_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_DE_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_DOT,
            PV_NORMALIZER_TAG_DE_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_DE_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_DE_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_DE_SPECIAL_CHARACTER,
    };

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            true,
            sentence_tags);
}

static void test_pv_normalizer_tagger_tag_currency(void) {
    const char *sentence =
            "$5 €10 10$ 10€ $1,25 1,25$ -$500 -500$ -1,25$ -$1,25 €1.000 -€1.000 1.000€ -1.000€ €1.000,25 1.000,25€ "
            "1,2$ $1,222 1USD USD1,25 1,25USD -1USD -1,25USD 1 $ -1 $ 1,25 $ -1,25 $";
    const pv_normalizer_token_tag_de_t sentence_tags[] = {
            PV_NORMALIZER_TAG_DE_CURRENCY,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CURRENCY,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CURRENCY,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CURRENCY,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CURRENCY,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CURRENCY,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CURRENCY,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CURRENCY,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CURRENCY,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CURRENCY,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_PUNCTUATION,
            PV_NORMALIZER_TAG_DE_NONE,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CURRENCY,
            PV_NORMALIZER_TAG_DE_PUNCTUATION,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CURRENCY,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CURRENCY,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CURRENCY,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CURRENCY_SYMBOL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CURRENCY_SYMBOL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_DECIMAL_POINT,
            PV_NORMALIZER_TAG_DE_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CURRENCY_SYMBOL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_DE_DECIMAL_POINT,
            PV_NORMALIZER_TAG_DE_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CURRENCY_SYMBOL,
    };

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            true,
            sentence_tags);
}

static void test_pv_normalizer_tagger_tag_abbreviations(void) {
    const char *sentence = "z. B. das d. h. Hr. Av. Str. Dr. jun. sen. Gouv. n.b. z.t U.A.";
    const pv_normalizer_token_tag_de_t sentence_tags[] = {
            PV_NORMALIZER_TAG_DE_ABBREVIATION,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_ABBREVIATION,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_ABBREVIATION,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_ABBREVIATION,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_ABBREVIATION,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_ABBREVIATION,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_ABBREVIATION,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_ABBREVIATION,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_ABBREVIATION,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_ABBREVIATION,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_DOT,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_ABBREVIATION,
    };

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            true,
            sentence_tags);
}

static void test_pv_normalizer_tagger_tag_digits_sequence(void) {
    const char *sentence = "(778) 239-1823 102-291091-4920 921.212.1203 123‒456‒7890";
    const pv_normalizer_token_tag_de_t sentence_tags[] = {
            PV_NORMALIZER_TAG_DE_DIGITS_WITH_PARENTHESES,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_DIGITS,
            PV_NORMALIZER_TAG_DE_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_DE_DIGITS,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_DIGITS,
            PV_NORMALIZER_TAG_DE_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_DE_DIGITS,
            PV_NORMALIZER_TAG_DE_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_DE_DIGITS,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_DIGITS,
            PV_NORMALIZER_TAG_DE_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_DE_DIGITS,
            PV_NORMALIZER_TAG_DE_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_DE_DIGITS,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_DIGITS,
            PV_NORMALIZER_TAG_DE_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_DE_DIGITS,
            PV_NORMALIZER_TAG_DE_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_DE_DIGITS,
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
    const char *sentence = "06-03-2020 31-02-1991 06/03/2020 6/3/2020 12-31-2024 1991/03/13 1991/13/13 07/13/182 2024-05-06";
    const pv_normalizer_token_tag_de_t sentence_tags[] = {
            PV_NORMALIZER_TAG_DE_DATE_DAY,
            PV_NORMALIZER_TAG_DE_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_DE_DATE_MONTH,
            PV_NORMALIZER_TAG_DE_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_DE_DATE_YEAR,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_DATE_DAY,
            PV_NORMALIZER_TAG_DE_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_DE_DATE_MONTH,
            PV_NORMALIZER_TAG_DE_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_DE_DATE_YEAR,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_DATE_DAY,
            PV_NORMALIZER_TAG_DE_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_DE_DATE_MONTH,
            PV_NORMALIZER_TAG_DE_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_DE_DATE_YEAR,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_DATE_DAY,
            PV_NORMALIZER_TAG_DE_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_DE_DATE_MONTH,
            PV_NORMALIZER_TAG_DE_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_DE_DATE_YEAR,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_DATE_MONTH,
            PV_NORMALIZER_TAG_DE_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_DE_DATE_DAY,
            PV_NORMALIZER_TAG_DE_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_DE_DATE_YEAR,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_DATE_YEAR,
            PV_NORMALIZER_TAG_DE_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_DE_DATE_MONTH,
            PV_NORMALIZER_TAG_DE_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_DE_DATE_DAY,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_DIGITS,
            PV_NORMALIZER_TAG_DE_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_DE_DIGITS,
            PV_NORMALIZER_TAG_DE_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_DE_DIGITS,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_DIGITS,
            PV_NORMALIZER_TAG_DE_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_DE_DIGITS,
            PV_NORMALIZER_TAG_DE_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_DE_DIGITS,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_DATE_YEAR,
            PV_NORMALIZER_TAG_DE_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_DE_DATE_MONTH,
            PV_NORMALIZER_TAG_DE_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_DE_DATE_DAY,
    };

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            true,
            sentence_tags);

    const char *sentence_two = "3 Juni 1637 03-Jun-2024 32-10-2024 1 Aug Jul 2024";
    const pv_normalizer_token_tag_de_t sentence_tags_two[] = {
            PV_NORMALIZER_TAG_DE_DATE_DAY,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_DATE_MONTH,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_DATE_YEAR,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_DATE_DAY,
            PV_NORMALIZER_TAG_DE_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_DE_DATE_MONTH,
            PV_NORMALIZER_TAG_DE_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_DE_DATE_YEAR,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_DIGITS,
            PV_NORMALIZER_TAG_DE_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_DE_DIGITS,
            PV_NORMALIZER_TAG_DE_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_DE_DIGITS,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_DATE_DAY,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_DATE_MONTH,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_DATE_MONTH,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_DATE_YEAR,
    };

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            sentence_two,
            PV_ARRAY_LEN(sentence_tags_two),
            true,
            sentence_tags_two);
}

static void test_pv_normalizer_tagger_tag_name(void) {
    const char *sentence = "Das ist J. R. R. Tolkien John F. Kennedy 20 Uhr.";
    const pv_normalizer_token_tag_de_t sentence_tags[] = {
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_DE_NAME_INITIAL_DOT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_DE_NAME_INITIAL_DOT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_DE_NAME_INITIAL_DOT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_DE_NAME_INITIAL_DOT,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_CARDINAL,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_TIME_UHR,
            PV_NORMALIZER_TAG_DE_PUNCTUATION,
    };

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            true,
            sentence_tags);
}

static void test_pv_normalizer_tagger_tag_single_quote(void) {
    const char *sentence = "'Hilfe'-Taste Produkt 'kaufen.' '(Hallo)' 'z. B.'";
    const pv_normalizer_token_tag_de_t sentence_tags[] = {
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_PUNCTUATION,
            PV_NORMALIZER_TAG_DE_SINGLE_QUOTE,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_SINGLE_QUOTE,
            PV_NORMALIZER_TAG_DE_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_DE_SINGLE_QUOTE,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_SINGLE_QUOTE,
            PV_NORMALIZER_TAG_DE_ABBREVIATION,
            PV_NORMALIZER_TAG_DE_SINGLE_QUOTE,
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

static void test_pv_normalizer_tagger_length_future_past_context_single_quote(void) {
    const char *sentence = "'Einkaufen gehen' oder 'Kochen'.";
    const pv_normalizer_token_tag_de_t sentence_tags[] = {
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_SPACE,
            PV_NORMALIZER_TAG_DE_WORD,
            PV_NORMALIZER_TAG_DE_PUNCTUATION,
    };
    const char *sentence_original_strings[] = {
            "'Einkaufen", " ",
            "gehen'", " ",
            "oder", " ",
            "'Kochen'", ".",
    };
    const int32_t sentence_futures[8] = {0};
    const int32_t sentence_pasts[8] = {0};

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
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_tagger_de_init, PV_STATUS_OUT_OF_MEMORY);

    static pv_normalizer_use_cases_de_t use_cases[] = {PV_NORMALIZER_USE_WORD_NORMALIZER_DE};
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
            "`pv_normalizer_tagger_de_init` failed with status `OUT_OF_MEMORY`.",
            true,
            "Error message mismatch"
    );
}


static void test_pv_normalizer_tagger_tag_cardinal_helper_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_util_check_token_is_before_character, PV_STATUS_INVALID_ARGUMENT);

    const char sentence[] = "123";
    static pv_normalizer_use_cases_de_t use_cases[] = {PV_NORMALIZER_USE_CARDINAL_NORMALIZER_DE};
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
    static pv_normalizer_use_cases_de_t use_cases[] = {PV_NORMALIZER_USE_CURRENCY_NORMALIZER_DE};
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
        {"tag_word", test_pv_normalizer_tagger_tag_word},
        {"tag_punctuation", test_pv_normalizer_tagger_tag_punctuation},
        {"tag_cardinal", test_pv_normalizer_tagger_tag_cardinal},
        {"tag_negative_cardinal", test_pv_normalizer_tagger_tag_negative_cardinal},
        {"tag_custom_pronunciation", test_pv_normalizer_tagger_tag_custom_pronunciation},
        {"tag_invalid_use_case", test_pv_normalizer_tagger_tag_invalid_use_case},
        {"tag_hyphenated_string", test_pv_normalizer_tagger_tag_hyphenated_string},
        {"tag_invalid_hyphens_only", test_pv_normalizer_tagger_tag_invalid_hyphens_only},
        {"tag_number_range", test_pv_normalizer_tagger_tag_number_range},
        {"tag_ordinal", test_pv_normalizer_tagger_tag_ordinal},
        {"tag_negative_ordinal", test_pv_normalizer_tagger_tag_negative_ordinal},
        {"test_correct_num_tag_weights", test_pv_normalizer_tagger_correct_num_tag_weights},

        {"tag_special_characters", test_pv_normalizer_tagger_tag_special_characters},
        {"tag_negative_percent", test_pv_normalizer_tagger_tag_negative_percent},
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
        {"tag_negative_dot_ordinal", test_pv_normalizer_tagger_tag_negative_dot_ordinal},
        {"tag_dot_number_range", test_pv_normalizer_tagger_tag_dot_number_range},
        {"tag_dot_decimal", test_pv_normalizer_tagger_tag_dot_decimal},
        {"tag_dot_fraction", test_pv_normalizer_tagger_tag_fraction},
        {"tag_url", test_pv_normalizer_tagger_tag_url},
        {"tag_currency", test_pv_normalizer_tagger_tag_currency},
        {"tag_abbreviations", test_pv_normalizer_tagger_tag_abbreviations},
        {"tag_digits_sequence", test_pv_normalizer_tagger_tag_digits_sequence},
        {"tag_date", test_pv_normalizer_tagger_tag_date},
        {"tag_name", test_pv_normalizer_tagger_tag_name},
        {"tag_single_quote", test_pv_normalizer_tagger_tag_single_quote},
        {"length_future_past_single_quote", test_pv_normalizer_tagger_length_future_past_context_single_quote},

#ifdef __PV_MOCKS__

        {"tagger_init_failure", test_pv_normalizer_tagger_init_failure},
        {"tagger_tag_cardinal_helper_failure", test_pv_normalizer_tagger_tag_cardinal_helper_failure},
        {"tagger_tag_currency_helper_failure", test_pv_normalizer_tagger_tag_currency_helper_failure},

#endif

};

const pv_test_suite_t PV_NORMALIZER_TAGGER_DE_TEST_SUITE = {
        .name = "normalizer_tagger_de",
        .setup = test_pv_normalizer_tagger_setup,
        .teardown = test_pv_normalizer_tagger_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_NORMALIZER_TAGGER_TEST_CASES),
        .test_cases = PV_NORMALIZER_TAGGER_TEST_CASES,
};
