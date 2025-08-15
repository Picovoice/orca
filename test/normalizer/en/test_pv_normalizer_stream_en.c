#include <stdlib.h>
#include <string.h>

#include "core/picovoice.h"
#include "core/pv_language.h"
#include "core/pv_language_json.h"
#include "mock/pv_mock.h"
#include "orca/normalizer/pv_normalizer.h"
#include "orca/normalizer/pv_normalizer_stream.h"
#include "test/pv_test.h"

#include "orca/normalizer/en/pv_normalizer_tags_en.h"
#include "orca/normalizer/test_pv_normalizer_stream_helper.h"

#ifdef __PV_MOCKS__

#include "core/mock/pv_core_runtime_mock.h"
#include "orca/mock/pv_normalizer_mock.h"

#endif

static pv_normalizer_stream_t *normalizer_stream_object = NULL;
static pv_normalizer_t *normalizer_object = NULL;
static pv_language_info_t *language_info_object = NULL;

static const char LANGUAGE_INFO_PATH[] = "normalizer/ref/pv_language_info_normalizer_en.json";

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

    status = pv_normalizer_init(language_info_object, NULL, NULL, &normalizer_object);
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
    char *tokens[][4] = {
            {"1", ".", "5", "%"},
            {"-1", ".", "5", "%"},
            {
                    "1",
                    ".",
                    " ",
                    "5",
            },
    };
    char *expected_phonemizable_text[][5] = {
            {NULL, NULL, NULL, "1.5%", NULL},
            {NULL, NULL, NULL, "-1.5%", NULL},
            {NULL, NULL, "1. ", NULL, "5"},
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
            {"15", " guys", " in", " a car"},
            {"hi", " {custom|K AH}", " pron", "!"},
            {"{", "custom|", "K AH", "}"},
            {"hi {custom", "|", "K", " AH}"},
            {"hi {cus", "tom|", "K AH}", " word"},
            {"{cus", "tom", "|K AH}", " word"},
            {"{cus", "tom|K A", "H}", " word"},
    };
    char *expected_phonemizable_text[][5] = {
            {NULL, NULL, "15 guys ", "in a ", "car"},
            {NULL, "hi {custom|K AH}", " ", "pron!", NULL},
            {NULL, NULL, NULL, "{custom|K AH}", NULL},
            {"hi ", NULL, NULL, "{custom|K AH}", NULL},
            {"hi ", NULL, "{custom|K AH}", " ", "word"},
            {NULL, NULL, "{custom|K AH}", " ", "word"},
            {NULL, NULL, "{custom|K AH}", " ", "word"},
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
            {"hi {cust", "om|", "ㅁ ", "AH}"},
            {"{cus", "toㅁ", "|K AH}", " word"},
            {"cus", "tom", "|K AH}", " word"},
            {"{cus", "tom", "|K AH", " word"},
            {"{", "cus", "tom|K A", "H"},
            {"{", "cus", "tom| K A", "H}"},
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
            {"it's", " 07:15", " a.m.", ".", "", ""},
            {"11", ":", "43", "PM", "", ""},
            {"11", ":", "03", " pm", " ", ""},
            {"8", ":", "08", " p.m.", "", ""},
            {"7", " ", "a", ".", "m", "."},
            {"8:19", " ", "p", ".", "m", "."},
            {"8:59", "A", ".", "M", ".", ""},
    };
    char *expected_phonemizable_text[][7] = {
            {NULL, "it's ", "07:15 ", NULL, NULL, NULL, "AM."},
            {NULL, NULL, NULL, NULL, NULL, NULL, "11:43PM"},
            {NULL, NULL, NULL, "11:03 ", "PM ", NULL, NULL},
            {NULL, NULL, NULL, "8:08 ", NULL, NULL, "PM"},
            {NULL, NULL, NULL, NULL, NULL, NULL, "7 AM"},
            {NULL, "8:19 ", NULL, NULL, NULL, NULL, "PM"},
            {NULL, NULL, NULL, NULL, NULL, NULL, "8:59A.M."},
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
            {"11", ":", "43", "PM", "", "", "", ""},
            {"11", ":", "15", "PM", ".", "", "", ""},
            {"03", ":", "00", " ", "a", ".", "m", "."},
            {"7", " ", "p.", "m.", " today", " ", "", ""},
            {"8", "am", "", " ", "", "", "", ""},
            {"8", " ", "am", "!", "", "", "", ""},
            {"8", " ", "a.m.", "!", "", "", "", ""},
            {"2", " ", "a", ".", "m", ".", ",", ""},
            {"2", "a", ".", "m", ".", "?", "", ""},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 0, 0, 0, 5},
            {0, 0, 0, 0, 0, 0, 0, 0, 4},
            {0, 0, 0, 2, 0, 0, 0, 0, 1},
            {0, 0, 0, 0, 4, 2, 0, 0, 0},
            {0, 0, 0, 3, 0, 0, 0, 0},
            {0, 0, 0, 4, 0, 0, 0, 0},
            {0, 0, 0, 4, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 4, 0},
            {0, 0, 0, 0, 0, 3, 0, 0},
    };
    char *expected[] = {
            "ELEVEN FORTY THREE {|P IY EH M}",
            "ELEVEN FIFTEEN {|P IY EH M} .",
            "THREE {|EY EH M}",
            "SEVEN {|P IY EH M} TODAY ",
            "EIGHT {|EY EH M} ",
            "EIGHT {|EY EH M} !",
            "EIGHT {|EY EH M} !",
            "TWO {|EY EH M} ,",
            "TWO {|EY EH M} ?",
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
            {"16th", " l", "", "", "", " ", "", ""},
            {"10", "-15", "°C", ".", " (or", " ", "22°C", ")."},
            {"1", ",", "0", "0", "0", "", "g", ""},
            {"25", "ft", ".", "", "", "-", "lb", "."},
            {"25", "ft", "", "", "", " ", "lb", "."},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 5, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 6, 0, 0},
            {0, 0, 0, 0, 0, 0, 3, 0, 0},
            {0, 2, 0, 0, 0, 2, 0, 0, 0},
            {0, 0, 0, 0, 8, 3, 0, 7, 1},
            {0, 0, 0, 0, 0, 0, 0, 0, 4},
            {0, 0, 0, 0, 0, 0, 0, 0, 6},
            {0, 0, 0, 0, 0, 0, 0, 0, 7},
    };
    char *expected[] = {
            "FIVE KILOMETERS PER HOUR ",
            "FIVE KILOMETERS PER HOUR ",
            "SIXTEEN LITERS ",
            "SIXTEENTH L ",
            "TEN TO FIFTEEN DEGREES CELSIUS . , OR TWENTY TWO DEGREES CELSIUS ,.",
            "ONE THOUSAND GRAMS",
            "TWENTY FIVE FOOT POUNDS .",
            "TWENTY FIVE FOOT POUNDS .",
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
            {"1", ",", "000", " ", "apples", ".", "", ""},
            {"These are ", "3,", "050", " ", "dollars", "!", "", ""},
            {"3,", "050", " ", "dollars", "!", "", "", ""},
            {"", "-", "9", ".", "2", "", "", ""},
            {"", "−", "9", ".", "2", "", "", ""},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 0, 0, 0, 6},
            {4, 0, 0, 0, 0, 8, 0, 0, 0},
            {0, 0, 0, 0, 8, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 5},
            {0, 0, 0, 0, 0, 0, 0, 0, 5},
    };
    char *expected[] = {
            "ONE THOUSAND APPLES .",
            "THESE ARE THREE THOUSAND FIFTY DOLLARS !",
            "THREE THOUSAND FIFTY DOLLARS !",
            "NEGATIVE NINE POINT TWO",
            "NEGATIVE NINE POINT TWO",
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
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 0, 7, 0, 0},
    };
    char *expected[] = {
            "ONE HALF TEASPOON . ",
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
            {0, 0, 0, 0, 0, 0, 7, 0, 0},
            {4, 0, 0, 0, 0, 0, 7, 0, 0},
            {4, 0, 0, 0, 0, 0, 7, 0, 0},
    };
    char *expected[] = {
            "ONE TO TWO TEASPOONS . ",
            "GIVE ME FOUR TO SEVEN APPLES . ",
            "GIVE ME FOUR TO SEVEN APPLES . ",
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
            {2, 2, 2, 0, 0, 1, 0, 0, 2},
            {0, 0, 0, 0, 8, 0, 0, 0, 0},
    };
    char *expected[] = {
            "THERE IS FIVE PERCENT WATER .",
            "FIVE REVOLUTIONS PER MINUTE ",
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
            {"such", " as ", ".com,", " .", "net", ",", " and ", ".org"},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 0, 0, 0, 8},
            {0, 0, 0, 0, 0, 0, 0, 0, 14},
            {0, 4, 3, 1, 0, 3, 3, 0, 2},
    };
    char *expected[] = {
            "W W W DOT EXAMPLE DOT COM .",
            "H T T P S COLON SLASH SLASH HELLO DOT X Y Z SLASH",
            "SUCH AS DOT COM , DOT NET , AND DOT ORG",
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
            {3, 1, 0, 0, 3, 0, 0, 0, 1},
            {0, 0, 0, 0, 15, 1, 0, 9, 0},
            {0, 0, 0, 0, 15, 1, 0, 9, 0},
    };
    char *expected[] = {
            "SEVEN EUROS SEVEN EUROS.",
            "FOUR DOLLARS ONE DOLLAR AND TWENTY FIVE CENTS NEGATIVE TWO UNITED STATES DOLLARS",
            "FOUR DOLLARS ONE DOLLAR AND TWENTY FIVE CENTS NEGATIVE TWO UNITED STATES DOLLARS",
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
            {"this", " ", "e", ".", "g", ".", " ", "counts"},
            {"i.e.", " ", "i.", "e.", "", " ", "prof", "."},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 2, 0, 0, 0, 3, 1, 0, 1},
            {3, 1, 0, 3, 0, 1, 0, 1, 0},
    };
    char *expected[] = {
            "THIS FOR EXAMPLE COUNTS",
            "THAT IS THAT IS PROFESSOR",
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
            {"(", "8", "1", "0", ")", " ", "719", ""},
            {"1", "-", "1", "2", "3", "-", "4", "5"},
            {"(718)", " ", "719", "-9810", ".", "", "", ""},
            {"(718)", " ", "719", "‒9810", ".", "", "", ""},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 9, 0, 0, 0, 0, 0, 0, 14},
            {0, 0, 0, 0, 8, 1, 0, 0, 5},
            {0, 0, 0, 0, 0, 0, 0, 0, 11},
            {0, 9, 0, 0, 0, 0, 0, 0, 14},
            {0, 9, 0, 0, 0, 0, 0, 0, 14},
    };
    char *expected[] = {
            ", SEVEN ONE EIGHT , SEVEN ONE NINE , NINE EIGHT ONE ZERO .",
            ", EIGHT ONE ZERO , SEVEN ONE NINE",
            "ONE , ONE TWO THREE , FOUR FIVE",
            ", SEVEN ONE EIGHT , SEVEN ONE NINE , NINE EIGHT ONE ZERO .",
            ", SEVEN ONE EIGHT , SEVEN ONE NINE , NINE EIGHT ONE ZERO .",
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
            {"the ", "8", "", " May ", "", "", "2023", " "},
            {"", "31", " ", "January", " ", "2020", "", ". "},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 0, 0, 0, 8},
            {0, 0, 0, 10, 0, 2, 0, 0, 0},
            {0, 0, 0, 16, 0, 0, 0, 0, 0},
            {0, 0, 0, 11, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 10},
            {2, 0, 0, 0, 0, 0, 0, 14, 0},
            {0, 0, 0, 0, 0, 0, 0, 17, 0},
    };
    char *expected[] = {
            "MARCH EIGHTH , EIGHTEEN TEN .",
            "MARCH EIGHTH , NINETEEN NINETY NINE . ",
            "THE THIRTY FIRST OF APRIL , TWO THOUSAND FIVE !",
            "FEBRUARY THIRTEENTH , EIGHTEEN TWENTY NINE !",
            "JANUARY SIXTH , TWENTY TWENTY FOUR",
            "THE EIGHTH OF MAY , TWENTY TWENTY THREE ",
            "THE THIRTY FIRST OF JANUARY , TWENTY TWENTY . ",
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

const pv_test_suite_t PV_NORMALIZER_STREAM_EN_TEST_SUITE = {
        .name = "normalizer_stream_en",
        .setup = test_pv_normalizer_stream_setup,
        .teardown = test_pv_normalizer_stream_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_NORMALIZER_STREAM_TEST_CASES),
        .test_cases = PV_NORMALIZER_STREAM_TEST_CASES,
};
