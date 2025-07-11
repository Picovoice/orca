#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/pv_error.h"
#include "core/pv_language_internal.h"
#include "core/pv_language_json.h"
#include "orca/normalizer/test_pv_normalizer_cases_helper.h"
#include "util/pv_file.h"

#define MIN(x, y) (((x) < (y)) ? (x) : (y))

void usage(const char *program) {
    (void) fprintf(stderr,
                   "usage: %s "
                   "-t TEXT_CASES_PATH -i LANGUAGE_INFO_PATH "
                   "[-n NOUN_GENDER_DICT_PATH -s TOKENIZER_BIN_PATH -b TOKENIZER_DATA] [-d]\n",
                   program);
}

void handle_error(char **message_stack, int32_t message_stack_depth) {
    pv_status_t error_status = pv_get_error_stack(&message_stack, &message_stack_depth);

    if (error_status != PV_STATUS_SUCCESS) {
        fprintf(stderr, ".\nUnable to get Normalizer error state with '%s'\n", pv_status_to_string(error_status));
        exit(EXIT_FAILURE);
    }

    if (message_stack_depth > 0) {
        fprintf(stderr, ":\n");
        for (int32_t i = 0; i < message_stack_depth; i++) {
            fprintf(stderr, "  [%d] %s\n", i, message_stack[i]);
        }
    }

    pv_free_error_stack(message_stack);
}

void handle_error_nice(char **message_stack, int32_t message_stack_depth) {
    pv_status_t error_status = pv_get_error_stack(&message_stack, &message_stack_depth);

    if (error_status != PV_STATUS_SUCCESS) {
        printf(".\nUnable to get Normalizer error state with '%s'\n", pv_status_to_string(error_status));
        exit(EXIT_FAILURE);
    }

    if (message_stack_depth > 0) {
        printf(":\n");
        for (int32_t i = 0; i < message_stack_depth; i++) {
            printf("  [%d] %s\n", i, message_stack[i]);
        }
    }

    pv_free_error_stack(message_stack);
}

void print_diff_detailed(
        int32_t case_no,
        const char *text_raw,
        const char *text_truth,
        const char *text_normalized_batch,
        bool print_stream,
        const char *text_normalized_stream,
        pv_status_t status) {
    printf("Case no `%d`:", case_no);
    printf("bad\n");
    printf("raw:          \t`%s`\n", text_raw);
    printf("truth:        \t`%s`\n", text_truth);
    printf("norm (batch): \t`%s`\n", text_normalized_batch);
    if (print_stream) {
        printf("norm (stream):\t`%s`\n", text_normalized_stream);
    }
    if (status != PV_STATUS_SUCCESS) {
        char **message_stack = NULL;
        int32_t message_stack_depth = 0;

        printf("Failed to normalize text with `%s`\n", pv_status_to_string(status));
        handle_error_nice(message_stack, message_stack_depth);
    }

    printf("diff:         \t ");
    int32_t min_str_len = (text_truth == NULL || text_normalized_batch == NULL) ? 0 : MIN(strlen(text_truth), strlen(text_normalized_batch)) + 1;
    int32_t i = 0;
    while (i < min_str_len) {
        bool equal = true;
        int32_t num_bytes = 0;
        pv_language_num_bytes_character(text_truth[i], &num_bytes);

        for (int32_t j = 0; j < num_bytes; j++) {
            if (text_truth[i + j] != text_normalized_batch[i + j]) {
                equal = false;
            }
        }

        if (!equal) {
            printf("^");
        } else {
            printf(" ");
        }

        i += num_bytes;
    }
    printf("\n");

    printf("\n");
}

int main(int argc, char *argv[]) {
    pv_error_prepare();

    const char *text_cases_path = NULL;
    const char *language_info_path = NULL;
    const char *noun_gender_dict_path = NULL;
    const char *tokenizer_data_path = NULL;

    const char *tokenizer_bin_path = NULL;
    bool details = false;

    const char *SHORT_OPTIONS = "t:i:l:n:s:b:d";

    int opt;
    while ((opt = getopt(argc, argv, SHORT_OPTIONS)) != -1) {
        switch (opt) {
            case 't':
                text_cases_path = optarg;
                break;
            case 'i':
                language_info_path = optarg;
                break;
            case 'n':
                noun_gender_dict_path = optarg;
                break;
            case 's':
                tokenizer_bin_path = optarg;
                break;
            case 'b':
                tokenizer_data_path = optarg;
                break;
            case 'd':
                details = true;
                break;
            default:
                break;
        }
    }

    if (!(text_cases_path && language_info_path)) {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    char **message_stack = NULL;
    int32_t message_stack_depth = 0;

    pv_picollm_tokenizer_t *stream_tokenizer = NULL;
    if (tokenizer_bin_path != NULL) {
        FILE *f = pv_fopen(tokenizer_bin_path, "rb");
        if (!f) {
            (void) fprintf(stderr, "Could not load stream tokenizer file: `%s`", tokenizer_bin_path);
            exit(EXIT_FAILURE);
        }

        pv_status_t status = pv_picollm_tokenizer_init(f, &stream_tokenizer);
        (void) fclose(f);
        if (status != PV_STATUS_SUCCESS) {
            (void) fprintf(stderr, "Could not load stream tokenizer: `%s`", pv_status_to_string(status));
            handle_error(message_stack, message_stack_depth);
            exit(EXIT_FAILURE);
        }
    }

    pv_language_info_t *language_info_object = NULL;
    pv_status_t status = pv_language_info_load_json(language_info_path, &language_info_object, true, true);
    if (status != PV_STATUS_SUCCESS) {
        (void) fprintf(stderr, "Could not load language info: `%s`", pv_status_to_string(status));
        handle_error(message_stack, message_stack_depth);
        exit(EXIT_FAILURE);
    }

    pv_noun_gender_dict_t *noun_gender_dict_object = NULL;
    if (noun_gender_dict_path != NULL) {
        status = pv_noun_gender_dict_init(noun_gender_dict_path, &noun_gender_dict_object);
        if (status != PV_STATUS_SUCCESS) {
            (void) fprintf(stderr, "Could not load noun gender dictionary: `%s`", pv_status_to_string(status));
            handle_error(message_stack, message_stack_depth);
            exit(EXIT_FAILURE);
        }
    }

    void *tokenizer_data = NULL;
    if (tokenizer_data_path) {
        int32_t num_content_bytes = 0;
        void *content = NULL;
        status = pv_file_load(tokenizer_data_path, &num_content_bytes, &content);
        if (status != PV_STATUS_SUCCESS) {
            fprintf(stderr, "failed to load tokenizer data with `%s`", pv_status_to_string(status));
            exit(EXIT_FAILURE);
        }
        tokenizer_data = content;
    }
    const void *shadow = tokenizer_data;

    static pv_normalizer_t *normalizer_object = NULL;
    status = pv_normalizer_init(
            language_info_object,
            noun_gender_dict_object,
            tokenizer_data != NULL ? &shadow : NULL,
            &normalizer_object);
    free(tokenizer_data);
    if (status != PV_STATUS_SUCCESS) {
        (void) fprintf(stderr, "Failed to initalize normalizer with `%s`", pv_status_to_string(status));
        handle_error(message_stack, message_stack_depth);
        exit(EXIT_FAILURE);
    }

    pv_normalizer_cases_helper_t *text_cases_helper = NULL;
    status = pv_normalizer_cases_helper_init(text_cases_path, &text_cases_helper);
    if (status != PV_STATUS_SUCCESS) {
        (void) fprintf(stderr, "Failed to open test cases file `%s`", text_cases_path);
        handle_error(message_stack, message_stack_depth);
        exit(EXIT_FAILURE);
    }

    int32_t total_cases = 0;
    int32_t total_passes = 0;

    int32_t case_no = 0;
    char text_raw[1024] = {0};
    char text_batch[1024] = {0};
    char text_stream[1024] = {0};

    while (pv_normalizer_cases_helper_next_case(text_cases_helper, &case_no, text_raw, PV_ARRAY_LEN(text_raw), text_batch, PV_ARRAY_LEN(text_batch), text_stream, PV_ARRAY_LEN(text_batch))) {
        if (pv_normalizer_cases_is_ignored(case_no, language_info_object->language_code)) {
            if (details) {
                printf("Case no `%d`:", case_no);
                printf("ignored\n");
                printf("raw:      \t`%s`\n", text_raw);
                printf("batch:    \t`%s`\n", text_batch);
                printf("stream:    \t`%s`\n", text_stream);
            }
            case_no = 0;
            memset(text_raw, 0, PV_ARRAY_LEN(text_raw));
            memset(text_batch, 0, PV_ARRAY_LEN(text_batch));
            memset(text_stream, 0, PV_ARRAY_LEN(text_stream));
            continue;
        }

        total_cases++;

        char *text_normalized_batch = NULL;
        char *text_normalized_stream = NULL;

        pv_status_t status_stream = PV_STATUS_SUCCESS;
        pv_status_t status_batch = pv_normalizer_cases_normalize_batch(
                normalizer_object,
                text_raw,
                &text_normalized_batch);
        if (stream_tokenizer != NULL) {
            status_stream = pv_normalizer_cases_normalize_stream(
                    stream_tokenizer,
                    normalizer_object,
                    strcmp(language_info_object->language_code, "ja") != 0,
                    text_raw,
                    &text_normalized_stream);
        }
        if (status_batch != PV_STATUS_SUCCESS) {
            print_diff_detailed(
                    case_no,
                    text_raw,
                    text_batch,
                    text_normalized_batch,
                    (stream_tokenizer != NULL),
                    text_normalized_stream,
                    status_batch);
        } else if (status_stream != PV_STATUS_SUCCESS) {
            print_diff_detailed(
                    case_no,
                    text_raw,
                    text_batch,
                    text_normalized_batch,
                    (stream_tokenizer != NULL),
                    text_normalized_stream,
                    status_stream);
        } else if (text_normalized_batch && strcmp(text_batch, text_normalized_batch) != 0) {
            print_diff_detailed(
                    case_no,
                    text_raw,
                    text_batch,
                    text_normalized_batch,
                    (stream_tokenizer != NULL),
                    text_normalized_stream,
                    status_batch);
        } else if (text_normalized_stream && strcmp(text_stream, text_normalized_stream) != 0) {
            print_diff_detailed(
                    case_no,
                    text_raw,
                    text_stream,
                    text_normalized_batch,
                    (stream_tokenizer != NULL),
                    text_normalized_stream,
                    status_stream);
        } else {
            if (details) {
                printf("Case no `%d`:", case_no);
                printf("✅\n");
            }
            total_passes++;
        }

        free(text_normalized_batch);
        free(text_normalized_stream);

        case_no = 0;
        memset(text_raw, 0, PV_ARRAY_LEN(text_raw));
        memset(text_batch, 0, PV_ARRAY_LEN(text_batch));
        memset(text_stream, 0, PV_ARRAY_LEN(text_stream));
    }

    printf("\nPassed `%d` out of `%d` cases (%f%%)\n", total_passes, total_cases, (float) total_passes / (float) total_cases * 100);

    pv_normalizer_cases_helper_delete(text_cases_helper);
    text_cases_helper = NULL;

    pv_picollm_tokenizer_delete(stream_tokenizer);
    stream_tokenizer = NULL;

    pv_normalizer_delete(normalizer_object);
    normalizer_object = NULL;

    pv_noun_gender_dict_delete(noun_gender_dict_object);
    noun_gender_dict_object = NULL;

    pv_language_info_delete(language_info_object);
    language_info_object = NULL;

    return EXIT_SUCCESS;
}
