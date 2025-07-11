#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test/pv_test.h"

static void test_pv_orca_words_pron_helper(
        const char *dict_uri,
        const char **words,
        const char **word_phonemes,
        int32_t num_words) {
    char *dict_path = pv_test_resource_path(dict_uri);
    pv_test_true(dict_path, "unable to create resource path for `%s`", dict_uri);
    if (!dict_path) {
        return;
    }

    FILE *f = fopen(dict_path, "r");
    pv_test_true(f, "unable to open `%s`", dict_path);
    free(dict_path);
    if (!f) {
        return;
    }

    for (int32_t i = 0; i < num_words; i++) {
        fseek(f, 0, SEEK_SET);

        const char *word = words[i];
        const char *expected_phonemes = word_phonemes[i];

        bool correct_pronunciation_found = false;

        char buffer[128];
        while (fgets(buffer, PV_ARRAY_LEN(buffer), f) != NULL) {
            const char *key = strtok(buffer, "\t");
            const char *value = strtok(NULL, "\n");

            if (strcmp(key, word) == 0) {
                if (strcmp(value, expected_phonemes) == 0) {
                    correct_pronunciation_found = true;
                }
                break;
            }
        }

        pv_test_true(correct_pronunciation_found, "No correct pronunciation found for `%s` in `%s`", word, dict_uri);
    }

    fclose(f);
}

static void test_pv_orca_hp_pron_de(void) {
    const char *words[] = {
        "HP",
        "H",
        "P",
    };
    const char *word_phonemes[] = {
        "h a p e",
        "h a",
        "p e"
    };

    test_pv_orca_words_pron_helper(
        "dictionary/dictionary_de_male.txt",
        words,
        word_phonemes,
        PV_ARRAY_LEN(words));
    test_pv_orca_words_pron_helper(
        "dictionary/dictionary_de_female.txt",
        words,
        word_phonemes,
        PV_ARRAY_LEN(words));
}

static void test_pv_orca_hp_pron_en(void) {
    const char *words[] = {
        "HP",
        "H",
        "P",
    };
    const char *word_phonemes[] = {
        "EY CH P IY",
        "EY CH",
        "P IY"
    };

    test_pv_orca_words_pron_helper(
        "dictionary/dictionary_en_male.txt",
        words,
        word_phonemes,
        PV_ARRAY_LEN(words));
    test_pv_orca_words_pron_helper(
        "dictionary/dictionary_en_female.txt",
        words,
        word_phonemes,
        PV_ARRAY_LEN(words));
}

static void test_pv_orca_hp_pron_es(void) {
    const char *words[] = {
        "HP",
        "H",
        "P",
    };
    const char *word_phonemes[] = {
        "a ʧ e p e",
        "a ʧ e",
        "p e"
    };

    test_pv_orca_words_pron_helper(
        "dictionary/dictionary_es_male.txt",
        words,
        word_phonemes,
        PV_ARRAY_LEN(words));
    test_pv_orca_words_pron_helper(
        "dictionary/dictionary_es_female.txt",
        words,
        word_phonemes,
        PV_ARRAY_LEN(words));
}

static void test_pv_orca_hp_pron_fr(void) {
    const char *words[] = {
        "HP",
        "H",
        "P",
    };
    const char *word_phonemes[] = {
        "a ʃ p e",
        "a ʃ",
        "p e"
    };

    test_pv_orca_words_pron_helper(
        "dictionary/dictionary_fr_male.txt",
        words,
        word_phonemes,
        PV_ARRAY_LEN(words));
    test_pv_orca_words_pron_helper(
        "dictionary/dictionary_fr_female.txt",
        words,
        word_phonemes,
        PV_ARRAY_LEN(words));
}

static void test_pv_orca_hp_pron_it(void) {
    const char *words[] = {
        "HP",
        "H",
        "P",
    };
    const char *word_phonemes[] = {
        "a k k a p i",
        "a k k a",
        "p i"
    };

    test_pv_orca_words_pron_helper(
        "dictionary/dictionary_it_male.txt",
        words,
        word_phonemes,
        PV_ARRAY_LEN(words));
    test_pv_orca_words_pron_helper(
        "dictionary/dictionary_it_female.txt",
        words,
        word_phonemes,
        PV_ARRAY_LEN(words));
}

static void test_pv_orca_hp_pron_ja(void) {
    const char *words[] = {
        "エイチピー",
        "エイチ",
        "ピー",
    };
    const char *word_phonemes[] = {
        "e ʧ i p i",
        "e ʧ i",
        "p i"
    };

    test_pv_orca_words_pron_helper(
        "dictionary/dictionary_ja_male.txt",
        words,
        word_phonemes,
        PV_ARRAY_LEN(words));
    test_pv_orca_words_pron_helper(
        "dictionary/dictionary_ja_female.txt",
        words,
        word_phonemes,
        PV_ARRAY_LEN(words));
}

static void test_pv_orca_hp_pron_ko(void) {
    const char *words[] = {
        "에이치피",
        "에이치",
        "피",
    };
    const char *word_phonemes[] = {
        "e i ʨʰ i pʰ i",
        "e i ʨʰ i",
        "pʰ i"
    };

    test_pv_orca_words_pron_helper(
        "dictionary/dictionary_ko_female.txt",
        words,
        word_phonemes,
        PV_ARRAY_LEN(words));
}

static void test_pv_orca_hp_pron_pt(void) {
    const char *words[] = {
        "HP",
        "H",
        "P",
    };
    const char *word_phonemes[] = {
        "ɐ ɡ ɐ p e",
        "ɐ ɡ ɐ",
        "p e"
    };

    test_pv_orca_words_pron_helper(
        "dictionary/dictionary_pt_male.txt",
        words,
        word_phonemes,
        PV_ARRAY_LEN(words));
    test_pv_orca_words_pron_helper(
        "dictionary/dictionary_pt_female.txt",
        words,
        word_phonemes,
        PV_ARRAY_LEN(words));
}

static const pv_test_case_t PV_ORCA_DICTIONARY_TEST_CASES[] = {

#if !defined(__PV_TARGET_NO_FILE_SYSTEM__) && !defined(__PV_TARGET_PLATFORM_WASM__)

        {"HP pron DE", test_pv_orca_hp_pron_de},
        {"HP pron EN", test_pv_orca_hp_pron_en},
        {"HP pron ES", test_pv_orca_hp_pron_es},
        {"HP pron FR", test_pv_orca_hp_pron_fr},
        {"HP pron IT", test_pv_orca_hp_pron_it},
        {"HP pron JA", test_pv_orca_hp_pron_ja},
        {"HP pron KO", test_pv_orca_hp_pron_ko},
        {"HP pron PT", test_pv_orca_hp_pron_pt},

#endif

};

const pv_test_suite_t PV_ORCA_DICTIONARY_TEST_SUITE = {
        .name = "orca_dictionary",
        .setup = NULL,
        .teardown = NULL,
        .num_test_cases = PV_ARRAY_LEN(PV_ORCA_DICTIONARY_TEST_CASES),
        .test_cases = PV_ORCA_DICTIONARY_TEST_CASES,
};
