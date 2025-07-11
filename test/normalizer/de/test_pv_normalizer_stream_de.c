#include <stdlib.h>
#include <string.h>

#include "core/picovoice.h"
#include "core/pv_language.h"
#include "core/pv_language_json.h"
#include "mock/pv_mock.h"
#include "orca/normalizer/pv_normalizer.h"
#include "orca/normalizer/pv_normalizer_stream.h"
#include "test/pv_test.h"

#include "orca/normalizer/de/pv_normalizer_tags_de.h"
#include "orca/normalizer/test_pv_normalizer_stream_helper.h"

#ifdef __PV_MOCKS__

#include "core/mock/pv_core_runtime_mock.h"
#include "orca/mock/pv_normalizer_mock.h"

#endif

static pv_normalizer_stream_t *normalizer_stream_object = NULL;
static pv_normalizer_t *normalizer_object = NULL;
static pv_noun_gender_dict_t *noun_gender_dict_object = NULL;
static pv_language_info_t *language_info_object = NULL;

static const char LANGUAGE_INFO_PATH[] = "normalizer/ref/pv_language_info_normalizer_de.json";
static const char NOUN_GENDER_DICT_PATH[] = "test_data/noun_gender_dict/noun_gender_dict_de.txt";

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
            {"Hallo", " Welt"},
    };
    char *expected_phonemizable_text[][3] = {
            {NULL, "Hallo ", "Welt"},
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
            {"Hallo", ".", " ", "Geht's los"},
            {"Hallo", ". ", "Geht's", " los"},
    };
    char *expected_phonemizable_text[][5] = {
            {NULL, NULL, NULL, "Hallo. Geht's ", "los"},
            {NULL, NULL, NULL, "Hallo. Geht's ", "los"},
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
            {"15", " Leute", " in", " einem Auto"},
            {"hallo", " {benutzerdefinierte|b ʤ õ ɛː}", " Auss", "!"},
            {"{", "benutzerdefinierte|", "b ʤ õ ɛː", "}"},
            {"hallo {benutzerdefinierte", "|", "b ʤ", " õ ɛː}"},
            {"hallo {benut", "zerdefinierte|", "b ʤ õ ɛː}", " Auss"},
            {"{benut", "zerdefinierte", "|b ʤ õ ɛː}", " Auss"},
            {"{benut", "zerdefinierte|b ʤ õ ɛ", "ː}", " Auss"},
    };
    char *expected_phonemizable_text[][5] = {
            {NULL, "15 ", "Leute ", "in einem ", "Auto"},
            {NULL, "hallo {benutzerdefinierte|b ʤ õ ɛː}", " ", "Auss!", NULL},
            {NULL, NULL, NULL, "{benutzerdefinierte|b ʤ õ ɛː}", NULL},
            {"hallo ", NULL, NULL, "{benutzerdefinierte|b ʤ õ ɛː}", NULL},
            {"hallo ", NULL, "{benutzerdefinierte|b ʤ õ ɛː}", " ", "Auss"},
            {NULL, NULL, "{benutzerdefinierte|b ʤ õ ɛː}", " ", "Auss"},
            {NULL, NULL, "{benutzerdefinierte|b ʤ õ ɛː}", " ", "Auss"},
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
            {"hallo {benutzer", "definierte|", "ㅁ ", "õ ɛː}"},
            {"{benut", "zerdefinieㅁ", "|b ʤ õ ɛː}", " Auss"},
            {"benut", "zerdefinierte", "|b ʤ õ ɛː}", " Auss"},
            {"{benut", "zerdefinierte", "|b ʤ õ ɛː", " Auss"},
            {"{", "benut", "zerdefinierte|b ʤ õ ɛ", "ː"},
            {"{", "benut", "zerdefinierte| b ʤ õ ɛ", "ː}"},
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
            {NULL, "CSS3 ", NULL, "html15 ", "8"},
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
            {NULL, "2 ", NULL, "ml"},
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
            {"ist", " 07:15", " uhr", ".", "", ""},
            {"23", ":", "43", "UHR", "", ""},
            {"23", ":", "03", " uhr", " ", ""},
            {"20", ":", "08", " Uhr", "", ""},
            {"7", " ", "u", "", "h", "r"},
            {"20:19", " ", "Uh", "", "", "r"},
            {"8:59", "U", "", "H", "", "R"},
    };
    char *expected_phonemizable_text[][7] = {
            {NULL, "ist ", "07:15 ", NULL, NULL, NULL, "UHR."},
            {NULL, NULL, NULL, NULL, NULL, NULL, "23:43UHR"},
            {NULL, NULL, NULL, "23:03 ", "UHR ", NULL, NULL},
            {NULL, NULL, NULL, "20:08 ", NULL, NULL, "UHR"},
            {NULL, "7 ", NULL, NULL, NULL, NULL, "uhr"},
            {NULL, "20:19 ", NULL, NULL, NULL, NULL, "Uhr"},
            {NULL, NULL, NULL, NULL, NULL, NULL, "8:59UHR"},
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
            {"23", ":", "43", "UHR", "", "", "", ""},
            {"23", ":", "15", "UHR", ".", "", "", ""},
            {"03", ":", "00", " ", "u", "h", "r", ""},
            {"19", " ", "uh", "r", " Heute", " ", "", ""},
            {"8", "uhr", "", " ", "", "", "", ""},
            {"8", " ", "uhr", "!", "", "", "", ""},
            {"8", " ", "", "uhr", "!", "", "", ""},
            {"2", " ", "u", "h", "r", "", ",", ""},
            {"2", "uhr", "", "", "", "?", "", ""},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 0, 0, 0, 4},
            {0, 0, 0, 0, 0, 0, 0, 0, 5},
            {0, 0, 0, 4, 0, 0, 0, 0, 0},
            {0, 2, 0, 0, 2, 2, 0, 0, 0},
            {0, 0, 0, 4, 0, 0, 0, 0},
            {0, 2, 0, 2, 0, 0, 0, 0},
            {0, 2, 0, 0, 2, 0, 0, 0},
            {0, 2, 0, 0, 0, 0, 2, 0},
            {0, 0, 0, 0, 0, 4, 0, 0},
    };
    char *expected[] = {
            "DREIUNDZWANZIG UHR DREIUNDVIERZIG",
            "DREIUNDZWANZIG UHR FÜNFZEHN .",
            "DREI UHR ", // TODO(Eric Mikulin): Should not include extra space?
            "NEUNZEHN UHR HEUTE ",
            "ACHT UHR ",
            "ACHT UHR !",
            "ACHT UHR !",
            "ZWEI UHR ,",
            "ZWEI UHR ?",
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
            {"16", "l", "", "", "", " ", " ", ""},
            {"-", "1", ".", "0", "0", "0", "g", ""},
            {"25", "ft", ".", "", "", "-", "lb", "."},
            {"25", "ft", "", "", "", " ", "lb", "."},

    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 5, 0, 0, 0},
            {0, 2, 0, 0, 0, 0, 4, 0, 0},
            {0, 0, 0, 0, 0, 0, 3, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 4},
            {0, 0, 0, 0, 0, 0, 0, 0, 4},
            {0, 0, 0, 0, 0, 0, 0, 0, 5},
    };
    char *expected[] = {
            "FÜNF KILOMETER PRO STUNDE ",
            "FÜNF KILOMETER PRO STUNDE ",
            "SECHZEHN LITER ",
            "MINUS EINTAUSEND GRAMM",
            "FÜNFUNDZWANZIG FUß PFUND .",
            "FÜNFUNDZWANZIG FUß PFUND .",
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

static void test_pv_normalizer_stream_verbalized_ordinals(void) {
    char *tokens[][8] = {
            {"1", ".", " ", "Sommer", " ", "", "", ""},
            {"16.", " Aalbaum", "", "", "", "", "", ""},
            {"1", ".", "005", ".", " ", "Aaleidechse", "", ""},
            {"2005", ".", "", "", " ", "Frühling", "", ""},
            {"2.000.000.000.000.", " ", "", "Februar", "", "", "", ""},
            {"2.", "000.", "000.", "000.", "000. ", "Februar", "", ""},
            {"111", ".", "005", ".", "", " ", "Aalbaum", ""},
            {"-2", ".", " ", "Sommer", "", "", "", ""},
            {"-", "26.", " Aalfischer", "", "", "", "", ""},
            {"-", "", "2.000.", "000. ", "Sommer", "", "", ""},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 4, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 3},
            {0, 0, 0, 0, 0, 0, 0, 0, 3},
            {0, 0, 0, 0, 0, 0, 0, 0, 3},
            {0, 0, 0, 0, 0, 0, 0, 0, 5},
            {0, 0, 0, 0, 0, 0, 0, 0, 28},
            {0, 0, 0, 0, 0, 0, 0, 0, 5},
            {0, 0, 0, 0, 0, 0, 0, 0, 5},
            {0, 0, 0, 0, 0, 0, 0, 0, 5},
            {0, 0, 0, 0, 0, 0, 0, 0, 12},
    };
    char *expected[] = {
            "ERSTER SOMMER ",
            "SECHZEHNTER AALBAUM",
            "EINTAUSENDFÜNFTE AALEIDECHSE",
            "ZWEITAUSENDFÜNFTES FRÜHLING",
            "ZWEI BILLIONSTER FEBRUAR",
            "ZWEI NULL NULL NULL , NULL NULL NULL , NULL NULL NULL , NULL NULL NULL . FEBRUAR",
            "EINHUNDERTELFTAUSEND FÜNFTER AALBAUM",
            "MINUS ZWEITER SOMMER",
            "MINUS SECHSUNDZWANZIGSTER AALFISCHER",
            "MINUS ZWEITAUSEND PUNKT NULL NULL NULL . SOMMER",
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
            {"1", ".", "000", " ", "Äpfel", ".", "", ""},
            {"Das sind ", "3.", "050", " ", "Dollar", "!", "", ""},
            {"3.", "050", " ", "Dollar", "!", "", "", ""},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 2, 0, 0, 0, 0, 2},
            {4, 0, 0, 2, 0, 2, 0, 0, 0},
            {0, 0, 2, 0, 2, 0, 0, 0, 0},
    };
    char *expected[] = {
            "EINTAUSEND ÄPFEL .",
            "DAS SIND DREITAUSENDFÜNFZIG DOLLAR !",
            "DREITAUSENDFÜNFZIG DOLLAR !",
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
            {"1", "/", "2", " ", "Teelöffel", ".", " ", ""},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 4, 0, 0, 0, 0, 3},
    };
    char *expected[] = {
            "EIN HALB TEELÖFFEL . ",
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
            {"1", "-", "2", " ", "Teelöffel", ".", " ", ""},
            {"gib mir ", "4-", "7", " ", "Äpfel", ".", " ", ""},
            {"gib mir ", "4–", "7", " ", "Äpfel", ".", " ", ""},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 4, 0, 0, 0, 0, 3},
            {4, 0, 0, 4, 0, 0, 0, 0, 3},
            {4, 0, 0, 4, 0, 0, 0, 0, 3},
    };
    char *expected[] = {
            "EINS BIS ZWEI TEELÖFFEL . ",
            "GIB MIR VIER BIS SIEBEN ÄPFEL . ",
            "GIB MIR VIER BIS SIEBEN ÄPFEL . ",
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
            {"es gibt", " 5", "%", "", "", " ", "wasser", "."},
            {"5 ", "G", "H", "Z", " ", "", "", ""},
    };
    int32_t expected_phonemizable_size[][9] = {
            {2, 2, 2, 0, 0, 1, 0, 0, 2},
            {2, 0, 0, 0, 2, 0, 0, 0, 0},
    };
    char *expected[] = {
            "ES GIBT FÜNF PROZENT WASSER .",
            "FÜNF GIGAHERTZ ",
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
            {"www", ".", "beispiel", ".", "com", ".", "", ""},
            {"https", ":", "/", "/", "hallo", ".", "xyz", "/"},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 0, 0, 0, 8},
            {0, 0, 0, 0, 0, 0, 0, 0, 14},
    };
    char *expected[] = {
            "W W W PUNKT BEISPIEL PUNKT COM .",
            "H T T P S DOPPELPUNKT SCHRÄGSTRICH SCHRÄGSTRICH HALLO PUNKT X Y Z SCHRÄGSTRICH",
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
            {"$", "4", " ", "1.25", "$", " ", "-2", " USD"},
            {"$", "4", " ", "1.25", "$", " ", "−2", " USD"},
    };
    int32_t expected_phonemizable_size[][9] = {
            {3, 1, 0, 2, 1, 0, 0, 0, 1},
            {0, 0, 4, 0, 5, 1, 0, 5, 0},
            {0, 0, 4, 0, 5, 1, 0, 5, 0},
    };
    char *expected[] = {
            "SIEBEN EURO SIEBEN EURO.",
            "VIER DOLLAR EINS PUNKT FÜNFUNDZWANZIG DOLLAR MINUS ZWEI US-DOLLAR",
            "VIER DOLLAR EINS PUNKT FÜNFUNDZWANZIG DOLLAR MINUS ZWEI US-DOLLAR",
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
    char *tokens[][9] = {
            {"Dies", " ", "gilt", " ", "z.", " ", "B.", " ", "als"},
            {"D. H.", " ", "D.", " H.", "", " ", "prof", ".", ""},
    };
    int32_t expected_phonemizable_size[][10] = {
            {0, 2, 0, 2, 0, 0, 3, 1, 0, 1},
            {3, 1, 0, 3, 0, 1, 0, 1, 0, 0},
    };
    char *expected[] = {
            "DIES GILT ZUM BEISPIEL ALS",
            "DAS HEISST DAS HEISST PROFESSOR",
    };

    for (size_t i = 0; i < PV_ARRAY_LEN(tokens); i++) {
        pv_normalizer_stream_reset(normalizer_stream_object);
        test_pv_normalizer_stream_helper_phonemizable(
                normalizer_stream_object,
                9,
                (char **) &tokens[i],
                (int32_t *) expected_phonemizable_size[i],
                (char *) expected[i]);
    }
}

static void test_pv_normalizer_stream_verbalized_digits_sequence(void) {
    char *tokens[][8] = {
            {"(718)", " ", "719", "-", "9810", ".", "", ""},
            {"(", "8", "1", "0", ")", " ", "719", ""},
            {"1", "-", "1", "2", "3", "-", "4", "5"},
            {"(718)", " ", "719", "-9810", ".", "", "", ""},
            {"(718)", " ", "719", "‒9810", ".", "", "", ""},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 10, 0, 0, 0, 0, 0, 0, 14},
            {0, 0, 0, 0, 9, 1, 0, 0, 5},
            {0, 0, 0, 0, 0, 0, 0, 0, 11},
            {0, 10, 0, 0, 0, 0, 0, 0, 14},
            {0, 10, 0, 0, 0, 0, 0, 0, 14},
    };
    char *expected[] = {
            ", SIEBEN EINS ACHT , SIEBEN EINS NEUN , NEUN ACHT EINS NULL .",
            ", ACHT EINS NULL , SIEBEN EINS NEUN",
            "EINS , EINS ZWEI DREI , VIER FÜNF",
            ", SIEBEN EINS ACHT , SIEBEN EINS NEUN , NEUN ACHT EINS NULL .",
            ", SIEBEN EINS ACHT , SIEBEN EINS NEUN , NEUN ACHT EINS NULL .",
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
            {"31", "-Apr-", "2005", "!", "", "", "", ""},
            {"1829-", "2", "-13", "!", "", "", "", ""},
            {"202", "4", "-", "0", "1", "-", "0", "6"},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 0, 0, 0, 6},
            {0, 0, 0, 6, 0, 0, 0, 0, 2},
            {0, 0, 0, 6, 0, 0, 0, 0, 0},
            {0, 0, 0, 8, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 7},
    };
    char *expected[] = {
            "DRITTER AUGUST , ACHTZEHNHUNDERTZEHN .",
            "DRITTER AUGUST , NEUNZEHNHUNDERTNEUNUNDNEUNZIG . ",
            "EINUNDDREISSIGSTER APRIL , ZWEITAUSENDFÜNF !",
            "DREIZEHNTER FEBRUAR , ACHTZEHNHUNDERTNEUNUNDZWANZIG !",
            "SECHSTER JANUAR , ZWEITAUSENDVIERUNDZWANZIG",
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
            {0, 2, 0, 0, 0, 0, 0, 0, 3},
            {0, 0, 0, 0, 0, 0, 0, 0, 7},
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
        {"verbalized ordinals", test_pv_normalizer_stream_verbalized_ordinals},
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

#ifdef __PV_MOCKS__

        {"stream open calloc failure", test_pv_normalizer_stream_open_calloc_failure},
        {"stream open internal failure 1", test_pv_normalizer_stream_open_internal_failure_1},
        {"stream open internal failure 2", test_pv_normalizer_stream_open_internal_failure_2},
        {"stream open internal failure 3", test_pv_normalizer_stream_open_internal_failure_3},
        {"stream open internal failure 4", test_pv_normalizer_stream_open_internal_failure_4},

#endif

};

const pv_test_suite_t PV_NORMALIZER_STREAM_DE_TEST_SUITE = {
        .name = "normalizer_stream_de",
        .setup = test_pv_normalizer_stream_setup,
        .teardown = test_pv_normalizer_stream_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_NORMALIZER_STREAM_TEST_CASES),
        .test_cases = PV_NORMALIZER_STREAM_TEST_CASES,
};
