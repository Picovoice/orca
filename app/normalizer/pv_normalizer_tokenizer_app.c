#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include "core/pv_error.h"
#include "core/pv_language_json.h"
#include "util/pv_file.h"
#include "orca/normalizer/pv_normalizer_token.h"
#include "orca/normalizer/pv_normalizer_tokenizer.h"

void usage(const char *program) {
    (void) fprintf(
            stderr,
            "usage: "
            "%s "
            "-i INPUT_TEXT "
            "-l LANGUAGE_INFO_PATH "
            "(-p PRESERVE_WORD_BOUNDARY -r REMOVE_UNKNOWN_CHARACTERS -t TOKENIZER_DATA)\n", program);
}

void handle_error(char **message_stack, int32_t message_stack_depth) {
    pv_status_t error_status = pv_get_error_stack(&message_stack, &message_stack_depth);

    if (error_status != PV_STATUS_SUCCESS) {
        fprintf(stderr, ".\nUnable to get tokenizer error state with '%s'\n", pv_status_to_string(error_status));
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
    bool preserve_word_boundary = false;
    bool remove_unknown_characters = false;
    const char *tokenizer_data_path = NULL;
    const char *SHORT_OPTIONS = "i:l:p:r:t:";

    int opt;
    while ((opt = getopt(argc, argv, SHORT_OPTIONS)) != -1) {
        switch (opt) {
            case 'i':
                text = optarg;
                break;
            case 'l':
                language_info_path = optarg;
                break;
            case 'p':
                preserve_word_boundary = (atoi(optarg) == 1);
                break;
            case 'r':
                remove_unknown_characters = (atoi(optarg) == 1);
                break;
            case 't':
                tokenizer_data_path = optarg;
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

    pv_normalizer_tokenizer_t *tokenizer_object = NULL;
    status = pv_normalizer_tokenizer_init(
            language_info_object,
            tokenizer_data != NULL ? &shadow : NULL,
            &tokenizer_object);
    free(tokenizer_data);
    if (status != PV_STATUS_SUCCESS) {
        (void) fprintf(stderr, "Failed to initialize tokenizer with `%s`", pv_status_to_string(status));
        handle_error(message_stack, message_stack_depth);
        exit(EXIT_FAILURE);
    }

    bool split_on_special_tokens = false;
    pv_normalizer_token_list_t *token_list = NULL;
    status = pv_normalizer_tokenizer_tokenize(
            tokenizer_object,
            text,
            pv_normalizer_tokenizer_default_word_boundary_character(tokenizer_object),
            preserve_word_boundary,
            remove_unknown_characters,
            split_on_special_tokens,
            true,
            &token_list);
    if (status != PV_STATUS_SUCCESS) {
        (void) fprintf(stderr, "Failed to tokenize text with `%s`", pv_status_to_string(status));
        handle_error(message_stack, message_stack_depth);
        exit(EXIT_FAILURE);
    }
    if (!token_list) {
        printf("No valid tokens found in input text\n");
        pv_normalizer_token_list_delete(token_list);
        pv_normalizer_tokenizer_delete(tokenizer_object);
        pv_language_info_delete(language_info_object);
        return EXIT_SUCCESS;
    }

    printf("Input text: `%s`\n", text);

    int32_t i = 0;
    pv_normalizer_token_t *current = token_list->head;
    printf("Tokens =>\n");
    while (current) {
        printf(
                "Token %d: string=`%s`, original=`%s`",
                i,
                current->string,
                current->original_string);
        if (current->tag_language_agnostic == PV_NORMALIZER_TAG_CUSTOM_PRONUNCIATION) {
            printf(", pronunciation=`%s`\n", current->pronunciation);
        } else {
            printf("\n");
        }
        current = current->next;
        i++;
    }

    pv_normalizer_token_list_delete(token_list);
    pv_normalizer_tokenizer_delete(tokenizer_object);
    pv_language_info_delete(language_info_object);

    return EXIT_SUCCESS;
}
