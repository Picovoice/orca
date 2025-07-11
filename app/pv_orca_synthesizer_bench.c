#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "core/pv_error.h"
#include "core/pv_error_messages.h"
#include "io/pv_dump.h"
#include "orca/pv_orca.h"
#include "orca/pv_orca_internal.h"
#include "orca/pv_orca_stream_state.h"
#include "orca/pv_orca_synthesizer.h"
#include "orca/pv_profiler.h"

void usage(const char *program) {
    (void) fprintf(
            stderr,
            "usage: %s -m MODEL_PATH "

#ifdef __PV_DUMP__

            "-d DUMP_FOLDER "

#endif

            "\n",

            program);
}

int main(int argc, char *argv[]) {

    const char *model_path = NULL;

#ifdef __PV_DUMP__

    const char *dump_path = NULL;

#endif

#ifdef __PV_DUMP__

    const char *SHORT_OPTIONS = "m:d:";

#else

    const char *SHORT_OPTIONS = "m:";

#endif

    int opt;
    while ((opt = getopt(argc, argv, SHORT_OPTIONS)) != -1) {
        switch (opt) {
            case 'm':
                model_path = optarg;
                break;

#ifdef __PV_DUMP__

                case 'd':
                    dump_path = optarg;
                    break;

#endif

            default:
                break;
        }
    }

#ifndef __PV_PROFILING_MODE__

    (void) fprintf(stderr, "Profiling mode is disabled. Please enable it in orca/CMakeLists.txt\n");
    exit(EXIT_FAILURE);

#endif

    if (!(model_path)) {
        usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    PV_DUMP_START(dump_path)

    FILE *f = pv_fopen(model_path, "rb");
    if (!f) {
        return PV_STATUS_IO_ERROR;
    }

    pv_orca_phonemizer_param_t *phonemizer_param = NULL;
    pv_orca_synthesizer_param_t *synthesizer_param = NULL;
    pv_status_t status = pv_orca_internal_param_load(f, &phonemizer_param, &synthesizer_param);
    if (status != PV_STATUS_SUCCESS) {
        fclose(f);
        (void) fprintf(stderr, "Could not load Orca parameters: `%s`", pv_status_to_string(status));
        return status;
    }

    pv_orca_stream_state_t *stream_state = NULL;
    int32_t eos_punctuation_indices[1] = {-1};
    status = pv_orca_stream_state_init(
            synthesizer_param,
            1,
            eos_punctuation_indices,
            1,
            eos_punctuation_indices,
            -1,
            200,
            &stream_state);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_module_init_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE(
                        "pv_orca_stream_state_init",
                        pv_status_to_string(status)));
        fclose(f);
        (void) fprintf(stderr, "Could not create Orca streaming state object: `%s`", pv_status_to_string(status));
        return status;
    }

    pv_orca_synthesizer_t *orca_synthesizer = NULL;
    status = pv_orca_synthesizer_init(
            synthesizer_param,
            stream_state,
            &orca_synthesizer);
    if (status != PV_STATUS_SUCCESS) {
        fclose(f);
        (void) fprintf(stderr, "Could not create Orca synthesizer object: `%s`", pv_status_to_string(status));
        return status;
    }

    pv_orca_synthesize_params_t *synthesize_params = NULL;
    status = pv_orca_synthesize_params_init(&synthesize_params);
    if (status != PV_STATUS_SUCCESS) {
        fclose(f);
        (void) fprintf(stderr, "Could not create Orca synthesize params object: `%s`", pv_status_to_string(status));
        exit(EXIT_FAILURE);
    }

    int32_t sample_rate = pv_orca_synthesizer_sample_rate(orca_synthesizer);

    const int token_lengths[] = {20, 50, 100};
    const int num_lengths = 3;
    const int num_iterations = 5;

    float rtf_sum = 0.f;
    float audio_length_second_sum = 0.f;
    float processing_time_sum = 0.f;

    for (int i = 0; i < num_lengths; i++) {
        int32_t num_tokens = token_lengths[i];
        int32_t *tokens = malloc(num_tokens * sizeof(int32_t));
        if (tokens == NULL) {
            (void) fprintf(stderr, "Memory allocation failed\n");
            exit(EXIT_FAILURE);
        }

        for (int32_t iteration = 0; iteration < num_iterations; iteration++) {
            printf("\n>> Num tokens: %d, Iteration: %d\n\n", num_tokens, iteration);

            for (int j = 0; j < num_tokens; j++) {
                tokens[j] = rand() % 30;
            }

            int32_t num_samples = 0;
            int16_t *pcm = NULL;
            int32_t *token_durations = NULL;
            float start = (float) clock() / CLOCKS_PER_SEC;

            status = pv_orca_synthesizer_forward(
                    orca_synthesizer,
                    synthesize_params,
                    false,
                    num_tokens,
                    tokens,
                    &token_durations,
                    &num_samples,
                    &pcm);
            if (status != PV_STATUS_SUCCESS) {
                fprintf(stderr, "Error synthesizing: `%s`\n", pv_status_to_string(status));
                free(tokens);
                exit(EXIT_FAILURE);
            }

            float end = (float) clock() / CLOCKS_PER_SEC;

            float rtf = (float) num_samples / (float) sample_rate / (end - start);
            float audio_length_second = (float) num_samples / (float) sample_rate;
            int32_t num_frames = num_samples / PV_ORCA_WINDOW_SHIFT;
            float processing_time = end - start;

            printf("\nForward pass: phoneme tokens (frames): %d (%d), audio length: %.2f s, "
                   "processing time: %.2f s, RTF: %.1f <<\n",
                   num_tokens,
                   num_frames,
                   audio_length_second,
                   processing_time,
                   rtf);

            rtf_sum += rtf;
            audio_length_second_sum += audio_length_second;
            processing_time_sum += processing_time;

            pv_orca_pcm_delete(pcm);
        }

        free(tokens);
    }

    printf("\n*************************************************");
    printf("\nBenchmark summary\n\n");
    printf("Total synthesized audio = %.2f seconds\n", audio_length_second_sum);
    printf("Total processing time = %.2f seconds\n", processing_time_sum);
    printf("Mean RTF = %.1f\n\n", rtf_sum / (float) (num_iterations * num_lengths));
    PV_ORCA_PROFILER_PRINT_DATA;
    printf("\n*************************************************\n");

    (void) fclose(f);
    pv_orca_synthesizer_delete(orca_synthesizer);
    pv_orca_synthesizer_param_delete(synthesizer_param);
    pv_orca_phonemizer_param_delete(phonemizer_param);
    pv_orca_synthesize_params_delete(synthesize_params);

    PV_DUMP_END()
    return EXIT_SUCCESS;
}
