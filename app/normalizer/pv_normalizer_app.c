#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "core/pv_error.h"
#include "core/pv_language_json.h"
#include "lm/pv_noun_gender_dict.h"
#include "orca/normalizer/pv_normalizer.h"
#include "util/pv_file.h"

void usage(const char *program) {
    (void) fprintf(
            stderr,
            "usage: %s "
            "-t INPUT_TEXT "
            "-i LANGUAGE_INFO_PATH "
            "[-d (ENABLE_DETAILED_OUTPUT)] [-n NOUN_GENDER_DICT_PATH] [-b TOKENIZER_DATA]\n",
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

int main(int argc, char *argv[]) {
    pv_error_prepare();

    const char *text = NULL;
    const char *language_info_path = NULL;
    const char *noun_gender_dict_path = NULL;
    const char *tokenizer_data_path = NULL;
    bool details = false;

    const char *SHORT_OPTIONS = "t:i:n:b:d";

    int opt;
    while ((opt = getopt(argc, argv, SHORT_OPTIONS)) != -1) {
        switch (opt) {
            case 't':
                text = optarg;
                break;
            case 'i':
                language_info_path = optarg;
                break;
            case 'n':
                noun_gender_dict_path = optarg;
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

    if (!(text && language_info_path)) {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    char **message_stack = NULL;
    int32_t message_stack_depth = 0;

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
            (void) fprintf(stderr, "Could not load noun gender dict: `%s`", pv_status_to_string(status));
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
        (void) fprintf(stderr, "Failed to initialize normalizer with `%s`", pv_status_to_string(status));
        handle_error(message_stack, message_stack_depth);
        exit(EXIT_FAILURE);
    }

    printf("Input text: `%s`\n", text);

    char *normalized = NULL;
    pv_normalizer_token_list_t *token_list = NULL;
    if (details) {
        status = pv_normalizer_normalize(
                normalizer_object,
                text,
                true,
                false,
                NULL,
                &token_list);
    } else {
        status = pv_normalizer_normalize(
                normalizer_object,
                text,
                true,
                false,
                &normalized,
                NULL);
    }
    if (status != PV_STATUS_SUCCESS) {
        (void) fprintf(stderr, "Failed to normalize text with `%s`", pv_status_to_string(status));
        handle_error(message_stack, message_stack_depth);
        exit(EXIT_FAILURE);
    }

    if ((token_list != NULL) && (token_list->size > 0)) {
        printf("Tokens =>\n");
        pv_normalizer_token_t *current = token_list->head;
        int32_t i = 0;
        while (current) {
            printf(
                    "Token %d: string=`%s`, original=`%s`, verbalized=`%s`, tag=`%d`",
                    i,
                    current->string,
                    current->original_string,
                    current->verbalized,
                    current->tag);
            if (current->reading != NULL) {
                printf(", reading=`%s`", current->reading);
            }
            if (current->tag_language_agnostic == PV_NORMALIZER_TAG_CUSTOM_PRONUNCIATION) {
                printf(", pronunciation=`%s`\n", current->pronunciation);
            } else {
                printf("\n");
            }
            current = current->next;
            i++;
        }
    }

    if (normalized != NULL) {
        printf("Normalized text: `%s`\n", normalized);
    }

    pv_noun_gender_dict_delete(noun_gender_dict_object);
    noun_gender_dict_object = NULL;
    pv_normalizer_delete(normalizer_object);
    normalizer_object = NULL;
    pv_language_info_delete(language_info_object);
    language_info_object = NULL;
    free(normalized);
    pv_normalizer_token_list_delete(token_list);

    return EXIT_SUCCESS;
}
