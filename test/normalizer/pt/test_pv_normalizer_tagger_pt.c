#include <stdlib.h>
#include <string.h>

#include "core/pv_language.h"
#include "core/pv_language_json.h"
#include "orca/normalizer/pv_normalizer_tagger.h"
#include "orca/normalizer/pv_normalizer_tokenizer.h"
#include "orca/normalizer/pv_normalizer_use_cases.h"
#include "test/pv_test.h"

#include "orca/normalizer/pt/pv_normalizer_tags_pt.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_normalizer_mock.h"

#endif

static pv_normalizer_tagger_t *normalizer_tagger_object = NULL;
static pv_normalizer_use_cases_pt_t ALL_TEST_USE_CASES[] = {
        PV_NORMALIZER_USE_WORD_NORMALIZER_PT,
        PV_NORMALIZER_USE_CUSTOM_PRONUNCIATION_NORMALIZER_PT,
        PV_NORMALIZER_USE_PUNCTUATION_NORMALIZER_PT,
        PV_NORMALIZER_USE_CARDINAL_NORMALIZER_PT,
        PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_PT,
        PV_NORMALIZER_USE_NUMBER_RANGE_NORMALIZER_PT,
        PV_NORMALIZER_USE_DECIMAL_NORMALIZER_PT,
        PV_NORMALIZER_USE_ALPHANUM_SPELL_OUT_NORMALIZER_PT,
        PV_NORMALIZER_USE_FRACTION_NORMALIZER_PT,
        PV_NORMALIZER_USE_MEASUREMENT_NORMALIZER_PT,
        PV_NORMALIZER_USE_TIME_NORMALIZER_PT,
        PV_NORMALIZER_USE_SPECIAL_CHARACTER_NORMALIZER_PT,
        PV_NORMALIZER_USE_URL_NORMALIZER_PT,
        PV_NORMALIZER_USE_CURRENCY_NORMALIZER_PT,
        PV_NORMALIZER_USE_ABBREVIATION_NORMALIZER_PT,
        PV_NORMALIZER_USE_DIGITS_SEQUENCE_NORMALIZER_PT,
        PV_NORMALIZER_USE_DATE_NORMALIZER_PT,
        PV_NORMALIZER_USE_NAME_NORMALIZER_PT,
        PV_NORMALIZER_USE_ORDINAL_NORMALIZER_PT,
        PV_NORMALIZER_USE_NEGATIVE_ORDINAL_NORMALIZER_PT,
};

static pv_normalizer_tokenizer_t *normalizer_tokenizer_object = NULL;
static pv_language_info_t *language_info_object = NULL;
static pv_noun_gender_dict_t *noun_gender_dict_object = NULL;

static const char LANGUAGE_INFO_PATH[] = "normalizer/ref/pv_language_info_normalizer_pt.json";
static const char NOUN_GENDER_DICT_PATH[] = "orca/test_data/noun_gender_dict/noun_gender_dict_pt.txt";

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
        const pv_normalizer_use_cases_pt_t *use_cases,
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
        const pv_normalizer_token_tag_pt_t *target_tags) {
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
        const pv_normalizer_token_tag_pt_t *target_tags,
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

static void test_pv_normalizer_tagger_tokenize_tag_and_check_tags_gender_helper(
        pv_normalizer_tokenizer_t *tokenizer,
        pv_normalizer_tagger_t *tagger,
        const char *sentence,
        int32_t num_target_tokens,
        const pv_normalizer_token_tag_pt_t *target_tags,
        bool preserve_word_boundary,
        const pv_normalizer_token_gender_t *target_genders) {
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
                current->gender == target_genders[i],
                "incorrect gender for `%s`: got `%d`, expected `%d`",
                current->string,
                current->gender,
                target_genders[i]);
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
        const pv_normalizer_token_tag_pt_t *target_tags,
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

static void test_pv_normalizer_tagger_tag_invalid_use_case(void) {
    static pv_normalizer_use_cases_pt_t invalid_use_cases[] = {99};
    const char sentence[] = "Quer bolo, leite, ou suco?";

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(invalid_use_cases), invalid_use_cases, &tagger);

    test_pv_normalizer_tagger_tokenize_tag_and_check_status_helper(
            normalizer_tokenizer_object,
            tagger,
            sentence,
            PV_STATUS_INVALID_ARGUMENT);

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_correct_num_tag_weights(void) {
    pv_test_true(
            PV_ARRAY_LEN(PV_NORMALIZER_TOKEN_TAG_PT_WEIGHTS) == PV_NORMALIZER_TAG_PT_NUM_TAGS,
            "Number of tags and tag weights do not match. Have %d tags and %d weights",
            PV_ARRAY_LEN(PV_NORMALIZER_TOKEN_TAG_PT_WEIGHTS),
            PV_NORMALIZER_TAG_PT_NUM_TAGS);
}

static void test_pv_normalizer_tagger_tag_weights_order(void) {
    const char sentence[] = "Quer bolo, leite, ou suco?";
    pv_normalizer_use_cases_pt_t use_cases[] = {PV_NORMALIZER_USE_WORD_NORMALIZER_PT};

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(use_cases), use_cases, &tagger);

    pv_normalizer_token_list_t *token_list = NULL;
    pv_status_t status = pv_normalizer_token_list_init(&token_list);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to initialize token list");

    pv_normalizer_token_t *empty_token = NULL;
    status = pv_normalizer_token_init(0, 4, "{|AY}", false, true, false, &empty_token);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to initialize token");
    empty_token->tag = PV_NORMALIZER_TAG_PT_WORD;

    pv_normalizer_token_list_append_token(token_list, empty_token);

    status = pv_normalizer_tagger_tag(tagger, token_list, 0, true);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to tag sentence: `%s`", sentence);

    pv_normalizer_token_t *current = token_list->head;
    pv_test_true(current != NULL, "token is empty");
    pv_test_true(
            current->tag == PV_NORMALIZER_TAG_PT_WORD,
            "incorrect tag for `%s`: got `%d`, expected `%d`",
            current->string,
            current->tag,
            PV_NORMALIZER_TAG_PT_WORD);

    pv_normalizer_tagger_delete(tagger);

    pv_normalizer_use_cases_pt_t use_cases2[] = {
            PV_NORMALIZER_USE_WORD_NORMALIZER_PT,
            PV_NORMALIZER_USE_CUSTOM_PRONUNCIATION_NORMALIZER_PT};

    tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(use_cases2), use_cases2, &tagger);

    status = pv_normalizer_tagger_tag(normalizer_tagger_object, token_list, 0, true);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to tag sentence: `%s`", sentence);

    current = token_list->head;
    pv_test_true(current != NULL, "token is empty");
    pv_test_true(
            current->tag == PV_NORMALIZER_TAG_PT_CUSTOM_PRONUNCIATION,
            "incorrect tag for `%s`: got `%d`, expected `%d`",
            current->string,
            current->tag,
            PV_NORMALIZER_TAG_PT_CUSTOM_PRONUNCIATION);

    pv_normalizer_token_list_delete(token_list);
    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_tag_word(void) {
    static pv_normalizer_use_cases_pt_t use_cases[] = {PV_NORMALIZER_USE_WORD_NORMALIZER_PT};

    const char sentence[] = "O amor é a força mais poderosa do universo";
    const pv_normalizer_token_tag_pt_t sentence_tags[] = {
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_WORD,
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

static void test_pv_normalizer_tagger_tag_hyphenated_string(void) {
    static pv_normalizer_use_cases_pt_t use_cases[] = {PV_NORMALIZER_USE_WORD_NORMALIZER_PT};

    const char sentence[] = "4-min carra -563 sedu-lo 2-2 a- some–where";
    const char *sentence_strings[] = {
            "4",
            "min",
            "carra",
            "563",
            "sedu",
            "lo",
            "2",
            "2",
            "a",
            "some",
            "where",
    };
    const pv_normalizer_token_tag_pt_t sentence_tags[] = {
            PV_NORMALIZER_TAG_PT_NONE,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_NONE,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_NONE,
            PV_NORMALIZER_TAG_PT_NONE,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_WORD,
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
    static pv_normalizer_use_cases_pt_t use_cases[] = {PV_NORMALIZER_USE_WORD_NORMALIZER_PT};

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

static void test_pv_normalizer_tagger_tag_punctuation(void) {
    static pv_normalizer_use_cases_pt_t use_cases[] = {PV_NORMALIZER_USE_PUNCTUATION_NORMALIZER_PT};

    const char sentence[] = "Quer bolo, leite, ou suco?";
    const pv_normalizer_token_tag_pt_t sentence_tags[] = {
            PV_NORMALIZER_TAG_PT_NONE,
            PV_NORMALIZER_TAG_PT_NONE,
            PV_NORMALIZER_TAG_PT_PUNCTUATION,
            PV_NORMALIZER_TAG_PT_NONE,
            PV_NORMALIZER_TAG_PT_PUNCTUATION,
            PV_NORMALIZER_TAG_PT_NONE,
            PV_NORMALIZER_TAG_PT_NONE,
            PV_NORMALIZER_TAG_PT_PUNCTUATION,
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

static void test_pv_normalizer_tagger_tag_custom_pronunciation(void) {
    static pv_normalizer_use_cases_pt_t use_cases[] = {PV_NORMALIZER_USE_CUSTOM_PRONUNCIATION_NORMALIZER_PT};

    const char sentence[] = "Uma pronúncia {lus|l u z}";
    const pv_normalizer_token_tag_pt_t sentence_tags[] = {
            PV_NORMALIZER_TAG_PT_NONE,
            PV_NORMALIZER_TAG_PT_NONE,
            PV_NORMALIZER_TAG_PT_CUSTOM_PRONUNCIATION};

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

static void test_pv_normalizer_tagger_tag_empty_custom_pron(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    pv_status_t status = pv_normalizer_token_list_init(&token_list);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to initialize token list");

    pv_normalizer_token_t *empty_token = NULL;
    status = pv_normalizer_token_init(0, 4, "{|AY}", false, true, false, &empty_token);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to initialize token");

    pv_normalizer_token_list_append_token(token_list, empty_token);

    const char sentence[] = "Quer bolo, leite, ou suco?";
    status = pv_normalizer_tagger_tag(normalizer_tagger_object, token_list, 0, true);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to tag sentence: `%s`", sentence);

    pv_normalizer_token_t *current = token_list->head;
    pv_test_true(current != NULL, "token is empty");
    pv_test_true(
            current->tag == PV_NORMALIZER_TAG_PT_CUSTOM_PRONUNCIATION,
            "incorrect tag for `%s`: got `%d`, expected `%d`",
            current->string,
            current->tag,
            PV_NORMALIZER_TAG_PT_CUSTOM_PRONUNCIATION);

    pv_normalizer_token_list_delete(token_list);
}

static void test_pv_normalizer_tagger_tag_cardinal(void) {
    static pv_normalizer_use_cases_pt_t use_cases[] = {
            PV_NORMALIZER_USE_PUNCTUATION_NORMALIZER_PT,
            PV_NORMALIZER_USE_CARDINAL_NORMALIZER_PT,
    };

    const char sentence[] = "1 abegoaria, 2 abarca, 3 acenos e 1 abóbora. 4";
    const pv_normalizer_token_tag_pt_t sentence_tags[] = {
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_NONE,
            PV_NORMALIZER_TAG_PT_PUNCTUATION,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_NONE,
            PV_NORMALIZER_TAG_PT_PUNCTUATION,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_NONE,
            PV_NORMALIZER_TAG_PT_NONE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_NONE,
            PV_NORMALIZER_TAG_PT_PUNCTUATION,
            PV_NORMALIZER_TAG_PT_CARDINAL,
    };
    const pv_normalizer_token_gender_t gender_tags[] = {
            PV_NORMALIZER_TOKEN_GENDER_FEMININE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_FEMININE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_FEMININE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
    };

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(use_cases), use_cases, &tagger);

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_gender_helper(
            normalizer_tokenizer_object,
            tagger,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            sentence_tags,
            false,
            gender_tags);

    pv_normalizer_tagger_delete(tagger);

    const char sentence2[] = "Em 2.023, cerca de 1.200.000.";
    const pv_normalizer_token_tag_pt_t sentence_tags2[] = {
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_PUNCTUATION,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_PUNCTUATION,
    };

    tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(ALL_TEST_USE_CASES), ALL_TEST_USE_CASES, &tagger);

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            tagger,
            sentence2,
            PV_ARRAY_LEN(sentence_tags2),
            false,
            sentence_tags2);

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_tag_negative_cardinal(void) {
    static pv_normalizer_use_cases_pt_t use_cases[] = {
            PV_NORMALIZER_USE_CARDINAL_NORMALIZER_PT,
            PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_PT,
    };

    const char sentence[] = "Os números -5 e -123 são menores que zero -5.000 abarca −123 abarca";
    const pv_normalizer_token_tag_pt_t sentence_tags[] = {
            PV_NORMALIZER_TAG_PT_NONE,
            PV_NORMALIZER_TAG_PT_NONE,
            PV_NORMALIZER_TAG_PT_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_PT_NONE,
            PV_NORMALIZER_TAG_PT_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_PT_NONE,
            PV_NORMALIZER_TAG_PT_NONE,
            PV_NORMALIZER_TAG_PT_NONE,
            PV_NORMALIZER_TAG_PT_NONE,
            PV_NORMALIZER_TAG_PT_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_PT_NONE,
            PV_NORMALIZER_TAG_PT_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_PT_NONE,
    };
    const pv_normalizer_token_gender_t gender_tags[] = {
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_FEMININE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_FEMININE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
    };

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(use_cases), use_cases, &tagger);

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_gender_helper(
            normalizer_tokenizer_object,
            tagger,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            sentence_tags,
            false,
            gender_tags);

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_tag_dot_cardinal(void) {
    static pv_normalizer_use_cases_pt_t use_cases[] = {
            PV_NORMALIZER_USE_PUNCTUATION_NORMALIZER_PT,
            PV_NORMALIZER_USE_CARDINAL_NORMALIZER_PT,
            PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_PT,
    };

    const char *sentence = "1.005 11.005 111.005 -5.123.123.123 -1.123.456 1111.000 1.1.000 1. 000";
    const pv_normalizer_token_tag_pt_t sentence_tags[] = {
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_PUNCTUATION,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_PUNCTUATION,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_PUNCTUATION,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_PUNCTUATION,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
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

static void test_pv_normalizer_tagger_tag_dot_cardinal_gender(void) {
    static pv_normalizer_use_cases_pt_t use_cases[] = {
            PV_NORMALIZER_USE_WORD_NORMALIZER_PT,
            PV_NORMALIZER_USE_PUNCTUATION_NORMALIZER_PT,
            PV_NORMALIZER_USE_CARDINAL_NORMALIZER_PT,
            PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_PT,
    };

    const char *sentence = "1.222 abafação 23.234 abarca 345.678 acenos 3 abóbora 4.345.356";

    const pv_normalizer_token_tag_pt_t sentence_tags[] = {
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
    };
    const pv_normalizer_token_gender_t gender_tags[] = {
            PV_NORMALIZER_TOKEN_GENDER_FEMININE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_FEMININE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_FEMININE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
    };

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(use_cases), use_cases, &tagger);

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_gender_helper(
            normalizer_tokenizer_object,
            tagger,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            sentence_tags,
            true,
            gender_tags);

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_tag_ordinal(void) {
    const char *sentence =
            "3º 26º 50ª 78ª 123ª 1.000º 12.123ª 4.066.o 789.a";
    const pv_normalizer_token_tag_pt_t sentence_tags[] = {
            PV_NORMALIZER_TAG_PT_ORDINAL,
            PV_NORMALIZER_TAG_PT_ORDINAL,
            PV_NORMALIZER_TAG_PT_ORDINAL,
            PV_NORMALIZER_TAG_PT_ORDINAL,
            PV_NORMALIZER_TAG_PT_ORDINAL,
            PV_NORMALIZER_TAG_PT_ORDINAL,
            PV_NORMALIZER_TAG_PT_ORDINAL,
            PV_NORMALIZER_TAG_PT_ORDINAL,
            PV_NORMALIZER_TAG_PT_ORDINAL,
    };
    const pv_normalizer_token_gender_t gender_tags[] = {
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_FEMININE,
            PV_NORMALIZER_TOKEN_GENDER_FEMININE,
            PV_NORMALIZER_TOKEN_GENDER_FEMININE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_FEMININE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_FEMININE,
    };

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(ALL_TEST_USE_CASES), ALL_TEST_USE_CASES, &tagger);

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_gender_helper(
            normalizer_tokenizer_object,
            tagger,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            sentence_tags,
            false,
            gender_tags);

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_tag_negative_ordinal(void) {
    const char *sentence =
            "-3º -26º -50ª -78ª -123ª -1.000º -12.123ª -466.o -789.a −3º";
    const pv_normalizer_token_tag_pt_t sentence_tags[] = {
            PV_NORMALIZER_TAG_PT_NEGATIVE_ORDINAL,
            PV_NORMALIZER_TAG_PT_NEGATIVE_ORDINAL,
            PV_NORMALIZER_TAG_PT_NEGATIVE_ORDINAL,
            PV_NORMALIZER_TAG_PT_NEGATIVE_ORDINAL,
            PV_NORMALIZER_TAG_PT_NEGATIVE_ORDINAL,
            PV_NORMALIZER_TAG_PT_NEGATIVE_ORDINAL,
            PV_NORMALIZER_TAG_PT_NEGATIVE_ORDINAL,
            PV_NORMALIZER_TAG_PT_NEGATIVE_ORDINAL,
            PV_NORMALIZER_TAG_PT_NEGATIVE_ORDINAL,
            PV_NORMALIZER_TAG_PT_NEGATIVE_ORDINAL,
    };
    const pv_normalizer_token_gender_t gender_tags[] = {
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_FEMININE,
            PV_NORMALIZER_TOKEN_GENDER_FEMININE,
            PV_NORMALIZER_TOKEN_GENDER_FEMININE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_FEMININE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_FEMININE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
    };

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(ALL_TEST_USE_CASES), ALL_TEST_USE_CASES, &tagger);

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_gender_helper(
            normalizer_tokenizer_object,
            tagger,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            sentence_tags,
            false,
            gender_tags);

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_tag_invalid_ordinal(void) {
    const char *sentence =
            "-10000000000000000000.a -10000000000000000000º 10000000000000000000.o 10000000000000000000ª";
    const pv_normalizer_token_tag_pt_t sentence_tags[] = {
            PV_NORMALIZER_TAG_PT_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_PT_DOT,
            PV_NORMALIZER_TAG_PT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_PT_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_DOT,
            PV_NORMALIZER_TAG_PT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_SPECIAL_CHARACTER,
    };
    const pv_normalizer_token_gender_t gender_tags[] = {
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
    };

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(ALL_TEST_USE_CASES), ALL_TEST_USE_CASES, &tagger);

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_gender_helper(
            normalizer_tokenizer_object,
            tagger,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            sentence_tags,
            true,
            gender_tags);

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_tag_number_range(void) {
    static pv_normalizer_use_cases_pt_t use_cases[] = {
            PV_NORMALIZER_USE_WORD_NORMALIZER_PT,
            PV_NORMALIZER_USE_CARDINAL_NORMALIZER_PT,
            PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_PT,
            PV_NORMALIZER_USE_NUMBER_RANGE_NORMALIZER_PT,
    };

    const char sentence[] = "12-5 4-3 2-22 abegoaria 123–12";
    const pv_normalizer_token_tag_pt_t sentence_tags[] = {
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_NUMBER_RANGE_TO,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_NUMBER_RANGE_TO,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_NUMBER_RANGE_TO,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_NUMBER_RANGE_TO,
            PV_NORMALIZER_TAG_PT_CARDINAL,
    };
    const pv_normalizer_token_gender_t gender_tags[] = {
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_FEMININE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_FEMININE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
    };

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(use_cases), use_cases, &tagger);

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_gender_helper(
            normalizer_tokenizer_object,
            tagger,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            sentence_tags,
            true,
            gender_tags);

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_tag_dot_number_range(void) {
    static pv_normalizer_use_cases_pt_t use_cases[] = {
            PV_NORMALIZER_USE_CARDINAL_NORMALIZER_PT,
            PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_PT,
            PV_NORMALIZER_USE_NUMBER_RANGE_NORMALIZER_PT,
    };

    const char *sentence = "1.000-2.000 32.007-12.324.245 456.789.789-23.245 abegoaria 1.000–2.000";

    const pv_normalizer_token_tag_pt_t sentence_tags[] = {
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_NUMBER_RANGE_TO,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_NUMBER_RANGE_TO,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_NUMBER_RANGE_TO,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_NONE,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_NUMBER_RANGE_TO,
            PV_NORMALIZER_TAG_PT_CARDINAL,
    };
    const pv_normalizer_token_gender_t gender_tags[] = {
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_FEMININE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_FEMININE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
    };

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(use_cases), use_cases, &tagger);

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_gender_helper(
            normalizer_tokenizer_object,
            tagger,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            sentence_tags,
            true,
            gender_tags);

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_tag_decimal(void) {
    static pv_normalizer_use_cases_pt_t use_cases[] = {
            PV_NORMALIZER_USE_CARDINAL_NORMALIZER_PT,
            PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_PT,
            PV_NORMALIZER_USE_DECIMAL_NORMALIZER_PT,
    };

    const char sentence[] = ",1 1,2 ,456 1,,2 -12,1";
    const pv_normalizer_token_tag_pt_t sentence_tags[] = {
            PV_NORMALIZER_TAG_PT_DECIMAL_POINT,
            PV_NORMALIZER_TAG_PT_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_DECIMAL_POINT,
            PV_NORMALIZER_TAG_PT_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_DECIMAL_POINT,
            PV_NORMALIZER_TAG_PT_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_PUNCTUATION,
            PV_NORMALIZER_TAG_PT_PUNCTUATION,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_PT_DECIMAL_POINT,
            PV_NORMALIZER_TAG_PT_DECIMAL_DIGITS,
    };
    const pv_normalizer_token_gender_t gender_tags[] = {
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
    };

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(use_cases), use_cases, &tagger);

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_gender_helper(
            normalizer_tokenizer_object,
            tagger,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            sentence_tags,
            true,
            gender_tags);

    const char sentence2[] = ", 5";
    const pv_normalizer_token_tag_pt_t sentence_tags2[] = {
            PV_NORMALIZER_TAG_PT_PUNCTUATION,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
    };

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            tagger,
            sentence2,
            PV_ARRAY_LEN(sentence_tags2),
            true,
            sentence_tags2);

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_tag_dot_decimal(void) {
    static pv_normalizer_use_cases_pt_t use_cases[] = {
            PV_NORMALIZER_USE_CARDINAL_NORMALIZER_PT,
            PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_PT,
            PV_NORMALIZER_USE_DECIMAL_NORMALIZER_PT,
    };

    const char *sentence = "1.000,2 -300.000,6666";
    const pv_normalizer_token_tag_pt_t sentence_tags[] = {
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_DECIMAL_POINT,
            PV_NORMALIZER_TAG_PT_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_PT_DECIMAL_POINT,
            PV_NORMALIZER_TAG_PT_DECIMAL_DIGITS,
    };
    const pv_normalizer_token_gender_t gender_tags[] = {
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
    };

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(use_cases), use_cases, &tagger);

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_gender_helper(
            normalizer_tokenizer_object,
            tagger,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            sentence_tags,
            true,
            gender_tags);

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_tag_alphanum_spell_out(void) {
    static pv_normalizer_use_cases_pt_t use_cases[] = {
            PV_NORMALIZER_USE_CARDINAL_NORMALIZER_PT,
            PV_NORMALIZER_USE_ALPHANUM_SPELL_OUT_NORMALIZER_PT,
    };

    const char sentence[] = "HTML5 A87B 12A 12";
    const pv_normalizer_token_tag_pt_t sentence_tags[] = {
            PV_NORMALIZER_TAG_PT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_PT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_PT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_PT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_PT_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_PT_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_PT_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_PT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_PT_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_PT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
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

static void test_pv_normalizer_tagger_tag_fraction(void) {
    static pv_normalizer_use_cases_pt_t use_cases[] = {
            PV_NORMALIZER_USE_CARDINAL_NORMALIZER_PT,
            PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_PT,
            PV_NORMALIZER_USE_DECIMAL_NORMALIZER_PT,
            PV_NORMALIZER_USE_FRACTION_NORMALIZER_PT,
    };

    const char *sentence = "1/2 1/3,2 1/1.000 1/-4 -4/1.000.000 5.000/2";
    const pv_normalizer_token_tag_pt_t sentence_tags[] = {
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_FRACTION_SLASH,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_FRACTION_SLASH,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_DECIMAL_POINT,
            PV_NORMALIZER_TAG_PT_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_FRACTION_SLASH,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_FRACTION_SLASH,
            PV_NORMALIZER_TAG_PT_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_PT_FRACTION_SLASH,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_FRACTION_SLASH,
            PV_NORMALIZER_TAG_PT_CARDINAL,
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

static void test_pv_normalizer_tagger_tag_measurement(void) {
    static pv_normalizer_use_cases_pt_t use_cases[] = {
            PV_NORMALIZER_USE_WORD_NORMALIZER_PT,
            PV_NORMALIZER_USE_CARDINAL_NORMALIZER_PT,
            PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_PT,
            PV_NORMALIZER_USE_DECIMAL_NORMALIZER_PT,
            PV_NORMALIZER_USE_MEASUREMENT_NORMALIZER_PT,
    };

    const char sentence[] = "5 g 1ml -7,3l 10,1km 5°C hora ml M² KM² 1.000g -1.000.111g 25ft.-lb. contexto claro ml-g";
    const pv_normalizer_token_tag_pt_t sentence_tags[] = {
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_MEASUREMENT,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_MEASUREMENT,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_PT_DECIMAL_POINT,
            PV_NORMALIZER_TAG_PT_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_PT_MEASUREMENT,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_DECIMAL_POINT,
            PV_NORMALIZER_TAG_PT_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_PT_MEASUREMENT,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_MEASUREMENT,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_MEASUREMENT,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_MEASUREMENT,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_MEASUREMENT,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_MEASUREMENT,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_PT_MEASUREMENT,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_MEASUREMENT,
            PV_NORMALIZER_TAG_PT_PUNCTUATION,
            PV_NORMALIZER_TAG_PT_MEASUREMENT,
            PV_NORMALIZER_TAG_PT_PUNCTUATION,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_MEASUREMENT,
            PV_NORMALIZER_TAG_PT_MEASUREMENT,
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

static void test_pv_normalizer_tagger_tag_per_measurement(void) {
    const char sentence[] = "5 km/h 10 oz/km 1,1m/l -5,41kg/ft m/s metros/hora 1.000.111m/s -1.000.111m/s";
    const pv_normalizer_token_tag_pt_t sentence_tags[] = {
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_MEASUREMENT,
            PV_NORMALIZER_TAG_PT_PER_SLASH,
            PV_NORMALIZER_TAG_PT_MEASUREMENT,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_MEASUREMENT,
            PV_NORMALIZER_TAG_PT_PER_SLASH,
            PV_NORMALIZER_TAG_PT_MEASUREMENT,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_DECIMAL_POINT,
            PV_NORMALIZER_TAG_PT_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_PT_MEASUREMENT,
            PV_NORMALIZER_TAG_PT_PER_SLASH,
            PV_NORMALIZER_TAG_PT_MEASUREMENT,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_PT_DECIMAL_POINT,
            PV_NORMALIZER_TAG_PT_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_PT_MEASUREMENT,
            PV_NORMALIZER_TAG_PT_PER_SLASH,
            PV_NORMALIZER_TAG_PT_MEASUREMENT,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_MEASUREMENT,
            PV_NORMALIZER_TAG_PT_PER_SLASH,
            PV_NORMALIZER_TAG_PT_MEASUREMENT,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_PER_SLASH,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_MEASUREMENT,
            PV_NORMALIZER_TAG_PT_PER_SLASH,
            PV_NORMALIZER_TAG_PT_MEASUREMENT,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_PT_MEASUREMENT,
            PV_NORMALIZER_TAG_PT_PER_SLASH,
            PV_NORMALIZER_TAG_PT_MEASUREMENT,
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

static void test_pv_normalizer_tagger_tag_measurement_gender(void) {
    const char *sentence = "1 ml 2lb 3yd/h 4h 14s 22 yd/h 1.000.111g";
    const pv_normalizer_token_tag_pt_t sentence_tags[] = {
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_MEASUREMENT,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_MEASUREMENT,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_MEASUREMENT,
            PV_NORMALIZER_TAG_PT_PER_SLASH,
            PV_NORMALIZER_TAG_PT_MEASUREMENT,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_MEASUREMENT,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_MEASUREMENT,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_MEASUREMENT,
            PV_NORMALIZER_TAG_PT_PER_SLASH,
            PV_NORMALIZER_TAG_PT_MEASUREMENT,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_MEASUREMENT,
    };
    const pv_normalizer_token_gender_t gender_tags[] = {
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_FEMININE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_FEMININE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_FEMININE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_FEMININE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
    };

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(ALL_TEST_USE_CASES), ALL_TEST_USE_CASES, &tagger);

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_gender_helper(
            normalizer_tokenizer_object,
            tagger,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            sentence_tags,
            true,
            gender_tags);

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_tag_time(void) {
    static pv_normalizer_use_cases_pt_t use_cases[] = {
            PV_NORMALIZER_USE_CARDINAL_NORMALIZER_PT,
            PV_NORMALIZER_USE_TIME_NORMALIZER_PT,
    };

    const char sentence[] = "7:00 09:18 18:23 21:89 25:09 8h45 06h15 14h30 13h66 26h21";
    const pv_normalizer_token_tag_pt_t sentence_tags[] = {
            PV_NORMALIZER_TAG_PT_TIME_HOURS,
            PV_NORMALIZER_TAG_PT_TIME_COLON,
            PV_NORMALIZER_TAG_PT_TIME_MINUTES,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_TIME_HOURS,
            PV_NORMALIZER_TAG_PT_TIME_COLON,
            PV_NORMALIZER_TAG_PT_TIME_MINUTES,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_TIME_HOURS,
            PV_NORMALIZER_TAG_PT_TIME_COLON,
            PV_NORMALIZER_TAG_PT_TIME_MINUTES,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_PUNCTUATION,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_PUNCTUATION,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_TIME_HOURS,
            PV_NORMALIZER_TAG_PT_TIME_H,
            PV_NORMALIZER_TAG_PT_TIME_MINUTES,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_TIME_HOURS,
            PV_NORMALIZER_TAG_PT_TIME_H,
            PV_NORMALIZER_TAG_PT_TIME_MINUTES,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_TIME_HOURS,
            PV_NORMALIZER_TAG_PT_TIME_H,
            PV_NORMALIZER_TAG_PT_TIME_MINUTES,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_NONE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_NONE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
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

static void test_pv_normalizer_tagger_tag_special_characters(void) {
    static pv_normalizer_use_cases_pt_t use_cases[] = {
            PV_NORMALIZER_USE_SPECIAL_CHARACTER_NORMALIZER_PT,
            PV_NORMALIZER_USE_MEASUREMENT_NORMALIZER_PT,
    };

    const char sentence[] = "hello 5% A&W @@ AºA f° °C MM³";
    const char *sentence_strings[] = {"hello", "5", "%", "A", "&", "W",
                                      "@", "@", "A", "º", "A", "f", "°", "°C", "MM³"};
    const pv_normalizer_token_tag_pt_t sentence_tags[] = {
            PV_NORMALIZER_TAG_PT_NONE,
            PV_NORMALIZER_TAG_PT_NONE,
            PV_NORMALIZER_TAG_PT_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_PT_NONE,
            PV_NORMALIZER_TAG_PT_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_PT_NONE,
            PV_NORMALIZER_TAG_PT_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_PT_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_PT_NONE,
            PV_NORMALIZER_TAG_PT_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_PT_NONE,
            PV_NORMALIZER_TAG_PT_NONE,
            PV_NORMALIZER_TAG_PT_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_PT_MEASUREMENT,
            PV_NORMALIZER_TAG_PT_MEASUREMENT,
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
    const pv_normalizer_token_tag_pt_t sentence_tags[] = {
            PV_NORMALIZER_TAG_PT_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_PT_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_PT_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_PT_SPECIAL_CHARACTER,
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

static void test_pv_normalizer_tagger_tag_invalid_measurement(void) {
    const char sentence[] = "k³ aMM² xcm²A x°C";
    const char *sentence_strings[] = {"k", "³", "a", "MM²", "xcm", "²", "A", "x", "°C"};
    const pv_normalizer_token_tag_pt_t sentence_tags[] = {
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_SPECIAL_CHARACTER,
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

static void test_pv_normalizer_tagger_tag_url(void) {
    const char *sentence = "www.example.com https://hello.xyz/";
    const pv_normalizer_token_tag_pt_t sentence_tags[] = {
            PV_NORMALIZER_TAG_PT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_PT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_PT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_PT_DOT,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_DOT,
            PV_NORMALIZER_TAG_PT_TOP_LEVEL_DOMAIN,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_PT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_PT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_PT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_PT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_PT_COLON,
            PV_NORMALIZER_TAG_PT_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_PT_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_DOT,
            PV_NORMALIZER_TAG_PT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_PT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_PT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_PT_SPECIAL_CHARACTER,
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
            "$5 €10 10$ 10€ $1,25 1,26$ -$500 -500$ -1,27$ -$1,28 €1.000 -€1.000 1.000€ -1.000€ €1.000,31 1.000,32€ "
            "$1,2 1,2$ $1,222 1USD USD1,34 1,45USD -1USD -1,55USD 1 $ -1 $ 1,65 $ -1,75 $";
    const pv_normalizer_token_tag_pt_t sentence_tags[] = {
            PV_NORMALIZER_TAG_PT_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CURRENCY,
            PV_NORMALIZER_TAG_PT_PUNCTUATION,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CURRENCY_SYMBOL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CURRENCY_SYMBOL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_DECIMAL_POINT,
            PV_NORMALIZER_TAG_PT_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CURRENCY_SYMBOL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_PT_DECIMAL_POINT,
            PV_NORMALIZER_TAG_PT_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CURRENCY_SYMBOL,
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

static void test_pv_normalizer_tagger_tag_currency_gender(void) {
    const char *sentence =
            "1£ £2 22 £ 1$ $2 -22 $ -1$ -2£ -2$ -1£ 2COP COP2 USD5 -5USD";
    const pv_normalizer_token_tag_pt_t sentence_tags[] = {
            PV_NORMALIZER_TAG_PT_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CURRENCY_SYMBOL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CURRENCY_SYMBOL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_NEGATIVE_CURRENCY,
    };
    const pv_normalizer_token_gender_t gender_tags[] = {
            PV_NORMALIZER_TOKEN_GENDER_FEMININE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_FEMININE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_FEMININE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_FEMININE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_FEMININE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_FEMININE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_FEMININE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
    };

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(ALL_TEST_USE_CASES), ALL_TEST_USE_CASES, &tagger);

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_gender_helper(
            normalizer_tokenizer_object,
            tagger,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            sentence_tags,
            true,
            gender_tags);

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_tag_dot_currency_gender(void) {
    const char *sentence =
            "1.000£ £2.123 22.000 £ 11.000$ $2.123 -22.245 $ -1.223$ -23.000£ "
            "-2.223$ -1.123£ 7.234COP COP7.000 USD5.000 -5.123USD";
    const pv_normalizer_token_tag_pt_t sentence_tags[] = {
            PV_NORMALIZER_TAG_PT_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CURRENCY_SYMBOL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CURRENCY_SYMBOL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_NEGATIVE_CURRENCY,
    };
    const pv_normalizer_token_gender_t gender_tags[] = {
            PV_NORMALIZER_TOKEN_GENDER_FEMININE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_FEMININE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_FEMININE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_FEMININE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_FEMININE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_FEMININE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_FEMININE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
            PV_NORMALIZER_TOKEN_GENDER_NONE,
            PV_NORMALIZER_TOKEN_GENDER_MASCULINE,
    };

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(ALL_TEST_USE_CASES), ALL_TEST_USE_CASES, &tagger);

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_gender_helper(
            normalizer_tokenizer_object,
            tagger,
            sentence,
            PV_ARRAY_LEN(sentence_tags),
            sentence_tags,
            true,
            gender_tags);

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_tag_abbreviations(void) {
    const char *sentence = "ex. dois tel. Sr. Av. R. Dra. S.A. Vol.";
    const pv_normalizer_token_tag_pt_t sentence_tags[] = {
            PV_NORMALIZER_TAG_PT_ABBREVIATION,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_ABBREVIATION,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_ABBREVIATION,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_ABBREVIATION,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_ABBREVIATION,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_ABBREVIATION,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_ABBREVIATION,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_ABBREVIATION,
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
    const char *sentence = "(772) 778-1923 102-291091-4920 921.212.1203 (um) 123‒456‒7890";
    const pv_normalizer_token_tag_pt_t sentence_tags[] = {
            PV_NORMALIZER_TAG_PT_DIGITS_WITH_PARENTHESES,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_DIGITS,
            PV_NORMALIZER_TAG_PT_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_PT_DIGITS,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_DIGITS,
            PV_NORMALIZER_TAG_PT_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_PT_DIGITS,
            PV_NORMALIZER_TAG_PT_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_PT_DIGITS,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_DIGITS,
            PV_NORMALIZER_TAG_PT_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_PT_DIGITS,
            PV_NORMALIZER_TAG_PT_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_PT_DIGITS,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_DIGITS,
            PV_NORMALIZER_TAG_PT_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_PT_DIGITS,
            PV_NORMALIZER_TAG_PT_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_PT_DIGITS,
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
    const pv_normalizer_token_tag_pt_t sentence_tags[] = {
            PV_NORMALIZER_TAG_PT_DATE_DAY,
            PV_NORMALIZER_TAG_PT_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_PT_DATE_MONTH,
            PV_NORMALIZER_TAG_PT_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_PT_DATE_YEAR,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_DATE_DAY,
            PV_NORMALIZER_TAG_PT_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_PT_DATE_MONTH,
            PV_NORMALIZER_TAG_PT_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_PT_DATE_YEAR,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_DATE_DAY,
            PV_NORMALIZER_TAG_PT_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_PT_DATE_MONTH,
            PV_NORMALIZER_TAG_PT_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_PT_DATE_YEAR,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_DATE_DAY,
            PV_NORMALIZER_TAG_PT_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_PT_DATE_MONTH,
            PV_NORMALIZER_TAG_PT_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_PT_DATE_YEAR,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_DATE_MONTH,
            PV_NORMALIZER_TAG_PT_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_PT_DATE_DAY,
            PV_NORMALIZER_TAG_PT_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_PT_DATE_YEAR,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_DATE_YEAR,
            PV_NORMALIZER_TAG_PT_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_PT_DATE_MONTH,
            PV_NORMALIZER_TAG_PT_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_PT_DATE_DAY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_DIGITS,
            PV_NORMALIZER_TAG_PT_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_PT_DIGITS,
            PV_NORMALIZER_TAG_PT_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_PT_DIGITS,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_DIGITS,
            PV_NORMALIZER_TAG_PT_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_PT_DIGITS,
            PV_NORMALIZER_TAG_PT_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_PT_DIGITS,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_DATE_YEAR,
            PV_NORMALIZER_TAG_PT_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_PT_DATE_MONTH,
            PV_NORMALIZER_TAG_PT_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_PT_DATE_DAY,
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

    const char *sentence_two = "3 junho 1637 03-Jun-2024 32-10-2024 1 Ago Jul 2024";
    const pv_normalizer_token_tag_pt_t sentence_tags_two[] = {
            PV_NORMALIZER_TAG_PT_DATE_DAY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_DATE_MONTH,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_DATE_YEAR,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_DATE_DAY,
            PV_NORMALIZER_TAG_PT_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_PT_DATE_MONTH,
            PV_NORMALIZER_TAG_PT_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_PT_DATE_YEAR,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_DIGITS,
            PV_NORMALIZER_TAG_PT_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_PT_DIGITS,
            PV_NORMALIZER_TAG_PT_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_PT_DIGITS,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_DATE_DAY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_DATE_MONTH,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_DATE_MONTH,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_DATE_YEAR,
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
    const char *sentence = "Este é J. P. T. Tolkien e John F. Kennedy.";
    const pv_normalizer_token_tag_pt_t sentence_tags[] = {
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_PT_NAME_INITIAL_DOT,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_PT_NAME_INITIAL_DOT,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_PT_NAME_INITIAL_DOT,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_PT_NAME_INITIAL_DOT,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_PUNCTUATION};

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

static void test_pv_normalizer_tagger_next_char_is_space(void) {
    static pv_normalizer_use_cases_pt_t use_cases[] = {PV_NORMALIZER_USE_WORD_NORMALIZER_PT};

    const char sentence[] = "4-min, carra -563  sedu-lo 2-2 a-";
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
                current->tag,
                sentence_next_character_is_space[i]);

        current = current->next;
    }
    pv_normalizer_token_list_delete(token_list);
    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_tag_cardinal_in_parentheses(void) {
    const char *sentence = "Everest (8.848 metros)? (1.101) (12.123$) (-12.123$)";
    const pv_normalizer_token_tag_pt_t sentence_tags[] = {
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_PT_PUNCTUATION,
            PV_NORMALIZER_TAG_PT_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_PT_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_PT_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_PT_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_PT_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPECIAL_CHARACTER,
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

static void test_pv_normalizer_tagger_tag_temperature_after_range(void) {
    const char *sentence = "Temperaturas entre 10-15°C. (cerca de 22°C).";
    const pv_normalizer_token_tag_pt_t sentence_tags[] = {
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_NUMBER_RANGE_TO,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_MEASUREMENT,
            PV_NORMALIZER_TAG_PT_PUNCTUATION,
            PV_NORMALIZER_TAG_PT_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_MEASUREMENT,
            PV_NORMALIZER_TAG_PT_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_PT_PUNCTUATION,
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

static void test_pv_normalizer_tagger_tag_single_quote(void) {
    const char *sentence = "Ela disse, 'como você está.' 'OLÁ%' '1.333ª'";
    const pv_normalizer_token_tag_pt_t sentence_tags[] = {
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_PUNCTUATION,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_PUNCTUATION,
            PV_NORMALIZER_TAG_PT_SINGLE_QUOTE,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_PT_SINGLE_QUOTE,
            PV_NORMALIZER_TAG_PT_SINGLE_QUOTE,
            PV_NORMALIZER_TAG_PT_ORDINAL,
            PV_NORMALIZER_TAG_PT_SINGLE_QUOTE,
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

static void test_pv_normalizer_tagger_length_future_past_context(void) {
    const char *sentence = "within 3-5 days. #AB12 www.example.com 9:30 1/2 1/1.000 12.123ª 2ml 2m/h -1.900,876";
    const pv_normalizer_token_tag_pt_t sentence_tags[] = {
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_NUMBER_RANGE_TO,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_PUNCTUATION,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_PT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_PT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_PT_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_PT_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_PT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_PT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_PT_DOT,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_DOT,
            PV_NORMALIZER_TAG_PT_TOP_LEVEL_DOMAIN,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_TIME_HOURS,
            PV_NORMALIZER_TAG_PT_TIME_COLON,
            PV_NORMALIZER_TAG_PT_TIME_MINUTES,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_FRACTION_SLASH,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_FRACTION_SLASH,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_ORDINAL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_MEASUREMENT,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CARDINAL,
            PV_NORMALIZER_TAG_PT_MEASUREMENT,
            PV_NORMALIZER_TAG_PT_PER_SLASH,
            PV_NORMALIZER_TAG_PT_MEASUREMENT,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_PT_DECIMAL_POINT,
            PV_NORMALIZER_TAG_PT_DECIMAL_DIGITS,
    };
    const char *sentence_original_strings[] = {
            "within",
            " ",
            "3-5",
            "3-5",
            "3-5",
            " ",
            "days",
            ".",
            " ",
            "#AB12",
            "#AB12",
            "#AB12",
            "#AB12",
            "#AB12",
            " ",
            "www.example.com",
            "www.example.com",
            "www.example.com",
            "www.example.com",
            "www.example.com",
            "www.example.com",
            "www.example.com",
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
            "12.123ª",
            " ",
            "2ml",
            "2ml",
            " ",
            "2m/h",
            "2m/h",
            "2m/h",
            "2m/h",
            " ",
            "-1.900,876",
            "-1.900,876",
            "-1.900,876",
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
            2,
            1,
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
            1,
            2,
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

static void test_pv_normalizer_tagger_length_future_past_context_date_digits(void) {
    const char *sentence = "12/25/2023 03-Jun-1829 (778) 239-1823 102-291-4920 123.456.789";
    const pv_normalizer_token_tag_pt_t sentence_tags[] = {
            PV_NORMALIZER_TAG_PT_DATE_MONTH,
            PV_NORMALIZER_TAG_PT_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_PT_DATE_DAY,
            PV_NORMALIZER_TAG_PT_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_PT_DATE_YEAR,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_DATE_DAY,
            PV_NORMALIZER_TAG_PT_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_PT_DATE_MONTH,
            PV_NORMALIZER_TAG_PT_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_PT_DATE_YEAR,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_DIGITS_WITH_PARENTHESES,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_DIGITS,
            PV_NORMALIZER_TAG_PT_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_PT_DIGITS,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_DIGITS,
            PV_NORMALIZER_TAG_PT_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_PT_DIGITS,
            PV_NORMALIZER_TAG_PT_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_PT_DIGITS,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_DIGITS,
            PV_NORMALIZER_TAG_PT_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_PT_DIGITS,
            PV_NORMALIZER_TAG_PT_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_PT_DIGITS,
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

static void test_pv_normalizer_tagger_length_future_past_context_merge(void) {
    const char *sentence = "$1.250,50 5.000,25$ Sr. 1.123.a S.A. Dr.A.B";
    const pv_normalizer_token_tag_pt_t sentence_tags[] = {
            PV_NORMALIZER_TAG_PT_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_CURRENCY,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_ABBREVIATION,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_ORDINAL,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_ABBREVIATION,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_ABBREVIATION,
            PV_NORMALIZER_TAG_PT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_PT_NAME_INITIAL_DOT,
            PV_NORMALIZER_TAG_PT_WORD,
    };
    const char *sentence_original_strings[] = {
            "$1.250,50",
            " ",
            "5.000,25$",
            " ",
            "Sr.",
            " ",
            "1.123.a",
            " ",
            "S.A.",
            " ",
            "Dr.A.B",
            "Dr.A.B",
            "Dr.A.B",
            "Dr.A.B",
    };
    const int32_t sentence_futures[] = {
            0,
            0,
            0,
            0,
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
    };
    const int32_t sentence_pasts[] = {
            0,
            0,
            0,
            0,
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
    const char *sentence = "diga 'ajuda' e corra";
    const pv_normalizer_token_tag_pt_t sentence_tags[] = {
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_WORD,
            PV_NORMALIZER_TAG_PT_SPACE,
            PV_NORMALIZER_TAG_PT_WORD,
    };
    const char *sentence_original_strings[] = {
            "diga", " ",
            "'ajuda'", " ",
            "e", " ",
            "corra",
    };
    const int32_t sentence_futures[7] = {0};
    const int32_t sentence_pasts[7] = {0};

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
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_tagger_pt_init, PV_STATUS_OUT_OF_MEMORY);

    static pv_normalizer_use_cases_pt_t use_cases[] = {PV_NORMALIZER_USE_WORD_NORMALIZER_PT};
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
        "`pv_normalizer_tagger_pt_init` failed with status `OUT_OF_MEMORY`.",
        true,
        "Error message mismatch"
);
}


static void test_pv_normalizer_tagger_tag_cardinal_helper_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_util_check_token_is_before_character, PV_STATUS_INVALID_ARGUMENT);

    const char sentence[] = "123";
    static pv_normalizer_use_cases_pt_t use_cases[] = {PV_NORMALIZER_USE_CARDINAL_NORMALIZER_PT};
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
    static pv_normalizer_use_cases_pt_t use_cases[] = {PV_NORMALIZER_USE_CURRENCY_NORMALIZER_PT};
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
        {"tag_invalid_use_case", test_pv_normalizer_tagger_tag_invalid_use_case},
        {"test_correct_num_tag_weights", test_pv_normalizer_tagger_correct_num_tag_weights},
        {"tag_weights_order", test_pv_normalizer_tagger_tag_weights_order},
        {"tag_word", test_pv_normalizer_tagger_tag_word},
        {"tag_hyphenated_string", test_pv_normalizer_tagger_tag_hyphenated_string},
        {"tag_invalid_hyphens_only", test_pv_normalizer_tagger_tag_invalid_hyphens_only},
        {"tag_punctuation", test_pv_normalizer_tagger_tag_punctuation},
        {"tag_custom_pronunciation", test_pv_normalizer_tagger_tag_custom_pronunciation},
        {"tag_empty_custom_pron", test_pv_normalizer_tagger_tag_empty_custom_pron},
        {"tag_cardinal", test_pv_normalizer_tagger_tag_cardinal},
        {"tag_negative_cardinal", test_pv_normalizer_tagger_tag_negative_cardinal},
        {"tag_dot_cardinal", test_pv_normalizer_tagger_tag_dot_cardinal},
        {"tag_dot_cardinal_gender", test_pv_normalizer_tagger_tag_dot_cardinal_gender},
        {"tag_ordinal", test_pv_normalizer_tagger_tag_ordinal},
        {"tag_negative_ordinal", test_pv_normalizer_tagger_tag_negative_ordinal},
        {"tag_invalid_ordinal", test_pv_normalizer_tagger_tag_invalid_ordinal},
        {"tag_number_range", test_pv_normalizer_tagger_tag_number_range},
        {"tag_dot_number_range", test_pv_normalizer_tagger_tag_dot_number_range},
        {"tag_decimal", test_pv_normalizer_tagger_tag_decimal},
        {"tag_dot_decimal", test_pv_normalizer_tagger_tag_dot_decimal},
        {"tag_alphanum_spell_out", test_pv_normalizer_tagger_tag_alphanum_spell_out},
        {"tag_fraction", test_pv_normalizer_tagger_tag_fraction},
        {"tag_measurement", test_pv_normalizer_tagger_tag_measurement},
        {"tag_per_measurement", test_pv_normalizer_tagger_tag_per_measurement},
        {"tag_measurement_gender", test_pv_normalizer_tagger_tag_measurement_gender},
        {"tag_time", test_pv_normalizer_tagger_tag_time},
        {"tag_special_characters", test_pv_normalizer_tagger_tag_special_characters},
        {"tag_negative_percent", test_pv_normalizer_tagger_tag_negative_percent},
        {"tag_invalid_measurement", test_pv_normalizer_tagger_tag_invalid_measurement},
        {"tag_url", test_pv_normalizer_tagger_tag_url},
        {"tag_currency", test_pv_normalizer_tagger_tag_currency},
        {"tag_currency_gender", test_pv_normalizer_tagger_tag_currency_gender},
        {"tag_dot_currency_gender", test_pv_normalizer_tagger_tag_dot_currency_gender},
        {"tag_abbreviations", test_pv_normalizer_tagger_tag_abbreviations},
        {"tag_digits_sequence", test_pv_normalizer_tagger_tag_digits_sequence},
        {"tag_date", test_pv_normalizer_tagger_tag_date},
        {"tag_name", test_pv_normalizer_tagger_tag_name},
        {"next_character_is_space", test_pv_normalizer_tagger_next_char_is_space},
        {"tag_cardinal_in_parentheses", test_pv_normalizer_tagger_tag_cardinal_in_parentheses},
        {"tag_temperature_after_range", test_pv_normalizer_tagger_tag_temperature_after_range},
        {"length_future_past_context", test_pv_normalizer_tagger_length_future_past_context},
        {"length_future_past_date_digits", test_pv_normalizer_tagger_length_future_past_context_date_digits},
        {"length_future_past_merge", test_pv_normalizer_tagger_length_future_past_context_merge},
        {"tag_single_quote", test_pv_normalizer_tagger_tag_single_quote},
        {"length_future_past_single_quote", test_pv_normalizer_tagger_length_future_past_context_single_quote},


#ifdef __PV_MOCKS__

        {"tagger_init_failure", test_pv_normalizer_tagger_init_failure},
        {"tagger_tag_cardinal_helper_failure", test_pv_normalizer_tagger_tag_cardinal_helper_failure},
        {"tagger_tag_currency_helper_failure", test_pv_normalizer_tagger_tag_currency_helper_failure},

#endif
};

const pv_test_suite_t PV_NORMALIZER_TAGGER_PT_TEST_SUITE = {
        .name = "normalizer_tagger_pt",
        .setup = test_pv_normalizer_tagger_setup,
        .teardown = test_pv_normalizer_tagger_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_NORMALIZER_TAGGER_TEST_CASES),
        .test_cases = PV_NORMALIZER_TAGGER_TEST_CASES,
};
