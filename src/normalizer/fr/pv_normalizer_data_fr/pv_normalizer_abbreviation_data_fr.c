#include "orca/normalizer/fr/pv_normalizer_data_fr/pv_normalizer_abbreviation_data_fr.h"

const char *const PV_NORMALIZER_ABBREVIATIONS_FR[PV_NORMALIZER_NUM_ABBREVIATIONS_FR] = {
        "E.G.", // 0
        "I.E.", // 1
        "ETC.", // 2
        "E.A.",

        "MR.", // 3
        "M.", // 3
        "MRS.", // 4
        "MME", // 4
        "MS.", // 5
        "MLLE", // 5
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
        "ED.", // 38

        "REP.", // 39
        "SEN.", // 40
        "GOV.", // 41
        "PRES.", // 42
        "GEN.", // 43

        "JR.", // 44
        "SR.", // 45
        "LT.", // 46

        "No.",
};

const char *const PV_NORMALIZER_ABBREVIATIONS_VERBALIZED_FR[PV_NORMALIZER_NUM_ABBREVIATIONS_FR] = {
        "PAR EXEMPLE", // 0
        "C'EST-À-DIRE", // 1
        "ET CETERA", // 2
        "ENTRE AUTRES", // 2

        "MONSIEUR", // 3
        "MONSIEUR", // 3
        "MADAME", // 4
        "MADAME", // 4
        "MADEMOISELLE", // 5
        "MADEMOISELLE", // 5
        "DOCTEUR", // 6
        "PROFESSEUR", // 7
        "RÉVÉREND", // 8

        "LIMITED", // 9
        "INCORPORATED", // 10
        "COMPAGNIE", // 11
        "CORPORATION", // 12
        "GOUVERNEMENT", // 13
        "UNIVERSITÉ", // 14
        "DÉPARTEMENT", // 15

        "AVENUE", // 16
        "RUE", // 17
        "ROAD", // 18
        "BOULEVARD", // 19
        "APPARTEMENT", // 20
        "BÂTIMENT", // 21

        "HEURES", // 22
        "MINUTES", // 23

        "VIDELICET", // 26
        "APPROXIMATIVEMENT", // 27
        "ABBREVIATION", // 28
        "INSTITUT", // 29

        "CONTRE", // 30
        "P S", // 31
        "FIGURE", // 32
        "AL", // 33

        "PAGES", // 34
        "CHAPITRE", // 35
        "ÉDITION", // 38

        "REPRESENTANT", // 39
        "SÉNATEUR", // 40
        "GOUVERNEUR", // 41
        "PRÉSIDENT", // 42
        "GÉNÉRAL", // 43

        "JUNIOR", // 44
        "SENIOR", // 45
        "LIEUTENANT", // 46

        "NUMÉRO",
};

/* Partial abbreviations that can potentially be tokenized before complete abbreviation is available, such as
 * "E." for "E.G." or "I." in "I.E.".
 * Used in normalizer_stream_context to determine if a token is verbalizable.
 */
const char *const PV_NORMALIZER_PARTIAL_ABBREVIATIONS_FR[PV_NORMALIZER_NUM_PARTIAL_ABBREVIATIONS_FR] = {
        "E.",
        "I.",
        "P.",
};
