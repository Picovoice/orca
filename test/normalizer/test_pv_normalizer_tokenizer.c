#include <stdlib.h>
#include <string.h>

#include "core/pv_language.h"
#include "core/pv_language_json.h"
#include "core/pv_type.h"
#include "orca/normalizer/pv_normalizer_token.h"
#include "orca/normalizer/pv_normalizer_tokenizer.h"
#include "test/pv_test.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_normalizer_mock.h"

#endif

static pv_normalizer_tokenizer_t *normalizer_tokenizer_object = NULL;
static pv_language_info_t *language_info_object = NULL;

static const char TOKENIZER_TEST_SENTENCE[] = "123 Neural networks \"work\", sometimes! -5 -";
static const int32_t TOKENIZER_TEST_SENTENCE_NUM_TOKENS = 11;
static const int32_t TOKENIZER_TEST_SENTENCE_IS_PUNCTUATION[] = {0, 0, 0, 1, 0, 1, 1, 0, 1, 0, 0};
static const char *TOKENIZER_TEST_SENTENCE_TOKENS[] = {
        "123",
        "Neural",
        "networks",
        "\"",
        "work",
        "\"",
        ",",
        "sometimes",
        "!",
        "-5",
        "-"};
static const char *TOKENIZER_TEST_SENTENCE_PRONUNCIATIONS[11] = {NULL};
static const pv_normalizer_token_tag_t TOKENIZER_TEST_SENTENCE_TAGS[11] = {
        PV_NORMALIZER_TAG_NONE,
        PV_NORMALIZER_TAG_NONE,
        PV_NORMALIZER_TAG_NONE,
        PV_NORMALIZER_TAG_PUNCTUATION,
        PV_NORMALIZER_TAG_NONE,
        PV_NORMALIZER_TAG_PUNCTUATION,
        PV_NORMALIZER_TAG_PUNCTUATION,
        PV_NORMALIZER_TAG_NONE,
        PV_NORMALIZER_TAG_PUNCTUATION,
        PV_NORMALIZER_TAG_NONE,
        PV_NORMALIZER_TAG_NONE};
static const char *TOKENIZER_TEST_SENTENCE_VERBALIZED[11] = {NULL};
static const int32_t TOKENIZER_TEST_SENTENCE_HAS_PRONUNCIATION[11] = {0};

static const char LANGUAGE_INFO_PATH[] = "normalizer/ref/pv_language_info_normalizer_en.json";

static pv_status_t test_pv_normalizer_tokenizer_setup(void) {
    char *language_info_path = pv_test_module_res_path(LANGUAGE_INFO_PATH);
    pv_test_true(language_info_path != NULL, "Failed to get language_info_path");
    if (!language_info_path) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_status_t status = pv_language_info_load_json(
            language_info_path,
            &language_info_object,
            true,
            true);
    free(language_info_path);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "Failed to load language info from path: '%s'",
            LANGUAGE_INFO_PATH);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_normalizer_tokenizer_init(language_info_object, NULL, &normalizer_tokenizer_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_normalizer_tokenizer_teardown(void) {
    pv_normalizer_tokenizer_delete(normalizer_tokenizer_object);
    normalizer_tokenizer_object = NULL;
}

static void test_pv_normalizer_tokenizer_tokenize_helper(
        const char *sentence,
        bool preserve_word_boundary,
        bool remove_unknown_characters,
        bool split_on_special_characters,
        bool split_on_apostrophe,
        pv_normalizer_token_list_t **token_list) {
    *token_list = NULL;

    pv_normalizer_token_list_t *token_list_internal = NULL;
    pv_status_t status = pv_normalizer_tokenizer_tokenize(
            normalizer_tokenizer_object,
            sentence,
            pv_normalizer_tokenizer_default_word_boundary_character(normalizer_tokenizer_object),
            preserve_word_boundary,
            remove_unknown_characters,
            split_on_special_characters,
            split_on_apostrophe,
            &token_list_internal);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to tokenize sentence: `%s`", sentence);

    *token_list = token_list_internal;
}

static void test_pv_normalizer_tokenizer_tokenize_and_check_strings_helper(
        const char *sentence,
        int32_t target_num_tokens,
        const char **sentence_strings,
        bool preserve_word_boundary,
        bool remove_unknown_characters,
        bool split_on_special_characters,
        bool split_on_apostrophe) {
    pv_normalizer_token_list_t *token_list = NULL;
    test_pv_normalizer_tokenizer_tokenize_helper(
            sentence,
            preserve_word_boundary,
            remove_unknown_characters,
            split_on_special_characters,
            split_on_apostrophe,
            &token_list);

    pv_test_true(
            token_list->size == target_num_tokens,
            "incorrect number of tokens: got `%d`, expected `%d`",
            token_list->size,
            target_num_tokens);

    pv_normalizer_token_t *current = token_list->head;
    for (int32_t i = 0; i < target_num_tokens; i++) {
        pv_test_true(
                strcmp(current->string, sentence_strings[i]) == 0,
                "incorrect string: got `%s`, expected `%s`",
                current->string,
                sentence_strings[i]);
        current = current->next;
    }

    pv_normalizer_token_list_delete(token_list);
}

static void test_pv_normalizer_tokenizer_tokenize_and_check_original_strings_helper(
        const char *sentence,
        int32_t target_num_tokens,
        const char **sentence_original_strings,
        const int32_t *length_future_context_list,
        const int32_t *length_past_context_list,
        bool preserve_word_boundary,
        bool remove_unknown_characters,
        bool split_on_special_characters,
        bool split_on_apostrophe) {
    pv_normalizer_token_list_t *token_list = NULL;
    test_pv_normalizer_tokenizer_tokenize_helper(
            sentence,
            preserve_word_boundary,
            remove_unknown_characters,
            split_on_special_characters,
            split_on_apostrophe,
            &token_list);

    pv_test_true(
            token_list->size == target_num_tokens,
            "incorrect number of tokens: got `%d`, expected `%d`",
            token_list->size,
            target_num_tokens);

    pv_normalizer_token_t *current = token_list->head;
    for (int32_t i = 0; i < target_num_tokens; i++) {
        pv_test_true(
                strcmp(current->original_string, sentence_original_strings[i]) == 0,
                "incorrect string: got `%s`, expected `%s`",
                current->string,
                sentence_original_strings[i]);

        pv_test_true(
                current->length_future_context == length_future_context_list[i],
                "incorrect length_future_context: got `%d`, expected `%d`",
                current->length_future_context,
                length_future_context_list[i]);

        pv_test_true(
                current->length_past_context == length_past_context_list[i],
                "incorrect length_past_context: got `%d`, expected `%d`",
                current->length_past_context,
                length_past_context_list[i]);
        current = current->next;
    }

    pv_normalizer_token_list_delete(token_list);
}

static void test_pv_normalizer_tokenizer_tokenize_and_check_everything_helper(
        const char *sentence,
        int32_t target_num_tokens,
        const char **sentence_strings,
        const int32_t *sentence_is_punctuation,
        const char **sentence_pronunciation,
        const pv_normalizer_token_tag_t sentence_tag[],
        const char **sentence_verbalized,
        const int32_t *sentence_has_pronunciation,
        bool preserve_word_boundary,
        bool remove_unknown_characters,
        bool split_on_special_characters,
        bool split_on_apostrophe) {
    pv_normalizer_token_list_t *token_list = NULL;
    test_pv_normalizer_tokenizer_tokenize_helper(
            sentence,
            preserve_word_boundary,
            remove_unknown_characters,
            split_on_special_characters,
            split_on_apostrophe,
            &token_list);

    pv_test_true(
            token_list->size == target_num_tokens,
            "incorrect number of tokens: got `%d`, expected `%d`",
            token_list->size,
            target_num_tokens);

    pv_normalizer_token_t *current = token_list->head;
    for (int32_t i = 0; i < target_num_tokens; i++) {
        pv_test_true(current != NULL, "list is empty or too short");
        pv_test_true(
                strcmp(current->string, sentence_strings[i]) == 0,
                "incorrect string: got `%s`, expected `%s`",
                current->string,
                sentence_strings[i]);
        bool is_punc_condition = false;
        if (sentence_is_punctuation[i]) {
            is_punc_condition = (current->tag_language_agnostic == PV_NORMALIZER_TAG_PUNCTUATION);
        } else {
            is_punc_condition = (current->tag_language_agnostic != PV_NORMALIZER_TAG_PUNCTUATION);
        }
        pv_test_true(is_punc_condition, "incorrect is_punctuation label");
        if (sentence_pronunciation[i] != NULL) {
            pv_test_true(
                    strcmp(current->pronunciation, sentence_pronunciation[i]) == 0,
                    "incorrect pronunciation: got `%s`, expected `%s`",
                    current->pronunciation,
                    sentence_pronunciation[i]);
        } else {
            pv_test_true(
                    current->pronunciation == sentence_pronunciation[i],
                    "incorrect pronunciation: got `%s`, expected `%s`",
                    current->pronunciation,
                    sentence_pronunciation[i]);
        }
        pv_test_true(
                current->tag_language_agnostic == sentence_tag[i],
                "incorrect tag: got `%d`, expected `%d`",
                current->tag_language_agnostic,
                sentence_tag[i]);
        pv_test_true(
                current->verbalized == sentence_verbalized[i],
                "incorrect verbalized: got `%s`, expected `%s`",
                current->verbalized,
                sentence_verbalized[i]);

        bool has_pron_condition = false;
        if (sentence_has_pronunciation[i]) {
            has_pron_condition = (current->tag_language_agnostic == PV_NORMALIZER_TAG_CUSTOM_PRONUNCIATION);
        } else {
            has_pron_condition = (current->tag_language_agnostic != PV_NORMALIZER_TAG_CUSTOM_PRONUNCIATION);
        }
        pv_test_true(has_pron_condition, "incorrect has_pronunciation label");

        current = current->next;
    }
    pv_normalizer_token_list_delete(token_list);
}

static void test_pv_normalizer_tokenizer_tokenize_and_check_status_helper(
        const char *sentence,
        pv_status_t target_status,
        bool preserve_word_boundary,
        bool remove_unknown_characters,
        bool split_on_special_characters,
        const char *expected_public_message_regex,
        const char *expected_private_message_regex) {
    pv_normalizer_token_list_t *token_list = NULL;

    pv_status_t status = pv_normalizer_tokenizer_tokenize(
            normalizer_tokenizer_object,
            sentence,
            pv_normalizer_tokenizer_default_word_boundary_character(normalizer_tokenizer_object),
            preserve_word_boundary,
            remove_unknown_characters,
            split_on_special_characters,
            false,
            &token_list);
    pv_test_true(
            status == target_status,
            "raised `%s` instead of `%s`",
            pv_status_to_string(status),
            pv_status_to_string(target_status));

    if (target_status != PV_STATUS_SUCCESS) {
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

    pv_normalizer_token_list_delete(token_list);
}

static void test_pv_normalizer_tokenizer_upper(void) {
    char lower[] = "hello 43A";
    const char *upper_target[] = {"hello", "43A"};

    test_pv_normalizer_tokenizer_tokenize_and_check_strings_helper(
            lower,
            PV_ARRAY_LEN(upper_target),
            upper_target,
            false,
            false,
            false,
            false);
}

static void test_pv_normalizer_tokenizer_is_punctuation(void) {
    pv_normalizer_token_list_t *token_list = NULL;

    char punctuation[] = ". , : ! \" ? hello";

    pv_status_t status = pv_normalizer_tokenizer_tokenize(
            normalizer_tokenizer_object,
            punctuation,
            pv_normalizer_tokenizer_default_word_boundary_character(normalizer_tokenizer_object),
            false,
            false,
            false,
            false,
            &token_list);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to tokenize sentence: `%s`", punctuation);

    pv_normalizer_token_t *current = token_list->head;
    for (int32_t i = 0; i < 6; i++) {
        pv_test_true(
                current->tag_language_agnostic == PV_NORMALIZER_TAG_PUNCTUATION,
                "missed punctuation symbol: `%s`",
                punctuation[i]);
    }
    pv_test_true(
            current->tag_language_agnostic == PV_NORMALIZER_TAG_PUNCTUATION,
            "incorrectly labeled as punctuation: `%s`",
            punctuation[6]);

    pv_normalizer_token_list_delete(token_list);
}

static void test_pv_normalizer_tokenizer_tokenize(void) {
    test_pv_normalizer_tokenizer_tokenize_and_check_everything_helper(
            TOKENIZER_TEST_SENTENCE,
            TOKENIZER_TEST_SENTENCE_NUM_TOKENS,
            TOKENIZER_TEST_SENTENCE_TOKENS,
            TOKENIZER_TEST_SENTENCE_IS_PUNCTUATION,
            TOKENIZER_TEST_SENTENCE_PRONUNCIATIONS,
            TOKENIZER_TEST_SENTENCE_TAGS,
            TOKENIZER_TEST_SENTENCE_VERBALIZED,
            TOKENIZER_TEST_SENTENCE_HAS_PRONUNCIATION,
            false,
            false,
            false,
            false);
}

static void test_pv_normalizer_tokenizer_tokenize_split_apostrophe(void) {
    const char sentence[] = "The dog's leash was next to Chris' shoes.";
    const int32_t sentence_num_tokens = 12;
    const int32_t sentence_is_punctuation[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
    const int32_t sentence_has_pronunciation[12] = {0};
    const char *sentence_strings[12] = {"The", "dog", "'", "s", "leash", "was", "next", "to", "Chris", "'", "shoes", "."};
    const char *sentence_pronunciations[12] = {NULL};
    const pv_normalizer_token_tag_t sentence_tags[] = {
            PV_NORMALIZER_TAG_NONE,
            PV_NORMALIZER_TAG_NONE,
            PV_NORMALIZER_TAG_NONE,
            PV_NORMALIZER_TAG_NONE,
            PV_NORMALIZER_TAG_NONE,
            PV_NORMALIZER_TAG_NONE,
            PV_NORMALIZER_TAG_NONE,
            PV_NORMALIZER_TAG_NONE,
            PV_NORMALIZER_TAG_NONE,
            PV_NORMALIZER_TAG_NONE,
            PV_NORMALIZER_TAG_NONE,
            PV_NORMALIZER_TAG_PUNCTUATION};
    const char *sentence_verbalized[12] = {NULL};
    test_pv_normalizer_tokenizer_tokenize_and_check_everything_helper(
            sentence,
            sentence_num_tokens,
            sentence_strings,
            sentence_is_punctuation,
            sentence_pronunciations,
            sentence_tags,
            sentence_verbalized,
            sentence_has_pronunciation,
            false,
            false,
            false,
            true);
}

static void test_pv_normalizer_tokenizer_tokenize_custom_pronunciation(void) {
    const char sentence[] = "A {custom|K AH S T AH M} pronunciation";
    const int32_t sentence_num_tokens = 3;
    const int32_t sentence_is_punctuation[] = {0, 0, 0};
    const int32_t sentence_has_pronunciation[] = {0, 1, 0};
    const char *sentence_strings[] = {"A", "custom", "pronunciation"};
    const char *sentence_pronunciations[] = {NULL, "K AH S T AH M", NULL};
    const pv_normalizer_token_tag_t sentence_tags[3] = {
            PV_NORMALIZER_TAG_NONE,
            PV_NORMALIZER_TAG_CUSTOM_PRONUNCIATION,
            PV_NORMALIZER_TAG_NONE};
    const char *sentence_verbalized[3] = {NULL};

    test_pv_normalizer_tokenizer_tokenize_and_check_everything_helper(
            sentence,
            sentence_num_tokens,
            sentence_strings,
            sentence_is_punctuation,
            sentence_pronunciations,
            sentence_tags,
            sentence_verbalized,
            sentence_has_pronunciation,
            false,
            false,
            false,
            false);
}

static void test_pv_normalizer_tokenizer_invalid_custom_pronunciation(void) {
    char *sentences[] = {
            "No separator in {custom K AH S T AM} pron",
            "Wrong {{custom}|K AH S T AM} pron",
            "Wrong {{custom||K AH S T AM}} pron",
            "Wrong {custom|K AH S T AM}}| pron",
            "Wrong {custom|K AH S T AM}| pron",
            "Wrong }custom|K AH S T AM{ pron",
            "Wrong {custom| K AH S T AM} pron",
            "Wrong {custom|} pron",
            "Wrong {custom|K AH S T AM } pron",
    };

    for (int32_t i = 0; i < PV_ARRAY_LEN(sentences); i++) {
        test_pv_normalizer_tokenizer_tokenize_and_check_status_helper(
                sentences[i],
                PV_STATUS_INVALID_ARGUMENT,
                false,
                false,
                false,
                pv_test_function_hash_regex(),
                "`pv_normalizer_tokenizer_generic_tokenize` failed with status `INVALID_ARGUMENT`.");
    }
}

static void test_pv_normalizer_tokenizer_invalid_character(void) {
    char *sentences[] = {
            "^",
            "*",
            "\\",
            "~",
    };

    for (int32_t i = 0; i < PV_ARRAY_LEN(sentences); i++) {
        test_pv_normalizer_tokenizer_tokenize_and_check_status_helper(
                sentences[i],
                PV_STATUS_INVALID_ARGUMENT,
                false,
                false,
                false,
                pv_test_function_hash_regex(),
                "`pv_normalizer_tokenizer_generic_tokenize` failed with status `INVALID_ARGUMENT`.");
    }
}

static void test_pv_normalizer_tokenizer_invalid_empty_text(void) {
    char *sentences[] = {" ", "     "};
    for (int32_t i = 0; i < PV_ARRAY_LEN(sentences); i++) {
        pv_normalizer_token_list_t *token_list = NULL;
        char *sentence = sentences[i];
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
        pv_test_true(
                token_list->size == 0,
                "For sentence `%s`, got list size of `%d`, expected it to be `0`",
                sentence,
                token_list->size);
        pv_normalizer_token_list_delete(token_list);
    }
}

static void test_pv_normalizer_tokenizer_hyphen_validity(void) {
    char *sentences[] = {"-2", "hello -5454", "welcome -", "welcome-", "agent-007", "2-2"};

    for (int32_t i = 0; i < PV_ARRAY_LEN(sentences); i++) {
        test_pv_normalizer_tokenizer_tokenize_and_check_status_helper(
                sentences[i],
                PV_STATUS_SUCCESS,
                false,
                false,
                false,
                NULL,
                NULL);
    }
}

static void test_pv_normalizer_tokenizer_tokenize_hyphen_word(void) {
    const char sentence[] = "hello step-mother";
    const char *sentence_strings[] = {"hello", "step-mother"};

    test_pv_normalizer_tokenizer_tokenize_and_check_strings_helper(
            sentence,
            PV_ARRAY_LEN(sentence_strings),
            sentence_strings,
            false,
            false,
            false,
            false);
}

static void test_pv_normalizer_tokenizer_preserve_space(void) {
    char sentence[] = "hello ";
    const char *sentence_strings[] = {"hello", " "};

    test_pv_normalizer_tokenizer_tokenize_and_check_strings_helper(
            sentence,
            PV_ARRAY_LEN(sentence_strings),
            sentence_strings,
            true,
            false,
            false,
            false);
}

static void test_pv_normalizer_tokenizer_remove_unknown_chars(void) {
    pv_normalizer_token_list_t *token_list = NULL;

    char sentence[] = "字 hello \n ء\u200E ß Youʘ̅ʔ";
    const char *sentence_strings[] = {"hello", "\n", "You"};

    test_pv_normalizer_tokenizer_tokenize_and_check_strings_helper(
            sentence,
            PV_ARRAY_LEN(sentence_strings),
            sentence_strings,
            false,
            true,
            false,
            false);

    token_list = NULL;
    char sentence_two[] = "*\\>";
    pv_status_t status = pv_normalizer_tokenizer_tokenize(
            normalizer_tokenizer_object,
            sentence_two,
            pv_normalizer_tokenizer_default_word_boundary_character(normalizer_tokenizer_object),
            false,
            true,
            false,
            false,
            &token_list);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to tokenize sentence: `%s`", sentence_two);
    pv_test_true(token_list == NULL, "failed to tokenize. expected token_list to be NULL");

    token_list = NULL;
    char sentence_three[] = "";
    status = pv_normalizer_tokenizer_tokenize(
            normalizer_tokenizer_object,
            sentence_three,
            pv_normalizer_tokenizer_default_word_boundary_character(normalizer_tokenizer_object),
            false,
            true,
            false,
            false,
            &token_list);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to tokenize sentence: `%s`", sentence_three);
    pv_test_true(token_list == NULL, "failed to tokenize. expected token_list to be NULL");
}

static void test_pv_normalizer_tokenizer_tokenize_special_characters(void) {
    const char sentence[] = "a&w -5% hello@hotmail.com";
    const char *sentence_strings[] = {"a", "&", "w", "-5", "%", "hello", "@", "hotmail", ".", "com"};

    test_pv_normalizer_tokenizer_tokenize_and_check_strings_helper(
            sentence,
            PV_ARRAY_LEN(sentence_strings),
            sentence_strings,
            false,
            false,
            true,
            false);
}

static void test_pv_normalizer_tokenizer_tokenize_custom_pron_cleaned(void) {
    const char sentence[] = "Not closed {custom|K AH S T AM pron";

    test_pv_normalizer_tokenizer_tokenize_and_check_status_helper(
            sentence,
            PV_STATUS_INVALID_ARGUMENT,
            false,
            true,
            false,
            pv_test_function_hash_regex(),
            "`pv_normalizer_tokenizer_generic_tokenize` failed with status `INVALID_ARGUMENT`.");
}

static void test_pv_normalizer_tokenizer_token_list_split_verbalized(void) {
    pv_normalizer_token_t text_token_one = {
            .string = "FIFTY FIVE",
            .original_string = "55",
            .verbalized = "FIFTY FIVE",
            .tag = PV_NORMALIZER_TAG_NONE,
            .length_future_context = 1,
            .length_past_context = 0,
            .pronunciation = NULL,
            .next = NULL,
            .previous = NULL};
    int32_t expected_num_tokens = 3;

    pv_normalizer_token_list_t *token_list = NULL;
    pv_status_t status = pv_normalizer_token_list_init(&token_list);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to initialize token list");
    pv_normalizer_token_list_append_token(token_list, &text_token_one);

    pv_normalizer_token_list_t *split_token_list = NULL;
    status = pv_normalizer_tokenizer_token_list_split_verbalized(
            normalizer_tokenizer_object,
            token_list,
            &split_token_list);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to split verbalized token");
    pv_test_true(
            split_token_list->size == expected_num_tokens,
            "incorrect number of tokens: got `%d`, expected `%d`",
            split_token_list->size,
            expected_num_tokens);
    free(token_list);
    pv_normalizer_token_list_delete(split_token_list);
}

static void test_pv_normalizer_next_char_is_space_helper(
        char *sentence,
        int32_t num_tokens,
        bool *target_next_character_is_space,
        bool preserve_word_boundary,
        bool split_on_special_characters) {
    pv_normalizer_token_list_t *token_list = NULL;

    pv_status_t status = pv_normalizer_tokenizer_tokenize(
            normalizer_tokenizer_object,
            sentence,
            pv_normalizer_tokenizer_default_word_boundary_character(normalizer_tokenizer_object),
            preserve_word_boundary,
            false,
            split_on_special_characters,
            false,
            &token_list);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to tokenize sentence: `%s`", sentence);
    pv_test_true(
            token_list->size == num_tokens,
            "incorrect number of tokens: got `%d`, expected `%d`",
            token_list->size,
            num_tokens);

    pv_normalizer_token_t *current = token_list->head;
    for (int32_t i = 0; i < num_tokens; i++) {
        pv_test_true(
                current->next_character_is_space == target_next_character_is_space[i],
                "next_char_is_space should be %d but got %d for `%s`",
                target_next_character_is_space[i],
                current->next_character_is_space,
                current->string);
        current = current->next;
    }

    pv_normalizer_token_list_delete(token_list);
}


static void test_pv_normalizer_tokenizer_next_character_is_space(void) {
    char sentence[] = "the big event is @5, be there!";
    bool target_next_char_is_space[] = {true, true, true, true, false, false, true, true, false, false};
    test_pv_normalizer_next_char_is_space_helper(
            sentence,
            PV_ARRAY_LEN(target_next_char_is_space),
            target_next_char_is_space,
            false,
            true);

    char sentence2[] = "this has  spaces ";
    bool target_next_char_is_space2[] = {true, false, true, true, false, true, false};
    test_pv_normalizer_next_char_is_space_helper(
            sentence2,
            PV_ARRAY_LEN(target_next_char_is_space2),
            target_next_char_is_space2,
            true,
            false);
}

static void test_pv_normalizer_tokenizer_original_string(void) {
    char sentence[] = "1/2 1/3 1/1,000, 6:30 10/25/2024 2ml";
    const char *sentence_original_strings[] = {
        "1/2", " ", "1/3", " ",
        "1/1,000", "1/1,000", "1/1,000", ",", " ",
        "6:30", "6:30", "6:30", " ",
        "10/25/2024", " ",
        "2ml"};
    const int32_t length_future_context_list[] = {
            0, 0, 0, 0,
            2, 1, 0, 0, 0,
            2, 1, 0, 0,
            0, 0,
            0};
    const int32_t length_past_context_list[] = {
            0, 0, 0, 0,
            0, 1, 2, 0, 0,
            0, 1, 2, 0,
            0, 0,
            0};

    test_pv_normalizer_tokenizer_tokenize_and_check_original_strings_helper(
            sentence,
            PV_ARRAY_LEN(sentence_original_strings),
            sentence_original_strings,
            length_future_context_list,
            length_past_context_list,
            true,
            false,
            false,
            false);

    const char *sentence_original_strings_split_sp[] = {
        "1/2", "1/2", "1/2", " ", "1/3", "1/3", "1/3", " ",
        "1/1,000", "1/1,000", "1/1,000", "1/1,000", "1/1,000", ",", " ",
        "6:30", "6:30", "6:30", " ",
        "10/25/2024", "10/25/2024", "10/25/2024", "10/25/2024", "10/25/2024", " ",
        "2ml"};
    const int32_t length_future_context_list2[] = {
            2, 1, 0, 0, 2, 1, 0, 0,
            4, 3, 2, 1, 0, 0, 0,
            2, 1, 0, 0,
            4, 3, 2, 1, 0, 0,
            0};
    const int32_t length_past_context_list2[] = {
            0, 1, 2, 0, 0, 1, 2, 0,
            0, 1, 2, 3, 4, 0, 0,
            0, 1, 2, 0,
            0, 1, 2, 3, 4, 0,
            0};

    test_pv_normalizer_tokenizer_tokenize_and_check_original_strings_helper(
            sentence,
            PV_ARRAY_LEN(sentence_original_strings_split_sp),
            sentence_original_strings_split_sp,
            length_future_context_list2,
            length_past_context_list2,
            true,
            false,
            true,
            false);
}


#ifdef __PV_MOCKS__

static void test_pv_normalizer_tokenizer_token_list_split_verbalized_failure_helper(
        pv_status_t expected_status,
        bool pv_normalizer_token_list_init_failure,
        const char *expected_public_message_regex,
        const char *expected_private_message_regex) {
    pv_normalizer_token_t text_token_one = {
            .string = "street",
            .original_string = "street",
            .verbalized = "{S T R IY T}",
            .tag_language_agnostic = PV_NORMALIZER_TAG_CUSTOM_PRONUNCIATION,
            .length_future_context = 0,
            .length_past_context = 0,
            .pronunciation = "S",
            .next = NULL,
            .previous = NULL};
    pv_normalizer_token_t text_token_two = {
            .string = "FIFTY FIVE",
            .original_string = "55",
            .verbalized = "FIFTY FIVE",
            .tag_language_agnostic = PV_NORMALIZER_TAG_NONE,
            .length_future_context = 0,
            .length_past_context = 0,
            .pronunciation = NULL,
            .next = NULL,
            .previous = NULL};
    pv_normalizer_token_t text_token_three = {
            .string = "here",
            .original_string = "here",
            .verbalized = "HERE",
            .tag_language_agnostic = PV_NORMALIZER_TAG_NONE,
            .length_future_context = 0,
            .length_past_context = 0,
            .pronunciation = NULL,
            .next = NULL,
            .previous = NULL};

    pv_normalizer_token_list_t *token_list = NULL;
    pv_status_t status = pv_normalizer_token_list_init(&token_list);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to initialize token list");
    pv_normalizer_token_list_append_token(token_list, &text_token_one);
    pv_normalizer_token_list_append_token(token_list, &text_token_two);
    pv_normalizer_token_list_append_token(token_list, &text_token_three);

    if (pv_normalizer_token_list_init_failure) {
        PV_SET_MOCK_RETURN_VAL(pv_normalizer_token_list_init, PV_STATUS_OUT_OF_MEMORY)
    }

    pv_normalizer_token_list_t *split_token_list = NULL;
    status = pv_normalizer_tokenizer_token_list_split_verbalized(
            normalizer_tokenizer_object,
            token_list,
            &split_token_list);
    pv_test_true(
            status == expected_status,
            "failed to fail when splitting verbalized token. Got status = `%s`, expected `%s`",
            pv_status_to_string(status),
            pv_status_to_string(expected_status));

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

static void test_pv_normalizer_tokenizer_token_list_split_verbalized_failure_1(void) {
    test_pv_normalizer_tokenizer_token_list_split_verbalized_failure_helper(
            PV_STATUS_OUT_OF_MEMORY,
            true,
            pv_test_function_hash_regex(),
            "`pv_normalizer_token_list_init` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_normalizer_tokenizer_token_list_split_verbalized_failure_2(void) {
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_token_copy, PV_STATUS_INVALID_ARGUMENT)

    test_pv_normalizer_tokenizer_token_list_split_verbalized_failure_helper(
            PV_STATUS_INVALID_ARGUMENT,
            false,
            pv_test_function_hash_regex(),
            "`pv_normalizer_token_copy` failed with status `INVALID_ARGUMENT`\\.");
}

static pv_status_t pv_normalizer_token_init_return_oom(
        const int32_t arg0,
        const int32_t arg1,
        const char *arg2,
        bool arg3,
        bool arg4,
        bool arg5,
        pv_normalizer_token_t **arg6) {
    (void) arg0;
    (void) arg1;
    (void) arg2;
    (void) arg3;
    (void) arg4;
    (void) arg5;
    (void) arg6;

    return PV_STATUS_OUT_OF_MEMORY;
}

static pv_status_t pv_normalizer_token_init_with_original_string_return_oom(
        char *arg0,
        char *arg1,
        bool arg2,
        bool arg3,
        int32_t arg4,
        int32_t arg5,
        pv_normalizer_token_t **arg6) {
    (void) arg0;
    (void) arg1;
    (void) arg2;
    (void) arg3;
    (void) arg4;
    (void) arg5;
    (void) arg6;

    return PV_STATUS_OUT_OF_MEMORY;
}

static void test_pv_normalizer_tokenizer_tokenize_failure_helper(
        pv_status_t expected_status,
        const char *expected_public_message_regex,
        const char *expected_private_message_regex) {
    const char sentence[] = "H . ";
    pv_normalizer_token_list_t *token_list = NULL;
    pv_status_t status = pv_normalizer_tokenizer_tokenize(
            normalizer_tokenizer_object,
            sentence,
            pv_normalizer_tokenizer_default_word_boundary_character(normalizer_tokenizer_object),
            true,
            true,
            false,
            false,
            &token_list);
    pv_test_true(
            status == expected_status,
            "failed to fail with `%s` error, got status `%s`",
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

static void test_pv_normalizer_tokenizer_tokenize_failure_1(void) {
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_util_validate_text, PV_STATUS_OUT_OF_MEMORY)

    test_pv_normalizer_tokenizer_tokenize_failure_helper(
            PV_STATUS_OUT_OF_MEMORY,
            pv_test_function_hash_regex(),
            "`pv_normalizer_util_validate_text` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_normalizer_tokenizer_tokenize_failure_2(void) {
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_token_list_init, PV_STATUS_OUT_OF_MEMORY)

    test_pv_normalizer_tokenizer_tokenize_failure_helper(
            PV_STATUS_OUT_OF_MEMORY,
            pv_test_function_hash_regex(),
            "`pv_normalizer_token_list_init` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_normalizer_tokenizer_tokenize_failure_3(void) {
    pv_status_t (*custom_funcs[])(
            const int32_t arg0,
            const int32_t arg1,
            const char *arg2,
            bool arg3,
            bool arg4,
            bool arg5,
            pv_normalizer_token_t **arg6) = {
            pv_normalizer_token_init_return_oom,

};
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(pv_normalizer_token_init, custom_funcs)

    test_pv_normalizer_tokenizer_tokenize_failure_helper(
            PV_STATUS_OUT_OF_MEMORY,
            pv_test_function_hash_regex(),
            "`pv_normalizer_token_init` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_normalizer_tokenizer_tokenize_failure_4(void) {
    pv_status_t (*custom_funcs[])(
            char *arg0,
            char *arg1,
            bool arg2,
            bool arg3,
            int32_t arg4,
            int32_t arg5,
            pv_normalizer_token_t **arg6) = {
            pv_normalizer_token_init_with_original_string_return_oom,

    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(pv_normalizer_token_init_with_original_string, custom_funcs)

    test_pv_normalizer_tokenizer_tokenize_failure_helper(
            PV_STATUS_OUT_OF_MEMORY,
            pv_test_function_hash_regex(),
            "`pv_normalizer_token_init_with_original_string` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_normalizer_tokenizer_tokenize_failure_5(void) {
    pv_status_t (*custom_funcs[])(
            char *arg0,
            char *arg1,
            bool arg2,
            bool arg3,
            int32_t arg4,
            int32_t arg5,
            pv_normalizer_token_t **arg6) = {
            pv_normalizer_token_init_with_original_string,
            pv_normalizer_token_init_with_original_string_return_oom,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(pv_normalizer_token_init_with_original_string, custom_funcs)

    test_pv_normalizer_tokenizer_tokenize_failure_helper(
            PV_STATUS_OUT_OF_MEMORY,
            pv_test_function_hash_regex(),
            "`pv_normalizer_token_init_with_original_string` failed with status `OUT_OF_MEMORY`\\.");
}

#endif

static const pv_test_case_t PV_NORMALIZER_TOKENIZER_TEST_CASES[] = {
        {"upper", test_pv_normalizer_tokenizer_upper},
        {"is_punctuation", test_pv_normalizer_tokenizer_is_punctuation},
        {"tokenize", test_pv_normalizer_tokenizer_tokenize},
        {"tokenize_custom_pronunciation", test_pv_normalizer_tokenizer_tokenize_custom_pronunciation},
        {"invalid_custom_pronunciation", test_pv_normalizer_tokenizer_invalid_custom_pronunciation},
        {"invalid_character", test_pv_normalizer_tokenizer_invalid_character},
        {"invalid_empty_text", test_pv_normalizer_tokenizer_invalid_empty_text},
        {"hyphen_validity", test_pv_normalizer_tokenizer_hyphen_validity},
        {"hyphen_word", test_pv_normalizer_tokenizer_tokenize_hyphen_word},
        {"preserve_space", test_pv_normalizer_tokenizer_preserve_space},
        {"remove_unknown_chars", test_pv_normalizer_tokenizer_remove_unknown_chars},
        {"tokenize_special_characters", test_pv_normalizer_tokenizer_tokenize_special_characters},
        {"tokenize_custom_pron_cleaned", test_pv_normalizer_tokenizer_tokenize_custom_pron_cleaned},
        {"split_token_list_verbalized", test_pv_normalizer_tokenizer_token_list_split_verbalized},
        {"next_character_is_space", test_pv_normalizer_tokenizer_next_character_is_space},
        {"original_string", test_pv_normalizer_tokenizer_original_string},
        {"split_apostrophe", test_pv_normalizer_tokenizer_tokenize_split_apostrophe},

#ifdef __PV_MOCKS__

        {"split_token_list_verbalized_failure_1", test_pv_normalizer_tokenizer_token_list_split_verbalized_failure_1},
        {"split_token_list_verbalized_failure_2", test_pv_normalizer_tokenizer_token_list_split_verbalized_failure_2},

        {"tokenize_failure_1", test_pv_normalizer_tokenizer_tokenize_failure_1},
        {"tokenize_failure_2", test_pv_normalizer_tokenizer_tokenize_failure_2},
        {"tokenize_failure_3", test_pv_normalizer_tokenizer_tokenize_failure_3},
        {"tokenize_failure_4", test_pv_normalizer_tokenizer_tokenize_failure_4},
        {"tokenize_failure_5", test_pv_normalizer_tokenizer_tokenize_failure_5},

#endif

};

const pv_test_suite_t PV_NORMALIZER_TOKENIZER_TEST_SUITE = {
        .name = "normalizer_tokenizer",
        .setup = test_pv_normalizer_tokenizer_setup,
        .teardown = test_pv_normalizer_tokenizer_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_NORMALIZER_TOKENIZER_TEST_CASES),
        .test_cases = PV_NORMALIZER_TOKENIZER_TEST_CASES,
};
