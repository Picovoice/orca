#include <stdlib.h>
#include <string.h>

#include "orca/normalizer/pv_normalizer_token.h"
#include "test/pv_test.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_normalizer_mock.h"

#endif


static pv_normalizer_token_list_t *token_list_object = NULL;

static const char *TOKEN_STRING = "hello";
static char *TOKEN_VERBALIZED = "HELLO";
static pv_normalizer_token_t *token_object = NULL;

static const char *TOKEN_STRING_CUSTOM_PRON = "{hello|HH EH L OW}";
static pv_normalizer_token_t *token_verbalized_pronunciation_object = NULL;

static pv_status_t test_pv_normalizer_token_setup(void) {
    pv_status_t status = pv_normalizer_token_init(0, 5, TOKEN_STRING, false, false, false, &token_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_normalizer_token_init(
            0,
            18,
            TOKEN_STRING_CUSTOM_PRON,
            false,
            true,
            false,
            &token_verbalized_pronunciation_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    char *token_verbalized = calloc(strlen(TOKEN_VERBALIZED) + 1, sizeof(char));
    if (!token_verbalized) {
        return PV_STATUS_OUT_OF_MEMORY;
    }
    strcpy(token_verbalized, TOKEN_VERBALIZED);

    pv_normalizer_token_set_verbalized(token_verbalized_pronunciation_object, token_verbalized);

    status = pv_normalizer_token_list_init(&token_list_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_normalizer_token_teardown(void) {
    pv_normalizer_token_list_delete(token_list_object);
    pv_normalizer_token_delete(token_verbalized_pronunciation_object);
    pv_normalizer_token_delete(token_object);
}

static void test_pv_normalizer_token_init_custom_pronunciation(void) {
    pv_normalizer_token_t *token = NULL;

    char *string2 = "{hello|K AH S}";
    bool is_punctuation = false;
    bool has_pronunciation = true;

    pv_status_t status = pv_normalizer_token_init(
            0,
            12,
            string2,
            is_punctuation,
            has_pronunciation,
            false,
            &token);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to initialize token");

    pv_test_true(
            strcmp(token->string, "hello") == 0,
            "Failed to initialize token: string is '%s' expected 'HELLO'", token->string);
    pv_test_true(
            strcmp(token->pronunciation, "K AH S") == 0,
            "Failed to initialize token: pronunciation is '%s' expected 'K AH S'", token->pronunciation);

    pv_normalizer_token_delete(token);
    token = NULL;
}

static void test_pv_normalizer_token_invalid_custom_pronunciation(void) {
    pv_normalizer_token_t *token = NULL;
    char *strings[] = {
            "hello",
            "Not closed {custom|K AH S T AM pron",
            "No separator in {custom K AH S T AM} pron",
            "Wrong {{custom}|K AH S T AM} pron",
            "Wrong {{custom||K AH S T AM}} pron",
            "Wrong {custom|K AH S T AM}}| pron",
            "Wrong }custom|K AH S T AM{ pron",
            "Wrong {custom| K AH S T AM} pron",
            "Wrong {custom|} pron",
            "Wrong {custom|K AH S T AM } pron",
    };
    bool is_punctuation = false;
    bool has_pronunciation = true;

    for (int32_t i = 0; i < PV_ARRAY_LEN(strings); i++) {
        pv_status_t status = pv_normalizer_token_init(
                0,
                4,
                strings[i],
                is_punctuation,
                has_pronunciation,
                true,
                &token);
        pv_test_true(
                status == PV_STATUS_INVALID_ARGUMENT,
                "raised %s instead of PV_STATUS_INVALID_ARGUMENT",
                pv_status_to_string(status));

        if (token != NULL) {
            pv_normalizer_token_delete(token);
            token = NULL;
        }
    }
}

static void test_pv_normalizer_token_list_append(void) {
    char *strings[] = {
            "A",
            "B",
            "C",
    };

    pv_normalizer_token_list_reset(token_list_object);

    int32_t length_before = token_list_object->size;

    for (int32_t i = 0; i < PV_ARRAY_LEN(strings); i++) {
        pv_normalizer_token_t *token = NULL;
        pv_status_t status = pv_normalizer_token_init(
                0,
                1,
                strings[i],
                false,
                false,
                false,
                &token);
        pv_test_true(status == PV_STATUS_SUCCESS, "failed to initialize token");

        pv_normalizer_token_list_append_token(token_list_object, token);
        pv_test_true(
                strcmp(token_list_object->tail->string, strings[i]) == 0,
                "Failed to initialize token: string is '%s' expected '%s'",
                token_list_object->tail->string,
                strings[i]);
    }

    pv_test_true(
            length_before + 3 == token_list_object->size,
            "failed to update token list size: got %d, expected %d",
            token_list_object->size,
            length_before + 3);
}

static void fill_token_list(pv_normalizer_token_list_t *token_list) {
    char *strings[] = {"A", "B", "C"};

    pv_normalizer_token_list_reset(token_list);

    for (int32_t i = 0; i < PV_ARRAY_LEN(strings); i++) {
        pv_normalizer_token_t *token = NULL;
        pv_status_t status = pv_normalizer_token_init(0, 1, strings[i], false, false, false, &token);
        pv_test_true(status == PV_STATUS_SUCCESS, "failed to initialize token");

        pv_normalizer_token_list_append_token(token_list, token);
    }
}

static void test_pv_normalizer_token_list_remove_token(void) {
    fill_token_list(token_list_object);

    pv_normalizer_token_list_remove_token(token_list_object, token_list_object->head);
    pv_test_true(
            token_list_object->size == 2,
            "failed to remove token from the head of the list: got %d, expected 2",
            token_list_object->size);
    pv_test_true(
            strcmp(token_list_object->head->string, "B") == 0,
            "failed to remove token from the head of the list: got %s, expected B",
            token_list_object->head->string);

    fill_token_list(token_list_object);
    pv_normalizer_token_list_remove_token(token_list_object, token_list_object->tail);
    pv_test_true(
            token_list_object->size == 2,
            "failed to remove token from the tail of the list: got %d, expected 2",
            token_list_object->size);
    pv_test_true(
            strcmp(token_list_object->tail->string, "B") == 0,
            "failed to remove token from the tail of the list: got %s, expected B",
            token_list_object->tail->string);

    fill_token_list(token_list_object);
    pv_normalizer_token_list_remove_token(token_list_object, token_list_object->head->next);
    pv_test_true(
            token_list_object->size == 2,
            "failed to remove token from the middle of the list: got %d, expected 2",
            token_list_object->size);
    pv_test_true(
            strcmp(token_list_object->head->next->string, "C") == 0,
            "failed to remove token from the middle of the list: got %s, expected C",
            token_list_object->head->next->string);
}

static void test_pv_normalizer_token_list_to_token_array(void) {
    char *strings[] = {"A", "B", "C",};

    pv_normalizer_token_list_reset(token_list_object);

    for (int32_t i = 0; i < PV_ARRAY_LEN(strings); i++) {
        pv_normalizer_token_t *token = NULL;
        pv_status_t status = pv_normalizer_token_init(
                0,
                1,
                strings[i],
                false,
                false,
                false,
                &token);
        pv_test_true(status == PV_STATUS_SUCCESS, "failed to initialize token");

        pv_normalizer_token_list_append_token(token_list_object, token);
        pv_test_true(
                strcmp(token_list_object->tail->string, strings[i]) == 0,
                "Failed to initialize token: string is '%s' expected '%s'",
                token_list_object->tail->string,
                strings[i]);
    }

    pv_normalizer_token_t **token_array = NULL;

    pv_normalizer_token_list_to_token_array(token_list_object, &token_array);

    pv_normalizer_token_t *current = token_list_object->head;
    for (int32_t i = 0; i < PV_ARRAY_LEN(strings); i++) {
        pv_test_true(
                strcmp(token_array[i]->string, current->string) == 0,
                "Token array does not match token list: string is '%s' expected '%s'",
                token_array[i]->string,
                current->string);
        current = current->next;
    }
    free(token_array);
}


static void test_pv_normalizer_token_copy(void) {
    pv_normalizer_token_t *token_two = NULL;
    pv_status_t status = pv_normalizer_token_copy(token_verbalized_pronunciation_object, &token_two);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to initialize token");
    pv_test_true(
            strcmp(token_verbalized_pronunciation_object->string, token_two->string) == 0,
            "Failed to copy token. Got `%s` expected `%s`",
            token_two->string,
            token_verbalized_pronunciation_object->string);

    pv_normalizer_token_delete(token_two);
}

static void test_pv_normalizer_token_list_unroll_token(void) {
    char *strings[] = {"AB", "CD", "EFG"};

    pv_normalizer_token_list_reset(token_list_object);

    for (int32_t i = 0; i < PV_ARRAY_LEN(strings); i++) {
        pv_normalizer_token_t *token = NULL;
        pv_status_t status = pv_normalizer_token_init(
                0,
                (int32_t) strlen(strings[i]),
                strings[i],
                false,
                false,
                false,
                &token);
        pv_test_true(status == PV_STATUS_SUCCESS, "failed to initialize token");

        pv_normalizer_token_list_append_token(token_list_object, token);
    }

    int32_t old_size = token_list_object->size;

    pv_normalizer_token_t *token = token_list_object->head;
    pv_status_t status = pv_normalizer_token_list_unroll_token(1, token, token_list_object);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to initialize token");

    pv_test_true(
            token_list_object->size == (old_size + 1),
            "expected token list size of %d but got %d",
            old_size + 1,
            token_list_object->size);
    pv_test_true(
            strcmp(token->string, "A") == 0,
            "expected string to be `A` but got `%s`",
            token->string);
    pv_test_true(
            strcmp(token->next->string, "B") == 0,
            "expected string to be `B` but got `%s`",
            token->next->string);

    pv_normalizer_token_t *token2 = token_list_object->tail;
    pv_normalizer_token_list_unroll_token(1, token2, token_list_object);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to initialize token");

    pv_test_true(
            token_list_object->size == (old_size + 2),
            "expected token list size of %d but got %d",
            old_size + 2,
            token_list_object->size);
    pv_test_true(
            strcmp(token2->string, "E") == 0,
            "expected string to be `E` but got `%s`",
            token2->string);
    pv_test_true(
            strcmp(token2->next->string, "FG") == 0,
            "expected string to be `FG` but got `%s`",
            token2->next->string);
    pv_test_true(token_list_object->tail == token2->next, "token list tail was not updated");
}

static void test_pv_normalizer_token_list_unroll_token_invalid_index(void) {
    fill_token_list(token_list_object);
    pv_status_t status = pv_normalizer_token_list_unroll_token(1, token_list_object->head, token_list_object);
    pv_test_true(
            status == PV_STATUS_INVALID_ARGUMENT,
            "raised %s instead of %s",
            pv_status_to_string(status),
            pv_status_to_string(PV_STATUS_INVALID_ARGUMENT));
}

#ifdef __PV_MOCKS__

static void *calloc_return_null(size_t arg0, size_t arg1) {
    (void) arg0;
    (void) arg1;
    return NULL;
}

static void test_pv_normalizer_token_init_failure_helper(void) {
    pv_normalizer_token_t *token = NULL;
    pv_status_t status = pv_normalizer_token_init(
            0,
            1,
            "A",
            false,
            false,
            false,
            &token);
    pv_test_true(status == PV_STATUS_OUT_OF_MEMORY, "failed to fail with oom error");
}

static void test_pv_normalizer_token_init_with_original_string_failure_helper(void) {
    pv_normalizer_token_t *token = NULL;
    pv_status_t status = pv_normalizer_token_init_with_original_string(
            "string",
            "orig string",
            false,
            false,
            0,
            0,
            &token);
    pv_test_true(status == PV_STATUS_OUT_OF_MEMORY, "failed to fail with oom error");
}

static void test_pv_normalizer_token_init_failure_1(void) {
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_token_read_text_segment, PV_STATUS_OUT_OF_MEMORY)

    test_pv_normalizer_token_init_failure_helper();
}

static void test_pv_normalizer_token_init_failure_2(void) {
    PV_SET_MOCK_RETURN_VAL(calloc, NULL)

    test_pv_normalizer_token_init_failure_helper();
    test_pv_normalizer_token_init_with_original_string_failure_helper();
}

static void test_pv_normalizer_token_init_failure_3(void) {
    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_real,
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs)

    test_pv_normalizer_token_init_failure_helper();
}

static void test_pv_normalizer_token_copy_failure_helper(void) {
    pv_normalizer_token_t *token_two = NULL;
    pv_status_t status = pv_normalizer_token_copy(token_verbalized_pronunciation_object, &token_two);
    pv_test_true(status == PV_STATUS_OUT_OF_MEMORY, "failed to fail to copy token with oom error");
}

static void test_pv_normalizer_token_copy_failure_calloc_1(void) {
    PV_SET_MOCK_RETURN_VAL(calloc, NULL)

    test_pv_normalizer_token_copy_failure_helper();
}

static void test_pv_normalizer_token_copy_failure_calloc_2(void) {
    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_real,
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs)

    test_pv_normalizer_token_copy_failure_helper();
}

static void test_pv_normalizer_token_copy_failure_calloc_3(void) {
    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_real,
            calloc_real,
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs)

    test_pv_normalizer_token_copy_failure_helper();
}

static void test_pv_normalizer_token_copy_failure_calloc_4(void) {
    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_real,
            calloc_real,
            calloc_real,
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs)

    test_pv_normalizer_token_copy_failure_helper();
}

static void test_pv_normalizer_token_copy_failure_calloc_5(void) {
    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_real,
            calloc_real,
            calloc_real,
            calloc_real,
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs)

    test_pv_normalizer_token_copy_failure_helper();
}

static pv_status_t pv_normalizer_token_read_text_segment_return_oom(
        int32_t arg0,
        int32_t arg1,
        const char *arg2,
        char **arg3) {
    (void) arg0;
    (void) arg1;
    (void) arg2;
    (void) arg3;

    return PV_STATUS_OUT_OF_MEMORY;
}

static void test_pv_normalizer_token_read_custom_pronunciation_failure_helper(void) {
    char *segment = NULL;
    char *pronunciation = NULL;
    pv_status_t status = pv_normalizer_token_read_custom_pronunciation(
            0,
            18,
            TOKEN_STRING_CUSTOM_PRON,
            &segment,
            &pronunciation);
    pv_test_true(status == PV_STATUS_OUT_OF_MEMORY, "failed to fail with oom error");
}

static void test_pv_normalizer_token_read_custom_pronunciation_failure_1(void) {
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_token_read_text_segment, PV_STATUS_OUT_OF_MEMORY)

    test_pv_normalizer_token_read_custom_pronunciation_failure_helper();
}

static void test_pv_normalizer_token_read_custom_pronunciation_failure_2(void) {
    pv_status_t(*custom_funcs[])(int32_t
    arg0, int32_t
    arg1,
    const char *arg2,
    char **arg3) = {
        pv_normalizer_token_read_text_segment_real,
                pv_normalizer_token_read_text_segment_return_oom,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(pv_normalizer_token_read_text_segment, custom_funcs)

    test_pv_normalizer_token_read_custom_pronunciation_failure_helper();
}

static void test_pv_normalizer_token_list_init_calloc_failure(void) {
    PV_SET_MOCK_RETURN_VAL(calloc, NULL)

    pv_normalizer_token_list_t *token_list = NULL;
    pv_status_t status = pv_normalizer_token_list_init(&token_list);
    pv_test_true(status == PV_STATUS_OUT_OF_MEMORY, "failed to fail with oom error");
}

#endif


static const pv_test_case_t PV_NORMALIZER_TOKEN_TEST_CASES[] = {
        {"token_init_custom_pronunciation", test_pv_normalizer_token_init_custom_pronunciation},
        {"token_init_invalid_custom_pronunciation", test_pv_normalizer_token_invalid_custom_pronunciation},
        {"token_list_append", test_pv_normalizer_token_list_append},
        {"token_list_remove_token", test_pv_normalizer_token_list_remove_token},
        {"token_list_to_token_array", test_pv_normalizer_token_list_to_token_array},
        {"token_copy", test_pv_normalizer_token_copy},
        {"token_list_unroll_token", test_pv_normalizer_token_list_unroll_token},
        {"token_list_unroll_token_invalid_index", test_pv_normalizer_token_list_unroll_token_invalid_index},

#ifdef __PV_MOCKS__

        {"token_init_failure_1", test_pv_normalizer_token_init_failure_1},
        {"token_init_failure_2", test_pv_normalizer_token_init_failure_2},
        {"token_init_failure_3", test_pv_normalizer_token_init_failure_3},

        {"token_copy_failure_calloc_1", test_pv_normalizer_token_copy_failure_calloc_1},
        {"token_copy_failure_calloc_2", test_pv_normalizer_token_copy_failure_calloc_2},
        {"token_copy_failure_calloc_3", test_pv_normalizer_token_copy_failure_calloc_3},
        {"token_copy_failure_calloc_4", test_pv_normalizer_token_copy_failure_calloc_4},
        {"token_copy_failure_calloc_5", test_pv_normalizer_token_copy_failure_calloc_5},

        {"token_read_custom_pronunciation_failure_1", test_pv_normalizer_token_read_custom_pronunciation_failure_1},
        {"token_read_custom_pronunciation_failure_2", test_pv_normalizer_token_read_custom_pronunciation_failure_2},

        {"token_list_init_calloc_failure", test_pv_normalizer_token_list_init_calloc_failure},

#endif

};

const pv_test_suite_t PV_NORMALIZER_TOKEN_TEST_SUITE = {
        .name = "normalizer_token",
        .setup = test_pv_normalizer_token_setup,
        .teardown = test_pv_normalizer_token_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_NORMALIZER_TOKEN_TEST_CASES),
        .test_cases = PV_NORMALIZER_TOKEN_TEST_CASES,
};
