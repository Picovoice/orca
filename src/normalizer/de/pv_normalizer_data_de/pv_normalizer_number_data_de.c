#include "orca/normalizer/de/pv_normalizer_data_de/pv_normalizer_number_data_de.h"

const int64_t MAX_VERBALIZED_CARDINAL_DE = 1000000000000000 - 1; // 1 Billiarde

const int64_t MAX_MULTIPLIER_DE = 1000000000000; // 1 Billion
const char *NUMBER_RANGE_INBETWEEN_STRING_DE = "BIS";

const char *const ONE_TO_NINETEEN_DE[20] = {
        "", "EINS", "ZWEI", "DREI", "VIER", "FÜNF", "SECHS", "SIEBEN", "ACHT", "NEUN", "ZEHN", "ELF", "ZWÖLF",
        "DREIZEHN", "VIERZEHN", "FÜNFZEHN", "SECHZEHN", "SIEBZEHN", "ACHTZEHN", "NEUNZEHN"};
const char *const TENS_DE[9] = {
        "", "ZWANZIG", "DREISSIG", "VIERZIG", "FÜNFZIG", "SECHZIG", "SIEBZIG", "ACHTZIG",
        "NEUNZIG"};
const char *const MULTIPLIERS_SINGULAR_DE[5] = {"", "BILLION", "MILLIARDE", "MILLION", "TAUSEND"};
const char *const MULTIPLIERS_PLURAL_DE[5] = {"", "BILLIONEN", "MILLIARDEN", "MILLIONEN", "TAUSEND"};

const char *const ORDINAL_KEYS_SINGULAR_DE[PV_NORMALIZER_NUM_ORDINALS_DE] = {
        "NULL", "EINS", "ZWEI", "DREI", "VIER", "FÜNF", "SECHS", "SIEBEN", "ACHT", "NEUN", "ZEHN", "ELF", "ZWÖLF",
        "DREIZEHN", "VIERZEHN", "FÜNFZEHN", "SECHZEHN", "SIEBZEHN", "ACHTZEHN", "NEUNZEHN", "ZWANZIG", "DREISSIG",
        "VIERZIG", "FÜNFZIG", "SECHZIG", "SIEBZIG", "ACHTZIG", "NEUNZIG", "HUNDERT", "TAUSEND", "MILLION", "MILLIARDE",
        "BILLION"};
const char *const ORDINAL_KEYS_PLURAL_DE[PV_NORMALIZER_NUM_ORDINALS_DE] = {
        "NULL", "EINS", "ZWEI", "DREI", "VIER", "FÜNF", "SECHS", "SIEBEN", "ACHT", "NEUN", "ZEHN", "ELF", "ZWÖLF",
        "DREIZEHN", "VIERZEHN", "FÜNFZEHN", "SECHZEHN", "SIEBZEHN", "ACHTZEHN", "NEUNZEHN", "ZWANZIG", "DREISSIG",
        "VIERZIG", "FÜNFZIG", "SECHZIG", "SIEBZIG", "ACHTZIG", "NEUNZIG", "HUNDERTE", "TAUSENDE", "MILLIONEN", "MILLIARDEN",
        "BILLIONEN"};

const char *const ORDINAL_VALUES_ROOT_DE[PV_NORMALIZER_NUM_ORDINALS_DE] = {
        "NULLTE", "ERSTE", "ZWEITE", "DRITTE", "VIERTE", "FÜNFTE", "SECHSTE", "SIEBTE", "ACHTE", "NEUNTE", "ZEHNTE",
        "ELFTE", "ZWÖLFTE", "DREIZEHNTE", "VIERZEHNTE", "FÜNFZEHNTE", "SECHZEHNTE", "SIEBZEHNTE", "ACHTZEHNTE",
        "NEUNZEHNTE", "ZWANZIGSTE", "DREISSIGSTE", "VIERZIGSTE", "FÜNFZIGSTE", "SECHZIGSTE", "SIEBZIGSTE", "ACHTZIGSTE",
        "NEUNZIGSTE", "HUNDERTSTE", "TAUSENDSTE", "MILLIONSTE", "MILLIARDSTE", "BILLIONSTE"};

const char *const ORDINAL_VALUES_SUFFIXES_DE[12][4] = {
        //            | masculine | feminine | neuter | plural
        //----------------------------------------------------
        // nominative | ...
        // genitive   | ...
        // dative     | ...
        // accusative | ...

        // strong declension (without article)
        {"R", "",  "S", "" },
        {"N", "R", "N", "R"},
        {"M", "R", "M", "N"},
        {"N", "",  "S", "" },

        // weak declension (with definite article)
        {"",  "",  "",  "N"},
        {"N", "N", "N", "N"},
        {"N", "N", "N", "N"},
        {"N", "",  "",  "N"},

        // mixed declension (with indefinite article)
        {"R", "",  "S", "N"},
        {"N", "N", "N", "N"},
        {"N", "N", "N", "N"},
        {"N", "",  "S", "N"},
};

const char *const NORMALIZER_DE_NOMITIVE_DEFINITE_ARTICLES[NORMALIZER_DE_NUM_NOMITIVE_DEFINITE_ARTICLES] = {
        "DER",
        "DIE",
        "DAS",
};

const char *const NORMALIZER_DE_NOMITIVE_INDEFINITE_ARTICLES[NORMALIZER_DE_NUM_NOMITIVE_INDEFINITE_ARTICLES] = {
        "EIN",
        "EINE",
};

const char *const NORMALIZER_DE_GENATIVE_DEFINITE_ARTICLES[NORMALIZER_DE_NUM_GENATIVE_DEFINITE_ARTICLES] = {
        "DES",
        "DER",
};

const char *const NORMALIZER_DE_DATIVE_DEFINITE_ARTICLES[NORMALIZER_DE_NUM_DATIVE_DEFINITE_ARTICLES] = {
        "DEM",
        // "DER",
        "AM",
        "IM",
        "UM",
        "ZUM",
};

const char *const NORMALIZER_DE_DATIVE_INDEFINITE_ARTICLES[NORMALIZER_DE_NUM_DATIVE_INDEFINITE_ARTICLES] = {
        "EINEM",
        "EINER",
};

const char *const NORMALIZER_DE_ACCUSATIVE_DEFINITE_ARTICLES[NORMALIZER_DE_NUM_ACCUSATIVE_DEFINITE_ARTICLES] = {
        "DEN",
        // "DIE",
        // "DAS",
};

const char *const NORMALIZER_DE_ACCUSATIVE_INDEFINITE_ARTICLES[NORMALIZER_DE_NUM_ACCUSATIVE_INDEFINITE_ARTICLES] = {
        "EINEN",
        "EINE",
        "EIN",
};

const char *const DENOMINATOR_ORDINAL_KEYS_DE[PV_NORMALIZER_NUM_DENOMINATOR_ORDINALS_DE] = {
        "2", "3", "4", "5", "6", "7", "8", "9", "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", "20",
        "30", "40", "50", "60", "70", "80", "90", "100", "1000", "1000000", "1000000000", "1000000000000"};
const char *const DENOMINATOR_ORDINAL_VALUES_SINGULAR_DE[PV_NORMALIZER_NUM_DENOMINATOR_ORDINALS_DE] = {
        "HALB", "DRITTEL", "VIERTEL", "FÜNFTEL", "SECHSTEL", "SIEBTEL", "ACHTEL", "NEUNTEL", "ZEHNTEL", "ELFTEL",
        "ZWÖLFTEL", "DREIZEHNTEL", "VIERZEHNTEL", "FÜNFZEHNTEL", "SECHZEHNTEL", "SIEBZIGSTEL", "ACHTZIGSTEL",
        "NEUNZIGSTEL", "ZWANZIGSTEL", "DREISSIGSTEL", "VIERZIGSTEL", "FÜNFZIGSTEL", "SECHZIGSTEL", "SIEBZIGSTEL",
        "ACHTZIGSTEL", "NEUNZIGSTEL", "HUNDERTSTEL", "TAUSENDSTEL", "MILLIONSTEL", "MILLIARDSTEL", "BILLIONSTEL"};
const char *const DENOMINATOR_ORDINAL_VALUES_PLURAL_DE[PV_NORMALIZER_NUM_DENOMINATOR_ORDINALS_DE] = {
        "HALBE", "DRITTEL", "VIERTEL", "FÜNFTEL", "SECHSTEL", "SIEBTEL", "ACHTEL", "NEUNTEL", "ZEHNTEL", "ELFTEL",
        "ZWÖLFTEL", "DREIZEHNTEL", "VIERZEHNTEL", "FÜNFZEHNTEL", "SECHZEHNTEL", "SIEBZIGSTEL", "ACHTZIGSTEL",
        "NEUNZIGSTEL", "ZWANZIGSTEL", "DREISSIGSTEL", "VIERZIGSTEL", "FÜNFZIGSTEL", "SECHZIGSTEL", "SIEBZIGSTEL",
        "ACHTZIGSTEL", "NEUNZIGSTEL", "HUNDERTSTEL", "TAUSENDSTEL", "MILLIONSTEL", "MILLIARDSTEL", "BILLIONSTEL"};
