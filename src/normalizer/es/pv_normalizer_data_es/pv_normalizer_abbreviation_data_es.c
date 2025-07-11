#include "orca/normalizer/es/pv_normalizer_data_es/pv_normalizer_abbreviation_data_es.h"

const char *const PV_NORMALIZER_ABBREVIATIONS_ES[PV_NORMALIZER_NUM_ABBREVIATIONS_ES] = {
        "P.EJ.",
        "EJ.",
        "ETC.",

        "SR.",
        "SRA.",
        "SRTA.",
        "DR.",
        "DRA.",
        "PROF.",
        "PROFA.",
        "REV.",

        "LTDA.",
        "S.A.",
        "SRL.",
        "CORP.",
        "GOB.",
        "UNIV.",
        "DEPTO.",

        "AVE.",
        "AV.",
        "C/",
        "PJE.",
        "BVD.",
        "EDIF.",

        "HRS.",
        "MIN.",
        "SEG.",

        "EST.",
        "V.GR.",
        "APROX.",
        "ABREV.",
        "INST.",

        "VS.",
        "P.D.",
        "FIG.",
        "ET AL.",

        "PP.",
        "CAP.",
        "T.S",
        "ED.",

        "REP.",
        "SEN.",
        "PRES.",
        "GRAL.",

        "JR.",
        "SR.",
        "TTE.",

        "UD.",
        "N.",
        "ST.",
        "BLV.",
        "BLVD.",
        "A.C.",
        "D.C.",
};

const char *const PV_NORMALIZER_ABBREVIATIONS_VERBALIZED_ES[PV_NORMALIZER_NUM_ABBREVIATIONS_ES] = {
        "POR EJEMPLO",
        "EJEMPLO",
        "ETCÉTERA",

        "SEÑOR",
        "SEÑORA",
        "SEÑORITA",
        "DOCTOR",
        "DOCTORA",
        "PROFESOR",
        "PROFESORA",
        "REVEREND",

        "LIMITADA",
        "SOCIEDAD ANÓNIMA",
        "SOCIEDAD DE RESPONSABILIDAD LIMITADA",
        "CORPORACIÓN",
        "GOBIERNO",
        "UNIVERSIDAD",
        "DEPARTAMENTO",

        "AVENIDA",
        "AVENIDA",
        "CALLE",
        "PASAJE",
        "BULEVAR",
        "EDIFICIO",

        "HORAS",
        "MINUTOS",
        "SEGUNDOS",

        "ESTABLECIDO",
        "VER EJEMPLO",
        "APROXIMADO",
        "ABREVIATURA",
        "INSTITUTO",

        "VERSUS",
        "POSDATA",
        "FIGURA",
        "ET AL",

        "PÁGINAS",
        "CAPÍTULO",
        "TOMAS",
        "EDICIÓN",

        "REPRESENTANTE",
        "SENADOR",
        "PRESIDENTE",
        "GENERAL",

        "JUNIOR",
        "SENIOR",
        "TENIENTE",

        "USTED",
        "NÚMERO",
        "SANTO",
        "BULEVAR",
        "BULEVAR",
        "ANTES DE CRISTO",
        "DESPUÉS DE CRISTO",
};

/* Partial abbreviations that can potentially be tokenized before complete abbreviation is available, such as
 * "V." for "VS."
 * Used in normalizer_stream_context to determine if a token is verbalizable.
 */
const char *const PV_NORMALIZER_PARTIAL_ABBREVIATIONS_ES[PV_NORMALIZER_NUM_PARTIAL_ABBREVIATIONS_ES] = {
        "V.",
};
