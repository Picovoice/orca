#include <stdlib.h>
#include <string.h>

#include "core/picovoice.h"
#include "core/pv_language.h"
#include "core/pv_language_json.h"
#include "mock/pv_mock.h"
#include "orca/normalizer/pv_normalizer.h"
#include "orca/normalizer/pv_normalizer_stream.h"
#include "test/pv_test.h"

#include "orca/normalizer/it/pv_normalizer_tags_it.h"
#include "orca/normalizer/test_pv_normalizer_stream_helper.h"

#ifdef __PV_MOCKS__

#include "core/mock/pv_core_runtime_mock.h"
#include "orca/mock/pv_normalizer_mock.h"

#endif

static pv_normalizer_stream_t *normalizer_stream_object = NULL;
static pv_normalizer_t *normalizer_object = NULL;
static pv_noun_gender_dict_t *noun_gender_dict_object = NULL;
static pv_language_info_t *language_info_object = NULL;

static const char LANGUAGE_INFO_PATH[] = "normalizer/ref/pv_language_info_normalizer_it.json";
static const char NOUN_GENDER_DICT_PATH[] = "test_data/noun_gender_dict/noun_gender_dict_it.txt";

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
            {"hello", " world"},
    };
    char *expected_phonemizable_text[][3] = {
            {NULL, "hello ", "world"},
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
            {"Hi", ".", " ", "What's up"},
            {"Hi", ". ", "What's", " up"},
    };
    char *expected_phonemizable_text[][5] = {
            {NULL, NULL, "Hi. ", "What's ", "up"},
            {NULL, "Hi. ", NULL, "What's ", "up"},
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
    char *tokens[][8] = {
            {"1", ",", "5", "%", "", "", "", ""},
            {"-1", ",", "5", "%", " ", "-1", ",", "5"},
            {"1", ",", " ", "5", "", "", "", ""},
            {"-", "2", ",", "5", "", "", "", ""},
    };
    char *expected_phonemizable_text[][9] = {
            {NULL, NULL, NULL, "1,5%", NULL, NULL, NULL, NULL, NULL},
            {NULL, NULL, NULL, "-1,5%", " ", NULL, NULL, NULL, "-1,5"},
            {NULL, NULL, "1, ", NULL, NULL, NULL, NULL, NULL, "5"},
            {NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "-2,5"},
    };

    for (size_t i = 0; i < PV_ARRAY_LEN(tokens); i++) {
        pv_normalizer_stream_reset(normalizer_stream_object);
        test_pv_normalizer_stream_helper_verbalizable(
                normalizer_stream_object,
                8,
                (char **) &tokens[i],
                (char **) &expected_phonemizable_text[i]);
    }
}

static void test_pv_normalizer_stream_custom_verbalizable_pron(void) {
    char *tokens[][4] = {
            {"15", " guys", " in", " a car"},
            {"hi", " {personalizzata|a b d ddʒ dz dʒ dʣ e f g i j k l m n o p r rr s t ts tts tʃ tʧ u v w z ŋ ŋg ŋk ɔ ɛ ɲ ʃ ʎ}", " pron", "!"},
            {"{", "personalizzata|", "d e dʒ", "}"},
            {"ciao {personalizzata", "|", "d e", " dʒ}"},
            {"ciao {persona", "lizzata|", "d e dʒ}", " parola"},
            {"{persona", "lizzata", "|d e dʒ}", " parola"},
            {"{persona", "lizzata|d e ", "dʒ}", " parola"},
    };
    char *expected_phonemizable_text[][5] = {
            {NULL, "15 ", "guys ", "in a ", "car"},
            {NULL, "hi {personalizzata|a b d ddʒ dz dʒ dʣ e f g i j k l m n o p r rr s t ts tts tʃ tʧ u v w z ŋ ŋg ŋk ɔ ɛ ɲ ʃ ʎ}", " ", "pron!", NULL},
            {NULL, NULL, NULL, "{personalizzata|d e dʒ}", NULL},
            {"ciao ", NULL, NULL, "{personalizzata|d e dʒ}", NULL},
            {"ciao ", NULL, "{personalizzata|d e dʒ}", " ", "parola"},
            {NULL, NULL, "{personalizzata|d e dʒ}", " ", "parola"},
            {NULL, NULL, "{personalizzata|d e dʒ}", " ", "parola"},
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
            {"ciao {personali", "zzata|", "ㅁ ", "d e dʒ}"},
            {"{persona", "lizzaㅁ", "|d e dʒ}", " parola"},
            {"persona", "lizzata", "|d e dʒ}", " parola"},
            {"{persona", "lizzata", "|d e dʒ", " parola"},
            {"{", "persona", "lizzata|d e d", "ʒ"},
            {"{", "persona", "lizzata| d e d", "ʒ}"},
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
            {"it's", " 07:15", " ."},
            {"00", ":", "00"},
            {"0", ":", "05"},
            {"03", ":", "00"},
    };
    char *expected_phonemizable_text[][7] = {
            {NULL, "it's ", "07:15 ", "."},
            {NULL, NULL, NULL, "00:00"},
            {NULL, NULL, NULL, "0:05"},
            {NULL, NULL, NULL, "03:00"},
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

static void test_pv_normalizer_stream_verbalized_decimals(void) {
    char *tokens[][8] = {
            {"1", ",", "5", "%", "", "", "", ""},
            {"-1", ",", "5", "%", " ", "-1", ",", "5"},
            {"-", "2", ",", "5", "", "", "", ""},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 6, 0, 0, 0, 0, 0},
            {0, 0, 0, 8, 1, 0, 0, 0, 5},
            {0, 0, 0, 0, 0, 0, 0, 0, 5},
    };
    char *expected[] = {
            "UNO VIRGOLA CINQUE PER CENTO",
            "MENO UNO VIRGOLA CINQUE PER CENTO MENO UNO VIRGOLA CINQUE",
            "MENO DUE VIRGOLA CINQUE",
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

static void test_pv_normalizer_stream_verbalized_time(void) {
    char *tokens[][3] = {
            {"it's", " 07:15", " ."},
            {"00", ":", "00"},
            {"0", ":", "05"},
            {"03", ":", "00"},
    };
    int32_t expected_phonemizable_size[][4] = {
            {0, 2, 5, 1},
            {0, 0, 0, 1},
            {0, 0, 0, 4},
            {0, 0, 0, 1},
    };
    char *expected[] = {
            "IT'S SETTE E QUINDICI .",
            "ZERO",
            "ZERO E CINQUE",
            "TRE",
    };

    for (size_t i = 0; i < PV_ARRAY_LEN(tokens); i++) {
        pv_normalizer_stream_reset(normalizer_stream_object);
        test_pv_normalizer_stream_helper_phonemizable(
                normalizer_stream_object,
                3,
                (char **) &tokens[i],
                (int32_t *) expected_phonemizable_size[i],
                (char *) expected[i]);
    }
}

static void test_pv_normalizer_stream_verbalized_measurements(void) {
    char *tokens[][8] = {
            {"5", "km", "/", "h", " ", "------", " ", ""},
            {"5", " ", "km", "/", "h", " ", "-", " "},
            {"16", "l", "", "", " ", " ", "", ""},
            // (Ted):  This is a design decision because even English normalizer don't pronounce "SIXTEENTH LITER" but rather "SIXTEENTH L".
            // In an offline discussion with Eric, we decided to keep it this way because this could mean "the sixteenth letter L".
            {"16º", " l", "", "", "", " ", "", ""},
            {"1", ".", "0", "0", "0", "g", "", ""},
            {"25", "ft", ".", "", "", "-", "lb", "."},
            {"25", "ft", "", "", "", " ", "lb", "."},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 0, 5, 0, 0},
            {0, 2, 0, 0, 0, 0, 0, 4, 0},
            {0, 0, 0, 0, 0, 3, 0, 0, 0},
            {0, 2, 0, 0, 0, 2, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 2},
            {0, 0, 0, 0, 0, 0, 0, 0, 4},
            {0, 0, 0, 0, 0, 0, 0, 0, 5},
    };

    char *expected[] = {
            "CINQUE CHILOMETRI PER ORA ",
            "CINQUE CHILOMETRI PER ORA ",
            "SEDICI LITRI ",
            "SEDICESIMO L ",
            "MILLE GRAMMI",
            "VENTICINQUE PIEDE LIBBRE .",
            "VENTICINQUE PIEDE LIBBRE .",
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
            {"1", ".", "000", " ", "apples", ".", "", ""},
            {"These are ", "3.", "050", " ", "dollars", "!", "", ""},
            {"3.", "050", " ", "dollars", "!", "", "", ""},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 2, 0, 0, 0, 0, 2},
            {4, 0, 0, 2, 0, 2, 0, 0, 0},
            {0, 0, 2, 0, 2, 0, 0, 0, 0},
    };
    char *expected[] = {
            "MILLE APPLES .",
            "THESE ARE TREMILACINQUANTA DOLLARS !",
            "TREMILACINQUANTA DOLLARS !",
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
            {"1", "/", "2", " ", "teaspoon", ".", " ", ""},
            {"3", "/", "2,3", " ", "teaspoon", ".", " ", ""},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 4, 0, 0, 3, 0, 0},
            {0, 0, 0, 6, 0, 0, 3, 0, 0},
    };
    char *expected[] = {
            "UN MEZZO TEASPOON . ",
            "TRE FRATTO DUE VIRGOLA TRE TEASPOON . ",
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
            {"1", "-", "2", " ", "teaspoons", ".", " ", ""},
            {"give me ", "4-", "7", " ", "apples", ".", " ", ""},
            {"give me ", "4–", "7", " ", "apples", ".", " ", ""},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 4, 0, 0, 3, 0, 0},
            {4, 0, 0, 4, 0, 0, 3, 0, 0},
            {4, 0, 0, 4, 0, 0, 3, 0, 0},
    };
    char *expected[] = {
            "UNO A DUE TEASPOONS . ",
            "GIVE ME QUATTRO A SETTE APPLES . ",
            "GIVE ME QUATTRO A SETTE APPLES . ",
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
            {"there is", " 5", "%", "", "", " ", "water", "."},
            {"5 ", "R", "P", "M", " ", "", "", ""},
    };
    int32_t expected_phonemizable_size[][9] = {
            {2, 2, 4, 0, 0, 1, 0, 0, 2},
            {2, 0, 0, 0, 6, 0, 0, 0, 0},
    };
    char *expected[] = {
            "THERE IS CINQUE PER CENTO WATER .",
            "CINQUE GIRI AL MINUTO ",
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
            {"www", ".", "example", ".", "com", ".", "", ""},
            {"https", ":", "/", "/", "hello", ".", "xyz", "/"},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 0, 0, 0, 8},
            {0, 0, 0, 0, 0, 0, 0, 0, 16},
    };
    char *expected[] = {
            "W W W PUNTO EXAMPLE PUNTO COM .",
            "H T T P S DUE PUNTI BARRA BARRA HELLO PUNTO X Y Z BARRA",
            // TODO: Our streamer (language-independent) currently doesn't add space to special characters, but this might be okay because Hippo might overcome this. For example, with token sequence "@", "@", "@", "@", our streamer will verbalize as "ATATATAT" as opposed to "AT AT AT AT". This was discussed and agreed on Slack channel.
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
            {2, 2, 0, 2, 0, 0, 0, 0, 2},
            {0, 0, 4, 0, 0, 10, 0, 4, 3},
            {0, 0, 4, 0, 0, 10, 0, 4, 3},
    };
    char *expected[] = {
            "SETTE EURO SETTE EURO .",
            "QUATTRO DOLLARI UN DOLLARO E VENTICINQUE CENTESIMI MENO DUE DOLLARI USA",
            "QUATTRO DOLLARI UN DOLLARO E VENTICINQUE CENTESIMI MENO DUE DOLLARI USA",
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
            {"Sig", ".", " Ted.", " Hello!"},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 1, 1, 5},
    };
    char *expected[] = {
            "SIGNOR TED . HELLO !",
    };

    for (size_t i = 0; i < PV_ARRAY_LEN(tokens); i++) {
        pv_normalizer_stream_reset(normalizer_stream_object);
        test_pv_normalizer_stream_helper_phonemizable(
                normalizer_stream_object,
                4,
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
            {"(718)", " ", "719", "-9810", ".", "", "", ""},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 9, 0, 0, 0, 0, 0, 0, 14},
            {0, 9, 0, 0, 0, 0, 0, 0, 14},
            {0, 0, 0, 0, 8, 1, 0, 0, 5},
            {0, 0, 0, 0, 0, 0, 0, 0, 11},
            {0, 9, 0, 0, 0, 0, 0, 0, 14},
    };
    char *expected[] = {
            ", SETTE UNO OTTO , SETTE UNO NOVE , NOVE OTTO UNO ZERO .",
            ", SETTE UNO OTTO , SETTE UNO NOVE , NOVE OTTO UNO ZERO .",
            ", OTTO UNO ZERO , SETTE UNO NOVE",
            "UNO , UNO DUE TRE , QUATTRO CINQUE",
            ", SETTE UNO OTTO , SETTE UNO NOVE , NOVE OTTO UNO ZERO .",
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
            {"31", "-Aprile-", "2005", "!", "", "", "", ""},
            {"1829-", "2", "-13", "!", "", "", "", ""},
            {"202", "4", "-", "0", "1", "-", "0", "6"},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 0, 0, 0, 4},
            {0, 0, 0, 4, 0, 2, 0, 0, 0},
            {0, 0, 0, 4, 0, 0, 0, 0, 0},
            {0, 0, 0, 6, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 5},
    };
    char *expected[] = {
            "TRE AGOSTO MILLEOTTOCENTODIECI .",
            "TRE AGOSTO MILLENOVECENTONOVANTANOVE . ",
            "TRENTUNO APRILE DUEMILACINQUE !",
            "TREDICI FEBBRAIO MILLEOTTOCENTOVENTINOVE !",
            "SEI GENNAIO DUEMILAVENTIQUATTRO",
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
        {"custom_pron", test_pv_normalizer_stream_custom_verbalizable_pron},
        {"invalid custom_pron", test_pv_normalizer_stream_invalid_custom_pron},
        {"alphanum_mix", test_pv_normalizer_stream_verbalizable_alphanum_mix},
        {"measurements", test_pv_normalizer_stream_verbalizable_measurements},
        {"time", test_pv_normalizer_stream_verbalizable_time},

        {"verbalized decimals", test_pv_normalizer_stream_verbalized_decimals},
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

const pv_test_suite_t PV_NORMALIZER_STREAM_IT_TEST_SUITE = {
        .name = "normalizer_stream_it",
        .setup = test_pv_normalizer_stream_setup,
        .teardown = test_pv_normalizer_stream_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_NORMALIZER_STREAM_TEST_CASES),
        .test_cases = PV_NORMALIZER_STREAM_TEST_CASES,
};
