#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "audio/pv_audio_file.h"
#include "audio/pv_container_wav.h"
#include "io/pv_dump.h"
#include "orca/pv_orca.h"

void usage(const char *program) {
    (void) fprintf(
            stderr,
            "usage: %s -a ACCESS_KEY -m MODEL_PATH [-y DEVICE] "

#ifdef __PV_DUMP__

            "-d DUMP_FOLDER "

#endif

            "-t INPUT_TEXT -o OUTPUT_AUDIO_PATH (-r SPEECH_RATE -s RANDOM_STATE -v VERBOSE)\n",
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
    bool verbose = false;

#ifdef __PV_DUMP__

    const char *SHORT_OPTIONS = "a:m:y:d:t:o:r:s:v:";

#else

    const char *SHORT_OPTIONS = "a:m:y:t:o:r:s:v:";

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
            case 'v':
                verbose = atoi(optarg);
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

    printf("Version: `%s`\n", pv_orca_version());

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

    printf("Synthesizing text `%s` ...\n", text);

    int32_t num_samples = 0;
    int16_t *pcm = NULL;
    int32_t num_alignments = 0;
    pv_orca_word_alignment_t **alignments = NULL;
    float start = (float) clock() / CLOCKS_PER_SEC;
    status = pv_orca_synthesize(
            orca,
            text,
            synthesize_params,
            &num_samples,
            &pcm,
            &num_alignments,
            &alignments);
    pv_orca_synthesize_params_delete(synthesize_params);
    if (status != PV_STATUS_SUCCESS) {
        (void) fprintf(stderr, "Error synthesizing: `%s`", pv_status_to_string(status));
        handle_error(message_stack, message_stack_depth);
        exit(EXIT_FAILURE);
    }
    float end = (float) clock() / CLOCKS_PER_SEC;

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
    if (status != PV_STATUS_SUCCESS) {
        (void) fprintf(stderr, "Error writing output file: `%s`", pv_status_to_string(status));
        handle_error(message_stack, message_stack_depth);
        exit(EXIT_FAILURE);
    }

    if (verbose) {
        printf("\n");
        for (int32_t i = 0; i < num_alignments; i++) {
            printf("{\n");
            printf("\t\"word\": \"%s\",\n", alignments[i]->word);
            printf("\t\"start_sec\": %.3f,\n", alignments[i]->start_sec);
            printf("\t\"end_sec\": %.3f,\n", alignments[i]->end_sec);
            printf("\t\"phonemes\": [\n");
            for (int32_t j = 0; j < alignments[i]->num_phonemes; j++) {
                printf("\t\t{\n");
                printf("\t\t\t\"phoneme\": \"%s\",\n", alignments[i]->phonemes[j]->phoneme);
                printf("\t\t\t\"start_sec\": %.3f,\n", alignments[i]->phonemes[j]->start_sec);
                printf("\t\t\t\"end_sec\": %.3f\n", alignments[i]->phonemes[j]->end_sec);
                printf("\t\t}%s\n", j < alignments[i]->num_phonemes - 1 ? "," : "");
            }
            printf("\t]\n");
            printf("}%s\n", i < num_alignments - 1 ? "," : "");
        }

        printf("\n");
        for (int32_t i = 0; i < num_alignments; i++) {
            for (int32_t j = 0; j < alignments[i]->num_phonemes; j++) {
                printf("%s ", alignments[i]->phonemes[j]->phoneme);
            }
        }
        printf("\n");
    }

    status = pv_orca_word_alignments_delete(num_alignments, alignments);
    if (status != PV_STATUS_SUCCESS) {
        (void) fprintf(stderr, "Error deleting word alignments: `%s`", pv_status_to_string(status));
        handle_error(message_stack, message_stack_depth);
        exit(EXIT_FAILURE);
    }

    printf("Saved audio to `%s`\n", output_path);
    printf("Audio length: %.3f s, processing time: %.3f s, RTF: %.3f\n",
           (float) num_samples / (float) sample_rate,
           (end - start),
           (float) num_samples / (float) sample_rate / (end - start));

    PV_DUMP_END()
    return EXIT_SUCCESS;
}
