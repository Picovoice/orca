#include "orca/normalizer/it/pv_normalizer_data_it/pv_normalizer_abbreviation_data_it.h"

const char *const PV_NORMALIZER_ABBREVIATIONS_IT[PV_NORMALIZER_NUM_ABBREVIATIONS_IT] = {
        "SIG.",
        "SIG.RA",
        "SIG.NA",
        "SIGG.",
        "DR.",
        "DOTT.",
        "DOTT.SSA",
        "PROF.",
        "PROF.SSA",
        "ECC.",
        "JR.",
        "SR.",
        "N.",
        "NR.",

        "ES.",
        "A.C.",
        "D.C.",

        "SEC.",
};

const char *const PV_NORMALIZER_ABBREVIATIONS_VERBALIZED_IT[PV_NORMALIZER_NUM_ABBREVIATIONS_IT] = {
        "SIGNOR", // Note that "Sig." by itself verbalizes to "Signore" but "Sig. Ted" verbalizes to "Signor Ted", we will go with the second more common verbalization.
        "SIGNORA",
        "SIGNORINA",
        "SIGNOR GIOVANE",
        "DOTTOR",
        "DOTTORE",
        "DOTTORESSA",
        "PROFESSORE",
        "PROFESSORESSA",
        "ECCETERA",
        "JUNIOR",
        "SENIOR",
        "NUMERO",
        "NUMERO",

        "ESEMPIO",
        "AVANTI CRISTO",
        "DOPO CRISTO",

        "SECOLO",
};
