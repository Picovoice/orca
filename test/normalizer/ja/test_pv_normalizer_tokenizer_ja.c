#include <stdlib.h>
#include <string.h>

#include "core/pv_language_json.h"
#include "orca/normalizer/ja/pv_normalizer_tokenizer_ja.h"
#include "test/pv_test.h"
#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_normalizer_mock.h"

#endif

static pv_normalizer_tokenizer_ja_t *tokenizer = NULL;
static pv_language_info_t *language_info_object = NULL;
static void *tokenizer_data = NULL;

static const char LANGUAGE_INFO_PATH[] = "normalizer/ref/pv_language_info_normalizer_ja.json";
static const char TOKENIZER_DATA_PATH[] = "normalizer/tokenizer_data/ipadic.bin";

static pv_status_t test_pv_normalizer_tokenizer_ja_setup(void) {
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

    char *tokenizer_data_path = pv_test_resource_path(TOKENIZER_DATA_PATH);
    pv_test_true(tokenizer_data_path != NULL, "Failed to get tokenizer_data_path");
    if (!tokenizer_data_path) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    int32_t num_content_bytes = 0;
    status = pv_file_load(tokenizer_data_path, &num_content_bytes, &tokenizer_data);
    free(tokenizer_data_path);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    const void *shadow = tokenizer_data;
    status = pv_normalizer_tokenizer_ja_init(
            PV_NORMALIZER_LANGUAGE_JA,
            language_info_object,
            &shadow,
            &tokenizer);
    return status;
}

static void test_pv_normalizer_tokenizer_ja_teardown(void) {
    free(tokenizer_data);
    pv_language_info_delete(language_info_object);
    pv_normalizer_tokenizer_ja_delete(tokenizer);
}

static void test_pv_normalizer_tokenizer_ja_basic(void) {
    const pv_normalizer_token_t expected_tokens[] = {
            {
                    .original_string = "日本語",
                    .string = "日本語",
                    .reading = "ニホンゴ",
                    .tag_language_agnostic = PV_NORMALIZER_TAG_NONE
            },
            {
                    .original_string = "が",
                    .string = "が",
                    .reading = "ガ",
                    .tag_language_agnostic = PV_NORMALIZER_TAG_NONE
            },
            {
                    .original_string = "少し",
                    .string = "少し",
                    .reading = "スコシ",
                    .tag_language_agnostic = PV_NORMALIZER_TAG_NONE
            },
            {
                    .original_string = "話せ",
                    .string = "話せ",
                    .reading = "ハナセ",
                    .tag_language_agnostic = PV_NORMALIZER_TAG_NONE
            },
            {
                    .original_string = "ます",
                    .string = "ます",
                    .reading = "マス",
                    .tag_language_agnostic = PV_NORMALIZER_TAG_NONE
            },
            {
                    .original_string = "。",
                    .string = "。",
                    .tag_language_agnostic = PV_NORMALIZER_TAG_PUNCTUATION
            },
    };

    pv_normalizer_token_list_t *token_list = NULL;
    pv_status_t status = pv_normalizer_tokenizer_ja_tokenize(
            tokenizer,
            "日本語が少し話せます。",
            false,
            false,
            true,
            &token_list);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "tokenize failed with %s",
            pv_status_to_string(status));
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    pv_normalizer_token_t *current = token_list->head;
    for (int32_t i = 0; i < PV_ARRAY_LEN(expected_tokens); i++) {
        pv_test_true(
                current,
                "tokenize only returned %d tokens, %d tokens expected",
                i + 1,
                PV_ARRAY_LEN(expected_tokens));
        if (!current) {
            return;
        }

        pv_test_true(
                strcmp(current->string, expected_tokens[i].string) == 0,
                "incorrect string: got `%s`, expected `%s`",
                current->string,
                expected_tokens[i].string);
        pv_test_true(
                strcmp(current->original_string, expected_tokens[i].original_string) == 0,
                "incorrect original_string: got `%s`, expected `%s`",
                current->original_string,
                expected_tokens[i].original_string);
        if (expected_tokens[i].reading != NULL) {
            pv_test_true(
                    strcmp(current->reading, expected_tokens[i].reading) == 0,
                    "incorrect reading: got `%s`, expected `%s`",
                    current->reading,
                    expected_tokens[i].reading);
        }
        pv_test_true(
                current->tag_language_agnostic == expected_tokens[i].tag_language_agnostic,
                "incorrect tag_language_agnostic: got `%d`, expected `%d`",
                current->tag_language_agnostic,
                expected_tokens[i].tag_language_agnostic);
        current = current->next;
    }

    pv_normalizer_token_list_delete(token_list);
}

static void test_pv_normalizer_tokenizer_ja_no_tokens(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    pv_status_t status = pv_normalizer_tokenizer_ja_tokenize(
            tokenizer,
            "",
            false,
            false,
            true,
            &token_list);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "tokenize failed with %s",
            pv_status_to_string(status));
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    pv_test_true(
            token_list == NULL,
            "tokenize was supposed to return no tokens");
    if (token_list) {
        pv_normalizer_token_list_delete(token_list);
    }
}

static void test_pv_normalizer_tokenizer_ja_custom_pronunciation(void) {
    const pv_normalizer_token_t expected_tokens[] = {
            {
                    .original_string = "日本語",
                    .string = "日本語",
                    .reading = "ニホンゴ",
                    .tag_language_agnostic = PV_NORMALIZER_TAG_NONE
            },
            {
                    .original_string = "が",
                    .string = "が",
                    .reading = "ガ",
                    .tag_language_agnostic = PV_NORMALIZER_TAG_NONE
            },
            {
                    .original_string = "少し",
                    .string = "少し",
                    .reading = "スコシ",
                    .tag_language_agnostic = PV_NORMALIZER_TAG_NONE
            },
            {
                    .original_string = "話せ",
                    .string = "話せ",
                    .pronunciation = "n o",
                    .tag_language_agnostic = PV_NORMALIZER_TAG_CUSTOM_PRONUNCIATION
            },
            {
                    .original_string = "ます",
                    .string = "ます",
                    .reading = "マス",
                    .tag_language_agnostic = PV_NORMALIZER_TAG_NONE
            },
            {
                    .original_string = "。",
                    .string = "。",
                    .tag_language_agnostic = PV_NORMALIZER_TAG_PUNCTUATION
            },
    };

    pv_normalizer_token_list_t *token_list = NULL;
    pv_status_t status = pv_normalizer_tokenizer_ja_tokenize(
            tokenizer,
            "日本語が少し{話せ|n o}ます。",
            false,
            false,
            true,
            &token_list);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "tokenize failed with %s",
            pv_status_to_string(status));
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    pv_normalizer_token_t *current = token_list->head;
    for (int32_t i = 0; i < PV_ARRAY_LEN(expected_tokens); i++) {
        pv_test_true(
                current,
                "tokenize only returned %d tokens, %d tokens expected",
                i + 1,
                PV_ARRAY_LEN(expected_tokens));
        if (!current) {
            return;
        }

        pv_test_true(
                strcmp(current->string, expected_tokens[i].string) == 0,
                "incorrect string: got `%s`, expected `%s`",
                current->string,
                expected_tokens[i].string);
        pv_test_true(
                strcmp(current->original_string, expected_tokens[i].original_string) == 0,
                "incorrect original_string: got `%s`, expected `%s`",
                current->original_string,
                expected_tokens[i].original_string);
        if (expected_tokens[i].reading != NULL) {
            pv_test_true(
                    strcmp(current->reading, expected_tokens[i].reading) == 0,
                    "incorrect reading: got `%s`, expected `%s`",
                    current->reading,
                    expected_tokens[i].reading);
        }
        pv_test_true(
                current->tag_language_agnostic == expected_tokens[i].tag_language_agnostic,
                "incorrect tag_language_agnostic: got `%d`, expected `%d`",
                current->tag_language_agnostic,
                expected_tokens[i].tag_language_agnostic);
        if (expected_tokens[i].pronunciation) {
            pv_test_true(current->pronunciation, "current->pronunciation was NULL");
            if (current->pronunciation) {
                pv_test_true(
                        strcmp(current->pronunciation, expected_tokens[i].pronunciation) == 0,
                        "incorrect pronunciation: got `%s`, expected `%s`",
                        current->pronunciation,
                        expected_tokens[i].pronunciation);
            }
        }
        current = current->next;
    }

    pv_normalizer_token_list_delete(token_list);
}

static void test_pv_normalizer_tokenizer_ja_alphanum(void) {
    const pv_normalizer_token_t expected_tokens[] = {
            {
                    .original_string = "私",
                    .string = "私",
                    .reading = "ワタシ",
            },
            {
                    .original_string = "の",
                    .string = "の",
                    .reading = "ノ",
            },
            {
                    .original_string = "名前",
                    .string = "名前",
                    .reading = "ナマエ",
            },
            {
                    .original_string = "は",
                    .string = "は",
                    .reading = "ワ",
            },
            {
                    .original_string = "ｃ３ｐＯ",
                    .string = "c3pO",
            },
            {
                    .original_string = "です",
                    .string = "です",
                    .reading = "デス"
            },
    };

    pv_normalizer_token_list_t *token_list = NULL;
    pv_status_t status = pv_normalizer_tokenizer_ja_tokenize(
            tokenizer,
            "私の名前はｃ３ｐＯです",
            false,
            false,
            true,
            &token_list);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "tokenize failed with %s",
            pv_status_to_string(status));
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    pv_normalizer_token_t *current = token_list->head;
    for (int32_t i = 0; i < PV_ARRAY_LEN(expected_tokens); i++) {
        pv_test_true(
                current,
                "tokenize only returned %d tokens, %d tokens expected",
                i + 1,
                PV_ARRAY_LEN(expected_tokens));
        if (!current) {
            return;
        }

        pv_test_true(
                strcmp(current->string, expected_tokens[i].string) == 0,
                "incorrect string: got `%s`, expected `%s`",
                current->string,
                expected_tokens[i].string);
        pv_test_true(
                strcmp(current->original_string, expected_tokens[i].original_string) == 0,
                "incorrect original_string: got `%s`, expected `%s`",
                current->original_string,
                expected_tokens[i].original_string);
        if (expected_tokens[i].reading != NULL) {
            pv_test_true(
                    strcmp(current->reading, expected_tokens[i].reading) == 0,
                    "incorrect reading: got `%s`, expected `%s`",
                    current->reading,
                    expected_tokens[i].reading);
        }

        current = current->next;
    }

    pv_normalizer_token_list_delete(token_list);
}

static void test_pv_normalizer_tokenizer_ja_tokenize_on_character(void) {
    const pv_normalizer_token_t expected_tokens[] = {
            {.original_string = "の", .string = "の"},
            {.original_string = "です", .string = "です"},
    };

    pv_normalizer_token_list_t *token_list = NULL;
    pv_status_t status = pv_normalizer_tokenizer_ja_tokenize_on_character(
            tokenizer,
            "の-です",
            '-',
            false,
            false,
            false,
            &token_list);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "tokenize failed with %s",
            pv_status_to_string(status));
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    pv_normalizer_token_t *current = token_list->head;
    for (int32_t i = 0; i < PV_ARRAY_LEN(expected_tokens); i++) {
        pv_test_true(
                current,
                "tokenize only returned %d tokens, %d tokens expected",
                i + 1,
                PV_ARRAY_LEN(expected_tokens));
        if (!current) {
            return;
        }

        pv_test_true(
                strcmp(current->string, expected_tokens[i].string) == 0,
                "incorrect string: got `%s`, expected `%s`",
                current->string,
                expected_tokens[i].string);
        pv_test_true(
                strcmp(current->original_string, expected_tokens[i].original_string) == 0,
                "incorrect original_string: got `%s`, expected `%s`",
                current->original_string,
                expected_tokens[i].original_string);

        current = current->next;
    }

    pv_normalizer_token_list_delete(token_list);
}

static void test_pv_normalizer_tokenizer_ja_basic_streaming(void) {
    static char *input_tokens[] = {
            "こ", "の", "レ", "ス", "ト", "ラ", "ン", "は", "料", "理", "が", "と", "て", "も", "美", "味", "し",
            "い", "の", "で", "、", "友", "達", "と", "一", "緒", "に", "よ", "く", "来", "ま", "す", "。"};
    const pv_normalizer_token_t expected_tokens[] = {
            {.original_string = "この", .string = "この", .reading = "コノ"},
            {.original_string = "レストラン", .string = "レストラン", .reading = "レストラン"},
            {.original_string = "は", .string = "は", .reading = "ワ"},
            {.original_string = "料理", .string = "料理", .reading = "リョーリ"},
            {.original_string = "が", .string = "が", .reading = "ガ"},
            {.original_string = "とても", .string = "とても", .reading = "トテモ"},
            {.original_string = "美味しい", .string = "美味しい", .reading = "オイシイ"},
            {.original_string = "ので", .string = "ので", .reading = "ノデ"},
            {.original_string = "、", .string = "、"},
            {.original_string = "友達", .string = "友達", .reading = "トモダチ"},
            {.original_string = "と", .string = "と", .reading = "ト"},
            {.original_string = "一緒", .string = "一緒", .reading = "イッショ"},
            {.original_string = "に", .string = "に", .reading = "ニ"},
            {.original_string = "よく", .string = "よく", .reading = "ヨク"},
            {.original_string = "来", .string = "来", .reading = "キ"},
            {.original_string = "ます", .string = "ます", .reading = "マス"},
            {.original_string = "。", .string = "。"},
    };
    pv_normalizer_token_list_t *full_token_list = NULL;
    pv_status_t status = pv_normalizer_token_list_init(&full_token_list);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "token_list_init failed with %s",
            pv_status_to_string(status));
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    pv_normalizer_tokenizer_ja_stream_t *stream = NULL;
    status = pv_normalizer_tokenizer_ja_stream_open(tokenizer, &stream);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "stream open failed with %s",
            pv_status_to_string(status));
    if (status != PV_STATUS_SUCCESS) {
        pv_normalizer_token_list_delete(full_token_list);
        return;
    }

    for (int32_t i = 0; i < PV_ARRAY_LEN(input_tokens); i++) {
        pv_normalizer_token_list_t *stream_token_list = NULL;
        status = pv_normalizer_tokenizer_ja_stream_tokenize(
                stream,
                input_tokens[i],
                false,
                false,
                &stream_token_list);
        pv_test_true(
                status == PV_STATUS_SUCCESS,
                "stream tokenize failed with %s",
                pv_status_to_string(status));
        if (status != PV_STATUS_SUCCESS) {
            pv_normalizer_token_list_delete(full_token_list);
            pv_normalizer_tokenizer_ja_stream_close(stream);
            return;
        }

        if (stream_token_list != NULL) {
            pv_normalizer_token_list_append_list(full_token_list, stream_token_list);
        }
    }

    pv_normalizer_token_list_t *flush_token_list = NULL;
    status = pv_normalizer_tokenizer_ja_stream_flush(
            stream,
            false,
            false,
            &flush_token_list);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "stream flush failed with %s",
            pv_status_to_string(status));
    if (status != PV_STATUS_SUCCESS) {
        pv_normalizer_token_list_delete(full_token_list);
        pv_normalizer_tokenizer_ja_stream_close(stream);
        return;
    }

    if (flush_token_list != NULL) {
        pv_normalizer_token_list_append_list(full_token_list, flush_token_list);
    }

    pv_normalizer_token_t *current = full_token_list->head;
    for (int32_t i = 0; i < PV_ARRAY_LEN(expected_tokens); i++) {
        pv_test_true(
                current,
                "tokenize only returned %d tokens, %d tokens expected",
                i + 1,
                PV_ARRAY_LEN(expected_tokens));
        if (!current) {
            return;
        }

        pv_test_true(
                strcmp(current->string, expected_tokens[i].string) == 0,
                "incorrect string: got `%s`, expected `%s`",
                current->string,
                expected_tokens[i].string);
        pv_test_true(
                strcmp(current->original_string, expected_tokens[i].original_string) == 0,
                "incorrect original_string: got `%s`, expected `%s`",
                current->original_string,
                expected_tokens[i].original_string);
        if (expected_tokens[i].reading != NULL) {
            pv_test_true(
                    strcmp(current->reading, expected_tokens[i].reading) == 0,
                    "incorrect reading: got `%s`, expected `%s`",
                    current->reading,
                    expected_tokens[i].reading);
        }
        current = current->next;
    }

    pv_normalizer_token_list_delete(full_token_list);
    pv_normalizer_tokenizer_ja_stream_close(stream);
}

static void test_pv_normalizer_tokenizer_init_tokenize_mecab_parse_empty(void) {
    pv_normalizer_token_list_t *token_list = NULL;
    pv_status_t status = pv_normalizer_tokenizer_ja_tokenize(
            tokenizer,
            " ",
            false,
            false,
            true,
            &token_list);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "expected tokenize to succeed, got `%s`",
            pv_status_to_string(status));
    pv_normalizer_token_list_delete(token_list);
}

#ifdef __PV_MOCKS__

static void test_pv_normalizer_tokenizer_init_calloc_fail(void) {
    PV_SET_MOCK_RETURN_VAL(calloc, NULL)

    const void *shadow = tokenizer_data;
    pv_normalizer_tokenizer_ja_t *t = NULL;
    pv_status_t status = pv_normalizer_tokenizer_ja_init(
            PV_NORMALIZER_LANGUAGE_JA,
            language_info_object,
            &shadow,
            &t);
    reset_mocks();
    pv_test_true(
            status == PV_STATUS_OUT_OF_MEMORY,
            "expected init to fail with `%s`, got `%s`",
            pv_status_to_string(PV_STATUS_OUT_OF_MEMORY),
            pv_status_to_string(status));
    pv_test_error_message(
            "Failed to allocate, out of memory\\.",
            "Failed to allocate memory for `o`\\.",
            false,
            "error message mismatch");
}

static void test_pv_normalizer_tokenizer_init_mecab_fail(void) {
    char *c = {0};
    pv_normalizer_tokenizer_ja_t *t = NULL;
    pv_status_t status = pv_normalizer_tokenizer_ja_init(
            PV_NORMALIZER_LANGUAGE_JA,
            language_info_object,
            (const void **) &c,
            &t);
    pv_test_true(
            status == PV_STATUS_IO_ERROR,
            "expected init to fail with `%s`, got `%s`",
            pv_status_to_string(PV_STATUS_IO_ERROR),
            pv_status_to_string(status));
    pv_test_error_message(
            "Picovoice Error",
            "`mecab_new_from_buffer` failed.",
            false,
            "error message mismatch");
}

static void test_pv_normalizer_tokenizer_init_tokenizer_generic_fail(void) {
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_tokenizer_generic_init, PV_STATUS_RUNTIME_ERROR)

    const void *shadow = tokenizer_data;
    pv_normalizer_tokenizer_ja_t *t = NULL;
    pv_status_t status = pv_normalizer_tokenizer_ja_init(
            PV_NORMALIZER_LANGUAGE_JA,
            language_info_object,
            &shadow,
            &t);
    pv_test_true(
            status == PV_STATUS_RUNTIME_ERROR,
            "expected init to fail with `%s`, got `%s`",
            pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
            pv_status_to_string(status));
    pv_test_error_message(
            pv_test_function_hash_regex(),
        "`pv_normalizer_tokenizer_generic_init` failed with status `RUNTIME_ERROR`.",
            false,
            "error message mismatch");
}

static void test_pv_normalizer_tokenizer_init_tokenize_validate_fail(void) {
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_util_validate_text, PV_STATUS_RUNTIME_ERROR)

    pv_normalizer_token_list_t *token_list = NULL;
    pv_status_t status = pv_normalizer_tokenizer_ja_tokenize(
            tokenizer,
            "日本語が少し話せます。",
            false,
            false,
            true,
            &token_list);
    pv_test_true(
            status == PV_STATUS_RUNTIME_ERROR,
            "expected tokenize to fail with `%s`, got `%s`",
            pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
            pv_status_to_string(status));
    pv_test_error_message(
            pv_test_function_hash_regex(),
            "`pv_normalizer_util_validate_text` failed with status `RUNTIME_ERROR`.",
            false,
            "error message mismatch");
}

static void test_pv_normalizer_tokenizer_tokenize_token_list_init_fail(void) {
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_token_list_init, PV_STATUS_RUNTIME_ERROR)

    pv_normalizer_token_list_t *token_list = NULL;
    pv_status_t status = pv_normalizer_tokenizer_ja_tokenize(
            tokenizer,
            "日本語が少し話せます。",
            false,
            false,
            true,
            &token_list);
    pv_test_true(
            status == PV_STATUS_RUNTIME_ERROR,
            "expected tokenize to fail with `%s`, got `%s`",
            pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
            pv_status_to_string(status));
    pv_test_error_message(
            pv_test_function_hash_regex(),
            "`pv_normalizer_token_list_init` failed with status `RUNTIME_ERROR`.",
            false,
            "error message mismatch");
}

static void test_pv_normalizer_tokenizer_tokenize_num_bytes_fail(void) {
    PV_SET_MOCK_RETURN_VAL(pv_language_num_bytes_character, PV_STATUS_INVALID_ARGUMENT)

    pv_normalizer_token_list_t *token_list = NULL;
    pv_status_t status = pv_normalizer_tokenizer_ja_tokenize(
            tokenizer,
            "日本語が少し話せます。",
            false,
            false,
            true,
            &token_list);
    pv_test_true(
            status == PV_STATUS_INVALID_ARGUMENT,
            "expected tokenize to fail with `%s`, got `%s`",
            pv_status_to_string(PV_STATUS_INVALID_ARGUMENT),
            pv_status_to_string(status));
    pv_test_error_message(
            "Argument `text` is invalid.",
            NULL,
            false,
            "error message mismatch");
}

static void test_pv_normalizer_tokenizer_tokenize_is_punctuation_fail(void) {
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_util_is_punctuation, PV_STATUS_RUNTIME_ERROR)

    pv_normalizer_token_list_t *token_list = NULL;
    pv_status_t status = pv_normalizer_tokenizer_ja_tokenize(
            tokenizer,
            "日本語が少し話せます。",
            false,
            false,
            true,
            &token_list);
    pv_test_true(
            status == PV_STATUS_RUNTIME_ERROR,
            "expected tokenize to fail with `%s`, got `%s`",
            pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
            pv_status_to_string(status));
    pv_test_error_message(
            pv_test_function_hash_regex(),
            "`pv_normalizer_util_is_punctuation` failed with status `RUNTIME_ERROR`.",
            false,
            "error message mismatch");
}

static void test_pv_normalizer_tokenizer_tokenize_generic_tokenizer_fail(void) {
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_tokenizer_generic_tokenize, PV_STATUS_RUNTIME_ERROR)

    pv_normalizer_token_list_t *token_list = NULL;
    pv_status_t status = pv_normalizer_tokenizer_ja_tokenize(
            tokenizer,
            "1 2 3",
            false,
            false,
            true,
            &token_list);
    pv_test_true(
            status == PV_STATUS_RUNTIME_ERROR,
            "expected tokenize to fail with `%s`, got `%s`",
            pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
            pv_status_to_string(status));
    pv_test_error_message(
            pv_test_function_hash_regex(),
            "`pv_normalizer_tokenizer_generic_tokenize` failed with status `RUNTIME_ERROR`.",
            false,
            "error message mismatch");
}

static void test_pv_normalizer_tokenizer_tokenize_normalize_full_width_fail(void) {
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_util_ja_normalize_full_width_text, PV_STATUS_RUNTIME_ERROR)

    pv_normalizer_token_list_t *token_list = NULL;
    pv_status_t status = pv_normalizer_tokenizer_ja_tokenize(
            tokenizer,
            "1 2 3",
            false,
            false,
            true,
            &token_list);
    pv_test_true(
            status == PV_STATUS_RUNTIME_ERROR,
            "expected tokenize to fail with `%s`, got `%s`",
            pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
            pv_status_to_string(status));
    pv_test_error_message(
            pv_test_function_hash_regex(),
            "`pv_normalizer_util_ja_normalize_full_width_text` failed with status `RUNTIME_ERROR`.",
            false,
            "error message mismatch");
}

static void test_pv_normalizer_tokenizer_tokenize_token_init_fail(void) {
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_token_init_with_original_string, PV_STATUS_RUNTIME_ERROR)

    pv_normalizer_token_list_t *token_list = NULL;
    pv_status_t status = pv_normalizer_tokenizer_ja_tokenize(
            tokenizer,
            "日本語が少し話せます。",
            false,
            false,
            true,
            &token_list);
    pv_test_true(
            status == PV_STATUS_RUNTIME_ERROR,
            "expected tokenize to fail with `%s`, got `%s`",
            pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
            pv_status_to_string(status));
    pv_test_error_message(
            pv_test_function_hash_regex(),
            "`pv_normalizer_token_init_with_original_string` failed with status `RUNTIME_ERROR`.",
            false,
            "error message mismatch");
}

static void test_pv_normalizer_tokenizer_tokenize_on_character_fail(void) {
    PV_SET_MOCK_RETURN_VAL(pv_normalizer_tokenizer_generic_tokenize, PV_STATUS_RUNTIME_ERROR)

    pv_normalizer_token_list_t *token_list = NULL;
    pv_status_t status = pv_normalizer_tokenizer_ja_tokenize_on_character(
            tokenizer,
            "1-2",
            '-',
            false,
            false,
            false,
            &token_list);
    pv_test_true(
            status == PV_STATUS_RUNTIME_ERROR,
            "expected tokenize to fail with `%s`, got `%s`",
            pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
            pv_status_to_string(status));
    pv_test_error_message(
            pv_test_function_hash_regex(),
            "`pv_normalizer_tokenizer_generic_tokenize` failed with status `RUNTIME_ERROR`.",
            false,
            "error message mismatch");
}

#endif

static const pv_test_case_t PV_NORMALIZER_TOKENIZER_JA_TEST_CASES[] = {
        {"basic", test_pv_normalizer_tokenizer_ja_basic},
        {"no tokens", test_pv_normalizer_tokenizer_ja_no_tokens},
        {"custom pronunciation", test_pv_normalizer_tokenizer_ja_custom_pronunciation},
        {"alphanum", test_pv_normalizer_tokenizer_ja_alphanum},
        {"tokenize on character", test_pv_normalizer_tokenizer_ja_tokenize_on_character},
        {"basic streaming", test_pv_normalizer_tokenizer_ja_basic_streaming},
        {"tokenize mecab parse empty", test_pv_normalizer_tokenizer_init_tokenize_mecab_parse_empty},

#ifdef __PV_MOCKS__

        {"calloc fail", test_pv_normalizer_tokenizer_init_calloc_fail},
        {"mecab fail", test_pv_normalizer_tokenizer_init_mecab_fail},
        {"tokenizer generic fail", test_pv_normalizer_tokenizer_init_tokenizer_generic_fail},
        {"tokenize validate fail", test_pv_normalizer_tokenizer_init_tokenize_validate_fail},
        {"tokenize token list init fail", test_pv_normalizer_tokenizer_tokenize_token_list_init_fail},
        {"tokenize num bytes fail", test_pv_normalizer_tokenizer_tokenize_num_bytes_fail},
        {"tokenize is punctuation fail", test_pv_normalizer_tokenizer_tokenize_is_punctuation_fail},
        {"tokenize generic tokenizer fail", test_pv_normalizer_tokenizer_tokenize_generic_tokenizer_fail},
        {"tokenize normalize full width fail", test_pv_normalizer_tokenizer_tokenize_normalize_full_width_fail},
        {"tokenize token init fail", test_pv_normalizer_tokenizer_tokenize_token_init_fail},
        {"tokenize on character fail", test_pv_normalizer_tokenizer_tokenize_on_character_fail},

#endif

};

const pv_test_suite_t PV_NORMALIZER_TOKENIZER_JA_TEST_SUITE = {
        .name = "normalizer_tokenizer_ja",
        .setup = test_pv_normalizer_tokenizer_ja_setup,
        .teardown = test_pv_normalizer_tokenizer_ja_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_NORMALIZER_TOKENIZER_JA_TEST_CASES),
        .test_cases = PV_NORMALIZER_TOKENIZER_JA_TEST_CASES,
};
