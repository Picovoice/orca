#include "orca/normalizer/ja/pv_normalizer_data_ja/pv_normalizer_number_data_ja.h"

const int64_t MAX_VERBALIZED_CARDINAL_JA = 1000000000000000 - 1; // 1 quadrillion

const int64_t MAX_MULTIPLIER_JA = 1000000000000; // 1 trillion
const char *NUMBER_RANGE_INBETWEEN_STRING_JA = "カラ";

const char *const ONE_TO_NINE_JA[10] = {"", "イチ", "ニ", "サン", "ヨン", "ゴ", "ロク", "ナナ", "ハチ", "キュー"};
const char *const SMALL_UNITS_JA[3] = {"セン", "ヒャク", "ジュー"};
const char *const LARGE_UNITS_JA[3] = {"チョー", "オク", "マン"};
