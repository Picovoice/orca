// Put all characters that can be a part of a word in the language, for example in English we have all upper and lower case alphabet letters as well as apostrophe "'".

#include "orca/normalizer/ko/pv_normalizer_data_ko/pv_normalizer_word_character_data_ko.h"
#include "orca/normalizer/pv_normalizer_data/pv_normalizer_shared_data.h"

const char *const PV_NORMALIZER_WORD_CHARACTERS_KO[PV_NORMALIZER_NUM_WORD_CHARACTERS_KO] = {
        "ㅎ",
        "ㅃ",
        "ㄳ",
        "ㅈ",
        "ㅑ",
        "ㅏ",
        "ㅛ",
        "ㅄ",
        "ㅓ",
        "ㅆ",
        "ㅚ",
        "ㅗ",
        "ㅟ",
        "ㅞ",
        "ㅔ",
        "ㄷ",
        "ㄱ",
        "ㄿ",
        "ㅣ",
        "ㄲ",
        "ㅉ",
        "ㅊ",
        "ㄻ",
        "ㅕ",
        "ㅋ",
        "ㅙ",
        "ㅀ",
        "ㅠ",
        "ㅐ",
        "ㅜ",
        "ㄵ",
        "ㄹ",
        "ㄴ",
        "ㅌ",
        "ㅍ",
        "ㅡ",
        "ㅒ",
        "ㅖ",
        "ㄸ",
        "ㄺ",
        "ㅢ",
        "ㄼ",
        "ㅂ",
        "ㅝ",
        "ㄽ",
        "ㅇ",
        "ㅘ",
        "ㄶ",
        "ㅅ",
        "ㄾ",
        "ㅁ",
};

const char *PV_ORCA_STREAM_KO_EOS_PUNCTUATIONS[] = {".", "?", "!"}; // TODO: Revisit this to ensure this is intended.
const char *PV_ORCA_STREAM_KO_FALLBACK_CUTOFF_CHARACTERS[] = {","}; // TODO: Revisit this to ensure this is intended.


