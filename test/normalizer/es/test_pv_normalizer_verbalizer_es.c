#include <stdlib.h>
#include <string.h>

#include "core/pv_language.h"
#include "core/pv_language_json.h"
#include "mock/pv_mock.h"
#include "orca/normalizer/es/pv_normalizer_tags_es.h"
#include "orca/normalizer/es/pv_normalizer_verbalizer_es.h"
#include "orca/normalizer/pv_normalizer_tagger.h"
#include "orca/normalizer/pv_normalizer_tokenizer.h"
#include "orca/normalizer/pv_normalizer_verbalizer.h"
#include "test/pv_test.h"

#ifdef __PV_MOCKS__

#include "core/mock/pv_core_runtime_mock.h"
#include "orca/mock/pv_normalizer_mock.h"

#endif

static pv_normalizer_verbalizer_t *normalizer_verbalizer_object = NULL;
static pv_normalizer_language_t LANGUAGE = PV_NORMALIZER_LANGUAGE_ES;
static int32_t ALL_TEST_USE_CASES[] = {
        PV_NORMALIZER_USE_WORD_NORMALIZER_ES,
        PV_NORMALIZER_USE_PUNCTUATION_NORMALIZER_ES,
        PV_NORMALIZER_USE_CARDINAL_NORMALIZER_ES,
        PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_ES,
        PV_NORMALIZER_USE_CUSTOM_PRONUNCIATION_NORMALIZER_ES,
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

static pv_normalizer_tagger_t *normalizer_tagger_object = NULL;
static pv_normalizer_tokenizer_t *normalizer_tokenizer_object = NULL;
static pv_language_info_t *language_info_object = NULL;
static pv_noun_gender_dict_t *noun_gender_dict_object = NULL;

static const char LANGUAGE_INFO_PATH[] = "normalizer/ref/pv_language_info_normalizer_es.json";
static const char NOUN_GENDER_DICT_PATH[] = "test_data/noun_gender_dict/noun_gender_dict_es.txt";

static const char VERBALIZER_TEST_SENTENCE[] = "¡mañana tomaré vitamina b-12, c, y 15 más!";
static const char *VERBALIZER_TEST_SENTENCE_VERBALIZED[] = {
        "¡",
        "MAÑANA",
        "TOMARÉ",
        "VITAMINA",
        "B",
        "DOCE",
        ",",
        "C",
        ",",
        "Y",
        "QUINCE",
        "MÁS",
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
    int32_t use_cases[] = {PV_NORMALIZER_USE_WORD_NORMALIZER_ES};
    static const char sentence[] = "¡mañana tomaré vitamina b-12, c, y 15 más! vita’mina";

    const char *sentence_verbalized[] = {
            NULL,
            "MAÑANA",
            "TOMARÉ",
            "VITAMINA",
            "B",
            NULL,
            NULL,
            "C",
            NULL,
            "Y",
            NULL,
            "MÁS",
            NULL,
            "VITA'MINA",
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
    int32_t use_cases[] = {PV_NORMALIZER_USE_PUNCTUATION_NORMALIZER_ES};

    const char *sentence_verbalized[] = {
            "¡",
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

    const char sentence[] = "La pronunciación {personalizada|p e ɾ s o n a l i θ a ð a}";
    const char *sentence_verbalized[] = {NULL, NULL, "{p e ɾ s o n a l i θ a ð a}"};

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
            "DOCE",
            NULL,
            NULL,
            NULL,
            NULL,
            "QUINCE",
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
    const char *sentence = "36 130 0 003 456 123456789 5242 999999999999999 1200000000000009 11111111111111111";
    const char *sentence_verbalized[] = {
            "TREINTA Y SEIS",
            "CIENTO TREINTA",
            "CERO",
            "CERO CERO TRES",
            "CUATROCIENTOS CINCUENTA Y SEIS",
            "CIENTO VEINTITRÉS MILLONES CUATROCIENTOS CINCUENTA Y SEIS MIL SETECIENTOS OCHENTA Y NUEVE",
            "CINCO MIL DOSCIENTOS CUARENTA Y DOS",
            "NOVECIENTOS NOVENTA Y NUEVE BILLONES NOVECIENTOS NOVENTA Y NUEVE MIL NOVECIENTOS NOVENTA Y NUEVE MILLONES NOVECIENTOS NOVENTA Y NUEVE MIL NOVECIENTOS NOVENTA Y NUEVE",
            "UNO DOS CERO CERO CERO CERO CERO CERO CERO CERO CERO CERO CERO CERO CERO NUEVE",
            "UNO UNO UNO UNO UNO UNO UNO UNO UNO UNO UNO UNO UNO UNO UNO UNO UNO"};

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
    int32_t use_cases[] = {PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_ES};

    const char sentence[] = "123 -123";
    const char *sentence_verbalized[] = {NULL, "NEGATIVO CIENTO VEINTITRÉS"};

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
            "DOCE",
            "A",
            "CINCO",
            "UNO",
            "A",
            "CUARENTA Y CUATRO",
            "UNO",
            "COMA",
            "UNO",
            "A",
            "UNO",
            "COMA",
            "TRES",
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
    int32_t use_cases[] = {
            PV_NORMALIZER_USE_ORDINAL_NORMALIZER,
            PV_NORMALIZER_USE_WORD_NORMALIZER,
            PV_NORMALIZER_USE_CARDINAL_NORMALIZER,
            PV_NORMALIZER_USE_PUNCTUATION_NORMALIZER,
    };

    const char *sentence =
            "1ro 22da 19no 200mo 4000ma 1000000mo 6000000000mo 5000000000000mo 123456789no";
    const char *sentence_verbalized[] = {
            "PRIMERO",
            NULL,
            "VIGÉSIMO SEGUNDA",
            NULL,
            "DECIMONOVENO",
            NULL,
            "DUCENTÉSIMO",
            NULL,
            "CUATRO MILÉSIMA",
            NULL,
            "MILLONÉSIMO",
            NULL,
            "SEIS MIL MILLONÉSIMO",
            NULL,
            "CINCO BILLONÉSIMO",
            NULL,
            "CIENTO VEINTITRÉS MILLONES CUATROCIENTOS CINCUENTA Y SEIS MIL SETECIENTOS OCHENTA Y NOVENO",
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

static void test_pv_normalizer_verbalizer_verbalize_invalid_ordinal(void) {
    static int32_t use_cases[] = {PV_NORMALIZER_USE_ORDINAL_NORMALIZER};

    char *sentences[] = {"01ro", "1200000000000009no"};

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
    int32_t use_cases[] = {PV_NORMALIZER_USE_NEGATIVE_ORDINAL_NORMALIZER};

    const char *sentence = "-1ro -22da -5000000000000mo -123456789no";
    const char *sentence_verbalized[] = {
            "PRIMERO NEGATIVO",
            "VIGÉSIMO SEGUNDA NEGATIVA",
            "CINCO BILLONÉSIMO NEGATIVO",
            "CIENTO VEINTITRÉS MILLONES CUATROCIENTOS CINCUENTA Y SEIS MIL SETECIENTOS OCHENTA Y NOVENO NEGATIVO",
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

static void test_pv_normalizer_verbalizer_verbalize_special_characters(void) {
    int32_t use_cases[] = {PV_NORMALIZER_USE_SPECIAL_CHARACTER_NORMALIZER};

    const char *sentence = "& % @ \n _ ( ) °C RPM KM²";
    const char *sentence_verbalized[] = {
            "Y",
            "POR CIENTO",
            "ARROBA",
            ".",
            "GUION BAJO",
            ",",
            ",",
            "GRADOS CELSIUS",
            "REVOLUCIONES POR MINUTO",
            "KILÓMETROS CUADRADOS"};

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
            "UNO",
            "COMA",
            "VEINTITRÉS",
            "COMA",
            "CINCO",
            "DIEZ",
            "COMA",
            "UNO",
            "POR CIENTO",
            "NEGATIVO DOS",
            "COMA",
            "DOS"};

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
    const char sentence[] = "5g 1 ml 7,3L 3 ft 10,1km (38°C) 12-14°C 1.000.000g 25ft.-lb.";
    const char *sentence_verbalized[] = {
            "CINCO",
            "GRAMOS",
            "UN",
            "MILILITRO",
            "SIETE",
            "COMA",
            "TRES",
            "LITROS",
            "TRES",
            "PIES",
            "DIEZ",
            "COMA",
            "UNO",
            "KILÓMETROS",
            ",",
            "TREINTA Y OCHO",
            "GRADOS CELSIUS",
            ",",
            "DOCE",
            "A",
            "CATORCE",
            "GRADOS CELSIUS",
            "UN MILLÓN",
            "GRAMOS",
            "VEINTICINCO",
            "PIE",
            NULL,
            "LIBRAS",
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
    const char sentence[] = "5 km/m 10 oz/km 1,1m/l -5,41kg/ft tb/hz 1.000m/s";
    const char *sentence_verbalized[] = {
            "CINCO",
            "KILÓMETROS",
            "POR",
            "METRO",
            "DIEZ",
            "ONZAS",
            "POR",
            "KILÓMETRO",
            "UNO",
            "COMA",
            "UNO",
            "METROS",
            "POR",
            "LITRO",
            "NEGATIVO CINCO",
            "COMA",
            "CUARENTA Y UNO",
            "KILOGRAMOS",
            "POR",
            "PIE",
            "TERABYTES",
            "POR",
            "HERCIO",
            "MIL",
            "METROS",
            "POR",
            "SEGUNDO",
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
    const char *sentence = "HTML5 C5B 12A Z78 ÓL1";
    const char *sentence_verbalized[] = {
            "H", "T", "M", "L", "CINCO",
            NULL,
            "C", "CINCO", "B",
            NULL,
            "UNO", "DOS", "A",
            NULL,
            "Z", "SIETE", "OCHO",
            NULL,
            "Ó", "L", "UNO"};

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
    const char *sentence = "7:07 0h31 12h59' 9h11m 11:15am 3:25 pm 8:00 p.m. 4am AM";
    const char *sentence_verbalized[] = {
            "SIETE",
            NULL,
            "Y SIETE",
            NULL,
            "CERO",
            NULL,
            "Y TREINTA Y UNO",
            NULL,
            "DOCE",
            NULL,
            "Y CINCUENTA Y NUEVE",
            NULL,
            "NUEVE",
            NULL,
            "Y ONCE",
            NULL,
            "ONCE",
            NULL,
            "Y QUINCE",
            "DE LA MAÑANA",
            NULL,
            "TRES",
            NULL,
            "Y VEINTICINCO",
            NULL,
            "DE LA TARDE",
            NULL,
            "OCHO",
            NULL,
            NULL,
            NULL,
            "DE LA NOCHE",
            NULL,
            "CUATRO",
            "DE LA MADRUGADA",
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
            "MIL CINCO",
            "ONCE MIL CINCO",
            "CIENTO ONCE MIL CINCO",
            "NEGATIVO CINCO MIL MILLONES",
            "MIL CIENTO ONCE",
            "PUNTO",
            "CERO CERO CERO",
            "UNO",
            "PUNTO",
            "UNO",
            "PUNTO",
            "CERO CERO CERO",
            "UNO",
            ".",
            "CERO CERO CERO",
            ",",
            "TRESCIENTOS OCHENTA Y CUATRO MIL CUATROCIENTOS",
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
    const char *sentence = "1.005to 11.005to 111.005to -5.003.000.003ro 1111.100mo 1.1.100mo 1. 100mo";
    const char *sentence_verbalized[] = {
            "MIL QUINTO",
            "ONCE MIL QUINTO",
            "CIENTO ONCE MIL QUINTO",
            "CINCO MIL TRES MILLONES TERCERO NEGATIVO",
            "MIL CIENTO ONCE",
            "PUNTO",
            "CENTÉSIMO",
            "UNO",
            "PUNTO",
            "UNO",
            "PUNTO",
            "CENTÉSIMO",
            "UNO",
            ".",
            "CENTÉSIMO",
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
            "MIL",
            "COMA",
            "DOS",
            "NEGATIVO TRESCIENTOS MIL",
            "COMA",
            "SEIS MIL SEISCIENTOS SESENTA Y SEIS",
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
            "MIL",
            "A",
            "DOS MIL",
            "MIL",
            "COMA",
            "DOS",
            "A",
            "MIL",
            "COMA",
            "CINCO",
            "DIEZ MIL",
            "A",
            "UNO",
            "COMA",
            "TRES",
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
            "MEDIO",
            ",",
            "NUEVE",
            "SOBRE",
            "TRES",
            "COMA",
            "DOS",
            "UN",
            NULL,
            "MILÉSIMO",
            "UNO",
            "SOBRE",
            "NEGATIVO CUATRO",
            "NEGATIVO CUATRO",
            NULL,
            "TERCIOS",
            "UN",
            NULL,
            "MIL UNAVO",
            "UN",
            NULL,
            "DIEZ MILÉSIMO"};

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
    const char *sentence = "1/3 parte de la torta";
    const char *sentence_verbalized[] = {
            "UNA",
            NULL,
            "TERCIA",
            "PARTE",
            "DE",
            "LA",
            "TORTA"};

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
            "W",
            "W",
            "W",
            "PUNTO",
            "EJEMPLO",
            "PUNTO",
            "COM",
            "H",
            "T",
            "T",
            "P",
            "S",
            "DOS PUNTOS",
            "DIAGONAL",
            "DIAGONAL",
            "HOLA",
            "PUNTO",
            "C",
            "A",
            "DIAGONAL",
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
            "CINCO DÓLARES",
            NULL,
            "DIEZ EUROS",
            NULL,
            "DIEZ DÓLARES",
            NULL,
            "DIEZ EUROS",
            NULL,
            "UN DÓLAR CON VEINTICINCO CENTAVOS",
            NULL,
            "UN DÓLAR CON VEINTICINCO CENTAVOS",
            NULL,
            "NEGATIVO QUINIENTOS DÓLARES",
            NULL,
            "NEGATIVO QUINIENTOS DÓLARES",
            NULL,
            "NEGATIVO UN DÓLAR CON VEINTICINCO CENTAVOS",
            NULL,
            "NEGATIVO UN DÓLAR CON VEINTICINCO CENTAVOS",
            NULL,
            "MIL EUROS",
            NULL,
            "NEGATIVO MIL EUROS",
            NULL,
            "MIL EUROS",
            NULL,
            "NEGATIVO MIL EUROS",
            NULL,
            "MIL EUROS CON VEINTICINCO CÉNTIMOS",
            NULL,
            "MIL EUROS CON VEINTICINCO CÉNTIMOS",
            NULL,
            "DOS YUANES",
            NULL,
            "DOS SHEKELS",
            NULL,
            "DOS LIBRAS",
            NULL,
            "DOS WON",
            NULL,
            "DOS LIRAS",
            NULL,
            "DOS PESOS",
            NULL,
            "DOS RUBLOS",
            NULL,
            "DOS BAHTS",
            NULL,
            "DOS GRIVNAS",
            NULL,
            "DOS RUPIAS",
            NULL,
            "DOS CENTAVOS",
            NULL,
            "UN DÓLAR",
            NULL,
            "UN DÓLAR CON UN CENTAVO",
            NULL,
            "CINCO",
            NULL,
            "EUROS",
            NULL,
            "UNO",
            "COMA",
            "VEINTICINCO",
            NULL,
            "LIBRAS",
            NULL,
            "NEGATIVO TRES",
            NULL,
            "EUROS",
            NULL,
            "CIEN MIL EUROS",
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
    const char *sentence = " p.ej. este Prof. Dr. Doe Av. C/ vs. etc. d.C.";
    const char *sentence_verbalized[] = {
            NULL,
            "POR EJEMPLO",
            NULL,
            "ESTE",
            NULL,
            "PROFESOR",
            NULL,
            "DOCTOR",
            NULL,
            "DOE",
            NULL,
            "AVENIDA",
            NULL,
            "CALLE",
            NULL,
            "VERSUS",
            NULL,
            "ETCÉTERA",
            NULL,
            "DESPUÉS DE CRISTO",
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
            ", SIETE SIETE DOS,",
            NULL,
            "SIETE SIETE OCHO",
            ",",
            "UNO NUEVE DOS TRES",
            NULL,
            "UNO",
            ",",
            "OCHO CERO CERO",
            ",",
            "UNO DOS TRES",
            ",",
            "CUATRO CINCO SEIS SIETE",
            NULL,
            "UNO DOS TRES",
            ",",
            "NUEVE NUEVE CUATRO CINCO SEIS",
            ",",
            "SIETE OCHO NUEVE CERO",
            NULL,
            "MÁS",
            "UNO",
            NULL,
            ", TRES OCHO UNO,",
            NULL,
            "UNO CERO DOS",
            ",",
            "UNO DOS NUEVE",
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
    const char *sentence = "03-Jun-2020 08/02/1877 2013-11-28 13/01/1992 1 Ago 32-10-2024 2 Oct 1800 09-21-1907";
    const char *sentence_verbalized[] = {
            "TRES DE",
            NULL,
            "JUNIO DE",
            NULL,
            "DOS MIL VEINTE",
            NULL,
            "OCHO DE",
            NULL,
            "FEBRERO DE",
            NULL,
            "MIL OCHOCIENTOS SETENTA Y SIETE",
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            "VEINTIOCHO DE NOVIEMBRE DE DOS MIL TRECE",
            NULL,
            "TRECE DE",
            NULL,
            "ENERO DE",
            NULL,
            "MIL NOVECIENTOS NOVENTA Y DOS",
            NULL,
            "PRIMERO DE",
            NULL,
            "AGOSTO",
            NULL,
            "TRES DOS",
            ",",
            "UNO CERO",
            ",",
            "DOS CERO DOS CUATRO",
            NULL,
            "DOS DE",
            NULL,
            "OCTUBRE DE",
            NULL,
            "MIL OCHOCIENTOS",
            NULL,
            NULL,
            NULL,
            "VEINTIUNO DE SEPTIEMBRE DE",
            NULL,
            "MIL NOVECIENTOS SIETE",
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
    const char *sentence = "51% 1 persona = 2,54 cm 1h y 15 minutos 493 personas 493000 personas 200.200.000 personas 276.841 km²";
    const char *sentence_verbalized[] = {
            "CINCUENTA Y UN",
            "POR CIENTO",
            "UNA",
            "PERSONA",
            "ES IGUAL A",
            "DOS",
            "COMA",
            "CINCUENTA Y CUATRO",
            "CENTÍMETROS",
            "UNA",
            "HORA",
            "Y",
            "QUINCE",
            "MINUTOS",
            "CUATROCIENTAS NOVENTA Y TRES",
            "PERSONAS",
            "CUATROCIENTAS NOVENTA Y TRES MIL",
            "PERSONAS",
            "DOSCIENTOS MILLONES DOSCIENTAS MIL",
            "PERSONAS",
            "DOSCIENTOS SETENTA Y SEIS MIL OCHOCIENTOS CUARENTA Y UN",
            "KILÓMETROS CUADRADOS",
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
    const char *sentence = "Diga, 'asegúrate de llegar a tiempo?' 'Hola&'";
    const char *sentence_verbalized[] = {
            "DIGA",
            ",",
            "'ASEGÚRATE",
            "DE",
            "LLEGAR",
            "A",
            "TIEMPO",
            "?",
            "\"",
            "'HOLA",
            "Y",
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
            PV_NORMALIZER_LANGUAGE_ES,
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
        pv_normalizer_token_tag_es_t tag) {
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
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_verbalizer_es_init, PV_STATUS_INVALID_ARGUMENT)

    test_pv_normalizer_verbalizer_init_failure_helper(PV_STATUS_INVALID_ARGUMENT, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_es_init` failed with status `INVALID_ARGUMENT`\\.");
}

static void test_pv_normalizer_verbalizer_verbalize_word_fail_calloc(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "hello";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_ES_WORD);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_verbalize_word` failed with status `OUT_OF_MEMORY`.");
}

static void test_pv_normalizer_verbalizer_verbalize_punctuation_fail_calloc(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = ",";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_ES_PUNCTUATION);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_verbalize_punctuation` failed with status `OUT_OF_MEMORY`.");
}

static void test_pv_normalizer_verbalizer_verbalize_cardinal_fail_calloc(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "5";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_ES_CARDINAL);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_verbalize_cardinal` failed with status `OUT_OF_MEMORY`.");
}

static void test_pv_normalizer_verbalizer_verbalize_negative_cardinal_fail_calloc(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "-5";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_ES_NEGATIVE_CARDINAL);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_verbalize_negative_cardinal` failed with status `OUT_OF_MEMORY`.");
}

static void test_pv_normalizer_verbalizer_verbalize_negative_cardinal_fail_calloc_2(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "-5";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_ES_NEGATIVE_CARDINAL);

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
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_ES_NUMBER_RANGE_TO);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_verbalize_number_range` failed with status `OUT_OF_MEMORY`.");
}

static void test_pv_normalizer_verbalizer_verbalize_ordinal_fail_calloc_1(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "1st";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_ES_ORDINAL);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `number_string`.");
}

static void test_pv_normalizer_verbalizer_verbalize_ordinal_fail_calloc_2(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "1st";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_ES_ORDINAL);

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
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_ES_NEGATIVE_ORDINAL);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `number_string`.");
}

static void test_pv_normalizer_verbalizer_verbalize_negative_ordinal_fail_calloc_2(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "-1st";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_ES_NEGATIVE_ORDINAL);

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
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_ES_NEGATIVE_ORDINAL);

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
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_ES_CURRENCY);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `number_string`.");
}

static void test_pv_normalizer_verbalizer_verbalize_currency_fail_calloc_2(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "$1";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_ES_CURRENCY);

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
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_ES_NEGATIVE_CURRENCY);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `number_string`.");
}

static void test_pv_normalizer_verbalizer_verbalize_negative_currency_fail_calloc_2(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "-$1";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_ES_NEGATIVE_CURRENCY);

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

const pv_test_suite_t PV_NORMALIZER_VERBALIZER_ES_TEST_SUITE = {
        .name = "normalizer_verbalizer_es",
        .setup = test_pv_normalizer_verbalizer_setup,
        .teardown = test_pv_normalizer_verbalizer_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_NORMALIZER_VERBALIZER_TEST_CASES),
        .test_cases = PV_NORMALIZER_VERBALIZER_TEST_CASES,
};
