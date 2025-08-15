#include <stdlib.h>
#include <string.h>

#include "core/picovoice.h"
#include "core/pv_language.h"
#include "core/pv_language_internal.h"
#include "core/pv_language_json.h"
#include "io/pv_log.h"
#include "mock/pv_mock.h"
#include "orca/normalizer/pv_normalizer.h"
#include "orca/normalizer/test_pv_normalizer_cases_helper.h"
#include "test/pv_test.h"
#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "core/mock/pv_core_runtime_mock.h"
#include "orca/mock/pv_normalizer_mock.h"
#include "tokenizer/mock/pv_tokenizer_mock.h"
#include "util/mock/pv_file_mock.h"

#endif

const char *TOKENIZER_PATHS[] = {
        "normalizer/tokenizers/tokenizer-gemma-2b-372.bin",
        "normalizer/tokenizers/tokenizer-llama-2-13b-267.bin",
        "normalizer/tokenizers/tokenizer-mistral-7b-instruct-v0.1-225.bin",
};

static pv_status_t test_pv_normalizer_cases_normalize_helper_init(
        const char *language_info_filename,
        const char *noun_gender_dict_filename,
        const char *tokenizer_data_filename,
        const char *test_cases_filename,
        pv_language_info_t **language_info,
        pv_noun_gender_dict_t **noun_gender_dict,
        pv_normalizer_t **normalizer,
        pv_normalizer_cases_helper_t **text_cases_helper) {

    char *language_info_path = pv_test_module_res_path(language_info_filename);
    pv_test_true(language_info_path, "Failed to get path for file: `%s`", language_info_filename);
    if (!language_info_path) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_language_info_t *language_info_internal = NULL;
    pv_status_t status = pv_language_info_load_json(language_info_path, &language_info_internal, true, true);
    free(language_info_path);
    pv_test_true(status == PV_STATUS_SUCCESS, "Failed to load language info from path: `%s`", language_info_path);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    char *noun_gender_dict_path = pv_test_module_res_path(noun_gender_dict_filename);
    pv_test_true(noun_gender_dict_path, "Failed to get path for file: `%s`", noun_gender_dict_filename);
    if (!noun_gender_dict_path) {
        pv_language_info_delete(language_info_internal);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_noun_gender_dict_t *noun_gender_dict_internal = NULL;
    status = pv_noun_gender_dict_init(noun_gender_dict_path, &noun_gender_dict_internal);
    free(noun_gender_dict_path);
    pv_test_true(status == PV_STATUS_SUCCESS, "Failed to load noun gender dict from path: `%s`", noun_gender_dict_path);
    if (status != PV_STATUS_SUCCESS) {
        pv_language_info_delete(language_info_internal);
        return status;
    }

    void *tokenizer_data = NULL;
    if (tokenizer_data_filename) {
        char *tokenizer_data_path = pv_test_module_res_path(tokenizer_data_filename);
        pv_test_true(tokenizer_data_path, "Failed to get path for file: `%s`", tokenizer_data_filename);
        if (!tokenizer_data_path) {
            pv_noun_gender_dict_delete(noun_gender_dict_internal);
            pv_language_info_delete(language_info_internal);
            return PV_STATUS_OUT_OF_MEMORY;
        }

        int32_t num_content_bytes = 0;
        status = pv_file_load(tokenizer_data_path, &num_content_bytes, &tokenizer_data);
        free(tokenizer_data_path);
        pv_test_true(status == PV_STATUS_SUCCESS, "Failed to load tokenizer data from path: `%s`", tokenizer_data_path);
        if (status != PV_STATUS_SUCCESS) {
            pv_noun_gender_dict_delete(noun_gender_dict_internal);
            pv_language_info_delete(language_info_internal);
            return status;
        }
    }
    const void *shadow = tokenizer_data;

    pv_normalizer_t *normalizer_internal = NULL;
    status = pv_normalizer_init(
            language_info_internal,
            noun_gender_dict_internal,
            shadow != NULL ? &shadow : NULL,
            &normalizer_internal);
    free(tokenizer_data);
    pv_test_true(status == PV_STATUS_SUCCESS, "Failed to initialize normalizer");
    if (status != PV_STATUS_SUCCESS) {
        pv_noun_gender_dict_delete(noun_gender_dict_internal);
        pv_language_info_delete(language_info_internal);
        return status;
    }

    char *text_cases_path = pv_test_module_res_path(test_cases_filename);
    pv_test_true(text_cases_path, "Failed to get path for file: `%s`", test_cases_filename);
    if (!text_cases_path) {
        pv_normalizer_delete(normalizer_internal);
        pv_noun_gender_dict_delete(noun_gender_dict_internal);
        pv_language_info_delete(language_info_internal);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_normalizer_cases_helper_t *text_cases_helper_internal = NULL;
    status = pv_normalizer_cases_helper_init(text_cases_path, &text_cases_helper_internal);
    free(text_cases_path);
    pv_test_true(status == PV_STATUS_SUCCESS, "Failed to load test cases file `%s`", text_cases_path);
    if (status != PV_STATUS_SUCCESS) {
        pv_normalizer_delete(normalizer_internal);
        pv_noun_gender_dict_delete(noun_gender_dict_internal);
        pv_language_info_delete(language_info_internal);
        return status;
    }

    *language_info = language_info_internal;
    *noun_gender_dict = noun_gender_dict_internal;
    *normalizer = normalizer_internal;
    *text_cases_helper = text_cases_helper_internal;
    return PV_STATUS_SUCCESS;
}

static void test_pv_normalizer_cases_normalize_batch_helper(
        const char *language_info_filename,
        const char *noun_gender_dict_filename,
        const char *tokenizer_data_filename,
        const char *test_cases_filename,
        float threshold) {

    pv_language_info_t *language_info = NULL;
    pv_noun_gender_dict_t *noun_gender_dict = NULL;
    pv_normalizer_t *normalizer = NULL;
    pv_normalizer_cases_helper_t *text_cases_helper = NULL;
    pv_status_t status = test_pv_normalizer_cases_normalize_helper_init(
            language_info_filename,
            noun_gender_dict_filename,
            tokenizer_data_filename,
            test_cases_filename,
            &language_info,
            &noun_gender_dict,
            &normalizer,
            &text_cases_helper);
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    int32_t total_cases = 0;
    int32_t total_passes = 0;

    int32_t case_no = 0;
    char text_raw[1024] = {0};
    char text_batch[1024] = {0};

    while (pv_normalizer_cases_helper_next_case(
            text_cases_helper,
            &case_no,
            text_raw,
            PV_ARRAY_LEN(text_raw),
            text_batch,
            PV_ARRAY_LEN(text_batch),
            NULL,
            0)) {
        if (pv_normalizer_cases_is_ignored(case_no, language_info->language_code)) {
            case_no = 0;
            memset(text_raw, 0, PV_ARRAY_LEN(text_raw));
            memset(text_batch, 0, PV_ARRAY_LEN(text_batch));
            continue;
        }

        total_cases++;

        char *text_normalized_batch = NULL;

        status = pv_normalizer_cases_normalize_batch(
                normalizer,
                text_raw,
                &text_normalized_batch);

        if (status == PV_STATUS_SUCCESS &&
            text_normalized_batch &&
            strcmp(text_batch, text_normalized_batch) == 0) {
            total_passes++;
        }

        free(text_normalized_batch);

        case_no = 0;
        memset(text_raw, 0, PV_ARRAY_LEN(text_raw));
        memset(text_batch, 0, PV_ARRAY_LEN(text_batch));
    }

    const float pass_ratio = (float) total_passes / (float) total_cases;
    pv_test_true(pass_ratio >= threshold, "Did not pass enough test cases (got `%f` expected >= `%f`", pass_ratio, threshold);

    pv_normalizer_cases_helper_delete(text_cases_helper);
    pv_normalizer_delete(normalizer);
    pv_noun_gender_dict_delete(noun_gender_dict);
    pv_language_info_delete(language_info);
}

static void test_pv_normalizer_cases_normalize_stream_helper(
        const char *language_info_filename,
        const char *noun_gender_dict_filename,
        const char *tokenizer_data_filename,
        const char *test_cases_filename,
        bool add_spaces,
        float threshold) {

    pv_language_info_t *language_info = NULL;
    pv_noun_gender_dict_t *noun_gender_dict = NULL;
    pv_normalizer_t *normalizer = NULL;
    pv_normalizer_cases_helper_t *text_cases_helper = NULL;
    pv_status_t status = test_pv_normalizer_cases_normalize_helper_init(
            language_info_filename,
            noun_gender_dict_filename,
            tokenizer_data_filename,
            test_cases_filename,
            &language_info,
            &noun_gender_dict,
            &normalizer,
            &text_cases_helper);
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    for (int32_t i = 0; i < PV_ARRAY_LEN(TOKENIZER_PATHS); i++) {
        const char *tokenizer_bin_filename = TOKENIZER_PATHS[i];
        LOG_INFO("    -> testing with tokenizer: `%s`", tokenizer_bin_filename);

        pv_normalizer_cases_helper_reindex(text_cases_helper);

        char *tokenizer_bin_path = pv_test_module_res_path(tokenizer_bin_filename);
        pv_test_true(tokenizer_bin_path, "Failed to get path for file: `%s`", tokenizer_bin_filename);
        if (!tokenizer_bin_path) {
            pv_normalizer_cases_helper_delete(text_cases_helper);
            pv_normalizer_delete(normalizer);
            pv_noun_gender_dict_delete(noun_gender_dict);
            pv_language_info_delete(language_info);
            return;
        }

        FILE *f_tokenizer = pv_fopen(tokenizer_bin_path, "rb");
        free(tokenizer_bin_path);
        pv_test_true(f_tokenizer, "Failed to load tokenizer file `%s`", tokenizer_bin_path);
        if (!f_tokenizer) {
            pv_normalizer_cases_helper_delete(text_cases_helper);
            pv_normalizer_delete(normalizer);
            pv_noun_gender_dict_delete(noun_gender_dict);
            pv_language_info_delete(language_info);
            return;
        }

        pv_tokenizer_t *tokenizer = NULL;
        status = pv_tokenizer_init(f_tokenizer, &tokenizer);
        (void) fclose(f_tokenizer);
        pv_test_true(status == PV_STATUS_SUCCESS, "Failed to load tokenizer: `%s`", pv_status_to_string(status));
        if (status != PV_STATUS_SUCCESS) {
            pv_normalizer_cases_helper_delete(text_cases_helper);
            pv_normalizer_delete(normalizer);
            pv_noun_gender_dict_delete(noun_gender_dict);
            pv_language_info_delete(language_info);
            return;
        }

        int32_t total_cases = 0;
        int32_t total_passes = 0;

        int32_t case_no = 0;
        char text_raw[1024] = {0};
        char text_batch[1024] = {0};
        char text_stream[1024] = {0};

        while (pv_normalizer_cases_helper_next_case(
                text_cases_helper,
                &case_no,
                text_raw,
                PV_ARRAY_LEN(text_raw),
                text_batch,
                PV_ARRAY_LEN(text_batch),
                text_stream,
                PV_ARRAY_LEN(text_stream))) {
            if (pv_normalizer_cases_is_ignored(case_no, language_info->language_code)) {
                case_no = 0;
                memset(text_raw, 0, PV_ARRAY_LEN(text_raw));
                memset(text_batch, 0, PV_ARRAY_LEN(text_batch));
                memset(text_stream, 0, PV_ARRAY_LEN(text_stream));
                continue;
            }

            total_cases++;

            char *text_normalized_stream = NULL;

            status = pv_normalizer_cases_normalize_stream(
                    tokenizer,
                    normalizer,
                    add_spaces,
                    text_raw,
                    &text_normalized_stream);

            if ((status == PV_STATUS_SUCCESS) &&
                (text_normalized_stream) &&
                (strcmp(text_stream, text_normalized_stream) == 0)) {
                total_passes++;
            }

            free(text_normalized_stream);

            case_no = 0;
            memset(text_raw, 0, PV_ARRAY_LEN(text_raw));
            memset(text_batch, 0, PV_ARRAY_LEN(text_batch));
            memset(text_stream, 0, PV_ARRAY_LEN(text_stream));
        }

        const float pass_ratio = (float) total_passes / (float) total_cases;
        pv_test_true(pass_ratio >= threshold, "Did not pass enough test cases (got `%f` expected >= `%f`", pass_ratio, threshold);
        pv_tokenizer_delete(tokenizer);
    }

    pv_normalizer_cases_helper_delete(text_cases_helper);
    pv_normalizer_delete(normalizer);
    pv_noun_gender_dict_delete(noun_gender_dict);
    pv_language_info_delete(language_info);
}

static void test_pv_normalizer_cases_normalize_batch_en(void) {
    test_pv_normalizer_cases_normalize_batch_helper(
            "language_info/pv_language_info_orca_normalizer_en.json",
            "noun_gender_dict/empty.txt",
            NULL,
            "normalizer/test_cases/en/sentences.csv",
            1.0f);
}

static void test_pv_normalizer_cases_normalize_stream_en(void) {
    test_pv_normalizer_cases_normalize_stream_helper(
            "language_info/pv_language_info_orca_normalizer_en.json",
            "noun_gender_dict/empty.txt",
            NULL,
            "normalizer/test_cases/en/sentences.csv",
            true,
            1.0f);
}

static void test_pv_normalizer_cases_normalize_batch_de(void) {
    test_pv_normalizer_cases_normalize_batch_helper(
            "language_info/pv_language_info_orca_normalizer_de.json",
            "noun_gender_dict/noun_gender_dict_de.txt",
            NULL,
            "normalizer/test_cases/de/sentences.csv",
            1.0f);
}

static void test_pv_normalizer_cases_normalize_stream_de(void) {
    test_pv_normalizer_cases_normalize_stream_helper(
            "language_info/pv_language_info_orca_normalizer_de.json",
            "noun_gender_dict/noun_gender_dict_de.txt",
            NULL,
            "normalizer/test_cases/de/sentences.csv",
            true,
            1.0f);
}

static void test_pv_normalizer_cases_normalize_batch_es(void) {
    test_pv_normalizer_cases_normalize_batch_helper(
            "language_info/pv_language_info_orca_normalizer_es.json",
            "noun_gender_dict/noun_gender_dict_es.txt",
            NULL,
            "normalizer/test_cases/es/sentences.csv",
            1.0f);
}

static void test_pv_normalizer_cases_normalize_stream_es(void) {
    test_pv_normalizer_cases_normalize_stream_helper(
            "language_info/pv_language_info_orca_normalizer_es.json",
            "noun_gender_dict/noun_gender_dict_es.txt",
            NULL,
            "normalizer/test_cases/es/sentences.csv",
            true,
            1.0f);
}

static void test_pv_normalizer_cases_normalize_batch_fr(void) {
    test_pv_normalizer_cases_normalize_batch_helper(
            "language_info/pv_language_info_orca_normalizer_fr.json",
            "noun_gender_dict/noun_gender_dict_fr.txt",
            NULL,
            "normalizer/test_cases/fr/sentences.csv",
            1.0f);
}

static void test_pv_normalizer_cases_normalize_stream_fr(void) {
    test_pv_normalizer_cases_normalize_stream_helper(
            "language_info/pv_language_info_orca_normalizer_fr.json",
            "noun_gender_dict/noun_gender_dict_fr.txt",
            NULL,
            "normalizer/test_cases/fr/sentences.csv",
            true,
            1.0f);
}

static void test_pv_normalizer_cases_normalize_batch_it(void) {
    test_pv_normalizer_cases_normalize_batch_helper(
            "language_info/pv_language_info_orca_normalizer_it.json",
            "noun_gender_dict/noun_gender_dict_it.txt",
            NULL,
            "normalizer/test_cases/it/sentences.csv",
            1.0f);
}

static void test_pv_normalizer_cases_normalize_stream_it(void) {
    test_pv_normalizer_cases_normalize_stream_helper(
            "language_info/pv_language_info_orca_normalizer_it.json",
            "noun_gender_dict/noun_gender_dict_it.txt",
            NULL,
            "normalizer/test_cases/it/sentences.csv",
            true,
            1.0f);
}

static void test_pv_normalizer_cases_normalize_batch_ko(void) {
    test_pv_normalizer_cases_normalize_batch_helper(
            "language_info/pv_language_info_orca_normalizer_ko.json",
            "noun_gender_dict/empty.txt",
            NULL,
            "normalizer/test_cases/ko/sentences.csv",
            1.0f);
}

static void test_pv_normalizer_cases_normalize_stream_ko(void) {
    test_pv_normalizer_cases_normalize_stream_helper(
            "language_info/pv_language_info_orca_normalizer_ko.json",
            "noun_gender_dict/empty.txt",
            NULL,
            "normalizer/test_cases/ko/sentences.csv",
            true,
            1.0f);
}

static void test_pv_normalizer_cases_normalize_batch_pt(void) {
    test_pv_normalizer_cases_normalize_batch_helper(
            "language_info/pv_language_info_orca_normalizer_pt.json",
            "noun_gender_dict/noun_gender_dict_pt.txt",
            NULL,
            "normalizer/test_cases/pt/sentences.csv",
            1.0f);
}

static void test_pv_normalizer_cases_normalize_stream_pt(void) {
    test_pv_normalizer_cases_normalize_stream_helper(
            "language_info/pv_language_info_orca_normalizer_pt.json",
            "noun_gender_dict/noun_gender_dict_pt.txt",
            NULL,
            "normalizer/test_cases/pt/sentences.csv",
            true,
            1.0f);
}

static void test_pv_normalizer_cases_normalize_batch_ja(void) {
    test_pv_normalizer_cases_normalize_batch_helper(
            "language_info/pv_language_info_orca_normalizer_ja.json",
            "noun_gender_dict/empty.txt",
            "normalizer/tokenizer_data/ipadic.bin",
            "normalizer/test_cases/ja/sentences.csv",
            1.0f);
}

static void test_pv_normalizer_cases_normalize_stream_ja(void) {
    test_pv_normalizer_cases_normalize_stream_helper(
            "language_info/pv_language_info_orca_normalizer_ja.json",
            "noun_gender_dict/empty.txt",
            "normalizer/tokenizer_data/ipadic.bin",
            "normalizer/test_cases/ja/sentences.csv",
            false,
            1.0f);
}

static const pv_test_case_t PV_NORMALIZER_CASES_TEST_CASES[] = {
        {"normalize batch en", test_pv_normalizer_cases_normalize_batch_en},
        {"normalize stream en", test_pv_normalizer_cases_normalize_stream_en},
        {"normalize batch de", test_pv_normalizer_cases_normalize_batch_de},
        {"normalize stream de", test_pv_normalizer_cases_normalize_stream_de},
        {"normalize batch es", test_pv_normalizer_cases_normalize_batch_es},
        {"normalize stream es", test_pv_normalizer_cases_normalize_stream_es},
        {"normalize batch fr", test_pv_normalizer_cases_normalize_batch_fr},
        {"normalize stream fr", test_pv_normalizer_cases_normalize_stream_fr},
        {"normalize_batch_it", test_pv_normalizer_cases_normalize_batch_it},
        {"normalize_stream_it", test_pv_normalizer_cases_normalize_stream_it},
        {"normalize batch ko", test_pv_normalizer_cases_normalize_batch_ko},
        {"normalize stream ko", test_pv_normalizer_cases_normalize_stream_ko},
        {"normalize batch pt", test_pv_normalizer_cases_normalize_batch_pt},
        {"normalize stream pt", test_pv_normalizer_cases_normalize_stream_pt},
        {"normalize batch ja", test_pv_normalizer_cases_normalize_batch_ja},
        {"normalize stream ja", test_pv_normalizer_cases_normalize_stream_ja},
};

const pv_test_suite_t PV_NORMALIZER_CASES_TEST_SUITE = {
        .name = "normalizer_cases",
        .setup = NULL,
        .teardown = NULL,
        .num_test_cases = PV_ARRAY_LEN(PV_NORMALIZER_CASES_TEST_CASES),
        .test_cases = PV_NORMALIZER_CASES_TEST_CASES,
};
