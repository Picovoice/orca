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
#include <getopt.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#if !(defined(_WIN32) || defined(_WIN64))

#include <dlfcn.h>

#else

#include <windows.h>

#define UTF8_COMPOSITION_FLAG (0)
#define NULL_TERMINATED       (-1)

#endif

#define DR_WAV_IMPLEMENTATION

#include "dr_wav.h"

#include "pv_orca.h"

#include "pv_speaker.h"

#define MAX_NUM_CHUNKS              (500)
#define MAX_NUM_BYTES_PER_CHARACTER (5)

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

static pv_status_t num_bytes_character(unsigned char c, int32_t *num_bytes) {
    *num_bytes = 0;

    int32_t nb;
    if ((c & 0x80) == 0x00) {
        nb = 1;
    } else if ((c & 0xE0) == 0xC0) {
        nb = 2;
    } else if ((c & 0xF0) == 0xE0) {
        nb = 3;
    } else if ((c & 0xF8) == 0xF0) {
        nb = 4;
    } else {
        return PV_STATUS_INVALID_ARGUMENT;
    }

    *num_bytes = nb;

    return PV_STATUS_SUCCESS;
}

static double get_time() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double) tv.tv_sec + ((double) tv.tv_usec * 1e-6);
}

static void print_usage(const char *program_name) {
    fprintf(
            stdout,
            "Usage: %s [-l LIBRARY_PATH -m MODEL_PATH -a ACCESS_KEY -t TEXT -o OUTPUT_PATH]\n",
            program_name);
}

typedef struct pcm_chunk pcm_chunk_t;

struct pcm_chunk {
    int32_t num_samples;
    int16_t *pcm;
    pcm_chunk_t *next;
};

static pv_status_t pcm_chunk_init(
        int32_t num_samples,
        int16_t *pcm,
        pcm_chunk_t **chunk) {
    *chunk = NULL;

    pcm_chunk_t *c = calloc(1, sizeof(pcm_chunk_t));
    if (!c) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    c->pcm = pcm;
    c->num_samples = num_samples;
    c->next = NULL;

    *chunk = c;

    return PV_STATUS_SUCCESS;
}

static pv_status_t pcm_chunk_delete(pcm_chunk_t *chunk) {
    if (chunk) {
        free(chunk->pcm);
        free(chunk);
    }
    return PV_STATUS_SUCCESS;
}

void print_error_message(char **message_stack, int32_t message_stack_depth) {
    for (int32_t i = 0; i < message_stack_depth; i++) {
        fprintf(stderr, "  [%d] %s\n", i, message_stack[i]);
    }
}

void handle_error(
        char **message_stack,
        int32_t message_stack_depth,
        pv_status_t (*pv_get_error_stack_func)(char ***, int32_t *),
        void (*pv_free_error_stack_func)(char **),
        const char *(*pv_status_to_string_func)(pv_status_t)) {
    pv_status_t error_status = pv_get_error_stack_func(&message_stack, &message_stack_depth);

    if (error_status != PV_STATUS_SUCCESS) {
        fprintf(stderr, ".\nUnable to get Orca error state with '%s'\n", pv_status_to_string_func(error_status));
        exit(EXIT_FAILURE);
    }

    if (message_stack_depth > 0) {
        fprintf(stderr, ":\n");
        for (int32_t i = 0; i < message_stack_depth; i++) {
            fprintf(stderr, "  [%d] %s\n", i, message_stack[i]);
        }
    }

    pv_free_error_stack_func(message_stack);
}

static void show_audio_devices(void) {
    char **device_list = NULL;
    int32_t device_list_length = 0;

    pv_speaker_status_t status = pv_speaker_get_available_devices(&device_list_length, &device_list);
    if (status != PV_SPEAKER_STATUS_SUCCESS) {
        fprintf(stderr, "failed to get audio devices with `%s`.\n", pv_speaker_status_to_string(status));
        exit(1);
    }

    for (int32_t i = 0; i < device_list_length; i++) {
        fprintf(stdout, "[%d] %s\n", i, device_list[i]);
    }

    pv_speaker_free_available_devices(device_list_length, device_list);
}

typedef struct {
    int16_t *pcm;
    int32_t num_samples;
} NodeData;

typedef struct Node {
    int16_t *pcm;
    int32_t num_samples;
    struct Node *prev;
    struct Node *next;
} Node;

typedef struct Deque {
    Node *front;
    Node *rear;
    size_t size;
} Deque;

Deque *createDeque();
Node *createNode(int16_t *pcm, int32_t num_samples);
void pushFront(Deque *deque, int16_t *pcm, int32_t num_samples);
void pushRear(Deque *deque, int16_t *pcm, int32_t num_samples);
NodeData popFront(Deque *deque);
void popRear(Deque *deque);
int isEmpty(Deque *deque);
void freeDeque(Deque *deque);

Deque *createDeque() {
    Deque *deque = (Deque *) malloc(sizeof(Deque));
    if (deque == NULL) {
        perror("Failed to create deque");
        exit(EXIT_FAILURE);
    }
    deque->front = deque->rear = NULL;
    deque->size = 0;
    return deque;
}

Node *createNode(int16_t *pcm, int32_t num_samples) {
    Node *node = (Node *) malloc(sizeof(Node));
    if (node == NULL) {
        perror("Failed to create node");
        exit(EXIT_FAILURE);
    }
    node->pcm = pcm;
    node->num_samples = num_samples;
    node->prev = node->next = NULL;
    return node;
}

void pushFront(Deque *deque, int16_t *pcm, int32_t num_samples) {
    Node *node = createNode(pcm, num_samples);
    if (isEmpty(deque)) {
        deque->front = deque->rear = node;
    } else {
        node->next = deque->front;
        deque->front->prev = node;
        deque->front = node;
    }
    deque->size++;
}

void pushRear(Deque *deque, int16_t *pcm, int32_t num_samples) {
    Node *node = createNode(pcm, num_samples);
    if (isEmpty(deque)) {
        deque->front = deque->rear = node;
    } else {
        node->prev = deque->rear;
        deque->rear->next = node;
        deque->rear = node;
    }
    deque->size++;
}

NodeData popFront(Deque *deque) {
    NodeData data = {NULL, 0};
    if (isEmpty(deque)) {
        printf("Deque is empty\n");
        return data;
    }
    Node *temp = deque->front;
    data.pcm = temp->pcm;
    data.num_samples = temp->num_samples;
    deque->front = deque->front->next;
    if (deque->front != NULL) {
        deque->front->prev = NULL;
    } else {
        deque->rear = NULL;
    }
    free(temp);
    deque->size--;
    return data;
}

void popRear(Deque *deque) {
    if (isEmpty(deque)) {
        printf("Deque is empty\n");
        return;
    }
    Node *temp = deque->rear;
    deque->rear = deque->rear->prev;
    if (deque->rear != NULL) {
        deque->rear->next = NULL;
    } else {
        deque->front = NULL;
    }
    free(temp);
    deque->size--;
}

int isEmpty(Deque *deque) {
    return deque->size == 0;
}

void freeDeque(Deque *deque) {
    while (!isEmpty(deque)) {
        popFront(deque);
    }
    free(deque);
}

typedef struct {
    pv_speaker_t *speaker;
    Deque *deque;
} ThreadData;

// Thread function
void *threadFunction(void *arg) {
    // Cast the argument to ThreadData*
    ThreadData *data = (ThreadData *) arg;

    // Access the struct members
    Deque *deque = data->deque;
    pv_speaker_t *speaker = data->speaker;

    while (true) {
        if (!isEmpty(deque)) {
            NodeData node_data = popFront(deque);
            if (node_data.num_samples == 0) {
                break;
            }
            int32_t written_length = 0;
            pv_speaker_status_t speaker_status = pv_speaker_write(speaker, (int8_t *) node_data.pcm, node_data.num_samples, &written_length);
            if (speaker_status != PV_SPEAKER_STATUS_SUCCESS) {
                fprintf(stderr, "Failed to write pcm with %s.\n", pv_speaker_status_to_string(speaker_status));
                exit(1);
            }
            if (written_length < node_data.num_samples) {
                pushFront(deque, &node_data.pcm[written_length * 16 / 2], node_data.num_samples - written_length);
            }
        } else {
            usleep(100 * 1000);
        }
    }

    return NULL;
}

int32_t picovoice_main(int32_t argc, char **argv) {
    const char *library_path = NULL;
    const char *model_path = NULL;
    const char *access_key = NULL;
    const char *text = NULL;
    const char *output_path = NULL;
    int32_t device_index = -1;

    int32_t c;
    while ((c = getopt_long(argc, argv, "l:m:a:t:o:i:s", long_options, NULL)) != -1) {
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
            case 'i':
                device_index = (int32_t) strtol(optarg, NULL, 10);
                if (device_index < -1) {
                    fprintf(stderr, "device index should be either `-1` (default) or a non-negative valid index\n");
                    exit(1);
                }
                break;
            case 's':
                show_audio_devices();
                exit(0);
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

    pv_status_t (*pv_orca_sample_rate_func)(pv_orca_t *, int32_t *) =
            load_symbol(orca_library, "pv_orca_sample_rate");
    if (!pv_orca_sample_rate_func) {
        print_dl_error("Failed to load 'pv_orca_sample_rate'");
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

    void (*pv_orca_pcm_delete_func)(int16_t *) = load_symbol(orca_library, "pv_orca_pcm_delete");
    if (!pv_orca_pcm_delete_func) {
        print_dl_error("Failed to load 'pv_orca_pcm_delete'");
        exit(EXIT_FAILURE);
    }

    pv_status_t (*pv_orca_stream_open_func)(
            pv_orca_t *,
            const pv_orca_synthesize_params_t *,
            pv_orca_stream_t **) = load_symbol(orca_library, "pv_orca_stream_open");
    if (!pv_orca_stream_open_func) {
        print_dl_error("Failed to load 'pv_orca_stream_open'");
        exit(EXIT_FAILURE);
    }

    pv_status_t (*pv_orca_stream_synthesize_func)(
            pv_orca_stream_t *,
            const char *,
            int32_t *,
            int16_t **) = load_symbol(orca_library, "pv_orca_stream_synthesize");
    if (!pv_orca_stream_synthesize_func) {
        print_dl_error("Failed to load 'pv_orca_stream_synthesize'");
        exit(EXIT_FAILURE);
    }

    pv_status_t (*pv_orca_stream_flush_func)(
            pv_orca_stream_t *,
            int32_t *,
            int16_t **) = load_symbol(orca_library, "pv_orca_stream_flush");
    if (!pv_orca_stream_flush_func) {
        print_dl_error("Failed to load 'pv_orca_stream_flush'");
        exit(EXIT_FAILURE);
    }

    void (*pv_orca_stream_close_func)(pv_orca_stream_t *) = load_symbol(orca_library, "pv_orca_stream_close");
    if (!pv_orca_stream_close_func) {
        print_dl_error("Failed to load 'pv_orca_stream_close'");
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

    char **message_stack = NULL;
    int32_t message_stack_depth = 0;

    fprintf(stdout, "Orca version: %s\n\n", pv_orca_version_func());

    double time_before_init = get_time();

    pv_orca_t *orca = NULL;
    pv_status_t orca_status = pv_orca_init_func(access_key, model_path, &orca);
    if (orca_status != PV_STATUS_SUCCESS) {
        fprintf(stderr, "Failed to create an instance of Orca with `%s`", pv_status_to_string_func(orca_status));
        handle_error(
                message_stack,
                message_stack_depth,
                pv_get_error_stack_func,
                pv_free_error_stack_func,
                pv_status_to_string_func);
        exit(EXIT_FAILURE);
    }

    double init_sec = get_time() - time_before_init;
    fprintf(stdout, "Initialized Orca in %.1f sec\n", init_sec);

    int32_t sample_rate = 0;
    pv_status_t status = pv_orca_sample_rate_func(orca, &sample_rate);
    if (status != PV_STATUS_SUCCESS) {
        fprintf(stderr, "Failed to get Orca sample rate with `%s`", pv_status_to_string_func(status));
        handle_error(
                message_stack,
                message_stack_depth,
                pv_get_error_stack_func,
                pv_free_error_stack_func,
                pv_status_to_string_func);
        exit(EXIT_FAILURE);
    }

    pv_speaker_t *speaker = NULL;
    pv_speaker_status_t speaker_status = pv_speaker_init(sample_rate, 16, 20, device_index, &speaker);
    if (speaker_status != PV_SPEAKER_STATUS_SUCCESS) {
        fprintf(stderr, "Failed to initialize audio device with `%s`.\n", pv_speaker_status_to_string(speaker_status));
        exit(1);
    }

    speaker_status = pv_speaker_start(speaker);
    if (speaker_status != PV_SPEAKER_STATUS_SUCCESS) {
        fprintf(stderr, "Failed to start device with %s.\n", pv_speaker_status_to_string(speaker_status));
        exit(1);
    }

    drwav_data_format format;
    format.container = drwav_container_riff;
    format.format = DR_WAVE_FORMAT_PCM;
    format.channels = 1;
    format.sampleRate = sample_rate;
    format.bitsPerSample = 16;

    drwav output_file;

#if defined(_WIN32) || defined(_WIN64)

    int output_path_wchars_num = MultiByteToWideChar(CP_UTF8, UTF8_COMPOSITION_FLAG, output_path, NULL_TERMINATED, NULL, 0);
    wchar_t output_path_w[output_path_wchars_num];
    MultiByteToWideChar(CP_UTF8, UTF8_COMPOSITION_FLAG, output_path, NULL_TERMINATED, output_path_w, output_path_wchars_num);
    unsigned int drwav_init_file_status = drwav_init_file_write_w(&output_file, output_path_w, &format, NULL);

#else

    unsigned int drwav_init_file_status = drwav_init_file_write(&output_file, output_path, &format, NULL);

#endif

    if (!drwav_init_file_status) {
        fprintf(stderr, "Failed to open the output wav file at '%s'.", output_path);
        exit(EXIT_FAILURE);
    }

    pv_orca_synthesize_params_t *synthesize_params = NULL;
    pv_status_t synthesize_params_status = pv_orca_synthesize_params_init_func(&synthesize_params);
    if (synthesize_params_status != PV_STATUS_SUCCESS) {
        fprintf(
                stderr,
                "Failed to create an instance of Orca synthesize params with `%s`",
                pv_status_to_string_func(synthesize_params_status));
        handle_error(
                message_stack,
                message_stack_depth,
                pv_get_error_stack_func,
                pv_free_error_stack_func,
                pv_status_to_string_func);
        exit(EXIT_FAILURE);
    }

    fprintf(stdout, "\nSynthesizing text `%s` \n", text);

    int32_t num_samples_chunks[MAX_NUM_CHUNKS] = {0};
    double start_chunks[MAX_NUM_CHUNKS] = {0};
    start_chunks[0] = get_time();
    double end_chunks[MAX_NUM_CHUNKS] = {0};
    int32_t num_chunks = 0;

    pcm_chunk_t *pcm_chunk_prev = NULL;
    pcm_chunk_t *pcm_chunk_head = NULL;

    pv_orca_stream_t *orca_stream = NULL;
    pv_status_t stream_open_status = pv_orca_stream_open_func(orca, synthesize_params, &orca_stream);
    if (stream_open_status != PV_STATUS_SUCCESS) {
        fprintf(stderr, "Error opening stream");
        handle_error(
                message_stack,
                message_stack_depth,
                pv_get_error_stack_func,
                pv_free_error_stack_func,
                pv_status_to_string_func);
        exit(EXIT_FAILURE);
    }

    Deque *deque = createDeque();

    pthread_t thread;
    ThreadData data = {speaker, deque};

    if (pthread_create(&thread, NULL, threadFunction, &data)) {
        fprintf(stderr, "Error creating thread\n");
        return 1;
    }

    char character[MAX_NUM_BYTES_PER_CHARACTER] = {0};
    for (int32_t i = 0; i < (int32_t) strlen(text); i++) {
        if (num_chunks > (MAX_NUM_CHUNKS - 1)) {
            fprintf(stderr, "Trying to synthesize too many chunks. Only `%d` chunks are supported.\n", MAX_NUM_CHUNKS);
            exit(EXIT_FAILURE);
        }

        int32_t num_bytes = 0;
        status = num_bytes_character((unsigned char) text[i], &num_bytes);
        if (status != PV_STATUS_SUCCESS) {
            fprintf(stderr, "Error getting number of bytes for character: `%c`", text[i]);
            exit(EXIT_FAILURE);
        }

        for (int32_t j = 0; j < num_bytes; j++) {
            character[j] = text[i + j];
        }
        character[num_bytes] = '\0';

        int32_t num_samples_chunk = 0;
        int16_t *pcm_chunk = NULL;
        status = pv_orca_stream_synthesize_func(orca_stream, character, &num_samples_chunk, &pcm_chunk);
        if (status != PV_STATUS_SUCCESS) {
            fprintf(stderr, "Error adding token: `%s`", character);
            handle_error(
                    message_stack,
                    message_stack_depth,
                    pv_get_error_stack_func,
                    pv_free_error_stack_func,
                    pv_status_to_string_func);
            exit(EXIT_FAILURE);
        }

        if (num_samples_chunk > 0) {
            if (pcm_chunk_prev == NULL) {
                pcm_chunk_init(num_samples_chunk, pcm_chunk, &pcm_chunk_prev);
                pcm_chunk_head = pcm_chunk_prev;
            } else {
                pcm_chunk_init(num_samples_chunk, pcm_chunk, &(pcm_chunk_prev->next));
                pcm_chunk_prev = pcm_chunk_prev->next;
            }

            double timestamp = get_time();
            num_samples_chunks[num_chunks] = num_samples_chunk;
            end_chunks[num_chunks++] = timestamp;
            start_chunks[num_chunks] = timestamp;

            pushRear(deque, pcm_chunk, num_samples_chunk);
        }
    }

    int32_t num_samples_chunk = 0;
    int16_t *pcm_chunk = NULL;
    status = pv_orca_stream_flush_func(orca_stream, &num_samples_chunk, &pcm_chunk);
    if (status != PV_STATUS_SUCCESS) {
        fprintf(stderr, "Error flushing Orca stream");
        handle_error(
                message_stack,
                message_stack_depth,
                pv_get_error_stack_func,
                pv_free_error_stack_func,
                pv_status_to_string_func);
        exit(EXIT_FAILURE);
    }

    pushRear(deque, NULL, 0);

    if (pthread_join(thread, NULL)) {
        fprintf(stderr, "Error joining thread\n");
        return 2;
    }
    freeDeque(deque);

    if (num_samples_chunk > 0) {
        int32_t written_length = 0;
        int8_t *pcm_ptr = (int8_t *) pcm_chunk;
        speaker_status = pv_speaker_flush(speaker, pcm_ptr, num_samples_chunk, &written_length);
        if (speaker_status != PV_SPEAKER_STATUS_SUCCESS) {
            fprintf(stderr, "Failed to flush pcm with %s.\n", pv_speaker_status_to_string(speaker_status));
            exit(1);
        }

        if (pcm_chunk_prev == NULL) {
            pcm_chunk_init(num_samples_chunk, pcm_chunk, &pcm_chunk_prev);
            pcm_chunk_head = pcm_chunk_prev;
        } else {
            pcm_chunk_init(num_samples_chunk, pcm_chunk, &(pcm_chunk_prev->next));
        }

        double timestamp = get_time();
        num_samples_chunks[num_chunks] = num_samples_chunk;
        end_chunks[num_chunks++] = timestamp;
        start_chunks[num_chunks] = timestamp;
    }

    pv_orca_stream_close_func(orca_stream);
    pv_orca_synthesize_params_delete_func(synthesize_params);
    pv_orca_delete_func(orca);

    speaker_status = pv_speaker_stop(speaker);
    if (speaker_status != PV_SPEAKER_STATUS_SUCCESS) {
        fprintf(stderr, "Failed to stop device with %s.\n", pv_speaker_status_to_string(speaker_status));
        exit(1);
    }

    int32_t num_samples = 0;
    pcm_chunk_t *pcm_chunk_iter = pcm_chunk_head;
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

    pcm_chunk_iter = pcm_chunk_head;
    while (pcm_chunk_iter != NULL) {
        pcm_chunk_t *tmp = pcm_chunk_iter;
        pcm_chunk_iter = pcm_chunk_iter->next;
        pcm_chunk_delete(tmp);
    }

    if ((int32_t) drwav_write_pcm_frames(&output_file, num_samples, pcm) != num_samples) {
        fprintf(stderr, "Failed to write to output file.\n");
        exit(EXIT_FAILURE);
    }

    drwav_uninit(&output_file);
    free(pcm);

    fprintf(
            stdout,
            "\nGenerated %d audio chunk%s in %.2f seconds.\n",
            num_chunks,
            num_chunks == 1 ? "" : "s",
            end_chunks[num_chunks - 1] - start_chunks[0]);

    for (int32_t i = 0; i < num_chunks; i++) {
        float num_seconds = (float) num_samples_chunks[i] / (float) sample_rate;
        double process_time = end_chunks[i] - start_chunks[i];
        fprintf(
                stdout,
                "Audio chunk #%d: length: %.2f s, processing time %.2f s\n",
                i,
                num_seconds,
                process_time);
    }

    fprintf(stdout, "\nSaved final audio to `%s`\n", output_path);

    close_dl(orca_library);

    return EXIT_SUCCESS;
}

int32_t main(int argc, char *argv[]) {

#if defined(_WIN32) || defined(_WIN64)

#define UTF8_COMPOSITION_FLAG (0)
#define NULL_TERMINATED       (-1)

    LPWSTR *wargv = CommandLineToArgvW(GetCommandLineW(), &argc);
    if (wargv == NULL) {
        fprintf(stderr, "CommandLineToArgvW failed\n");
        exit(1);
    }

    char *utf8_argv[argc];

    for (int32_t i = 0; i < argc; ++i) {
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
