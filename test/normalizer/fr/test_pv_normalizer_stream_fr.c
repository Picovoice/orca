#include <stdlib.h>
#include <string.h>

#include "core/picovoice.h"
#include "core/pv_language.h"
#include "core/pv_language_json.h"
#include "mock/pv_mock.h"
#include "orca/normalizer/pv_normalizer.h"
#include "orca/normalizer/pv_normalizer_stream.h"
#include "test/pv_test.h"

#include "orca/normalizer/fr/pv_normalizer_tags_fr.h"
#include "orca/normalizer/test_pv_normalizer_stream_helper.h"

#ifdef __PV_MOCKS__

#include "core/mock/pv_core_runtime_mock.h"
#include "orca/mock/pv_normalizer_mock.h"

#endif

static pv_normalizer_stream_t *normalizer_stream_object = NULL;
static pv_normalizer_t *normalizer_object = NULL;
static pv_noun_gender_dict_t *noun_gender_dict_object = NULL;
static pv_language_info_t *language_info_object = NULL;

static const char LANGUAGE_INFO_PATH[] = "normalizer/ref/pv_language_info_normalizer_fr.json";
static const char NOUN_GENDER_DICT_PATH[] = "test_data/noun_gender_dict/noun_gender_dict_fr.txt";

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
            {"Salut", " copain"},
    };
    char *expected_phonemizable_text[][3] = {
            {NULL, "Salut ", "copain"},
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
            {"Salut", ".", " ", "Comment ça va"},
            {"Salut", ". ", "Comment", " ça va"},
    };
    char *expected_phonemizable_text[][6] = {
            {NULL, NULL, "Salut. ", "Comment ça ", "va", NULL},
            {NULL, "Salut. ", NULL, "Comment ça ", "va", NULL},
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
            {"1", ",", "5", ",", "5"},
            {"1", "", ",5", "", ",5"},
    };
    char *expected_phonemizable_text[][6] = {
            {NULL, NULL, NULL, NULL, "1,5,", "5"},
            {NULL, NULL, NULL, NULL, "1,5", ",5"},
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
            {"15", " gars", " dans", " une voiture"},
            {"salut", " {personalized|p e}", " person", "!"},
            {"{", "personalized|", "p e œ̃", "}"},
            {"salut {personalized", "|", "p e", " œ̃}"},
            {"salut {person", "alized|", "p e œ̃}", " mot"},
            {"{person", "alized", "|p e œ̃}", " mot"},
            {"{person", "alized|p e œ", "̃}", " mot"},
    };
    char *expected_phonemizable_text[][5] = {
            {NULL, NULL, NULL, "15 gars dans une ", "voiture"},
            {NULL, "salut {personalized|p e}", " ", "person!", NULL},
            {NULL, NULL, NULL, "{personalized|p e œ̃}", NULL},
            {"salut ", NULL, NULL, "{personalized|p e œ̃}", NULL},
            {"salut ", NULL, "{personalized|p e œ̃}", " ", "mot"},
            {NULL, NULL, "{personalized|p e œ̃}", " ", "mot"},
            {NULL, NULL, "{personalized|p e œ̃}", " ", "mot"},
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
            {"salut {personali", "zed|", "ㅁ ", "e œ̃}"},
            {"{person", "liㅁ", "|p e œ̃}", " mot"},
            {"person", "alized", "|p e œ̃}", " mot"},
            {"{person", "alized", "|p e œ̃", " mot"},
            {"{", "person", "alized|p e œ", "̃"},
            {"{", "person", "alized| p e œ", "̃}"},
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
            {"Il est", " 07:15", ".", ""},
            {"11", ":", "43", ""},
            {"23", ":", "03", ""},
            {"8", "h", "8", "m"},
            {"7", "h", "6", "'"}};
    char *expected_phonemizable_text[][7] = {
            {"Il ", "est ", NULL, NULL, "07:15."},
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
            {0, 0, 0, 0, 0, 6},
            {0, 0, 0, 0, 0, 7},
            {0, 0, 0, 0, 0, 3},
            {0, 0, 0, 0, 0, 4},
            {0, 0, 0, 4, 0, 0},
            {0, 0, 0, 0, 0, 4},
            {0, 0, 0, 4, 0, 0},
            {0, 0, 0, 0, 0, 7},
            {0, 0, 0, 0, 0, 9},
            {0, 0, 0, 0, 0, 7},
    };
    char *expected[] = {
            "ONZE HEURES QUARANTE TROIS",
            "VINGT TROIS HEURES QUINZE .",
            "TROIS HEURES",
            "SEPT HEURES NEUF",
            "HUIT HEURES !",
            "DEUX HEURES ,",
            "DEUX HEURES ?",
            "HUIT HEURES TRENTE DU MATIN",
            "DOUZE HEURES TRENTE CINQ DE L'APRÈS-MIDI",
            "ONZE HEURES QUINZE DE L'APRÈS-MIDI",
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
            {"16er", " l", "", "", "", " ", "", ""},
            {"-", "1", ".", "0", "0", "0", "g", ""},
            {"25", "ft", "", "", "", " ", "lb", "."},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 0, 0, 0, 5},
            {0, 0, 0, 0, 0, 0, 0, 0, 6},
            {0, 0, 0, 0, 0, 0, 0, 0, 3},
            {0, 0, 0, 0, 0, 0, 0, 0, 4},
            {0, 0, 0, 0, 0, 0, 0, 0, 4},
            {0, 0, 0, 0, 0, 0, 0, 0, 7},
    };
    char *expected[] = {
            "CINQ KILOMÈTRES PAR HEURE ",
            "CINQ KILOMÈTRES PAR HEURE ",
            "SEIZE LITRES ",
            "SEIZIÈME L ",
            "MOINS MILLE GRAMMES",
            "VINGT CINQ PIED POUNDS .",
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
            {"1", ".", "000", " ", "pommes", ".", "", ""},
            {"C'est ", "3.", "050", " ", "dollars", "!", "", ""},
            {"3.", "050", " ", "dollars", "!", "", "", ""},
            {"1", " ", "heure", " par", " ", "séance", ".", ""},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 0, 0, 0, 4},
            {2, 0, 0, 0, 0, 8, 0, 0, 0},
            {0, 0, 0, 0, 8, 0, 0, 0, 0},
            {0, 0, 0, 0, 6, 0, 0, 0, 2},
    };
    char *expected[] = {
            "MILLE POMMES .",
            "C'EST TROIS MILLE CINQUANTE DOLLARS !",
            "TROIS MILLE CINQUANTE DOLLARS !",
            "UN HEURE PAR SÉANCE .",
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
            {"1", "/", "2", " ", "cuillère à café", ".", " ", ""},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 8, 0, 3, 0, 0},
    };
    char *expected[] = {
            "UN MOITIÉ CUILLÈRE À CAFÉ . ",
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
            {"1", "-", "2", " ", "cuillère à café", ".", " ", ""},
            {"1", "–", "2", " ", "cuillère à café", ".", " ", ""},
            {"donnez-moi ", "4-", "7", " ", "pommes", ".", " ", ""},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 8, 0, 3, 0, 0},
            {0, 0, 0, 0, 8, 0, 3, 0, 0},
            {2, 0, 0, 0, 0, 0, 7, 0, 0},
    };
    char *expected[] = {
            "UN À DEUX CUILLÈRE À CAFÉ . ",
            "UN À DEUX CUILLÈRE À CAFÉ . ",
            "DONNEZ-MOI QUATRE À SEPT POMMES . ",
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
            {"c'est", " 5", "%", "", "", " ", "eau", "."},
            {"5 ", "R", "P", "M", " ", "", "", ""},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 2, 4, 0, 0, 0, 0, 0, 3},
            {0, 0, 0, 0, 0, 0, 0, 0, 8},
    };
    char *expected[] = {
            "C'EST CINQ POUR CENT EAU .",
            "CINQ RÉVOLUTIONS PAR MINUTE ",
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
            {"www", ".", "exemple", ".", "com", ".", "", ""},
            {"https", ":", "/", "/", "salut", ".", "xyz", "/"},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 0, 0, 0, 8},
            {0, 0, 0, 0, 0, 0, 0, 0, 20},
    };
    char *expected[] = {
            "W W W POINT EXEMPLE POINT COM .",
            "H T T P S DEUX-POINTS BARRE OBLIQUE BARRE OBLIQUE SALUT POINT X Y Z BARRE OBLIQUE",
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
            {0, 0, 0, 0, 0, 0, 0, 0, 23},
            {0, 0, 0, 0, 0, 0, 0, 0, 23},
    };
    char *expected[] = {
            "SEPT EUROS SEPT EUROS .",
            "QUATRE DOLLARS UN DOLLAR AVEC VINGT CINQ CENTS MOINS DEUX DOLLARS AMÉRICAIN",
            "QUATRE DOLLARS UN DOLLAR AVEC VINGT CINQ CENTS MOINS DEUX DOLLARS AMÉRICAIN",
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
            {"cela", " ", "e", ".", "g", ".", " ", "compte"},
            {"vs.", " ", "v", "s", ".", " ", "prof", "."},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 2, 0, 0, 0, 3, 1, 0, 1},
            {1, 1, 0, 0, 1, 1, 0, 1, 0},
    };
    char *expected[] = {
            "CELA PAR EXEMPLE COMPTE",
            "CONTRE CONTRE PROFESSEUR",
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
            {"718", "‒", "719", "‒", "9810", ".", "", ""},
            {"718", "-", "719", "-", "9810", ".", "", ""},
            {"1", "-", "1", "2", "3", "-", "4", "5"},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 0, 0, 0, 20},
            {0, 0, 0, 0, 0, 0, 0, 0, 20},
            {0, 0, 0, 0, 0, 0, 0, 0, 11},
    };
    char *expected[] = {
            "SEPT UN HUIT , SEPT UN NEUF , NEUF HUIT UN ZÉRO .",
            "SEPT UN HUIT , SEPT UN NEUF , NEUF HUIT UN ZÉRO .",
            "UN , UN DEUX TROIS , QUATRE CINQ",
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
            {"31", "-abr-", "2005", "!", "", "", "", ""},
            {"1829-", "2-", "13", "!", "", "", "", ""},
            {"202", "4", "-", "0", "1", "-", "0", "6"},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 0, 0, 0, 10},
            {0, 0, 0, 0, 0, 0, 0, 0, 14},
            {0, 0, 0, 12, 0, 0, 0, 0, 0},
            {0, 0, 0, 14, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 11},
    };
    char *expected[] = {
            "TROIS AOÛT MILLE HUIT CENT DIX .",
            "TROIS AOÛT MILLE NEUF CENT QUATRE-VINGT-DIX NEUF . ",
            "TRENTE ET UN ABR DEUX MILLE CINQ !",
            "TREIZE FÉVRIER MILLE HUIT CENT VINGT NEUF !",
            "SIX JANVIER DEUX MILLE VINGT QUATRE",
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

const pv_test_suite_t PV_NORMALIZER_STREAM_FR_TEST_SUITE = {
        .name = "normalizer_stream_fr",
        .setup = test_pv_normalizer_stream_setup,
        .teardown = test_pv_normalizer_stream_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_NORMALIZER_STREAM_TEST_CASES),
        .test_cases = PV_NORMALIZER_STREAM_TEST_CASES,
};
