#include <stdlib.h>
#include <string.h>

#include "core/pv_language.h"
#include "core/pv_language_json.h"
#include "mock/pv_mock.h"
#include "orca/normalizer/pt/pv_normalizer_tags_pt.h"
#include "orca/normalizer/pt/pv_normalizer_verbalizer_pt.h"
#include "orca/normalizer/pv_normalizer_tagger.h"
#include "orca/normalizer/pv_normalizer_tokenizer.h"
#include "orca/normalizer/pv_normalizer_verbalizer.h"
#include "test/pv_test.h"

#ifdef __PV_MOCKS__

#include "core/mock/pv_core_runtime_mock.h"
#include "orca/mock/pv_normalizer_mock.h"

#endif

static pv_normalizer_verbalizer_t *normalizer_verbalizer_object = NULL;
static pv_normalizer_language_t LANGUAGE = PV_NORMALIZER_LANGUAGE_PT;
static pv_normalizer_use_cases_pt_t ALL_TEST_USE_CASES[] = {
        PV_NORMALIZER_USE_WORD_NORMALIZER_PT,
        PV_NORMALIZER_USE_PUNCTUATION_NORMALIZER_PT,
        PV_NORMALIZER_USE_CUSTOM_PRONUNCIATION_NORMALIZER_PT,
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

static pv_normalizer_tagger_t *normalizer_tagger_object = NULL;
static pv_normalizer_tokenizer_t *normalizer_tokenizer_object = NULL;
static pv_language_info_t *language_info_object = NULL;
static pv_noun_gender_dict_t *noun_gender_dict_object = NULL;

static const char LANGUAGE_INFO_PATH[] = "normalizer/ref/pv_language_info_normalizer_pt.json";
static const char NOUN_GENDER_DICT_PATH[] = "test_data/noun_gender_dict/noun_gender_dict_pt.txt";

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
            (const int32_t *) ALL_TEST_USE_CASES,
            normalizer_tokenizer_object,
            noun_gender_dict_object,
            &normalizer_tagger_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_normalizer_verbalizer_init(
            LANGUAGE,
            PV_ARRAY_LEN(ALL_TEST_USE_CASES),
            (const int32_t *) ALL_TEST_USE_CASES,
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
        const pv_normalizer_use_cases_pt_t *use_cases,
        pv_normalizer_tagger_t **tagger,
        pv_normalizer_verbalizer_t **verbalizer) {
    *tagger = NULL;
    *verbalizer = NULL;

    pv_status_t status = pv_normalizer_tagger_init(
            language_info_object,
            num_use_cases,
            (const int32_t *) use_cases,
            normalizer_tokenizer_object,
            noun_gender_dict_object,
            tagger);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to initialize tagger");

    status = pv_normalizer_verbalizer_init(
            LANGUAGE,
            num_use_cases,
            (const int32_t *) use_cases,
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

static void test_pv_normalizer_verbalizer_verbalize_invalid_use_case(void) {
    static pv_normalizer_use_cases_pt_t use_cases[] = {99};
    const char sentence[] = "O amor é a força mais poderosa do universo";

    pv_normalizer_tagger_t *tagger = NULL;
    pv_normalizer_verbalizer_t *verbalizer = NULL;
    test_pv_normalizer_verbalizer_init_helper(PV_ARRAY_LEN(use_cases), use_cases, &tagger, &verbalizer);

    test_pv_normalizer_verbalizer_verbalize_and_check_status_helper(
            normalizer_tokenizer_object,
            tagger,
            verbalizer,
            sentence,
            PV_STATUS_INVALID_ARGUMENT,
            PV_STATUS_INVALID_ARGUMENT);

    pv_normalizer_tagger_delete(tagger);
    pv_normalizer_verbalizer_delete(verbalizer);
}

static void test_pv_normalizer_verbalizer_verbalize_word(void) {
    static pv_normalizer_use_cases_pt_t use_cases[] = {PV_NORMALIZER_USE_WORD_NORMALIZER_PT};

    const char sentence[] = "O amor é a força mais poderosa do universo d’o";
    const char *sentence_verbalized[] = {
            "O",
            NULL,
            "AMOR",
            NULL,
            "É",
            NULL,
            "A",
            NULL,
            "FORÇA",
            NULL,
            "MAIS",
            NULL,
            "PODEROSA",
            NULL,
            "DO",
            NULL,
            "UNIVERSO",
            NULL,
            "D'O",
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

static void test_pv_normalizer_verbalizer_verbalize_punctuation(void) {
    static pv_normalizer_use_cases_pt_t use_cases[] = {PV_NORMALIZER_USE_PUNCTUATION_NORMALIZER_PT};

    const char sentence[] = "Quer bolo, leite, ou suco?";
    const char *sentence_verbalized[] = {
            NULL,
            NULL,
            ",",
            NULL,
            ",",
            NULL,
            NULL,
            "?",
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

static void test_pv_normalizer_verbalizer_verbalize_custom_pronunciation(void) {
    static pv_normalizer_use_cases_pt_t use_cases[] = {PV_NORMALIZER_USE_CUSTOM_PRONUNCIATION_NORMALIZER_PT};

    const char sentence[] = "Uma pronúncia {lus|l u z}";
    const char *sentence_verbalized[] = {NULL, NULL, "{l u z}"};

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
    static pv_normalizer_use_cases_pt_t use_cases[] = {PV_NORMALIZER_USE_CARDINAL_NORMALIZER_PT};

    const char sentence[] = "Contar 1, 2, 15, 126, 450 e 32562";
    const char *sentence_verbalized[] = {
            NULL,
            "UM",
            NULL,
            "DOIS",
            NULL,
            "QUINZE",
            NULL,
            "CENTO E VINTE E SEIS",
            NULL,
            "QUATROCENTOS E CINQUENTA",
            NULL,
            "TRINTA E DOIS MIL QUINHENTOS E SESSENTA E DOIS",
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

    const char sentence2[] = "Em 2.023, cerca de 1.200.000.";
    const char *sentence_verbalized2[] = {
            "EM",
            "DOIS MIL E VINTE E TRÊS",
            ",",
            "CERCA",
            "DE",
            "UM MILHÃO E DUZENTOS MIL",
            ".",
    };

    tagger = NULL;
    verbalizer = NULL;
    test_pv_normalizer_verbalizer_init_helper(PV_ARRAY_LEN(ALL_TEST_USE_CASES), ALL_TEST_USE_CASES, &tagger, &verbalizer);

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            tagger,
            verbalizer,
            sentence2,
            PV_ARRAY_LEN(sentence_verbalized2),
            false,
            sentence_verbalized2);

    pv_normalizer_tagger_delete(tagger);
    pv_normalizer_verbalizer_delete(verbalizer);
}

static void test_pv_normalizer_verbalizer_verbalize_negative_cardinal(void) {
    static pv_normalizer_use_cases_pt_t use_cases[] = {PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_PT};

    const char sentence[] = "Os números -5 e -123 são menores que zero";
    const char *sentence_verbalized[] = {
            NULL,
            NULL,
            "MENOS CINCO",
            NULL,
            "MENOS CENTO E VINTE E TRÊS",
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
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            false,
            sentence_verbalized);

    pv_normalizer_tagger_delete(tagger);
    pv_normalizer_verbalizer_delete(verbalizer);
}

static void test_pv_normalizer_verbalizer_verbalize_cardinal_gender(void) {
    static pv_normalizer_use_cases_pt_t use_cases[] = {
            PV_NORMALIZER_USE_CARDINAL_NORMALIZER_PT,
            PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_PT,
    };

    const char sentence[] = "1 abegoaria, 2 abarca, 22 acenos e 450 abóbora. -500 abarca";
    const char *sentence_verbalized[] = {
            "UMA",
            NULL,
            NULL,
            "DUAS",
            NULL,
            NULL,
            "VINTE E DOIS",
            NULL,
            NULL,
            "QUATROCENTAS E CINQUENTA",
            NULL,
            NULL,
            "MENOS QUINHENTAS",
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

static void test_pv_normalizer_verbalizer_verbalize_dot_cardinal(void) {
    static pv_normalizer_use_cases_pt_t use_cases[] = {
            PV_NORMALIZER_USE_CARDINAL_NORMALIZER_PT,
            PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_PT,
    };

    const char *sentence = "1.000 1.005 11.050 111.500 101.020 -5.002.023.275 -100.123.456 1.005.030.608.394 1.000.000.000";
    const char *sentence_verbalized[] = {
            "MIL",
            "MIL E CINCO",
            "ONZE MIL E CINQUENTA",
            "CENTO E ONZE MIL E QUINHENTOS",
            "CENTO E UM MIL E VINTE",
            "MENOS CINCO MIL E DOIS MILHÕES E VINTE E TRÊS MIL DUZENTOS E SETENTA E CINCO",
            "MENOS CEM MILHÕES CENTO E VINTE E TRÊS MIL QUATROCENTOS E CINQUENTA E SEIS",
            "UM BILHÃO E CINCO MIL E TRINTA MILHÕES SEISCENTOS E OITO MIL TREZENTOS E NOVENTA E QUATRO",
            "MIL MILHÕES",
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

static void test_pv_normalizer_verbalizer_verbalize_dot_cardinal_gender(void) {
    static pv_normalizer_use_cases_pt_t use_cases[] = {
            PV_NORMALIZER_USE_CARDINAL_NORMALIZER_PT,
            PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_PT,
    };

    const char sentence[] = "1.222 abafação 23.234 abarca 345.678 acenos 4.345.356 -2.000 abóbora";
    const char *sentence_verbalized[] = {
            "MIL DUZENTAS E VINTE E DUAS",
            NULL,
            "VINTE E TRÊS MIL DUZENTAS E TRINTA E QUATRO",
            NULL,
            "TREZENTOS E QUARENTA E CINCO MIL SEISCENTOS E SETENTA E OITO",
            NULL,
            "QUATRO MILHÕES TREZENTOS E QUARENTA E CINCO MIL TREZENTOS E CINQUENTA E SEIS",
            "MENOS DUAS MIL",
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

static void test_pv_normalizer_verbalizer_verbalize_ordinal(void) {
    const char sentence[] = "1º 642º 120º 1.001º 2ª 150ª 642ª 2.004ª 466.o 789.a";
    const char *sentence_verbalized[] = {
            "PRIMEIRO",
            NULL,
            "SEXCENTÉSIMO QUADRAGÉSIMO SEGUNDO",
            NULL,
            "CENTÉSIMO VIGÉSIMO",
            NULL,
            "MILÉSIMO PRIMEIRO",
            NULL,
            "SEGUNDA",
            NULL,
            "CENTÉSIMA QUINQUAGÉSIMA",
            NULL,
            "SEXCENTÉSIMA QUADRAGÉSIMA SEGUNDA",
            NULL,
            "SEGUNDA MILÉSIMA QUARTA",
            NULL,
            "QUADRINGENTÉSIMO SEXAGÉSIMO SEXTO",
            NULL,
            "SEPTINGENTÉSIMA OCTOGÉSIMA NONA",
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

static void test_pv_normalizer_verbalizer_verbalize_negative_ordinal(void) {
    const char sentence[] = "-1º -642º -120º -1.001º -2ª -150ª -642ª -2.004ª -466.o -789.a";
    const char *sentence_verbalized[] = {
            "MENOS PRIMEIRO",
            NULL,
            "MENOS SEXCENTÉSIMO QUADRAGÉSIMO SEGUNDO",
            NULL,
            "MENOS CENTÉSIMO VIGÉSIMO",
            NULL,
            "MENOS MILÉSIMO PRIMEIRO",
            NULL,
            "MENOS SEGUNDA",
            NULL,
            "MENOS CENTÉSIMA QUINQUAGÉSIMA",
            NULL,
            "MENOS SEXCENTÉSIMA QUADRAGÉSIMA SEGUNDA",
            NULL,
            "MENOS SEGUNDA MILÉSIMA QUARTA",
            NULL,
            "MENOS QUADRINGENTÉSIMO SEXAGÉSIMO SEXTO",
            NULL,
            "MENOS SEPTINGENTÉSIMA OCTOGÉSIMA NONA",
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
    const char sentence[] =
            "-10000000000000000000.a -10000000000000000000º 10000000000000000000.o 10000000000000000000ª";
    const char *sentence_verbalized[] = {
            "MENOS UM ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO",
            "PONTO",
            "A",
            NULL,
            "MENOS UM ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO",
            "Ó",
            NULL,
            "UM ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO",
            "PONTO",
            "O",
            NULL,
            "UM ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO",
            "Á",
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

static void test_pv_normalizer_verbalizer_verbalize_number_range(void) {
    static pv_normalizer_use_cases_pt_t use_cases[] = {
            PV_NORMALIZER_USE_NUMBER_RANGE_NORMALIZER_PT,
            PV_NORMALIZER_USE_CARDINAL_NORMALIZER_PT,
            PV_NORMALIZER_USE_DECIMAL_NORMALIZER_PT,
    };

    const char sentence[] = "12-5 1-44 1,1-1,3";
    const char *sentence_verbalized[] = {
            "DOZE",
            "A",
            "CINCO",
            "UM",
            "A",
            "QUARENTA E QUATRO",
            "UM",
            "VÍRGULA",
            "UM",
            "A",
            "UM",
            "VÍRGULA",
            "TRÊS",
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

static void test_pv_normalizer_verbalizer_verbalize_dot_number_range(void) {
    const char *sentence = "1.000-2.000 1.000,2-1.000,5 1,5-10.000";
    const char *sentence_verbalized[] = {
            "MIL",
            "A",
            "DOIS MIL",
            "MIL",
            "VÍRGULA",
            "DOIS",
            "A",
            "MIL",
            "VÍRGULA",
            "CINCO",
            "UM",
            "VÍRGULA",
            "CINCO",
            "A",
            "DEZ MIL",
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

static void test_pv_normalizer_verbalizer_verbalize_number_range_gender(void) {
    static pv_normalizer_use_cases_pt_t use_cases[] = {
            PV_NORMALIZER_USE_NUMBER_RANGE_NORMALIZER_PT,
            PV_NORMALIZER_USE_CARDINAL_NORMALIZER_PT,
            PV_NORMALIZER_USE_DECIMAL_NORMALIZER_PT,
    };

    const char sentence[] = "1-2 abafação 2-1.222 abegoaria";
    const char *sentence_verbalized[] = {
            "UMA",
            "A",
            "DUAS",
            NULL,
            "DUAS",
            "A",
            "MIL DUZENTAS E VINTE E DUAS",
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

static void test_pv_normalizer_verbalizer_verbalize_decimal(void) {
    static pv_normalizer_use_cases_pt_t use_cases[] = {
            PV_NORMALIZER_USE_CARDINAL_NORMALIZER_PT,
            PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_PT,
            PV_NORMALIZER_USE_DECIMAL_NORMALIZER_PT,
    };

    const char *sentence = "1,23 ,5 10,1 -42,42";
    const char *sentence_verbalized[] = {
            "UM",
            "VÍRGULA",
            "DOIS TRÊS",
            "VÍRGULA",
            "CINCO",
            "DEZ",
            "VÍRGULA",
            "UM",
            "MENOS QUARENTA E DOIS",
            "VÍRGULA",
            "QUATRO DOIS",
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

static void test_pv_normalizer_verbalizer_verbalize_dot_decimal(void) {
    static pv_normalizer_use_cases_pt_t use_cases[] = {
            PV_NORMALIZER_USE_CARDINAL_NORMALIZER_PT,
            PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_PT,
            PV_NORMALIZER_USE_DECIMAL_NORMALIZER_PT,
    };

    const char *sentence = "1.000,2 -300.000,6666 1.000.003.000,79";
    const char *sentence_verbalized[] = {
            "MIL",
            "VÍRGULA",
            "DOIS",
            "MENOS TREZENTOS MIL",
            "VÍRGULA",
            "SEIS SEIS SEIS SEIS",
            "MIL MILHÕES E TRÊS MIL",
            "VÍRGULA",
            "SETE NOVE",
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

static void test_pv_normalizer_verbalizer_verbalize_alphanum_spell_out(void) {
    static pv_normalizer_use_cases_pt_t use_cases[] = {
            PV_NORMALIZER_USE_CARDINAL_NORMALIZER_PT,
            PV_NORMALIZER_USE_ALPHANUM_SPELL_OUT_NORMALIZER_PT,
    };

    const char *sentence = "HTML5 C5B 12A Z78";
    const char *sentence_verbalized[] = {
            "H",
            "T",
            "M",
            "L",
            "CINCO",
            NULL,
            "C",
            "CINCO",
            "B",
            NULL,
            "UM",
            "DOIS",
            "A",
            NULL,
            "Z",
            "SETE",
            "OITO",
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

static void test_pv_normalizer_verbalizer_verbalize_fraction(void) {
    static pv_normalizer_use_cases_pt_t use_cases[] = {
            PV_NORMALIZER_USE_CARDINAL_NORMALIZER_PT,
            PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_PT,
            PV_NORMALIZER_USE_DECIMAL_NORMALIZER_PT,
            PV_NORMALIZER_USE_FRACTION_NORMALIZER_PT,
    };

    const char *sentence = "1/2 2/3 9/3,2 1/1.000 1/-4 -4/23 3/14";
    const char *sentence_verbalized[] = {
            "UM",
            NULL,
            "MEIO",
            "DOIS",
            NULL,
            "TERÇOS",
            "NOVE",
            "SOBRE",
            "TRÊS",
            "VÍRGULA",
            "DOIS",
            "UM",
            NULL,
            "MILÉSIMO",
            "UM",
            "SOBRE",
            "MENOS QUATRO",
            "MENOS QUATRO",
            NULL,
            "VINTE E TRÊS AVOS",
            "TRÊS",
            NULL,
            "CATORZE AVOS",
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

static void test_pv_normalizer_verbalizer_verbalize_measurement(void) {
    static pv_normalizer_use_cases_pt_t use_cases[] = {
            PV_NORMALIZER_USE_CARDINAL_NORMALIZER_PT,
            PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_PT,
            PV_NORMALIZER_USE_DECIMAL_NORMALIZER_PT,
            PV_NORMALIZER_USE_MEASUREMENT_NORMALIZER_PT,
    };

    const char sentence[] = "5g 1 ml 7,3L 3 ft 10,1km 5°C 2yd 1 oz -1.000g 25ft.-lb.";
    const char *sentence_verbalized[] = {
            "CINCO",
            "GRAMAS",
            "UM",
            "MILILITRO",
            "SETE",
            "VÍRGULA",
            "TRÊS",
            "LITROS",
            "TRÊS",
            "PÉS",
            "DEZ",
            "VÍRGULA",
            "UM",
            "QUILÓMETROS",
            "CINCO",
            "GRAUS CELSIUS",
            "DUAS",
            "JARDAS",
            "UMA",
            "ONÇA",
            "MENOS MIL",
            "GRAMAS",
            "VINTE E CINCO",
            "PÉS",
            NULL,
            "LIBRAS",
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

static void test_pv_normalizer_verbalizer_verbalize_per_measurement(void) {
    static pv_normalizer_use_cases_pt_t use_cases[] = {
            PV_NORMALIZER_USE_CARDINAL_NORMALIZER_PT,
            PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_PT,
            PV_NORMALIZER_USE_DECIMAL_NORMALIZER_PT,
            PV_NORMALIZER_USE_MEASUREMENT_NORMALIZER_PT,
    };

    const char sentence[] = "5 km/m 10 oz/km 1,1m/l -5,41kg/ft tb/hz 2h/l 1.000.000M/S";
    const char *sentence_verbalized[] = {
            "CINCO",
            "QUILÓMETROS",
            "POR",
            "METRO",
            "DEZ",
            "ONÇAS",
            "POR",
            "QUILÓMETRO",
            "UM",
            "VÍRGULA",
            "UM",
            "METROS",
            "POR",
            "LITRO",
            "MENOS CINCO",
            "VÍRGULA",
            "QUATRO UM",
            "QUILOGRAMAS",
            "POR",
            "PÉ",
            "TERABYTES",
            "POR",
            "HERTZ",
            "DUAS",
            "HORAS",
            "POR",
            "LITRO",
            "UM MILHÃO",
            "METROS",
            "POR",
            "SEGUNDO",
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

static void test_pv_normalizer_verbalizer_verbalize_time(void) {
    static pv_normalizer_use_cases_pt_t use_cases[] = {
            PV_NORMALIZER_USE_CARDINAL_NORMALIZER_PT,
            PV_NORMALIZER_USE_TIME_NORMALIZER_PT,
    };

    const char *sentence = "7:00 09:18 18:23 21:89 25:09 8h45 06h15 14h30 13h66 26h21 2:22 22:30";
    const char *sentence_verbalized[] = {
            "SETE",
            NULL,
            "HORAS",
            NULL,
            "NOVE",
            NULL,
            "E DEZOITO",
            NULL,
            "DEZOITO",
            NULL,
            "E VINTE E TRÊS",
            NULL,
            "VINTE E UM",
            NULL,
            "OITENTA E NOVE",
            NULL,
            "VINTE E CINCO",
            NULL,
            "ZERO NOVE",
            NULL,
            "OITO",
            NULL,
            "E QUARENTA E CINCO",
            NULL,
            "SEIS",
            NULL,
            "E QUINZE",
            NULL,
            "CATORZE",
            NULL,
            "E TRINTA",
            NULL,
            "TREZE",
            NULL,
            "SESSENTA E SEIS",
            NULL,
            "VINTE E SEIS",
            NULL,
            "VINTE E UM",
            NULL,
            "DUAS",
            NULL,
            "E VINTE E DOIS",
            NULL,
            "VINTE E DUAS",
            NULL,
            "E TRINTA",
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

    const char *sentence2 = "14:00 horas";
    const char *sentence_verbalized2[] = {
            "CATORZE",
            NULL,
            NULL,
            "HORAS",
    };

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence2,
            PV_ARRAY_LEN(sentence_verbalized2),
            false,
            sentence_verbalized2);
}

static void test_pv_normalizer_verbalizer_verbalize_special_characters(void) {
    static pv_normalizer_use_cases_pt_t use_cases[] = {PV_NORMALIZER_USE_SPECIAL_CHARACTER_NORMALIZER_PT};

    const char *sentence = "& % @ \n _ ( ) ° º aa²";
    const char *sentence_verbalized[] = {
            "E",
            "POR CENTO",
            "ARROBA",
            ".",
            "SUBLINHADO",
            ",",
            ",",
            "GRAUS",
            "Ó",
            NULL,
            "SOBRESCRITO DOIS",
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

static void test_pv_normalizer_verbalizer_verbalize_invalid_measurement(void) {
    static pv_normalizer_use_cases_pt_t use_cases[] = {
            PV_NORMALIZER_USE_SPECIAL_CHARACTER_NORMALIZER_PT,
            PV_NORMALIZER_USE_MEASUREMENT_NORMALIZER_PT,
    };

    const char sentence[] = "k³ aMM² xcm²A x°C";
    const char *sentence_verbalized[] = {
            NULL,
            "SOBRESCRITO TRÊS",
            NULL,
            "MILÍMETROS QUADRADOS",
            NULL,
            "SOBRESCRITO DOIS",
            NULL,
            NULL,
            "GRAUS CELSIUS",
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

static void test_pv_normalizer_verbalizer_verbalize_url(void) {
    const char *sentence = "www.exemplo.com https://exemplo.ca/";
    const char *sentence_verbalized[] = {
            "W",
            "W",
            "W",
            "PONTO",
            "EXEMPLO",
            "PONTO",
            "COM",
            "H",
            "T",
            "T",
            "P",
            "S",
            "DOIS PONTOS",
            "BARRA",
            "BARRA",
            "EXEMPLO",
            "PONTO",
            "C",
            "A",
            "BARRA",
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
            "$5 €10 10$ 10€ $1,25 1,25$ -$500 -500$ -1,25$ -$1,25 "
            "€1.000 -€1.000 1.000€, -1.000€ €1.000,25 1.000,25€ "
            "¥2 ₪2 £2 ₩2 ₺2 ₱2 ₽2 ฿2 ₹2 ¢2 $1,00 $1,01 5 EUR 1,25 £ -3 €";
    const char *sentence_verbalized[] = {
            "CINCO DÓLARES",
            NULL,
            "DEZ EUROS",
            NULL,
            "DEZ DÓLARES",
            NULL,
            "DEZ EUROS",
            NULL,
            "UM DÓLAR E VINTE E CINCO CENTAVOS",
            NULL,
            "UM DÓLAR E VINTE E CINCO CENTAVOS",
            NULL,
            "MENOS QUINHENTOS DÓLARES",
            NULL,
            "MENOS QUINHENTOS DÓLARES",
            NULL,
            "MENOS UM DÓLAR E VINTE E CINCO CENTAVOS",
            NULL,
            "MENOS UM DÓLAR E VINTE E CINCO CENTAVOS",
            NULL,
            "MIL EUROS",
            NULL,
            "MENOS MIL EUROS",
            NULL,
            "MIL EUROS",
            ",",
            NULL,
            "MENOS MIL EUROS",
            NULL,
            "MIL EUROS E VINTE E CINCO CÊNTIMOS",
            NULL,
            "MIL EUROS E VINTE E CINCO CÊNTIMOS",
            NULL,
            "DOIS YUAN",
            NULL,
            "DOIS SHEKELS",
            NULL,
            "DUAS LIBRAS",
            NULL,
            "DOIS WONS",
            NULL,
            "DUAS LIRAS",
            NULL,
            "DUAS PESOS",
            NULL,
            "DOIS RÚBLOS",
            NULL,
            "DOIS BAHTES",
            NULL,
            "DUAS RUPIAS",
            NULL,
            "DOIS CÊNTIMOS",
            NULL,
            "UM DÓLAR",
            NULL,
            "UM DÓLAR E UM CENTAVO",
            NULL,
            "CINCO",
            NULL,
            "EUROS",
            NULL,
            "UM",
            "VÍRGULA",
            "DOIS CINCO",
            NULL,
            "LIBRAS",
            NULL,
            "MENOS TRÊS",
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

    const char sentence2[] = "$25.000: e $1.200.000.";
    const char *sentence_verbalized2[] = {
            "VINTE E CINCO MIL DÓLARES",
            ":",
            "E",
            "UM MILHÃO E DUZENTOS MIL DÓLARES",
            ".",
    };

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence2,
            PV_ARRAY_LEN(sentence_verbalized2),
            false,
            sentence_verbalized2);
}

static void test_pv_normalizer_verbalizer_verbalize_abbreviation(void) {
    const char *sentence = "ex. dois tel. Sr. Av. R. Dra. S.A. Vol. blvd.";
    const char *sentence_verbalized[] = {
            "POR EXEMPLO",
            NULL,
            "DOIS",
            NULL,
            "TELEFONE",
            NULL,
            "SENHOR",
            NULL,
            "AVENIDA",
            NULL,
            "RUA",
            NULL,
            "DOUTORA",
            NULL,
            "SOCIEDADE ANÔNIMA",
            NULL,
            "VOLUME",
            NULL,
            "BOULEVARD",
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
    const char *sentence = " (772) 778-1923 1-800-123-4567 123-99456-7890 91.456.7890 +351 21.102.1279";
    const char *sentence_verbalized[] = {
            NULL,
            ", SETE SETE DOIS ,",
            NULL,
            "SETE SETE OITO",
            ",",
            "UM NOVE DOIS TRÊS",
            NULL,
            "UM",
            ",",
            "OITO ZERO ZERO",
            ",",
            "UM DOIS TRÊS",
            ",",
            "QUATRO CINCO SEIS SETE",
            NULL,
            "UM DOIS TRÊS",
            ",",
            "NOVE NOVE QUATRO CINCO SEIS",
            ",",
            "SETE OITO NOVE ZERO",
            NULL,
            "NOVE UM",
            ",",
            "QUATRO CINCO SEIS",
            ",",
            "SETE OITO NOVE ZERO",
            NULL,
            "MAIS",
            "TRÊS CINCO UM",
            NULL,
            "DOIS UM",
            ",",
            "UM ZERO DOIS",
            ",",
            "UM DOIS SETE NOVE",
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
    const char *sentence = "03-Jun-2020 08/02/1877 2013-11-28 13/01/1992 1 Ago 32-10-2024 2 Out 1800 09-21-1907 Jul 2024";
    const char *sentence_verbalized[] = {
            "TRÊS DE",
            NULL,
            "JUNHO DE",
            NULL,
            "DOIS MIL E VINTE",
            NULL,
            "OITO DE",
            NULL,
            "FEVEREIRO DE",
            NULL,
            "MIL OITOCENTOS E SETENTA E SETE",
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            "VINTE E OITO DE NOVEMBRO DE DOIS MIL E TREZE",
            NULL,
            "TREZE DE",
            NULL,
            "JANEIRO DE",
            NULL,
            "MIL NOVECENTOS E NOVENTA E DOIS",
            NULL,
            "UM DE",
            NULL,
            "AGOSTO",
            NULL,
            "TRÊS DOIS",
            ",",
            "UM ZERO",
            ",",
            "DOIS ZERO DOIS QUATRO",
            NULL,
            "DOIS DE",
            NULL,
            "OUTUBRO DE",
            NULL,
            "MIL E OITOCENTOS",
            NULL,
            NULL,
            NULL,
            "VINTE E UM DE SETEMBRO DE",
            NULL,
            "MIL NOVECENTOS E SETE",
            NULL,
            "JULHO DE",
            NULL,
            "DOIS MIL E VINTE E QUATRO",
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

static void test_pv_normalizer_verbalizer_verbalize_cardinal_in_parentheses(void) {
    const char *sentence = "Everest (8.848 metros)?";
    const char *sentence_verbalized[] = {
            "EVEREST",
            ",",
            "OITO MIL OITOCENTOS E QUARENTA E OITO",
            "METROS",
            ",",
            "?",
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

static void test_pv_normalizer_verbalizer_verbalize_temperature_after_range(void) {
    const char *sentence = "Temperaturas entre 10-15°C. (cerca de 22°C).";
    const char *sentence_verbalized[] = {
            "TEMPERATURAS",
            "ENTRE",
            "DEZ",
            "A",
            "QUINZE",
            "GRAUS CELSIUS",
            ".",
            ",",
            "CERCA",
            "DE",
            "VINTE E DOIS",
            "GRAUS CELSIUS",
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
    const char *sentence = "Ela disse, 'como você está.' 'OLÁ%'";
    const char *sentence_verbalized[] = {
            "ELA",
            "DISSE",
            ",",
            "'COMO",
            "VOCÊ",
            "ESTÁ",
            ".",
            "\"",
            "'OLÁ",
            "POR CENTO",
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

static void *calloc_return_null(size_t arg0, size_t arg1) {
    (void) arg0;
    (void) arg1;
    return NULL;
}

static void test_pv_normalizer_verbalizer_init_failure_helper(
        pv_status_t expected_status,
        const char *expected_public_message_regex,
        const char *expected_private_message_regex) {
    pv_normalizer_verbalizer_t *normalizer_verbalizer = NULL;
    pv_status_t status = pv_normalizer_verbalizer_init(
            PV_NORMALIZER_LANGUAGE_PT,
            PV_ARRAY_LEN(ALL_TEST_USE_CASES),
            (const int32_t *) ALL_TEST_USE_CASES,
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
        pv_normalizer_token_tag_pt_t tag) {
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
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_verbalizer_pt_init, PV_STATUS_INVALID_ARGUMENT)

    test_pv_normalizer_verbalizer_init_failure_helper(PV_STATUS_INVALID_ARGUMENT, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_pt_init` failed with status `INVALID_ARGUMENT`\\.");
}

static void test_pv_normalizer_verbalizer_verbalize_word_fail_calloc(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "hello";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_PT_WORD);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_verbalize_word` failed with status `OUT_OF_MEMORY`.");
}

static void test_pv_normalizer_verbalizer_verbalize_punctuation_fail_calloc(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = ",";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_PT_PUNCTUATION);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_verbalize_punctuation` failed with status `OUT_OF_MEMORY`.");
}

static void test_pv_normalizer_verbalizer_verbalize_cardinal_fail_calloc(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "5";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_PT_CARDINAL);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_cardinal_to_string` failed with status `OUT_OF_MEMORY`.");
}

static void test_pv_normalizer_verbalizer_verbalize_negative_cardinal_fail_calloc(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "-5";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_PT_NEGATIVE_CARDINAL);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `result`\\.");
}

static void test_pv_normalizer_verbalizer_verbalize_negative_cardinal_fail_calloc_2(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "-5";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_PT_NEGATIVE_CARDINAL);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_real,
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `token_verbalized`.");
}

static void test_pv_normalizer_verbalizer_verbalize_number_range_fail_calloc_1(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "1-5";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_PT_NUMBER_RANGE_TO);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `token_verbalized`.");
}

static void test_pv_normalizer_verbalizer_verbalize_ordinal_fail_calloc_1(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "1.a";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_PT_ORDINAL);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `ordinal_string`.");
}

static void test_pv_normalizer_verbalizer_verbalize_ordinal_fail_calloc_2(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "1.o";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_PT_ORDINAL);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_real,
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `result`.");
}

static void test_pv_normalizer_verbalizer_verbalize_negative_ordinal_fail_calloc_1(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "-1º";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_PT_NEGATIVE_ORDINAL);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `ordinal_string`.");
}

static void test_pv_normalizer_verbalizer_verbalize_negative_ordinal_fail_calloc_2(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "-1ª";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_PT_NEGATIVE_ORDINAL);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_real,
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `result`.");
}

static void test_pv_normalizer_verbalizer_verbalize_negative_ordinal_fail_calloc_3(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "-1º";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_PT_NEGATIVE_ORDINAL);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
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
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_PT_CURRENCY);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `number_string`.");
}

static void test_pv_normalizer_verbalizer_verbalize_currency_fail_calloc_2(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "$1";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_PT_CURRENCY);

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
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_PT_NEGATIVE_CURRENCY);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `number_string`.");
}

static void test_pv_normalizer_verbalizer_verbalize_negative_currency_fail_calloc_2(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "-$1";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_PT_NEGATIVE_CURRENCY);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_real,
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `result`.");
}

#endif


static const pv_test_case_t PV_NORMALIZER_VERBALIZER_TEST_CASES[] = {
        {"verbalize_invalid_use_case", test_pv_normalizer_verbalizer_verbalize_invalid_use_case},
        {"verbalize_word", test_pv_normalizer_verbalizer_verbalize_word},
        {"verbalize_punctuation", test_pv_normalizer_verbalizer_verbalize_punctuation},
        {"verbalize_custom_pronunciation", test_pv_normalizer_verbalizer_verbalize_custom_pronunciation},
        {"verbalize_cardinal", test_pv_normalizer_verbalizer_verbalize_cardinal},
        {"verbalize_negative_cardinal", test_pv_normalizer_verbalizer_verbalize_negative_cardinal},
        {"verbalize_cardinal_gender", test_pv_normalizer_verbalizer_verbalize_cardinal_gender},
        {"verbalize_dot_cardinal", test_pv_normalizer_verbalizer_verbalize_dot_cardinal},
        {"verbalize_dot_cardinal_gender", test_pv_normalizer_verbalizer_verbalize_dot_cardinal_gender},
        {"verbalize_ordinal", test_pv_normalizer_verbalizer_verbalize_ordinal},
        {"verbalize_negative_ordinal", test_pv_normalizer_verbalizer_verbalize_negative_ordinal},
        {"verbalize_invalid_ordinal", test_pv_normalizer_verbalizer_verbalize_invalid_ordinal},
        {"verbalize_number_range", test_pv_normalizer_verbalizer_verbalize_number_range},
        {"verbalize_dot_number_range", test_pv_normalizer_verbalizer_verbalize_dot_number_range},
        {"verbalize_number_range_gender", test_pv_normalizer_verbalizer_verbalize_number_range_gender},
        {"verbalize_decimal", test_pv_normalizer_verbalizer_verbalize_decimal},
        {"verbalize_dot_decimal", test_pv_normalizer_verbalizer_verbalize_dot_decimal},
        {"verbalize_alphanum_spell_out", test_pv_normalizer_verbalizer_verbalize_alphanum_spell_out},
        {"verbalize_fraction", test_pv_normalizer_verbalizer_verbalize_fraction},
        {"verbalize_measurement", test_pv_normalizer_verbalizer_verbalize_measurement},
        {"verbalize_per_measurement", test_pv_normalizer_verbalizer_verbalize_per_measurement},
        {"verbalize_time", test_pv_normalizer_verbalizer_verbalize_time},
        {"verbalize_special_characters", test_pv_normalizer_verbalizer_verbalize_special_characters},
        {"verbalize_invalid_measurement", test_pv_normalizer_verbalizer_verbalize_invalid_measurement},
        {"verbalize_url", test_pv_normalizer_verbalizer_verbalize_url},
        {"verbalize_currency", test_pv_normalizer_verbalizer_verbalize_currency},
        {"verbalize_abbreviation", test_pv_normalizer_verbalizer_verbalize_abbreviation},
        {"verbalize_digits_sequence", test_pv_normalizer_verbalizer_verbalize_digits_sequence},
        {"verbalize_date", test_pv_normalizer_verbalizer_verbalize_date},
        {"verbalize_name", test_pv_normalizer_verbalizer_verbalize_name},
        {"verbalize_cardinal_in_parentheses", test_pv_normalizer_verbalizer_verbalize_cardinal_in_parentheses},
        {"verbalize_temperature_after_range", test_pv_normalizer_verbalizer_verbalize_temperature_after_range},
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

const pv_test_suite_t PV_NORMALIZER_VERBALIZER_PT_TEST_SUITE = {
        .name = "normalizer_verbalizer_pt",
        .setup = test_pv_normalizer_verbalizer_setup,
        .teardown = test_pv_normalizer_verbalizer_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_NORMALIZER_VERBALIZER_TEST_CASES),
        .test_cases = PV_NORMALIZER_VERBALIZER_TEST_CASES,
};
