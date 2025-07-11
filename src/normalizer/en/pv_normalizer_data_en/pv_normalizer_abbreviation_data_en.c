#include "orca/normalizer/en/pv_normalizer_data_en/pv_normalizer_abbreviation_data_en.h"

const char *const PV_NORMALIZER_ABBREVIATIONS[PV_NORMALIZER_NUM_ABBREVIATIONS] = {
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

const char *const PV_NORMALIZER_ABBREVIATIONS_VERBALIZED[PV_NORMALIZER_NUM_ABBREVIATIONS] = {
        "FOR EXAMPLE", // 0
        "THAT IS", // 1
        "ETCETERA", // 2

        "MISTER", // 3
        "MISSUS", // 4
        "MISS", // 5
        "DOCTOR", // 6
        "PROFESSOR", // 7
        "REVEREND", // 8

        "LIMITED", // 9
        "INCORPORATED", // 10
        "COMPANY", // 11
        "CORPORATION", // 12
        "GOVERNMENT", // 13
        "UNIVERSITY", // 14
        "DEPARTMENT", // 15

        "AVENUE", // 16
        "STREET", // 17
        "ROAD", // 18
        "BOULEVARD", // 19
        "APARTMENT", // 20
        "BUILDING", // 21

        "HOURS", // 22
        "MINUTES", // 23
        "SECONDS", // 24

        "ESTABLISHED", // 25
        "VIDELICET", // 26
        "APPROXIMATELY", // 27
        "ABBREVIATION", // 28
        "INSTITUTE", // 29

        "VERSUS", // 30
        "P S", // 31
        "FIGURE", // 32
        "AL", // 33

        "PAGES", // 34
        "CHAPTER", // 35
        "VOLUME", // 36
        "VOLUMES", // 37
        "EDITION", // 38

        "REPRESENTATIVE", // 39
        "SENATOR", // 40
        "GOVERNOR", // 41
        "PRESIDENT", // 42
        "GENERAL", // 43

        "JUNIOR", // 44
        "SENIOR", // 45
        "LIEUTENANT", // 46

        "MOUNT", //47

        "INTERIOR", //48
        "EXTERIOR", //49
        "P H D", //50
        "BACHELOR OF SCIENCE", //51
        "DOCTOR OF SCIENCE", //52
};

/* Partial abbreviations that can potentially be tokenized before complete abbreviation is available, such as
 * "E." for "E.G." or "I." in "I.E.".
 * Used in normalizer_stream_context to determine if a token is verbalizable.
 */
const char *const PV_NORMALIZER_PARTIAL_ABBREVIATIONS[PV_NORMALIZER_NUM_PARTIAL_ABBREVIATIONS] = {
        "E.",
        "I.",
        "P.",
        "PH.",
        "B.",
        "D.",
};
