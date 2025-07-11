#include "orca/normalizer/pt/pv_normalizer_data_pt/pv_normalizer_abbreviation_data_pt.h"

const char *const PV_NORMALIZER_ABBREVIATIONS_PT[PV_NORMALIZER_NUM_ABBREVIATIONS_PT] = {
        "EX.", // 0
        "ETC.", // 1

        // title and name
        "SR.", // 2
        "SRA.", // 3
        "SRAS.", // 4
        "DR.", // 5
        "DRA.", // 6
        "PROF.", // 7
        "PROFA.", // 8
        "REV.", // 9

        // company
        "LDA.", // 10
        "S.A.", // 11
        "CIA.", // 12
        "GOV.", // 13
        "UNIV.", // 14
        "DEP.", // 15
        "ADM.", // 16

        // address
        "AV.", // 17
        "R.", // 18
        "BL.", // 19
        "APT.", // 20
        "AND.", // 21
        "COND.", // 22
        "CX.", // 23
        "DIR.", // 24
        "ESQ.", // 25
        "LG.", // 26
        "LJ.", // 27
        "PÇ.", // 28
        "QD.", // 29
        "TRAV.", // 30

        // books
        "PÁG.", // 31
        "PG.", // 32
        "CAP.", // 33
        "VOL.", // 34
        "VOLS.", // 35

        "TEL.", // 36
        "REF.", // 37

        "BLVD.", // 38
        "E.G.", // 39
        "ENG.", //40
        "ENGA.", //41
};


const char *const PV_NORMALIZER_ABBREVIATIONS_VERBALIZED_PT[PV_NORMALIZER_NUM_ABBREVIATIONS_PT] = {
        "POR EXEMPLO", // 0
        "ET CETERA", // 1

        // title and name
        "SENHOR", // 2
        "SENHORA", // 3
        "SENHORAS", // 4
        "DOUTOR", // 5
        "DOUTORA", // 6
        "PROFESSOR", // 7
        "PROFESSORA", // 8
        "REVERENDO", // 9

        // company
        "LIMITADA", // 10
        "SOCIEDADE ANÔNIMA", // 11
        "COMPANHIA", // 12
        "GOVERNO", // 13
        "UNIVERSIDADE", // 14
        "DEPARTAMENTO", // 15
        "ADMINISTRAÇÃO", // 16

        // address
        "AVENIDA", // 17
        "RUA", // 18
        "BLOCO", // 19
        "APARTAMENTO", // 20
        "ANDAR", // 21
        "CONDOMÍNIO", // 22
        "CAIXA", // 23
        "DIRTA", // 24
        "ESQUERDA", // 25
        "LARGO", // 26
        "LOJA", // 27
        "PRAÇA", // 28
        "QUADRA", // 29
        "TRAVESSA", // 30

        // books
        "PÁGINA", // 31
        "PÁGINA", // 32
        "CAPÍTULO", // 33
        "VOLUME", // 34
        "VOLUMES", // 35

        "TELEFONE", // 36
        "REFERÊNCIA", // 37

        "BOULEVARD", // 38
        "POR EXEMPLO", //39
        "ENGENHEIRO", //40
        "ENGENHEIRA", //41
};

/* Partial abbreviations that can potentially be tokenized before complete abbreviation is available, such as
 * "E." for "E.G." or "I." in "I.E.".
 * Used in normalizer_stream_context to determine if a token is verbalizable.
 */
const char *const PV_NORMALIZER_PARTIAL_ABBREVIATIONS_PT[PV_NORMALIZER_NUM_PARTIAL_ABBREVIATIONS_PT] = {
        "S.",
        "E.",
};
