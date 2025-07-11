#include "orca/normalizer/ko/pv_normalizer_data_ko/pv_normalizer_number_data_ko.h"

const int64_t MAX_VERBALIZED_CARDINAL_KO = 1000000000000000000 - 1; // 1000 quadrillion, 100경

const char *NUMBER_RANGE_INBETWEEN_STRING_KO = "에서";

const char *const ONE_TO_NINE_KO[10] = {
        "", "일", "이", "삼", "사", "오", "육", "칠", "팔", "구"};

// 10 quadrillion, 1 trillion, 100 million, 10 thousand, 1 thousand, 1 hundred, ten
const char *const MULTIPLIERS_KO[7] = {"경", "조", "억", "만", "천", "백", "십"};
const int64_t MULTIPLIERS_KO_VALUES[7] = {10000000000000000, 1000000000000, 100000000, 10000, 1000, 100, 10};

const char *const ONE_TO_NINE_NATIVE_KO[10] = {
        "", "한", "두", "세", "네", "다섯", "여섯", "일곱", "여덟", "아홉"};
const char *const TENS_NATIVE_KO[10] = {
        "", "열", "스물", "서른" ,"마흔", "쉰", "예순", "일흔", "여든", "아흔"};

const char *const IGNORE_NATIVE_CASES_KO[1] = {
        "개월"};
const char *const NATIVE_CASES_KO[12] = {
        "명", "마리", "개", "권" ,"병", "장", "켤레", "벌", "채", "살", "가지", "시"};
