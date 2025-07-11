#ifndef PV_NORMALIZER_TAGS_IT_H
#define PV_NORMALIZER_TAGS_IT_H

typedef enum {
    PV_NORMALIZER_TAG_IT_NONE = 0,
    PV_NORMALIZER_TAG_IT_WORD = 1,
    PV_NORMALIZER_TAG_IT_CUSTOM_PRONUNCIATION = 2,
    PV_NORMALIZER_TAG_IT_PUNCTUATION = 3,
    PV_NORMALIZER_TAG_IT_CARDINAL = 4,
    PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL = 5,
    PV_NORMALIZER_TAG_IT_NUMBER_RANGE_TO = 6,
    PV_NORMALIZER_TAG_IT_ORDINAL_MASCULINE = 7,
    PV_NORMALIZER_TAG_IT_ORDINAL_FEMININE = 8,
    PV_NORMALIZER_TAG_IT_SPECIAL_CHARACTER = 9,
    PV_NORMALIZER_TAG_IT_ALPHANUM_SPELL_OUT = 10,
    PV_NORMALIZER_TAG_IT_DECIMAL_COMMA = 11,
    PV_NORMALIZER_TAG_IT_DECIMAL_DIGITS = 12,
    PV_NORMALIZER_TAG_IT_MEASUREMENT = 13,
    PV_NORMALIZER_TAG_IT_PER_SLASH = 14,
    PV_NORMALIZER_TAG_IT_TIME_COLON = 15,
    PV_NORMALIZER_TAG_IT_TIME_MINUTES = 16,
    PV_NORMALIZER_TAG_IT_TIME_HOURS = 17,
    PV_NORMALIZER_TAG_IT_TIME_AM_PM = 18, // TODO (Ted): Italian language doesn't use AM/PM, but if decided to support it anyway then use this tag.
    PV_NORMALIZER_TAG_IT_FRACTION_SLASH = 19,
    PV_NORMALIZER_TAG_IT_DOT = 20,
    PV_NORMALIZER_TAG_IT_COLON = 21,
    PV_NORMALIZER_TAG_IT_ACRONYM = 22,
    PV_NORMALIZER_TAG_IT_TOP_LEVEL_DOMAIN = 23,
    PV_NORMALIZER_TAG_IT_CURRENCY = 24,
    PV_NORMALIZER_TAG_IT_CURRENCY_SYMBOL = 25,
    PV_NORMALIZER_TAG_IT_NEGATIVE_CURRENCY = 26,
    PV_NORMALIZER_TAG_IT_ABBREVIATION = 27,
    PV_NORMALIZER_TAG_IT_DIGITS = 28,
    PV_NORMALIZER_TAG_IT_DIGITS_WITH_PARENTHESES = 29,
    PV_NORMALIZER_TAG_IT_DIGITS_SEPARATOR = 30,
    PV_NORMALIZER_TAG_IT_DATE_SEPARATOR = 31,
    PV_NORMALIZER_TAG_IT_DATE_DAY = 32,
    PV_NORMALIZER_TAG_IT_DATE_MONTH = 33,
    PV_NORMALIZER_TAG_IT_DATE_YEAR = 34,
    PV_NORMALIZER_TAG_IT_NAME_INITIAL_LETTER = 35,
    PV_NORMALIZER_TAG_IT_NAME_INITIAL_DOT = 36,
    PV_NORMALIZER_TAG_IT_LETTER_SPELL_OUT = 37,
    PV_NORMALIZER_TAG_IT_CARDINAL_SPELL_OUT = 38,
    PV_NORMALIZER_TAG_IT_SPACE = 39,
    PV_NORMALIZER_TAG_IT_NEGATIVE_ORDINAL_MASCULINE = 40,
    PV_NORMALIZER_TAG_IT_NEGATIVE_ORDINAL_FEMININE = 41,
    PV_NORMALIZER_TAG_IT_CARDINAL_ONE = 42,
    PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL_ONE = 43,
    PV_NORMALIZER_TAG_IT_SINGLE_QUOTE = 44,
    PV_NORMALIZER_TAG_IT_NUM_TAGS = 45,
} pv_normalizer_token_tag_it_t;

static const float PV_NORMALIZER_TOKEN_TAG_IT_WEIGHTS[PV_NORMALIZER_TAG_IT_NUM_TAGS] = {
        0.f, // 0
        1.f, // 1
        100.f, // 2
        0.5f, // 3
        1.f, // 4
        1.f, // 5
        1.f, // 6
        5.f, // 7 // Needs to be larger than measurement because ordinal use "°", and "ª" for ordinal abbreviation.
        5.f, // 8 // Needs to be larger than measurement because ordinal use "°", and "ª" for ordinal abbreviation.
        1.f, // 9
        0.5f, // 10
        2.f, // 11
        4.f, // 12
        4.f, // 13
        2.f, // 14
        3.f, // 15
        3.f, // 16
        3.f, // 17
        3.f, // 18
        2.f, // 19
        1.f, // 20,
        1.f, // 21,
        10.f, // 22,
        11.f, // 23,
        5.f, // 24
        5.f, // 25
        5.f, // 26
        5.f, // 27
        2.f, // 28
        2.f, // 29
        2.f, // 30
        5.f, // 31
        5.f, // 32
        5.f, // 33
        5.f, // 34
        2.f, // 35
        2.f, // 36
        2.f, // 37
        2.f, // 38
        0.5f, // 39
        5.f, // 40
        5.f, // 41
        1.f, // 42 // Setting to the same as cardinal because it really is a special cardinal when it comes to verbalization. Changing this may affect tagger.
        1.f, // 43 // Setting to the same as cardinal because it really is a special cardinal when it comes to verbalization. Changing this may affect tagger.
        2.f, // 44
};

#endif // PV_NORMALIZER_TAGS_IT_H
