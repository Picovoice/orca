#include "orca/normalizer/de/pv_normalizer_data_de/pv_normalizer_abbreviation_data_de.h"

const char *const PV_NORMALIZER_ABBREVIATIONS_DE[PV_NORMALIZER_NUM_ABBREVIATIONS_DE] = {
        "Z. B.", // 0
        "D. H.", // 1
        "USW.", // 2

        "HR.", // 3
        "FR.", // 4
        "FR.", // 5
        "DR.", // 6
        "PROF.", // 7
        "PFR.", // 8

        "GMBH.", // 9
        "AG.", // 10
        "CO.", // 11
        "AG.", // 12
        "REG.", // 13
        "UNI.", // 14
        "ABT.", // 15

        "AV.", // 16
        "STR.", // 17
        "BLVD.", // 18
        "WHG.", // 19
        "GEB.", // 20

        "STD.", // 21
        "MIN.", // 22
        "SEK.", // 23

        "GEGR.", // 24
        "VIZ.", // 25
        "CA.", // 26
        "ABK.", // 27
        "INST.", // 28

        "GG.", // 29
        "N.B.", // 30
        "ABB.", // 31
        "USW.", // 32

        "S.", // 33
        "KAP.", // 34
        "BD.", // 35
        "BDE.", // 36
        "HRSG.", // 37

        "ABG.", // 38
        "SEN.", // 39
        "GOUV.", // 40
        "PRÄS.", // 41
        "GEN.", // 42

        "JUN.", // 43
        "SEN.", // 44
        "LT.", // 45

        "BSP.", // 46
        "U.A.", // 47
        "U.U.", // 48
        "Z.T.", // 49
        "I.V.", // 50
        "I.A.", // 51
        "M.E.", // 52
        "BZW.", // 53
        "GGF.", // 54
        "CA.", // 55
        "V.A.", // 56
        "DGL.", // 57
        "EVTL.", // 58
        "Z.BSP.", // 59

        "BSPW.", // 60
        "Z.B.", // 61
        "PKW", // 62
        "Z.Z.", // 63
        "Z.ZT.", // 64
        "KGL.", // 65
        "LKW", // 66
        "ETC.", // 67
        "TEL.", // 68
        "MRD.", // 69
        "PLZ", // 70
        "SOG.", // 71
        "D.H.", // 72

};

const char *const PV_NORMALIZER_ABBREVIATIONS_VERBALIZED_DE[PV_NORMALIZER_NUM_ABBREVIATIONS_DE] = {
        "ZUM BEISPIEL", // 0
        "DAS HEISST", // 1
        "UND SO WEITER", // 2

        "HERR", // 3
        "FRAU", // 4
        "FRÄULEIN", // 5
        "DOKTOR", // 6
        "PROFESSOR", // 7
        "PFARRER", // 8

        "GESELLSCHAFT MIT BESCHRÄNKTER HAFTUNG", // 9
        "AKTIENGESELLSCHAFT", // 10
        "GESELLSCHAFT", // 11
        "KORPORATION", // 12
        "REGIERUNG", // 13
        "UNIVERSITÄT", // 14
        "ABTEILUNG", // 15

        "ALLEE", // 16
        "STRASSE", // 17
        "BOULEVARD", // 18
        "WOHNUNG", // 19
        "GEBÄUDE", // 20

        "STUNDEN", // 21
        "MINUTEN", // 22
        "SEKUNDEN", // 23

        "GEGRÜNDET", // 24
        "D. H.", // 25 (for "videlicet," the abbreviation for "das heißt")
        "UNGEFÄHR", // 26
        "ABKÜRZUNG", // 27
        "INSTITUT", // 28

        "GEGEN", // 29
        "N.B.", // 30 (for "P.S.")
        "FIGUR", // 31
        "UND SO WEITER", // 32 (for "AL")

        "SEITEN", // 33
        "KAPITEL", // 34
        "BAND", // 35
        "BÄNDE", // 36
        "AUSGABE", // 37

        "VERTRETER", // 38
        "SENATOR", // 39
        "GOUVERNEUR", // 40
        "PRÄSIDENT", // 41
        "GENERAL", // 42

        "JUNIOR", // 43
        "SENIOR", // 44
        "LEUTNANT", // 45

        "BEISPIEL", // 46
        "UNTER ANDEREM", // 47
        "UNTER UMSTÄNDEN", // 48
        "ZUM TEIL", // 49
        "IN VERTRETUNG", // 50
        "IM AUFTRAG", // 51
        "MEINES ERACHTENS", // 52
        "BEZIEHUNGSWEISE", // 53
        "GEGEBENENFALLS", // 54
        "CIRCA", // 55
        "VOR ALLEM", // 56
        "DERGLEICHEN", // 57
        "EVENTUELL", // 58
        "ZUM BEISPIEL", // 59

        "BEISPIELSWEISE", // 60
        "ZUM BEISPIEL", // 61
        "PERSONENKRAFTWAGEN", // 62
        "ZUR ZEIT", // 63
        "ZUR ZEIT", // 64
        "KÖNIGLICH", // 65
        "LASTKRAFTWAGEN", //66
        "ET CETERA", // 67
        "TELEFON", // 68
        "MILLIARDEN", // 69
        "POSTLEITZAHL", // 70
        "SOGENANNT", // 71
        "DAS HEISST", // 72
};

/* Partial abbreviations that can potentially be tokenized before complete abbreviation is available, such as
 * "Z." for "Z. B." or "U." in "U.A.".
 * Used in normalizer_stream_context to determine if a token is verbalizable.
 */
const char *const PV_NORMALIZER_PARTIAL_ABBREVIATIONS_DE[PV_NORMALIZER_NUM_PARTIAL_ABBREVIATIONS_DE] = {
        "Z.",
        "D.",
        "U.",
        "I.",
        "M.",
        "V.",
};
