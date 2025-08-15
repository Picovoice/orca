#include <stdlib.h>
#include <string.h>

#include "core/pv_language_json.h"
#include "core/pv_language_internal.h"
#include "hippo/pv_hippo_internal.h"
#include "orca/normalizer/pv_normalizer_token.h"
#include "orca/pv_orca_phonemizer.h"
#include "test/pv_test.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

static pv_orca_phonemizer_t *orca_phonemizer_object = NULL;
static pv_hippo_t *hippo_object = NULL;
static pv_language_info_t *language_info_object = NULL;
static pv_dict_t *dict_object = NULL;
static pv_lexicon_t *lexicon_object = NULL;
static pv_heteronym_tree_t *tree_object = NULL;

static const char *CORRECT_PRONUNCIATIONS[] = {"HH AA HH AA", "W OY", "CH"};
static const char *INCORRECT_PRONUNCIATIONS[] = {"HH AA ", "HH  AA", "BELLA", "-", "@", "C"};

static const pv_normalizer_token_t token1 = {
        .string = "NEURAL",
        .original_string = "NEURAL",
        .verbalized = "NEURAL",
        .next = NULL,
};
static const pv_normalizer_token_t token2 = {
        .string = "NETWORKS",
        .original_string = "NETWORKS",
        .verbalized = "NETWORKS",
        .next = NULL,
};
static const pv_normalizer_token_t token3 = {
        .string = "\"",
        .original_string = "\"",
        .verbalized = "\"",
        .tag_language_agnostic = PV_NORMALIZER_TAG_PUNCTUATION,
        .next = NULL,
};
static const pv_normalizer_token_t token4 = {
        .string = "work", // not capitalized to test hippo_pronounce path
        .original_string = "work",
        .verbalized = "work",
        .next = NULL,
};
static const pv_normalizer_token_t token5 = {
        .string = "\"",
        .original_string = "\"",
        .verbalized = "\"",
        .tag_language_agnostic = PV_NORMALIZER_TAG_PUNCTUATION,
        .next = NULL,
};
static const pv_normalizer_token_t token6 = {
        .string = ",",
        .original_string = ",",
        .verbalized = ",",
        .tag_language_agnostic = PV_NORMALIZER_TAG_PUNCTUATION,
        .next = NULL,
};
static const pv_normalizer_token_t token7 = {
        .string = "SOMETIMES",
        .original_string = "SOMETIMES",
        .verbalized = "SOMETIMES",
        .next = NULL,
};
static const pv_normalizer_token_t token8 = {
        .string = "HAHA",
        .original_string = "HAHA",
        .verbalized = "HH AA HH AA",
        .pronunciation = "HH AA HH AA",
        .tag_language_agnostic = PV_NORMALIZER_TAG_CUSTOM_PRONUNCIATION,
        .next = NULL,
};
static const pv_normalizer_token_t token9 = {
        .string = "!",
        .original_string = "!",
        .verbalized = "!",
        .tag_language_agnostic = PV_NORMALIZER_TAG_PUNCTUATION,
        .next = NULL,
};
static const pv_normalizer_token_t token10 = {
        .string = "The",
        .original_string = "the",
        .verbalized = "THE",
        .next = NULL,
};
static const pv_normalizer_token_t *TOKENS[] =
        {&token1, &token2, &token3, &token4, &token5, &token6, &token7, &token8, &token9};
static const pv_normalizer_token_t *TOKENS_NO_EOS_PUNC[] =
        {&token1, &token2, &token3, &token4, &token5, &token6, &token7};
static const pv_normalizer_token_t *TOKEN_COMMA[] = {&token6};

static const int32_t PHONEME_TOKENS[] = {
        97, 44, 45, 64, 65, 54, 55, 4, 5, 40, 41, 96, 44, 45, 20, 21, 60, 61, 70, 71, 22, 23, 38, 39, 56, 57,
        96, 84, 85, 96, 70, 71, 22, 23, 38, 39, 96, 84, 85, 96, 82, 83, 96, 56, 57, 4, 5, 42, 43, 60, 61, 10,
        11, 42, 43, 74, 75, 96, 30, 31, 0, 1, 30, 31, 0, 1, 96, 88, 89, 98};
static const int32_t PHONEME_TOKENS_NO_EOS_TOKEN[] = {
        97, 44, 45, 64, 65, 54, 55, 4, 5, 40, 41, 96, 44, 45, 20, 21, 60, 61, 70, 71, 22, 23, 38, 39, 56, 57,
        96, 84, 85, 96, 70, 71, 22, 23, 38, 39, 96, 84, 85, 96, 82, 83, 96, 56, 57, 4, 5, 42, 43, 60, 61, 10,
        11, 42, 43, 74, 75, 96, 30, 31, 0, 1, 30, 31, 0, 1, 96, 88, 89};
static const int32_t PHONEME_TOKENS_NO_EOS_PUNC[] = {
        97, 44, 45, 64, 65, 54, 55, 4, 5, 40, 41, 96, 44, 45, 20, 21, 60, 61, 70, 71, 22, 23, 38, 39, 56, 57,
        96, 84, 85, 96, 70, 71, 22, 23, 38, 39, 96, 84, 85, 96, 82, 83, 96, 56, 57, 4, 5, 42, 43, 60, 61, 10,
        11, 42, 43, 74, 75};

static struct phonemize_args {
    pv_orca_phonemizer_t *object;
    int32_t num_text_tokens;
    const pv_normalizer_token_t **text_tokens;
    bool append_terminator;
    bool allow_prepend_bos;
    bool allow_append_eos;
    int32_t num_phoneme_tokens;
    int32_t *phoneme_token;
    int32_t *text_tokens_num_phonemes;
} default_phonemize_args = {
        .num_text_tokens = PV_ARRAY_LEN(TOKENS),
        .text_tokens = (const pv_normalizer_token_t **) TOKENS,
        .append_terminator = true,
        .allow_prepend_bos = true,
        .allow_append_eos = true,
        .num_phoneme_tokens = 0,
        .phoneme_token = NULL,
};

static struct phonemize_args phonemize_args_comma = {
        .num_text_tokens = PV_ARRAY_LEN(TOKEN_COMMA),
        .text_tokens = (const pv_normalizer_token_t **) TOKEN_COMMA,
        .append_terminator = true,
        .allow_prepend_bos = true,
        .allow_append_eos = true,
        .num_phoneme_tokens = 0,
        .phoneme_token = NULL,
};

static const pv_orca_phonemizer_param_t DEFAULT_PHONEMIZER_PARAM = {
        .add_eos_punctuation = true,
        .add_bos_phoneme = true,
        .add_eos_phoneme = true,
        .add_word_boundary_phoneme = true,
        .num_phoneme_multiplier = 2,
};

static pv_status_t test_pv_orca_phonemizer_setup(void) {
    char *hippo_path = pv_test_shared_res_path("hippo/param/hippo_params_en.pv");
    pv_status_t status = pv_hippo_init(hippo_path, &hippo_object);
    free(hippo_path);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    char *lexicon_path = pv_test_module_res_path("test_data/phonemizer_test/lexicon.txt");
    if (!lexicon_path) {
        return PV_STATUS_OUT_OF_MEMORY;
    }
    char *language_info_path = pv_test_module_res_path("language_info/pv_language_info_orca_normalizer_en.json");
    if (!language_info_path) {
        free(lexicon_path);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    status = pv_language_info_load_json(language_info_path, &language_info_object, true, true);
    if (status != PV_STATUS_SUCCESS) {
        free(lexicon_path);
        return status;
    }
    status = pv_lexicon_init(lexicon_path, language_info_path, &lexicon_object);
    free(language_info_path);
    free(lexicon_path);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    char *phoneme_path = pv_test_shared_res_path("phonebook/en-phonemes.txt");
    if (!phoneme_path) {
        return PV_STATUS_OUT_OF_MEMORY;
    }
    char *dict_path = pv_test_module_res_path("test_data/phonemizer_test/dictionary.txt");
    if (!dict_path) {
        return PV_STATUS_OUT_OF_MEMORY;
    }
    status = pv_dict_init(phoneme_path, dict_path, lexicon_object, &dict_object);
    free(dict_path);
    free(phoneme_path);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    char *tree_path = pv_test_module_res_path("test_data/phonemizer_test/heteronym_tree.txt");
    if (!tree_path) {
        return PV_STATUS_OUT_OF_MEMORY;
    }
    status = pv_heteronym_tree_init(tree_path, lexicon_object, 10, &tree_object);
    free(tree_path);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_orca_phonemizer_init(
            &DEFAULT_PHONEMIZER_PARAM,
            hippo_object,
            lexicon_object,
            dict_object,
            tree_object,
            language_info_object,
            &orca_phonemizer_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    default_phonemize_args.object = orca_phonemizer_object;
    phonemize_args_comma.object = orca_phonemizer_object;

    return PV_STATUS_SUCCESS;
}

static void test_pv_orca_phonemizer_teardown(void) {
    pv_language_info_delete(language_info_object);
    pv_dict_delete(dict_object);
    pv_lexicon_delete(lexicon_object);
    pv_hippo_delete(hippo_object);
    pv_orca_phonemizer_delete(orca_phonemizer_object);
    pv_heteronym_tree_delete(tree_object);
}

#ifdef __PV_MOCKS__

static void *calloc_return_null(size_t arg0, size_t arg1) {
    (void) arg0;
    (void) arg1;
    return NULL;
}

static void *malloc_return_null(size_t arg0) {
    (void) arg0;
    return NULL;
}

static void test_pv_orca_phonemizer_init_failure_helper(
        pv_status_t expected_status,
        const char *expected_public_error_message_regex,
        const char *expected_private_error_message_regex) {
    pv_orca_phonemizer_t *test_orca_phonemizer_object = NULL;
    pv_log_disable();
    pv_status_t status = pv_orca_phonemizer_init(
            &DEFAULT_PHONEMIZER_PARAM,
            hippo_object,
            lexicon_object,
            dict_object,
            tree_object,
            language_info_object,
            &test_orca_phonemizer_object);
    pv_log_enable();
    reset_mocks();
    pv_test_true(
            status == expected_status,
            "failed to fail init. Got unexpected status `%s`",
            pv_status_to_string(status));

    if (expected_status != PV_STATUS_SUCCESS) {
        const char *expected_message = expected_public_error_message_regex;

#ifdef __PV_ERROR_SHOW_PRIVATE_MSGS__

        if (expected_private_error_message_regex) {
            expected_message = expected_private_error_message_regex;
        }

#endif

        pv_test_error_message(
                expected_public_error_message_regex,
                expected_private_error_message_regex,
                true,
                "init error message mismatch, expected '%s'",
                expected_message);
    }
}

static void test_pv_orca_phonemizer_init_1st_calloc_failure(void) {
    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs)
    test_pv_orca_phonemizer_init_failure_helper(
        PV_STATUS_OUT_OF_MEMORY,
        "Failed to allocate, out of memory\\.",
        "Failed to allocate memory for `o`\\.");
}

static void test_pv_orca_phonemizer_phonemize_failure_helper(
        struct phonemize_args *args,
        pv_status_t expected_status,
        const char *expected_public_error_message_regex,
        const char *expected_private_error_message_regex) {
    pv_log_disable();
    pv_status_t status = pv_orca_phonemizer_phonemize(
            args->object,
            args->num_text_tokens,
            args->text_tokens,
            0,
            NULL,
            args->append_terminator,
            args->allow_prepend_bos,
            args->allow_append_eos,
            &(args->num_phoneme_tokens),
            &(args->phoneme_token),
            &(args->text_tokens_num_phonemes));
    pv_log_enable();
    reset_mocks();
    pv_test_true(
            status == expected_status,
            "failed to fail phonemizing sentence. Got unexpected status `%s`",
            pv_status_to_string(status));

    if (expected_status != PV_STATUS_SUCCESS) {
        const char *expected_message = expected_public_error_message_regex;

#ifdef __PV_ERROR_SHOW_PRIVATE_MSGS__

        if (expected_private_error_message_regex) {
            expected_message = expected_private_error_message_regex;
        }

#endif

        pv_test_error_message(
                expected_public_error_message_regex,
                expected_private_error_message_regex,
                false,
                "phonemize error message mismatch, expected '%s'",
                expected_message);
    }
}

static void test_pv_orca_phonemizer_phonemize_1st_calloc_failure(void) {
    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs)
    test_pv_orca_phonemizer_phonemize_failure_helper(
        &default_phonemize_args,
        PV_STATUS_OUT_OF_MEMORY,
        "Failed to allocate, out of memory\\.",
        "Failed to allocate memory for `phoneme_tokens_buffer`\\.");
}

static void test_pv_orca_phonemizer_phonemize_2nd_calloc_failure(void) {
    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_real,
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs)
    test_pv_orca_phonemizer_phonemize_failure_helper(
        &default_phonemize_args,
        PV_STATUS_OUT_OF_MEMORY,
        "Failed to allocate, out of memory\\.",
        "Failed to allocate memory for `num_phonemes_buffer`\\.");
}

static void test_pv_orca_phonemizer_phonemize_after_phonemes_buffer_1st_calloc_failure(void) {
    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_real,
            calloc_real,
            calloc_real,
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs)
    test_pv_orca_phonemizer_phonemize_failure_helper(
        &phonemize_args_comma,
        PV_STATUS_OUT_OF_MEMORY,
        "Failed to allocate, out of memory\\.",
        "Failed to allocate memory for `num_phonemes_buffer_with_pronunciation`\\.");
}


static void test_pv_orca_phonemizer_phonemize_after_phonemes_buffer_2nd_calloc_failure(void) {
    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_real,
            calloc_real,
            calloc_real,
            calloc_real,
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs)
    test_pv_orca_phonemizer_phonemize_failure_helper(
        &phonemize_args_comma,
        PV_STATUS_OUT_OF_MEMORY,
        "Failed to allocate, out of memory\\.",
        "Failed to allocate memory for `phonemes_buffer_with_pronunciation`\\.");
}


static void test_pv_orca_phonemizer_phonemize_after_phonemes_buffer_3rd_calloc_failure(void) {
    void *(*custom_funcs[])(size_t arg0, size_t arg1) = {
            calloc_real,
            calloc_real,
            calloc_real,
            calloc_real,
            calloc_real,
            calloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(calloc, custom_funcs)
    test_pv_orca_phonemizer_phonemize_failure_helper(
        &phonemize_args_comma,
        PV_STATUS_OUT_OF_MEMORY,
        "Failed to allocate, out of memory\\.",
        "Failed to allocate memory for `text_tokens_num_encoded_phonemes_final`\\.");
}

static void test_pv_orca_phonemizer_phonemize_1st_language_info_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_language_info_phoneme_index_from_string, PV_STATUS_INVALID_ARGUMENT)
    test_pv_orca_phonemizer_phonemize_failure_helper(
        &default_phonemize_args,
        PV_STATUS_INVALID_ARGUMENT,
        pv_test_function_hash_regex(),
        "`pv_language_info_phoneme_index_from_string` failed with status `INVALID_ARGUMENT`\\.");
}

static void test_pv_orca_phonemizer_phonemize_hippo_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_phonemizer_get_phonemes_hippo, PV_STATUS_INVALID_ARGUMENT)
    test_pv_orca_phonemizer_phonemize_failure_helper(
        &default_phonemize_args,
        PV_STATUS_INVALID_ARGUMENT,
        pv_test_function_hash_regex(),
        "`pv_orca_phonemizer_get_phonemes_hippo` failed with status `INVALID_ARGUMENT`\\.");
}

static void test_pv_orca_phonemizer_get_phonemes_custom_pronunciation_1st_malloc_failure(void) {
    PV_SET_MOCK_CUSTOM_FUNC(malloc, malloc_return_null)
    int32_t num_phonemes = 0;
    int32_t *phonemes = NULL;
    pv_status_t status = pv_orca_phonemizer_get_phonemes_pronunciation(
            orca_phonemizer_object,
            "HH AA HH AA",
            &num_phonemes,
            &phonemes);
    pv_test_true(status == PV_STATUS_OUT_OF_MEMORY, "failed to fail with out of memory");
}

static void test_pv_orca_phonemizer_get_phonemes_custom_pronunciation_2nd_malloc_failure(void) {
    void *(*custom_funcs[])(size_t arg0) = {
            malloc_real,
            malloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(malloc, custom_funcs)
    int32_t num_phonemes = 0;
    int32_t *phonemes = NULL;
    pv_status_t status = pv_orca_phonemizer_get_phonemes_pronunciation(
            orca_phonemizer_object,
            "HH AA HH AA",
            &num_phonemes,
            &phonemes);
    pv_test_true(status == PV_STATUS_OUT_OF_MEMORY, "failed to fail with out of memory");
}

static void test_pv_orca_phonemizer_get_phonemes_lexicon_malloc_failure(void) {
    PV_SET_MOCK_CUSTOM_FUNC(malloc, malloc_return_null)
    int32_t num_phonemes = 0;
    int32_t *phonemes = NULL;
    pv_status_t status = pv_orca_phonemizer_get_phonemes_pronunciation(
            orca_phonemizer_object,
            "HH AA HH AA",
            &num_phonemes,
            &phonemes);
    pv_test_true(status == PV_STATUS_OUT_OF_MEMORY, "failed to fail with out of memory");
}

static void test_pv_orca_phonemizer_get_phonemes_hippo_1st_malloc_failure(void) {
    PV_SET_MOCK_CUSTOM_FUNC(malloc, malloc_return_null)
    int32_t num_phonemes = 0;
    int32_t *phonemes = NULL;
    pv_status_t status = pv_orca_phonemizer_get_phonemes_hippo(
            orca_phonemizer_object,
            "HH AA HH AA",
            &num_phonemes,
            &phonemes);
    pv_test_true(status == PV_STATUS_OUT_OF_MEMORY, "failed to fail with out of memory");
}

static void test_pv_orca_phonemizer_get_phonemes_hippo_2nd_malloc_failure(void) {
    void *(*custom_funcs[])(size_t arg0) = {
            malloc_real,
            malloc_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(malloc, custom_funcs)

    int32_t num_phonemes = 0;
    int32_t *phonemes = NULL;
    pv_status_t status = pv_orca_phonemizer_get_phonemes_hippo(
            orca_phonemizer_object,
            "HH AA HH AA",
            &num_phonemes,
            &phonemes);
    pv_test_true(status == PV_STATUS_OUT_OF_MEMORY, "failed to fail with out of memory");
}

static void test_pv_orca_phonemizer_get_terminator_index_language_info_failure_1(void) {
    PV_SET_MOCK_RETURN_VAL(pv_language_info_terminator_index_to_string, PV_STATUS_INVALID_ARGUMENT)

    int32_t terminator = 0;
    pv_status_t status = pv_orca_phonemizer_get_terminator_index(language_info_object, &terminator);
    pv_test_true(status == PV_STATUS_INVALID_ARGUMENT, "failed to fail with out of memory");
}

static void test_pv_orca_phonemizer_get_terminator_index_language_info_failure_2(void) {
    PV_SET_MOCK_RETURN_VAL(pv_language_info_phoneme_index_from_string, PV_STATUS_INVALID_ARGUMENT)

    int32_t terminator = 0;
    pv_status_t status = pv_orca_phonemizer_get_terminator_index(language_info_object, &terminator);
    pv_test_true(status == PV_STATUS_INVALID_ARGUMENT, "failed to fail with out of memory");
}

static size_t pv_fread_ret_zero(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    (void) ptr;
    (void) size;
    (void) nmemb;
    (void) stream;

    return 0;
}

static size_t pv_fread_ret_one(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    (void) ptr;
    (void) size;
    (void) nmemb;
    (void) stream;

    return 1;
}

static void test_pv_orca_phonemizer_param_load_helper(pv_status_t expected) {
    FILE *dummy_file = malloc(sizeof(FILE *));
    pv_test_true(dummy_file != NULL, "failed create dummy file");
    if (!dummy_file) {
        return;
    }

    pv_orca_phonemizer_param_t *param = NULL;
    pv_status_t status = pv_orca_phonemizer_param_load(dummy_file, &param);
    pv_test_true(
            status == expected,
            "param load error, got '%s' expected '%s'",
            pv_status_to_string(status),
            pv_status_to_string(expected));
    free(dummy_file);
}

static void test_pv_orca_phonemizer_param_load_failure_1(void) {
    PV_SET_MOCK_RETURN_VAL(pv_fread, 0)

    test_pv_orca_phonemizer_param_load_helper(PV_STATUS_IO_ERROR);
}

static void test_pv_orca_phonemizer_param_load_failure_2(void) {
    size_t (*custom_funcs[])(void *arg0, size_t arg1, size_t arg2, FILE *arg3) = {
            pv_fread_ret_one,
            pv_fread_ret_zero,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(pv_fread, custom_funcs)

    test_pv_orca_phonemizer_param_load_helper(PV_STATUS_IO_ERROR);
}

static void test_pv_orca_phonemizer_param_load_failure_3(void) {
    size_t (*custom_funcs[])(void *arg0, size_t arg1, size_t arg2, FILE *arg3) = {
            pv_fread_ret_one,
            pv_fread_ret_one,
            pv_fread_ret_zero,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(pv_fread, custom_funcs)

    test_pv_orca_phonemizer_param_load_helper(PV_STATUS_IO_ERROR);
}

static void test_pv_orca_phonemizer_param_load_failure_4(void) {
    size_t (*custom_funcs[])(void *arg0, size_t arg1, size_t arg2, FILE *arg3) = {
            pv_fread_ret_one,
            pv_fread_ret_one,
            pv_fread_ret_one,
            pv_fread_ret_zero,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(pv_fread, custom_funcs)


    test_pv_orca_phonemizer_param_load_helper(PV_STATUS_IO_ERROR);
}

static void test_pv_orca_phonemizer_param_load_failure_5(void) {
    size_t (*custom_funcs[])(void *arg0, size_t arg1, size_t arg2, FILE *arg3) = {
            pv_fread_ret_one,
            pv_fread_ret_one,
            pv_fread_ret_one,
            pv_fread_ret_one,
            pv_fread_ret_zero,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(pv_fread, custom_funcs)

    test_pv_orca_phonemizer_param_load_helper(PV_STATUS_IO_ERROR);
}

#endif

static void test_pv_orca_phonemizer_get_phonemes_custom_pronunciation(void) {
    for (int32_t i = 0; i < PV_ARRAY_LEN(CORRECT_PRONUNCIATIONS); i++) {
        int32_t num_phonemes = 0;
        int32_t *phonemes = NULL;
        pv_status_t status = pv_orca_phonemizer_get_phonemes_pronunciation(
                orca_phonemizer_object,
                CORRECT_PRONUNCIATIONS[i],
                &num_phonemes,
                &phonemes);
        pv_test_true(
                status == PV_STATUS_SUCCESS,
                "failed to get phoneme indices for input `%s`",
                CORRECT_PRONUNCIATIONS[i]);
        free(phonemes);
    }

    for (int32_t i = 0; i < PV_ARRAY_LEN(INCORRECT_PRONUNCIATIONS); i++) {
        int32_t num_phonemes = 0;
        int32_t *phonemes = NULL;
        pv_status_t status = pv_orca_phonemizer_get_phonemes_pronunciation(
                orca_phonemizer_object,
                INCORRECT_PRONUNCIATIONS[i],
                &num_phonemes,
                &phonemes);
        pv_test_true(
                status == PV_STATUS_INVALID_ARGUMENT,
                "failed to fail with invalid argument for phoneme input `%s`. Got status `%s`",
                INCORRECT_PRONUNCIATIONS[i],
                pv_status_to_string(status));
        free(phonemes);
    }
}

static void test_pv_orca_phonemizer_spell_out(void) {
    const char *string = "XAV";
    char *expected_pronunciation = "EH K S EY V IY";
    char *pronunciation = NULL;
    pv_status_t status = pv_orca_phonemizer_spell_out(orca_phonemizer_object, string, &pronunciation);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "failed to get phoneme indices for input `%s`",
            string);
    pv_test_true(
            strcmp(pronunciation, expected_pronunciation) == 0,
            "failed to get correct pronunciation for input `%s`. Got `%s`, expected `%s`",
            string,
            pronunciation,
            expected_pronunciation);
    free(pronunciation);
}

static void test_pv_orca_phonemizer_phonemize_1(void) {
    int32_t num_encoded_phonemes = 0;
    int32_t *encoded_phonemes = NULL;
    int32_t *text_tokens_num_encoded_phonemes = NULL;
    pv_status_t status = pv_orca_phonemizer_phonemize(
            orca_phonemizer_object,
            PV_ARRAY_LEN(TOKENS),
            (const pv_normalizer_token_t **) TOKENS,
            0,
            NULL,
            true,
            true,
            true,
            &num_encoded_phonemes,
            &encoded_phonemes,
            &text_tokens_num_encoded_phonemes);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to phonemize sentence with `%s`", pv_status_to_string(status));

    pv_test_true(
            num_encoded_phonemes == PV_ARRAY_LEN(PHONEME_TOKENS),
            "num_phoneme_tokens mismatch: got  `%d`, expected `%d`",
            num_encoded_phonemes,
            PV_ARRAY_LEN(PHONEME_TOKENS));

    for (int32_t i = 0; i < num_encoded_phonemes; i++) {
        pv_test_true(
                encoded_phonemes[i] == PHONEME_TOKENS[i],
                "phoneme_tokens mismatch at index `%d`. got `%d`, expected `%d`",
                i,
                encoded_phonemes[i],
                PHONEME_TOKENS[i]);
    }
    free(encoded_phonemes);
    free(text_tokens_num_encoded_phonemes);
}

static void test_pv_orca_phonemizer_phonemize_2(void) {
    const pv_normalizer_token_t token = {
            .string = "HELLO",
            .original_string = "HELLO",
            .verbalized = "HELLO",
            .next = NULL,
    };
    const pv_normalizer_token_t *tokens[] = {&token};

    const int32_t target[] = {97, 30, 31, 20, 21, 40, 41, 48, 49, 98};

    int32_t num_encoded_phonemes = 0;
    int32_t *encoded_phonemes = NULL;
    int32_t *text_tokens_num_encoded_phonemes = NULL;
    pv_status_t status = pv_orca_phonemizer_phonemize(
            orca_phonemizer_object,
            PV_ARRAY_LEN(tokens),
            (const pv_normalizer_token_t **) tokens,
            0,
            NULL,
            false,
            true,
            true,
            &num_encoded_phonemes,
            &encoded_phonemes,
            &text_tokens_num_encoded_phonemes);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to phonemize sentence with `%s`", pv_status_to_string(status));

    pv_test_true(
            num_encoded_phonemes == PV_ARRAY_LEN(target),
            "num_phoneme_tokens mismatch: got  `%d`, expected `%d`",
            num_encoded_phonemes,
            PV_ARRAY_LEN(target));

    for (int32_t i = 0; i < num_encoded_phonemes; i++) {
        pv_test_true(
                encoded_phonemes[i] == target[i],
                "phoneme_tokens mismatch at index `%d`. got `%d`, expected `%d`",
                i,
                encoded_phonemes[i],
                target[i]);
    }
    free(encoded_phonemes);
    free(text_tokens_num_encoded_phonemes);
}

static void test_pv_orca_phonemizer_phonemize_heteronym_1(void) {
    const pv_normalizer_token_t random_token = {
            .string = "dolphin",
            .original_string = "dolphin",
            .verbalized = "DOLPHIN",
            .next = NULL,
    };

    const pv_normalizer_token_t heteronym_token = {
            .string = "reuse",
            .original_string = "reuse",
            .verbalized = "REUSE",
            .next = NULL,
    };

    const pv_normalizer_token_t *tokens[] = {&token10, &random_token, &heteronym_token};

    const int32_t target[] = {
            18, 19, 4, 5, 96,
            16, 17, 0, 1, 40, 41, 26, 27, 4, 5, 44, 45, 96,
            54, 55, 34, 35, 72, 73, 66, 67, 56, 57
    };

    int32_t num_encoded_phonemes = 0;
    int32_t *encoded_phonemes = NULL;
    int32_t *text_tokens_num_encoded_phonemes = NULL;
    pv_status_t status = pv_orca_phonemizer_phonemize(
            orca_phonemizer_object,
            PV_ARRAY_LEN(tokens),
            (const pv_normalizer_token_t **) tokens,
            0,
            NULL,
            false,
            false,
            false,
            &num_encoded_phonemes,
            &encoded_phonemes,
            &text_tokens_num_encoded_phonemes);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to phonemize sentence with `%s`", pv_status_to_string(status));

    pv_test_true(
            num_encoded_phonemes == PV_ARRAY_LEN(target),
            "num_phoneme_tokens mismatch: got  `%d`, expected `%d`",
            num_encoded_phonemes,
            PV_ARRAY_LEN(target));

    for (int32_t i = 0; i < num_encoded_phonemes; i++) {
        pv_test_true(
                encoded_phonemes[i] == target[i],
                "phoneme_tokens mismatch at index `%d`. got `%d`, expected `%d`",
                i,
                encoded_phonemes[i],
                target[i]);
    }
    free(encoded_phonemes);
    free(text_tokens_num_encoded_phonemes);
}

static void test_pv_orca_phonemizer_phonemize_heteronym_2(void) {
    const pv_normalizer_token_t heteronym_token = {
            .string = "aged",
            .original_string = "aged",
            .verbalized = "AGED",
            .next = NULL,
    };

    const pv_normalizer_token_t *tokens[] = {&token10, &heteronym_token};

    const int32_t target[] = {18, 19, 4, 5, 96, 24, 25, 36, 37, 32, 33, 16, 17};

    int32_t num_encoded_phonemes = 0;
    int32_t *encoded_phonemes = NULL;
    int32_t *text_tokens_num_encoded_phonemes = NULL;
    pv_status_t status = pv_orca_phonemizer_phonemize(
            orca_phonemizer_object,
            PV_ARRAY_LEN(tokens),
            (const pv_normalizer_token_t **) tokens,
            0,
            NULL,
            false,
            false,
            false,
            &num_encoded_phonemes,
            &encoded_phonemes,
            &text_tokens_num_encoded_phonemes);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to phonemize sentence with `%s`", pv_status_to_string(status));

    pv_test_true(
            num_encoded_phonemes == PV_ARRAY_LEN(target),
            "num_phoneme_tokens mismatch: got  `%d`, expected `%d`",
            num_encoded_phonemes,
            PV_ARRAY_LEN(target));

    for (int32_t i = 0; i < num_encoded_phonemes; i++) {
        pv_test_true(
                encoded_phonemes[i] == target[i],
                "phoneme_tokens mismatch at index `%d`. got `%d`, expected `%d`",
                i,
                encoded_phonemes[i],
                target[i]);
    }
    free(encoded_phonemes);
    free(text_tokens_num_encoded_phonemes);

    const pv_normalizer_token_t *tokens2[] = {&heteronym_token};
    const int32_t target2[] = {24, 25, 36, 37, 16, 17};

    num_encoded_phonemes = 0;
    encoded_phonemes = NULL;
    text_tokens_num_encoded_phonemes = NULL;
    status = pv_orca_phonemizer_phonemize(
            orca_phonemizer_object,
            PV_ARRAY_LEN(tokens2),
            (const pv_normalizer_token_t **) tokens2,
            0,
            NULL,
            false,
            false,
            false,
            &num_encoded_phonemes,
            &encoded_phonemes,
            &text_tokens_num_encoded_phonemes);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to phonemize sentence with `%s`", pv_status_to_string(status));

    pv_test_true(
            num_encoded_phonemes == PV_ARRAY_LEN(target2),
            "num_phoneme_tokens mismatch: got  `%d`, expected `%d`",
            num_encoded_phonemes,
            PV_ARRAY_LEN(target2));

    for (int32_t i = 0; i < num_encoded_phonemes; i++) {
        pv_test_true(
                encoded_phonemes[i] == target2[i],
                "phoneme_tokens mismatch at index `%d`. got `%d`, expected `%d`",
                i,
                encoded_phonemes[i],
                target2[i]);
    }
    free(encoded_phonemes);
    free(text_tokens_num_encoded_phonemes);
}

static void test_pv_orca_phonemizer_phonemize_no_eos_token(void) {
    const pv_orca_phonemizer_param_t phonemizer_param = {
            .add_eos_punctuation = true,
            .add_bos_phoneme = true,
            .add_eos_phoneme = false,
            .add_word_boundary_phoneme = true,
            .num_phoneme_multiplier = 2,
    };

    pv_orca_phonemizer_t *orca_phonemizer = NULL;
    pv_status_t status = pv_orca_phonemizer_init(
            &phonemizer_param,
            hippo_object,
            lexicon_object,
            dict_object,
            tree_object,
            language_info_object,
            &orca_phonemizer);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to initialize phonemizer with `%s`", pv_status_to_string(status));
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    int32_t num_encoded_phonemes = 0;
    int32_t *encoded_phonemes = NULL;
    int32_t *text_tokens_num_encoded_phonemes = NULL;
    status = pv_orca_phonemizer_phonemize(
            orca_phonemizer,
            PV_ARRAY_LEN(TOKENS),
            (const pv_normalizer_token_t **) TOKENS,
            0,
            NULL,
            false,
            true,
            true,
            &num_encoded_phonemes,
            &encoded_phonemes,
            &text_tokens_num_encoded_phonemes);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to phonemize sentence with `%s`", pv_status_to_string(status));

    pv_test_true(
            num_encoded_phonemes == PV_ARRAY_LEN(PHONEME_TOKENS_NO_EOS_TOKEN),
            "num_phoneme_tokens mismatch: got  `%d`, expected `%d`",
            num_encoded_phonemes,
            PV_ARRAY_LEN(PHONEME_TOKENS_NO_EOS_TOKEN));

    for (int32_t i = 0; i < num_encoded_phonemes; i++) {
        pv_test_true(
                encoded_phonemes[i] == PHONEME_TOKENS_NO_EOS_TOKEN[i],
                "phoneme_tokens mismatch at index `%d`. got `%d`, expected `%d`",
                i,
                encoded_phonemes[i],
                PHONEME_TOKENS_NO_EOS_TOKEN[i]);
    }
    free(encoded_phonemes);
    free(text_tokens_num_encoded_phonemes);
    pv_orca_phonemizer_delete(orca_phonemizer);
}

static void test_pv_orca_phonemizer_phonemize_no_eos_punc(void) {
    const pv_orca_phonemizer_param_t phonemizer_param = {
            .add_eos_punctuation = false,
            .add_bos_phoneme = true,
            .add_eos_phoneme = false,
            .add_word_boundary_phoneme = true,
            .num_phoneme_multiplier = 2,
    };

    pv_orca_phonemizer_t *orca_phonemizer = NULL;
    pv_status_t status = pv_orca_phonemizer_init(
            &phonemizer_param,
            hippo_object,
            lexicon_object,
            dict_object,
            tree_object,
            language_info_object,
            &orca_phonemizer);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to initialize phonemizer with `%s`", pv_status_to_string(status));
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    int32_t num_encoded_phonemes = 0;
    int32_t *encoded_phonemes = NULL;
    int32_t *text_tokens_num_encoded_phonemes = NULL;
    status = pv_orca_phonemizer_phonemize(
            orca_phonemizer,
            PV_ARRAY_LEN(TOKENS_NO_EOS_PUNC),
            (const pv_normalizer_token_t **) TOKENS_NO_EOS_PUNC,
            0,
            NULL,
            false,
            true,
            true,
            &num_encoded_phonemes,
            &encoded_phonemes,
            &text_tokens_num_encoded_phonemes);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to phonemize sentence with `%s`", pv_status_to_string(status));

    pv_test_true(
            num_encoded_phonemes == PV_ARRAY_LEN(PHONEME_TOKENS_NO_EOS_PUNC),
            "num_phoneme_tokens mismatch: got  `%d`, expected `%d`",
            num_encoded_phonemes,
            PV_ARRAY_LEN(PHONEME_TOKENS_NO_EOS_PUNC));

    for (int32_t i = 0; i < num_encoded_phonemes; i++) {
        pv_test_true(
                encoded_phonemes[i] == PHONEME_TOKENS_NO_EOS_PUNC[i],
                "phoneme_tokens mismatch at index `%d`. got `%d`, expected `%d`",
                i,
                encoded_phonemes[i],
                PHONEME_TOKENS_NO_EOS_PUNC[i]);
    }
    free(encoded_phonemes);
    free(text_tokens_num_encoded_phonemes);
    pv_orca_phonemizer_delete(orca_phonemizer);
}

static const pv_test_case_t PV_ORCA_PHONEMIZER_TEST_CASES[] = {
        {"phonemize 1", test_pv_orca_phonemizer_phonemize_1},
        {"phonemize 2", test_pv_orca_phonemizer_phonemize_2},
        {"phonemize heteronym 1", test_pv_orca_phonemizer_phonemize_heteronym_1},
        {"phonemize heteronym 2", test_pv_orca_phonemizer_phonemize_heteronym_2},
        {"phonemize no eos token", test_pv_orca_phonemizer_phonemize_no_eos_token},
        {"phonemize no eos punc", test_pv_orca_phonemizer_phonemize_no_eos_punc},
        {"phonemize custom pronunciation", test_pv_orca_phonemizer_get_phonemes_custom_pronunciation},
        {"phonemize spell out", test_pv_orca_phonemizer_spell_out},

#ifdef __PV_MOCKS__

        {"init 1st calloc failure", test_pv_orca_phonemizer_init_1st_calloc_failure},
        {"phonemize 1st calloc failure", test_pv_orca_phonemizer_phonemize_1st_calloc_failure},
        {"phonemize 2nd calloc failure", test_pv_orca_phonemizer_phonemize_2nd_calloc_failure},
        {"phonemize after phonemes buffer 1st calloc failure", test_pv_orca_phonemizer_phonemize_after_phonemes_buffer_1st_calloc_failure},
        {"phonemize after phonemes buffer 2nd calloc failure", test_pv_orca_phonemizer_phonemize_after_phonemes_buffer_2nd_calloc_failure},
        {"phonemize after phonemes buffer 3rd calloc failure", test_pv_orca_phonemizer_phonemize_after_phonemes_buffer_3rd_calloc_failure},
        {"phonemize 1st language_info failure", test_pv_orca_phonemizer_phonemize_1st_language_info_failure},
        {"phonemize hippo failure", test_pv_orca_phonemizer_phonemize_hippo_failure},
        {"phonemize custom pronunciation 1st malloc failure",
         test_pv_orca_phonemizer_get_phonemes_custom_pronunciation_1st_malloc_failure},
        {"phonemize custom pronunciation 2nd malloc failure",
         test_pv_orca_phonemizer_get_phonemes_custom_pronunciation_2nd_malloc_failure},
        {"phonemize lexicon malloc failure", test_pv_orca_phonemizer_get_phonemes_lexicon_malloc_failure},
        {"phonemize hippo 1st malloc failure", test_pv_orca_phonemizer_get_phonemes_hippo_1st_malloc_failure},
        {"phonemize hippo 2nd malloc failure", test_pv_orca_phonemizer_get_phonemes_hippo_2nd_malloc_failure},
        {"get terminator index language_info failure 1",
         test_pv_orca_phonemizer_get_terminator_index_language_info_failure_1},
        {"get terminator index language_info failure 2",
         test_pv_orca_phonemizer_get_terminator_index_language_info_failure_2},

        {"param load failure 1", test_pv_orca_phonemizer_param_load_failure_1},
        {"param load failure 2", test_pv_orca_phonemizer_param_load_failure_2},
        {"param load failure 3", test_pv_orca_phonemizer_param_load_failure_3},
        {"param load failure 4", test_pv_orca_phonemizer_param_load_failure_4},
        {"param load failure 5", test_pv_orca_phonemizer_param_load_failure_5},

#endif

};

const pv_test_suite_t PV_ORCA_PHONEMIZER_TEST_SUITE = {
        .name = "orca_phonemizer",
        .setup = test_pv_orca_phonemizer_setup,
        .teardown = test_pv_orca_phonemizer_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_ORCA_PHONEMIZER_TEST_CASES),
        .test_cases = PV_ORCA_PHONEMIZER_TEST_CASES,
};
