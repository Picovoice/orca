#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../../gatekeeper/test/test_pv_gatekeeper_usage_helper.h"
#include "core/pv_error_messages.h"
#include "gatekeeper/pv_gatekeeper_usage_animal.h"
#include "normalizer/pv_normalizer_util.h"
#include "orca/pv_orca.h"
#include "orca/pv_orca_internal.h"
#include "orca/pv_orca_metric_internal.h"
#include "orca/pv_orca_stream_state.h"
#include "test/pv_test.h"
#include "tokenizer/pv_tokenizer.h"

#if !defined(__PV_TARGET_NO_FILE_SYSTEM__) && !defined(__PV_TARGET_PLATFORM_WASM__)

#include "audio/pv_audio_file.h"
#include "audio/pv_container_wav.h"

#endif

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"
#include "tokenizer/mock/pv_tokenizer_mock.h"

#endif

#define NUM_TOKENIZERS 3

static const float ORCA_INTELLIGIBILITY_THRESHOLD = 0.65f;

static pv_ypu_t *ypu = NULL;
static pv_tokenizer_t *TOKENIZERS[NUM_TOKENIZERS] = {NULL};

const char *TOKENIZER_PATH_ARRAY[NUM_TOKENIZERS] = {
        "tokenizer/tokenizer-gemma-2b-372.bin",
        "tokenizer/tokenizer-llama-2-13b-267.bin",
        "tokenizer/tokenizer-mistral-7b-instruct-v0.1-225.bin",
};

typedef struct pcm_chunk_node pcm_chunk_node_t;

struct pcm_chunk_node {
    int32_t num_samples;
    int16_t *pcm;
    pcm_chunk_node_t *next;
};

static pv_status_t pcm_chunk_init(
        int32_t num_samples,
        int16_t *pcm,
        pcm_chunk_node_t **chunk) {
    PV_ASSERT(pcm);
    PV_ASSERT(chunk);

    *chunk = NULL;

    pcm_chunk_node_t *c = (pcm_chunk_node_t *) calloc(1, sizeof(pcm_chunk_node_t));
    if (!c) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    c->pcm = pcm;
    c->num_samples = num_samples;
    c->next = NULL;

    *chunk = c;

    return PV_STATUS_SUCCESS;
}

static pv_status_t pcm_chunk_delete(pcm_chunk_node_t *chunk) {
    if (chunk) {
        pv_orca_pcm_delete(chunk->pcm);
        free(chunk);
    }
    return PV_STATUS_SUCCESS;
}

static pv_status_t test_pv_orca_setup_helper(
        const char *param_name,
        pv_orca_t **orca_object,
        pv_orca_synthesize_params_t **synthesize_params_object) {
    PV_ASSERT(param_name);
    PV_ASSERT(orca_object);
    PV_ASSERT(synthesize_params_object);

    char *model_path = pv_test_module_res_path(param_name);
    if (!model_path) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    char *access_key = NULL;
    pv_status_t status = pv_access_serialize(&BYPASS_ACCESS, &access_key);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_https_client_factory_t *factory = NULL;
    status = get_https_client_factory_usage_success(&factory);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_orca_internal_init(
            access_key,
            factory,
            model_path,
            pv_ypu_clone(ypu),
            orca_object);
    free(access_key);
    free(model_path);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_orca_synthesize_params_init(synthesize_params_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return status;
}

static pv_status_t test_pv_orca_metric_classifier_setup_helper(
        const char *metric_classifier_path,
        const pv_orca_t *orca_object,
        pv_orca_metric_t **metric) {
    PV_ASSERT(metric_classifier_path);
    PV_ASSERT(orca_object);
    PV_ASSERT(metric);

    char *metric_classifier_path_resolved = pv_test_module_res_path(metric_classifier_path);
    pv_test_true(
            metric_classifier_path_resolved != NULL,
            "failed to open file with `%s`",
            pv_status_to_string(PV_STATUS_OUT_OF_MEMORY));
    if (!metric_classifier_path_resolved) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    int32_t sample_rate = 0;
    pv_status_t status = pv_orca_sample_rate(orca_object, &sample_rate);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "failed to get orca sample rate; got failure status `%s`",
            pv_status_to_string(status));
    if (status != PV_STATUS_SUCCESS) {
        free(metric_classifier_path_resolved);
        return status;
    }

    status = pv_orca_metric_init(
            ypu,
            metric_classifier_path_resolved,
            sample_rate,
            metric);
    free(metric_classifier_path_resolved);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "metric init error, expected `%s` got `%s`",
            pv_status_to_string(PV_STATUS_SUCCESS),
            pv_status_to_string(status));
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static pv_status_t test_pv_orca_integration_setup(void) {
    pv_status_t status = pv_ypu_init_cpu(1, &ypu);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    for (int32_t i = 0; i < PV_ARRAY_LEN(TOKENIZER_PATH_ARRAY); ++i) {
        const char *tokenizer_bin_filename = TOKENIZER_PATH_ARRAY[i];

        char *tokenizer_bin_path = pv_test_shared_res_path(tokenizer_bin_filename);

        FILE *f_tokenizer = pv_fopen(tokenizer_bin_path, "rb");
        pv_test_true(f_tokenizer, "Failed to load tokenizer file `%s`", tokenizer_bin_path);
        free(tokenizer_bin_path);
        if (!f_tokenizer) {
            return PV_STATUS_IO_ERROR;
        }

        status = pv_tokenizer_init(f_tokenizer, &(TOKENIZERS[i]));
        (void) fclose(f_tokenizer);
        pv_test_true(status == PV_STATUS_SUCCESS, "Failed to load tokenizer: `%s`",
                     pv_status_to_string(status));
        if (status != PV_STATUS_SUCCESS) {
            return status;
        }
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_orca_integration_teardown(void) {
    for (int32_t i = 0; i < PV_ARRAY_LEN(TOKENIZER_PATH_ARRAY); ++i) {
        pv_tokenizer_delete(TOKENIZERS[i]);
    }

    pv_ypu_delete(ypu);
}

typedef struct pv_orca_sentences_helper {
    uint8_t *buffer;
    int32_t buffer_length;

    int32_t idx;
} pv_orca_sentences_helper_t;

static pv_status_t pv_orca_sentences_helper_init(const char *file_path, pv_orca_sentences_helper_t **object) {
    PV_ASSERT(file_path);
    PV_ASSERT(object);

    *object = NULL;

    pv_orca_sentences_helper_t *o = (pv_orca_sentences_helper_t *) calloc(1, sizeof(pv_orca_sentences_helper_t));
    if (!o) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    FILE *f = pv_fopen(file_path, "rb");
    if (!f) {
        return PV_STATUS_IO_ERROR;
    }

    fseek(f, 0, SEEK_END);
    o->buffer_length = (int32_t) ftell(f);

    o->buffer = (uint8_t *) malloc(o->buffer_length * sizeof(uint8_t));
    if (!(o->buffer)) {
        (void) fclose(f);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    fseek(f, 0, SEEK_SET);

    const int32_t bytes_read = (int32_t) fread(o->buffer, sizeof(uint8_t), o->buffer_length, f);
    (void) fclose(f);
    if (bytes_read != o->buffer_length) {
        return PV_STATUS_IO_ERROR;
    }

    o->idx = 0;

    *object = o;

    return PV_STATUS_SUCCESS;
}

static void pv_orca_sentences_helper_delete(pv_orca_sentences_helper_t *object) {
    if (object) {
        free(object->buffer);
        free(object);
    }
}

static void pv_orca_sentences_helper_read_int(pv_orca_sentences_helper_t *object, int32_t *number) {
    PV_ASSERT(object);
    PV_ASSERT(number);

    *number = 0;

    int32_t idx = 0;
    char number_buffer[256] = {0};
    while (object->idx < object->buffer_length && idx < PV_ARRAY_LEN(number_buffer)) {
        char c = (char) object->buffer[(object->idx)++];
        if (c == ',') {
            break;
        }
        number_buffer[idx++] = c;
    }

    *number = atoi(number_buffer);
}

static void pv_orca_sentences_helper_read_float(pv_orca_sentences_helper_t *object, float *number) {
    PV_ASSERT(object);
    PV_ASSERT(number);

    *number = 0;

    int32_t idx = 0;
    char number_buffer[256] = {0};
    while (object->idx < object->buffer_length && idx < PV_ARRAY_LEN(number_buffer)) {
        char c = (char) object->buffer[(object->idx)++];
        if (c == ',') {
            break;
        }
        number_buffer[idx++] = c;
    }

    *number = strtof(number_buffer, NULL);
}

static void pv_orca_sentences_helper_read_to_char(pv_orca_sentences_helper_t *object, char character) {
    PV_ASSERT(object);

    while (object->idx < object->buffer_length) {
        char c = (char) object->buffer[(object->idx)++];
        if (c == character) {
            break;
        }
    }
}

static void pv_orca_sentences_helper_read_to_eol(pv_orca_sentences_helper_t *object) {
    PV_ASSERT(object);

    while (object->idx < object->buffer_length) {
        char c = (char) object->buffer[(object->idx)++];

        if (c == '\n') {
            break;
        } else if (c == '\r') {
            char next_char = (char) object->buffer[(object->idx)++];
            if (next_char == '\n') {
                break;
            }
        }
    }
}

static bool pv_orca_sentences_helper_read_string(pv_orca_sentences_helper_t *object, char *text, int32_t text_length) {
    PV_ASSERT(object);
    PV_ASSERT(text);
    PV_ASSERT(text_length >= 0);

    int32_t idx = 0;
    while (object->idx < object->buffer_length && idx < text_length) {
        char c = (char) object->buffer[(object->idx)++];
        if (c == '"') {
            char ci = (char) object->buffer[(object->idx)++];
            if (object->idx >= object->buffer_length) {
                return false;
            }
            if (ci != '"') {
                (object->idx)--;
                break;
            }
        } else if (c == '\r') {
            char ci = (char) object->buffer[(object->idx)++];
            if (ci != '\n') {
                (object->idx)--;
            }
            c = ci;
        }
        text[idx++] = c;
    }

    return true;
}

static bool pv_orca_sentences_helper_next_case(
        pv_orca_sentences_helper_t *object,
        int32_t *case_no,
        char *text_raw,
        int32_t text_raw_length,
        char *text_truth_original_phonemes,
        int32_t text_truth_original_phonemes_length,
        char *text_truth_spoken_phonemes,
        int32_t text_truth_spoken_phonemes_length,
        float *threshold_worst_case,
        float *threshold_average_case,
        char *text_true_words,
        int32_t text_true_words_length) {
    PV_ASSERT(object);
    PV_ASSERT(case_no);
    PV_ASSERT(text_raw);
    PV_ASSERT(text_raw_length >= 0);
    PV_ASSERT(text_truth_original_phonemes);
    PV_ASSERT(text_truth_original_phonemes_length >= 0);
    PV_ASSERT(text_truth_spoken_phonemes);
    PV_ASSERT(text_truth_spoken_phonemes_length >= 0);
    PV_ASSERT(threshold_worst_case);
    PV_ASSERT(threshold_average_case);
    PV_ASSERT(text_true_words);
    PV_ASSERT(text_true_words_length >= 0);

    if (object->idx >= object->buffer_length) {
        return false;
    }

    pv_orca_sentences_helper_read_int(object, case_no);

    pv_orca_sentences_helper_read_to_char(object, '"');

    if (!pv_orca_sentences_helper_read_string(object, text_raw, text_raw_length)) {
        return false;
    }

    pv_orca_sentences_helper_read_to_char(object, '"');

    if (!pv_orca_sentences_helper_read_string(object, text_truth_original_phonemes, text_truth_original_phonemes_length)) {
        return false;
    }

    pv_orca_sentences_helper_read_to_char(object, '"');

    if (!pv_orca_sentences_helper_read_string(object, text_truth_spoken_phonemes, text_truth_spoken_phonemes_length)) {
        return false;
    }

    pv_orca_sentences_helper_read_to_char(object, ',');

    pv_orca_sentences_helper_read_float(object, threshold_worst_case);

    pv_orca_sentences_helper_read_float(object, threshold_average_case);

    pv_orca_sentences_helper_read_to_char(object, '"');

    if (!pv_orca_sentences_helper_read_string(object, text_true_words, text_true_words_length)) {
        return false;
    }

    pv_orca_sentences_helper_read_to_eol(object);

    return (object->idx <= object->buffer_length);
}

static void split_true_words_helper(char *text_true_words, const char ***true_words_list, int *true_words_list_count) {
    PV_ASSERT(text_true_words);
    PV_ASSERT(true_words_list);
    PV_ASSERT(true_words_list_count);

    *true_words_list = NULL;
    *true_words_list_count = 0;

    int32_t max_tokens = 1;
    for (int32_t i = 0; i < (int32_t) strlen(text_true_words); i++) {
        if (text_true_words[i] == ' ') {
            max_tokens++;
        }
    }

    *true_words_list = (const char **) calloc(max_tokens, sizeof(const char *));
    pv_test_true(*true_words_list != NULL, "Failed to allocate memory for true_words_list");

    const char *token = (const char *) strtok(text_true_words, " ");
    while (token != NULL) {
        (*true_words_list)[*true_words_list_count] = token;
        (*true_words_list_count)++;
        token = (const char *) strtok(NULL, " ");
    }
}

/*
- Running the PCM match between batch and stream over the integration text corpus or perhaps a (random?) subset of it if the runtime is a constraint.
- Closely follow "test_pv_orca.c" `test_pv_orca_synthesize()` function which compares the PCM of batch and stream for one test sentence.
*/
static pv_status_t test_pv_orca_integration_batch_stream_pcm_match_helper(
        pv_orca_t *orca_object,
        pv_orca_synthesize_params_t *synthesize_params_object,
        pv_orca_metric_t *metric,
        float stream_batch_length_diff_threshold,
        const char *text_raw,
        const char *text_truth_spoken_phonemes,
        float threshold_worst_case,
        float threshold_average_case,
        int32_t num_samples,
        int16_t *pcm,
        int32_t num_alignments,
        pv_orca_word_alignment_t **alignments) {
    PV_ASSERT(orca_object);
    PV_ASSERT(synthesize_params_object);
    PV_ASSERT(metric);
    PV_ASSERT(stream_batch_length_diff_threshold >= 0 && stream_batch_length_diff_threshold <= 1.0);
    PV_ASSERT(text_raw);
    PV_ASSERT(text_truth_spoken_phonemes);
    PV_ASSERT(threshold_worst_case >= 0 && threshold_worst_case <= 1.0);
    PV_ASSERT(threshold_average_case >= 0 && threshold_average_case <= 1.0);
    PV_ASSERT(num_samples >= 0);
    PV_ASSERT(pcm);
    PV_ASSERT(num_alignments >= 0);
    PV_ASSERT(alignments);

    LOG_INFO("        %s", "`test_pv_orca_integration_batch_stream_pcm_match_helper`");
    for (int32_t tokenizer_index = 0; tokenizer_index < PV_ARRAY_LEN(TOKENIZER_PATH_ARRAY); ++tokenizer_index) {
        pv_tokenizer_t *tokenizer_object = TOKENIZERS[tokenizer_index];
        LOG_INFO("            -> testing with tokenizer: `%s`", TOKENIZER_PATH_ARRAY[tokenizer_index]);

        // Sanity checks for word alignments and phoneme alignments:
        for (int32_t i = 0; i < num_alignments; ++i) {
            pv_orca_word_alignment_t *word = alignments[i];
            pv_test_true(word->start_sec >= 0, "invalid alignment start");
            pv_test_true(word->end_sec >= 0, "invalid alignment end");
            pv_test_true(word->num_phonemes > 0, "invalid number of phonemes");
            pv_test_true(word->start_sec <= word->end_sec, "invalid alignment start and end");

            for (int32_t j = 0; j < word->num_phonemes; ++j) {
                pv_orca_phoneme_alignment_t *phoneme = word->phonemes[j];

                pv_test_true(phoneme->start_sec >= 0, "invalid phoneme start");
                pv_test_true(phoneme->end_sec >= 0, "invalid phoneme end");
                pv_test_true(phoneme->start_sec <= phoneme->end_sec, "invalid phoneme start and end");
            }
        }

        int16_t *pcm_streaming = NULL;
        int32_t num_streaming_tokens_encoded = 0;
        int32_t *streaming_tokens_encoded = NULL;

        // tokenizer preprocess for streaming synthesize:
        pv_status_t status = pv_tokenizer_encode(
                tokenizer_object,
                text_raw,
                false,
                false,
                &num_streaming_tokens_encoded,
                &streaming_tokens_encoded);
        pv_test_true(
                status == PV_STATUS_SUCCESS,
                "failed to encode `text_raw` with tokenizer; expected `%s` got `%s`",
                pv_status_to_string(PV_STATUS_SUCCESS),
                pv_status_to_string(status));
        if (status != PV_STATUS_SUCCESS) {
            return status;
        }

        // Streaming synthesize:
        pcm_chunk_node_t *pcm_chunk_prev = NULL;
        pcm_chunk_node_t *pcm_chunk_head = NULL;

        pv_orca_stream_t *orca_stream = NULL;
        status = pv_orca_stream_open(orca_object, synthesize_params_object, &orca_stream);
        pv_test_true(status == PV_STATUS_SUCCESS, "failed to open stream");
        if (status != PV_STATUS_SUCCESS) {
            free(streaming_tokens_encoded);
            return status;
        }

        int32_t num_partial = 0;
        for (int32_t i = 0; i < num_streaming_tokens_encoded; ++i) {
            char *decoded_token = NULL;
            bool is_partial = false;
            status = pv_tokenizer_decode(
                    tokenizer_object,
                    streaming_tokens_encoded + i - num_partial,
                    1 + num_partial,
                    0,
                    &is_partial,
                    &decoded_token);

            if (is_partial) {
                num_partial += 1;
                free(decoded_token);
                continue;
            } else {
                num_partial = 0;
            }

            pv_test_true(
                    status == PV_STATUS_SUCCESS,
                    "failed to decode `streaming_tokens_encoded + i` with tokenizer; expected `%s` got `%s`",
                    pv_status_to_string(PV_STATUS_SUCCESS),
                    pv_status_to_string(status));
            if (status != PV_STATUS_SUCCESS) {
                free(streaming_tokens_encoded);
                pv_orca_stream_close(orca_stream);
                return status;
            }

            int32_t num_samples_chunk = 0;
            int16_t *pcm_chunk = NULL;
            status = pv_orca_stream_synthesize_internal(
                    orca_stream,
                    true,
                    false,
                    decoded_token,
                    true,
                    &num_samples_chunk,
                    &pcm_chunk);
            pv_test_true(status == PV_STATUS_SUCCESS, "failed to add `decoded_token`");
            free(decoded_token);
            if (status != PV_STATUS_SUCCESS) {
                free(streaming_tokens_encoded);
                pv_orca_stream_close(orca_stream);
                return status;
            }

            if (num_samples_chunk > 0) {
                if (pcm_chunk_prev == NULL) {
                    pcm_chunk_init(num_samples_chunk, pcm_chunk, &pcm_chunk_prev);
                    pcm_chunk_head = pcm_chunk_prev;
                } else {
                    pcm_chunk_init(num_samples_chunk, pcm_chunk, &pcm_chunk_prev->next);
                    pcm_chunk_prev = pcm_chunk_prev->next;
                }
            }
        }

        free(streaming_tokens_encoded);
        streaming_tokens_encoded = NULL;

        int32_t num_samples_chunk = 0;
        int16_t *pcm_chunk = NULL;
        status = pv_orca_stream_flush_internal(
                orca_stream,
                true,
                false,
                true,
                &num_samples_chunk,
                &pcm_chunk);
        pv_orca_stream_close(orca_stream);
        pv_test_true(status == PV_STATUS_SUCCESS, "failed to flush");
        if (status != PV_STATUS_SUCCESS) {
            return status;
        }

        if (num_samples_chunk > 0) {
            if (pcm_chunk_prev == NULL) {
                pcm_chunk_init(num_samples_chunk, pcm_chunk, &pcm_chunk_prev);
                pcm_chunk_head = pcm_chunk_prev;
            } else {
                pcm_chunk_init(num_samples_chunk, pcm_chunk, &pcm_chunk_prev->next);
                pcm_chunk_prev = pcm_chunk_prev->next;
            }
        }

        int32_t num_samples_streaming = 0;
        pcm_chunk_node_t *pcm_chunk_iter = pcm_chunk_head;
        while (pcm_chunk_iter != NULL) {
            num_samples_streaming += pcm_chunk_iter->num_samples;
            pcm_chunk_iter = pcm_chunk_iter->next;
        }

        pcm_streaming = (int16_t *) malloc(num_samples_streaming * sizeof(int16_t));
        pv_test_true(
                pcm_streaming != NULL,
                "Failed to allocate memory when malloc for `pcm_streaming`");
        if (!pcm_streaming) {
            pcm_chunk_iter = pcm_chunk_head;
            while (pcm_chunk_iter != NULL) {
                pcm_chunk_prev = pcm_chunk_iter;
                pcm_chunk_iter = pcm_chunk_iter->next;
                pcm_chunk_delete(pcm_chunk_prev);
            }
            return PV_STATUS_OUT_OF_MEMORY;
        }

        int32_t offset = 0;
        pcm_chunk_iter = pcm_chunk_head;
        while (pcm_chunk_iter != NULL) {
            memcpy(&pcm_streaming[offset], pcm_chunk_iter->pcm, pcm_chunk_iter->num_samples * sizeof(int16_t));
            offset += pcm_chunk_iter->num_samples;
            pcm_chunk_prev = pcm_chunk_iter;
            pcm_chunk_iter = pcm_chunk_iter->next;
            pcm_chunk_delete(pcm_chunk_prev);
        }

        int32_t num_samples_diff = abs(num_samples_streaming - num_samples);
        int32_t max_samples = ((num_samples_streaming > num_samples) ? num_samples_streaming : num_samples);
        float num_sample_diff_percentage = (float) num_samples_diff / (float) max_samples;
        pv_test_true(
                (num_sample_diff_percentage <= stream_batch_length_diff_threshold) && (num_sample_diff_percentage >= 0),
                "different number of samples with streaming synthesis, got `%d` expected `%d`",
                num_samples_streaming,
                num_samples);
        if ((num_sample_diff_percentage > stream_batch_length_diff_threshold) || (num_sample_diff_percentage < 0)) {
            pv_orca_pcm_delete(pcm_streaming);
            pcm_streaming = NULL;
            return PV_STATUS_SUCCESS;
        }

        // this test is only relevant for synthesized audio that is expected to have the same length
        if (stream_batch_length_diff_threshold == 0.0) {
            float threshold = 5000;
            int32_t num_differences_above_threshold = 0;
            for (int32_t k = 0; k < num_samples; ++k) {
                float sample_difference = fabsf((float) pcm_streaming[k] - (float) pcm[k]);
                if (sample_difference > threshold) {
                    num_differences_above_threshold++;
                }
            }

            float percentage_outliers = (float) num_differences_above_threshold / (float) num_samples;
            pv_test_true(
                    percentage_outliers < 0.05,
                    "too many outliers, got `%d`, maximum allowed: `%d`",
                    num_differences_above_threshold,
                    (int32_t) (num_samples * 0.055));
        } else {
            bool passed = false;
            status = pv_orca_metric_pcm_frame_level_error_evaluation(
                    ypu,
                    metric,
                    num_samples,
                    pcm,
                    text_truth_spoken_phonemes,
                    threshold_worst_case,
                    threshold_average_case,
                    &passed);
            pv_test_true(
                    status == PV_STATUS_SUCCESS,
                    "Unexpected error occurred at function `pv_orca_metric_pcm_frame_level_error_evaluation()`, the status is `%s`",
                    pv_status_to_string(status));
            if (status != PV_STATUS_SUCCESS) {
                pv_orca_pcm_delete(pcm_streaming);
                pcm_streaming = NULL;
                return PV_STATUS_SUCCESS;
            }

            pv_test_true(
                    passed == true,
                    "Did not pass the pcm frame-level error metric where threshold_worst_case: %f; threshold_average_case: %f",
                    threshold_worst_case, threshold_average_case);
        }

        pv_orca_pcm_delete(pcm_streaming);
        pcm_streaming = NULL;
    }
    return PV_STATUS_SUCCESS;
}


static void test_pv_orca_integration_phonemes_from_word_alignment_helper(
        int32_t num_alignments,
        pv_orca_word_alignment_t **alignments,
        const char *text_truth_original_phonemes) {
    PV_ASSERT(num_alignments >= 0);
    PV_ASSERT(alignments);
    PV_ASSERT(text_truth_original_phonemes);

    LOG_INFO("        %s", "`test_pv_orca_integration_phonemes_from_word_alignment_helper`");

    // Sanity checks for word alignments and phoneme alignments:
    char phoneme_sequence[8192] = {0};
    int32_t potential_phoneme_sequence_length = 0;
    for (int32_t i = 0; i < num_alignments; ++i) {
        pv_orca_word_alignment_t *word = alignments[i];
        pv_test_true(word->start_sec >= 0, "invalid alignment start");
        pv_test_true(word->end_sec >= 0, "invalid alignment end");
        pv_test_true(word->num_phonemes > 0, "invalid number of phonemes");
        pv_test_true(word->start_sec <= word->end_sec, "invalid alignment start and end");

        for (int32_t j = 0; j < word->num_phonemes; ++j) {
            pv_orca_phoneme_alignment_t *phoneme = word->phonemes[j];

            pv_test_true(phoneme->start_sec >= 0, "invalid phoneme start");
            pv_test_true(phoneme->end_sec >= 0, "invalid phoneme end");
            pv_test_true(phoneme->start_sec <= phoneme->end_sec, "invalid phoneme start and end");

            // +1 for the blank symbol.
            potential_phoneme_sequence_length += (int32_t) strlen(phoneme->phoneme) + 1;
            if (potential_phoneme_sequence_length < 8192) {
                strcat(phoneme_sequence, phoneme->phoneme);
                strcat(phoneme_sequence, " ");
            }
        }
    }
    // Remove the last padded space.
    if (strlen(phoneme_sequence) > 0) {
        phoneme_sequence[strlen(phoneme_sequence) - 1] = '\0';
    }

    pv_test_true(
            strcmp(text_truth_original_phonemes, phoneme_sequence) == 0,
            "Phoneme sequence unmatch:\t\n`text_truth_original_phonemes`:\t\t\n`%s`\t\n`phoneme_sequence`:\t\t\n`%s`",
            text_truth_original_phonemes,
            phoneme_sequence);
}

static pv_status_t test_pv_orca_integration_word_alignment_helper(
        const char *text_true_words,
        int32_t num_alignments,
        pv_orca_word_alignment_t **alignments) {
    PV_ASSERT(text_true_words);
    PV_ASSERT(num_alignments >= 0);
    PV_ASSERT(alignments);

    LOG_INFO("        %s", "`test_pv_orca_integration_word_alignment_helper`");

    char *text_true_words_copy = (char *) calloc(strlen(text_true_words) + 1, sizeof(char));
    if (!text_true_words_copy) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("string"));
        return PV_STATUS_OUT_OF_MEMORY;
    }
    strcpy(text_true_words_copy, text_true_words);

    const char **true_words = NULL;
    int32_t num_true_words = 0;
    split_true_words_helper(text_true_words_copy, &true_words, &num_true_words);
    if (num_true_words == 0 || !true_words) {
        LOG_ERROR_SIMPLE("failed to split text_true_words");
        free(text_true_words_copy);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    pv_test_true(num_alignments == num_true_words,
                 "wrong number of word alignments; got `%d`, expected `%d`",
                 num_alignments, num_true_words);
    for (int32_t i = 0; i < num_alignments; ++i) {
        pv_orca_word_alignment_t *word = alignments[i];
        pv_test_true(strcmp(word->word, true_words[i]) == 0,
                     "wrong word alignment; got `%s`, expected `%s`",
                     word->word, true_words[i]);
        pv_test_true(word->start_sec >= 0, "invalid alignment start");
        pv_test_true(word->end_sec >= 0, "invalid alignment end");
        pv_test_true(word->num_phonemes > 0, "invalid number of phonemes");
        pv_test_true(word->start_sec <= word->end_sec, "invalid alignment start and end");
    }

    free(true_words);
    free(text_true_words_copy);

    return PV_STATUS_SUCCESS;
}

/*
1. Synthesize as usual to derive the `pcm`.
2. Call `pv_orca_metric_pcm_frame_level_error_evaluation()` to decide if we passed the metric by passing a pointer of boolean variable,
    so the metric regardless of its internal implementation will be a Boolean metric (as opposed to a real-valued metric like Phoneme Error Rate (PER)).
*/
static pv_status_t test_pv_orca_integration_pcm_frame_level_error_helper(
        pv_orca_metric_t *metric,
        const char *text_truth_spoken_phonemes,
        float threshold_worst_case,
        float threshold_average_case,
        int32_t num_samples,
        int16_t *pcm) {
    PV_ASSERT(metric);
    PV_ASSERT(text_truth_spoken_phonemes);
    PV_ASSERT(threshold_worst_case >= 0 && threshold_worst_case <= 1.0);
    PV_ASSERT(threshold_average_case >= 0 && threshold_average_case <= 1.0);
    PV_ASSERT(num_samples);
    PV_ASSERT(pcm);

    LOG_INFO("        %s", "`test_pv_orca_integration_pcm_frame_level_error_helper`");
    bool passed = false;
    pv_status_t status = pv_orca_metric_pcm_frame_level_error_evaluation(
            ypu,
            metric,
            num_samples,
            pcm,
            text_truth_spoken_phonemes,
            threshold_worst_case,
            threshold_average_case,
            &passed);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "Unexpected error occured at function `pv_orca_metric_pcm_frame_level_error_evaluation()`, the status is `%s`",
            pv_status_to_string(status));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_metric_delete(ypu, metric);
        return status;
    }

    pv_test_true(
            passed == true,
            "Did not pass the pcm frame-level error metric where threshold_worst_case: %f; threshold_average_case: %f",
            threshold_worst_case,
            threshold_average_case);

    return PV_STATUS_SUCCESS;
}


static pv_status_t test_pv_orca_integration_intelligibility_helper(
        pv_orca_metric_t *metric,
        const char *text_truth_spoken_phonemes,
        int32_t num_samples,
        int16_t *pcm) {
    PV_ASSERT(metric);
    PV_ASSERT(text_truth_spoken_phonemes);
    PV_ASSERT(num_samples);
    PV_ASSERT(pcm);

    LOG_INFO("        %s", "`test_pv_orca_integration_intelligibility_helper`");
    float per = 0.0f;
    int32_t num_phonemes = 0;
    const char **phonemes_aos = NULL;
    char *text_truth_spoken_phonemes_copy = (char *) calloc(strlen(text_truth_spoken_phonemes) + 1, sizeof(char));
    if (!text_truth_spoken_phonemes_copy) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("text_truth_spoken_phonemes_copy"));
        return PV_STATUS_OUT_OF_MEMORY;
    }
    strcpy(text_truth_spoken_phonemes_copy, text_truth_spoken_phonemes);

    split_true_words_helper(text_truth_spoken_phonemes_copy, &phonemes_aos, &num_phonemes);
    if (num_phonemes == 0) {
        free(text_truth_spoken_phonemes_copy);
        text_truth_spoken_phonemes_copy = NULL;
        free(phonemes_aos);
        phonemes_aos = NULL;
        return PV_STATUS_INVALID_ARGUMENT;
    }

    pv_status_t status = pv_orca_metric_process(
            ypu,
            metric,
            num_samples,
            pcm,
            num_phonemes,
            phonemes_aos,
            &per);
    free(text_truth_spoken_phonemes_copy);
    text_truth_spoken_phonemes_copy = NULL;
    free(phonemes_aos);
    phonemes_aos = NULL;
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_metric_process,
                pv_status_to_string(status));
        return status;
    }

    pv_test_true(
            per < ORCA_INTELLIGIBILITY_THRESHOLD,
            "PER too high, got `%f`, expected a value lower than `%f`",
            per,
            ORCA_INTELLIGIBILITY_THRESHOLD);

    return PV_STATUS_SUCCESS;
}


static void test_pv_orca_integration_speaker_suite(
        const char *test_sentences_filename,
        const char *model_path,
        const char *metric_classifier_path,
        float stream_batch_length_diff_threshold) {
    PV_ASSERT(test_sentences_filename);
    PV_ASSERT(model_path);
    PV_ASSERT(metric_classifier_path);
    PV_ASSERT(stream_batch_length_diff_threshold >= 0 && stream_batch_length_diff_threshold <= 1.0);

    pv_orca_t *orca_object = NULL;
    pv_orca_synthesize_params_t *synthesize_params_object = NULL;
    pv_orca_metric_t *metric = NULL;

    pv_status_t status = test_pv_orca_setup_helper(model_path, &orca_object, &synthesize_params_object);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "Failed to set up orca related objects using function `test_pv_orca_setup_helper`");
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    status = test_pv_orca_metric_classifier_setup_helper(metric_classifier_path, orca_object, &metric);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "Failed to set up metric classifier using function `test_pv_orca_metric_classifier_setup_helper`");
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_synthesize_params_delete(synthesize_params_object);
        pv_orca_delete(orca_object);
        return;
    }

    char *test_sentences_path = pv_test_module_res_path(test_sentences_filename);
    pv_test_true(
            test_sentences_path,
            "Failed to get path of for test file `%s`",
            test_sentences_filename);
    if (!test_sentences_path) {
        pv_orca_synthesize_params_delete(synthesize_params_object);
        pv_orca_delete(orca_object);
        pv_orca_metric_delete(ypu, metric);
        return;
    }

    pv_orca_sentences_helper_t *test_cases_helper = NULL;
    status = pv_orca_sentences_helper_init(test_sentences_path, &test_cases_helper);
    pv_test_true(status == PV_STATUS_SUCCESS, "Failed to load test cases file `%s`", test_sentences_path);
    free(test_sentences_path);
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_synthesize_params_delete(synthesize_params_object);
        pv_orca_delete(orca_object);
        pv_orca_metric_delete(ypu, metric);
        return;
    }

    int32_t case_no = 0;
    char text_raw[1024] = {0};
    char text_truth_original_phonemes[8192] = {0};
    char text_truth_spoken_phonemes[8192] = {0};
    char text_true_words[1024] = {0};
    float threshold_worst_case;
    float threshold_average_case;

    while (pv_orca_sentences_helper_next_case(
            test_cases_helper,
            &case_no,
            text_raw,
            PV_ARRAY_LEN(text_raw),
            text_truth_original_phonemes,
            PV_ARRAY_LEN(text_truth_original_phonemes),
            text_truth_spoken_phonemes,
            PV_ARRAY_LEN(text_truth_spoken_phonemes),
            &threshold_worst_case,
            &threshold_average_case,
            text_true_words,
            PV_ARRAY_LEN(text_true_words))) {
        // Synthesize non-streaming audio once for all.
        LOG_INFO("    Case no: %d", case_no);

        int32_t num_samples = 0;
        int16_t *pcm = NULL;
        int32_t num_alignments = 0;
        pv_orca_word_alignment_t **alignments = NULL;

        // Set random seed for deterministic randomness.
        status = pv_orca_synthesize_params_set_random_state(synthesize_params_object, 29);
        pv_test_true(
                status == PV_STATUS_SUCCESS,
                "failed to set random state for `synthesize_params_object`; expected `%s` got `%s`",
                pv_status_to_string(PV_STATUS_SUCCESS),
                pv_status_to_string(status));
        if (status != PV_STATUS_SUCCESS) {
            pv_orca_synthesize_params_delete(synthesize_params_object);
            pv_orca_delete(orca_object);
            pv_orca_metric_delete(ypu, metric);
            pv_orca_sentences_helper_delete(test_cases_helper);
            return;
        }

        status = pv_orca_synthesize_internal(
                orca_object,
                text_raw,
                synthesize_params_object,
                true,
                true,
                &num_samples,
                &pcm,
                &num_alignments,
                &alignments);
        pv_test_true(
                status == PV_STATUS_SUCCESS,
                "failed to synthesize; expected `%s` got `%s`",
                pv_status_to_string(PV_STATUS_SUCCESS),
                pv_status_to_string(status));

        // Set back the random seed to default to allow true randomness as opposed to controlled deterministic randomness.
        pv_status_t status_random_seed = pv_orca_synthesize_params_set_default_random_state(synthesize_params_object);
        pv_test_true(
                status_random_seed == PV_STATUS_SUCCESS,
                "failed to set to default random state for `synthesize_params_object`; expected `%s` got `%s`",
                pv_status_to_string(PV_STATUS_SUCCESS),
                pv_status_to_string(status_random_seed));

        if (status != PV_STATUS_SUCCESS || status_random_seed != PV_STATUS_SUCCESS) {
            pv_orca_synthesize_params_delete(synthesize_params_object);
            pv_orca_delete(orca_object);
            pv_orca_metric_delete(ypu, metric);
            pv_orca_sentences_helper_delete(test_cases_helper);
            if (status == PV_STATUS_SUCCESS) {
                pv_orca_pcm_delete(pcm);
                pv_orca_word_alignments_delete(num_alignments, alignments);
            }
            return;
        }

        status = test_pv_orca_integration_batch_stream_pcm_match_helper(
                orca_object,
                synthesize_params_object,
                metric,
                stream_batch_length_diff_threshold,
                text_raw,
                text_truth_spoken_phonemes,
                threshold_worst_case,
                threshold_average_case,
                num_samples,
                pcm,
                num_alignments,
                alignments);
        pv_test_true(
                status == PV_STATUS_SUCCESS,
                "`test_pv_orca_integration_batch_stream_pcm_match_helper` failed; expected `%s` got `%s`",
                pv_status_to_string(PV_STATUS_SUCCESS),
                pv_status_to_string(status));
        if (status != PV_STATUS_SUCCESS) {
            pv_orca_synthesize_params_delete(synthesize_params_object);
            pv_orca_delete(orca_object);
            pv_orca_metric_delete(ypu, metric);
            pv_orca_sentences_helper_delete(test_cases_helper);
            pv_orca_pcm_delete(pcm);
            pv_orca_word_alignments_delete(num_alignments, alignments);
            return;
        }

        status = test_pv_orca_integration_pcm_frame_level_error_helper(
                metric,
                text_truth_spoken_phonemes,
                threshold_worst_case,
                threshold_average_case,
                num_samples,
                pcm);
        pv_test_true(
                status == PV_STATUS_SUCCESS,
                "`test_pv_orca_integration_pcm_frame_level_error_helper` failed; expected `%s` got `%s`",
                pv_status_to_string(PV_STATUS_SUCCESS),
                pv_status_to_string(status));
        if (status != PV_STATUS_SUCCESS) {
            pv_orca_synthesize_params_delete(synthesize_params_object);
            pv_orca_delete(orca_object);
            pv_orca_metric_delete(ypu, metric);
            pv_orca_sentences_helper_delete(test_cases_helper);
            pv_orca_pcm_delete(pcm);
            pv_orca_word_alignments_delete(num_alignments, alignments);
            return;
        }

        status = test_pv_orca_integration_intelligibility_helper(
                metric,
                text_truth_spoken_phonemes,
                num_samples,
                pcm);
        pv_test_true(
                status == PV_STATUS_SUCCESS,
                "`test_pv_orca_integration_intelligibility_helper` failed; expected `%s` got `%s`",
                pv_status_to_string(PV_STATUS_SUCCESS),
                pv_status_to_string(status));
        if (status != PV_STATUS_SUCCESS) {
            pv_orca_synthesize_params_delete(synthesize_params_object);
            pv_orca_delete(orca_object);
            pv_orca_metric_delete(ypu, metric);
            pv_orca_sentences_helper_delete(test_cases_helper);
            pv_orca_pcm_delete(pcm);
            pv_orca_word_alignments_delete(num_alignments, alignments);
            return;
        }

        test_pv_orca_integration_phonemes_from_word_alignment_helper(
                num_alignments,
                alignments,
                text_truth_original_phonemes);

        status = test_pv_orca_integration_word_alignment_helper(
                text_true_words,
                num_alignments,
                alignments);
        pv_test_true(
                status == PV_STATUS_SUCCESS,
                "`test_pv_orca_integration_word_alignment_helper` failed; expected `%s` got `%s`",
                pv_status_to_string(PV_STATUS_SUCCESS),
                pv_status_to_string(status));
        if (status != PV_STATUS_SUCCESS) {
            pv_orca_synthesize_params_delete(synthesize_params_object);
            pv_orca_delete(orca_object);
            pv_orca_metric_delete(ypu, metric);
            pv_orca_sentences_helper_delete(test_cases_helper);
            pv_orca_pcm_delete(pcm);
            pv_orca_word_alignments_delete(num_alignments, alignments);
            return;
        }

#if !defined(__PV_TARGET_NO_FILE_SYSTEM__) && !defined(__PV_TARGET_PLATFORM_WASM__)

        int32_t sample_rate = 0;
        pv_orca_sample_rate(orca_object, &sample_rate);

        char filename[256];
        static int32_t file_index = 0;
        sprintf(filename, "pcm/%03d.wav", file_index++);

        char *path = pv_test_module_res_path(filename);

        pv_writer_wav_t *output_file = NULL;
        status = pv_writer_wav_init(
            path,
            sample_rate,
            &output_file);
        free(path);
        pv_test_true(
                status == PV_STATUS_SUCCESS,
                "`pv_writer_wav_init` failed; expected `%s` got `%s`",
                pv_status_to_string(PV_STATUS_SUCCESS),
                pv_status_to_string(status));
        if (status != PV_STATUS_SUCCESS) {
            pv_orca_synthesize_params_delete(synthesize_params_object);
            pv_orca_delete(orca_object);
            pv_orca_metric_delete(ypu, metric);
            pv_orca_sentences_helper_delete(test_cases_helper);
            pv_orca_pcm_delete(pcm);
            pv_orca_word_alignments_delete(num_alignments, alignments);
            return;
        }

        status = pv_writer_wav_write(
            output_file,
            num_samples,
            pcm);
        pv_writer_wav_delete(output_file);
        pv_test_true(
                status == PV_STATUS_SUCCESS,
                "`pv_writer_wav_write` failed; expected `%s` got `%s`",
                pv_status_to_string(PV_STATUS_SUCCESS),
                pv_status_to_string(status));
        if (status != PV_STATUS_SUCCESS) {
            pv_orca_synthesize_params_delete(synthesize_params_object);
            pv_orca_delete(orca_object);
            pv_orca_metric_delete(ypu, metric);
            pv_orca_sentences_helper_delete(test_cases_helper);
            pv_orca_pcm_delete(pcm);
            pv_orca_word_alignments_delete(num_alignments, alignments);
            return;
        }

#endif

        memset(text_raw, '\0', 1024);
        memset(text_truth_original_phonemes, '\0', 8192);
        memset(text_truth_spoken_phonemes, '\0', 8192);
        memset(text_true_words, '\0', 1024);
        pv_orca_pcm_delete(pcm);
        pv_orca_word_alignments_delete(num_alignments, alignments);

        // TODO(Eric): Tests cause memory fragmentation in WASM. Revisit once we switch to EMSCRIPTEN.

#if (defined(__PV_BUILD_TYPE_DEBUG__) && (defined(__PV_TARGET_PLATFORM_RASPBERRYPI__) || defined(__PV_TARGET_PLATFORM_ANDROID__))) || (defined(__PV_TARGET_PLATFORM_WASM__))

        break;

#endif

    }

    pv_orca_synthesize_params_delete(synthesize_params_object);
    pv_orca_delete(orca_object);
    pv_orca_metric_delete(ypu, metric);
    pv_orca_sentences_helper_delete(test_cases_helper);
}


// GERMAN:
static void test_pv_orca_integration_de_male(void) {
    test_pv_orca_integration_speaker_suite(
            "test_data/integration_sentences/de/sentences_de_male.csv",
            "param/orca_params_de_male.pv",
            "metric/orca_metric_classifier_params_de.pv",
            0.f);
}

static void test_pv_orca_integration_de_female(void) {
    test_pv_orca_integration_speaker_suite(
            "test_data/integration_sentences/de/sentences_de_female.csv",
            "param/orca_params_de_female.pv",
            "metric/orca_metric_classifier_params_de.pv",
            0.f);
}

// ENGLISH:
static void test_pv_orca_integration_en_male(void) {
    test_pv_orca_integration_speaker_suite(
            "test_data/integration_sentences/en/sentences_en_male.csv",
            "param/orca_params_en_male.pv",
            "metric/orca_metric_classifier_params_en.pv",
            0.f);
}

static void test_pv_orca_integration_en_female(void) {
    test_pv_orca_integration_speaker_suite(
            "test_data/integration_sentences/en/sentences_en_female.csv",
            "param/orca_params_en_female.pv",
            "metric/orca_metric_classifier_params_en.pv",
            0.f);
}

#ifdef __PV_YPU_CUDA_SUPPORT__

static void test_pv_orca_integration_en_cuda(void) {
    pv_ypu_t *prev = ypu;
    pv_status_t status = pv_ypu_init_cuda(0, &ypu);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "`pv_ypu_init_cuda` failed; expected `%s` got `%s`",
            pv_status_to_string(PV_STATUS_SUCCESS),
            pv_status_to_string(status));
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    test_pv_orca_integration_speaker_suite(
            "test_data/integration_sentences/en/sentences_en_female.csv",
            "param/orca_params_en_female.pv",
            "metric/orca_metric_classifier_params_en.pv",
            0.f);

    pv_ypu_delete(ypu);
    ypu = prev;
}

#endif

#ifdef __PV_YPU_METAL_SUPPORT__

static void test_pv_orca_integration_en_metal(void) {
    pv_ypu_t *prev = ypu;
    pv_status_t status = pv_ypu_init_metal(0, &ypu);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "`pv_ypu_init_metal` failed; expected `%s` got `%s`",
            pv_status_to_string(PV_STATUS_SUCCESS),
            pv_status_to_string(status));
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    test_pv_orca_integration_speaker_suite(
            "test_data/integration_sentences/en/sentences_en_female.csv",
            "param/orca_params_en_female.pv",
            "metric/orca_metric_classifier_params_en.pv",
            0.f);

    pv_ypu_delete(ypu);
    ypu = prev;
}

#endif

// SPANISH:
static void test_pv_orca_integration_es_female(void) {
    test_pv_orca_integration_speaker_suite(
            "test_data/integration_sentences/es/sentences_es_female.csv",
            "param/orca_params_es_female.pv",
            "metric/orca_metric_classifier_params_es.pv",
            0.f);
}

static void test_pv_orca_integration_es_male(void) {
    test_pv_orca_integration_speaker_suite(
            "test_data/integration_sentences/es/sentences_es_male.csv",
            "param/orca_params_es_male.pv",
            "metric/orca_metric_classifier_params_es.pv",
            0.f);
}

// FRENCH:
static void test_pv_orca_integration_fr_male(void) {
    test_pv_orca_integration_speaker_suite(
            "test_data/integration_sentences/fr/sentences_fr_male.csv",
            "param/orca_params_fr_male.pv",
            "metric/orca_metric_classifier_params_fr.pv",
            0.f);
}

static void test_pv_orca_integration_fr_female(void) {
    test_pv_orca_integration_speaker_suite(
            "test_data/integration_sentences/fr/sentences_fr_female.csv",
            "param/orca_params_fr_female.pv",
            "metric/orca_metric_classifier_params_fr.pv",
            0.f);
}

// ITALIAN:
static void test_pv_orca_integration_it_female(void) {
    test_pv_orca_integration_speaker_suite(
            "test_data/integration_sentences/it/sentences_it_female.csv",
            "param/orca_params_it_female.pv",
            "metric/orca_metric_classifier_params_it.pv",
            0.f);
}

static void test_pv_orca_integration_it_male(void) {
    test_pv_orca_integration_speaker_suite(
            "test_data/integration_sentences/it/sentences_it_male.csv",
            "param/orca_params_it_male.pv",
            "metric/orca_metric_classifier_params_it.pv",
            0.f);
}

// PORTUGUESE:
static void test_pv_orca_integration_pt_male(void) {
    test_pv_orca_integration_speaker_suite(
            "test_data/integration_sentences/pt/sentences_pt_male.csv",
            "param/orca_params_pt_male.pv",
            "metric/orca_metric_classifier_params_pt.pv",
            0.f);
}

static void test_pv_orca_integration_pt_female(void) {
    test_pv_orca_integration_speaker_suite(
            "test_data/integration_sentences/pt/sentences_pt_female.csv",
            "param/orca_params_pt_female.pv",
            "metric/orca_metric_classifier_params_pt.pv",
            0.f);
}

// KOREAN:
static void test_pv_orca_integration_ko_female(void) {
    test_pv_orca_integration_speaker_suite(
            "test_data/integration_sentences/ko/sentences_ko_female.csv",
            "param/orca_params_ko_female.pv",
            "metric/orca_metric_classifier_params_ko.pv",
            0.f);
}

// JAPANESE:
static void test_pv_orca_integration_ja_female(void) {
    test_pv_orca_integration_speaker_suite(
            "test_data/integration_sentences/ja/sentences_ja_female.csv",
            "param/orca_params_ja_female.pv",
            "metric/orca_metric_classifier_params_ja.pv",
            0.08f);
}

static void test_pv_orca_integration_ja_male(void) {
    test_pv_orca_integration_speaker_suite(
            "test_data/integration_sentences/ja/sentences_ja_male.csv",
            "param/orca_params_ja_male.pv",
            "metric/orca_metric_classifier_params_ja.pv",
            0.08f);
}

static const pv_test_case_t PV_ORCA_INTEGRATION_TEST_CASES[] = {

        {"orca_integration_de_male", test_pv_orca_integration_de_male},
        {"orca_integration_de_female", test_pv_orca_integration_de_female},

        {"orca_integration_en_male", test_pv_orca_integration_en_male},
        {"orca_integration_en_female", test_pv_orca_integration_en_female},

        {"orca_integration_es_female", test_pv_orca_integration_es_female},
        {"orca_integration_es_male", test_pv_orca_integration_es_male},

        {"orca_integration_fr_male", test_pv_orca_integration_fr_male},
        {"orca_integration_fr_female", test_pv_orca_integration_fr_female},

        {"orca_integration_it_female", test_pv_orca_integration_it_female},
        {"orca_integration_it_male", test_pv_orca_integration_it_male},

        {"orca_integration_pt_male", test_pv_orca_integration_pt_male},
        {"orca_integration_pt_female", test_pv_orca_integration_pt_female},

        // TODO: KOREAN needs to update when zoo-research Korean gets updated, update test_data/sentences.csv with new
        //       supported punctuations and pv_orca_language_data.c.
        {"orca_integration_ko_female", test_pv_orca_integration_ko_female},

        {"orca_integration_ja_female", test_pv_orca_integration_ja_female},
        {"orca_integration_ja_male", test_pv_orca_integration_ja_male},

#ifdef __PV_YPU_CUDA_SUPPORT__

        {"orca_integration_en_cuda", test_pv_orca_integration_en_cuda},

#endif

#ifdef __PV_YPU_METAL_SUPPORT__

        {"orca_integration_en_metal", test_pv_orca_integration_en_metal},

#endif

};

const pv_test_suite_t PV_ORCA_INTEGRATION_TEST_SUITE = {
        .name = "orca_integration",
        .setup = test_pv_orca_integration_setup,
        .teardown = test_pv_orca_integration_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_ORCA_INTEGRATION_TEST_CASES),
        .test_cases = PV_ORCA_INTEGRATION_TEST_CASES,
};
