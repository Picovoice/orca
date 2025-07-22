#include <stdlib.h>
#include <string.h>

#include "core/pv_language.h"
#include "core/pv_language_json.h"
#include "mock/pv_mock.h"
#include "orca/normalizer/it/pv_normalizer_tags_it.h"
#include "orca/normalizer/it/pv_normalizer_verbalizer_it.h"
#include "orca/normalizer/pv_normalizer_tagger.h"
#include "orca/normalizer/pv_normalizer_tokenizer.h"
#include "orca/normalizer/pv_normalizer_verbalizer.h"
#include "test/pv_test.h"

#ifdef __PV_MOCKS__

#include "core/mock/pv_core_runtime_mock.h"
#include "orca/mock/pv_normalizer_mock.h"

#endif

static pv_normalizer_verbalizer_t *normalizer_verbalizer_object = NULL;
static pv_normalizer_language_t LANGUAGE = PV_NORMALIZER_LANGUAGE_IT;
static int32_t ALL_TEST_USE_CASES[] = {
        PV_NORMALIZER_USE_WORD_NORMALIZER_IT,
        PV_NORMALIZER_USE_PUNCTUATION_NORMALIZER,
        PV_NORMALIZER_USE_CUSTOM_PRONUNCIATION_NORMALIZER,
        PV_NORMALIZER_USE_SPECIAL_CHARACTER_NORMALIZER,
        PV_NORMALIZER_USE_ABBREVIATION_NORMALIZER,
        PV_NORMALIZER_USE_CARDINAL_NORMALIZER,
        PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER,
        PV_NORMALIZER_USE_NUMBER_RANGE_NORMALIZER,
        PV_NORMALIZER_USE_ORDINAL_NORMALIZER,
        PV_NORMALIZER_USE_NEGATIVE_ORDINAL_NORMALIZER,
        PV_NORMALIZER_USE_DECIMAL_NORMALIZER,
        PV_NORMALIZER_USE_ALPHANUM_SPELL_OUT_NORMALIZER,
        PV_NORMALIZER_USE_MEASUREMENT_NORMALIZER,
        PV_NORMALIZER_USE_TIME_NORMALIZER,
        PV_NORMALIZER_USE_FRACTION_NORMALIZER,
        PV_NORMALIZER_USE_URL_NORMALIZER,
        PV_NORMALIZER_USE_CURRENCY_NORMALIZER,
        PV_NORMALIZER_USE_DIGITS_SEQUENCE_NORMALIZER,
        PV_NORMALIZER_USE_DATE_NORMALIZER,
        PV_NORMALIZER_USE_NAME_NORMALIZER,
};

static pv_normalizer_tagger_t *normalizer_tagger_object = NULL;
static pv_normalizer_tokenizer_t *normalizer_tokenizer_object = NULL;
static pv_language_info_t *language_info_object = NULL;
static pv_noun_gender_dict_t *noun_gender_dict_object = NULL;

static const char LANGUAGE_INFO_PATH[] = "normalizer/ref/pv_language_info_normalizer_it.json";
static const char NOUN_GENDER_DICT_PATH[] = "orca/test_data/noun_gender_dict/noun_gender_dict_it.txt";

static void *calloc_return_null(size_t arg0, size_t arg1) {
    (void) arg0;
    (void) arg1;
    return NULL;
}

static pv_status_t test_pv_normalizer_verbalizer_setup(void) {
    char *language_info_path = pv_test_resource_path(LANGUAGE_INFO_PATH);
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

    char *noun_gender_dict_path = pv_test_resource_path(NOUN_GENDER_DICT_PATH);
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

static void test_pv_normalizer_verbalizer_verbalize_word(void) {
    int32_t use_cases[] = {PV_NORMALIZER_USE_WORD_NORMALIZER_IT};

    const char sentence[] = "La città di Roma è piena di storia e cultura La’";
    const char *sentence_verbalized[] = {
            "LA",
            NULL,
            "CITTÀ",
            NULL,
            "DI",
            NULL,
            "ROMA",
            NULL,
            "È",
            NULL,
            "PIENA",
            NULL,
            "DI",
            NULL,
            "STORIA",
            NULL,
            "E",
            NULL,
            "CULTURA",
            NULL,
            "LA'",
    };

    pv_normalizer_tagger_t *tagger = NULL;
    pv_normalizer_verbalizer_t *verbalizer = NULL;
    test_pv_normalizer_verbalizer_init_helper(PV_ARRAY_LEN(use_cases), use_cases, &tagger, &verbalizer);

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            tagger,
            verbalizer,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            true,
            sentence_verbalized);

    pv_normalizer_tagger_delete(tagger);
    pv_normalizer_verbalizer_delete(verbalizer);
}

static void test_pv_normalizer_verbalizer_verbalize_punctuation(void) {
    int32_t use_cases[] = {PV_NORMALIZER_USE_PUNCTUATION_NORMALIZER};

    const char sentence[] = "Sto prendendo vitamina B-12, C, e altre 15!";
    const char *sentence_verbalized[] = {
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            ",",
            NULL,
            ",",
            NULL,
            NULL,
            NULL,
            "!",
    };

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

static void test_pv_normalizer_verbalizer_verbalize_custom_pronunciation(void) {
    int32_t use_cases[] = {PV_NORMALIZER_USE_CUSTOM_PRONUNCIATION_NORMALIZER};

    const char sentence[] = "Una pronuncia {personalizzata|a b d ddʒ dz dʒ dʣ e f g i j k l m n o p r rr s t ts tts tʃ tʧ u v w z ŋ ŋg ŋk ɔ ɛ ɲ ʃ ʎ}";
    const char *sentence_verbalized[] = {
            NULL,
            NULL,
            "{a b d ddʒ dz dʒ dʣ e f g i j k l m n o p r rr s t ts tts tʃ tʧ u v w z ŋ ŋg ŋk ɔ ɛ ɲ ʃ ʎ}",
    };

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

static void test_pv_normalizer_verbalizer_verbalize_special_characters(void) {
    int32_t use_cases[] = {PV_NORMALIZER_USE_SPECIAL_CHARACTER_NORMALIZER};

    const char *sentence = "& % @ \n _ ( ) °C RPM";
    const char *sentence_verbalized[] = {
            "E",
            "PER CENTO",
            "CHIOCCIOLA",
            ".",
            "TRATTINO BASSO",
            ",",
            ",",
            "GRADI CELSIUS",
            "GIRI AL MINUTO"};

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

static void test_pv_normalizer_verbalizer_verbalize_abbreviation(void) {
    const char *sentence = " Sig. Ted Sig.ra Ted Sig.na Ted Sigg. Dr. Ted PROF. Ted PROF.ssa Ted";
    const char *sentence_verbalized[] = {
            NULL,
            "SIGNOR",
            NULL,
            "TED",
            NULL,
            "SIGNORA",
            NULL,
            "TED",
            NULL,
            "SIGNORINA",
            NULL,
            "TED",
            NULL,
            "SIGNOR GIOVANE",
            NULL,
            "DOTTOR",
            NULL,
            "TED",
            NULL,
            "PROFESSORE",
            NULL,
            "TED",
            NULL,
            "PROFESSORESSA",
            NULL,
            "TED",
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

static void test_pv_normalizer_verbalizer_verbalize_cardinal(void) {
    int32_t use_cases[] = {PV_NORMALIZER_USE_CARDINAL_NORMALIZER};

    const char *sentence = "Sto prendendo vitamina B-12, C, e altre 15!";
    const char *sentence_verbalized[] = {
            NULL,
            NULL,
            NULL,
            NULL,
            "DODICI",
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            "QUINDICI",
            NULL,
    };

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

static void test_pv_normalizer_verbalizer_verbalize_comma_cardinal(void) {
    const char *sentence = "1.005 11.005 111.005 -5.000.000.000 1111.000 1.1.000 1. 000 5.000.000.003";
    const char *sentence_verbalized[] = {
            "MILLECINQUE",
            "UNDICIMILACINQUE",
            "CENTOUNDICIMILACINQUE",
            "MENO CINQUE MILIARDI",
            "MILLECENTOUNDICI",
            "PUNTO",
            "ZERO ZERO ZERO",
            "UNO",
            "PUNTO",
            "UNO",
            "PUNTO",
            "ZERO ZERO ZERO",
            "UNO",
            ".",
            "ZERO ZERO ZERO",
            "CINQUE MILIARDI TRE",
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

static void test_pv_normalizer_verbalizer_verbalize_number_to_string(void) {
    const char *sentence = "300000 36 108 100 1000 1000000 108000 108000000 108000001 130 0 003 23 23000 23000000 123456789 5242 999999999999999 1200000000000009 11111111111111111 103 103000000";
    const char *sentence_verbalized[] = {
            "TRECENTOMILA",
            "TRENTASEI",
            "CENTOTTO",
            "CENTO",
            "MILLE",
            "UN MILIONE",
            "CENTOTTOMILA",
            "CENTOTTO MILIONI",
            "CENTOTTO MILIONI UNO",
            "CENTOTRENTA",
            "ZERO",
            "ZERO ZERO TRE",
            "VENTITRÉ",
            "VENTITREMILA",
            "VENTITRÉ MILIONI",
            "CENTOVENTITRÉ MILIONI QUATTROCENTOCINQUANTASEIMILASETTECENTOTTANTANOVE",
            "CINQUEMILADUECENTOQUARANTADUE",
            "NOVECENTONOVANTANOVE BILIONI NOVECENTONOVANTANOVE MILIARDI NOVECENTONOVANTANOVE MILIONI NOVECENTONOVANTANOVEMILANOVECENTONOVANTANOVE",
            "UNO DUE ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO NOVE",
            "UNO UNO UNO UNO UNO UNO UNO UNO UNO UNO UNO UNO UNO UNO UNO UNO UNO",
            "CENTOTRÉ",
            "CENTOTRÉ MILIONI",
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

static void test_pv_normalizer_verbalizer_verbalize_negative_cardinal(void) {
    int32_t use_cases[] = {PV_NORMALIZER_USE_NEGATIVE_CARDINAL_NORMALIZER_IT};

    const char sentence[] = "123 -123";
    const char *sentence_verbalized[] = {
            NULL,
            "MENO CENTOVENTITRÉ",
    };

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

static void test_pv_normalizer_verbalizer_verbalize_number_range(void) {
    int32_t use_cases[] = {
            PV_NORMALIZER_USE_NUMBER_RANGE_NORMALIZER,
            PV_NORMALIZER_USE_CARDINAL_NORMALIZER,
            PV_NORMALIZER_USE_DECIMAL_NORMALIZER,
    };

    const char sentence[] = "12-5 1-44 1,1-1,3";
    const char *sentence_verbalized[] = {
            "DODICI",
            "A",
            "CINQUE",
            "UNO",
            "A",
            "QUARANTAQUATTRO",
            "UNO",
            "VIRGOLA",
            "UNO",
            "A",
            "UNO",
            "VIRGOLA",
            "TRE",
    };

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

static void test_pv_normalizer_verbalizer_verbalize_comma_number_range(void) {
    const char *sentence = "1.000-2.000 1.000,2-1.000,5 10.000-1,3";
    const char *sentence_verbalized[] = {
            "MILLE",
            "A",
            "DUEMILA",
            "MILLE",
            "VIRGOLA",
            "DUE",
            "A",
            "MILLE",
            "VIRGOLA",
            "CINQUE",
            "DIECIMILA",
            "A",
            "UNO",
            "VIRGOLA",
            "TRE",
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

static void test_pv_normalizer_verbalizer_verbalize_decimal(void) {
    const char *sentence = "1,23 ,5 10,1% -2,2";
    const char *sentence_verbalized[] = {
            "UNO",
            "VIRGOLA",
            "DUE TRE",
            "VIRGOLA",
            "CINQUE",
            "DIECI",
            "VIRGOLA",
            "UNO",
            "PER CENTO",
            "MENO DUE",
            "VIRGOLA",
            "DUE"};

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            false,
            sentence_verbalized);
}

static void test_pv_normalizer_verbalizer_verbalize_comma_decimal(void) {
    const char *sentence = "1.000,2 -300.000,6666";
    const char *sentence_verbalized[] = {
            "MILLE",
            "VIRGOLA",
            "DUE",
            "MENO TRECENTOMILA",
            "VIRGOLA",
            "SEI SEI SEI SEI",
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
            "H", "T", "M", "L", "CINQUE",
            NULL,
            "C", "CINQUE", "B",
            NULL,
            "UNO", "DUE", "A",
            NULL,
            "Z", "SETTE", "OTTO",
            NULL,
            "QUATTRO", "ZERO", "ZERO", "B", "C"};

    pv_normalizer_tagger_t *tagger = NULL;
    pv_normalizer_verbalizer_t *verbalizer = NULL;
    test_pv_normalizer_verbalizer_init_helper(
            PV_ARRAY_LEN(ALL_TEST_USE_CASES),
            ALL_TEST_USE_CASES,
            &tagger,
            &verbalizer);
    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            tagger,
            verbalizer,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            true,
            sentence_verbalized);

    pv_normalizer_tagger_delete(tagger);
    pv_normalizer_verbalizer_delete(verbalizer);
}

static void test_pv_normalizer_verbalizer_verbalize_measurement(void) {
    const char sentence[] = "5g 1 ml 7,3L 3 ft 10,1km 5°C -1°C -1 °C 1°C 1 °C -1.000g 25ft.-lb.";
    const char *sentence_verbalized[] = {
            "CINQUE",
            "GRAMMI",
            "UN",
            "MILLILITRO",
            "SETTE",
            "VIRGOLA",
            "TRE",
            "LITRI",
            "TRE",
            "PIEDI",
            "DIECI",
            "VIRGOLA",
            "UNO",
            "CHILOMETRI",
            "CINQUE",
            "GRADI CELSIUS",
            "MENO UN",
            "GRADO CELSIUS",
            "MENO UN",
            "GRADO CELSIUS",
            "UN",
            "GRADO CELSIUS",
            "UN",
            "GRADO CELSIUS",
            "MENO MILLE",
            "GRAMMI",
            "VENTICINQUE",
            "PIEDE",
            NULL,
            "LIBBRE",
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
            "CINQUE",
            "CHILOMETRI",
            "PER",
            "METRO",
            "DIECI",
            "ONCE",
            "PER",
            "CHILOMETRO",
            "UNO",
            "VIRGOLA",
            "UNO",
            "METRI",
            "PER",
            "LITRO",
            "MENO CINQUE",
            "VIRGOLA",
            "QUATTRO UNO",
            "CHILOGRAMMI",
            "PER",
            "PIEDE",
            "TERABYTE",
            "PER",
            "HERTZ",
            "UN MILIONE",
            "METRI",
            "PER",
            "SECONDO",
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

static void test_pv_normalizer_verbalizer_verbalize_time(void) {
    const char *sentence = "0:00 00:15 7:00 09:18 18:23 9 21:89 90 25:09 03:03";
    const char *sentence_verbalized[] = {
            "ZERO",
            NULL,
            NULL,
            NULL,
            "ZERO",
            NULL,
            "E QUINDICI",
            NULL,
            "SETTE",
            NULL,
            NULL,
            NULL,
            "NOVE",
            NULL,
            "E DICIOTTO",
            NULL,
            "DICIOTTO",
            NULL,
            "E VENTITRÉ",
            NULL,
            "NOVE",
            NULL,
            "VENTUNO",
            "DUE PUNTI",
            "OTTANTANOVE",
            NULL,
            "NOVANTA",
            NULL,
            "VENTICINQUE",
            "DUE PUNTI",
            "ZERO NOVE",
            NULL,
            "TRE",
            NULL,
            "E TRE",
    };

    pv_normalizer_tagger_t *tagger = NULL;
    pv_normalizer_verbalizer_t *verbalizer = NULL;
    test_pv_normalizer_verbalizer_init_helper(
            PV_ARRAY_LEN(ALL_TEST_USE_CASES),
            ALL_TEST_USE_CASES,
            &tagger,
            &verbalizer);
    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            tagger,
            verbalizer,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            true,
            sentence_verbalized);

    pv_normalizer_tagger_delete(tagger);
    pv_normalizer_verbalizer_delete(verbalizer);
}

static void test_pv_normalizer_verbalizer_verbalize_url(void) {
    const char *sentence = "www.example.com https://hello.ca/";
    const char *sentence_verbalized[] = {
            "W",
            "W",
            "W",
            "PUNTO",
            "EXAMPLE",
            "PUNTO",
            "COM",
            "H",
            "T",
            "T",
            "P",
            "S",
            "DUE PUNTI",
            "BARRA",
            "BARRA",
            "HELLO",
            "PUNTO",
            "C",
            "A",
            "BARRA",
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

static void test_pv_normalizer_verbalizer_verbalize_currency(void) {
    const char *sentence_1 =
            "$ $5 $ €10 $1 10$ 10€ $1,25 1,25$ -$500 -500$ -1,25$ -$1,25 €1.000 -€1.000 1.000€ -1.000€ €1.000,25 1.000,25€ "
            "¥2 ₪2 £2 ₩2 ₺2 ₺1 ₱2 ₽2 ฿2 ₴2 ₹2 ¢2 $1,00 $1,01 -$1,01 -$1,00 5 EUR 1,25 £ -3 € -$1 1 USD -1 USD $ $";
    const char *sentence_verbalized_1[] = {
            "DOLLARO",
            NULL,
            "CINQUE DOLLARI",
            NULL,
            "DOLLARO",
            NULL,
            "DIECI EURO",
            NULL,
            "UN DOLLARO",
            NULL,
            "DIECI DOLLARI",
            NULL,
            "DIECI EURO",
            NULL,
            "UN DOLLARO E VENTICINQUE CENTESIMI",
            NULL,
            "UN DOLLARO E VENTICINQUE CENTESIMI",
            NULL,
            "MENO CINQUECENTO DOLLARI",
            NULL,
            "MENO CINQUECENTO DOLLARI",
            NULL,
            "MENO UN DOLLARO E VENTICINQUE CENTESIMI",
            NULL,
            "MENO UN DOLLARO E VENTICINQUE CENTESIMI",
            NULL,
            "MILLE EURO",
            NULL,
            "MENO MILLE EURO",
            NULL,
            "MILLE EURO",
            NULL,
            "MENO MILLE EURO",
            NULL,
            "MILLE EURO E VENTICINQUE CENTESIMI",
            NULL,
            "MILLE EURO E VENTICINQUE CENTESIMI",
            NULL,
            "DUE YUAN",
            NULL,
            "DUE SHEKELIM",
            NULL,
            "DUE STERLINE",
            NULL,
            "DUE WON",
            NULL,
            "DUE LIRE",
            NULL,
            "UNA LIRA",
            NULL,
            "DUE PESOS",
            NULL,
            "DUE RUBLI",
            NULL,
            "DUE BAHT",
            NULL,
            "DUE GRIVNIE",
            NULL,
            "DUE RUPIE",
            NULL,
            "DUE CENTESIMI",
            NULL,
            "UN DOLLARO",
            NULL,
            "UN DOLLARO E UN CENTESIMO",
            NULL,
            "MENO UN DOLLARO E UN CENTESIMO",
            NULL,
            "MENO UN DOLLARO",
            NULL,
            "CINQUE",
            NULL,
            "EURO",
            NULL,
            "UNO",
            "VIRGOLA",
            "DUE CINQUE",
            NULL,
            "STERLINE",
            NULL,
            "MENO TRE",
            NULL,
            "EURO",
            NULL,
            "MENO UN DOLLARO",
            NULL,
            "UN",
            NULL,
            "DOLLARO USA",
            NULL,
            "MENO UN",
            NULL,
            "DOLLARO USA",
            NULL,
            "DOLLARO",
            NULL,
            "DOLLARO",
    };

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence_1,
            PV_ARRAY_LEN(sentence_verbalized_1),
            true,
            sentence_verbalized_1);

    const char *sentence_2 = "$25, $25,25, $25.000, $25.000,25, -$25.000,25, 25$, 25,25$, 25.000$, 25.000,25$, -25.000,25$, -25.000,25$,";
    const char *sentence_verbalized_2[] = {
            "VENTICINQUE DOLLARI",
            ",",
            NULL,
            "VENTICINQUE DOLLARI E VENTICINQUE CENTESIMI",
            ",",
            NULL,
            "VENTICINQUEMILA DOLLARI",
            ",",
            NULL,
            "VENTICINQUEMILA DOLLARI E VENTICINQUE CENTESIMI",
            ",",
            NULL,
            "MENO VENTICINQUEMILA DOLLARI E VENTICINQUE CENTESIMI",
            ",",
            NULL,
            "VENTICINQUE DOLLARI",
            ",",
            NULL,
            "VENTICINQUE DOLLARI E VENTICINQUE CENTESIMI",
            ",",
            NULL,
            "VENTICINQUEMILA DOLLARI",
            ",",
            NULL,
            "VENTICINQUEMILA DOLLARI E VENTICINQUE CENTESIMI",
            ",",
            NULL,
            "MENO VENTICINQUEMILA DOLLARI E VENTICINQUE CENTESIMI",
            ",",
            NULL,
            "MENO VENTICINQUEMILA DOLLARI E VENTICINQUE CENTESIMI",
            ",",
    };

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence_2,
            PV_ARRAY_LEN(sentence_verbalized_2),
            true,
            sentence_verbalized_2);
}

static void
test_pv_normalizer_verbalizer_verbalize_digits_sequence(void) {
    const char *sentence = " (772) 778-1923 1-800-123-4567 123-99456-7890 +1 (381) 102-129";
    const char *sentence_verbalized[] = {
            NULL,
            ", SETTE SETTE DUE,",
            NULL,
            "SETTE SETTE OTTO",
            ",",
            "UNO NOVE DUE TRE",
            NULL,
            "UNO",
            ",",
            "OTTO ZERO ZERO",
            ",",
            "UNO DUE TRE",
            ",",
            "QUATTRO CINQUE SEI SETTE",
            NULL,
            "UNO DUE TRE",
            ",",
            "NOVE NOVE QUATTRO CINQUE SEI",
            ",",
            "SETTE OTTO NOVE ZERO",
            NULL,
            "PIÙ",
            "UNO",
            NULL,
            ", TRE OTTO UNO,",
            NULL,
            "UNO ZERO DUE",
            ",",
            "UNO DUE NOVE",
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

static void test_pv_normalizer_verbalizer_verbalize_date(void) {
    const char *sentence = "31-02-1991 01-03-1993 06/03/2020 6/3/2020 1991/03/13 1991/13/13 07/13/182 2024-05-06 1-Gennaio-2024 Gennaio 2024 2 GENNAIO 2024 31-GENNAIO-2024";
    const char *sentence_verbalized[] = {
            "TRENTUNO",
            NULL,
            "FEBBRAIO",
            NULL,
            "MILLENOVECENTONOVANTUNO",
            NULL,
            "PRIMO",
            NULL,
            "MARZO",
            NULL,
            "MILLENOVECENTONOVANTATRÉ",
            NULL,
            "SEI",
            NULL,
            "MARZO",
            NULL,
            "DUEMILAVENTI",
            NULL,
            "SEI",
            NULL,
            "MARZO",
            NULL,
            "DUEMILAVENTI",
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            "TREDICI MARZO MILLENOVECENTONOVANTUNO",
            NULL,
            "UNO NOVE NOVE UNO",
            ",",
            "UNO TRE",
            ",",
            "UNO TRE",
            NULL,
            "ZERO SETTE",
            ",",
            "UNO TRE",
            ",",
            "UNO OTTO DUE",
            NULL,
            NULL,
            NULL,
            NULL,
            NULL,
            "SEI MAGGIO DUEMILAVENTIQUATTRO",
            NULL,
            "PRIMO",
            NULL,
            "GENNAIO",
            NULL,
            "DUEMILAVENTIQUATTRO",
            NULL,
            "GENNAIO",
            NULL,
            "DUEMILAVENTIQUATTRO",
            NULL,
            "DUE",
            NULL,
            "GENNAIO",
            NULL,
            "DUEMILAVENTIQUATTRO",
            NULL,
            "TRENTUNO",
            NULL,
            "GENNAIO",
            NULL,
            "DUEMILAVENTIQUATTRO",
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
    const char *sentence = "Questo è J. R. R. Tolkien John F. Kennedy";
    const char *sentence_verbalized[] = {
            "QUESTO",
            NULL,
            "È",
            NULL,
            "J",
            NULL,
            NULL,
            "R",
            NULL,
            NULL,
            "R",
            NULL,
            NULL,
            "TOLKIEN",
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

static void test_pv_normalizer_verbalizer_verbalize_ordinal(void) {
    int32_t use_cases[] = {
            PV_NORMALIZER_USE_ORDINAL_NORMALIZER_IT,
            PV_NORMALIZER_USE_WORD_NORMALIZER_IT,
            PV_NORMALIZER_USE_CARDINAL_NORMALIZER_IT,
            PV_NORMALIZER_USE_PUNCTUATION_NORMALIZER_IT,
            PV_NORMALIZER_USE_SPECIAL_CHARACTER_NORMALIZER_IT,
    };

    const char *sentence =
            "0º 1º 2ª 100ª 1000º 1000000ª 2000000000000ª 10000000000000000000º 103º 106º 103ª 106ª 2000000000000º 10000000000000000000ª 103";
    const char *sentence_verbalized[] = {
            "ZERO",
            NULL,
            "PRIMO",
            NULL,
            "SECONDA",
            NULL,
            "CENTESIMA",
            NULL,
            "MILLESIMO",
            NULL,
            "MILIONESIMA",
            NULL,
            "DUE BILIONESIMA",
            NULL,
            "UNO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO",
            "GRADO MASCHILE",
            NULL,
            "CENTOTREESIMO",
            NULL,
            "CENTOSEIESIMO",
            NULL,
            "CENTOTREESIMA",
            NULL,
            "CENTOSEIESIMA",
            NULL,
            "DUE BILIONESIMO",
            NULL,
            "UNO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO ZERO",
            "A FEMMINILE",
            NULL,
            "CENTOTRÉ"};

    pv_normalizer_tagger_t *tagger = NULL;
    pv_normalizer_verbalizer_t *verbalizer = NULL;
    test_pv_normalizer_verbalizer_init_helper(PV_ARRAY_LEN(use_cases), use_cases, &tagger, &verbalizer);

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            tagger,
            verbalizer,
            sentence,
            PV_ARRAY_LEN(sentence_verbalized),
            true,
            sentence_verbalized);

    pv_normalizer_tagger_delete(tagger);
    pv_normalizer_verbalizer_delete(verbalizer);
}

static void test_pv_normalizer_verbalizer_verbalize_comma_ordinal(void) {
    const char *sentence = "0º 1º 2ª 100ª 1.000º 1.000.000ª 2.000.000.000.000ª 10.000.000.000.000.000.000º "
                           "10.000.000.000.000.000.000ª 5.000.000.003ª -5.000.000.003º 5.000.000.003º -5.000.000.003ª";
    const char *sentence_verbalized[] = {
            "ZERO",
            "PRIMO",
            "SECONDA",
            "CENTESIMA",
            "MILLESIMO",
            "MILIONESIMA",
            "DUE BILIONESIMA",
            NULL, // Default for super-sized comma-ordinal is DON'T PRONOUNCE ANYTHING! Though for non-comma ordinal we do verbalize right away.
            NULL,
            "CINQUE MILIARDI TRESIMA", // TODO: Need an Italian expert to determine which one is correct for large ordinals, "TRESIMA" or "TERZO"? Google translation prounciation gives "TRESIMA" and most of the time so does ChatGPT, but logically, I thought it should be "TERZO" because there's a space after "MILIARDI".
            "MENO CINQUE MILIARDI TRESIMO",
            "CINQUE MILIARDI TRESIMO",
            "MENO CINQUE MILIARDI TRESIMA",
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

static void test_pv_normalizer_verbalizer_verbalize_negative_ordinal(void) {
    int32_t use_cases[] = {PV_NORMALIZER_USE_NEGATIVE_ORDINAL_NORMALIZER_IT};

    const char *sentence = "-1º -22ª  -5000000000000º -123456789ª -0º";
    const char *sentence_verbalized[] = {
            "MENO PRIMO",
            "MENO VENTIDUESIMA",
            "MENO CINQUE BILIONESIMO",
            "MENO CENTOVENTITRÉ MILIONI QUATTROCENTOCINQUANTASEIMILASETTECENTOTTANTANOVESIMA",
            "MENO ZERO",
    };

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

static void test_pv_normalizer_verbalizer_verbalize_fraction(void) {
    const char *sentence = "1/2 9/3,2 1/1.000 1/-4 4/3 -1/2 -9/3,2 -1/1.000 -1/4 -4/3 -1/-4 1/3,2 -1/3,2 1/513 2/513 513.000/513 -513/513";
    const char *sentence_verbalized[] = {
            NULL,
            NULL,
            "UN MEZZO",
            "NOVE",
            "FRATTO",
            "TRE",
            "VIRGOLA",
            "DUE",
            NULL,
            NULL,
            "UN MILLESIMO",
            "UNO",
            "FRATTO",
            "MENO QUATTRO",
            "QUATTRO",
            NULL,
            "TERZI",

            NULL,
            NULL,
            "MENO UN MEZZO",
            "MENO NOVE",
            "FRATTO",
            "TRE",
            "VIRGOLA",
            "DUE",
            NULL,
            NULL,
            "MENO UN MILLESIMO",
            NULL,
            NULL,
            "MENO UN QUARTO",
            "MENO QUATTRO",
            NULL,
            "TERZI",
            "MENO UNO",
            "FRATTO",
            "MENO QUATTRO",
            "UNO",
            "FRATTO",
            "TRE",
            "VIRGOLA",
            "DUE",
            "MENO UNO",
            "FRATTO",
            "TRE",
            "VIRGOLA",
            "DUE",
            NULL,
            NULL,
            "UN CINQUECENTOTREDICESIMO",
            "DUE",
            NULL,
            "CINQUECENTOTREDICESIMI",
            "CINQUECENTOTREDICIMILA",
            NULL,
            "CINQUECENTOTREDICESIMI",
            "MENO CINQUECENTOTREDICI",
            NULL,
            "CINQUECENTOTREDICESIMI",
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

static void test_pv_normalizer_verbalizer_verbalize_only_special_one(void) {
    const char *sentence_1 = "1 -1 1 -1 1 1";
    const char *sentence_verbalized_1[] = {
            "UNO",
            NULL,
            "MENO UNO",
            NULL,
            "UNO",
            NULL,
            "MENO UNO",
            NULL,
            "UNO",
            NULL,
            "UNO",
    };

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence_1,
            PV_ARRAY_LEN(sentence_verbalized_1),
            true,
            sentence_verbalized_1);

    const char *sentence_2 = "-1 1 -1 1 -1 -1";
    const char *sentence_verbalized_2[] = {
            "MENO UNO",
            NULL,
            "UNO",
            NULL,
            "MENO UNO",
            NULL,
            "UNO",
            NULL,
            "MENO UNO",
            NULL,
            "MENO UNO",
    };

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence_2,
            PV_ARRAY_LEN(sentence_verbalized_2),
            true,
            sentence_verbalized_2);
}

static void test_pv_normalizer_verbalizer_verbalize_special_one_gender(void) {
    const char *sentence_1 = "1 word -1 word 1 negozio -1 negozio 1 ufficio -1 ufficio 1 scontrino -1 scontrino "
                             "1 sbaglio -1 sbaglio 1 bottiglia -1 bottiglia 1 città -1 città 1 aranciata -1 aranciata 1 ora -1 ora";
    const char *sentence_verbalized_1[] = {
            "UN",
            NULL,
            "WORD",
            NULL,
            "MENO UN",
            NULL,
            "WORD",
            NULL,
            "UN",
            NULL,
            "NEGOZIO",
            NULL,
            "MENO UN",
            NULL,
            "NEGOZIO",
            NULL,
            "UN",
            NULL,
            "UFFICIO",
            NULL,
            "MENO UN",
            NULL,
            "UFFICIO",
            NULL,
            "UNO",
            NULL,
            "SCONTRINO",
            NULL,
            "MENO UNO",
            NULL,
            "SCONTRINO",
            NULL,
            "UNO",
            NULL,
            "SBAGLIO",
            NULL,
            "MENO UNO",
            NULL,
            "SBAGLIO",
            NULL,
            "UNA",
            NULL,
            "BOTTIGLIA",
            NULL,
            "MENO UNA",
            NULL,
            "BOTTIGLIA",
            NULL,
            "UNA",
            NULL,
            "CITTÀ",
            NULL,
            "MENO UNA",
            NULL,
            "CITTÀ",
            NULL,
            NULL,
            NULL,
            "UN'ARANCIATA",
            NULL,
            NULL,
            NULL,
            "MENO UN'ARANCIATA",
            NULL,
            NULL,
            NULL,
            "UN'ORA",
            NULL,
            NULL,
            NULL,
            "MENO UN'ORA",
    };

    test_pv_normalizer_verbalizer_verbalize_and_check_verbalized_helper(
            normalizer_tokenizer_object,
            normalizer_tagger_object,
            normalizer_verbalizer_object,
            sentence_1,
            PV_ARRAY_LEN(sentence_verbalized_1),
            true,
            sentence_verbalized_1);
}

static void test_pv_normalizer_verbalizer_verbalize_single_quote(void) {
    const char *sentence = "Ha detto, 'come stai?' '(Ciao)'";
    const char *sentence_verbalized[] = {
            "HA",
            "DETTO",
            ",",
            "'COME",
            "STAI",
            "?",
            "\"",
            "\"",
            ",",
            "CIAO",
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
            PV_NORMALIZER_LANGUAGE_IT,
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
        pv_normalizer_token_tag_it_t tag) {
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
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_verbalizer_it_init, PV_STATUS_INVALID_ARGUMENT)

    test_pv_normalizer_verbalizer_init_failure_helper(PV_STATUS_INVALID_ARGUMENT, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_it_init` failed with status `INVALID_ARGUMENT`\\.");
}

static void test_pv_normalizer_verbalizer_verbalize_word_fail_calloc(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "hello";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_IT_WORD);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_verbalize_word` failed with status `OUT_OF_MEMORY`.");
}

static void test_pv_normalizer_verbalizer_verbalize_punctuation_fail_calloc(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = ",";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_IT_PUNCTUATION);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_verbalize_punctuation` failed with status `OUT_OF_MEMORY`.");
}

static void test_pv_normalizer_verbalizer_verbalize_cardinal_fail_calloc(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "5";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_IT_CARDINAL);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_verbalize_cardinal` failed with status `OUT_OF_MEMORY`.");
}

static void test_pv_normalizer_verbalizer_verbalize_negative_cardinal_fail_calloc(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "-5";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_verbalize_negative_cardinal` failed with status `OUT_OF_MEMORY`.");
}

static void test_pv_normalizer_verbalizer_verbalize_negative_cardinal_fail_calloc_2(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "-5";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_IT_NEGATIVE_CARDINAL);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_real,
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_verbalize_negative_cardinal` failed with status `OUT_OF_MEMORY`.");
}

static void test_pv_normalizer_verbalizer_verbalize_number_range_fail_calloc_1(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "2-5";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_IT_NUMBER_RANGE_TO);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_normalizer_verbalizer_verbalize_number_range` failed with status `OUT_OF_MEMORY`.");
}

static void test_pv_normalizer_verbalizer_verbalize_ordinal_fail_calloc_1(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "2º";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_IT_ORDINAL_MASCULINE);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `number_string`.");
}

static void test_pv_normalizer_verbalizer_verbalize_negative_ordinal_fail_calloc_1(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "-2º";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_IT_NEGATIVE_ORDINAL_MASCULINE);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `number_string`.");
}

static void test_pv_normalizer_verbalizer_verbalize_currency_fail_calloc_1(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "$2";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_IT_CURRENCY);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `number_string`.");
}

static void test_pv_normalizer_verbalizer_verbalize_currency_fail_calloc_2(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "$2";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_IT_CURRENCY);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_real,
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `result`.");
}

static void test_pv_normalizer_verbalizer_verbalize_negative_currency_fail_calloc_1(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "-$2";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_IT_NEGATIVE_CURRENCY);

    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs);
    test_pv_normalizer_verbalizer_verbalize_helper(token_list, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `number_string`.");
}

static void test_pv_normalizer_verbalizer_verbalize_negative_currency_fail_calloc_2(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    char *text = "-$2";
    test_pv_normalizer_verbalizer_input_setup_helper(&token_list, text, PV_NORMALIZER_TAG_IT_NEGATIVE_CURRENCY);

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
        {"verbalize_custom_pronunciation", test_pv_normalizer_verbalizer_verbalize_custom_pronunciation},
        {"verbalize_special_characters", test_pv_normalizer_verbalizer_verbalize_special_characters},
        {"verbalize_abbreviation", test_pv_normalizer_verbalizer_verbalize_abbreviation},
        {"verbalize_cardinal", test_pv_normalizer_verbalizer_verbalize_cardinal},
        {"verbalize_comma_cardinal", test_pv_normalizer_verbalizer_verbalize_comma_cardinal},
        {"verbalize_number_to_string", test_pv_normalizer_verbalizer_verbalize_number_to_string},
        {"verbalize_negative_cardinal", test_pv_normalizer_verbalizer_verbalize_negative_cardinal},
        {"verbalize_number_range", test_pv_normalizer_verbalizer_verbalize_number_range},
        {"verbalize_comma_number_range", test_pv_normalizer_verbalizer_verbalize_comma_number_range},
        {"verbalize_decimal", test_pv_normalizer_verbalizer_verbalize_decimal},
        {"verbalize_comma_decimal", test_pv_normalizer_verbalizer_verbalize_comma_decimal},
        {"verbalize_alphanum_spell_out", test_pv_normalizer_verbalizer_verbalize_alphanum_spell_out},
        {"verbalize_measurement", test_pv_normalizer_verbalizer_verbalize_measurement},
        {"verbalize_per_measurement", test_pv_normalizer_verbalizer_verbalize_per_measurement},
        {"verbalize_time", test_pv_normalizer_verbalizer_verbalize_time},
        {"verbalize_url", test_pv_normalizer_verbalizer_verbalize_url},
        {"verbalize_currency", test_pv_normalizer_verbalizer_verbalize_currency},
        {"verbalize_digits_sequence", test_pv_normalizer_verbalizer_verbalize_digits_sequence},
        {"verbalize_date", test_pv_normalizer_verbalizer_verbalize_date},
        {"verbalize_name", test_pv_normalizer_verbalizer_verbalize_name},
        {"verbalize_ordinal", test_pv_normalizer_verbalizer_verbalize_ordinal},
        {"verbalize_comma_ordinal", test_pv_normalizer_verbalizer_verbalize_comma_ordinal},
        {"verbalize_negative_ordinal", test_pv_normalizer_verbalizer_verbalize_negative_ordinal},
        {"verbalize_fraction", test_pv_normalizer_verbalizer_verbalize_fraction},
        {"verbalize_only_special_one", test_pv_normalizer_verbalizer_verbalize_only_special_one},
        {"verbalize_special_one_gender", test_pv_normalizer_verbalizer_verbalize_special_one_gender},
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
        {"verbalize_negative_ordinal_fail_calloc_1",
         test_pv_normalizer_verbalizer_verbalize_negative_ordinal_fail_calloc_1},
        {"verbalize_currency_fail_calloc_1", test_pv_normalizer_verbalizer_verbalize_currency_fail_calloc_1},
        {"verbalize_currency_fail_calloc_2", test_pv_normalizer_verbalizer_verbalize_currency_fail_calloc_2},
        {"verbalize_negative_currency_fail_calloc_1", test_pv_normalizer_verbalizer_verbalize_negative_currency_fail_calloc_1},
        {"verbalize_negative_currency_fail_calloc_2", test_pv_normalizer_verbalizer_verbalize_negative_currency_fail_calloc_2},

#endif
};

const pv_test_suite_t PV_NORMALIZER_VERBALIZER_IT_TEST_SUITE = {
        .name = "normalizer_verbalizer_it",
        .setup = test_pv_normalizer_verbalizer_setup,
        .teardown = test_pv_normalizer_verbalizer_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_NORMALIZER_VERBALIZER_TEST_CASES),
        .test_cases = PV_NORMALIZER_VERBALIZER_TEST_CASES,
};
