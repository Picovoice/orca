#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "audio/pv_audio_file.h"
#include "audio/pv_container_wav.h"
#include "core/pv_assert.h"
#include "io/pv_dump.h"
#include "orca/pv_orca.h"
#include "orca/pv_orca_internal.h"

#define MAX_NUM_CHUNKS (100)

typedef struct pcm_chunk_list pcm_chunk_list_t;

struct pcm_chunk_list {
    int32_t num_samples;
    int16_t *pcm;
    pcm_chunk_list_t *next;
};

static pv_status_t pcm_chunk_init(
        int32_t num_samples,
        int16_t *pcm,
        pcm_chunk_list_t **chunk) {
    PV_ASSERT(num_samples > 0);
    PV_ASSERT(pcm);
    PV_ASSERT(chunk);

    *chunk = NULL;

    pcm_chunk_list_t *c = calloc(1, sizeof(pcm_chunk_list_t));
    if (!c) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    c->pcm = pcm;
    c->num_samples = num_samples;
    c->next = NULL;

    *chunk = c;

    return PV_STATUS_SUCCESS;
}

static pv_status_t pcm_chunk_delete(pcm_chunk_list_t *chunk) {
    if (chunk) {
        free(chunk->pcm);
        free(chunk);
    }
    return PV_STATUS_SUCCESS;
}

void usage(const char *program) {
    (void) fprintf(
            stderr,
            "usage: %s -a ACCESS_KEY -m MODEL_PATH [-y DEVICE] "

#ifdef __PV_DUMP__

            "-d DUMP_FOLDER "

#endif

            "-t INPUT_TEXT -o OUTPUT_AUDIO_PATH (-r SPEECH_RATE -s RANDOM_STATE)\n",
            program);
}

void handle_error(char **message_stack, int32_t message_stack_depth) {
    pv_status_t error_status = pv_get_error_stack(&message_stack, &message_stack_depth);

    if (error_status != PV_STATUS_SUCCESS) {
        fprintf(stderr, ".\nUnable to get Orca error state with '%s'\n", pv_status_to_string(error_status));
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

    const char *access_key = NULL;
    const char *model_path = NULL;
    const char *device = "cpu:1";

#ifdef __PV_DUMP__

    const char *dump_path = NULL;

#endif

    const char *text = NULL;
    const char *output_path = NULL;
    float speech_rate = 1.0f;
    int64_t random_state = 0;

#ifdef __PV_DUMP__

    const char *SHORT_OPTIONS = "a:m:y:d:t:o:r:";

#else

    const char *SHORT_OPTIONS = "a:m:y:t:o:r:s:";

#endif

    int opt;
    while ((opt = getopt(argc, argv, SHORT_OPTIONS)) != -1) {
        switch (opt) {
            case 'a':
                access_key = optarg;
                break;
            case 'm':
                model_path = optarg;
                break;
            case 'y':
                device = optarg;
                break;

#ifdef __PV_DUMP__

            case 'd':
                dump_path = optarg;
                break;

#endif

            case 't':
                text = optarg;
                break;
            case 'o':
                output_path = optarg;
                break;
            case 'r':
                speech_rate = atof(optarg);
                break;
            case 's':
                random_state = atoi(optarg);
                break;
            default:
                break;
        }
    }

    if (!(access_key && model_path && text && output_path)) {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    PV_DUMP_START(dump_path)

    char **message_stack = NULL;
    int32_t message_stack_depth = 0;

    pv_orca_t *orca = NULL;
    pv_status_t status = pv_orca_init(access_key, model_path, device, &orca);
    if (status != PV_STATUS_SUCCESS) {
        (void) fprintf(stderr, "Could not create Orca object: `%s`", pv_status_to_string(status));
        handle_error(message_stack, message_stack_depth);
        exit(EXIT_FAILURE);
    }

    pv_orca_synthesize_params_t *synthesize_params = NULL;
    status = pv_orca_synthesize_params_init(&synthesize_params);
    if (status != PV_STATUS_SUCCESS) {
        (void) fprintf(stderr, "Could not create Orca synthesize params object: `%s`", pv_status_to_string(status));
        handle_error(message_stack, message_stack_depth);
        exit(EXIT_FAILURE);
    }

    if (speech_rate != 1.0f) {
        status = pv_orca_synthesize_params_set_speech_rate(synthesize_params, speech_rate);
        if (status != PV_STATUS_SUCCESS) {
            (void) fprintf(stderr, "Could not set speech rate: `%s`", pv_status_to_string(status));
            handle_error(message_stack, message_stack_depth);
            exit(EXIT_FAILURE);
        }
    }

    if (random_state >= 0) {
        status = pv_orca_synthesize_params_set_random_state(synthesize_params, random_state);
        if (status != PV_STATUS_SUCCESS) {
            (void) fprintf(stderr, "Could not set random seed: `%s`", pv_status_to_string(status));
            handle_error(message_stack, message_stack_depth);
            exit(EXIT_FAILURE);
        }
    }

    printf("Simulating input text stream with the following text: `%s`\n", text);

    pv_normalizer_t *normalizer = pv_orca_get_normalizer(orca);
    if (normalizer == NULL) {
        (void) fprintf(stderr, "Error getting normalizer");
        handle_error(message_stack, message_stack_depth);
        exit(EXIT_FAILURE);
    }

    int32_t num_samples_chunks[MAX_NUM_CHUNKS] = {0};
    float start_chunks[MAX_NUM_CHUNKS] = {0};
    start_chunks[0] = (float) clock() / CLOCKS_PER_SEC;
    float end_chunks[MAX_NUM_CHUNKS] = {0};
    int32_t num_chunks = 0;

    pcm_chunk_list_t *pcm_chunk_prev = NULL;
    pcm_chunk_list_t *pcm_chunk_head = NULL;

    pv_orca_stream_t *orca_stream = NULL;
    status = pv_orca_stream_open(orca, synthesize_params, &orca_stream);
    if (status != PV_STATUS_SUCCESS) {
        (void) fprintf(stderr, "Error opening stream");
        handle_error(message_stack, message_stack_depth);
        exit(EXIT_FAILURE);
    }

    char character[PV_NORMALIZER_MAX_NUM_BYTES_PER_CHARACTER] = {0};
    size_t index = 0;
    while (index < strlen(text)) {
        if (num_chunks > (MAX_NUM_CHUNKS - 1)) {
            (void) fprintf(stderr, "Too many chunks");
            handle_error(message_stack, message_stack_depth);
            exit(EXIT_FAILURE);
        }

        int32_t num_bytes_character = 0;
        status = pv_language_num_bytes_character((unsigned char) text[index], &num_bytes_character);
        if (status != PV_STATUS_SUCCESS) {
            (void) fprintf(stderr, "Error getting number of bytes for character: `%c`", text[index]);
            handle_error(message_stack, message_stack_depth);
            exit(EXIT_FAILURE);
        }

        for (int32_t j = 0; j < num_bytes_character; j++) {
            character[j] = text[index + j];
        }
        character[num_bytes_character] = '\0';

        int32_t num_samples_chunk = 0;
        int16_t *pcm_chunk = NULL;
        status = pv_orca_stream_synthesize(orca_stream, character, &num_samples_chunk, &pcm_chunk);
        if (status != PV_STATUS_SUCCESS) {
            (void) fprintf(stderr, "Error adding token: `%s`", character);
            handle_error(message_stack, message_stack_depth);
            exit(EXIT_FAILURE);
        }

        if (num_samples_chunk > 0) {
            if (pcm_chunk_prev == NULL) {
                pcm_chunk_init(num_samples_chunk, pcm_chunk, &pcm_chunk_prev);
                pcm_chunk_head = pcm_chunk_prev;
            } else {
                pcm_chunk_init(num_samples_chunk, pcm_chunk, &pcm_chunk_prev->next);
                pcm_chunk_prev = pcm_chunk_prev->next;
            }

            float timestamp = (float) clock() / CLOCKS_PER_SEC;
            num_samples_chunks[num_chunks] = num_samples_chunk;
            end_chunks[num_chunks++] = timestamp;
            start_chunks[num_chunks] = timestamp;
        }
        index += num_bytes_character;
    }
    int32_t num_samples_chunk = 0;
    int16_t *pcm_chunk = NULL;
    status = pv_orca_stream_flush(orca_stream, &num_samples_chunk, &pcm_chunk);
    if (status != PV_STATUS_SUCCESS) {
        (void) fprintf(stderr, "Error flushing");
        handle_error(message_stack, message_stack_depth);
        exit(EXIT_FAILURE);
    }

    if (num_samples_chunk > 0) {
        if (pcm_chunk_prev == NULL) {
            pcm_chunk_init(num_samples_chunk, pcm_chunk, &pcm_chunk_prev);
            pcm_chunk_head = pcm_chunk_prev;
        } else {
            pcm_chunk_init(num_samples_chunk, pcm_chunk, &pcm_chunk_prev->next);
            pcm_chunk_prev = pcm_chunk_prev->next;
        }

        float timestamp = (float) clock() / CLOCKS_PER_SEC;
        num_samples_chunks[num_chunks] = num_samples_chunk;
        end_chunks[num_chunks++] = timestamp;
        start_chunks[num_chunks] = timestamp;
    }

    pv_orca_stream_close(orca_stream);
    pv_orca_synthesize_params_delete(synthesize_params);

    // CONCATENATE PCM CHUNKS
    int32_t num_samples = 0;
    pcm_chunk_list_t *pcm_chunk_iter = pcm_chunk_head;
    while (pcm_chunk_iter != NULL) {
        num_samples += pcm_chunk_iter->num_samples;
        pcm_chunk_iter = pcm_chunk_iter->next;
    }

    int16_t *pcm = malloc(num_samples * sizeof(int16_t));
    int32_t offset = 0;
    pcm_chunk_iter = pcm_chunk_head;
    while (pcm_chunk_iter != NULL) {
        memcpy(&pcm[offset], pcm_chunk_iter->pcm, pcm_chunk_iter->num_samples * sizeof(int16_t));
        offset += pcm_chunk_iter->num_samples;
        pcm_chunk_iter = pcm_chunk_iter->next;
    }

    int32_t sample_rate = 0;
    pv_orca_sample_rate(orca, &sample_rate);

    pv_orca_delete(orca);

    pv_writer_wav_t *output_file;
    status = pv_writer_wav_init(output_path, sample_rate, &output_file);
    if (status != PV_STATUS_SUCCESS) {
        free(pcm);
        (void) fprintf(stderr, "Error opening output file: `%s`", pv_status_to_string(status));
        handle_error(message_stack, message_stack_depth);
        exit(EXIT_FAILURE);
    }

    status = pv_writer_wav_write(output_file, num_samples, pcm);
    pv_writer_wav_delete(output_file);
    pv_orca_pcm_delete(pcm);
    pcm_chunk_iter = pcm_chunk_head;
    while (pcm_chunk_iter != NULL) {
        pcm_chunk_list_t *tmp = pcm_chunk_iter;
        pcm_chunk_iter = pcm_chunk_iter->next;
        pcm_chunk_delete(tmp);
    }
    if (status != PV_STATUS_SUCCESS) {
        (void) fprintf(stderr, "Error writing output file: `%s`", pv_status_to_string(status));
        handle_error(message_stack, message_stack_depth);
        exit(EXIT_FAILURE);
    }

    printf("Saved audio to `%s`\n", output_path);
    printf("Time until first chunk: %.3f\n", (end_chunks[0] - start_chunks[0]));
    printf("Audio length: %.3f s, proc time: %.3f s, RTF: %.3f\n",
           (float) num_samples / (float) sample_rate,
           (end_chunks[num_chunks - 1] - start_chunks[0]),
           (float) num_samples / (float) sample_rate / (end_chunks[num_chunks - 1] - start_chunks[0]));

    for (int32_t i = 0; i < num_chunks; i++) {
        float num_seconds = num_samples_chunks[i] / (float) sample_rate;
        float process_time = end_chunks[i] - start_chunks[i];
        float rtf = num_seconds / process_time;
        printf(
                "Chunk #%d: audio length: %.3f s, proc time %.3f s, RTF: %.3f\n",
                i,
                num_seconds,
                process_time,
                rtf);
    }

    PV_DUMP_END()
    return EXIT_SUCCESS;
}
