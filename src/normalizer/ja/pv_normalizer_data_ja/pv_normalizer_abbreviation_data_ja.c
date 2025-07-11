#include "orca/normalizer/ja/pv_normalizer_data_ja/pv_normalizer_abbreviation_data_ja.h"

const char *const PV_NORMALIZER_ABBREVIATIONS_JA[PV_NORMALIZER_NUM_ABBREVIATIONS_JA] = {
        "E.G.", // 0
        "I.E.", // 1
        "ETC.", // 2

        "MR.", // 3
        "MRS.", // 4
        "MS.", // 5
        "DR.", // 6
        "PROF.", // 7
        "REV.", // 8

        "LTD.", // 9
        "INC.", // 10
        "CO.", // 11
        "CORP.", // 12
        "GOVT.", // 13
        "UNIV.", // 14
        "DEPT.", // 15

        "AVE.", // 16
        "ST.", // 17
        "RD.", // 18
        "BLVD.", // 19
        "APT.", // 20
        "BLDG.", // 21

        "HRS.", // 22
        "MIN.", // 23
        "SEC.", // 24

        "EST.", // 25
        "VIZ.", // 26
        "APPROX.", // 27
        "ABBR.", // 28
        "INST.", // 29

        "VS.", // 30
        "P.S.", // 31
        "FIG.", // 32
        "AL.", // 33

        "PP.", // 34
        "CH.", // 35
        "VOL.", // 36
        "VOLS.", // 37
        "ED.", // 38

        "REP.", // 39
        "SEN.", // 40
        "GOV.", // 41
        "PRES.", // 42
        "GEN.", // 43

        "JR.", // 44
        "SR.", // 45
        "LT.", // 46

        "MT.", //47

        "INT.", //48
        "EXT.", //49
        "PH.D.", //50
        "B.SC.", //51
        "D.SC.", //52
};

const char *const PV_NORMALIZER_ABBREVIATIONS_VERBALIZED_JA[PV_NORMALIZER_NUM_ABBREVIATIONS_JA] = {
        "タトエバ", // 0
        "スナワチ", // 1
        "ソノタ", // 2

        "ミスター", // 3
        "ミス", // 4
        "ミス", // 5
        "ハカセ", // 6
        "キョージュ", // 7
        "ボクシ", // 8

        "ユーゲンガイシャ", // 9
        "カブシキガイシャ", // 10
        "カイシャ", // 11
        "ホージン", // 12
        "セーフ", // 13
        "ダイガク", // 14
        "ブモン", // 15

        "オオドオリー", // 16
        "トオリー", // 17
        "ミチ", // 18
        "オオドオリー", // 19
        "アパート", // 20
        "ビル", // 21

        "ジカン", // 22
        "フン", // 23
        "ビョー", // 24

        "セツリツ", // 25
        "スナワチ", // 26
        "オオヨソ", // 27
        "リャクゴ", // 28
        "キカン", // 29

        "タイ", // 30
        "ツイシン", // 31
        "ズ", // 32
        "ソノタ", // 33

        "ページ", // 34
        "ショー", // 35
        "カン", // 36
        "カン", // 37
        "ハン", // 38

        "ダイヒョー", // 39
        "ジョーインギイン", // 40
        "チジ", // 41
        "ダイトーリョー", // 42
        "ショーグン", // 43

        "ジュニア", // 44
        "シニア", // 45
        "チューイ", // 46

        "ヤマ", // 47

        "ナイセン", // 48
        "ガイセン", // 49
        "ハカセゴー", // 50
        "リガクシ", // 51
        "リガクハカセ", // 52
};

/* Partial abbreviations that can potentially be tokenized before complete abbreviation is available, such as
 * "E." for "E.G." or "I." in "I.E.".
 * Used in normalizer_stream_context to determine if a token is verbalizable.
 */
const char *const PV_NORMALIZER_PARTIAL_ABBREVIATIONS_JA[PV_NORMALIZER_NUM_PARTIAL_ABBREVIATIONS_JA] = {
        "E.",
        "I.",
        "P.",
        "PH.",
        "B.",
        "D.",
};
