#include <stdlib.h>
#include <string.h>

#include "core/picovoice.h"
#include "core/pv_language.h"
#include "core/pv_language_json.h"
#include "mock/pv_mock.h"
#include "orca/normalizer/pv_normalizer_util.h"
#include "test/pv_test.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_normalizer_mock.h"

#endif

#define NUM_CHARACTERS_MACRO 27

static const int32_t NUM_CHARACTERS = NUM_CHARACTERS_MACRO;
static const char *CHARACTERS[NUM_CHARACTERS_MACRO] = {
        "A",
        "B",
        "C",
        "D",
        "E",
        "F",
        "G",
        "H",
        "I",
        "J",
        "K",
        "L",
        "M",
        "N",
        "O",
        "P",
        "Q",
        "R",
        "S",
        "T",
        "U",
        "V",
        "W",
        "X",
        "Y",
        "Z",
        "°",
};

static pv_language_info_t *language_info_object = NULL;
const char LANGUAGE_INFO_PATH[] = "normalizer/ref/pv_language_info_normalizer_de.json";

static pv_status_t test_pv_normalizer_util_setup(void) {
    char *language_info_path = pv_test_resource_path(LANGUAGE_INFO_PATH);
    pv_test_true(language_info_path != NULL, "Failed to get language_info_path");
    if (!language_info_path) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_status_t status = pv_language_info_load_json(
            language_info_path,
            &language_info_object,
            true,
            true);
    free(language_info_path);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "Failed to load language info from path: '%s'",
            LANGUAGE_INFO_PATH);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_normalizer_util_teardown(void) {
    if (language_info_object) {
        pv_language_info_delete(language_info_object);
    }
}

static void test_pv_normalizer_util_trie_node_init(void) {
    pv_normalizer_util_trie_node_t *node = NULL;
    pv_status_t status = pv_normalizer_util_trie_node_init(NUM_CHARACTERS, &node);
    pv_test_true(status == PV_STATUS_SUCCESS, "Failed to initialize trie node");
    pv_test_true(node->index == -1, "Trie node index is %d but should be -1", node->index);
    pv_normalizer_util_trie_node_delete(node);
}

static void test_pv_normalizer_util_trie_node_build_trie_helper(
        int32_t num_strings,
        const char **strings,
        pv_normalizer_util_trie_t **trie) {
    *trie = NULL;
    pv_status_t status = pv_normalizer_util_trie_create(NUM_CHARACTERS, CHARACTERS, num_strings, strings, trie);
    pv_test_true(status == PV_STATUS_SUCCESS, "Failed to initialize trie node");
}

static void test_pv_normalizer_util_trie_node_build_trie(void) {
    const char *strings[] = {"APPLE", "BANANA", "ORANGE", "APP", "APPLICATION", "°C"};
    int32_t num_strings = PV_ARRAY_LEN(strings);

    pv_normalizer_util_trie_t *trie = NULL;
    test_pv_normalizer_util_trie_node_build_trie_helper(num_strings, strings, &trie);

    pv_normalizer_util_trie_delete(trie);
}

static void test_pv_normalizer_util_trie_node_search(void) {
    const char *strings[] = {"APPLE", "BANANA", "ORANGE", "APP", "APPLICATION", "°C"};
    int32_t num_strings = PV_ARRAY_LEN(strings);

    pv_normalizer_util_trie_t *trie = NULL;
    test_pv_normalizer_util_trie_node_build_trie_helper(num_strings, strings, &trie);

    int32_t idx = 0;
    for (int32_t i = 0; i < num_strings; i++) {
        pv_normalizer_util_trie_search(trie, strings[i], &idx);
        pv_test_true(idx == i, "For `%s`, expected %d but got %d", strings[i], i, idx);
    }

    pv_normalizer_util_trie_search(trie, "ap", &idx);
    pv_test_true(idx == -1, "For `ap`, expected -1 but got %d", idx);
    pv_normalizer_util_trie_search(trie, "xyz", &idx);
    pv_test_true(idx == -1, "For `xyz`, expected -1 but got %d", idx);
    pv_normalizer_util_trie_search(trie, "°", &idx);
    pv_test_true(idx == -1, "For ``, expected -1 but got %d", idx);
    pv_normalizer_util_trie_search(trie, "", &idx);
    pv_test_true(idx == -1, "For ``, expected -1 but got %d", idx);

    pv_normalizer_util_trie_delete(trie);
}

static void test_pv_normalizer_util_trie_node_build_trie_invalid_input(void) {
    const char *strings[] = {"^"};
    int32_t num_strings = PV_ARRAY_LEN(strings);

    pv_normalizer_util_trie_t *trie = NULL;
    pv_status_t status = pv_normalizer_util_trie_create(
            NUM_CHARACTERS,
            CHARACTERS,
            num_strings,
            strings,
            &trie);
    pv_test_true(
            status == PV_STATUS_INVALID_ARGUMENT,
            "raised %s instead of %s",
            pv_status_to_string(status),
            pv_status_to_string(PV_STATUS_INVALID_ARGUMENT));

    pv_normalizer_util_trie_delete(trie);
}

static void test_pv_normalizer_util_validate_text(void) {
    char *test_sentence = "Ich {he|ɛ̃ ɛ̃}";

    char *result = NULL;
    pv_status_t status = pv_normalizer_util_validate_text(
            PV_NORMALIZER_LANGUAGE_DE,
            language_info_object,
            test_sentence,
            false,
            false,
            &result);
    pv_test_true(status == PV_STATUS_SUCCESS,
                 "Expected '%s' got '%s'",
                 pv_status_to_string(PV_STATUS_SUCCESS),
                 pv_status_to_string(status));
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    char *expected = "Ich heɛ̃ ɛ̃";
    pv_test_true(strcmp(result, expected) == 0,
                 "Failed string comparison, expected '%s' got '%s'",
                 expected,
                 result);
    free(result);

    status = pv_normalizer_util_validate_text(
            PV_NORMALIZER_LANGUAGE_DE,
            language_info_object,
            test_sentence,
            false,
            true,
            &result);
    pv_test_true(status == PV_STATUS_SUCCESS,
                 "Expected '%s' got '%s'",
                 pv_status_to_string(PV_STATUS_SUCCESS),
                 pv_status_to_string(status));
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    expected = "Ich {he|ɛ̃ ɛ̃}";
    pv_test_true(strcmp(result, expected) == 0,
                 "Failed string comparison, expected '%s' got '%s'",
                 expected,
                 result);
    free(result);
}

static void test_pv_normalizer_util_validate_text_invalid_custom_pron(void) {
    char *result = NULL;
    pv_status_t status = pv_normalizer_util_validate_text(
            PV_NORMALIZER_LANGUAGE_DE,
            language_info_object,
            "Ich {he|ŋ̃ ɛ̃}",
            false,
            true,
            &result);
    pv_test_true(status == PV_STATUS_INVALID_ARGUMENT,
                 "Expected '%s' got '%s'",
                 pv_status_to_string(PV_STATUS_INVALID_ARGUMENT),
                 pv_status_to_string(status));
    if (status == PV_STATUS_SUCCESS) {
        free(result);
    }

    status = pv_normalizer_util_validate_text(
            PV_NORMALIZER_LANGUAGE_DE,
            language_info_object,
            "Ich {he|ɛ̃ɛ̃}",
            false,
            true,
            &result);
    pv_test_true(status == PV_STATUS_INVALID_ARGUMENT,
                 "Expected '%s' got '%s'",
                 pv_status_to_string(PV_STATUS_INVALID_ARGUMENT),
                 pv_status_to_string(status));
}


static void test_pv_normalizer_util_remap_characters(void) {
    const char text[] = "hello-hello−hello –hello ‒‒ —― ‘and’ 1⁄2 ẞ";
    const char truth_text[] = "hello-hello-hello -hello -- —― 'and' 1/2 ß";
    char *remapped_text = NULL;
    pv_status_t status = pv_normalizer_util_remap_characters(text, &remapped_text);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "Expected status `%s`, got status `%s`",
            pv_status_to_string(PV_STATUS_SUCCESS),
            pv_status_to_string(status));
    if (status != PV_STATUS_SUCCESS) {
        free(remapped_text);
        return;
    }
    pv_test_true(
            strcmp(truth_text, remapped_text) == 0,
            "Text mismatch: expected remapped_text `%s`, got remapped text `%s`",
            truth_text,
            remapped_text);
    free(remapped_text);
}

static void test_pv_normalizer_util_remap_spaces(void) {
    const char text[] = "a b c d e f G H i j k L";
    const char truth_text[] = "a b c d e f G H i j k L";
    char *remapped_text = NULL;
    pv_status_t status = pv_normalizer_util_remap_space(text, &remapped_text);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "Exepcted status `%s`, got status `%s`",
            pv_status_to_string(PV_STATUS_SUCCESS),
            pv_status_to_string(status));
    if (status != PV_STATUS_SUCCESS) {
        free(remapped_text);
        return;
    }
    pv_test_true(
            strcmp(truth_text, remapped_text) == 0,
            "Text mismatch: expected remapped_text `%s`, got remapped text `%s`",
            truth_text,
            remapped_text);
    free(remapped_text);
}


#ifdef __PV_MOCKS__

static void test_pv_normalizer_util_failure_helper(
        pv_status_t expected_status,
        const char *expected_public_message_regex,
        const char *expected_private_message_regex) {
    pv_status_t status = pv_normalizer_util_validate_text(
            PV_NORMALIZER_LANGUAGE_DE,
            language_info_object,
            "Ich {he|ɛ̃}",
            false,
            true,
            NULL);
    reset_mocks();
    pv_test_true(status == expected_status,
                 "Expected '%s' got '%s'",
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
                true,
                "error message mismatch, expected '%s'",
                expected_message);
    }
}

static void test_pv_normalizer_util_calloc_failure(void) {
    PV_SET_MOCK_RETURN_VAL(calloc, NULL)

    test_pv_normalizer_util_failure_helper(
            PV_STATUS_OUT_OF_MEMORY,
            "Failed to allocate, out of memory\\.",
            "Failed to allocate memory for `cleaned_text_internal`\\.");
}

static void test_pv_normalizer_util_pv_language_num_bytes_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_language_num_bytes_character, PV_STATUS_INVALID_ARGUMENT)

    test_pv_normalizer_util_failure_helper(
            PV_STATUS_INVALID_ARGUMENT,
            "Argument `text` is invalid.",
            NULL);
}

static void test_pv_normalizer_util_ph_malloc_failure(void) {
    PV_SET_MOCK_RETURN_VAL(malloc, NULL)

    test_pv_normalizer_util_failure_helper(
            PV_STATUS_OUT_OF_MEMORY,
            "Failed to allocate, out of memory\\.",
            "Failed to allocate memory for `token`\\.");
}

static void test_pv_normalizer_util_language_info_phoneme_index_from_string_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_language_info_phoneme_index_from_string, PV_STATUS_INVALID_ARGUMENT)

    test_pv_normalizer_util_failure_helper(
            PV_STATUS_INVALID_ARGUMENT,
            "Phoneme `ɛ̃` in custom pronunciation is invalid.",
            NULL);
}

static void test_pv_normalizer_util_normalizable_character_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_util_is_normalizable_character, PV_STATUS_INVALID_ARGUMENT)

    test_pv_normalizer_util_failure_helper(
            PV_STATUS_INVALID_ARGUMENT,
            pv_test_function_hash_regex(),
            "`pv_normalizer_util_is_normalizable_character` failed with status `INVALID_ARGUMENT`\\.");
}


#endif


static const pv_test_case_t PV_NORMALIZER_UTIL_TEST_CASES[] = {
        {"trie_node_init", test_pv_normalizer_util_trie_node_init},
        {"trie_node_build_trie", test_pv_normalizer_util_trie_node_build_trie},
        {"trie_node_search", test_pv_normalizer_util_trie_node_search},
        {"trie_node_build_trie_invalid_input", test_pv_normalizer_util_trie_node_build_trie_invalid_input},
        {"validate_text", test_pv_normalizer_util_validate_text},
        {"validate_text_invalid_custom_pron", test_pv_normalizer_util_validate_text_invalid_custom_pron},
        {"remap_characters", test_pv_normalizer_util_remap_characters},
        {"remap_spaces", test_pv_normalizer_util_remap_spaces},

#ifdef __PV_MOCKS__

        {"normalizer_util_calloc_failure", test_pv_normalizer_util_calloc_failure},
        {"normalizer_util_pv_language_num_bytes_failure", test_pv_normalizer_util_pv_language_num_bytes_failure},
        {"normalizer_util_ph_malloc_failure", test_pv_normalizer_util_ph_malloc_failure},
        {"normalizer_util_language_info_phoneme_index_from_string_failure", test_pv_normalizer_util_language_info_phoneme_index_from_string_failure},
        {"normalizer_util_normalizable_character_failure", test_pv_normalizer_util_normalizable_character_failure},

#endif

};

const pv_test_suite_t PV_NORMALIZER_UTIL_TEST_SUITE = {
        .name = "normalizer_util",
        .setup = test_pv_normalizer_util_setup,
        .teardown = test_pv_normalizer_util_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_NORMALIZER_UTIL_TEST_CASES),
        .test_cases = PV_NORMALIZER_UTIL_TEST_CASES,
};
