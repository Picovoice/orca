#include <stdlib.h>
#include <string.h>

#include "core/pv_error_messages.h"
#include "core/pv_language.h"
#include "core/pv_language_json.h"
#include "orca/normalizer/it/pv_normalizer_use_cases_it.h"
#include "orca/normalizer/pv_normalizer_tagger.h"
#include "orca/normalizer/pv_normalizer_tokenizer.h"
#include "orca/normalizer/pv_normalizer_use_cases.h"
#include "test/pv_test.h"

#include "orca/normalizer/it/pv_normalizer_tags_it.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_normalizer_mock.h"

#endif

static pv_normalizer_tagger_t *normalizer_tagger_object = NULL;
static pv_normalizer_use_cases_it_t ALL_TEST_USE_CASES[] = {
        PV_NORMALIZER_USE_WORD_NORMALIZER_IT,
        PV_NORMALIZER_USE_CUSTOM_PRONUNCIATION_NORMALIZER_IT,
        PV_NORMALIZER_USE_PUNCTUATION_NORMALIZER_IT,
        PV_NORMALIZER_USE_CARDINAL_NORMALIZER_IT,
        PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_IT,
        PV_NORMALIZER_USE_NUMBER_RANGE_NORMALIZER_IT,
        PV_NORMALIZER_USE_ORDINAL_NORMALIZER_IT,
        PV_NORMALIZER_USE_NEGATIVE_ORDINAL_NORMALIZER_IT,
        PV_NORMALIZER_USE_SPECIAL_CHARACTER_NORMALIZER_IT,
        PV_NORMALIZER_USE_DECIMAL_NORMALIZER_IT,
        PV_NORMALIZER_USE_MEASUREMENT_NORMALIZER_IT,
        PV_NORMALIZER_USE_ALPHANUM_SPELL_OUT_NORMALIZER_IT,
        PV_NORMALIZER_USE_TIME_NORMALIZER_IT,
        PV_NORMALIZER_USE_FRACTION_NORMALIZER_IT,
        PV_NORMALIZER_USE_URL_NORMALIZER_IT,
        PV_NORMALIZER_USE_CURRENCY_NORMALIZER_IT,
        PV_NORMALIZER_USE_ABBREVIATION_NORMALIZER_IT,
        PV_NORMALIZER_USE_DIGITS_SEQUENCE_NORMALIZER_IT,
        PV_NORMALIZER_USE_DATE_NORMALIZER_IT,
        PV_NORMALIZER_USE_NAME_NORMALIZER_IT,
};

static pv_normalizer_tokenizer_t *normalizer_tokenizer_object = NULL;
static pv_language_info_t *language_info_object = NULL;
static pv_noun_gender_dict_t *noun_gender_dict_object = NULL;

static const char LANGUAGE_INFO_PATH[] = "normalizer/ref/pv_language_info_normalizer_it.json";
static const char NOUN_GENDER_DICT_PATH[] = "orca/test_data/noun_gender_dict/noun_gender_dict_it.txt";

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
    noun_gender_dict_object = NULL;

    pv_normalizer_tagger_delete(normalizer_tagger_object);
    normalizer_tagger_object = NULL;

    pv_normalizer_tokenizer_delete(normalizer_tokenizer_object);
    normalizer_tokenizer_object = NULL;
}

static void test_pv_normalizer_tagger_init_helper(
        int32_t num_use_cases,
        const pv_normalizer_use_cases_it_t *use_cases,
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
        const pv_normalizer_token_tag_it_t *target_tags) {
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
        const pv_normalizer_token_tag_it_t *target_tags,
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
                "incorrect tag for `%s`: got `%d`, expected `%d`",
                current->string,
                current->tag,
                target_tags[i]);

        current = current->next;
    }

    pv_normalizer_token_list_delete(token_list);
}

static void test_pv_normalizer_tagger_tag_word(void) {
    static pv_normalizer_use_cases_it_t use_cases[] = {PV_NORMALIZER_USE_WORD_NORMALIZER_IT};

    const char sentence[] = "La città di Roma è piena di storia e cultura";
    const pv_normalizer_token_tag_it_t sentence_tags[] = {
            PV_NORMALIZER_TAG_IT_WORD,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_WORD,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_WORD,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_WORD,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_WORD,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_WORD,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_WORD,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_WORD,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_WORD,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_WORD,
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
    static pv_normalizer_use_cases_it_t use_cases[] = {PV_NORMALIZER_USE_CUSTOM_PRONUNCIATION_NORMALIZER_IT};

    const char sentence[] = "Una pronuncia {personalizzata|a b d ddʒ dz dʒ dʣ e f g i j k l m n o p r rr s t ts tts tʃ tʧ u v w z ŋ ŋg ŋk ɔ ɛ ɲ ʃ ʎ}";
    const pv_normalizer_token_tag_it_t sentence_tags[] = {
            PV_NORMALIZER_TAG_IT_NONE,
            PV_NORMALIZER_TAG_IT_NONE,
            PV_NORMALIZER_TAG_IT_CUSTOM_PRONUNCIATION,
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

static void test_pv_normalizer_tagger_tag_punctuation(void) {
    static pv_normalizer_use_cases_it_t use_cases[] = {PV_NORMALIZER_USE_PUNCTUATION_NORMALIZER_IT};

    const char sentence[] = "Sto prendendo vitamina B-12, C, e altre 15!";
    // Expecting: <Sto> <Prendendo> <vitamina> <B> <12> <,> <C> <,> <e> <altre> <15> <!>.
    const pv_normalizer_token_tag_it_t sentence_tags[] = {
            PV_NORMALIZER_TAG_IT_NONE,
            PV_NORMALIZER_TAG_IT_NONE,
            PV_NORMALIZER_TAG_IT_NONE,
            PV_NORMALIZER_TAG_IT_NONE,
            PV_NORMALIZER_TAG_IT_NONE,
            PV_NORMALIZER_TAG_IT_PUNCTUATION,
            PV_NORMALIZER_TAG_IT_NONE,
            PV_NORMALIZER_TAG_IT_PUNCTUATION,
            PV_NORMALIZER_TAG_IT_NONE,
            PV_NORMALIZER_TAG_IT_NONE,
            PV_NORMALIZER_TAG_IT_NONE,
            PV_NORMALIZER_TAG_IT_PUNCTUATION};

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

static void test_pv_normalizer_tagger_tag_special_characters(void) {
    static pv_normalizer_use_cases_it_t use_cases[] = {PV_NORMALIZER_USE_SPECIAL_CHARACTER_NORMALIZER_IT};

    const char sentence[] = "ciao 5% A&W @@ °C RPM (°C)";
    const char *sentence_strings[] = {"ciao", "5", "%", "A", "&", "W", "@", "@", "°C", "RPM", "(", "°C", ")"};
    const pv_normalizer_token_tag_it_t sentence_tags[] = {
            PV_NORMALIZER_TAG_IT_NONE,
            PV_NORMALIZER_TAG_IT_NONE,
            PV_NORMALIZER_TAG_IT_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_IT_NONE,
            PV_NORMALIZER_TAG_IT_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_IT_NONE,
            PV_NORMALIZER_TAG_IT_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_IT_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_IT_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_IT_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_IT_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_IT_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_IT_SPECIAL_CHARACTER,
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

static void test_pv_normalizer_tagger_tag_cardinal(void) {
    static pv_normalizer_use_cases_it_t use_cases[] = {PV_NORMALIZER_USE_CARDINAL_NORMALIZER_IT};

    const char sentence[] = "Sto prendendo vitamina B-12, C, e altre 15!";
    const pv_normalizer_token_tag_it_t sentence_tags[] = {
            PV_NORMALIZER_TAG_IT_NONE,
            PV_NORMALIZER_TAG_IT_NONE,
            PV_NORMALIZER_TAG_IT_NONE,
            PV_NORMALIZER_TAG_IT_NONE,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_PUNCTUATION,
            PV_NORMALIZER_TAG_IT_NONE,
            PV_NORMALIZER_TAG_IT_PUNCTUATION,
            PV_NORMALIZER_TAG_IT_NONE,
            PV_NORMALIZER_TAG_IT_NONE,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_PUNCTUATION,
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

static void test_pv_normalizer_tagger_tag_negative_cardinal(void) {
    static pv_normalizer_use_cases_it_t use_cases[] = {PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_IT};

    const char sentence[] = "123 -563 −123";
    const pv_normalizer_token_tag_it_t sentence_tags[] = {
            PV_NORMALIZER_TAG_IT_NONE,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL,
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

static void test_pv_normalizer_tagger_tag_number_range(void) {
    const char sentence[] = "12-5 50-50! 123–12 1.000-2.000 1.000–2.000";
    const pv_normalizer_token_tag_it_t sentence_tags[] = {
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_NUMBER_RANGE_TO,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_NUMBER_RANGE_TO,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_PUNCTUATION,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_NUMBER_RANGE_TO,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_NUMBER_RANGE_TO,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_NUMBER_RANGE_TO,
            PV_NORMALIZER_TAG_IT_CARDINAL,
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

static void test_pv_normalizer_tagger_tag_decimal(void) {
    const char sentence[] = ",1 1,2 ,456 1,,2 a,6 ,5% -1,1";
    const pv_normalizer_token_tag_it_t sentence_tags[] = {
            PV_NORMALIZER_TAG_IT_DECIMAL_COMMA,
            PV_NORMALIZER_TAG_IT_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL, // In Italian language, for the "1" before comma in decimal is always verbalized to "UNO", so fallback to the default cardinal case.
            PV_NORMALIZER_TAG_IT_DECIMAL_COMMA,
            PV_NORMALIZER_TAG_IT_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_DECIMAL_COMMA,
            PV_NORMALIZER_TAG_IT_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL_ONE,
            PV_NORMALIZER_TAG_IT_PUNCTUATION,
            PV_NORMALIZER_TAG_IT_PUNCTUATION,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_WORD,
            PV_NORMALIZER_TAG_IT_PUNCTUATION,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_DECIMAL_COMMA,
            PV_NORMALIZER_TAG_IT_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_IT_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_IT_DECIMAL_COMMA,
            PV_NORMALIZER_TAG_IT_DECIMAL_DIGITS,
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
    const pv_normalizer_token_tag_it_t sentence_tags2[] = {
            PV_NORMALIZER_TAG_IT_PUNCTUATION,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL};

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
    const char sentence[] = "HTML5 A87B 12A 12 400AD";
    const pv_normalizer_token_tag_it_t sentence_tags[] = {
            PV_NORMALIZER_TAG_IT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_IT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_IT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_IT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_IT_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_IT_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_IT_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_IT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_IT_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_IT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_IT_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_IT_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_IT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_IT_LETTER_SPELL_OUT,
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
    const char *sentence = "1/2 1/3,2 1/1.000 1/-4 4/1 -1/2 -1/3,2 -1/1.000 -1/-4 -4/1";
    const pv_normalizer_token_tag_it_t sentence_tags[] = {
            PV_NORMALIZER_TAG_IT_CARDINAL_ONE,
            PV_NORMALIZER_TAG_IT_FRACTION_SLASH,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL_ONE,
            PV_NORMALIZER_TAG_IT_FRACTION_SLASH,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_DECIMAL_COMMA,
            PV_NORMALIZER_TAG_IT_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL_ONE,
            PV_NORMALIZER_TAG_IT_FRACTION_SLASH,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL_ONE,
            PV_NORMALIZER_TAG_IT_FRACTION_SLASH,
            PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_FRACTION_SLASH,
            PV_NORMALIZER_TAG_IT_CARDINAL, // Denominator should be retagged from special "1" or "-1" back to ordinary "1" or "-1", because in verbalizer, Italian denominator don't need context. If we don't do this, then it could be double-pronounced!
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL_ONE,
            PV_NORMALIZER_TAG_IT_FRACTION_SLASH,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL_ONE, // Don't want to look forward 4 tokens ahead in tagger.
            PV_NORMALIZER_TAG_IT_FRACTION_SLASH,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_DECIMAL_COMMA,
            PV_NORMALIZER_TAG_IT_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL_ONE,
            PV_NORMALIZER_TAG_IT_FRACTION_SLASH,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL_ONE,
            PV_NORMALIZER_TAG_IT_FRACTION_SLASH,
            PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_IT_FRACTION_SLASH,
            PV_NORMALIZER_TAG_IT_CARDINAL, // Denominator should be retagged from special "1" or "-1" back to ordinary "1" or "-1", because in verbalizer, Italian denominator don't need context. If we don't do this, then it could be double-pronounced!
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
    // Decided to keep the current implementation as opposed to verbalizing verbatim. Will rely on Hippo to deal with the concatenated words.
    const char *sentence = "www.example.com https://hello.xyz/";
    const pv_normalizer_token_tag_it_t sentence_tags[] = {
            PV_NORMALIZER_TAG_IT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_IT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_IT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_IT_DOT,
            PV_NORMALIZER_TAG_IT_WORD,
            PV_NORMALIZER_TAG_IT_DOT,
            PV_NORMALIZER_TAG_IT_TOP_LEVEL_DOMAIN,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_IT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_IT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_IT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_IT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_IT_COLON,
            PV_NORMALIZER_TAG_IT_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_IT_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_IT_WORD,
            PV_NORMALIZER_TAG_IT_DOT,
            PV_NORMALIZER_TAG_IT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_IT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_IT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_IT_SPECIAL_CHARACTER,
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
    // Unlike English, in Italian, we don't use "." as digit separator!
    const char *sentence = "(778) 239-1823 102-291091-4920 921.212.1203 123‒456‒7890";
    const pv_normalizer_token_tag_it_t sentence_tags[] = {
            PV_NORMALIZER_TAG_IT_DIGITS_WITH_PARENTHESES,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_DIGITS,
            PV_NORMALIZER_TAG_IT_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_IT_DIGITS,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_DIGITS,
            PV_NORMALIZER_TAG_IT_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_IT_DIGITS,
            PV_NORMALIZER_TAG_IT_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_IT_DIGITS,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_DOT,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_DOT,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_DIGITS,
            PV_NORMALIZER_TAG_IT_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_IT_DIGITS,
            PV_NORMALIZER_TAG_IT_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_IT_DIGITS,
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
    const char *sentence = "Sig. Ted Sig.ra Ted Sig.na Ted Sigg. Dr. Ted PROF. Ted PROF.ssa Ted";
    const pv_normalizer_token_tag_it_t sentence_tags[] = {
            PV_NORMALIZER_TAG_IT_ABBREVIATION,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_WORD,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_ABBREVIATION,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_WORD,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_ABBREVIATION,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_WORD,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_ABBREVIATION,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_ABBREVIATION,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_WORD,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_ABBREVIATION,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_WORD,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_ABBREVIATION,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_WORD,
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
    const char *sentence = "06-13-2020 31-02-1991 06/03/2020 6/3/2020 1991/03/13 1991/13/13 07/13/182 2024-05-06 03-gennaio-2024 gennaio 2024";
    const pv_normalizer_token_tag_it_t sentence_tags[] = {
            PV_NORMALIZER_TAG_IT_DATE_MONTH,
            PV_NORMALIZER_TAG_IT_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_IT_DATE_DAY,
            PV_NORMALIZER_TAG_IT_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_IT_DATE_YEAR,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_DATE_DAY,
            PV_NORMALIZER_TAG_IT_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_IT_DATE_MONTH,
            PV_NORMALIZER_TAG_IT_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_IT_DATE_YEAR,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_DATE_DAY,
            PV_NORMALIZER_TAG_IT_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_IT_DATE_MONTH,
            PV_NORMALIZER_TAG_IT_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_IT_DATE_YEAR,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_DATE_DAY,
            PV_NORMALIZER_TAG_IT_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_IT_DATE_MONTH,
            PV_NORMALIZER_TAG_IT_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_IT_DATE_YEAR,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_DATE_YEAR,
            PV_NORMALIZER_TAG_IT_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_IT_DATE_MONTH,
            PV_NORMALIZER_TAG_IT_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_IT_DATE_DAY,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_DIGITS,
            PV_NORMALIZER_TAG_IT_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_IT_DIGITS,
            PV_NORMALIZER_TAG_IT_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_IT_DIGITS,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_DIGITS,
            PV_NORMALIZER_TAG_IT_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_IT_DIGITS,
            PV_NORMALIZER_TAG_IT_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_IT_DIGITS,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_DATE_YEAR,
            PV_NORMALIZER_TAG_IT_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_IT_DATE_MONTH,
            PV_NORMALIZER_TAG_IT_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_IT_DATE_DAY,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_DATE_DAY,
            PV_NORMALIZER_TAG_IT_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_IT_DATE_MONTH,
            PV_NORMALIZER_TAG_IT_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_IT_DATE_YEAR,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_DATE_MONTH,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL,
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

    const char *sentence_two = "giugno 3, 2020 settembre 8, 1991 03-giugno-2024 32-10-2024 agosto 1";
    const pv_normalizer_token_tag_it_t sentence_tags_two[] = {
            PV_NORMALIZER_TAG_IT_DATE_MONTH,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_DATE_DAY,
            PV_NORMALIZER_TAG_IT_PUNCTUATION,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_DATE_YEAR,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_DATE_MONTH,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_DATE_DAY,
            PV_NORMALIZER_TAG_IT_PUNCTUATION,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_DATE_YEAR,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_DATE_DAY,
            PV_NORMALIZER_TAG_IT_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_IT_DATE_MONTH,
            PV_NORMALIZER_TAG_IT_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_IT_DATE_YEAR,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_DIGITS,
            PV_NORMALIZER_TAG_IT_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_IT_DIGITS,
            PV_NORMALIZER_TAG_IT_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_IT_DIGITS,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_DATE_MONTH,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_DATE_DAY,
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

static void test_pv_normalizer_tagger_tag_time(void) {
    const char sentence[] = "0:00 00:15 7:00 09:18 18:23 9 21:89 90 25:09 8";
    const pv_normalizer_token_tag_it_t sentence_tags[] = {
            PV_NORMALIZER_TAG_IT_TIME_HOURS,
            PV_NORMALIZER_TAG_IT_TIME_COLON,
            PV_NORMALIZER_TAG_IT_TIME_MINUTES,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_TIME_HOURS,
            PV_NORMALIZER_TAG_IT_TIME_COLON,
            PV_NORMALIZER_TAG_IT_TIME_MINUTES,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_TIME_HOURS,
            PV_NORMALIZER_TAG_IT_TIME_COLON,
            PV_NORMALIZER_TAG_IT_TIME_MINUTES,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_TIME_HOURS,
            PV_NORMALIZER_TAG_IT_TIME_COLON,
            PV_NORMALIZER_TAG_IT_TIME_MINUTES,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_TIME_HOURS,
            PV_NORMALIZER_TAG_IT_TIME_COLON,
            PV_NORMALIZER_TAG_IT_TIME_MINUTES,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_COLON,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_COLON,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL,
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
            "$ $5 $ €10 10$ 10€ $1,25 1,25$ -$500 -500$ -1,25$ -$1,25 €1.000 -€1.000 1.000€ -1.000€ €1.000,25 1.000,25€ "
            "1,2$ $1,222 1USD USD1,25 1,25USD -1USD -1,25USD 1 $ -1 $ 1,25 $ -1,25 $ 1 USD -1 USD $ $";

    const pv_normalizer_token_tag_it_t sentence_tags[] = {
            PV_NORMALIZER_TAG_IT_CURRENCY_SYMBOL,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CURRENCY,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CURRENCY_SYMBOL,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CURRENCY,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CURRENCY,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CURRENCY,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CURRENCY,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CURRENCY,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CURRENCY,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CURRENCY,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CURRENCY,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CURRENCY,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL_ONE,
            PV_NORMALIZER_TAG_IT_PUNCTUATION,
            PV_NORMALIZER_TAG_IT_NONE, // TODO: Decide if this not an issue? For all normalizer this occurs!
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CURRENCY,
            PV_NORMALIZER_TAG_IT_PUNCTUATION,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CURRENCY,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CURRENCY,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CURRENCY,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_NEGATIVE_CURRENCY,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL_ONE,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CURRENCY_SYMBOL,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL_ONE,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CURRENCY_SYMBOL,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_DECIMAL_COMMA,
            PV_NORMALIZER_TAG_IT_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CURRENCY_SYMBOL,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_IT_DECIMAL_COMMA,
            PV_NORMALIZER_TAG_IT_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CURRENCY_SYMBOL,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL_ONE,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CURRENCY_SYMBOL,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL_ONE,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CURRENCY_SYMBOL,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CURRENCY_SYMBOL,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CURRENCY_SYMBOL,
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
    const char sentence[] = "5 g 1ml -7,3l ciao ml 10,1km 5°C 1°C 1 °C -1°C -1 °C 1.000g 1.000.111g 25ft.-lb. contesto chiaro ml-g";
    const pv_normalizer_token_tag_it_t sentence_tags[] = {
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_MEASUREMENT,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL_ONE,
            PV_NORMALIZER_TAG_IT_MEASUREMENT,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_IT_DECIMAL_COMMA,
            PV_NORMALIZER_TAG_IT_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_IT_MEASUREMENT,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_WORD,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_MEASUREMENT,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_DECIMAL_COMMA,
            PV_NORMALIZER_TAG_IT_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_IT_MEASUREMENT,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_MEASUREMENT,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL_ONE,
            PV_NORMALIZER_TAG_IT_MEASUREMENT,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL_ONE,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_MEASUREMENT,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL_ONE,
            PV_NORMALIZER_TAG_IT_MEASUREMENT,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL_ONE,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_MEASUREMENT,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_MEASUREMENT,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_MEASUREMENT,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_MEASUREMENT,
            PV_NORMALIZER_TAG_IT_DOT,
            PV_NORMALIZER_TAG_IT_MEASUREMENT,
            PV_NORMALIZER_TAG_IT_PUNCTUATION,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_WORD,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_WORD,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_MEASUREMENT,
            PV_NORMALIZER_TAG_IT_MEASUREMENT,
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
    const char sentence[] = "5 km/m 10 oz/km 1,1m/l -5,41kg/ft m/s 7 miglia/ora 1 miglio/ora "
                            "7 miglia/ore 1 miglio/ore -1.000.111m/s";
    const pv_normalizer_token_tag_it_t sentence_tags[] = {
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_MEASUREMENT,
            PV_NORMALIZER_TAG_IT_PER_SLASH,
            PV_NORMALIZER_TAG_IT_MEASUREMENT,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_MEASUREMENT,
            PV_NORMALIZER_TAG_IT_PER_SLASH,
            PV_NORMALIZER_TAG_IT_MEASUREMENT,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_DECIMAL_COMMA,
            PV_NORMALIZER_TAG_IT_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_IT_MEASUREMENT,
            PV_NORMALIZER_TAG_IT_PER_SLASH,
            PV_NORMALIZER_TAG_IT_MEASUREMENT,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_IT_DECIMAL_COMMA,
            PV_NORMALIZER_TAG_IT_DECIMAL_DIGITS,
            PV_NORMALIZER_TAG_IT_MEASUREMENT,
            PV_NORMALIZER_TAG_IT_PER_SLASH,
            PV_NORMALIZER_TAG_IT_MEASUREMENT,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_MEASUREMENT,
            PV_NORMALIZER_TAG_IT_PER_SLASH,
            PV_NORMALIZER_TAG_IT_MEASUREMENT,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_WORD,
            PV_NORMALIZER_TAG_IT_PER_SLASH,
            PV_NORMALIZER_TAG_IT_WORD,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL_ONE,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_WORD,
            PV_NORMALIZER_TAG_IT_PER_SLASH,
            PV_NORMALIZER_TAG_IT_WORD,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_WORD,
            PV_NORMALIZER_TAG_IT_PER_SLASH,
            PV_NORMALIZER_TAG_IT_WORD,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL_ONE,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_WORD,
            PV_NORMALIZER_TAG_IT_PER_SLASH,
            PV_NORMALIZER_TAG_IT_WORD,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_IT_MEASUREMENT,
            PV_NORMALIZER_TAG_IT_PER_SLASH,
            PV_NORMALIZER_TAG_IT_MEASUREMENT,
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

static void test_pv_normalizer_tagger_tag_name(void) {
    const char *sentence = "Questo è J. R. R. Tolkien John F. Kennedy J1.";
    const pv_normalizer_token_tag_it_t sentence_tags[] = {
            PV_NORMALIZER_TAG_IT_WORD,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_WORD,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_IT_NAME_INITIAL_DOT,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_IT_NAME_INITIAL_DOT,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_IT_NAME_INITIAL_DOT,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_WORD,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_WORD,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_IT_NAME_INITIAL_DOT,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_WORD,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_IT_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_IT_PUNCTUATION,
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

static void test_pv_normalizer_tagger_tag_ordinal(void) {
    static pv_normalizer_use_cases_it_t use_cases[] = {
            PV_NORMALIZER_USE_ORDINAL_NORMALIZER_IT,
            PV_NORMALIZER_USE_CARDINAL_NORMALIZER_IT,
            PV_NORMALIZER_USE_ALPHANUM_SPELL_OUT_NORMALIZER_IT,
            PV_NORMALIZER_USE_WORD_NORMALIZER_IT,
            PV_NORMALIZER_USE_SPECIAL_CHARACTER_NORMALIZER_IT,
    };

    const char *sentence =
            "hello 1º 2ª 100ª 1.000º 1.000.000ª 2000000000000ª 10000000000000000000º 1° 100° 1.000° 10000000000000000000°";
    const pv_normalizer_token_tag_it_t sentence_tags[] = {
            PV_NORMALIZER_TAG_IT_WORD,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_ORDINAL_MASCULINE,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_ORDINAL_FEMININE,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_ORDINAL_FEMININE,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_ORDINAL_MASCULINE,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_ORDINAL_FEMININE,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_ORDINAL_FEMININE,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_ORDINAL_MASCULINE,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_ORDINAL_MASCULINE,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_ORDINAL_MASCULINE,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_SPECIAL_CHARACTER,
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

static void test_pv_normalizer_tagger_tag_negative_ordinal(void) {
    static pv_normalizer_use_cases_it_t use_cases[] = {
            PV_NORMALIZER_USE_ORDINAL_NORMALIZER_IT,
            PV_NORMALIZER_USE_NEGATIVE_ORDINAL_NORMALIZER_IT,
            PV_NORMALIZER_USE_CARDINAL_NORMALIZER_IT,
            PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_IT,
            PV_NORMALIZER_USE_ALPHANUM_SPELL_OUT_NORMALIZER_IT,
            PV_NORMALIZER_USE_WORD_NORMALIZER_IT,
            PV_NORMALIZER_USE_SPECIAL_CHARACTER_NORMALIZER_IT,
    };

    const char sentence[] =
            "hello -1º -2ª -100ª -1.000º -1.000.000ª -2000000000000ª -10000000000000000000º -1° -100° -1.000° -10000000000000000000° −1º";
    const pv_normalizer_token_tag_it_t sentence_tags[] = {
            PV_NORMALIZER_TAG_IT_WORD,
            PV_NORMALIZER_TAG_IT_NEGATIVE_ORDINAL_MASCULINE,
            PV_NORMALIZER_TAG_IT_NEGATIVE_ORDINAL_FEMININE,
            PV_NORMALIZER_TAG_IT_NEGATIVE_ORDINAL_FEMININE,
            PV_NORMALIZER_TAG_IT_NEGATIVE_ORDINAL_MASCULINE,
            PV_NORMALIZER_TAG_IT_NEGATIVE_ORDINAL_FEMININE,
            PV_NORMALIZER_TAG_IT_NEGATIVE_ORDINAL_FEMININE,
            PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_IT_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_IT_NEGATIVE_ORDINAL_MASCULINE,
            PV_NORMALIZER_TAG_IT_NEGATIVE_ORDINAL_MASCULINE,
            PV_NORMALIZER_TAG_IT_NEGATIVE_ORDINAL_MASCULINE,
            PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_IT_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_IT_NEGATIVE_ORDINAL_MASCULINE,
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

static void test_pv_normalizer_tagger_tag_only_special_one(void) {
    const char sentence_1[] =
            "1 -1 1 -1 1 1";
    const pv_normalizer_token_tag_it_t sentence_tags_1[] = {
            PV_NORMALIZER_TAG_IT_CARDINAL_ONE,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL_ONE,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL_ONE,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL_ONE,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL_ONE,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL_ONE,
    };

    pv_normalizer_tagger_t *tagger = NULL;
    test_pv_normalizer_tagger_init_helper(PV_ARRAY_LEN(ALL_TEST_USE_CASES), ALL_TEST_USE_CASES, &tagger);

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            tagger,
            sentence_1,
            PV_ARRAY_LEN(sentence_tags_1),
            true,
            sentence_tags_1);

    const char sentence_2[] =
            "-1 1 -1 1 -1 -1";
    const pv_normalizer_token_tag_it_t sentence_tags_2[] = {
            PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL_ONE,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL_ONE,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL_ONE,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL_ONE,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL_ONE,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL_ONE,
    };

    test_pv_normalizer_tagger_tokenize_tag_and_check_tags_helper(
            normalizer_tokenizer_object,
            tagger,
            sentence_2,
            PV_ARRAY_LEN(sentence_tags_2),
            true,
            sentence_tags_2);

    pv_normalizer_tagger_delete(tagger);
}

static void test_pv_normalizer_tagger_tag_single_quote(void) {
    const char *sentence = "Ha detto, 'come stai.' '(Ciao)' Nell'agosto '-1000º'";
    const pv_normalizer_token_tag_it_t sentence_tags[] = {
            PV_NORMALIZER_TAG_IT_WORD,
            PV_NORMALIZER_TAG_IT_WORD,
            PV_NORMALIZER_TAG_IT_PUNCTUATION,
            PV_NORMALIZER_TAG_IT_WORD,
            PV_NORMALIZER_TAG_IT_WORD,
            PV_NORMALIZER_TAG_IT_PUNCTUATION,
            PV_NORMALIZER_TAG_IT_SINGLE_QUOTE,
            PV_NORMALIZER_TAG_IT_SINGLE_QUOTE,
            PV_NORMALIZER_TAG_IT_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_IT_WORD,
            PV_NORMALIZER_TAG_IT_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_IT_SINGLE_QUOTE,
            PV_NORMALIZER_TAG_IT_WORD,
            PV_NORMALIZER_TAG_IT_SINGLE_QUOTE,
            PV_NORMALIZER_TAG_IT_NEGATIVE_ORDINAL_MASCULINE,
            PV_NORMALIZER_TAG_IT_SINGLE_QUOTE,
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

static void test_pv_normalizer_tagger_tokenize_tag_and_check_length_future_past_context_tags_helper(
        pv_normalizer_tokenizer_t *tokenizer,
        pv_normalizer_tagger_t *tagger,
        const char *sentence,
        int32_t num_target_tokens,
        bool preserve_word_boundary,
        const pv_normalizer_token_tag_it_t *target_tags,
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
                "incorrect original string: got `%s`, expected `%s`",
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

static void test_pv_normalizer_tagger_length_future_past_context_after_tag(void) {
    const char *sentence = "within 3-5 days. #AB12 www.example.com 9:30 1/2 1/1.000 1.123º 2ml 2m/h -1.900.876 ¥10.000,00 10.000,00¥ Prof.ssa Ted Prof. Ted";
    const pv_normalizer_token_tag_it_t sentence_tags[] = {
            PV_NORMALIZER_TAG_IT_WORD,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_NUMBER_RANGE_TO,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_WORD,
            PV_NORMALIZER_TAG_IT_PUNCTUATION,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_SPECIAL_CHARACTER,
            PV_NORMALIZER_TAG_IT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_IT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_IT_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_IT_CARDINAL_SPELL_OUT,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_IT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_IT_LETTER_SPELL_OUT,
            PV_NORMALIZER_TAG_IT_DOT,
            PV_NORMALIZER_TAG_IT_WORD,
            PV_NORMALIZER_TAG_IT_DOT,
            PV_NORMALIZER_TAG_IT_TOP_LEVEL_DOMAIN,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_TIME_HOURS,
            PV_NORMALIZER_TAG_IT_TIME_COLON,
            PV_NORMALIZER_TAG_IT_TIME_MINUTES,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL_ONE,
            PV_NORMALIZER_TAG_IT_FRACTION_SLASH,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL_ONE,
            PV_NORMALIZER_TAG_IT_FRACTION_SLASH,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_ORDINAL_MASCULINE,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_MEASUREMENT,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CARDINAL,
            PV_NORMALIZER_TAG_IT_MEASUREMENT,
            PV_NORMALIZER_TAG_IT_PER_SLASH,
            PV_NORMALIZER_TAG_IT_MEASUREMENT,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CURRENCY,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_CURRENCY,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_ABBREVIATION,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_WORD,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_ABBREVIATION,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_WORD,
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
            "1.123º",
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
            "¥10.000,00",
            " ",
            "10.000,00¥",
            " ",
            "Prof.ssa",
            " ",
            "Ted",
            " ",
            "Prof.",
            " ",
            "Ted",
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
            0,
            0,
            0,
            0,
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
            0,
            0,
            0,
            0,
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
    const char *sentence = "12/25/2023 03-gennaio-1829 (778) 239-1823 102-291-4920";
    const pv_normalizer_token_tag_it_t sentence_tags[] = {
            PV_NORMALIZER_TAG_IT_DATE_MONTH,
            PV_NORMALIZER_TAG_IT_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_IT_DATE_DAY,
            PV_NORMALIZER_TAG_IT_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_IT_DATE_YEAR,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_DATE_DAY,
            PV_NORMALIZER_TAG_IT_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_IT_DATE_MONTH,
            PV_NORMALIZER_TAG_IT_DATE_SEPARATOR,
            PV_NORMALIZER_TAG_IT_DATE_YEAR,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_DIGITS_WITH_PARENTHESES,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_DIGITS,
            PV_NORMALIZER_TAG_IT_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_IT_DIGITS,
            PV_NORMALIZER_TAG_IT_SPACE,
            PV_NORMALIZER_TAG_IT_DIGITS,
            PV_NORMALIZER_TAG_IT_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_IT_DIGITS,
            PV_NORMALIZER_TAG_IT_DIGITS_SEPARATOR,
            PV_NORMALIZER_TAG_IT_DIGITS,
    };
    const char *sentence_original_strings[] = {
            "12/25/2023",
            "12/25/2023",
            "12/25/2023",
            "12/25/2023",
            "12/25/2023",
            " ",
            "03-gennaio-1829",
            "03-gennaio-1829",
            "03-gennaio-1829",
            "03-gennaio-1829",
            "03-gennaio-1829",
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
    const char *sentence = "un po' di sollievo.";
    const pv_normalizer_token_tag_it_t sentence_tags[] = {
        PV_NORMALIZER_TAG_IT_WORD,
        PV_NORMALIZER_TAG_IT_SPACE,
        PV_NORMALIZER_TAG_IT_WORD,
        PV_NORMALIZER_TAG_IT_SPACE,
        PV_NORMALIZER_TAG_IT_WORD,
        PV_NORMALIZER_TAG_IT_SPACE,
        PV_NORMALIZER_TAG_IT_WORD,
        PV_NORMALIZER_TAG_IT_PUNCTUATION,
};
    const char *sentence_original_strings[] = {
        "un", " ",
        "po'", " ",
        "di", " ",
        "sollievo", ".",
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
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_tagger_it_init, PV_STATUS_OUT_OF_MEMORY);

    static pv_normalizer_use_cases_it_t use_cases[] = {PV_NORMALIZER_USE_WORD_NORMALIZER_IT};
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
            "`pv_normalizer_tagger_it_init` failed with status `OUT_OF_MEMORY`\\.",
            true,
            "error message mismatch");
}

static void test_pv_normalizer_tagger_tag_cardinal_helper_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_util_check_token_is_before_character, PV_STATUS_INVALID_ARGUMENT);

    const char sentence[] = "123";
    static pv_normalizer_use_cases_it_t use_cases[] = {PV_NORMALIZER_USE_CARDINAL_NORMALIZER_IT};
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
            "`pv_normalizer_util_check_token_is_before_character` failed with status `INVALID_ARGUMENT`\\.",
            false,
            "error message mismatch");

    pv_normalizer_tagger_delete(tagger);
    pv_normalizer_token_list_delete(token_list);
}

static void test_pv_normalizer_tagger_tag_currency_helper_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_util_check_token_is_before_character, PV_STATUS_INVALID_ARGUMENT);

    const char sentence[] = "5$";
    static pv_normalizer_use_cases_it_t use_cases[] = {PV_NORMALIZER_USE_CURRENCY_NORMALIZER_IT};
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
            "`pv_normalizer_util_check_token_is_before_character` failed with status `INVALID_ARGUMENT`\\.",
            false,
            "error message mismatch");

    pv_normalizer_tagger_delete(tagger);
    pv_normalizer_token_list_delete(token_list);
}

#endif


static const pv_test_case_t PV_NORMALIZER_TAGGER_TEST_CASES[] = {
        {"tag_word", test_pv_normalizer_tagger_tag_word},
        {"tag_custom_pronunciation", test_pv_normalizer_tagger_tag_custom_pronunciation},
        {"tag_punctuation", test_pv_normalizer_tagger_tag_punctuation},
        {"tag_special_character", test_pv_normalizer_tagger_tag_special_characters},
        {"tag_cardinal", test_pv_normalizer_tagger_tag_cardinal},
        {"tag_negative_cardinal", test_pv_normalizer_tagger_tag_negative_cardinal},
        {"tag_number_range", test_pv_normalizer_tagger_tag_number_range},
        {"tag_decimal", test_pv_normalizer_tagger_tag_decimal},
        {"tag_alphanum_spell_out", test_pv_normalizer_tagger_tag_alphanum_spell_out},
        {"tag_fraction", test_pv_normalizer_tagger_tag_fraction},
        {"tag_url", test_pv_normalizer_tagger_tag_url},
        {"tag_digits_sequence", test_pv_normalizer_tagger_tag_digits_sequence},
        {"tag_abbreviation", test_pv_normalizer_tagger_tag_abbreviations},
        {"tag_date", test_pv_normalizer_tagger_tag_date},
        {"tag_time", test_pv_normalizer_tagger_tag_time},
        {"tag_currency", test_pv_normalizer_tagger_tag_currency},
        {"tag_measurement", test_pv_normalizer_tagger_tag_measurement},
        {"tag_per_measurement", test_pv_normalizer_tagger_tag_per_measurement},
        {"tag_name", test_pv_normalizer_tagger_tag_name},
        {"tag_ordinal", test_pv_normalizer_tagger_tag_ordinal},
        {"tag_negative_ordinal", test_pv_normalizer_tagger_tag_negative_ordinal},
        {"tag_only_special_one", test_pv_normalizer_tagger_tag_only_special_one},
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

const pv_test_suite_t PV_NORMALIZER_TAGGER_IT_TEST_SUITE = {
        .name = "normalizer_tagger_it",
        .setup = test_pv_normalizer_tagger_setup,
        .teardown = test_pv_normalizer_tagger_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_NORMALIZER_TAGGER_TEST_CASES),
        .test_cases = PV_NORMALIZER_TAGGER_TEST_CASES,
};
