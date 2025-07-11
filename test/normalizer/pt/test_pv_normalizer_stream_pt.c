#include <stdlib.h>
#include <string.h>

#include "core/picovoice.h"
#include "core/pv_language.h"
#include "core/pv_language_json.h"
#include "mock/pv_mock.h"
#include "orca/normalizer/pv_normalizer.h"
#include "orca/normalizer/pv_normalizer_stream.h"
#include "test/pv_test.h"

#include "orca/normalizer/pt/pv_normalizer_tags_pt.h"
#include "orca/normalizer/test_pv_normalizer_stream_helper.h"

#ifdef __PV_MOCKS__

#include "core/mock/pv_core_runtime_mock.h"
#include "orca/mock/pv_normalizer_mock.h"

#endif

static pv_normalizer_stream_t *normalizer_stream_object = NULL;
static pv_normalizer_t *normalizer_object = NULL;
static pv_noun_gender_dict_t *noun_gender_dict_object = NULL;
static pv_language_info_t *language_info_object = NULL;

static const char LANGUAGE_INFO_PATH[] = "normalizer/ref/pv_language_info_normalizer_pt.json";
static const char NOUN_GENDER_DICT_PATH[] = "test_data/noun_gender_dict/noun_gender_dict_pt.txt";

static pv_status_t test_pv_normalizer_stream_setup(void) {
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

    status = pv_normalizer_init(language_info_object, noun_gender_dict_object, NULL, &normalizer_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_normalizer_stream_open(normalizer_object, &normalizer_stream_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_normalizer_stream_teardown(void) {
    pv_noun_gender_dict_delete(noun_gender_dict_object);
    pv_normalizer_stream_close(normalizer_stream_object);
    pv_normalizer_delete(normalizer_object);
    pv_language_info_delete(language_info_object);
}

static void test_pv_normalizer_stream_verbalizable_basic(void) {
    char *tokens[][2] = {
            {"Eu", " vou"},
    };
    char *expected_phonemizable_text[][3] = {
            {NULL, "Eu ", "vou"},
    };

    for (size_t i = 0; i < PV_ARRAY_LEN(tokens); i++) {
        pv_normalizer_stream_reset(normalizer_stream_object);
        test_pv_normalizer_stream_helper_verbalizable(
                normalizer_stream_object,
                2,
                (char **) &tokens[i],
                (char **) &expected_phonemizable_text[i]);
    }
}

static void test_pv_normalizer_stream_verbalizable_punctuation(void) {
    char *tokens[][4] = {
            {"Olá", ".", " ", "Tudo bem"},
            {"Olá", ". ", "Tudo", " bem"},
    };
    char *expected_phonemizable_text[][5] = {
            {NULL, NULL, "Olá. ", "Tudo ", "bem"},
            {NULL, "Olá. ", NULL, "Tudo ", "bem"},
    };

    for (size_t i = 0; i < PV_ARRAY_LEN(tokens); i++) {
        pv_normalizer_stream_reset(normalizer_stream_object);
        test_pv_normalizer_stream_helper_verbalizable(
                normalizer_stream_object,
                4,
                (char **) &tokens[i],
                (char **) &expected_phonemizable_text[i]);
    }
}

static void test_pv_normalizer_stream_verbalizable_decimals(void) {
    char *tokens[][4] = {
            {"1", ",", "5", "%"},
            {"-1", ",", "5", "%"},
            {"1", ",", " ", "5"},
    };
    char *expected_phonemizable_text[][5] = {
            {NULL, NULL, NULL, "1,5%", NULL},
            {NULL, NULL, NULL, "-1,5%", NULL},
            {NULL, NULL, "1, ", NULL, "5"},
    };

    for (size_t i = 0; i < PV_ARRAY_LEN(tokens); i++) {
        pv_normalizer_stream_reset(normalizer_stream_object);
        test_pv_normalizer_stream_helper_verbalizable(
                normalizer_stream_object,
                4,
                (char **) &tokens[i],
                (char **) &expected_phonemizable_text[i]);
    }
}

static void test_pv_normalizer_stream_custom_verbalizable_pron(void) {
    char *tokens[][4] = {
            {"15", " rapazes", " num", " carro"},
            {"pronúncia", " {personalizado|p e}", "!", ""},
            {"{", "personalizado|", "p e tʃ", "}"},
            {"oi {personalizado", "|", "p e", " tʃ}"},
            {"oi {persona", "lizado|", "p e tʃ}", " palavra"},
            {"{persona", "lizado", "|p e tʃ}", " palavra"},
            {"{persona", "lizado|p e t", "ʃ}", " palavra"},
    };
    char *expected_phonemizable_text[][5] = {
            {NULL, NULL, "15 rapazes ", "num ", "carro"},
            {NULL, "pronúncia {personalizado|p e}", "!", NULL, NULL},
            {NULL, NULL, NULL, "{personalizado|p e tʃ}", NULL},
            {"oi ", NULL, NULL, "{personalizado|p e tʃ}", NULL},
            {"oi ", NULL, "{personalizado|p e tʃ}", " ", "palavra"},
            {NULL, NULL, "{personalizado|p e tʃ}", " ", "palavra"},
            {NULL, NULL, "{personalizado|p e tʃ}", " ", "palavra"},
    };

    for (size_t i = 0; i < PV_ARRAY_LEN(tokens); i++) {
        pv_normalizer_stream_reset(normalizer_stream_object);
        test_pv_normalizer_stream_helper_verbalizable(
                normalizer_stream_object,
                4,
                (char **) &tokens[i],
                (char **) &expected_phonemizable_text[i]);
    }
}

static void test_pv_normalizer_stream_invalid_custom_pron(void) {
    char *tokens[][4] = {
            {"oi {personali", "zado|", "ㅁ ", "tʃ}"},
            {"{persona", "lizaㅁ", "|p e tʃ}", " palavra"},
            {"persona", "lizado", "|p e tʃ}", " palavra"},
            {"{persona", "lizado", "|p e tʃ", " palavra"},
            {"{", "persona", "lizado|p e t", "ʃ"},
            {"{", "persona", "lizado| p e t", "ʃ}"},
    };
    pv_status_t add_statuses[][4] = {
            {PV_STATUS_SUCCESS, PV_STATUS_SUCCESS, PV_STATUS_INVALID_ARGUMENT, PV_STATUS_INVALID_ARGUMENT},
            {PV_STATUS_SUCCESS, PV_STATUS_INVALID_ARGUMENT, PV_STATUS_INVALID_ARGUMENT, PV_STATUS_SUCCESS},
            {PV_STATUS_SUCCESS, PV_STATUS_SUCCESS, PV_STATUS_INVALID_ARGUMENT, PV_STATUS_SUCCESS},
            {PV_STATUS_SUCCESS, PV_STATUS_SUCCESS, PV_STATUS_SUCCESS, PV_STATUS_SUCCESS},
            {PV_STATUS_SUCCESS, PV_STATUS_SUCCESS, PV_STATUS_SUCCESS, PV_STATUS_SUCCESS},
            {PV_STATUS_SUCCESS, PV_STATUS_SUCCESS, PV_STATUS_INVALID_ARGUMENT, PV_STATUS_INVALID_ARGUMENT},
    };
    pv_status_t flush_status[] = {
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_SUCCESS,
            PV_STATUS_INVALID_ARGUMENT,
            PV_STATUS_INVALID_ARGUMENT,
            PV_STATUS_SUCCESS,
    };

    for (size_t i = 0; i < PV_ARRAY_LEN(tokens); i++) {
        pv_normalizer_stream_reset(normalizer_stream_object);
        test_pv_normalizer_stream_helper(
                normalizer_stream_object,
                4,
                (char **) &tokens[i],
                add_statuses[i],
                flush_status[i]);
    }
}

static void test_pv_normalizer_stream_verbalizable_alphanum_mix(void) {
    char *tokens[][4] = {
            {"CSS3", " html1", "5", " 8"},
    };
    char *expected_phonemizable_text[][5] = {
            {NULL, "CSS3 ", NULL, NULL, "html15 8"},
    };

    for (size_t i = 0; i < PV_ARRAY_LEN(tokens); i++) {
        pv_normalizer_stream_reset(normalizer_stream_object);
        test_pv_normalizer_stream_helper_verbalizable(
                normalizer_stream_object,
                4,
                (char **) &tokens[i],
                (char **) &expected_phonemizable_text[i]);
    }
}

static void test_pv_normalizer_stream_verbalizable_measurements(void) {
    char *tokens[][3] = {
            {"2", " ", "ml"},
    };
    char *expected_phonemizable_text[][4] = {
            {NULL, NULL, NULL, "2 ml"},
    };

    for (size_t i = 0; i < PV_ARRAY_LEN(tokens); i++) {
        pv_normalizer_stream_reset(normalizer_stream_object);
        test_pv_normalizer_stream_helper_verbalizable(
                normalizer_stream_object,
                3,
                (char **) &tokens[i],
                (char **) &expected_phonemizable_text[i]);
    }
}

static void test_pv_normalizer_stream_verbalizable_time(void) {
    char *tokens[][6] = {
            {"são", " 07:15", " da", " manhã.", "", ""},
            {"11", ":", "", "", "43", ""},
            {"8", ":", "08", " ", "", ""},
            {"7", "", "", "", "", ":00"},
            {"8h19", " ", "", "", "", ""},
            {"12", "", "", "", "h", "12"},
    };
    char *expected_phonemizable_text[][7] = {
            {NULL, "são ", NULL, "07:15 da ", NULL, NULL, "manhã."},
            {NULL, NULL, NULL, NULL, NULL, NULL, "11:43"},
            {NULL, NULL, NULL, NULL, NULL, NULL, "8:08 "},
            {NULL, NULL, NULL, NULL, NULL, NULL, "7:00"},
            {NULL, NULL, NULL, NULL, NULL, NULL, "8h19 "},
            {NULL, NULL, NULL, NULL, NULL, NULL, "12h12"},
    };

    for (size_t i = 0; i < PV_ARRAY_LEN(tokens); i++) {
        pv_normalizer_stream_reset(normalizer_stream_object);
        test_pv_normalizer_stream_helper_verbalizable(
                normalizer_stream_object,
                6,
                (char **) &tokens[i],
                (char **) &expected_phonemizable_text[i]);
    }
}

static void test_pv_normalizer_stream_verbalized_time(void) {
    char *tokens[][8] = {
            {"são ", "7", ":", "15", " da", " manhã.", "", ""},
            {"11", ":", "15", "", "", "", ".", ""},
            {"03", ":", "00", " ", "", "", "", ""},
            {"2", "", "h", "00", " hoje", "", "", "?"},
            {"18", "", "h", "", "23", "", "", ""},
            {"", "17", ":", "", "00", " horas", "", ""},
    };
    int32_t expected_phonemizable_size[][9] = {
            {2, 0, 0, 0, 0, 7, 0, 0, 2},
            {0, 0, 0, 0, 0, 0, 0, 0, 5},
            {0, 0, 0, 0, 0, 0, 0, 0, 3},
            {0, 0, 0, 0, 0, 0, 0, 5, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 8},
            {0, 0, 0, 0, 0, 0, 0, 0, 3},
    };
    char *expected[] = {
            "SÃO SETE E QUINZE DA MANHÃ .",
            "ONZE E QUINZE .",
            "TRÊS HORAS ",
            "DUAS HORAS HOJE ?",
            "DEZOITO E VINTE E TRÊS",
            "DEZASSETE HORAS",
    };

    for (size_t i = 0; i < PV_ARRAY_LEN(tokens); i++) {
        pv_normalizer_stream_reset(normalizer_stream_object);
        test_pv_normalizer_stream_helper_phonemizable(
                normalizer_stream_object,
                8,
                (char **) &tokens[i],
                (int32_t *) expected_phonemizable_size[i],
                (char *) expected[i]);
    }
}

static void test_pv_normalizer_stream_verbalized_measurements(void) {
    char *tokens[][8] = {
            {"5", "km", "/", "h", " ", " ", "", ""},
            {"5", " ", "km", "/", "h", " ", " ", ""},
            {"16", "l", "", "", " ", " ", "", ""},
            {"16", " l", "", "", " ", " ", "", ""},
            {"22", "", "", " ", "", "oz", "", ""},
            {"22", "", "", "", "lb", " ", "", ""},
            {"", "1", ".", "0", "0", "0l", "b", ""},
            {"25", "ft", ".", "", "", "-", "lb", "."},
            {"25", "ft", "", "", "", " ", "lb", "."},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 5, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 6, 0, 0},
            {0, 0, 0, 0, 0, 3, 0, 0, 0},
            {0, 0, 0, 0, 0, 4, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 7},
            {0, 0, 0, 0, 0, 0, 0, 0, 7},
            {0, 0, 0, 0, 0, 0, 0, 0, 2},
            {0, 0, 0, 0, 0, 0, 0, 0, 8},
            {0, 0, 0, 0, 0, 0, 0, 0, 9},
    };
    char *expected[] = {
            "CINCO QUILÓMETROS POR HORA ",
            "CINCO QUILÓMETROS POR HORA ",
            "DEZASSEIS LITROS ",
            "DEZASSEIS LITROS ",
            "VINTE E DUAS ONÇAS",
            "VINTE E DUAS LIBRAS ",
            "MIL LIBRAS",
            "VINTE E CINCO PÉ LIBRAS .",
            "VINTE E CINCO PÉ LIBRAS .",
    };

    for (size_t i = 0; i < PV_ARRAY_LEN(tokens); i++) {
        pv_normalizer_stream_reset(normalizer_stream_object);
        test_pv_normalizer_stream_helper_phonemizable(
                normalizer_stream_object,
                8,
                (char **) &tokens[i],
                (int32_t *) expected_phonemizable_size[i],
                (char *) expected[i]);
    }
}

static void test_pv_normalizer_stream_verbalized_cardinals(void) {
    char *tokens[][8] = {
            {"1", ".", "000", " ", "maçãs", ".", "", ""},
            {"são ", "3.", "050", " ", "dólares", "!", "", ""},
            {"3.", "050", " ", "dólares", "!", "", "", ""},
            {"-9,", "142", ".", "", "", "", "", ""},
            {"−9,", "142", ".", "", "", "", "", ""},
            {"2", "", "2", " ", "", "abarca", " ", ""},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 0, 0, 0, 4},
            {2, 0, 0, 0, 0, 10, 0, 0, 0},
            {0, 0, 0, 0, 10, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 10},
            {0, 0, 0, 0, 0, 0, 0, 0, 10},
            {0, 0, 0, 0, 0, 0, 8, 0, 0},
    };
    char *expected[] = {
            "MIL MAÇÃS .",
            "SÃO TRÊS MIL E CINQUENTA DÓLARES !",
            "TRÊS MIL E CINQUENTA DÓLARES !",
            "MENOS NOVE VÍRGULA UM QUATRO DOIS .",
            "MENOS NOVE VÍRGULA UM QUATRO DOIS .",
            "VINTE E DUAS ABARCA ",
    };

    for (size_t i = 0; i < PV_ARRAY_LEN(tokens); i++) {
        pv_normalizer_stream_reset(normalizer_stream_object);
        test_pv_normalizer_stream_helper_phonemizable(
                normalizer_stream_object,
                8,
                (char **) &tokens[i],
                (int32_t *) expected_phonemizable_size[i],
                (char *) expected[i]);
    }
}

static void test_pv_normalizer_stream_verbalized_fractions(void) {
    char *tokens[][8] = {
            {"1", "/", "2", " ", "colher de chá", ".", " ", ""},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 8, 0, 3, 0, 0},
    };
    char *expected[] = {
            "UM MEIO COLHER DE CHÁ . ",
    };

    for (size_t i = 0; i < PV_ARRAY_LEN(tokens); i++) {
        pv_normalizer_stream_reset(normalizer_stream_object);
        test_pv_normalizer_stream_helper_phonemizable(
                normalizer_stream_object,
                8,
                (char **) &tokens[i],
                (int32_t *) expected_phonemizable_size[i],
                (char *) expected[i]);
    }
}

static void test_pv_normalizer_stream_verbalized_ranges(void) {
    char *tokens[][8] = {
            {"1", "-", "2", " ", "abarca", ".", " ", ""},
            {"1", "–", "2", " ", "abarca", ".", " ", ""},
            {"4-", "7", " ", "maçãs", ".", " ", "", ""},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 0, 7, 0, 0},
            {0, 0, 0, 0, 0, 0, 7, 0, 0},
            {0, 0, 0, 0, 0, 7, 0, 0, 0},
    };
    char *expected[] = {
            "UMA A DUAS ABARCA . ",
            "UMA A DUAS ABARCA . ",
            "QUATRO A SETE MAÇÃS . ",
    };

    for (size_t i = 0; i < PV_ARRAY_LEN(tokens); i++) {
        pv_normalizer_stream_reset(normalizer_stream_object);
        test_pv_normalizer_stream_helper_phonemizable(
                normalizer_stream_object,
                8,
                (char **) &tokens[i],
                (int32_t *) expected_phonemizable_size[i],
                (char *) expected[i]);
    }
}

static void test_pv_normalizer_stream_verbalized_special_characters(void) {
    char *tokens[][8] = {
            {"há", " 5", "%", "", "", " ", "de água", "."},
            {"5 ", "²", " ", "e", " ", "&", " ", "#"},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 2, 4, 0, 0, 1, 2, 0, 2},
            {0, 5, 1, 0, 2, 1, 1, 1, 0},
    };
    char *expected[] = {
            "HÁ CINCO POR CENTO DE ÁGUA .",
            "CINCO SOBRESCRITO DOIS E E NÚMERO",
    };

    for (size_t i = 0; i < PV_ARRAY_LEN(tokens); i++) {
        pv_normalizer_stream_reset(normalizer_stream_object);
        test_pv_normalizer_stream_helper_phonemizable(
                normalizer_stream_object,
                8,
                (char **) &tokens[i],
                (int32_t *) expected_phonemizable_size[i],
                (char *) expected[i]);
    }
}

static void test_pv_normalizer_stream_verbalized_url(void) {
    char *tokens[][8] = {
            {"www", ".", "exemplo", ".", "com", ".", "", ""},
            {"https", ":", "/", "/", "ol%C3%A1", ".", "xyz", "/"},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 0, 0, 0, 8},
            {0, 0, 0, 0, 0, 0, 0, 0, 26},
    };
    char *expected[] = {
            "W W W PONTO EXEMPLO PONTO COM .",
            "H T T P S DOIS PONTOS BARRA BARRA OL POR CENTO C TRÊS POR CENTO A UM PONTO X Y Z BARRA",
    };

    for (size_t i = 0; i < PV_ARRAY_LEN(tokens); i++) {
        pv_normalizer_stream_reset(normalizer_stream_object);
        test_pv_normalizer_stream_helper_phonemizable(
                normalizer_stream_object,
                8,
                (char **) &tokens[i],
                (int32_t *) expected_phonemizable_size[i],
                (char *) expected[i]);
    }
}

static void test_pv_normalizer_stream_verbalized_currency(void) {
    char *tokens[][8] = {
            {"7 EUR", " ", "7", " ", "EUR", ".", "", ""},
            {"€", "4", " ", "1,25", "$", " ", "-2", " USD"},
            {"₹", "22", " ", "2", "₹", " ", "-2", "COP"},
            {"₹", "22", " ", "2", "₹", " ", "−2", "COP"},
            {"símbolo do euro ", "é €,", " ", "símbolo ", "do dólar", " ", "é $.", ""},
    };
    int32_t expected_phonemizable_size[][9] = {
            {3, 1, 0, 0, 3, 0, 0, 0, 1},
            {0, 0, 0, 0, 17, 1, 0, 7, 0},
            {0, 0, 0, 0, 11, 1, 0, 7, 0},
            {0, 0, 0, 0, 11, 1, 0, 7, 0},
            {6, 4, 1, 2, 2, 2, 2, 0, 2},
    };
    char *expected[] = {
            "SETE EUROS SETE EUROS.",
            "QUATRO EUROS UM DÓLAR E VINTE E CINCO CENTAVOS MENOS DOIS DÓLARES AMERICANOS",
            "VINTE E DUAS RUPIAS DUAS RUPIAS MENOS DUAS PESOS COLOMBIANOS",
            "VINTE E DUAS RUPIAS DUAS RUPIAS MENOS DUAS PESOS COLOMBIANOS",
            "SÍMBOLO DO EURO É EURO , SÍMBOLO DO DÓLAR É DÓLAR .",
    };

    for (size_t i = 0; i < PV_ARRAY_LEN(tokens); i++) {
        pv_normalizer_stream_reset(normalizer_stream_object);
        test_pv_normalizer_stream_helper_phonemizable(
                normalizer_stream_object,
                8,
                (char **) &tokens[i],
                (int32_t *) expected_phonemizable_size[i],
                (char *) expected[i]);
    }
}

static void test_pv_normalizer_stream_verbalized_abbreviation(void) {
    char *tokens[][8] = {
            {"isto é", " ", "e", "", "x", ".", " ", "EXEMPLO"},
            {"bananas,", " ", "etc", "", "", "", "", "."},
    };
    int32_t expected_phonemizable_size[][9] = {
            {2, 2, 0, 0, 0, 3, 1, 0, 1},
            {2, 1, 0, 0, 0, 0, 0, 3, 0},
    };
    char *expected[] = {
            "ISTO É POR EXEMPLO EXEMPLO",
            "BANANAS , ET CETERA",
    };

    for (size_t i = 0; i < PV_ARRAY_LEN(tokens); i++) {
        pv_normalizer_stream_reset(normalizer_stream_object);
        test_pv_normalizer_stream_helper_phonemizable(
                normalizer_stream_object,
                8,
                (char **) &tokens[i],
                (int32_t *) expected_phonemizable_size[i],
                (char *) expected[i]);
    }
}

static void test_pv_normalizer_stream_verbalized_digits_sequence(void) {
    char *tokens[][8] = {
            {"(718)", " ", "719", "-", "9810", ".", "", ""},
            {"(718)", " ", "719", "‒", "9810", ".", "", ""},
            {"(", "8", "1", "0", ")", " ", "719", ""},
            {"1", "-", "1", "2", "3", "-", "4", "5"},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 10, 0, 0, 0, 0, 0, 0, 14},
            {0, 10, 0, 0, 0, 0, 0, 0, 14},
            {0, 0, 0, 0, 9, 1, 0, 0, 5},
            {0, 0, 0, 0, 0, 0, 0, 0, 11},
    };
    char *expected[] = {
            ", SETE UM OITO , SETE UM NOVE , NOVE OITO UM ZERO .",
            ", SETE UM OITO , SETE UM NOVE , NOVE OITO UM ZERO .",
            ", OITO UM ZERO , SETE UM NOVE",
            "UM , UM DOIS TRÊS , QUATRO CINCO",
    };

    for (size_t i = 0; i < PV_ARRAY_LEN(tokens); i++) {
        pv_normalizer_stream_reset(normalizer_stream_object);
        test_pv_normalizer_stream_helper_phonemizable(
                normalizer_stream_object,
                8,
                (char **) &tokens[i],
                (int32_t *) expected_phonemizable_size[i],
                (char *) expected[i]);
    }
}

static void test_pv_normalizer_stream_verbalized_dates(void) {
    char *tokens[][8] = {
            {"3", "-", "8", "-", "1810", ".", "", ""},
            {"03-", "08", "-1999", " ", ".", " ", "", ""},
            {"30", "-Jan-", "2005", "!", "", "", "", ""},
            {"1829-", "2", "-13", "!", "", "", "", ""},
            {"202", "4", "-", "0", "1", "-", "0", "6"},
            {"09/", "", "21", "", "/1907", "", "", ""}};
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 0, 0, 0, 14},
            {0, 0, 0, 0, 0, 20, 0, 0, 0},
            {0, 0, 0, 14, 0, 0, 0, 0, 0},
            {0, 0, 0, 20, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 19},
            {0, 0, 0, 0, 0, 0, 0, 0, 18},
    };
    char *expected[] = {
            "TRÊS DE AGOSTO DE MIL OITOCENTOS E DEZ .",
            "TRÊS DE AGOSTO DE MIL NOVECENTOS E NOVENTA E NOVE . ",
            "TRINTA DE JANEIRO DE DOIS MIL E CINCO !",
            "TREZE DE FEVEREIRO DE MIL OITOCENTOS E VINTE E NOVE !",
            "SEIS DE JANEIRO DE DOIS MIL E VINTE E QUATRO",
            "VINTE E UM DE SETEMBRO DE MIL NOVECENTOS E SETE"};

    for (size_t i = 0; i < PV_ARRAY_LEN(tokens); i++) {
        pv_normalizer_stream_reset(normalizer_stream_object);
        test_pv_normalizer_stream_helper_phonemizable(
                normalizer_stream_object,
                8,
                (char **) &tokens[i],
                (int32_t *) expected_phonemizable_size[i],
                (char *) expected[i]);
    }
}

static void test_pv_normalizer_stream_verbalized_names(void) {
    char *tokens[][8] = {
            {"J", ".", " ", "K. ", "", "", "Rowling", ""},
            {"John", " ", "F", ".", " ", "Kennedy", "", ""},
    };

    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 2, 2, 0, 0, 0, 0, 1},
            {0, 2, 0, 0, 2, 0, 0, 0, 1},
    };
    char *expected[] = {
            "J K ROWLING",
            "JOHN F KENNEDY",
    };

    for (size_t i = 0; i < PV_ARRAY_LEN(tokens); i++) {
        pv_normalizer_stream_reset(normalizer_stream_object);
        test_pv_normalizer_stream_helper_phonemizable(
                normalizer_stream_object,
                8,
                (char **) &tokens[i],
                (int32_t *) expected_phonemizable_size[i],
                (char *) expected[i]);
    }
}

static void test_pv_normalizer_stream_verbalized_ordinal(void) {
    char *tokens[][8] = {
            {"1º", "", "", "", "", "", "", ""},
            {"", "2", "ª", "", "", " ", "", ""},
            {"", "", "-12ª", "", "", " ", "", ""},
            {"", "-", "1", "0", "º", " ", "", ""},
            {"-", "", "78.", "a", "", "", " ", ""},
            {"", "", "45", "0", "", ".", "o", ""},
            {"1.001", "", "", "", "ª", "", " ", ""},
    };

    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 0, 0, 0, 1},
            {0, 0, 0, 0, 0, 2, 0, 0, 0},
            {0, 0, 0, 0, 0, 6, 0, 0, 0},
            {0, 0, 0, 0, 0, 4, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 6, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 3},
            {0, 0, 0, 0, 0, 0, 4, 0, 0},
    };
    char *expected[] = {
            "PRIMEIRO",
            "SEGUNDA ",
            "MENOS DÉCIMA SEGUNDA ",
            "MENOS DÉCIMO ",
            "MENOS SEPTUAGÉSIMA OITAVA ",
            "QUADRINGENTÉSIMO QUINQUAGÉSIMO",
            "MILÉSIMA PRIMEIRA ",
    };

    for (size_t i = 0; i < PV_ARRAY_LEN(tokens); i++) {
        pv_normalizer_stream_reset(normalizer_stream_object);
        test_pv_normalizer_stream_helper_phonemizable(
                normalizer_stream_object,
                8,
                (char **) &tokens[i],
                (int32_t *) expected_phonemizable_size[i],
                (char *) expected[i]);
    }
}

#ifdef __PV_MOCKS__

static void *calloc_return_null(size_t arg0, size_t arg1) {
    (void) arg0;
    (void) arg1;
    return NULL;
}

static pv_status_t pv_normalizer_token_list_init_return_oom(pv_normalizer_token_list_t **arg) {
    (void) arg;
    return PV_STATUS_OUT_OF_MEMORY;
}

static void test_pv_normalizer_stream_open_helper(pv_status_t expected) {
    pv_status_t status = pv_normalizer_stream_open(normalizer_object, &normalizer_stream_object);
    pv_test_true(
            status == expected,
            "init internal error, expected `%s` got `%s`",
            pv_status_to_string(expected),
            pv_status_to_string(status));
}

static void test_pv_normalizer_stream_open_calloc_failure(void) {
    PV_SET_MOCK_CUSTOM_FUNC(calloc, calloc_return_null)

    test_pv_normalizer_stream_open_helper(PV_STATUS_OUT_OF_MEMORY);
}

static void test_pv_normalizer_stream_open_internal_failure_1(void) {
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_get_use_cases_from_language, PV_STATUS_INVALID_ARGUMENT)

    test_pv_normalizer_stream_open_helper(PV_STATUS_INVALID_ARGUMENT);
}

static void test_pv_normalizer_stream_open_internal_failure_2(void) {
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_get_use_cases_from_language, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_token_list_init, PV_STATUS_OUT_OF_MEMORY)

    test_pv_normalizer_stream_open_helper(PV_STATUS_OUT_OF_MEMORY);
}

static void test_pv_normalizer_stream_open_internal_failure_3(void) {
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_get_use_cases_from_language, PV_STATUS_SUCCESS)
    pv_status_t (*custom_funcs[])(pv_normalizer_token_list_t **) = {
            pv_normalizer_token_list_init_real,
            pv_normalizer_token_list_init_return_oom,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(pv_normalizer_token_list_init, custom_funcs)

    test_pv_normalizer_stream_open_helper(PV_STATUS_OUT_OF_MEMORY);
}

static void test_pv_normalizer_stream_open_internal_failure_4(void) {
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_get_use_cases_from_language, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_token_list_init, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_stream_context_scanner_init, PV_STATUS_INVALID_ARGUMENT)

    test_pv_normalizer_stream_open_helper(PV_STATUS_INVALID_ARGUMENT);
}


#endif

static const pv_test_case_t PV_NORMALIZER_STREAM_TEST_CASES[] = {
        {"basic", test_pv_normalizer_stream_verbalizable_basic},
        {"punctuation", test_pv_normalizer_stream_verbalizable_punctuation},
        {"decimals", test_pv_normalizer_stream_verbalizable_decimals},
        {"custom_pron", test_pv_normalizer_stream_custom_verbalizable_pron},
        {"invalid custom_pron", test_pv_normalizer_stream_invalid_custom_pron},
        {"alphanum_mix", test_pv_normalizer_stream_verbalizable_alphanum_mix},
        {"measurements", test_pv_normalizer_stream_verbalizable_measurements},
        {"time", test_pv_normalizer_stream_verbalizable_time},

        {"verbalized time", test_pv_normalizer_stream_verbalized_time},
        {"verbalized measurements", test_pv_normalizer_stream_verbalized_measurements},
        {"verbalized cardinals", test_pv_normalizer_stream_verbalized_cardinals},
        {"verbalized fractions", test_pv_normalizer_stream_verbalized_fractions},
        {"verbalized ranges", test_pv_normalizer_stream_verbalized_ranges},
        {"verbalized special characters", test_pv_normalizer_stream_verbalized_special_characters},
        {"verbalized url", test_pv_normalizer_stream_verbalized_url},
        {"verbalized currency", test_pv_normalizer_stream_verbalized_currency},
        {"verbalized abbreviation", test_pv_normalizer_stream_verbalized_abbreviation},
        {"verbalized digits", test_pv_normalizer_stream_verbalized_digits_sequence},
        {"verbalized dates", test_pv_normalizer_stream_verbalized_dates},
        {"verbalized names", test_pv_normalizer_stream_verbalized_names},
        {"verbalized ordinal", test_pv_normalizer_stream_verbalized_ordinal},

#ifdef __PV_MOCKS__

        {"stream open calloc failure", test_pv_normalizer_stream_open_calloc_failure},
        {"stream open internal failure 1", test_pv_normalizer_stream_open_internal_failure_1},
        {"stream open internal failure 2", test_pv_normalizer_stream_open_internal_failure_2},
        {"stream open internal failure 3", test_pv_normalizer_stream_open_internal_failure_3},
        {"stream open internal failure 4", test_pv_normalizer_stream_open_internal_failure_4},

#endif

};

const pv_test_suite_t PV_NORMALIZER_STREAM_PT_TEST_SUITE = {
        .name = "normalizer_stream_pt",
        .setup = test_pv_normalizer_stream_setup,
        .teardown = test_pv_normalizer_stream_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_NORMALIZER_STREAM_TEST_CASES),
        .test_cases = PV_NORMALIZER_STREAM_TEST_CASES,
};
