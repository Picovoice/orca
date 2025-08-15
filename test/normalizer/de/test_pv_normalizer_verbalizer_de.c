#include <stdlib.h>
#include <string.h>

#include "core/pv_language.h"
#include "core/pv_language_json.h"
#include "mock/pv_mock.h"
#include "orca/normalizer/de/pv_normalizer_tags_de.h"
#include "orca/normalizer/de/pv_normalizer_verbalizer_de.h"
#include "orca/normalizer/pv_normalizer_tagger.h"
#include "orca/normalizer/pv_normalizer_tokenizer.h"
#include "orca/normalizer/pv_normalizer_verbalizer.h"
#include "test/pv_test.h"

#ifdef __PV_MOCKS__

#include "core/mock/pv_core_runtime_mock.h"
#include "orca/mock/pv_normalizer_mock.h"

#endif

static pv_normalizer_verbalizer_t *normalizer_verbalizer_object = NULL;
static pv_normalizer_language_t LANGUAGE = PV_NORMALIZER_LANGUAGE_DE;
static int32_t ALL_TEST_USE_CASES[] = {
        PV_NORMALIZER_USE_WORD_NORMALIZER_DE,
        PV_NORMALIZER_USE_CUSTOM_PRONUNCIATION_NORMALIZER_DE,
        PV_NORMALIZER_USE_PUNCTUATION_NORMALIZER_DE,
        PV_NORMALIZER_USE_CARDINAL_NORMALIZER_DE,
        PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_DE,
        PV_NORMALIZER_USE_NUMBER_RANGE_NORMALIZER_DE,
        PV_NORMALIZER_USE_ORDINAL_NORMALIZER_DE,
        PV_NORMALIZER_USE_SPECIAL_CHARACTER_NORMALIZER_DE,
        PV_NORMALIZER_USE_DECIMAL_NORMALIZER_DE,
        PV_NORMALIZER_USE_MEASUREMENT_NORMALIZER_DE,
        PV_NORMALIZER_USE_ALPHANUM_SPELL_OUT_NORMALIZER_DE,
        PV_NORMALIZER_USE_TIME_NORMALIZER_DE,
        PV_NORMALIZER_USE_FRACTION_NORMALIZER_DE,
        PV_NORMALIZER_USE_URL_NORMALIZER_DE,
        PV_NORMALIZER_USE_CURRENCY_NORMALIZER_DE,
        PV_NORMALIZER_USE_ABBREVIATION_NORMALIZER_DE,
        PV_NORMALIZER_USE_DIGITS_SEQUENCE_NORMALIZER_DE,
        PV_NORMALIZER_USE_DATE_NORMALIZER_DE,
        PV_NORMALIZER_USE_NAME_NORMALIZER_DE,
};

static pv_normalizer_tagger_t *normalizer_tagger_object = NULL;
static pv_normalizer_tokenizer_t *normalizer_tokenizer_object = NULL;
static pv_language_info_t *language_info_object = NULL;
static pv_noun_gender_dict_t *noun_gender_dict_object = NULL;

static const char LANGUAGE_INFO_PATH[] = "normalizer/ref/pv_language_info_normalizer_de.json";
static const char NOUN_GENDER_DICT_PATH[] = "test_data/noun_gender_dict/noun_gender_dict_de.txt";

static void *calloc_return_null(size_t arg0, size_t arg1) {
    (void) arg0;
    (void) arg1;
    return NULL;
}

static pv_status_t test_pv_normalizer_verbalizer_setup(void) {
    char *language_info_path = pv_test_module_res_path(LANGUAGE_INFO_PATH);
    pv_test_true(language_info_path != NULL, "Failed to get language_info_path");
    if (!language_info_path) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_status_t status = pv_language_info_load_json(language_info_path, &language_info_object, true, true);
    free(language_info_path);
    pv_test_true(status == PV_STATUS_SUCCESS, "Failed to load language info from path: '%s'", LANGUAGE_INFO_PATH);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_normalizer_tokenizer_init(language_info_object, NULL, &normalizer_tokenizer_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    char *noun_gender_dict_path = pv_test_module_res_path(NOUN_GENDER_DICT_PATH);
    pv_test_true(noun_gender_dict_path != NULL, "Failed to get noun_gender_dict_path");
    if (!noun_gender_dict_path) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    status = pv_noun_gender_dict_init(noun_gender_dict_path, &noun_gender_dict_object);
    free(noun_gender_dict_path);
    pv_test_true(status == PV_STATUS_SUCCESS, "Failed to load noun gender dict from path: '%s'", NOUN_GENDER_DICT_PATH);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_normalizer_tagger_init(
            language_info_object,
            PV_ARRAY_LEN(ALL_TEST_USE_CASES),
            ALL_TEST_USE_CASES,
            normalizer_tokenizer_object,
            noun_gender_dict_object,
            &normalizer_tagger_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_normalizer_verbalizer_init(
            LANGUAGE,
            PV_ARRAY_LEN(ALL_TEST_USE_CASES),
            ALL_TEST_USE_CASES,
            noun_gender_dict_object,
            &normalizer_verbalizer_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_normalizer_verbalizer_teardown(void) {
    pv_normalizer_tokenizer_delete(normalizer_tokenizer_object);
    normalizer_tokenizer_object = NULL;

    pv_noun_gender_dict_delete(noun_gender_dict_object);
    noun_gender_dict_object = NULL;

    pv_normalizer_tagger_delete(normalizer_tagger_object);
    normalizer_tagger_object = NULL;

    pv_normalizer_verbalizer_delete(normalizer_verbalizer_object);
    normalizer_verbalizer_object = NULL;
}

static void test_pv_normalizer_verbalizer_init_helper(
        int32_t num_use_cases,
        int32_t *use_cases,
        pv_normalizer_tagger_t **tagger,
        pv_normalizer_verbalizer_t **verbalizer) {
    *tagger = NULL;
    *verbalizer = NULL;

    pv_status_t status = pv_normalizer_tagger_init(
            language_info_object,
            num_use_cases,
            use_cases,
            normalizer_tokenizer_object,
            noun_gender_dict_object,
            tagger);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to initialize tagger");

    status = pv_normalizer_verbalizer_init(LANGUAGE,
                                           num_use_cases,
                                           use_cases,
                                           noun_gender_dict_object,
                                           verbalizer);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to initialize verbalizer");
}

static void test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
        pv_normalizer_tokenizer_t *tokenizer,
        pv_normalizer_tagger_t *tagger,
        pv_normalizer_verbalizer_t *verbalizer,
        const char *sentence,
        int32_t target_num_tokens,
        bool preserve_word_boundary,
        const char **sentence_verbalize) {
    pv_normalizer_token_list_t *token_list = NULL;

    pv_status_t status = pv_normalizer_tokenizer_tokenize(
            tokenizer,
            sentence,
            pv_normalizer_tokenizer_default_word_boundary_character(normalizer_tokenizer_object),
            preserve_word_boundary,
            false,
            false,
            true,
            &token_list);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to tokenize sentence: `%s`", sentence);

    status = pv_normalizer_tagger_tag(tagger, token_list, 0, true);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to tag sentence: `%s`", sentence);
    pv_test_true(
            token_list->size == target_num_tokens,
            "incorrect number of tokens. got `%d`, expected `%d`",
            token_list->size,
            target_num_tokens);

    status = pv_normalizer_verbalizer_verbalize(verbalizer, token_list, 0);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to verbalize sentence: `%s`", sentence);
    pv_normalizer_token_t *current = token_list->head;
    for (int32_t i = 0; i < token_list->size; i++) {
        pv_test_true(current != NULL, "token is empty");
        if (sentence_verbalize[i] == NULL) {
            pv_test_true(
                    current->verbalized == sentence_verbalize[i],
                    "incorrect verbalized: got `%s`, expected `%s`",
                    current->verbalized,
                    sentence_verbalize[i]);
        } else {
            if (current->verbalized == NULL) {
                pv_test_true(false, "verbalized is empty but expected to be `%s`", sentence_verbalize[i]);
            } else {
                pv_test_true(
                        strcmp(current->verbalized, sentence_verbalize[i]) == 0,
                        "incorrect verbalized: got `%s`, expected `%s`, at index `%d`",
                        current->verbalized,
                        sentence_verbalize[i],
                        i);
            }
        }
        current = current->next;
    }
    pv_normalizer_token_list_delete(token_list);
}

static void test_pv_normalizer_verbalizer_verbalize_and_check_status_helper(
        pv_normalizer_tokenizer_t *tokenizer,
        pv_normalizer_tagger_t *tagger,
        pv_normalizer_verbalizer_t *verbalizer,
        const char *sentence,
        pv_status_t target_tagger_status,
        pv_status_t target_verbalizer_status) {
    pv_normalizer_token_list_t *token_list = NULL;

    pv_status_t status = pv_normalizer_tokenizer_tokenize(
            tokenizer,
            sentence,
            pv_normalizer_tokenizer_default_word_boundary_character(tokenizer),
            false,
            false,
            false,
            false,
            &token_list);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to tokenize sentence: `%s`", sentence);

    status = pv_normalizer_tagger_tag(tagger, token_list, 0, true);
    pv_test_true(
            status == target_tagger_status,
            "raised %s instead of %s",
            pv_status_to_string(status),
            pv_status_to_string(target_tagger_status));

    status = pv_normalizer_verbalizer_verbalize(verbalizer, token_list, 0);
    pv_test_true(
            status == target_verbalizer_status,
            "raised %s instead of %s",
            pv_status_to_string(status),
            pv_status_to_string(target_verbalizer_status));

    pv_normalizer_token_list_delete(token_list);
}

static void test_pv_normalizer_verbalizer_verbalize_word(void) {
    const char sentence[] = "Ich träume von Urlaub I’ch";
    const char *sentence_verbalized[] = {
            "ICH",
            "TRÄUME",
            "VON",
            "URLAUB",
            "I'CH",
    };

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            false,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_punctuation(void) {
    const char sentence[] = "Komm her, schau mal, das ist fantastisch!";
    const char *sentence_verbalized[] = {
            "KOMM",
            "HER",
            ",",
            "SCHAU",
            "MAL",
            ",",
            "DAS",
            "IST",
            "FANTASTISCH",
            "!"};

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            false,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_custom_pronunciation(void) {
    int32_t use_cases[] = {PV_NORMALIZER_USE_CUSTOM_PRONUNCIATION_NORMALIZER};

    const char sentence[] = "Eine {ein|aɪ ŋ} Aussprache";
    const char *sentence_verbalized[] = {NULL, "{aɪ ŋ}", NULL};

    pv_normalizer_tagger_t *tagger = NULL;
    pv_normalizer_verbalizer_t *verbalizer = NULL;
    test_pv_normalizer_verbalizer_init_helper(PV_ARRAY_LEN(use_cases), use_cases, &tagger, &verbalizer);

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            tagger,
            verbalizer,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            false,
            sentence_verbalized);

    pv_normalizer_tagger_delete(tagger);
    pv_normalizer_verbalizer_delete(verbalizer);
}

static void test_pv_normalizer_verbalizer_verbalize_cardinal(void) {
    const char *sentence = "0 1 2 12 36 140 902 1001 2017 32004 180920202 70 18 4000050 003 1017 1000000000 "
                           "5000000000 500030000 999999999999999 1000001 9876543210987 700700700 9009009 808080808080";
    const char *sentence_verbalized[] = {
            "NULL",
            "EINS",
            "ZWEI",
            "ZWÖLF",
            "SECHSUNDDREISSIG",
            "EINHUNDERTVIERZIG",
            "NEUNHUNDERTZWEI",
            "EINTAUSENDEINS",
            "ZWEITAUSENDSIEBZEHN",
            "ZWEIUNDDREISSIGTAUSENDVIER",
            "EINHUNDERTACHTZIG MILLIONEN NEUNHUNDERTZWANZIGTAUSEND ZWEIHUNDERTZWEI",
            "SIEBZIG",
            "ACHTZEHN",
            "VIER MILLIONEN FÜNFZIG",
            "NULL NULL DREI",
            "EINTAUSENDSIEBZEHN",
            "EINE MILLIARDE",
            "FÜNF MILLIARDEN",
            "FÜNFHUNDERT MILLIONEN DREISSIGTAUSEND",
            "NEUNHUNDERTNEUNUNDNEUNZIG BILLIONEN NEUNHUNDERTNEUNUNDNEUNZIG MILLIARDEN NEUNHUNDERTNEUNUNDNEUNZIG MILLIONEN NEUNHUNDERTNEUNUNDNEUNZIGTAUSEND NEUNHUNDERTNEUNUNDNEUNZIG",
            "EINE MILLION EINS",
            "NEUN BILLIONEN ACHTHUNDERTSECHSUNDSIEBZIG MILLIARDEN FÜNFHUNDERTDREIUNDVIERZIG MILLIONEN ZWEIHUNDERTZEHNTAUSEND NEUNHUNDERTSIEBENUNDACHTZIG",
            "SIEBENHUNDERT MILLIONEN SIEBENHUNDERTTAUSEND SIEBENHUNDERT",
            "NEUN MILLIONEN NEUNTAUSENDNEUN",
            "ACHTHUNDERTACHT MILLIARDEN ACHTZIG MILLIONEN ACHTHUNDERTACHTTAUSEND ACHTZIG",
    };

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            false,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_number_to_string(void) {
    const char *sentence = "36 130 0 003 123456789 5242 999999999999999 1200000000000009 11111111111111111";
    const char *sentence_verbalized[] = {
            "SECHSUNDDREISSIG",
            "EINHUNDERTDREISSIG",
            "NULL",
            "NULL NULL DREI",
            "EINHUNDERTDREIUNDZWANZIG MILLIONEN VIERHUNDERTSECHSUNDFÜNFZIGTAUSEND SIEBENHUNDERTNEUNUNDACHTZIG",
            "FÜNFTAUSENDZWEIHUNDERTZWEIUNDVIERZIG",
            "NEUNHUNDERTNEUNUNDNEUNZIG BILLIONEN NEUNHUNDERTNEUNUNDNEUNZIG MILLIARDEN NEUNHUNDERTNEUNUNDNEUNZIG MILLIONEN NEUNHUNDERTNEUNUNDNEUNZIGTAUSEND NEUNHUNDERTNEUNUNDNEUNZIG",
            "EINS ZWEI NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NULL NEUN",
            "EINS EINS EINS EINS EINS EINS EINS EINS EINS EINS EINS EINS EINS EINS EINS EINS EINS"};

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            false,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_negative_cardinal(void) {
    const char *sentence = "-36 -140 -0 -180920202 -70 -18 -1017 -4000050 -003";
    const char *sentence_verbalized[] = {
            "MINUS SECHSUNDDREISSIG",
            "MINUS EINHUNDERTVIERZIG",
            "MINUS NULL",
            "MINUS EINHUNDERTACHTZIG MILLIONEN NEUNHUNDERTZWANZIGTAUSEND ZWEIHUNDERTZWEI",
            "MINUS SIEBZIG",
            "MINUS ACHTZEHN",
            "MINUS EINTAUSENDSIEBZEHN",
            "MINUS VIER MILLIONEN FÜNFZIG",
            "MINUS NULL NULL DREI",
    };

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            false,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_number_range(void) {
    const char sentence[] = "12-5 1-44 1,1-1,3";
    const char *sentence_verbalized[] = {
            "ZWÖLF",
            "BIS",
            "FÜNF",
            "EINS",
            "BIS",
            "VIERUNDVIERZIG",
            "EINS",
            "KOMMA",
            "EINS",
            "BIS",
            "EINS",
            "KOMMA",
            "DREI",
    };

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            false,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_invalid_use_case(void) {
    int32_t use_cases[] = {99};
    const char sentence[] = "Hallo";

    pv_normalizer_tagger_t *tagger = NULL;
    pv_normalizer_verbalizer_t *verbalizer = NULL;
    test_pv_normalizer_verbalizer_init_helper(PV_ARRAY_LEN(use_cases), use_cases, &tagger, &verbalizer);

    test_pv_normalizer_verbalizer_verbalize_and_check_status_helper(
            normalizer_tokenizer_object,
            tagger,
            verbalizer,
            sentence,
            PV_STATUS_INVALID_ARGUMENT,
            PV_STATUS_INVALID_ARGUMENT);

    pv_normalizer_tagger_delete(tagger);
    pv_normalizer_verbalizer_delete(verbalizer);
}

static void test_pv_normalizer_verbalizer_verbalize_ordinal(void) {
    const char sentence[] = "1. Sommer 2. Aalbaum 3. Aaleidechse 10. Frühling 29. Februar AM 29. Februar 33. Aalenium 11. Aalfischer "
                            "100. Aalgabel 531. Aaltierchen 2000000000000. Aasgeier 351. Aasfliege 23. Abandonnement 12. Der Mann";
    const char *sentence_verbalized[] = {
            "ERSTER",
            NULL,
            NULL,
            "SOMMER",
            NULL,
            "ZWEITER",
            NULL,
            NULL,
            "AALBAUM",
            NULL,
            "DRITTE",
            NULL,
            NULL,
            "AALEIDECHSE",
            NULL,
            "ZEHNTES",
            NULL,
            NULL,
            "FRÜHLING",
            NULL,
            "NEUNUNDZWANZIGSTER",
            NULL,
            NULL,
            "FEBRUAR",
            NULL,
            "AM",
            NULL,
            "NEUNUNDZWANZIGSTEN",
            NULL,
            NULL,
            "FEBRUAR",
            NULL,
            "DREIUNDDREISSIGSTES",
            NULL,
            NULL,
            "AALENIUM",
            NULL,
            "ELFTER",
            NULL,
            NULL,
            "AALFISCHER",
            NULL,
            "EINHUNDERTSTE",
            NULL,
            NULL,
            "AALGABEL",
            NULL,
            "FÜNFHUNDERTEINUNDDREISSIGSTES",
            NULL,
            NULL,
            "AALTIERCHEN",
            NULL,
            "ZWEI BILLIONSTER",
            NULL,
            NULL,
            "AASGEIER",
            NULL,
            "DREIHUNDERTEINUNDFÜNFZIGSTE",
            NULL,
            NULL,
            "AASFLIEGE",
            NULL,
            "DREIUNDZWANZIGSTES",
            NULL,
            NULL,
            "ABANDONNEMENT",
            NULL,
            "ZWÖLF",
            ".",
            NULL,
            "DER",
            NULL,
            "MANN",
    };

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            true,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_negative_ordinal(void) {
    const char sentence[] = "-1. Sommer -2. Aalbaum -3. Aaleidechse -10. Frühling -29. Februar -33. Aalenium -11. Aalfischer "
                            "-100. Aalgabel -531. Aaltierchen -2000000000000. Aasgeier -351. Aasfliege -23. Abandonnement -12. Der Mann";
    const char *sentence_verbalized[] = {
            "MINUS ERSTER",
            NULL,
            NULL,
            "SOMMER",
            NULL,
            "MINUS ZWEITER",
            NULL,
            NULL,
            "AALBAUM",
            NULL,
            "MINUS DRITTE",
            NULL,
            NULL,
            "AALEIDECHSE",
            NULL,
            "MINUS ZEHNTES",
            NULL,
            NULL,
            "FRÜHLING",
            NULL,
            "MINUS NEUNUNDZWANZIGSTER",
            NULL,
            NULL,
            "FEBRUAR",
            NULL,
            "MINUS DREIUNDDREISSIGSTES",
            NULL,
            NULL,
            "AALENIUM",
            NULL,
            "MINUS ELFTER",
            NULL,
            NULL,
            "AALFISCHER",
            NULL,
            "MINUS EINHUNDERTSTE",
            NULL,
            NULL,
            "AALGABEL",
            NULL,
            "MINUS FÜNFHUNDERTEINUNDDREISSIGSTES",
            NULL,
            NULL,
            "AALTIERCHEN",
            NULL,
            "MINUS ZWEI BILLIONSTER",
            NULL,
            NULL,
            "AASGEIER",
            NULL,
            "MINUS DREIHUNDERTEINUNDFÜNFZIGSTE",
            NULL,
            NULL,
            "AASFLIEGE",
            NULL,
            "MINUS DREIUNDZWANZIGSTES",
            NULL,
            NULL,
            "ABANDONNEMENT",
            NULL,
            "MINUS ZWÖLF",
            ".",
            NULL,
            "DER",
            NULL,
            "MANN",
    };

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            true,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_special_characters(void) {
    const char *sentence = "& % @ \n _ ( ) °C RPM";
    const char *sentence_verbalized[] = {
            "UND",
            "PROZENT",
            "BEI",
            ".",
            "UNTERSTRICH",
            ",",
            ",",
            "GRAD CELSIUS",
            "UMDREHUNGEN PRO MINUTE"};

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            false,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_decimal(void) {
    const char *sentence = "1,23 ,5 10,1% -2,2";
    const char *sentence_verbalized[] = {
            "EINS",
            "KOMMA",
            "ZWEI DREI",
            "KOMMA",
            "FÜNF",
            "ZEHN",
            "KOMMA",
            "EINS",
            "PROZENT",
            "MINUS ZWEI",
            "KOMMA",
            "ZWEI"};

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            false,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_measurement(void) {
    const char sentence[] = "5g 1 ml 7,3L 3 ft 10,1km 5°C 1.000g 25ft.-lb.";
    const char *sentence_verbalized[] = {
            "FÜNF",
            "GRAMM",
            "EINS",
            "MILLILITER",
            "SIEBEN",
            "KOMMA",
            "DREI",
            "LITER",
            "DREI",
            "FUß",
            "ZEHN",
            "KOMMA",
            "EINS",
            "KILOMETER",
            "FÜNF",
            "GRAD CELSIUS",
            "EINTAUSEND",
            "GRAMM",
            "FÜNFUNDZWANZIG",
            "FUß",
            NULL,
            "PFUND",
            ".",
    };

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            false,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_per_measurement(void) {
    const char sentence[] = "5 km/m 10 oz/km 1,1m/l -5,41kg/ft tb/hz 1.000.000m/s";
    const char *sentence_verbalized[] = {
            "FÜNF",
            "KILOMETER",
            "PRO",
            "METER",
            "ZEHN",
            "UNZEN",
            "PRO",
            "KILOMETER",
            "EINS",
            "KOMMA",
            "EINS",
            "METER",
            "PRO",
            "LITER",
            "MINUS FÜNF",
            "KOMMA",
            "VIER EINS",
            "KILOGRAMM",
            "PRO",
            "FUß",
            "TERABYTE",
            "PRO",
            "HERTZ",
            "EINE MILLION",
            "METER",
            "PRO",
            "SEKUNDE",
    };

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            false,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_alphanum_spell_out(void) {
    const char *sentence = "HTML5 C5B 12A Z78 400BC";
    const char *sentence_verbalized[] = {
            "H", "T", "M", "L", "FÜNF",
            NULL,
            "C", "FÜNF", "B",
            NULL,
            "EINS", "ZWEI", "A",
            NULL,
            "Z", "SIEBEN", "ACHT",
            NULL,
            "VIERHUNDERT", "B", "C"};

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            true,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_time(void) {
    const char *sentence = "7:07 uhr 8UHR uhr 89 UHR 2:01 Uhr. 11:15 03:00 UHR 03:14UHR 9uhr";
    const char *sentence_verbalized[] = {
            "SIEBEN UHR",
            NULL,
            "SIEBEN",
            NULL,
            NULL,
            NULL,
            "ACHT UHR",
            NULL,
            NULL,
            "UHR",
            NULL,
            "NEUNUNDACHTZIG",
            NULL,
            "UHR",
            NULL,
            "ZWEI UHR",
            NULL,
            "EINS",
            NULL,
            NULL,
            ".",
            NULL,
            "ELF UHR",
            NULL,
            "FÜNFZEHN",
            NULL,
            "DREI UHR",
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            "DREI UHR",
            NULL,
            "VIERZEHN",
            NULL,
            NULL,
            "NEUN UHR",
            NULL,
    };

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            true,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_url(void) {
    const char *sentence = "www.beispiel.com https://hallo.ca/";
    const char *sentence_verbalized[] = {
            "W",
            "W",
            "W",
            "PUNKT",
            "BEISPIEL",
            "PUNKT",
            "COM",
            "H",
            "T",
            "T",
            "P",
            "S",
            "DOPPELPUNKT",
            "SCHRÄGSTRICH",
            "SCHRÄGSTRICH",
            "HALLO",
            "PUNKT",
            "C",
            "A",
            "SCHRÄGSTRICH",
    };

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            false,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_dot_cardinal(void) {
    const char *sentence = "1.005 11.005 111.005 -5.000.000.000 1111.000 1.1.000 1. 000";
    const char *sentence_verbalized[] = {
            "EINTAUSENDFÜNF",
            "ELFTAUSENDFÜNF",
            "EINHUNDERTELFTAUSEND FÜNF",
            "MINUS FÜNF MILLIARDEN",
            "EINS EINS EINS EINS",
            ",",
            "NULL NULL NULL",
            "EINS",
            ",",
            "EINS",
            ",",
            "NULL NULL NULL",
            "EINS",
            ".",
            "NULL NULL NULL",
    };

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            false,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_dot_ordinal(void) {
    const char *sentence = "1.005. Sommer 2.005. Sommer 11.005. Sommer 111.005. Sommer 5.000.000.003. Sommer "
                           "1111.100. Sommer -1.1.100. Sommer -100. Sommer 200. Sommer 1. ";

    const char *sentence_verbalized[] = {
            "EINTAUSENDFÜNFTER",
            NULL,
            NULL,
            "SOMMER",
            NULL,
            "ZWEITAUSENDFÜNFTER",
            NULL,
            NULL,
            "SOMMER",
            NULL,
            "ELFTAUSENDFÜNFTER",
            NULL,
            NULL,
            "SOMMER",
            NULL,
            "EINHUNDERTELFTAUSEND FÜNFTER",
            NULL,
            NULL,
            "SOMMER",
            NULL,
            "FÜNF MILLIARDEN DRITTER",
            NULL,
            NULL,
            "SOMMER",
            NULL,
            "EINTAUSENDEINHUNDERTELF",
            "PUNKT",
            "EINHUNDERT",
            ".",
            NULL,
            "SOMMER",
            NULL,
            "MINUS EINS",
            "PUNKT",
            "EINS",
            "PUNKT",
            "EINHUNDERT",
            ".",
            NULL,
            "SOMMER",
            NULL,
            "MINUS EINHUNDERTSTER",
            NULL,
            NULL,
            "SOMMER",
            NULL,
            "ZWEIHUNDERTSTER",
            NULL,
            NULL,
            "SOMMER",
            NULL,
            "EINS",
            ".",
            NULL,
    };

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            true,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_dot_decimal(void) {
    const char *sentence = "1.000,2 -300.000,6666";
    const char *sentence_verbalized[] = {
            "EINTAUSEND",
            "KOMMA",
            "ZWEI",
            "MINUS DREIHUNDERTTAUSEND",
            "KOMMA",
            "SECHS SECHS SECHS SECHS",
    };

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            false,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_dot_number_range(void) {
    const char *sentence = "1.000-2.000 1.000,2-1.000,5 10.000-1,3";
    const char *sentence_verbalized[] = {
            "EINTAUSEND",
            "BIS",
            "ZWEITAUSEND",
            "EINTAUSEND",
            "KOMMA",
            "ZWEI",
            "BIS",
            "EINTAUSEND",
            "KOMMA",
            "FÜNF",
            "ZEHNTAUSEND",
            "BIS",
            "EINS",
            "KOMMA",
            "DREI",
    };

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            false,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_fraction(void) {
    const char *sentence = "1/2 9/3,2 1/1.000 1/-4 -4/3";
    const char *sentence_verbalized[] = {
            "EIN",
            NULL,
            "HALB",
            "NEUN",
            "DURCH",
            "DREI",
            "KOMMA",
            "ZWEI",
            "EIN",
            NULL,
            "TAUSENDSTEL",
            "EINS",
            "DURCH",
            "MINUS VIER",
            "MINUS VIER",
            NULL,
            "DRITTEL"};

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            false,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_currency(void) {
    const char *sentence =
            "$5 €10 10$ 10€ $1,25 1,25$ -$500 -500$ -1,25$ -$1,25 €1.000 -€1.000 1.000€ -1.000€ €1.000,25 1.000,25€ "
            "¥2 ₪2 £2 ₩2 ₺2 ₱2 ₽2 ฿2 ₴2 ₹2 ¢2 $1,00 $1,01 5 EUR 1,25 £ -3 €";
    const char *sentence_verbalized[] = {
            "FÜNF DOLLAR",
            "ZEHN EURO",
            "ZEHN DOLLAR",
            "ZEHN EURO",
            "EIN DOLLAR UND FÜNFUNDZWANZIG CENT",
            "EIN DOLLAR UND FÜNFUNDZWANZIG CENT",
            "MINUS FÜNFHUNDERT DOLLAR",
            "MINUS FÜNFHUNDERT DOLLAR",
            "MINUS EIN DOLLAR UND FÜNFUNDZWANZIG CENT",
            "MINUS EIN DOLLAR UND FÜNFUNDZWANZIG CENT",
            "EINTAUSEND EURO",
            "MINUS EINTAUSEND EURO",
            "EINTAUSEND EURO",
            "MINUS EINTAUSEND EURO",
            "EINTAUSEND EURO UND FÜNFUNDZWANZIG CENT",
            "EINTAUSEND EURO UND FÜNFUNDZWANZIG CENT",
            "ZWEI YUAN",
            "ZWEI SCHEKEL",
            "ZWEI PFUND",
            "ZWEI WON",
            "ZWEI LIRA",
            "ZWEI PESO",
            "ZWEI RUBEL",
            "ZWEI BAHT",
            "ZWEI HRYWNJA",
            "ZWEI RUPIE",
            "ZWEI CENT",
            "EIN DOLLAR",
            "EIN DOLLAR UND EIN CENT",
            "FÜNF",
            "EURO",
            "EINS",
            "KOMMA",
            "ZWEI FÜNF",
            "PFUND",
            "MINUS DREI",
            "EURO",
    };

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            false,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_abbreviation(void) {
    const char *sentence = " z. B. ist Prof. Dr. Doe Av. Str. i.v. usw.";
    const char *sentence_verbalized[] = {
            NULL,
            "ZUM BEISPIEL",
            NULL,
            "IST",
            NULL,
            "PROFESSOR",
            NULL,
            "DOKTOR",
            NULL,
            "DOE",
            NULL,
            "ALLEE",
            NULL,
            "STRASSE",
            NULL,
            "IN VERTRETUNG",
            NULL,
            "UND SO WEITER",
    };

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            true,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_digits_sequence(void) {
    const char *sentence = " (772) 778-1923 1-800-123-4567 123-99456-7890 893.456.7890 +1 (381) 102-129";
    const char *sentence_verbalized[] = {
            NULL,
            ", SIEBEN SIEBEN ZWEI ,",
            NULL,
            "SIEBEN SIEBEN ACHT",
            ",",
            "EINS NEUN ZWEI DREI",
            NULL,
            "EINS",
            ",",
            "ACHT NULL NULL",
            ",",
            "EINS ZWEI DREI",
            ",",
            "VIER FÜNF SECHS SIEBEN",
            NULL,
            "EINS ZWEI DREI",
            ",",
            "NEUN NEUN VIER FÜNF SECHS",
            ",",
            "SIEBEN ACHT NEUN NULL",
            NULL,
            "ACHT NEUN DREI",
            ",",
            "VIER FÜNF SECHS",
            ",",
            "SIEBEN ACHT NEUN NULL",
            NULL,
            "PLUS",
            "EINS",
            NULL,
            ", DREI ACHT EINS ,",
            NULL,
            "EINS NULL ZWEI",
            ",",
            "EINS ZWEI NEUN"};

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            true,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_date(void) {
    const char *sentence = "03-Jun-2020 08/02/1877 2013-11-28 13/01/1992 1 Aug 32-10-2024 2 Okt 1800 09-21-1907 Jul 2024";
    const char *sentence_verbalized[] = {
            "DRITTER",
            NULL,
            "JUNI ,",
            NULL,
            "ZWEITAUSENDZWANZIG",
            NULL,
            "ACHTER",
            NULL,
            "FEBRUAR ,",
            NULL,
            "ACHTZEHNHUNDERTSIEBENUNDSIEBZIG",
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            "ACHTUNDZWANZIGSTER NOVEMBER , ZWEITAUSENDDREIZEHN",
            NULL,
            "DREIZEHNTER",
            NULL,
            "JANUAR ,",
            NULL,
            "NEUNZEHNHUNDERTZWEIUNDNEUNZIG",
            NULL,
            "ERSTER",
            NULL,
            "AUGUST",
            NULL,
            "DREI ZWEI",
            ",",
            "EINS NULL",
            ",",
            "ZWEI NULL ZWEI VIER",
            NULL,
            "ZWEITER",
            NULL,
            "OKTOBER ,",
            NULL,
            "ACHTZEHNHUNDERT",
            NULL,
            NULL,
            NULL,
            "EINUNDZWANZIGSTER SEPTEMBER ,",
            NULL,
            "NEUNZEHNHUNDERTSIEBEN",
            NULL,
            "JULI",
            NULL,
            "ZWEITAUSENDVIERUNDZWANZIG",
    };

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            true,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_name(void) {
    const char *sentence = "J. K. Rowling, John F. Kennedy";
    const char *sentence_verbalized[] = {
            "J",
            NULL,
            NULL,
            "K",
            NULL,
            NULL,
            "ROWLING",
            ",",
            NULL,
            "JOHN",
            NULL,
            "F",
            NULL,
            NULL,
            "KENNEDY",
    };

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            true,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_single_quote(void) {
    const char *sentence = "Möchten Sie das Produkt 'kaufen?' '(Hallo)'";;
    const char *sentence_verbalized[] = {
            "MÖCHTEN",
            "SIE",
            "DAS",
            "PRODUKT",
            "'KAUFEN",
            "?",
            "\"",
            "\"",
            ",",
            "HALLO",
            ",",
            "\"",
    };

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            false,
            sentence_verbalized);
}

#ifdef __PV_MOCKS__

static void test_pv_normalizer_verbalizer_init_failure_helper(
        pv_status_t expected_status,
        const char *expected_public_message_regex,
        const char *expected_private_message_regex) {
    pv_normalizer_verbalizer_t *normalizer_verbalizer = NULL;
    pv_status_t status = pv_normalizer_verbalizer_init(
            PV_NORMALIZER_LANGUAGE_DE,
            PV_ARRAY_LEN(ALL_TEST_USE_CASES),
            ALL_TEST_USE_CASES,
            noun_gender_dict_object,
            &normalizer_verbalizer);
    reset_mocks();
    pv_test_true(
            status == expected_status,
            "init internal error, expected `%s` got `%s`",
            pv_status_to_string(expected_status),
            pv_status_to_string(status));
    if (expected_status != PV_STATUS_SUCCESS) {
        const char *expected_message = expected_public_message_regex;

        #ifdef __PV_ERROR_SHOW_PRIVATE_MSGS__

        if (expected_private_message_regex) {
            expected_message = expected_private_message_regex;
        }

        #endif

        pv_test_error_message(
                expected_public_message_regex,
                expected_private_message_regex,
                false,
                "error message mismatch, expected '%s'",
                expected_message);
    }
}

static void test_pv_normalizer_verbalizer_input_setup_helper(
        pv_normalizer_token_list_t **token_list,
        char *string,
        pv_normalizer_token_tag_de_t tag) {
    *token_list = NULL;
    pv_status_t status = pv_normalizer_token_list_init(token_list);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to initialize token list");

    pv_normalizer_token_t *token = NULL;
    status = pv_normalizer_token_init(
            0,
            5,
            string,
            false,
            false,
            true,
            &token);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to initialize token");

    token->tag = tag;

    pv_normalizer_token_list_append_token(*token_list, token);
}

static void test_pv_normalizer_verbalizer_verbalize_helper(
        pv_normalizer_token_list_t *token_list,
        pv_status_t expected_status,
        const char *expected_public_message_regex,
        const char *expected_private_message_regex) {

    pv_status_t status = pv_normalizer_verbalizer_verbalize(normalizer_verbalizer_object, token_list, 0);
    reset_mocks();
    pv_test_true(
            status == expected_status,
            "verbalize error, expected '%s' got '%s'",
            pv_status_to_string(expected_status),
            pv_status_to_string(status));
    if (expected_status != PV_STATUS_SUCCESS) {
        const char *expected_message = expected_public_message_regex;

        #ifdef __PV_ERROR_SHOW_PRIVATE_MSGS__

        if (expected_private_message_regex) {
            expected_message = expected_private_message_regex;
        }

        #endif

        pv_test_error_message(
                expected_public_message_regex,
                expected_private_message_regex,
                false,
                "error message mismatch, expected '%s'",
                expected_message);
    }

    pv_normalizer_token_list_delete(token_list);
}

static void test_pv_normalizer_verbalizer_init_calloc_failure(void) {
    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);

    test_pv_normalizer_verbalizer_init_failure_helper(PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `o`\\.");
}

static void test_pv_normalizer_verbalizer_init_internal_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_verbalizer_de_init, PV_STATUS_INVALID_ARGUMENT)

    test_pv_normalizer_verbalizer_init_failure_helper(PV_STATUS_INVALID_ARGUMENT, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_de_init` failed with status `INVALID_ARGUMENT`\\.");
}

static void test_pv_normalizer_verbalizer_verbalize_word_fail_calloc(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "hello";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_DE_WORD);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_verbalize_word` failed with status `OUT_OF_MEMORY`.");
}

static void test_pv_normalizer_verbalizer_verbalize_punctuation_fail_calloc(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "?";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_DE_PUNCTUATION);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_verbalize_punctuation` failed with status `OUT_OF_MEMORY`.");
}

static void test_pv_normalizer_verbalizer_verbalize_cardinal_fail_calloc(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "5";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_DE_CARDINAL);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_verbalize_cardinal` failed with status `OUT_OF_MEMORY`.");
}

static void test_pv_normalizer_verbalizer_verbalize_negative_cardinal_fail_calloc(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "-5";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_DE_NEGATIVE_CARDINAL);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_verbalize_negative_cardinal` failed with status `OUT_OF_MEMORY`.");
}

static void test_pv_normalizer_verbalizer_verbalize_negative_cardinal_fail_calloc_2(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "-5";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_DE_NEGATIVE_CARDINAL);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_real,
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_verbalize_negative_cardinal` failed with status `OUT_OF_MEMORY`.");
}

static void test_pv_normalizer_verbalizer_verbalize_number_range_fail_calloc_1(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "1-5";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_DE_NUMBER_RANGE_TO);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_verbalize_number_range` failed with status `OUT_OF_MEMORY`.");
}

static void test_pv_normalizer_verbalizer_verbalize_ordinal_fail_calloc_1(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "1st";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_DE_ORDINAL_FEMININE);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `result`.");
}

static void test_pv_normalizer_verbalizer_verbalize_ordinal_fail_calloc_2(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "1st";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_DE_ORDINAL_MASCULINE);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_real,
            calloc_real,
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `last_ordinal_str`.");
}

static void test_pv_normalizer_verbalizer_verbalize_negative_ordinal_fail_calloc_1(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "-1st";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_DE_NEGATIVE_ORDINAL_FEMININE);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `result`.");
}

static void test_pv_normalizer_verbalizer_verbalize_negative_ordinal_fail_calloc_2(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "-1st";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_DE_NEGATIVE_ORDINAL_MASCULINE);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_real,
            calloc_real,
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `last_ordinal_str`.");
}

static void test_pv_normalizer_verbalizer_verbalize_negative_ordinal_fail_calloc_3(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "-1st";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_DE_NEGATIVE_ORDINAL_NEUTER);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_real,
            calloc_real,
            calloc_real,
            calloc_real,
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `token_verbalized`.");
}

static void test_pv_normalizer_verbalizer_verbalize_currency_fail_calloc_1(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "$1";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_DE_CURRENCY);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `number_string`.");
}

static void test_pv_normalizer_verbalizer_verbalize_currency_fail_calloc_2(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "$1";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_DE_CURRENCY);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_real,
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `result`.");
}

static void test_pv_normalizer_verbalizer_verbalize_negative_currency_fail_calloc_1(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "-$1";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_DE_NEGATIVE_CURRENCY);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `number_string`.");
}

static void test_pv_normalizer_verbalizer_verbalize_negative_currency_fail_calloc_2(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "-$1";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_DE_NEGATIVE_CURRENCY);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_real,
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `result`.");
}

#endif

static const pv_test_case_t PV_NORMALIZER_VERBALIZER_TEST_CASES[] = {
        {"verbalize_word", test_pv_normalizer_verbalizer_verbalize_word},
        {"verbalize_punctuation", test_pv_normalizer_verbalizer_verbalize_punctuation},
        {"verbalize_cardinal", test_pv_normalizer_verbalizer_verbalize_cardinal},
        {"verbalize_negative_cardinal", test_pv_normalizer_verbalizer_verbalize_negative_cardinal},
        {"verbalize_custom_pronunciation", test_pv_normalizer_verbalizer_verbalize_custom_pronunciation},
        {"verbalize_number_to_string", test_pv_normalizer_verbalizer_number_to_string},
        {"verbalize_invalid_use_case", test_pv_normalizer_verbalizer_verbalize_invalid_use_case},
        {"verbalize_number_range", test_pv_normalizer_verbalizer_verbalize_number_range},
        {"verbalize_ordinal", test_pv_normalizer_verbalizer_verbalize_ordinal},
        {"verbalize_negative_ordinal", test_pv_normalizer_verbalizer_verbalize_negative_ordinal},
        {"verbalize_special_characters", test_pv_normalizer_verbalizer_verbalize_special_characters},
        {"verbalize_decimal", test_pv_normalizer_verbalizer_verbalize_decimal},
        {"verbalize_measurement", test_pv_normalizer_verbalizer_verbalize_measurement},
        {"verbalize_per_measurement", test_pv_normalizer_verbalizer_verbalize_per_measurement},
        {"verbalize_alphanum_spell_out", test_pv_normalizer_verbalizer_verbalize_alphanum_spell_out},
        {"verbalize_time", test_pv_normalizer_verbalizer_verbalize_time},
        {"verbalize_dot_cardinal", test_pv_normalizer_verbalizer_verbalize_dot_cardinal},
        {"verbalize_dot_ordinal", test_pv_normalizer_verbalizer_verbalize_dot_ordinal},
        {"verbalize_dot_decimal", test_pv_normalizer_verbalizer_verbalize_dot_decimal},
        {"verbalize_dot_number_range", test_pv_normalizer_verbalizer_verbalize_dot_number_range},
        {"verbalize_fraction", test_pv_normalizer_verbalizer_verbalize_fraction},
        {"verbalize_url", test_pv_normalizer_verbalizer_verbalize_url},
        {"verbalize_currency", test_pv_normalizer_verbalizer_verbalize_currency},
        {"verbalize_abbreviation", test_pv_normalizer_verbalizer_verbalize_abbreviation},
        {"verbalize_digits_sequence", test_pv_normalizer_verbalizer_verbalize_digits_sequence},
        {"verbalize_date", test_pv_normalizer_verbalizer_verbalize_date},
        {"verbalize_name", test_pv_normalizer_verbalizer_verbalize_name},
        {"verbalize_single_quote", test_pv_normalizer_verbalizer_verbalize_single_quote},

#ifdef __PV_MOCKS__

        {"verbalizer_init_calloc_failure", test_pv_normalizer_verbalizer_init_calloc_failure},
        {"verbalizer_init_internal_failure", test_pv_normalizer_verbalizer_init_internal_failure},
        {"verbalize_word_fail_calloc", test_pv_normalizer_verbalizer_verbalize_word_fail_calloc},
        {"verbalize_punctuation_fail_calloc", test_pv_normalizer_verbalizer_verbalize_punctuation_fail_calloc},
        {"verbalize_cardinal_fail_calloc", test_pv_normalizer_verbalizer_verbalize_cardinal_fail_calloc},
        {"verbalize_negative_cardinal_fail_calloc",
         test_pv_normalizer_verbalizer_verbalize_negative_cardinal_fail_calloc},
        {"verbalize_negative_cardinal_fail_calloc_2",
         test_pv_normalizer_verbalizer_verbalize_negative_cardinal_fail_calloc_2},
        {"verbalize_number_range_fail_calloc_1",
         test_pv_normalizer_verbalizer_verbalize_number_range_fail_calloc_1},
        {"verbalize_ordinal_fail_calloc_1", test_pv_normalizer_verbalizer_verbalize_ordinal_fail_calloc_1},
        {"verbalize_ordinal_fail_calloc_2", test_pv_normalizer_verbalizer_verbalize_ordinal_fail_calloc_2},
        {"verbalize_negative_ordinal_fail_calloc_1",
         test_pv_normalizer_verbalizer_verbalize_negative_ordinal_fail_calloc_1},
        {"verbalize_negative_ordinal_fail_calloc_2",
         test_pv_normalizer_verbalizer_verbalize_negative_ordinal_fail_calloc_2},
        {"verbalize_negative_ordinal_fail_calloc_3",
         test_pv_normalizer_verbalizer_verbalize_negative_ordinal_fail_calloc_3},
        {"verbalize_currency_fail_calloc_1", test_pv_normalizer_verbalizer_verbalize_currency_fail_calloc_1},
        {"verbalize_currency_fail_calloc_2", test_pv_normalizer_verbalizer_verbalize_currency_fail_calloc_2},
        {"verbalize_negative_currency_fail_calloc_1",
         test_pv_normalizer_verbalizer_verbalize_negative_currency_fail_calloc_1},
        {"verbalize_negative_currency_fail_calloc_2",
         test_pv_normalizer_verbalizer_verbalize_negative_currency_fail_calloc_2},

#endif

};

const pv_test_suite_t PV_NORMALIZER_VERBALIZER_DE_TEST_SUITE = {
        .name = "normalizer_verbalizer_de",
        .setup = test_pv_normalizer_verbalizer_setup,
        .teardown = test_pv_normalizer_verbalizer_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_NORMALIZER_VERBALIZER_TEST_CASES),
        .test_cases = PV_NORMALIZER_VERBALIZER_TEST_CASES,
};
