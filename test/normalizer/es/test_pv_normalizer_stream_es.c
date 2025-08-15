#include <stdlib.h>
#include <string.h>

#include "core/picovoice.h"
#include "core/pv_language.h"
#include "core/pv_language_json.h"
#include "mock/pv_mock.h"
#include "orca/normalizer/pv_normalizer.h"
#include "orca/normalizer/pv_normalizer_stream.h"
#include "test/pv_test.h"

#include "orca/normalizer/es/pv_normalizer_tags_es.h"
#include "orca/normalizer/test_pv_normalizer_stream_helper.h"

#ifdef __PV_MOCKS__

#include "core/mock/pv_core_runtime_mock.h"
#include "orca/mock/pv_normalizer_mock.h"

#endif

static pv_normalizer_stream_t *normalizer_stream_object = NULL;
static pv_normalizer_t *normalizer_object = NULL;
static pv_noun_gender_dict_t *noun_gender_dict_object = NULL;
static pv_language_info_t *language_info_object = NULL;

static const char LANGUAGE_INFO_PATH[] = "normalizer/ref/pv_language_info_normalizer_es.json";
static const char NOUN_GENDER_DICT_PATH[] = "test_data/noun_gender_dict/noun_gender_dict_es.txt";

static pv_status_t test_pv_normalizer_stream_setup(void) {
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
            {"Hola", " mundo"},
    };
    char *expected_phonemizable_text[][3] = {
            {NULL, "Hola ", "mundo"},
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
            {"Hola", ".", " ", "Como estas"},
            {"Hola", ". ", "Como", " estas"},
    };
    char *expected_phonemizable_text[][5] = {
            {NULL, NULL, "Hola. ", "Como ", "estas"},
            {NULL, "Hola. ", NULL, "Como ", "estas"},
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
            {
                    "1",
                    ",",
                    " ",
                    "5",
            },
    };
    char *expected_phonemizable_text[][5] = {
            {NULL, NULL, NULL, "1,5%", NULL},
            {NULL, NULL, NULL, "-1,5%", NULL},
            {NULL, NULL, NULL, NULL, "1, 5"},
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

static void test_pv_normalizer_stream_verbalizable_fake_decimals(void) {
    char *tokens[][5] = {
            {"1", ",", "5", ","
                            "5"},
            {"1", "", ",5", ""
                            ",5"},
    };
    char *expected_phonemizable_text[][6] = {
            {NULL, NULL, NULL, NULL, "1,5,5"},
            {NULL, NULL, NULL, NULL, "1,5,5"},
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
            {"15", " gente", " en", " el carro"},
            {"hola", " {personalizada|p e}", " persona", "!"},
            {"{", "personalizada|", "p e", "}"},
            {"hola {personalizada", "|", "p", " e}"},
            {"hola {persona", "lizada|", "p e}", " palabra"},
            {"{persona", "lizada", "|p e}", " palabra"},
    };
    char *expected_phonemizable_text[][5] = {
            {NULL, NULL, NULL, "15 gente en el ", "carro"},
            {NULL, "hola {personalizada|p e}", " ", "persona!", NULL},
            {NULL, NULL, NULL, "{personalizada|p e}", NULL},
            {"hola ", NULL, NULL, "{personalizada|p e}", NULL},
            {"hola ", NULL, "{personalizada|p e}", " ", "palabra"},
            {NULL, NULL, "{personalizada|p e}", " ", "palabra"},
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
            {"hola {personali", "zada|", "ㅁ ", "e}"},
            {"{persona", "lizaㅁ", "|p e}", " palabra"},
            {"persona", "lizada", "|p e}", " palabra"},
            {"{persona", "lizada", "|p e", " palabra"},
            {"{", "persona", "lizada|p ", "e"},
            {"{", "persona", "lizada| p", " e}"},
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
            {NULL, NULL, NULL, NULL, "CSS3 html15 8"},
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
            {"son", " 07:15", ".", ""},
            {"11", ":", "43", ""},
            {"23", ":", "03", ""},
            {"8", "h", "8", "m"},
            {"7", "h", "6", "'"}};
    char *expected_phonemizable_text[][7] = {
            {NULL, "son ", NULL, NULL, "07:15."},
            {NULL, NULL, NULL, NULL, "11:43"},
            {NULL, NULL, NULL, NULL, "23:03"},
            {NULL, NULL, NULL, NULL, "8h8m"},
            {NULL, NULL, NULL, NULL, "7h6'"},
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

static void test_pv_normalizer_stream_verbalized_time(void) {
    char *tokens[][5] = {
            {"11", ":", "43", "", ""},
            {"23", "h", "15", "", "."},
            {"3", "h", "00", "'", ""},
            {"7", "h", "09", "m", ""},
            {"8", ":", "00", "!", ""},
            {"2", ":", "00", ",", ""},
            {"2", ":", "00", "?", ""},
            {"8:30", "a", "m", "", ""},
            {"12:35", "p.", "m", ".", ""},
            {"11:15", "p", ".", "m", "."},
    };
    int32_t expected_phonemizable_size[][6] = {
            {0, 0, 0, 0, 0, 8},
            {0, 0, 0, 0, 0, 5},
            {0, 0, 0, 0, 0, 1},
            {0, 0, 0, 0, 0, 4},
            {0, 0, 0, 2, 0, 0},
            {0, 0, 0, 0, 0, 2},
            {0, 0, 0, 2, 0, 0},
            {0, 0, 0, 0, 0, 9},
            {0, 0, 0, 0, 0, 13},
            {0, 0, 0, 0, 0, 9},
    };
    char *expected[] = {
            "ONCE Y CUARENTA Y TRES",
            "VEINTITRÉS Y QUINCE .",
            "TRES",
            "SIETE Y NUEVE",
            "OCHO !",
            "DOS ,",
            "DOS ?",
            "OCHO Y TREINTA DE LA MAÑANA",
            "DOCE Y TREINTA Y CINCO DE LA TARDE",
            "ONCE Y QUINCE DE LA NOCHE",
    };

    for (size_t i = 0; i < PV_ARRAY_LEN(tokens); i++) {
        pv_normalizer_stream_reset(normalizer_stream_object);
        test_pv_normalizer_stream_helper_phonemizable(
                normalizer_stream_object,
                5,
                (char **) &tokens[i],
                (int32_t *) expected_phonemizable_size[i],
                (char *) expected[i]);
    }
}

static void test_pv_normalizer_stream_verbalized_measurements(void) {
    char *tokens[][8] = {
            {"5", "km", "/", "h", " ", " ", "", ""},
            {"5", " ", "km", "/", "h", " ", " ", ""},
            {"16", "l", "", "", "", " ", "", ""},
            {"16to", " l", "", "", "", " ", "", ""},
            {"-", "1", ".", "0", "0", "0", "g", ""},
            {"25", "ft", ".", "", "", "-", "lb", "."},
            {"25", "ft", "", "", "", " ", "lb", "."},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 0, 0, 0, 5},
            {0, 0, 0, 0, 0, 0, 0, 0, 6},
            {0, 0, 0, 0, 0, 0, 0, 0, 3},
            {0, 0, 0, 0, 0, 0, 0, 0, 4},
            {0, 0, 0, 0, 0, 0, 0, 0, 4},
            {0, 0, 0, 0, 0, 0, 0, 0, 4},
            {0, 0, 0, 0, 0, 0, 0, 0, 5},
    };
    char *expected[] = {
            "CINCO KILÓMETROS POR HORA ",
            "CINCO KILÓMETROS POR HORA ",
            "DIECISÉIS LITROS ",
            "DECIMOSEXTO L ",
            "NEGATIVO MIL GRAMOS",
            "VEINTICINCO PIE LIBRAS .",
            "VEINTICINCO PIE LIBRAS .",

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
            {"1", ".", "000", " ", "manzanas", ".", "", ""},
            {"Estos son ", "3.", "050", " ", "dólares", "!", "", ""},
            {"3.", "050", " ", "dólares", "!", "", "", ""},
            {"1", " ", "hora", " por", " ", "sesión", ".", ""},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 0, 0, 0, 4},
            {4, 0, 0, 0, 0, 8, 0, 0, 0},
            {0, 0, 0, 0, 8, 0, 0, 0, 0},
            {0, 0, 0, 0, 6, 0, 0, 0, 2},
    };
    char *expected[] = {
            "MIL MANZANAS .",
            "ESTOS SON TRES MIL CINCUENTA DÓLARES !",
            "TRES MIL CINCUENTA DÓLARES !",
            "UNA HORA POR SESIÓN .",
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
            {"1", "/", "2", " ", "cucharadita", ".", " ", ""},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 0, 7, 0, 0},
    };
    char *expected[] = {
            "UN MEDIO CUCHARADITA . ",
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
            {"1", "-", "2", " ", "cucharaditas", ".", " ", ""},
            {"quiero ", "4-", "7", " ", "manzanas", ".", " ", ""},
            {"quiero ", "4–", "7", " ", "manzanas", ".", " ", ""},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 0, 7, 0, 0},
            {2, 0, 0, 0, 0, 0, 7, 0, 0},
            {2, 0, 0, 0, 0, 0, 7, 0, 0},
    };
    char *expected[] = {
            "UNO A DOS CUCHARADITAS . ",
            "QUIERO CUATRO A SIETE MANZANAS . ",
            "QUIERO CUATRO A SIETE MANZANAS . ",
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
            {"hay", " 5", "%", "", "", " ", "agua", "."},
            {"5 ", "R", "P", "M", " ", "", "", ""},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 2, 4, 0, 0, 0, 0, 0, 3},
            {0, 0, 0, 0, 0, 0, 0, 0, 8},
    };
    char *expected[] = {
            "HAY CINCO POR CIENTO AGUA .",
            "CINCO REVOLUCIONES POR MINUTO ",
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
            {"www", ".", "ejemplo", ".", "com", ".", "", ""},
            {"https", ":", "/", "/", "hola", ".", "xyz", "/"},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 0, 0, 0, 8},
            {0, 0, 0, 0, 0, 0, 0, 0, 16},
    };
    char *expected[] = {
            "W W W PUNTO EJEMPLO PUNTO COM .",
            "H T T P S DOS PUNTOS DIAGONAL DIAGONAL HOLA PUNTO X Y Z DIAGONAL",
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
            {"$", "4", " ", "1,25", "$", " ", "-2", " USD"},
            {"$", "4", " ", "1,25", "$", " ", "−2", " USD"},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 0, 0, 0, 8},
            {0, 0, 0, 0, 0, 0, 0, 0, 21},
            {0, 0, 0, 0, 0, 0, 0, 0, 21},
    };
    char *expected[] = {
            "SIETE EUROS SIETE EUROS .",
            "CUATRO DÓLARES UN DÓLAR CON VEINTICINCO CENTAVOS NEGATIVO DOS DÓLARES ESTADOUNIDENSES",
            "CUATRO DÓLARES UN DÓLAR CON VEINTICINCO CENTAVOS NEGATIVO DOS DÓLARES ESTADOUNIDENSES",
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
            {"esto", " ", "p", ".", "ej", ".", " ", "cuenta"},
            {"vs.", " ", "v", "s", ".", " ", "prof", "."},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 2, 0, 0, 0, 3, 1, 0, 1},
            {1, 1, 0, 0, 1, 1, 0, 1, 0},
    };
    char *expected[] = {
            "ESTO POR EJEMPLO CUENTA",
            "VERSUS VERSUS PROFESOR",
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
            {"718", "-", "719", "-", "9810", ".", "", ""},
            {"718", "‒", "719", "‒", "9810", ".", "", ""},
            {"1", "-", "1", "2", "3", "-", "4", "5"},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 0, 0, 0, 20},
            {0, 0, 0, 0, 0, 0, 0, 0, 20},
            {0, 0, 0, 0, 0, 0, 0, 0, 11},
    };
    char *expected[] = {
            "SIETE UNO OCHO , SIETE UNO NUEVE , NUEVE OCHO UNO CERO .",
            "SIETE UNO OCHO , SIETE UNO NUEVE , NUEVE OCHO UNO CERO .",
            "UNO , UNO DOS TRES , CUATRO CINCO",
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
            {"31", "-Abr-", "2005", "!", "", "", "", ""},
            {"1829-", "2-", "13", "!", "", "", "", ""},
            {"202", "4", "-", "0", "1", "-", "0", "6"},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 0, 0, 0, 12},
            {0, 0, 0, 0, 0, 0, 0, 0, 18},
            {0, 0, 0, 16, 0, 0, 0, 0, 0},
            {0, 0, 0, 14, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 13},
    };
    char *expected[] = {
            "TRES DE AGOSTO DE MIL OCHOCIENTOS DIEZ .",
            "TRES DE AGOSTO DE MIL NOVECIENTOS NOVENTA Y NUEVE . ",
            "TREINTA Y UNO DE ABRIL DE DOS MIL CINCO !",
            "TRECE DE FEBRERO DE MIL OCHOCIENTOS VEINTINUEVE !",
            "SEIS DE ENERO DE DOS MIL VEINTICUATRO",
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

static void test_pv_normalizer_stream_verbalized_names(void) {
    char *tokens[][8] = {
            {"John", " ", "F", ".", " ", "Kennedy", "", ""},
            {"J. ", "R.", " R", ".", " Tolkien", "", "", ""},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 2, 0, 0, 2, 0, 0, 0, 1},
            {2, 0, 2, 0, 2, 0, 0, 0, 1},
    };
    char *expected[] = {
            "JOHN F KENNEDY",
            "J R R TOLKIEN",
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

static void test_pv_normalizer_stream_open_helper(
        pv_status_t expected_status,
        const char *expected_public_message_regex,
        const char *expected_private_message_regex) {
    pv_status_t status = pv_normalizer_stream_open(normalizer_object, &normalizer_stream_object);
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
                true,
                "error message mismatch, expected '%s'",
                expected_message);
    }
}

static void test_pv_normalizer_stream_open_calloc_failure(void) {
    PV_SET_MOCK_CUSTOM_FUNC(calloc, calloc_return_null)

    test_pv_normalizer_stream_open_helper(PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `o`\\.");
}

static void test_pv_normalizer_stream_open_internal_failure_1(void) {
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_get_use_cases_from_language, PV_STATUS_INVALID_ARGUMENT)

    test_pv_normalizer_stream_open_helper(PV_STATUS_INVALID_ARGUMENT, pv_test_function_hash_regex(), "`pv_normalizer_get_use_cases_from_language` failed with status `INVALID_ARGUMENT`\\.");
}

static void test_pv_normalizer_stream_open_internal_failure_2(void) {
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_get_use_cases_from_language, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_token_list_init, PV_STATUS_OUT_OF_MEMORY)

    test_pv_normalizer_stream_open_helper(PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_token_list_init` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_normalizer_stream_open_internal_failure_3(void) {
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_get_use_cases_from_language, PV_STATUS_SUCCESS)
    pv_status_t (*custom_funcs[])(pv_normalizer_token_list_t **) = {
            pv_normalizer_token_list_init_real,
            pv_normalizer_token_list_init_return_oom,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(pv_normalizer_token_list_init, custom_funcs)

    test_pv_normalizer_stream_open_helper(PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_token_list_init` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_normalizer_stream_open_internal_failure_4(void) {
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_get_use_cases_from_language, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_token_list_init, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_stream_context_scanner_init, PV_STATUS_INVALID_ARGUMENT)

    test_pv_normalizer_stream_open_helper(PV_STATUS_INVALID_ARGUMENT, pv_test_function_hash_regex(), "`pv_normalizer_stream_context_scanner_init` failed with status `INVALID_ARGUMENT`\\.");
}

#endif

static const pv_test_case_t PV_NORMALIZER_STREAM_TEST_CASES[] = {
        {"basic", test_pv_normalizer_stream_verbalizable_basic},
        {"punctuation", test_pv_normalizer_stream_verbalizable_punctuation},
        {"decimals", test_pv_normalizer_stream_verbalizable_decimals},
        {"fake_decimals", test_pv_normalizer_stream_verbalizable_fake_decimals},
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
        {"verbalized digits sequence", test_pv_normalizer_stream_verbalized_digits_sequence},
        {"verbalized dates", test_pv_normalizer_stream_verbalized_dates},
        {"verbalized names", test_pv_normalizer_stream_verbalized_names},

#ifdef __PV_MOCKS__

        {"stream open calloc failure", test_pv_normalizer_stream_open_calloc_failure},
        {"stream open internal failure 1", test_pv_normalizer_stream_open_internal_failure_1},
        {"stream open internal failure 2", test_pv_normalizer_stream_open_internal_failure_2},
        {"stream open internal failure 3", test_pv_normalizer_stream_open_internal_failure_3},
        {"stream open internal failure 4", test_pv_normalizer_stream_open_internal_failure_4},

#endif

};

const pv_test_suite_t PV_NORMALIZER_STREAM_ES_TEST_SUITE = {
        .name = "normalizer_stream_es",
        .setup = test_pv_normalizer_stream_setup,
        .teardown = test_pv_normalizer_stream_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_NORMALIZER_STREAM_TEST_CASES),
        .test_cases = PV_NORMALIZER_STREAM_TEST_CASES,
};
