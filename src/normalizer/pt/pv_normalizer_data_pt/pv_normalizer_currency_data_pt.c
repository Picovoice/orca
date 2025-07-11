#include <stdbool.h>

#include "orca/normalizer/pt/pv_normalizer_data_pt/pv_normalizer_currency_data_pt.h"

const char *const PV_NORMALIZER_CURRENCIES_PT[PV_NORMALIZER_NUM_CURRENCIES_PT] = {
        "$",
        "€",
        "¥", // Note: JPY and CNY use the same unicode symbol. Currently verbalizes to YUAN
        "₪",
        "£",
        "₩",
        "₺",
        "₱",
        "₽",
        "฿",
        // "₴", // not in lexicon
        "₹",
        "¢",

        "USD",
        "EUR",
        "CAD",
        "JPY",
        "GBP",
        "AUD",
        "CHF",
        "CNY",
        "SEK",
        "NZD",
        "MXN",
        "SGD",
        "HKD",
        "NOK",
        "KRW",
        "TRY",
        "RUB",
        "INR",
        "BRL",
        "ZAR",
        "DKK",
        "PLN",
        "THB",
        "IDR",
        "MYR",
        "CZK",
        "HUF",
        "ILS",
        "ARS",
        "PHP",
        "CLP",
        "COP",
        "EGP",
        "SAR",
        "AED",
        "KWD",
        "PKR",
        "BDT",
        "NGN",
        "VES",
};

const char *const PV_NORMALIZER_CURRENCIES_VERBALIZED_SINGULAR_PT[PV_NORMALIZER_NUM_CURRENCIES_PT] = {
        "DÓLAR",
        "EURO",
        "YUAN",
        "SHEKEL",
        "LIBRA",
        "WON",
        "LIRA",
        "PESO",
        "RÚBLO",
        "BAHT",
        // "GRÍVNIA", // not in lexicon
        "RUPIA",
        "CÊNTIMO",

        "DÓLAR AMERICANO",
        "EUROS",
        "DÓLAR CANADIANO",
        "IENE JAPONÊS",
        "LIBRA ESTERLINA",
        "DÓLAR AUSTRALIANO",
        "FRANCO SUÍÇO",
        "RENMINBI",
        "COROA SUECA",
        "DÓLAR NEO-ZELANDÊS",
        "PESO MEXICANO",
        "DÓLAR DE SINGAPURA",
        "DÓLAR DE HONG KONG",
        "COROA NORUEGUESA",
        "WON SUL-COREANO",
        "LIRA TURCA",
        "RÚBLO RUSSO",
        "RUPIA INDIANA",
        "REAL BRASILEIRO",
        "RAND SUL-AFRICANO",
        "COROA DINAMARQUESA",
        "ZLOTY POLACO",
        "BAHT TAILANDÊS",
        "RUPIAH INDONÉSIA",
        "RINGGIT MALAIO",
        "COROA CHECA",
        "FLORIM HÚNGARO",
        "SHEKEL ISRAELITA",
        "PESO ARGENTINO",
        "PESO FILIPINO",
        "PESO CHILENO",
        "PESO COLOMBIANO",
        "LIBRA EGÍPCIA",
        "RIYAL SAUDITA",
        "DIRHAM DOS EAU",
        "DINAR KUWAITIANO",
        "RUPIA PAQUISTANESA",
        "TAKA DE BANGLADESH",
        "NAIRA NIGERIANA",
        "BOLÍVAR VENEZUELANO",
};

const char *const PV_NORMALIZER_CURRENCIES_VERBALIZED_PLURAL_PT[PV_NORMALIZER_NUM_CURRENCIES_PT] = {
        "DÓLARES",
        "EUROS",
        "YUAN",
        "SHEKELS",
        "LIBRAS",
        "WONS",
        "LIRAS",
        "PESOS",
        "RÚBLOS",
        "BAHTES",
        // "GRÍVNIAS", // not in lexicon
        "RUPIAS",
        "CÊNTIMOS",

        "DÓLARES AMERICANOS",
        "EUROS",
        "DÓLARES CANADIANOS",
        "IENES JAPONESES",
        "LIBRAS ESTERLINAS",
        "DÓLARES AUSTRALIANOS",
        "FRANCOS SUÍÇOS",
        "RENMINBI",
        "COROAS SUECAS",
        "DÓLARES NEO-ZELANDESES",
        "PESOS MEXICANOS",
        "DÓLARES DE SINGAPURA",
        "DÓLARES DE HONG KONG",
        "COROAS NORUEGUESAS",
        "WONS SUL-COREANOS",
        "LIRAS TURCAS",
        "RÚBLOS RUSSOS",
        "RUPIAS INDIANAS",
        "REAIS BRASILEIROS",
        "RANDS SUL-AFRICANOS",
        "COROAS DINAMARQUESAS",
        "ZLOTYS POLACOS",
        "BAHTS TAILANDÊSES",
        "RUPIAHS INDONÉSIAS",
        "RINGGITS MALAIOS",
        "COROAS CHECAS",
        "FLORINS HÚNGAROS",
        "SHEKELS ISRAELITAS",
        "PESOS ARGENTINOS",
        "PESOS FILIPINOS",
        "PESOS CHILENOS",
        "PESOS COLOMBIANOS",
        "LIBRAS EGÍPCIAS",
        "RIYAIS SAUDITAS",
        "DIRHAMS DOS EAU",
        "DINARES KUWAITIANOS",
        "RUPIAS PAQUISTANESAS",
        "TAKAS DE BANGLADESH",
        "NAIRAS NIGERIANAS",
        "BOLÍVARES VENEZUELANOS",
};

const char *const PV_NORMALIZER_SUB_CURRENCIES_VERBALIZED_SINGULAR_PT[PV_NORMALIZER_NUM_CURRENCIES_PT] = {
        "CENTAVO",
        "CÊNTIMO",
        "FEN",
        "AGORA",
        "PENNY",
        "JEON",
        "KURUS",
        "CENTAVO",
        "KOPEYKA",
        "SATANG",
        // "KOPIYKA",
        "PAISA",
        "",

        "CENTAVO",
        "CÊNTIMO",
        "CENTAVO",
        "SEN",
        "PENNY",
        "CENTAVO",
        "CENTAVO",
        "FEN",
        "ORE",
        "CENTAVO",
        "CENTAVO",
        "CENT",
        "CENT",
        "ORE",
        "JEON",
        "KURUS",
        "KOPEYKA",
        "PAISA",
        "CENTAVO",
        "CENT",
        "ORE",
        "GROSZ",
        "SATANG",
        "SEN",
        "SEN",
        "HALÉŘ",
        "FILLÉR",
        "AGORA",
        "CENTAVO",
        "CENTAVO",
        "CENTAVO",
        "CENTAVO",
        "PIASTRE",
        "HALALA",
        "FIL",
        "FIL",
        "PAISA",
        "POISHA",
        "KOBO",
        "CÉNTIMO",
};

const char *const PV_NORMALIZER_SUB_CURRENCIES_VERBALIZED_PLURAL_PT[PV_NORMALIZER_NUM_CURRENCIES_PT] = {
        "CENTAVOS",
        "CÊNTIMOS",
        "FEN",
        "AGOROT",
        "PENCE",
        "JEON",
        "KURUS",
        "CENTAVOS",
        "KOPEKS",
        "SATANG",
        // "KOPIYOK",
        "PAISAS",
        "",

        "CENTAVOS",
        "CÊNTIMOS",
        "CENTAVOS",
        "SEN",
        "PENCE",
        "CENTAVOS",
        "CENTAVOS",
        "FEN",
        "ORE",
        "CENTAVOS",
        "CENTAVOS",
        "CENTS",
        "CENTS",
        "ORE",
        "JEON",
        "KURUS",
        "KOPEKS",
        "PAISA",
        "CENTAVOS",
        "CENTS",
        "ØRE",
        "GROSZY",
        "SATANG",
        "SEN",
        "SEN",
        "HALÉŘ",
        "FILLÉR",
        "AGOROT",
        "CENTAVOS",
        "CENTAVOS",
        "CENTAVOS",
        "CENTAVOS",
        "PIASTRE",
        "HALALA",
        "FILS",
        "FILS",
        "PAISA",
        "POISHA",
        "KOBO",
        "CÉNTIMOS"
};

const bool PV_NORMALIZER_CURRENCIES_IS_FEMININE[PV_NORMALIZER_NUM_CURRENCIES_PT] = {
        false, // DÓLAR
        false, // EURO
        false, // YUAN
        false, // SHEKEL
        true, // LIBRA
        false, // WON
        true, // LIRA
        true, // PESO
        false, // RÚBLO
        false, // BAHT
        // false,  // GRÍVNIA (not in lexicon)
        true, // RUPIA
        false, // CÊNTIMO

        false, // DÓLAR AMERICANO
        false, // EUROS
        false, // DÓLAR CANADIANO
        false, // IENE JAPONÊS
        true, // LIBRA ESTERLINA
        false, // DÓLAR AUSTRALIANO
        false, // FRANCO SUÍÇO
        false, // RENMINBI
        false, // COROA SUECA
        false, // DÓLAR NEO-ZELANDÊS
        true, // PESO MEXICANO
        false, // DÓLAR DE SINGAPURA
        false, // DÓLAR DE HONG KONG
        false, // COROA NORUEGUESA
        false, // WON SUL-COREANO
        true, // LIRA TURCA
        false, // RÚBLO RUSSO
        true, // RUPIA INDIANA
        true, // REAL BRASILEIRO
        false, // RAND SUL-AFRICANO
        false, // COROA DINAMARQUESA
        false, // ZLOTY POLACO
        false, // BAHT TAILANDÊS
        true, // RUPIAH INDONÉSIA
        false, // RINGGIT MALAIO
        false, // COROA CHECA
        false, // FLORIM HÚNGARO
        false, // SHEKEL ISRAELITA
        true, // PESO ARGENTINO
        true, // PESO FILIPINO
        true, // PESO CHILENO
        true, // PESO COLOMBIANO
        true, // LIBRA EGÍPCIA
        false, // RIYAL SAUDITA
        false, // DIRHAM DOS EAU
        false, // DINAR KUWAITIANO
        true, // RUPIA PAQUISTANESA
        false, // TAKA DE BANGLADESH
        false, // NAIRA NIGERIANA
        false, // BOLÍVAR VENEZUELANO
};
