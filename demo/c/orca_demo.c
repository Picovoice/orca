/*
Copyright 2024 Picovoice Inc.

You may not use this file except in compliance with the license. A copy of
the license is located in the "LICENSE" file accompanying this source.

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
License for the specific language governing permissions and limitations under
the License.
*/

#if !(defined(_WIN32) || defined(_WIN64))

#include <dlfcn.h>

#endif

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#if defined(_WIN32) || defined(_WIN64)

#include <windows.h>

#endif

#include "pv_orca.h"

static void *open_dl(const char *dl_path) {

#if defined(_WIN32) || defined(_WIN64)

    return LoadLibrary(dl_path);

#else

    return dlopen(dl_path, RTLD_NOW);

#endif
}

static void *load_symbol(void *handle, const char *symbol) {

#if defined(_WIN32) || defined(_WIN64)

    return GetProcAddress((HMODULE) handle, symbol);

#else

    return dlsym(handle, symbol);

#endif
}

static void close_dl(void *handle) {

#if defined(_WIN32) || defined(_WIN64)

    FreeLibrary((HMODULE) handle);

#else

    dlclose(handle);

#endif
}

static void print_dl_error(const char *message) {

#if defined(_WIN32) || defined(_WIN64)

    fprintf(stderr, "%s with code `%lu`.\n", message, GetLastError());

#else

    fprintf(stderr, "%s with `%s`.\n", message, dlerror());

#endif
}

static struct option long_options[] = {
        {"access_key",   required_argument, NULL, 'a'},
        {"library_path", required_argument, NULL, 'l'},
        {"model_path",   required_argument, NULL, 'm'},
        {"text",         required_argument, NULL, 't'},
        {"output_path",  required_argument, NULL, 'o'},
};

static void print_usage(const char *program_name) {
    fprintf(
            stdout,
            "Usage: %s [-l LIBRARY_PATH -m MODEL_PATH -a ACCESS_KEY -t TEXT -o OUTPUT_PATH]\n",
            program_name);
}

void print_error_message(char **message_stack, int32_t message_stack_depth) {
    for (int32_t i = 0; i < message_stack_depth; i++) {
        fprintf(stderr, "  [%d] %s\n", i, message_stack[i]);
    }
}

int picovoice_main(int argc, char **argv) {
    const char *library_path = NULL;
    const char *model_path = NULL;
    const char *access_key = NULL;
    const char *text = NULL;
    const char *output_path = NULL;

    int c;
    while ((c = getopt_long(argc, argv, "l:m:a:t:o:", long_options, NULL)) != -1) {
        switch (c) {
            case 'l':
                library_path = optarg;
                break;
            case 'm':
                model_path = optarg;
                break;
            case 'a':
                access_key = optarg;
                break;
            case 't':
                text = optarg;
                break;
            case 'o':
                output_path = optarg;
                break;
            default:
                exit(EXIT_FAILURE);
        }
    }

    if (!library_path || !model_path || !access_key || !text || !output_path) {
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }

    void *orca_library = open_dl(library_path);
    if (!orca_library) {
        fprintf(stderr, "Failed to open library at `%s`.\n", library_path);
        exit(EXIT_FAILURE);
    }

    const char *(*pv_status_to_string_func)(pv_status_t) =
            load_symbol(orca_library, "pv_status_to_string");
    if (!pv_status_to_string_func) {
        print_dl_error("Failed to load 'pv_status_to_string'");
        exit(EXIT_FAILURE);
    }

    pv_status_t (*pv_orca_init_func)(const char *, const char *, pv_orca_t **) =
            load_symbol(orca_library, "pv_orca_init");
    if (!pv_orca_init_func) {
        print_dl_error("Failed to load 'pv_orca_init'");
        exit(EXIT_FAILURE);
    }

    void (*pv_orca_delete_func)(pv_orca_t *) = load_symbol(orca_library, "pv_orca_delete");
    if (!pv_orca_delete_func) {
        print_dl_error("Failed to load 'pv_orca_delete'");
        exit(EXIT_FAILURE);
    }

    pv_status_t (*pv_orca_synthesize_params_init_func)(pv_orca_synthesize_params_t **) =
            load_symbol(orca_library, "pv_orca_synthesize_params_init");
    if (!pv_orca_synthesize_params_init_func) {
        print_dl_error("Failed to load 'pv_orca_synthesize_params_init'");
        exit(EXIT_FAILURE);
    }

    void (*pv_orca_synthesize_params_delete_func)(pv_orca_synthesize_params_t *) =
            load_symbol(orca_library, "pv_orca_synthesize_params_delete");
    if (!pv_orca_synthesize_params_delete_func) {
        print_dl_error("Failed to load 'pv_orca_synthesize_params_delete'");
        exit(EXIT_FAILURE);
    }

    pv_status_t (*pv_orca_synthesize_to_file_func)(
            pv_orca_t *,
            const char *,
            const pv_orca_synthesize_params_t *,
            const char *,
            int32_t *num_alignments,
            pv_orca_word_alignment_t ***alignments) =
            load_symbol(orca_library, "pv_orca_synthesize_to_file");
    if (!pv_orca_synthesize_to_file_func) {
        print_dl_error("Failed to load 'pv_orca_synthesize_to_file'");
        exit(EXIT_FAILURE);
    }

    pv_status_t (*pv_orca_word_alignments_delete_func)(int32_t, pv_orca_word_alignment_t **) =
            load_symbol(orca_library, "pv_orca_word_alignments_delete");
    if (!pv_orca_word_alignments_delete_func) {
        print_dl_error("Failed to load 'pv_orca_word_alignments_delete'");
        exit(EXIT_FAILURE);
    }

    void (*pv_orca_pcm_delete_func)(int16_t *) = load_symbol(orca_library, "pv_orca_pcm_delete");
    if (!pv_orca_pcm_delete_func) {
        print_dl_error("Failed to load 'pv_orca_pcm_delete'");
        exit(EXIT_FAILURE);
    }

    const char *(*pv_orca_version_func)() = load_symbol(orca_library, "pv_orca_version");
    if (!pv_orca_version_func) {
        print_dl_error("Failed to load 'pv_orca_version'");
        exit(EXIT_FAILURE);
    }

    pv_status_t (*pv_get_error_stack_func)(char ***, int32_t *) = load_symbol(orca_library, "pv_get_error_stack");
    if (!pv_get_error_stack_func) {
        print_dl_error("Failed to load 'pv_get_error_stack'");
        exit(EXIT_FAILURE);
    }

    void (*pv_free_error_stack_func)(char **) = load_symbol(orca_library, "pv_free_error_stack");
    if (!pv_free_error_stack_func) {
        print_dl_error("Failed to load 'pv_free_error_stack'");
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "v%s\n\n", pv_orca_version_func());

    struct timeval before;
    gettimeofday(&before, NULL);

    char **message_stack = NULL;
    int32_t message_stack_depth = 0;
    pv_status_t error_status;

    pv_orca_t *orca = NULL;
    pv_status_t orca_status = pv_orca_init_func(access_key, model_path, &orca);
    if (orca_status != PV_STATUS_SUCCESS) {
        fprintf(stderr, "Failed to create an instance of Orca with `%s`", pv_status_to_string_func(orca_status));
        error_status = pv_get_error_stack_func(&message_stack, &message_stack_depth);
        if (error_status != PV_STATUS_SUCCESS) {
            fprintf(stderr, ".\nUnable to get Orca error state with `%s`.\n", pv_status_to_string_func(error_status));
            exit(EXIT_FAILURE);
        }

        if (message_stack_depth > 0) {
            fprintf(stderr, ":\n");
            print_error_message(message_stack, message_stack_depth);
            pv_free_error_stack_func(message_stack);
        }
        exit(EXIT_FAILURE);
    }

    struct timeval after;
    gettimeofday(&after, NULL);

    double init_sec =
            ((double) (after.tv_sec - before.tv_sec) +
             ((double) (after.tv_usec - before.tv_usec)) * 1e-6);
    fprintf(stdout, "Initialized Orca in %.1f sec\n", init_sec);

    pv_orca_synthesize_params_t *synthesize_params = NULL;
    pv_status_t synthesize_params_status = pv_orca_synthesize_params_init_func(&synthesize_params);
    if (synthesize_params_status != PV_STATUS_SUCCESS) {
        fprintf(
                stderr,
                "Failed to create an instance of Orca synthesize params with `%s`",
                pv_status_to_string_func(synthesize_params_status));
        error_status = pv_get_error_stack_func(&message_stack, &message_stack_depth);
        if (error_status != PV_STATUS_SUCCESS) {
            fprintf(
                    stderr,
                    ".\nUnable to get Orca synthesize params error state with `%s`.\n",
                    pv_status_to_string_func(error_status));
            exit(EXIT_FAILURE);
        }

        if (message_stack_depth > 0) {
            fprintf(stderr, ":\n");
            print_error_message(message_stack, message_stack_depth);
            pv_free_error_stack_func(message_stack);
        }
        exit(EXIT_FAILURE);
    }

    double proc_sec = 0.;
    gettimeofday(&before, NULL);

    fprintf(stdout, "Synthesizing text `%s` ...\n", text);

    int32_t num_alignments = 0;
    pv_orca_word_alignment_t **alignments = NULL;
    pv_status_t synthesize_status = pv_orca_synthesize_to_file_func(
            orca,
            text,
            synthesize_params,
            output_path,
            &num_alignments,
            &alignments);
    if (synthesize_status != PV_STATUS_SUCCESS) {
        fprintf(
                stderr,
                "Failed to synthesize text with `%s`",
                pv_status_to_string_func(synthesize_params_status));
        error_status = pv_get_error_stack_func(&message_stack, &message_stack_depth);
        if (error_status != PV_STATUS_SUCCESS) {
            fprintf(
                    stderr,
                    ".\nUnable to get Orca synthesize error state with `%s`.\n",
                    pv_status_to_string_func(error_status));
            exit(EXIT_FAILURE);
        }

        if (message_stack_depth > 0) {
            fprintf(stderr, ":\n");
            print_error_message(message_stack, message_stack_depth);
            pv_free_error_stack_func(message_stack);
        }
        exit(EXIT_FAILURE);
    }

    gettimeofday(&after, NULL);

    proc_sec +=
            ((double) (after.tv_sec - before.tv_sec) +
             ((double) (after.tv_usec - before.tv_usec)) * 1e-6);

    fprintf(stdout, "Synthesized text in %.1f sec\n", proc_sec);
    fprintf(stdout, "Saved audio to `%s`\n", output_path);

    pv_status_t delete_status = pv_orca_word_alignments_delete_func(num_alignments, alignments);
    if (delete_status != PV_STATUS_SUCCESS) {
        fprintf(stderr, "Failed to delete word alignments with `%s`.\n", pv_status_to_string_func(delete_status));
        exit(EXIT_FAILURE);
    }

    pv_orca_synthesize_params_delete_func(synthesize_params);
    pv_orca_delete_func(orca);
    close_dl(orca_library);

    return EXIT_SUCCESS;
}

int main(int argc, char *argv[]) {

#if defined(_WIN32) || defined(_WIN64)

#define UTF8_COMPOSITION_FLAG (0)
#define NULL_TERMINATED       (-1)

    LPWSTR *wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (wargv == NULL) {
        fprintf(stderr, "CommandLineToArgvW failed\n");
        exit(1);
    }

    char *utf8_argv[argc];

    for (int i = 0; i < argc; ++i) {
        // WideCharToMultiByte:
        // https://docs.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-widechartomultibyte
        int arg_chars_num =
                WideCharToMultiByte(CP_UTF8, UTF8_COMPOSITION_FLAG, wargv[i], NULL_TERMINATED, NULL, 0, NULL, NULL);
        utf8_argv[i] = (char *) malloc(arg_chars_num * sizeof(char));
        if (!utf8_argv[i]) {
            fprintf(stderr, "failed to to allocate memory for converting args");
        }
        WideCharToMultiByte(CP_UTF8, UTF8_COMPOSITION_FLAG, wargv[i], NULL_TERMINATED, utf8_argv[i], arg_chars_num, NULL, NULL);
    }

    LocalFree(wargv);
    argv = utf8_argv;

#endif

    int result = picovoice_main(argc, argv);

#if defined(_WIN32) || defined(_WIN64)

    for (int i = 0; i < argc; ++i) {
        free(utf8_argv[i]);
    }

#endif

    return result;
}
