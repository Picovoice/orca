#include <stdlib.h>
#include <string.h>

#include "core/picovoice.h"
#include "core/pv_language.h"
#include "core/pv_language_json.h"
#include "mock/pv_mock.h"
#include "orca/normalizer/pv_normalizer.h"
#include "orca/normalizer/pv_normalizer_stream.h"
#include "test/pv_test.h"

#include "orca/normalizer/ko/pv_normalizer_tags_ko.h"
#include "orca/normalizer/test_pv_normalizer_stream_helper.h"

#ifdef __PV_MOCKS__

#include "core/mock/pv_core_runtime_mock.h"
#include "orca/mock/pv_normalizer_mock.h"

#endif

static pv_normalizer_stream_t *normalizer_stream_object = NULL;
static pv_normalizer_t *normalizer_object = NULL;
static pv_language_info_t *language_info_object = NULL;

static const char LANGUAGE_INFO_PATH[] = "normalizer/ref/pv_language_info_normalizer_ko.json";

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
            {"안녕", ".", " ", "어떻게 지내세요"},
            {"안녕", ". ", "어떻게", " 지내세요"},
    };
    char *expected_phonemizable_text[][5] = {
            {NULL, NULL, "안녕. ", "어떻게 ", "지내세요"},
            {NULL, "안녕. ", NULL, "어떻게 ", "지내세요"},
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
            {NULL, NULL, NULL, NULL, "1. 5"},
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
            {"15", " 명", " 차에", " 있어"},
            {"안", " {맞춤형|p e}", " 사람", "!"},
            {"{", "맞춤형|", "p e we", "}"},
            {"안녕 {맞춤형", "|", "p e", " we}"},
            {"안녕 {맞춤", "형|", "p e we}", " 단어"},
            {"{맞춤", "형", "|p e we}", " 단어"},
            {"{맞춤", "형|p e w", "e}", " 단어"},
    };
    char *expected_phonemizable_text[][5] = {
            {NULL, NULL, "15 명 ", "차에 ", "있어"},
            {NULL, "안 {맞춤형|p e}", " ", "사람!", NULL},
            {NULL, NULL, NULL, "{맞춤형|p e we}", NULL},
            {"안녕 ", NULL, NULL, "{맞춤형|p e we}", NULL},
            {"안녕 ", NULL, "{맞춤형|p e we}", " ", "단어"},
            {NULL, NULL, "{맞춤형|p e we}", " ", "단어"},
            {NULL, NULL, "{맞춤형|p e we}", " ", "단어"},
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
            {"안녕 {맞춤", "형|", "い ", "e we}"},
            {"{맞춤", "형い", "|p e we}", " 단어"},
            {"맞춤", "형", "|p e we}", " 단어"},
            {"{맞춤", "형", "|p e we", " 단어"},
            {"{", "맞춤", "형|p e w", "e"},
            {"{", "맞춤", "형| p e w", "e}"},
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
            {"", " 07:15", " a.m.", ".", "", ""},
            {"11", ":", "43", "PM", "", ""},
            {"11", ":", "03", " pm", " ", ""},
            {"8", ":", "08", " p.m.", "", ""},
            {"7", " ", "a", ".", "m", "."},
            {"8:19", " ", "p", ".", "m", "."},
            {"8:59", "A", ".", "M", ".", ""},
    };
    char *expected_phonemizable_text[][7] = {
            {NULL, " ", "07:15 AM", NULL, NULL, NULL, "."},
            {NULL, NULL, NULL, NULL, NULL, NULL, "11:43PM"},
            {NULL, NULL, NULL, "11:03 PM", " ", NULL, NULL},
            {NULL, NULL, NULL, "8:08 PM", NULL, NULL, NULL},
            {NULL, NULL, NULL, NULL, NULL, "7 AM", NULL},
            {NULL, NULL, NULL, NULL, NULL, "8:19 PM", NULL},
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
            {"7", "", "p.", "m.", " 오늘", " ", "", ""},
            {"8", "am", "", " ", "", "", "", ""},
            {"8", " ", "am", "!", "", "", "", ""},
            {"8", " ", "a.m.", "!", "", "", "", ""},
            {"2", " ", "a", ".", "m", ".", ",", ""},
            {"2", "a", ".", "m", ".", "?", "", ""},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 0, 0, 0, 4},
            {0, 0, 0, 0, 0, 0, 0, 0, 5},
            {0, 0, 0, 0, 0, 0, 0, 4, 0},
            {0, 0, 0, 0, 4, 2, 0, 0, 0},
            {0, 0, 0, 4, 0, 0, 0, 0},
            {0, 0, 4, 1, 0, 0, 0, 0},
            {0, 0, 4, 1, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 4, 1, 0},
            {0, 0, 0, 0, 0, 4, 0, 0},
    };
    char *expected[] = {
            "오후 열한시 사십삼분",
            "오후 열한시 십오분 .",
            "오전 세시 ",
            "오후 일곱시 오늘 ",
            "오전 여덟시 ",
            "오전 여덟시 !",
            "오전 여덟시 !",
            "오전 두시 ,",
            "오전 두시 ?",
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
            {"16to", " l", "", "", "", " ", "", ""},
            {"-", "1", ",", "0", "0", "0", "g", ""},
            {"25", "ft", ".", "", "", "-", "lb", "."},
            {"25", "ft", "", "", "", " ", "lb", "."},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 4, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 4, 0, 0},
            {0, 0, 0, 0, 0, 3, 0, 0, 0},
            {0, 5, 0, 0, 0, 2, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 4},
            {0, 0, 0, 0, 0, 0, 0, 0, 4},
            {0, 0, 0, 0, 0, 0, 0, 0, 5},
    };
    char *expected[] = {
            "시간당 오킬로미터 ",
            "시간당 오킬로미터 ",
            "십육 리터 ",
            "일 육 티 오 엘 ",
            "마이너스 천 그램",
            "이십오 피트 파운드 .",
            "이십오 피트 파운드 .",
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
            {"사과", " ", "1", ",", "000", "개", "", "."},
            {"이것은 ", "3,", "050", " ", "달러", "!", "", ""},
            {"3,", "050", " ", "달러", "!", "", "", ""},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 2, 0, 0, 0, 0, 0, 0, 3},
            {2, 0, 0, 0, 0, 4, 0, 0, 0},
            {0, 0, 0, 0, 4, 0, 0, 0, 0},
    };
    char *expected[] = {
            "사과 천 개 .",
            "이것은 삼천오십 달러 !",
            "삼천오십 달러 !",
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
            {"1", "/", "2", " ", "숟가락", ".", " ", ""},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 0, 7, 0, 0},
    };
    char *expected[] = {
            "이분의 일 숟가락 . ",
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
            {"1", "-", "2", " ", "숟가락", ".", " ", ""},
            {"사과 ", "4-", "7", " ", "먹고싶다", ".", " ", ""},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 0, 7, 0, 0},
            {2, 0, 0, 0, 0, 0, 7, 0, 0},
    };
    char *expected[] = {
            "일 에서 이 숟가락 . ",
            "사과 사 에서 칠 먹고싶다 . ",
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
            {"", "5", "%", " ", "물이", " ", "있어", "."},
            {"5 ", "R", "P", "M", " ", "", "", ""},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 2, 1, 0, 2, 0, 0, 2},
            {0, 0, 0, 0, 4, 0, 0, 0, 0},
    };
    char *expected[] = {
            "오 퍼센트 물이 있어 .",
            "분당 오회전수 ",
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
            {0, 0, 0, 0, 0, 0, 0, 0, 14},
            {0, 0, 0, 0, 0, 0, 0, 0, 17},
    };
    char *expected[] = {
            "더블유 더블유 더블유 점 이 제이 이 엠 피 엘 오 점 컴 .",
            "에이치 티 티 피 에스 쌍점 슬래시 슬래시 에이치 오 엘 에이 점 엑스 와이 제트 슬래시",
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
    };
    int32_t expected_phonemizable_size[][9] = {
            {3, 1, 0, 0, 3, 0, 0, 0, 1},
            {0, 0, 0, 0, 11, 1, 0, 7, 0},
    };
    char *expected[] = {
            "칠 유로 칠 유로.",
            "사 달러 일 달러 이십오 센트 마이너스 이 미국 달러",
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
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 9, 0, 0, 0, 0, 0, 0, 14},
            {0, 0, 0, 0, 8, 1, 0, 0, 5},
            {0, 0, 0, 0, 0, 0, 0, 0, 11},
            {0, 9, 0, 0, 0, 0, 0, 0, 14},
    };
    char *expected[] = {
            ", 칠 일 팔 , 칠 일 구 , 구 팔 일 영 .",
            ", 팔 일 영 , 칠 일 구",
            "일 , 일 이 삼 , 사 오",
            ", 칠 일 팔 , 칠 일 구 , 구 팔 일 영 .",
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
            {"1829-", "2", "-13", "!", "", "", "", ""},
            {"202", "4", "-", "0", "1", "-", "0", "6"},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 0, 0, 0, 12},
            {0, 0, 0, 16, 0, 2, 0, 0, 0},
            {0, 0, 0, 4, 0, 0, 0, 0, 0},
            {0, 0, 0, 0, 0, 0, 0, 0, 3},
    };
    char *expected[] = {
            "삼 , 팔 , 일 팔 일 영 .",
            "영 삼 , 영 팔 , 일 구 구 구 . ",
            "천팔백이십구년 이월 십삼일 !",
            "이천이십사년 일월 육일",
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
        {"verbalized digits", test_pv_normalizer_stream_verbalized_digits_sequence},
        {"verbalized dates", test_pv_normalizer_stream_verbalized_dates},

#ifdef __PV_MOCKS__

        {"stream open calloc failure", test_pv_normalizer_stream_open_calloc_failure},
        {"stream open internal failure 1", test_pv_normalizer_stream_open_internal_failure_1},
        {"stream open internal failure 2", test_pv_normalizer_stream_open_internal_failure_2},
        {"stream open internal failure 3", test_pv_normalizer_stream_open_internal_failure_3},
        {"stream open internal failure 4", test_pv_normalizer_stream_open_internal_failure_4},

#endif

};

const pv_test_suite_t PV_NORMALIZER_STREAM_KO_TEST_SUITE = {
        .name = "normalizer_stream_ko",
        .setup = test_pv_normalizer_stream_setup,
        .teardown = test_pv_normalizer_stream_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_NORMALIZER_STREAM_TEST_CASES),
        .test_cases = PV_NORMALIZER_STREAM_TEST_CASES,
};
