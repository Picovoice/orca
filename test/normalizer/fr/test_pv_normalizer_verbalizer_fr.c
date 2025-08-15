#include <stdlib.h>
#include <string.h>

#include "core/pv_language.h"
#include "core/pv_language_json.h"
#include "mock/pv_mock.h"
#include "orca/normalizer/fr/pv_normalizer_tags_fr.h"
#include "orca/normalizer/fr/pv_normalizer_verbalizer_fr.h"
#include "orca/normalizer/pv_normalizer_tagger.h"
#include "orca/normalizer/pv_normalizer_tokenizer.h"
#include "orca/normalizer/pv_normalizer_verbalizer.h"
#include "test/pv_test.h"

#ifdef __PV_MOCKS__

#include "core/mock/pv_core_runtime_mock.h"
#include "orca/mock/pv_normalizer_mock.h"

#endif

static pv_normalizer_verbalizer_t *normalizer_verbalizer_object = NULL;
static pv_normalizer_language_t LANGUAGE = PV_NORMALIZER_LANGUAGE_FR;
static int32_t ALL_TEST_USE_CASES[] = {
        PV_NORMALIZER_USE_WORD_NORMALIZER_FR,
        PV_NORMALIZER_USE_PUNCTUATION_NORMALIZER_FR,
        PV_NORMALIZER_USE_CARDINAL_NORMALIZER_FR,
        PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_FR,
        PV_NORMALIZER_USE_CUSTOM_PRONUNCIATION_NORMALIZER_FR,
        PV_NORMALIZER_USE_NUMBER_RANGE_NORMALIZER_FR,
        PV_NORMALIZER_USE_ORDINAL_NORMALIZER_FR,
        PV_NORMALIZER_USE_NEGATIVE_ORDINAL_NORMALIZER_FR,
        PV_NORMALIZER_USE_SPECIAL_CHARACTER_NORMALIZER_FR,
        PV_NORMALIZER_USE_DECIMAL_NORMALIZER_FR,
        PV_NORMALIZER_USE_ALPHANUM_SPELL_OUT_NORMALIZER_FR,
        PV_NORMALIZER_USE_MEASUREMENT_NORMALIZER_FR,
        PV_NORMALIZER_USE_TIME_NORMALIZER_FR,
        PV_NORMALIZER_USE_FRACTION_NORMALIZER_FR,
        PV_NORMALIZER_USE_URL_NORMALIZER_FR,
        PV_NORMALIZER_USE_CURRENCY_NORMALIZER_FR,
        PV_NORMALIZER_USE_ABBREVIATION_NORMALIZER_FR,
        PV_NORMALIZER_USE_DIGITS_SEQUENCE_NORMALIZER_FR,
        PV_NORMALIZER_USE_DATE_NORMALIZER_FR,
        PV_NORMALIZER_USE_NAME_NORMALIZER_FR,
};

static pv_normalizer_tagger_t *normalizer_tagger_object = NULL;
static pv_normalizer_tokenizer_t *normalizer_tokenizer_object = NULL;
static pv_language_info_t *language_info_object = NULL;
static pv_noun_gender_dict_t *noun_gender_dict_object = NULL;

static const char LANGUAGE_INFO_PATH[] = "normalizer/ref/pv_language_info_normalizer_fr.json";
static const char NOUN_GENDER_DICT_PATH[] = "test_data/noun_gender_dict/noun_gender_dict_fr.txt";

static const char VERBALIZER_TEST_SENTENCE[] = "je prends de la vitamine b-12, c, et 15 autres!";
static const char *VERBALIZER_TEST_SENTENCE_VERBALIZED[] = {
        "JE",
        "PRENDS",
        "DE",
        "LA",
        "VITAMINE",
        "B",
        "DOUZE",
        ",",
        "C",
        ",",
        "ET",
        "QUINZE",
        "AUTRES",
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

    char *noun_gender_dict_path = pv_test_module_res_path(NOUN_GENDER_DICT_PATH);
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
            ALL_TEST_USE_CASES,
            normalizer_tokenizer_object,
            noun_gender_dict_object,
            &normalizer_tagger_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_normalizer_verbalizer_init(
            LANGUAGE,
            PV_ARRAY_LEN(ALL_TEST_USE_CASES),
            ALL_TEST_USE_CASES,
            noun_gender_dict_object,
            &normalizer_verbalizer_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_normalizer_verbalizer_teardown(void) {
    pv_normalizer_tokenizer_delete(normalizer_tokenizer_object);
    normalizer_tokenizer_object = NULL;

    pv_noun_gender_dict_delete(noun_gender_dict_object);
    noun_gender_dict_object = NULL;

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
            noun_gender_dict_object,
            tagger);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to initialize tagger");

    status = pv_normalizer_verbalizer_init(LANGUAGE,
                                           num_use_cases,
                                           use_cases,
                                           noun_gender_dict_object,
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
    int32_t use_cases[] = {PV_NORMALIZER_USE_WORD_NORMALIZER_FR};

    static const char sentence[] = "je prends de la vitamine b-12, c, et 15 autres! j’e";

    const char *sentence_verbalized[] = {
            "JE",
            "PRENDS",
            "DE",
            "LA",
            "VITAMINE",
            "B",
            NULL,
            NULL,
            "C",
            NULL,
            "ET",
            NULL,
            "AUTRES",
            NULL,
            "J'E",
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
    int32_t use_cases[] = {PV_NORMALIZER_USE_PUNCTUATION_NORMALIZER_FR};

    const char *sentence_verbalized[] = {
            NULL,
            NULL,
            NULL,
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

static void test_pv_normalizer_verbalizer_verbalize_custom_pronunciation(void) {
    int32_t use_cases[] = {PV_NORMALIZER_USE_CUSTOM_PRONUNCIATION_NORMALIZER};

    const char sentence[] = "La prononciation {personal|p e s o n a l}";
    const char *sentence_verbalized[] = {NULL, NULL, "{p e s o n a l}"};

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
    int32_t use_cases[] = {PV_NORMALIZER_USE_CARDINAL_NORMALIZER};

    const char *sentence_verbalized[] = {
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            "DOUZE",
            NULL,
            NULL,
            NULL,
            NULL,
            "QUINZE",
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
    const char *sentence = "400 1800 36 130 0 003 456 123456789 5242 999999999999999 1200000000000009 11111111111111111";
    const char *sentence_verbalized[] = {
            "QUATRE CENTS",
            "MILLE HUIT CENTS",
            "TRENTE SIX",
            "CENT TRENTE",
            "ZÉRO",
            "ZÉRO ZÉRO TROIS",
            "QUATRE CENT CINQUANTE SIX",
            "CENT VINGT TROIS MILLION QUATRE CENT CINQUANTE SIX MILLE SEPT CENT QUATRE-VINGT NEUF",
            "CINQ MILLE DEUX CENT QUARANTE DEUX",
            "NEUF CENT QUATRE-VINGT-DIX NEUF BILLION NEUF CENT QUATRE-VINGT-DIX NEUF MILLIARD NEUF CENT QUATRE-VINGT-DIX NEUF MILLION NEUF CENT QUATRE-VINGT-DIX NEUF MILLE NEUF CENT QUATRE-VINGT-DIX NEUF",
            "UN DEUX ZÉRO ZÉRO ZÉRO ZÉRO ZÉRO ZÉRO ZÉRO ZÉRO ZÉRO ZÉRO ZÉRO ZÉRO ZÉRO NEUF",
            "UN UN UN UN UN UN UN UN UN UN UN UN UN UN UN UN UN"};

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
    int32_t use_cases[] = {PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_FR};

    const char sentence[] = "123 -123";
    const char *sentence_verbalized[] = {NULL, "MOINS CENT VINGT TROIS"};

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

    const char sentence[] = "12-5 1-44 1,1-1,3";
    const char *sentence_verbalized[] = {
            "DOUZE",
            "À",
            "CINQ",
            "UN",
            "À",
            "QUARANTE QUATRE",
            "UN",
            "VIRGULE",
            "UN",
            "À",
            "UN",
            "VIRGULE",
            "TROIS",
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
    const char *sentence =
            "1er 22e 19e 200e 4000e 1000000e 6000000000e 5000000000000e 123456789e";
    const char *sentence_verbalized[] = {
            "PREMIER",
            NULL,
            "VINGT DEUXIÈME",
            NULL,
            "DIX-NOUVIÈME",
            NULL,
            "DEUX CENTIÈME",
            NULL,
            "QUATRE MILLIÈME",
            NULL,
            "MILLIONIÈME",
            NULL,
            "SIX MILLIARDIÈME",
            NULL,
            "CINQ BILLIONIÈME",
            NULL,
            "CENT VINGT TROIS MILLION QUATRE CENT CINQUANTE SIX MILLE SEPT CENT QUATRE-VINGT NEUVIÈME",
    };

    pv_normalizer_tagger_t *tagger = NULL;
    pv_normalizer_verbalizer_t *verbalizer = NULL;
    test_pv_normalizer_verbalizer_init_helper(PV_ARRAY_LEN(ALL_TEST_USE_CASES), ALL_TEST_USE_CASES, &tagger, &verbalizer);

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

    char *sentences[] = {"01er", "1200000000000009e"};

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
    const char *sentence = "-1er -22e -5000000000000e -123456789e";
    const char *sentence_verbalized[] = {
            "MOINS PREMIER",
            "MOINS VINGT DEUXIÈME",
            "MOINS CINQ BILLIONIÈME",
            "MOINS CENT VINGT TROIS MILLION QUATRE CENT CINQUANTE SIX MILLE SEPT CENT QUATRE-VINGT NEUVIÈME",
    };

    pv_normalizer_tagger_t *tagger = NULL;
    pv_normalizer_verbalizer_t *verbalizer = NULL;
    test_pv_normalizer_verbalizer_init_helper(PV_ARRAY_LEN(ALL_TEST_USE_CASES), ALL_TEST_USE_CASES, &tagger, &verbalizer);

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

    const char *sentence = "& % @ \n _ ( ) °C RPM KM²";
    const char *sentence_verbalized[] = {
            "ET",
            "POUR CENT",
            "AROBASE",
            ".",
            "TIRET BAS",
            ",",
            ",",
            "DEGRÉ CELSIUS",
            "TOURS PAR MINUTE",
            "KILOMÈTRE CARRÉ"};

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
    const char *sentence = "1,23 ,5 10,1% -2,2";
    const char *sentence_verbalized[] = {
            "UN",
            "VIRGULE",
            "VINGT TROIS",
            "VIRGULE",
            "CINQ",
            "DIX",
            "VIRGULE",
            "UN",
            "POUR CENT",
            "MOINS DEUX",
            "VIRGULE",
            "DEUX"};

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
    const char sentence[] = "5g 1 ml 7,3L 3 ft 10,1km (38°C) 12-14°C 1.000.000g 25ft lb";
    const char *sentence_verbalized[] = {
            "CINQ",
            "GRAMMES",
            "UN",
            "MILLILITRE",
            "SEPT",
            "VIRGULE",
            "TROIS",
            "LITRES",
            "TROIS",
            "PIEDS",
            "DIX",
            "VIRGULE",
            "UN",
            "KILOMÈTRES",
            ",",
            "TRENTE HUIT",
            "DEGRÉS CELSIUS",
            ",",
            "DOUZE",
            "À",
            "QUATORZE",
            "DEGRÉS CELSIUS",
            "MILLION",
            "GRAMMES",
            "VINGT CINQ",
            "PIED",
            "POUNDS",
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
    const char sentence[] = "5 km/m 10 oz/km 1,1m/l -5,41kg/ft tb/hz 1.000m/s";
    const char *sentence_verbalized[] = {
            "CINQ",
            "KILOMÈTRES",
            "PAR",
            "MÈTRE",
            "DIX",
            "OUNCES",
            "PAR",
            "KILOMÈTRE",
            "UN",
            "VIRGULE",
            "UN",
            "MÈTRES",
            "PAR",
            "LITRE",
            "MOINS CINQ",
            "VIRGULE",
            "QUARANTE ET UN",
            "KILOGRAMMES",
            "PAR",
            "PIED",
            "TERABYTES",
            "PAR",
            "HERTZ",
            "MILLE",
            "MÈTRES",
            "PAR",
            "SECONDE",
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
    const char *sentence = "HTML5 C5B 12A Z78 ÇL1";
    const char *sentence_verbalized[] = {
            "H", "T", "M", "L", "CINQ",
            NULL,
            "C", "CINQ", "B",
            NULL,
            "UN", "DEUX", "A",
            NULL,
            "Z", "SEPT", "HUIT",
            NULL,
            "Ç", "L", "UN"};

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
    const char *sentence = "14:00 20h01 1h45 7:07 0h31 12h59' 9h11m 11:15am 3:25 pm 8:00 p.m. 4am AM";
    const char *sentence_verbalized[] = {
            "QUATORZE HEURES",
            NULL,
            NULL,
            NULL,
            "VINGT HEURES",
            NULL,
            "ET UNE",
            NULL,
            "UNE HEURE",
            NULL,
            "QUARANTE CINQ",
            NULL,
            "SEPT HEURES",
            NULL,
            "SEPT",
            NULL,
            "ZÉRO HEURES",
            NULL,
            "TRENTE ET UN",
            NULL,
            "DOUZE HEURES",
            NULL,
            "CINQUANTE NEUF",
            NULL,
            "NEUF HEURES",
            NULL,
            "ONZE",
            NULL,
            "ONZE HEURES",
            NULL,
            "QUINZE",
            "DU MATIN",
            NULL,
            "TROIS HEURES",
            NULL,
            "VINGT CINQ",
            NULL,
            "DE L'APRÈS-MIDI",
            NULL,
            "HUIT HEURES",
            NULL,
            NULL,
            NULL,
            "DE L'APRÈS-MIDI",
            NULL,
            "QUATRE HEURES",
            "DU MATIN",
            NULL,
            "AM",
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

static void test_pv_normalizer_verbalizer_verbalize_dot_cardinal(void) {
    const char *sentence = "1.005 11.005 111.005 -5.000.000.000 1111.000 1.1.000 1. 000 (384.400)";
    const char *sentence_verbalized[] = {
            "MILLE CINQ",
            "ONZE MILLE CINQ",
            "CENT ONZE MILLE CINQ",
            "MOINS CINQ MILLIARD",
            "MILLE CENT ONZE",
            "POINT",
            "ZÉRO ZÉRO ZÉRO",
            "UN",
            "POINT",
            "UN",
            "POINT",
            "ZÉRO ZÉRO ZÉRO",
            "UN",
            ".",
            "ZÉRO ZÉRO ZÉRO",
            ",",
            "TROIS CENT QUATRE-VINGT QUATRE MILLE QUATRE CENTS",
            ","};

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            false,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_dot_ordinal(void) {
    const char *sentence = "1.005e 11.005e 111.005e -5.003.000.003e 1111.100e 1.1.100e 1. 100e";
    const char *sentence_verbalized[] = {
            "MILLE CINQUIÈME",
            "ONZE MILLE CINQUIÈME",
            "CENT ONZE MILLE CINQUIÈME",
            "MOINS CINQ MILLIARD TROIS MILLION TROISIÈME",
            "MILLE CENT ONZE",
            "POINT",
            "CENTIÈME",
            "UN",
            "POINT",
            "UN",
            "POINT",
            "CENTIÈME",
            "UN",
            ".",
            "CENTIÈME",
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

static void test_pv_normalizer_verbalizer_verbalize_dot_decimal(void) {
    const char *sentence = "1.000,2 -300.000,6666";
    const char *sentence_verbalized[] = {
            "MILLE",
            "VIRGULE",
            "DEUX",
            "MOINS TROIS CENT MILLE",
            "VIRGULE",
            "SIX MILLE SIX CENT SOIXANTE SIX",
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

static void test_pv_normalizer_verbalizer_verbalize_dot_number_range(void) {
    const char *sentence = "1.000-2.000 1.000,2-1.000,5 10.000-1,3";
    const char *sentence_verbalized[] = {
            "MILLE",
            "À",
            "DEUX MILLE",
            "MILLE",
            "VIRGULE",
            "DEUX",
            "À",
            "MILLE",
            "VIRGULE",
            "CINQ",
            "DIX MILLE",
            "À",
            "UN",
            "VIRGULE",
            "TROIS",
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
    const char *sentence = "(1/2) 9/3,2 1/1.000 1/-4 -4/3 1/1001 1/10000";
    const char *sentence_verbalized[] = {
            ",",
            "UN",
            NULL,
            "MOITIÉ",
            ",",
            "NEUF",
            "SUR",
            "TROIS",
            "VIRGULE",
            "DEUX",
            "UN",
            NULL,
            "MILLIÈME",
            "UN",
            "SUR",
            "MOINS QUATRE",
            "MOINS QUATRE",
            NULL,
            "TIERS",
            "UN",
            NULL,
            "MILLE",
            "UN",
            NULL,
            "DIX MILLE"};

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            false,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_fraction_feminine(void) {
    const char *sentence = "1/3 d'une fleur";
    const char *sentence_verbalized[] = {
            "UN",
            NULL,
            "TIERS",
            "D'UNE",
            "FLEUR",
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

static void test_pv_normalizer_verbalizer_verbalize_url(void) {
    const char *sentence = "www.exemple.com https://salut.ca/";
    const char *sentence_verbalized[] = {
            "W",
            "W",
            "W",
            "POINT",
            "EXEMPLE",
            "POINT",
            "COM",
            "H",
            "T",
            "T",
            "P",
            "S",
            "DEUX-POINTS",
            "BARRE OBLIQUE",
            "BARRE OBLIQUE",
            "SALUT",
            "POINT",
            "C",
            "A",
            "BARRE OBLIQUE",
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
            "$5 €10 10$ 10€ $1,25 1,25$ -$500 -500$ -1,25$ -$1,25 €1.000 -€1.000 1.000€ -1.000€ €1.000,25 1.000,25€ "
            "¥2 ₪2 £2 ₩2 ₺2 ₱2 ₽2 ฿2 ₴2 ₹2 ¢2 $1,00 $1,01 5 EUR 1,25 £ -3 € €100.000.";
    const char *sentence_verbalized[] = {
            "CINQ DOLLARS",
            NULL,
            "DIX EUROS",
            NULL,
            "DIX DOLLARS",
            NULL,
            "DIX EUROS",
            NULL,
            "UN DOLLAR AVEC VINGT CINQ CENTS",
            NULL,
            "UN DOLLAR AVEC VINGT CINQ CENTS",
            NULL,
            "MOINS CINQ CENT DOLLARS",
            NULL,
            "MOINS CINQ CENT DOLLARS",
            NULL,
            "MOINS UN DOLLAR AVEC VINGT CINQ CENTS",
            NULL,
            "MOINS UN DOLLAR AVEC VINGT CINQ CENTS",
            NULL,
            "MILLE EUROS",
            NULL,
            "MOINS MILLE EUROS",
            NULL,
            "MILLE EUROS",
            NULL,
            "MOINS MILLE EUROS",
            NULL,
            "MILLE EUROS AVEC VINGT CINQ CENTS",
            NULL,
            "MILLE EUROS AVEC VINGT CINQ CENTS",
            NULL,
            "DEUX YUAN",
            NULL,
            "DEUX SHEKELS",
            NULL,
            "DEUX POUNDS",
            NULL,
            "DEUX WON",
            NULL,
            "DEUX LIRA",
            NULL,
            "DEUX PESOS",
            NULL,
            "DEUX RUBLES",
            NULL,
            "DEUX BAHT",
            NULL,
            "DEUX HRYVNIAS",
            NULL,
            "DEUX RUPEES",
            NULL,
            "DEUX CENTS",
            NULL,
            "UN DOLLAR",
            NULL,
            "UN DOLLAR AVEC UN CENT",
            NULL,
            "CINQ",
            NULL,
            "EUROS",
            NULL,
            "UN",
            "VIRGULE",
            "VINGT CINQ",
            NULL,
            "POUNDS",
            NULL,
            "MOINS TROIS",
            NULL,
            "EUROS",
            NULL,
            "CENT MILLE EUROS",
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

static void test_pv_normalizer_verbalizer_verbalize_abbreviation(void) {
    const char *sentence = " e.g. c'est Prof. Dr. Doe Ave. vs. etc.";
    const char *sentence_verbalized[] = {
            NULL,
            "PAR EXEMPLE",
            NULL,
            "C'EST",
            NULL,
            "PROFESSEUR",
            NULL,
            "DOCTEUR",
            NULL,
            "DOE",
            NULL,
            "AVENUE",
            NULL,
            "CONTRE",
            NULL,
            "ET CETERA",
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
    const char *sentence = " (772) 778-1923 1-800-123-4567 123-99456-7890 +1 (381) 102-129";
    const char *sentence_verbalized[] = {
            NULL,
            ", SEPT SEPT DEUX,",
            NULL,
            "SEPT SEPT HUIT",
            ",",
            "UN NEUF DEUX TROIS",
            NULL,
            "UN",
            ",",
            "HUIT ZÉRO ZÉRO",
            ",",
            "UN DEUX TROIS",
            ",",
            "QUATRE CINQ SIX SEPT",
            NULL,
            "UN DEUX TROIS",
            ",",
            "NEUF NEUF QUATRE CINQ SIX",
            ",",
            "SEPT HUIT NEUF ZÉRO",
            NULL,
            "PLUS",
            "UN",
            NULL,
            ", TROIS HUIT UN,",
            NULL,
            "UN ZÉRO DEUX",
            ",",
            "UN DEUX NEUF",
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
    const char *sentence = "03-juin-2020 08/02/1877 2013-11-28 13/01/1992 1 août 32-10-2024 2 oct 1800 09-21-1907";
    const char *sentence_verbalized[] = {
            "TROIS",
            NULL,
            "JUIN",
            NULL,
            "DEUX MILLE VINGT",
            NULL,
            "HUIT",
            NULL,
            "FÉVRIER",
            NULL,
            "MILLE HUIT CENT SOIXANTE-DIX SEPT",
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            "VINGT HUIT NOVEMBRE DEUX MILLE TREIZE",
            NULL,
            "TREIZE",
            NULL,
            "JANVIER",
            NULL,
            "MILLE NEUF CENT QUATRE-VINGT-DIX DEUX",
            NULL,
            "PREMIER",
            NULL,
            "AOÛT",
            NULL,
            "TROIS DEUX",
            ",",
            "UN ZÉRO",
            ",",
            "DEUX ZÉRO DEUX QUATRE",
            NULL,
            "DEUX",
            NULL,
            "OCTOBRE",
            NULL,
            "MILLE HUIT CENTS",
            NULL,
            NULL,
            NULL,
            "VINGT ET UN SEPTEMBRE",
            NULL,
            "MILLE NEUF CENT SEPT",
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

static void test_pv_normalizer_verbalizer_verbalize_complex_cardinal(void) {
    const char *sentence = "51% 1 personne = 2,54 cm 1h et 15 minute 493 personnes 493000 personnes 200.200.000 personnes 276.841 km²";
    const char *sentence_verbalized[] = {
            "CINQUANTE ET UN",
            "POUR CENT",
            "UN",
            "PERSONNE",
            "ÉGAL",
            "DEUX",
            "VIRGULE",
            "CINQUANTE QUATRE",
            "CENTIMÈTRES",
            "UN",
            "HEURE",
            "ET",
            "QUINZE",
            "MINUTE",
            "QUATRE CENT QUATRE-VINGT-DIX TROIS",
            "PERSONNES",
            "QUATRE CENT QUATRE-VINGT-DIX TROIS MILLE",
            "PERSONNES",
            "DEUX CENT MILLION DEUX CENT MILLE",
            "PERSONNES",
            "DEUX CENT SOIXANTE-DIX SIX MILLE HUIT CENT QUARANTE ET UN",
            "KILOMÈTRES CARRÉS",
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
    const char *sentence = "Elle a demandé, 'comment vas-tu.' 'Bonjour@'";
    const char *sentence_verbalized[] = {
            "ELLE",
            "A",
            "DEMANDÉ",
            ",",
            "'COMMENT",
            "VAS-TU",
            ".",
            "\"",
            "'BONJOUR",
            "AROBASE",
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
            PV_NORMALIZER_LANGUAGE_FR,
            PV_ARRAY_LEN(ALL_TEST_USE_CASES),
            ALL_TEST_USE_CASES,
            noun_gender_dict_object,
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
        pv_normalizer_token_tag_fr_t tag) {
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
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_verbalizer_fr_init, PV_STATUS_INVALID_ARGUMENT)

    test_pv_normalizer_verbalizer_init_failure_helper(PV_STATUS_INVALID_ARGUMENT, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_fr_init` failed with status `INVALID_ARGUMENT`\\.");
}

static void test_pv_normalizer_verbalizer_verbalize_word_fail_calloc(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "hello";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_FR_WORD);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_verbalize_word` failed with status `OUT_OF_MEMORY`.");
}

static void test_pv_normalizer_verbalizer_verbalize_punctuation_fail_calloc(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = ",";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_FR_PUNCTUATION);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_verbalize_punctuation` failed with status `OUT_OF_MEMORY`.");
}

static void test_pv_normalizer_verbalizer_verbalize_cardinal_fail_calloc(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "5";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_FR_CARDINAL);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_verbalize_cardinal` failed with status `OUT_OF_MEMORY`.");
}

static void test_pv_normalizer_verbalizer_verbalize_negative_cardinal_fail_calloc(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "-5";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_FR_NEGATIVE_CARDINAL);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_verbalize_negative_cardinal` failed with status `OUT_OF_MEMORY`.");
}

static void test_pv_normalizer_verbalizer_verbalize_negative_cardinal_fail_calloc_2(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "-5";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_FR_NEGATIVE_CARDINAL);

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
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_FR_NUMBER_RANGE_TO);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_verbalize_number_range` failed with status `OUT_OF_MEMORY`.");
}

static void test_pv_normalizer_verbalizer_verbalize_ordinal_fail_calloc_1(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "1st";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_FR_ORDINAL);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `number_string`.");
}

static void test_pv_normalizer_verbalizer_verbalize_ordinal_fail_calloc_2(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "1st";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_FR_ORDINAL);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_real,
            calloc_real,
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `last_cardinal`.");
}

static void test_pv_normalizer_verbalizer_verbalize_negative_ordinal_fail_calloc_1(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "-1st";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_FR_NEGATIVE_ORDINAL);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `number_string`.");
}

static void test_pv_normalizer_verbalizer_verbalize_negative_ordinal_fail_calloc_2(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "-1st";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_FR_NEGATIVE_ORDINAL);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_real,
            calloc_real,
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `last_cardinal`.");
}

static void test_pv_normalizer_verbalizer_verbalize_negative_ordinal_fail_calloc_3(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "-1st";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_FR_NEGATIVE_ORDINAL);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_real,
            calloc_real,
            calloc_real,
            calloc_real,
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `token_verbalized`.");
}

static void test_pv_normalizer_verbalizer_verbalize_currency_fail_calloc_1(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "$1";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_FR_CURRENCY);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `number_string`.");
}

static void test_pv_normalizer_verbalizer_verbalize_currency_fail_calloc_2(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "$1";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_FR_CURRENCY);

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
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_FR_NEGATIVE_CURRENCY);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `number_string`.");
}

static void test_pv_normalizer_verbalizer_verbalize_negative_currency_fail_calloc_2(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "-$1";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_FR_NEGATIVE_CURRENCY);

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
        {"verbalize_dot_cardinal", test_pv_normalizer_verbalizer_verbalize_dot_cardinal},
        {"verbalize_dot_ordinal", test_pv_normalizer_verbalizer_verbalize_dot_ordinal},
        {"verbalize_dot_decimal", test_pv_normalizer_verbalizer_verbalize_dot_decimal},
        {"verbalize_dot_number_range", test_pv_normalizer_verbalizer_verbalize_dot_number_range},
        {"verbalize_fraction", test_pv_normalizer_verbalizer_verbalize_fraction},
        {"verbalize_fraction_feminine", test_pv_normalizer_verbalizer_verbalize_fraction_feminine},
        {"verbalize_url", test_pv_normalizer_verbalizer_verbalize_url},
        {"verbalize_currency", test_pv_normalizer_verbalizer_verbalize_currency},
        {"verbalize_abbreviation", test_pv_normalizer_verbalizer_verbalize_abbreviation},
        {"verbalize_digits_sequence", test_pv_normalizer_verbalizer_verbalize_digits_sequence},
        {"verbalize_date", test_pv_normalizer_verbalizer_verbalize_date},
        {"verbalize_name", test_pv_normalizer_verbalizer_verbalize_name},
        {"verbalize_complex_cardinal", test_pv_normalizer_verbalizer_verbalize_complex_cardinal},
        {"verbalize_single_quote", test_pv_normalizer_verbalizer_verbalize_single_quote},

#ifdef __PV_MOCKS__

        {"verbalizer_init_calloc_failure", test_pv_normalizer_verbalizer_init_calloc_failure},
        {"verbalizer_init_internal_failure", test_pv_normalizer_verbalizer_init_internal_failure},
        {"verbalize_word_fail_calloc", test_pv_normalizer_verbalizer_verbalize_word_fail_calloc},
        {"verbalize_punctuation_fail_calloc", test_pv_normalizer_verbalizer_verbalize_punctuation_fail_calloc},
        {"verbalize_cardinal_fail_calloc", test_pv_normalizer_verbalizer_verbalize_cardinal_fail_calloc},
        {"verbalize_negative_cardinal_fail_calloc", test_pv_normalizer_verbalizer_verbalize_negative_cardinal_fail_calloc},
        {"verbalize_negative_cardinal_fail_calloc_2", test_pv_normalizer_verbalizer_verbalize_negative_cardinal_fail_calloc_2},
        {"verbalize_number_range_fail_calloc_1", test_pv_normalizer_verbalizer_verbalize_number_range_fail_calloc_1},
        {"verbalize_ordinal_fail_calloc_1", test_pv_normalizer_verbalizer_verbalize_ordinal_fail_calloc_1},
        {"verbalize_ordinal_fail_calloc_2", test_pv_normalizer_verbalizer_verbalize_ordinal_fail_calloc_2},
        {"verbalize_negative_ordinal_fail_calloc_1", test_pv_normalizer_verbalizer_verbalize_negative_ordinal_fail_calloc_1},
        {"verbalize_negative_ordinal_fail_calloc_2", test_pv_normalizer_verbalizer_verbalize_negative_ordinal_fail_calloc_2},
        {"verbalize_negative_ordinal_fail_calloc_3", test_pv_normalizer_verbalizer_verbalize_negative_ordinal_fail_calloc_3},
        {"verbalize_currency_fail_calloc_1", test_pv_normalizer_verbalizer_verbalize_currency_fail_calloc_1},
        {"verbalize_currency_fail_calloc_2", test_pv_normalizer_verbalizer_verbalize_currency_fail_calloc_2},
        {"verbalize_negative_currency_fail_calloc_1", test_pv_normalizer_verbalizer_verbalize_negative_currency_fail_calloc_1},
        {"verbalize_negative_currency_fail_calloc_2", test_pv_normalizer_verbalizer_verbalize_negative_currency_fail_calloc_2},

#endif
};

const pv_test_suite_t PV_NORMALIZER_VERBALIZER_FR_TEST_SUITE = {
        .name = "normalizer_verbalizer_fr",
        .setup = test_pv_normalizer_verbalizer_setup,
        .teardown = test_pv_normalizer_verbalizer_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_NORMALIZER_VERBALIZER_TEST_CASES),
        .test_cases = PV_NORMALIZER_VERBALIZER_TEST_CASES,
};
