#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/pv_language_json.h"
#include "hippo/pv_hippo_internal.h"
#include "io/pv_log.h"
#include "model/pv_offline_token_classifier.h"
#include "orca/pv_orca.h"
#include "orca/pv_orca_internal.h"
#include "orca/pv_orca_synthesizer.h"

extern const pv_orca_phonemizer_param_t PV_ORCA_PHONEMIZER_PARAM;
extern const pv_orca_synthesizer_param_t PV_ORCA_SYNTHESIZER_PARAM;
extern const pv_offline_token_classifier_param_t PV_HIPPO_CLASSIFIER_PARAM;
extern const uint8_t PV_DICT[];
extern const int32_t PV_DICT_LENGTH;
extern const uint8_t PV_NOUN_GENDER_DICT[];
extern const int32_t PV_NOUN_GENDER_DICT_LENGTH;
extern const uint8_t PV_LEXICON[];
extern const int32_t PV_LEXICON_LENGTH;
extern const uint8_t PV_HETERONYM_TREE[];
extern const int32_t PV_HETERONYM_TREE_LENGTH;

static const char *HIPPO_TMP_PATH = "hippo.tmp";
static const int32_t PV_ORCA_MAXIMUM_CHARACTER_LIMIT = 2000;

void usage(const char *program) {
    (void) fprintf(
            stderr,
            "usage: %s -h LANGUAGE_INFO_HIPPO -l LANGUAGE_INFO_NORMALIZER -o OUTPUT_PATH (-t ONLY_TEST_OUTPUTS -d TOKENIZER_DATA) \n",
            program);
}

int main(int argc, char *argv[]) {

    const char *language_info_hippo_path = NULL;
    const char *language_info_normalizer_path = NULL;
    const char *output_path = NULL;
    const char *tokenizer_data_path = NULL;
    bool only_test_helpers = false;

    const char *SHORT_OPTIONS = "h:l:o:t:d:";

    int opt;
    while ((opt = getopt(argc, argv, SHORT_OPTIONS)) != -1) {
        switch (opt) {
            case 'h':
                language_info_hippo_path = optarg;
                break;
            case 'l':
                language_info_normalizer_path = optarg;
                break;
            case 'o':
                output_path = optarg;
                break;
            case 't':
                only_test_helpers = atoi(optarg) == 1;
                break;
            case 'd':
                tokenizer_data_path = optarg;
                break;
            default:
                break;
        }
    }

    if (!(language_info_hippo_path && language_info_normalizer_path && output_path)) {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    if (!only_test_helpers) {
        pv_status_t status = pv_orca_internal_param_serialize(
                &PV_ORCA_PHONEMIZER_PARAM,
                &PV_ORCA_SYNTHESIZER_PARAM,
                output_path);
        if (status != PV_STATUS_SUCCESS) {
            LOG_ERROR("orca param serialization failed with `%s`", pv_status_to_string(status));
            exit(EXIT_FAILURE);
        }
    }

    pv_status_t status = pv_offline_token_classifier_param_serialize(
            &PV_HIPPO_CLASSIFIER_PARAM,
            PV_HIPPO_MAGIC_TOKEN,
            PV_HIPPO_VERSION,
            666,
            HIPPO_TMP_PATH);
    if (status != PV_STATUS_SUCCESS) {
        pv_remove(output_path);
        LOG_ERROR("hippo param serialization failed with '%s'", pv_status_to_string(status));
        exit(EXIT_FAILURE);
    }

    int32_t num_content_bytes = 0;
    void *content = NULL;
    status = pv_file_load(HIPPO_TMP_PATH, &num_content_bytes, &content);
    pv_remove(HIPPO_TMP_PATH);
    if (status != PV_STATUS_SUCCESS) {
        pv_remove(output_path);
        LOG_ERROR("failed with '%s'", pv_status_to_string(status));
        exit(EXIT_FAILURE);
    }

    const char *open_mode = only_test_helpers ? "wb" : "ab";

    FILE *f = fopen(output_path, open_mode);
    if (!f) {
        free(content);
        pv_remove(output_path);
        LOG_ERROR("failed with '%s'", pv_status_to_string(PV_STATUS_IO_ERROR));
        exit(EXIT_FAILURE);
    }

    if (!only_test_helpers) {
        size_t num_write = fwrite(&PV_ORCA_MAXIMUM_CHARACTER_LIMIT, sizeof(int32_t), 1, f);
        if (num_write != (size_t) 1) {
            (void) fclose(f);
            pv_remove(output_path);
            LOG_ERROR("failed with '%s'", pv_status_to_string(PV_STATUS_IO_ERROR));
            exit(EXIT_FAILURE);
        }
    }

    pv_language_info_t *language_info_normalizer = NULL;
    status = pv_language_info_load_json(language_info_normalizer_path, &language_info_normalizer, true, true);
    if (status != PV_STATUS_SUCCESS) {
        LOG_ERROR("failed to load language info with '%s'", pv_status_to_string(status));
        exit(EXIT_FAILURE);
    }

    status = pv_serialized_section_pack(pv_language_info_serialized_context(), language_info_normalizer, f);
    pv_language_info_delete(language_info_normalizer);
    if (status != PV_STATUS_SUCCESS) {
        LOG_ERROR("failed to write language info to '%s'", output_path);
        (void) fclose(f);
        exit(EXIT_FAILURE);
    }

    size_t num_write = fwrite(content, 1, num_content_bytes, f);
    free(content);
    if (num_write != (size_t) num_content_bytes) {
        (void) fclose(f);
        pv_remove(output_path);
        LOG_ERROR("failed with '%s'", pv_status_to_string(PV_STATUS_IO_ERROR));
        exit(EXIT_FAILURE);
    }

    pv_language_info_t *language_info_hippo = NULL;
    status = pv_language_info_load_json(language_info_hippo_path, &language_info_hippo, true, true);
    if (status != PV_STATUS_SUCCESS) {
        LOG_ERROR("failed to load language info with '%s'", pv_status_to_string(status));
        exit(EXIT_FAILURE);
    }

    status = pv_serialized_section_pack(pv_language_info_serialized_context(), language_info_hippo, f);
    pv_language_info_delete(language_info_hippo);
    if (status != PV_STATUS_SUCCESS) {
        LOG_ERROR("failed to write language info to '%s'", output_path);
        (void) fclose(f);
        exit(EXIT_FAILURE);
    }

    num_write = fwrite(PV_LEXICON, sizeof(uint8_t), PV_LEXICON_LENGTH, f);
    if (num_write != (size_t) PV_LEXICON_LENGTH) {
        (void) fclose(f);
        pv_remove(output_path);
        LOG_ERROR("failed with '%s'", pv_status_to_string(PV_STATUS_IO_ERROR));
        exit(EXIT_FAILURE);
    }

    num_write = fwrite(PV_DICT, sizeof(uint8_t), PV_DICT_LENGTH, f);
    if (num_write != (size_t) PV_DICT_LENGTH) {
        (void) fclose(f);
        pv_remove(output_path);
        LOG_ERROR("failed with '%s'", pv_status_to_string(PV_STATUS_IO_ERROR));
        exit(EXIT_FAILURE);
    }

    num_write = fwrite(PV_HETERONYM_TREE, sizeof(uint8_t), PV_HETERONYM_TREE_LENGTH, f);
    if (num_write != (size_t) PV_HETERONYM_TREE_LENGTH) {
        (void) fclose(f);
        pv_remove(output_path);
        LOG_ERROR("failed with '%s'", pv_status_to_string(PV_STATUS_IO_ERROR));
        exit(EXIT_FAILURE);
    }

    num_write = fwrite(PV_NOUN_GENDER_DICT, sizeof(uint8_t), PV_NOUN_GENDER_DICT_LENGTH, f);
    if (num_write != (size_t) PV_NOUN_GENDER_DICT_LENGTH) {
        (void) fclose(f);
        pv_remove(output_path);
        LOG_ERROR("failed with '%s'", pv_status_to_string(PV_STATUS_IO_ERROR));
        exit(EXIT_FAILURE);
    }

    if (tokenizer_data_path != NULL) {
        FILE *f_tokenizer_data = fopen(tokenizer_data_path, "rb");

        char buffer[1024];
        size_t bytes_read;

        while ((bytes_read = fread(buffer, 1, sizeof(buffer), f_tokenizer_data)) > 0) {
            if (fwrite(buffer, 1, bytes_read, f) != bytes_read) {
                (void) fclose(f);
                pv_remove(output_path);
                (void) fclose(f_tokenizer_data);
                LOG_ERROR("failed with `%s`", pv_status_to_string(PV_STATUS_IO_ERROR));
                exit(EXIT_FAILURE);
            }
        }

        (void) fclose(f_tokenizer_data);
    }

    (void) fclose(f);

    return EXIT_SUCCESS;
}
