#include <stdlib.h>
#include <string.h>

#include "core/picovoice.h"
#include "core/pv_language.h"
#include "core/pv_language_json.h"
#include "mock/pv_mock.h"
#include "orca/normalizer/pv_normalizer.h"
#include "orca/normalizer/pv_normalizer_stream.h"
#include "test/pv_test.h"
#include "util/pv_file.h"

#include "orca/normalizer/ja/pv_normalizer_tags_ja.h"
#include "orca/normalizer/ja/pv_normalizer_stream_context_scanner_ja.h"
#include "orca/normalizer/ja/pv_normalizer_use_cases_ja.h"
#include "orca/normalizer/test_pv_normalizer_stream_helper.h"

#ifdef __PV_MOCKS__

#include "core/mock/pv_core_runtime_mock.h"
#include "orca/mock/pv_normalizer_mock.h"

#endif

static pv_normalizer_stream_t *normalizer_stream_object = NULL;
static pv_normalizer_t *normalizer_object = NULL;
static pv_language_info_t *language_info_object = NULL;

static const char LANGUAGE_INFO_PATH[] = "normalizer/ref/pv_language_info_normalizer_ja.json";
static const char TOKENIZER_DATA_PATH[] = "normalizer/tokenizer_data/ipadic.bin";

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

    char *tokenizer_data_path = pv_test_module_res_path(TOKENIZER_DATA_PATH);
    pv_test_true(tokenizer_data_path != NULL, "Failed to get tokenizer_data_path");
    if (!tokenizer_data_path) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    int32_t num_content_bytes = 0;
    void *tokenizer_data = NULL;
    status = pv_file_load(tokenizer_data_path, &num_content_bytes, &tokenizer_data);
    free(tokenizer_data_path);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }
    const void *shadow = tokenizer_data;

    status = pv_normalizer_init(language_info_object, NULL, &shadow, &normalizer_object);
    free(tokenizer_data);
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
            {"こんにちは", "世界"},
    };
    char *expected_phonemizable_text[][3] = {
            {NULL, NULL, "こんにちは 世界"},
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
    char *tokens[][14] = {
            {"猫", "が", "寝", "て", "い", "ま", "す", "。",
             "花", "が", "咲", "い", "た", "。"}
    };
    char *expected_phonemizable_text[][15] = {{}};
    expected_phonemizable_text[0][7] = "猫 が ";
    expected_phonemizable_text[0][9] = "寝 て い ";
    expected_phonemizable_text[0][11] = "ます 。 ";
    expected_phonemizable_text[0][13] = "花 ";
    expected_phonemizable_text[0][14] = "が 咲い た 。";

    for (size_t i = 0; i < PV_ARRAY_LEN(tokens); i++) {
        pv_normalizer_stream_reset(normalizer_stream_object);
        test_pv_normalizer_stream_helper_verbalizable(
                normalizer_stream_object,
                14,
                (char **) &tokens[i],
                (char **) &expected_phonemizable_text[i]);
    }
}

static void test_pv_normalizer_stream_verbalizable_decimals(void) {
    char *tokens[][4] = {
            {"1",  ".", "5", "%"},
            {"-1", ".", "5", "%"},
            {"1",  ".", " ", "5",},
    };
    char *expected_phonemizable_text[][5] = {
            {NULL, NULL, NULL, NULL, "1.5%"},
            {NULL, NULL, NULL, NULL, "-1.5%"},
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
    char *tokens[][14] = {
            {"猫", "が", "{寝|n a n i}", "て", "い", "ま", "す", "。",
             "花", "が", "咲", "い", "た", "。"},
            {"こ", "んにちは", "{", "カ", "スタム|", "n ", "a n", " i k",
             "kj}", "言葉", "{カスタム", "|n a n i kkj", "}", "。"},
    };
    char *expected_phonemizable_text[][15] = {{}, {}};
    expected_phonemizable_text[0][7] = "猫 ";
    expected_phonemizable_text[0][9] = "が {寝|n a n i} て い ";
    expected_phonemizable_text[0][11] = "ます 。 ";
    expected_phonemizable_text[0][13] = "花 ";
    expected_phonemizable_text[0][14] = "が 咲い た 。";

    expected_phonemizable_text[1][14] = "こんにちは {カスタム|n a n i kkj} 言葉 {カスタム|n a n i kkj}";

    for (size_t i = 0; i < PV_ARRAY_LEN(tokens); i++) {
        pv_normalizer_stream_reset(normalizer_stream_object);
        test_pv_normalizer_stream_helper_verbalizable(
                normalizer_stream_object,
                14,
                (char **) &tokens[i],
                (char **) &expected_phonemizable_text[i]);
    }
}

static void test_pv_normalizer_stream_invalid_custom_pron(void) {
    char *tokens[][4] = {
            {"こんにちは {カスタ", "ム|", "ㅁ ", "a n i kkj}"},
            {"{カス", "タㅁ", "|n a n i kkj}", " 言葉"},
            {"カス", "タム", "|n a n i kkj}", " 言葉"},
            {"{カス", "タム", "|n a n i kkj", " 言葉"},
            {"{", "カス", "タム|n a n i k", "kj"},
            {"{", "カス", "タム| n a n i k", "kj}"},
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
            {NULL, NULL, NULL, NULL, "CSS3 HTML15 8"},
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
            {"午",   "前", "7",  "時",    "15", "分"},
            {"11",   ":",  "43", "PM",    "",   ""},
            {"11",   ":",  "03", " pm",   "",  ""},
            {"8",    ":",  "08", " p.m.", "",   ""},
            {"7",    " ",  "a",  ".",     "m",  "."},
            {"8:19", " ",  "p",  ".",     "m",  "."},
            {"8:59", "A",  ".",  "M",     ".",  ""},
    };
    char *expected_phonemizable_text[][7] = {
            {NULL, NULL, NULL, NULL, NULL, NULL, "午前 7 時 15 分"},
            {NULL, NULL, NULL, NULL, NULL, NULL, "11:43PM"},
            {NULL, NULL, NULL, NULL, NULL, NULL, "11:03 PM"},
            {NULL, NULL, NULL, NULL, NULL, NULL, "8:08 PM"},
            {NULL, NULL, NULL, NULL, NULL, NULL, "7 AM"},
            {NULL, NULL, NULL, NULL, NULL, NULL, "8:19 PM"},
            {NULL, NULL, NULL, NULL, NULL, NULL, "8:59AM"},
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

static void test_pv_normalizer_stream_verbalized_words(void) {
    char *tokens[][14] = {
            {"猫", "が", "寝", "て", "い", "ま", "す", "。", "花", "が", "咲", "い", "た", "。"},
    };
    int32_t expected_phonemizable_size[][15] = {{}};
    expected_phonemizable_size[0][7] = 4;
    expected_phonemizable_size[0][9] = 6;
    expected_phonemizable_size[0][11] = 4;
    expected_phonemizable_size[0][13] = 2;
    expected_phonemizable_size[0][14] = 7;
    char *expected[] = {
            "猫 が 寝 て い ます 。 花 が 咲い た 。",
    };

    for (size_t i = 0; i < PV_ARRAY_LEN(tokens); i++) {
        pv_normalizer_stream_reset(normalizer_stream_object);
        test_pv_normalizer_stream_helper_phonemizable(
                normalizer_stream_object,
                14,
                (char **) &tokens[i],
                (int32_t *) expected_phonemizable_size[i],
                (char *) expected[i]);
    }
}

static void test_pv_normalizer_stream_verbalized_time(void) {
    char *tokens[][8] = {
            {"午", "前", "7",    "時", "15",    "分", "",  ""},
            {"11", ":",  "15",   "PM", "。",     "",   "",  ""},
            {"03", ":",  "00",   " ",  "a",     ".",  "m", "."},
            {"7",  " ",  "p.",   "m.", " 今日", " ",  "",  ""},
            {"8",  "am", "",     " ",  "",      "",   "",  ""},
            {"8",  " ",  "am",   "！",  "",      "",   "",  ""},
            {"8",  " ",  "a.m.", "！",  "",      "",   "",  ""},
            {"８",  " ",  "ａ",    "．",  "ｍ",     "．",  "、", ""},
            {"2",  "a",  ".",    "m",  ".",     "？",  "",  ""},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 0, 0, 0, 9},
            {0, 0, 0, 0, 0, 0, 0, 0, 3},
            {0, 0, 0, 0, 0, 0, 0, 0, 2},
            {0, 0, 0, 0, 0, 0, 0, 0, 3},
            {0, 0, 0, 0, 0, 0, 0, 0, 1},
            {0, 0, 0, 0, 0, 0, 0, 0, 3},
            {0, 0, 0, 0, 0, 0, 0, 0, 3},
            {0, 0, 0, 0, 0, 0, 0, 0, 3},
            {0, 0, 0, 0, 0, 0, 0, 0, 2},
    };
    char *expected[] = {
            "午前 ナナ 時 ジューゴ 分",
            "ゴゴジューイチジ ジューゴプン 。",
            "ゴゼンサンジ ",
            "ゴゴナナジ 今日",
            "ゴゼンハチジ",
            "ゴゼンハチジ ！",
            "ゴゼンハチジ ！",
            "ゴゼンハチジ 、",
            "ゴゼンニジ ？",
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
            {"5",    "km", "/",  "h", " ", "",  "", ""},
            {"5",    " ",  "km", "/", "h", " ", "", ""},
            {"16",   "l",  "",   "",  "",  " ", "", ""},
            {"16th", " l", "",   "",  "",  " ", "", ""},
            {"-", "1", ",",   "0",  "0",  "5", "g", ""},
            {"25", "ft", ".", "", "", "-", "lb", "."},
            {"25", "ft", "", "", "", " ", "lb", "."},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 0, 0, 0, 4},
            {0, 0, 0, 0, 0, 0, 0, 0, 5},
            {0, 0, 0, 0, 0, 0, 0, 0, 2},
            {0, 0, 0, 0, 0, 0, 0, 0, 6},
            {0, 0, 0, 0, 0, 0, 0, 0, 2},
            {0, 0, 0, 0, 0, 0, 0, 0, 4},
            {0, 0, 0, 0, 0, 0, 0, 0, 5},
    };
    char *expected[] = {
            "ゴ キロメートル マイ ジカン",
            "ゴ キロメートル マイ ジカン",
            "ジューロク リットル",
            "イチ ロク ティー エイチ エル",
            "マイナスセンゴ グラム",
            "ニジューゴ フィート ポンド 。",
            "ニジューゴ フィート ポンド 。",
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
            {"1",      ",",   "000", " ",  "リンゴ", "。", "", ""},
            {"これは", "3,",  "050", "円", "！",      "",  "", ""},
            {"3,",     "050", " ",   "円", "！",      "",  "", ""},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 0, 0, 0, 5},
            {0, 0, 0, 0, 4, 0, 0, 0, 5},
            {0, 0, 0, 0, 0, 0, 0, 0, 5},
    };
    char *expected[] = {
            "セン リンゴ 。",
            "これ は サンセンゴジュー 円 ！",
            "サンセンゴジュー 円 ！",
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
            {"1", "/", "2", "小さじ", "。", "", " ", ""},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 0, 0, 0, 7},
    };
    char *expected[] = {
            "イチ ワル ニ 小さじ 。",
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
            {"1",      "〜",  "2",  "小さじ", "。", "",  " ", ""},
            {"リンゴ", "を", "4-", "7",      "",  "。", " ", ""},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 0, 0, 0, 7},
            {0, 0, 0, 0, 0, 0, 0, 0, 8},
    };
    char *expected[] = {
            "イチ カラ ニ 小さじ 。",
            "リンゴ を ヨン カラ ナナ 。",
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
            {"これ", "は", "5", "%", "の", "水", "です", "。"},
            {"5 ",   "R",  "P", "M", " ",  "",   "",     ""},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 2, 0, 5, 7},
            {0, 0, 0, 0, 0, 0, 0, 0, 3},
    };
    char *expected[] = {
            "これ は ゴ パーセント の 水 です 。",
            "ゴ アールピーエム",
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
            {"www",   ".", "example", ".", "com",   ".", "",    ""},
            {"https", ":", "/",       "/", "hello", ".", "xyz", "/"},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 0, 0, 0, 14},
            {0, 0, 0, 0, 0, 0, 0, 0, 18},
    };
    char *expected[] = {
            "ダブリュー ダブリュー ダブリュー ドット イー エックス エー エム ピー エル イー ドット コム 。",
            "エイチ ティー ティー ピー エス コロン スラッシュ スラッシュ エイチ イー エル エル オー ドット エックス ワイ ゼット スラッシュ",
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
            {"7 EUR", " ", "7", " ",    "EUR", ".", "",   ""},
            {"$",     "4", " ", "1.25", "$",   " ", "-2", " USD"},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 0, 0, 0, 8},
            {0, 0, 0, 0, 0, 0, 0, 0, 7}
    };
    char *expected[] = {
            "ナナ ユーロ ナナ ユーロ 。",
            "ヨンドル イチドルニジューゴセント マイナスニ アメリカドル",
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
            {"これ", "は", "e",  ".",  "g", ".", "",    "カウント"},
            {"i.e.", " ", "i.", "e.", "",  " ", "prof", "."},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 0, 0, 0, 7},
            {0, 0, 0, 0, 0, 0, 0, 0, 5}
    };
    char *expected[] = {
            "これ は タトエバ カウント",
            "スナワチ スナワチ キョージュ",
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
            {"(718)", " ", "719", "-",     "9810", ".", "",    ""},
            {"(",     "8", "1",   "0",     ")",    " ", "719", ""},
            {"1",     "-", "1",   "2",     "3",    "-", "4",   "5"},
            {"(718)", " ", "719", "-9810", ".",    "",  "",    ""},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 0, 0, 0, 8},
            {0, 0, 0, 0, 0, 0, 0, 0, 6},
            {0, 0, 0, 0, 0, 0, 0, 0, 3},
            {0, 0, 0, 0, 0, 0, 0, 0, 8},
    };
    char *expected[] = {
            "、 ナナイチハチ 、 ナナイチキュー キューハチイチゼロ 。",
            "、 ハチイチゼロ 、 ナナイチキュー",
            "イチ イチニサン ヨンゴ",
            "、 ナナイチハチ 、 ナナイチキュー キューハチイチゼロ 。",
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
            {"3",     "-",     "8",     "-", "1810", ".", "",  ""},
            {"03-",   "08",    "-1999", " ", ".",    " ", "",  ""},
            {"31",    "-Apr-", "2005",  "!", "",     "",  "",  ""},
            {"1829-", "2",     "-13",   "!", "",     "",  "",  ""},
            {"202",   "4",     "-",     "0", "1",    "-", "0", "6"},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 0, 0, 0, 4},
            {0, 0, 0, 0, 0, 0, 0, 0, 5},
            {0, 0, 0, 0, 0, 0, 0, 0, 6},
            {0, 0, 0, 0, 0, 0, 0, 0, 4},
            {0, 0, 0, 0, 0, 0, 0, 0, 3},
    };
    char *expected[] = {
            "サンガツ ヨーカ センハチヒャクジューネン 。",
            "サンガツ ヨーカ センキューヒャクキュージューキューネン 。",
            "サンジューイチ エー ピー アール ニセンゴ ！",
            "センハチヒャクニジューキューネン ニガツ ジューサンニチ ！",
            "ニセンニジューヨンネン イチガツ ムイカ",
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
            {"John", " ",  "F",  ".", " ",        "Kennedy", "", ""},
            {"J. ",  "R.", " R", ".", " Tolkien", "",        "", ""},
    };
    int32_t expected_phonemizable_size[][9] = {
            {0, 0, 0, 0, 0, 0, 0, 0, 14},
            {0, 0, 0, 0, 0, 0, 0, 0, 13},
    };
    char *expected[] = {
            "ジェイ オー エイチ エヌ エフ ケー イー エヌ エヌ イー ディー ワイ",
            "ジェイ アール アール ティー オー エル ケー アイ イー エヌ",
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

static void test_pv_normalizer_stream_context_scanner_simple_cases(void) {
    pv_normalizer_use_cases_ja_t TEST_USE_CASES[] = {PV_NORMALIZER_USE_URL_NORMALIZER_JA};
    pv_normalizer_token_t t[] = {
            {.tag = PV_NORMALIZER_TAG_JA_SPECIAL_CHARACTER},
            {.tag = PV_NORMALIZER_TAG_JA_WORD},
            {.tag = PV_NORMALIZER_TAG_JA_CURRENCY_SYMBOL},
            {.tag = PV_NORMALIZER_TAG_JA_CUSTOM_PRONUNCIATION},
            {.tag = PV_NORMALIZER_TAG_JA_ABBREVIATION},
            {.tag = PV_NORMALIZER_TAG_JA_SPACE},
            {.tag = PV_NORMALIZER_TAG_JA_PUNCTUATION, .string = ""}};
    for (int32_t i = 0; i < PV_ARRAY_LEN(t); i ++) {
        bool b = pv_normalizer_stream_context_scanner_ja_is_verbalizable(
                &t[i],
                PV_ARRAY_LEN(TEST_USE_CASES),
                TEST_USE_CASES);
        pv_test_true(b, "expected phone number to not be verbalizable yet");
    }
}

static void test_pv_normalizer_stream_context_scanner_special_character(void) {
    pv_normalizer_use_cases_ja_t TEST_USE_CASES[] = {
            PV_NORMALIZER_USE_FRACTION_NORMALIZER_JA,
            PV_NORMALIZER_USE_DIGITS_SEQUENCE_NORMALIZER_JA
    };
    pv_normalizer_token_t t[] = {
            {.tag = PV_NORMALIZER_TAG_JA_SPECIAL_CHARACTER, .string = "("},
            {.tag = PV_NORMALIZER_TAG_JA_SPECIAL_CHARACTER, .string = "/"}};

    for (int32_t i = 0; i < PV_ARRAY_LEN(t); i ++) {
        bool b = pv_normalizer_stream_context_scanner_ja_is_verbalizable(
                &t[i],
                PV_ARRAY_LEN(TEST_USE_CASES),
                TEST_USE_CASES);
        pv_test_true(!b, "expected special character to not be verbalizable yet");
    }
}

static void test_pv_normalizer_stream_context_scanner_punctuation(void) {
    pv_normalizer_use_cases_ja_t TEST_USE_CASES[] = {
            PV_NORMALIZER_USE_TIME_NORMALIZER_JA,
            PV_NORMALIZER_USE_CARDINAL_NORMALIZER_JA,
            PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_JA,
            PV_NORMALIZER_USE_DECIMAL_NORMALIZER_JA,
            PV_NORMALIZER_USE_URL_NORMALIZER_JA,
            PV_NORMALIZER_USE_ABBREVIATION_NORMALIZER_JA,
    };
    pv_normalizer_token_t t_prev_cardinal = {.tag = PV_NORMALIZER_TAG_JA_CARDINAL, .string = "1"};
    pv_normalizer_token_t t_prev_p = {.tag = PV_NORMALIZER_TAG_JA_ALPHANUM_SPELL_OUT, .string = "P"};
    pv_normalizer_token_t t_prev_p2 = {.string = "P", .previous = &t_prev_cardinal};
    pv_normalizer_token_t t[] = {
            {.tag = PV_NORMALIZER_TAG_JA_PUNCTUATION, .string = "."},
            {.tag = PV_NORMALIZER_TAG_JA_PUNCTUATION, .string = ":"},
            {.tag = PV_NORMALIZER_TAG_JA_PUNCTUATION, .string = ":", .previous = &t_prev_cardinal},
            {.tag = PV_NORMALIZER_TAG_JA_PUNCTUATION, .string = ".", .previous = &t_prev_p},
            {.tag = PV_NORMALIZER_TAG_JA_PUNCTUATION, .string = ".", .previous = &t_prev_p2},
            {.tag = PV_NORMALIZER_TAG_JA_PUNCTUATION, .string = ",", .previous = &t_prev_cardinal},
            {.tag = PV_NORMALIZER_TAG_JA_PUNCTUATION, .string = ".", .previous = &t_prev_cardinal},
    };

    for (int32_t i = 0; i < PV_ARRAY_LEN(t); i ++) {
        bool b = pv_normalizer_stream_context_scanner_ja_is_verbalizable(
                &t[i],
                PV_ARRAY_LEN(TEST_USE_CASES),
                TEST_USE_CASES);
        pv_test_true(!b, "expected punctuation to not be verbalizable yet");
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

static const pv_test_case_t PV_NORMALIZER_STREAM_JA_TEST_CASES[] = {
        {"basic", test_pv_normalizer_stream_verbalizable_basic},
        {"punctuation", test_pv_normalizer_stream_verbalizable_punctuation},
        {"decimals", test_pv_normalizer_stream_verbalizable_decimals},
        {"custom_pron", test_pv_normalizer_stream_custom_verbalizable_pron},
        {"invalid custom_pron", test_pv_normalizer_stream_invalid_custom_pron},
        {"alphanum_mix", test_pv_normalizer_stream_verbalizable_alphanum_mix},
        {"measurements", test_pv_normalizer_stream_verbalizable_measurements},
        {"time", test_pv_normalizer_stream_verbalizable_time},

        {"verbalized words", test_pv_normalizer_stream_verbalized_words},
        {"verbalized time",  test_pv_normalizer_stream_verbalized_time},
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

        {"context scanner simple cases", test_pv_normalizer_stream_context_scanner_simple_cases},
        {"context scanner special character", test_pv_normalizer_stream_context_scanner_special_character},
        {"context scanner punctuation", test_pv_normalizer_stream_context_scanner_punctuation},

#ifdef __PV_MOCKS__

        {"stream open calloc failure", test_pv_normalizer_stream_open_calloc_failure},
        {"stream open internal failure 1", test_pv_normalizer_stream_open_internal_failure_1},
        {"stream open internal failure 2", test_pv_normalizer_stream_open_internal_failure_2},
        {"stream open internal failure 3", test_pv_normalizer_stream_open_internal_failure_3},
        {"stream open internal failure 4", test_pv_normalizer_stream_open_internal_failure_4},

#endif

};

const pv_test_suite_t PV_NORMALIZER_STREAM_JA_TEST_SUITE = {
        .name = "normalizer_stream_ja",
        .setup = test_pv_normalizer_stream_setup,
        .teardown = test_pv_normalizer_stream_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_NORMALIZER_STREAM_JA_TEST_CASES),
        .test_cases = PV_NORMALIZER_STREAM_JA_TEST_CASES,
};
