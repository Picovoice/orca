#include <errno.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "io/pv_log.h"
#include "test/pv_test.h"
#include "test/pv_test_memory.h"
#include "cJSON.h"

#include "../../gatekeeper/test/test_pv_gatekeeper_usage_helper.h"
#include "gatekeeper/pv_gatekeeper_usage_animal.h"
#include "hippo/pv_hippo_internal.h"
#include "normalizer/pv_normalizer_token.h"
#include "normalizer/pv_normalizer_util.h"
#include "orca/pv_orca.h"
#include "orca/pv_orca_internal.h"
#include "orca/pv_orca_stream_state.h"
#include "orca/pv_orca_synthesizer.h"
#include "orca/pv_orca_util.h"
#include "normalizer/pv_normalizer_data/pv_normalizer_shared_data.h"
#include "util/pv_string.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#else

#include "ypu/pv_ypu_impl_cpu_internal.h"

#endif

#define max(a, b) ((a) > (b) ? (a) : (b))
#define MEMORY_CHECK_INTERVAL (10)
#define STREAMING_TEST_MEMORY_UPPER_BOUND (15000000)
#define STREAMING_TEST_SENTENCE_REPETITION (400)

static const char *MODEL_PATH = "param/orca_params_en_female.pv";
extern const pv_orca_phonemizer_param_t PV_ORCA_PHONEMIZER_PARAM;
extern const pv_orca_synthesizer_param_t PV_ORCA_SYNTHESIZER_PARAM;
static pv_orca_synthesize_params_t *synthesize_params_object = NULL;

static const char *DEFAULT_SENTENCE = "Orca performance test";

#ifdef __PV_TARGET_PLATFORM_LINUX__

static const char *DEFAULT_SENTENCE_STREAMING = "A fox wandered through the forest at sunrise, wondering why birds seemed to know a secret it didn’t. ";

#endif

static cJSON *ENVIRONMENT = NULL;
static cJSON *RESULTS = NULL;
static cJSON *MEMORY = NULL;
static cJSON *MEMORY_STREAMING = NULL;
static const char *YPU_ITERATIONS = NULL;
static const char *YPU_MEMORY_ITERATIONS = NULL;
static const char *YPU_DURATION = NULL;
static const char *YPU_MACHINE = NULL;
static const char *YPU_DEVICE = NULL;

static int32_t test_iterations = 1;
static int32_t memory_test_iterations = 1;
static int64_t test_duration_usec = 1;
static int64_t *test_durations = NULL;

static cJSON *pv_load_json(const char *filename) {
    char *path = pv_test_module_res_path(filename);
    FILE *fd = fopen(path, "r");
    free(path);
    if (fd == NULL) {
        return NULL;
    }

    fseek(fd, 0, SEEK_END);
    int64_t length = ftell(fd);
    fseek(fd, 0, SEEK_SET);

    char *buffer = malloc(length + 1);
    if (buffer == NULL) {
        fclose(fd);
        return NULL;
    }

    int64_t bytes_read = (int64_t)fread(buffer, 1, length, fd);
    buffer[bytes_read] = '\0';
    fclose(fd);

    cJSON *result = cJSON_Parse(buffer);
    free(buffer);
    if (result == NULL) {
        return NULL;
    }

    return result;
}

static const char *pv_getenv(const char *key, const cJSON *env_json) {
    const char *result = getenv(key);
    if (result != NULL) {
        return result;
    }

    if (env_json != NULL) {
        cJSON *t0 = cJSON_GetObjectItemCaseSensitive(env_json, key);
        if (t0 != NULL) {
            return t0->valuestring;
        }
    }
    return NULL;
}

static int64_t get_now_usec(void) {
    struct timeval time;
    gettimeofday(&time, NULL);
    return (int64_t) time.tv_sec * 1e6 + (int64_t) time.tv_usec;
}

static int compare_int64(const void *a, const void *b) {
    int64_t da = *(const int64_t *)a;
    int64_t db = *(const int64_t *)b;
    return (da > db) - (da < db);
}

static pv_status_t test_performance_pv_orca_setup(void) {
    pv_status_t status = pv_orca_synthesize_params_init(&synthesize_params_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    ENVIRONMENT = pv_load_json("environment.json");

    YPU_ITERATIONS = pv_getenv("YPU_ITERATIONS", ENVIRONMENT);
    YPU_MEMORY_ITERATIONS = pv_getenv("YPU_MEMORY_ITERATIONS", ENVIRONMENT);
    YPU_DURATION = pv_getenv("YPU_DURATION", ENVIRONMENT);
    YPU_MACHINE = pv_getenv("YPU_MACHINE", ENVIRONMENT);
    YPU_DEVICE = pv_getenv("YPU_DEVICE", ENVIRONMENT);

    test_iterations = YPU_ITERATIONS ? atoi(YPU_ITERATIONS) : 1;
    memory_test_iterations = YPU_MEMORY_ITERATIONS ? atoi(YPU_MEMORY_ITERATIONS) : 1;
    double ypu_duration = YPU_DURATION ? atof(YPU_DURATION) : 1.0;
    test_duration_usec = (int64_t) (ypu_duration * 1e6);

    test_durations = malloc(test_iterations * sizeof(int64_t));
    if (!test_durations) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    RESULTS = cJSON_CreateObject();
    if (!RESULTS) {
        return PV_STATUS_OUT_OF_MEMORY;
    }
    cJSON *results_machine = cJSON_CreateObject();
    cJSON_AddItemToObject(RESULTS, YPU_MACHINE, results_machine);

    MEMORY = cJSON_CreateObject();
    if (!MEMORY) {
        return PV_STATUS_OUT_OF_MEMORY;
    }
    cJSON *results_memory = cJSON_CreateObject();
    cJSON_AddItemToObject(MEMORY, YPU_MACHINE, results_memory);

#ifdef __PV_TARGET_PLATFORM_LINUX__

    MEMORY_STREAMING = cJSON_CreateObject();
    if (!MEMORY_STREAMING) {
        return PV_STATUS_OUT_OF_MEMORY;
    }
    cJSON *results_memory_streaming = cJSON_CreateObject();
    cJSON_AddItemToObject(MEMORY_STREAMING, YPU_MACHINE, results_memory_streaming);

#endif

    return PV_STATUS_SUCCESS;
}

static void test_performance_pv_orca_teardown(void) {
    pv_orca_synthesize_params_delete(synthesize_params_object);

    free(test_durations);

    if (MEMORY != NULL) {
        char *memory_results_str = cJSON_PrintUnformatted(MEMORY);
        pv_test_true(
                memory_results_str != NULL,
                "Unable to allocate JSON output string for `orca` memory results");
        if (!memory_results_str) {
            return;
        }

        LOG_INFO("MEMORY JSON=`%s`", memory_results_str);
        cJSON_Delete(MEMORY);
        free(memory_results_str);
    }

    if (MEMORY_STREAMING != NULL) {
        char *memory_streaming_results_str = cJSON_PrintUnformatted(MEMORY_STREAMING);
        pv_test_true(
                memory_streaming_results_str != NULL,
                "Unable to allocate JSON output string for `orca` streaming memory results");
        if (!memory_streaming_results_str) {
            return;
        }

        LOG_INFO("STREAMING MEMORY JSON=`%s`", memory_streaming_results_str);
        cJSON_Delete(MEMORY_STREAMING);
        free(memory_streaming_results_str);
    }

    if (RESULTS != NULL) {
        char *perf_results_str = cJSON_PrintUnformatted(RESULTS);
        pv_test_true(
                perf_results_str != NULL,
                "Unable to allocate JSON output string for `orca` performance results");
        if (!perf_results_str) {
            return;
        }

        LOG_INFO("PERFORMANCE JSON=`%s`", perf_results_str);
        cJSON_Delete(RESULTS);
        free(perf_results_str);
    }

    if (ENVIRONMENT != NULL) {
        cJSON_Delete(ENVIRONMENT);
    }
}

static void test_memory_init_helper(
        pv_ypu_t *ypu,
        pv_orca_t **orca,
        size_t *init_pre,
        size_t *init_post,
        size_t *init_peak,
        size_t *init_system_peak) {
    pv_status_t status = pv_test_memory_start(0);
    if (status != PV_STATUS_SUCCESS) {
        LOG_ERROR("failed to start pv_test_monitor thread", pv_status_to_string(status));
        return;
    }

    char *model_path = pv_test_module_res_path(MODEL_PATH);
    if (!model_path) {
        return;
    }

    char *access_key = NULL;
    status = pv_access_serialize(&BYPASS_ACCESS, &access_key);
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    pv_https_client_factory_t *factory = NULL;
    status = get_https_client_factory_usage_success(&factory);
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    *init_pre = pv_test_memory_get_current_usage();

    status = pv_orca_internal_init(
            access_key,
            factory,
            model_path,
            ypu,
            orca);
    free(access_key);
    free(model_path);
    if (status != PV_STATUS_SUCCESS) {
        LOG_ERROR("pv_orca_init_internal failed with `%s`", pv_status_to_string(status));
        return;
    }

    *init_post = pv_test_memory_get_current_usage();
    pv_test_memory_stop();
    *init_peak = pv_test_memory_get_peak_usage();
    *init_system_peak = pv_test_memory_get_system_peak_usage();
}

static void test_memory_helper(pv_ypu_t *ypu, const char *ypu_device) {
    cJSON *results_machine = cJSON_GetObjectItemCaseSensitive(MEMORY, YPU_MACHINE);
    cJSON *results_device = cJSON_CreateObject();
    cJSON_AddItemToObject(results_machine, ypu_device, results_device);
    cJSON *results_orca = cJSON_CreateObject();
    cJSON_AddItemToObject(results_device, "orca", results_orca);

    pv_orca_t *object = NULL;
    size_t init_pre = 0;
    size_t init_post = 0;
    size_t init_peak = 0;
    size_t init_system_peak = 0;

    test_memory_init_helper(
            ypu,
            &object,
            &init_pre,
            &init_post,
            &init_peak,
            &init_system_peak);

    pv_status_t status = pv_test_memory_start(MEMORY_CHECK_INTERVAL);
    if (status != PV_STATUS_SUCCESS) {
        LOG_ERROR("failed to start pv_test_monitor thread", pv_status_to_string(status));
        return;
    }
    size_t process_pre = pv_test_memory_get_current_usage();

    int32_t num_samples = 0;
    int16_t *pcms = NULL;
    pv_orca_word_alignment_t **alignments = NULL;
    int32_t num_alignments = 0;

    for (int32_t i = 0; i < memory_test_iterations; i++) {
        status = pv_orca_synthesize(
                object,
                DEFAULT_SENTENCE,
                synthesize_params_object,
                &num_samples,
                &pcms,
                &num_alignments,
                &alignments);
        if (status != PV_STATUS_SUCCESS) {
            LOG_ERROR("synthesize failed with `%s`", pv_status_to_string(status));
            return;
        }

        free(pcms);
        pv_orca_word_alignments_delete(num_alignments, alignments);
    }

    pv_orca_delete(object);

    size_t process_post = pv_test_memory_get_current_usage();
    pv_test_memory_stop();
    size_t process_peak = pv_test_memory_get_peak_usage();
    size_t process_system_peak = pv_test_memory_get_system_peak_usage();

    LOG_INFO("init memory usage (bytes) - pre: %d, post: %d, peak: %d, system peak: %d",
        init_pre,
        init_post,
        init_peak,
        init_system_peak);
    LOG_INFO("process memory usage (bytes) - pre: %d, post: %d, peak: %d, system peak: %d",
        process_pre,
        process_post,
        process_peak,
        process_system_peak);

    size_t init_result = max(init_peak, init_system_peak) - init_pre;
    size_t process_result = max(process_peak, process_system_peak) - process_pre;

    cJSON_AddNumberToObject(results_orca, "init_bytes", (double) init_result);
    cJSON_AddNumberToObject(results_orca, "process_bytes", (double) process_result);
}

#ifdef __PV_TARGET_PLATFORM_LINUX__

static void test_memory_streaming_helper(pv_ypu_t *ypu, const char *ypu_device) {
    cJSON *results_machine = cJSON_GetObjectItemCaseSensitive(MEMORY_STREAMING, YPU_MACHINE);
    cJSON *results_device = cJSON_CreateObject();
    cJSON_AddItemToObject(results_machine, ypu_device, results_device);
    cJSON *results_orca = cJSON_CreateObject();
    cJSON_AddItemToObject(results_device, "orca", results_orca);

    pv_orca_t *object = NULL;
    size_t init_pre = 0;
    size_t init_post = 0;
    size_t init_peak = 0;
    size_t init_system_peak = 0;

    test_memory_init_helper(
            ypu,
            &object,
            &init_pre,
            &init_post,
            &init_peak,
            &init_system_peak);

    pv_status_t status = pv_test_memory_start(MEMORY_CHECK_INTERVAL);
    if (status != PV_STATUS_SUCCESS) {
        LOG_ERROR("failed to start pv_test_monitor thread", pv_status_to_string(status));
        return;
    }
    size_t process_pre = pv_test_memory_get_current_usage();

    pv_orca_synthesize_params_t *synthesize_params = NULL;
    status = pv_orca_synthesize_params_init(&synthesize_params);
    if (status != PV_STATUS_SUCCESS) {
        LOG_ERROR("pv_orca_synthesize_params_init failed with `%s`", pv_status_to_string(status));
        return;
    }

    int64_t random_state = 0;
    status = pv_orca_synthesize_params_set_random_state(synthesize_params, random_state);
    if (status != PV_STATUS_SUCCESS) {
        LOG_ERROR("pv_orca_synthesize_params_set_random_state failed with `%s`", pv_status_to_string(status));
        return;
    }

    pv_orca_stream_t *orca_stream = NULL;
    status = pv_orca_stream_open(object, synthesize_params, &orca_stream);
    if (status != PV_STATUS_SUCCESS) {
        LOG_ERROR("pv_orca_stream_open failed with `%s`", pv_status_to_string(status));
        return;
    }

    int32_t num_samples_chunk = 0;
    int16_t *pcm_chunk = NULL;
    char character[PV_NORMALIZER_MAX_NUM_BYTES_PER_CHARACTER] = {0};
    size_t index = 0;
    size_t text_length = strlen(DEFAULT_SENTENCE_STREAMING);
    while (index < text_length * STREAMING_TEST_SENTENCE_REPETITION) {
        size_t i = index % text_length;
        int32_t num_bytes_character = 0;
        status = pv_language_num_bytes_character((unsigned char) DEFAULT_SENTENCE_STREAMING[i], &num_bytes_character);
        if (status != PV_STATUS_SUCCESS) {
            LOG_ERROR("pv_language_num_bytes_character failed with `%s`", pv_status_to_string(status));
            return;
        }

        for (int32_t j = 0; j < num_bytes_character; j++) {
            character[j] = DEFAULT_SENTENCE_STREAMING[i + j];
        }
        character[num_bytes_character] = '\0';

        num_samples_chunk = 0;
        pcm_chunk = NULL;
        status = pv_orca_stream_synthesize(orca_stream, character, &num_samples_chunk, &pcm_chunk);
        pv_orca_pcm_delete(pcm_chunk);
        if (status != PV_STATUS_SUCCESS) {
            LOG_ERROR("pv_orca_stream_synthesize failed with `%s`", pv_status_to_string(status));
            return;
        }

        index += num_bytes_character;
    }

    num_samples_chunk = 0;
    pcm_chunk = NULL;
    status = pv_orca_stream_flush(orca_stream, &num_samples_chunk, &pcm_chunk);
    pv_orca_pcm_delete(pcm_chunk);
    if (status != PV_STATUS_SUCCESS) {
        LOG_ERROR("pv_orca_stream_flush failed with `%s`", pv_status_to_string(status));
        return;
    }

    pv_orca_stream_close(orca_stream);
    pv_orca_synthesize_params_delete(synthesize_params);
    pv_orca_delete(object);

    size_t process_post = pv_test_memory_get_current_usage();
    pv_test_memory_stop();
    size_t process_peak = pv_test_memory_get_peak_usage();
    size_t process_system_peak = pv_test_memory_get_system_peak_usage();

    LOG_INFO("init memory usage (bytes) - pre: %d, post: %d, peak: %d, system peak: %d",
        init_pre,
        init_post,
        init_peak,
        init_system_peak);
    LOG_INFO("process memory usage (bytes) - pre: %d, post: %d, peak: %d, system peak: %d",
        process_pre,
        process_post,
        process_peak,
        process_system_peak);

    cJSON_AddNumberToObject(results_orca, "init_pre_bytes", init_pre);
    cJSON_AddNumberToObject(results_orca, "init_post_bytes", init_post);
    cJSON_AddNumberToObject(results_orca, "init_peak_bytes", init_peak);
    cJSON_AddNumberToObject(results_orca, "init_system_peak_bytes", init_system_peak);

    cJSON_AddNumberToObject(results_orca, "process_pre_bytes", process_pre);
    cJSON_AddNumberToObject(results_orca, "process_post_bytes", process_post);
    cJSON_AddNumberToObject(results_orca, "process_peak_bytes", process_peak);
    cJSON_AddNumberToObject(results_orca, "process_system_peak_bytes", process_system_peak);

    size_t init_result = max(init_peak, init_system_peak) - init_pre;
    size_t process_result = max(process_peak, process_system_peak) - process_pre;
    size_t total_result = max(init_result, process_result);
    pv_test_true(
            total_result <= STREAMING_TEST_MEMORY_UPPER_BOUND,
            "Peak memory usage exceeded upper bound! Upper bound: %d bytes; Using: %d bytes.",
            STREAMING_TEST_MEMORY_UPPER_BOUND,
            total_result);
}

#endif

static void test_performance_helper(pv_ypu_t *ypu, const char *ypu_device) {
    static pv_orca_t *object = NULL;

    cJSON *results_machine = cJSON_GetObjectItemCaseSensitive(RESULTS, YPU_MACHINE);
    cJSON *results_device = cJSON_CreateObject();
    cJSON_AddItemToObject(results_machine, ypu_device, results_device);
    cJSON *results_orca = cJSON_CreateObject();
    cJSON_AddItemToObject(results_device, "orca", results_orca);

    char *model_path = pv_test_module_res_path(MODEL_PATH);
    pv_test_true(model_path != NULL, "Failed to get model_path");
    if (!model_path) {
        return;
    }

    char *access_key = NULL;
    pv_status_t status = pv_access_serialize(&BYPASS_ACCESS, &access_key);
    pv_test_true(
        status == PV_STATUS_SUCCESS,
        "`pv_access_serialize` failed with `%s`",
        pv_status_to_string(status));
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    pv_https_client_factory_t *factory = NULL;
    status = get_https_client_factory_usage_success(&factory);
    pv_test_true(
        status == PV_STATUS_SUCCESS,
        "`get_https_client_factory_usage_success` failed with `%s`",
        pv_status_to_string(status));
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    status = pv_orca_internal_init(
            access_key,
            factory,
            model_path,
            ypu,
            &object);
    free(access_key);
    free(model_path);
    pv_test_true(
        status == PV_STATUS_SUCCESS,
        "`pv_orca_internal_init` failed with `%s`",
        pv_status_to_string(status));
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    int32_t iterations = 1;
    int64_t duration_usec = 0;
    while (iterations < (1 << 30) && duration_usec < test_duration_usec) {
        iterations <<= 1;

        int16_t *pcms[iterations];
        memset(pcms, 0, sizeof(char *) * iterations);
        pv_orca_word_alignment_t **alignments[iterations];
        memset(alignments, 0, sizeof(pv_orca_word_alignment_t **) * iterations);
        int32_t num_alignments[iterations];
        memset(num_alignments, 0, sizeof(int32_t) * iterations);

        int64_t start = get_now_usec();
        for (int32_t i = 0; i < iterations; i++) {
            int32_t num_samples = 0;
            pv_status_t status = pv_orca_synthesize(
                    object,
                    DEFAULT_SENTENCE,
                    synthesize_params_object,
                    &num_samples,
                    &pcms[i],
                    &num_alignments[i],
                    &alignments[i]);
            if (status != PV_STATUS_SUCCESS) {
                LOG_ERROR("processing failed with `%s`", pv_status_to_string(status));
                return;
            }
        }
        duration_usec = get_now_usec() - start;
        LOG_INFO("orca: %d iterations in %.3f sec", iterations, duration_usec / 1e6);

        for (int32_t i = 0; i < iterations; i++) {
            free(pcms[i]);
            pv_orca_word_alignments_delete(num_alignments[i], alignments[i]);
        }
    }

    int64_t last_sleep_time = get_now_usec();
    for (int32_t i = 0; i < test_iterations; i++) {
        int16_t *pcms[iterations];
        memset(pcms, 0, sizeof(char *) * iterations);
        pv_orca_word_alignment_t **alignments[iterations];
        memset(alignments, 0, sizeof(pv_orca_word_alignment_t **) * iterations);
        int32_t num_alignments[iterations];
        memset(num_alignments, 0, sizeof(int32_t) * iterations);

        int64_t start = get_now_usec();
        for (int32_t j = 0; j < iterations; j++) {
            int32_t num_samples = 0;
            pv_status_t status = pv_orca_synthesize(
                    object,
                    DEFAULT_SENTENCE,
                    synthesize_params_object,
                    &num_samples,
                    &pcms[j],
                    &num_alignments[j],
                    &alignments[j]);
            if (status != PV_STATUS_SUCCESS) {
                LOG_ERROR("processing failed with `%s`", pv_status_to_string(status));
                return;
            }
        }
        int64_t now = get_now_usec();
        test_durations[i] = now - start;
        LOG_INFO("orca: %d iterations in %.3f sec", iterations, test_durations[i] / 1e6);

        for (int32_t j = 0; j < iterations; j++) {
            free(pcms[j]);
            pv_orca_word_alignments_delete(num_alignments[j], alignments[j]);
        }

        if (now - last_sleep_time > 3 * 1e6) {
            sleep(1);

#ifdef __PV_TARGET_PLATFORM_IOS__

            // Sleep 25+% of runtime to avoid being kill by iOS
            sleep((now - last_sleep_time) / 1e6 / 4 + 1);

#endif

            last_sleep_time = get_now_usec();
        }

    }

    qsort(test_durations, test_iterations, sizeof(int64_t), compare_int64);

    int64_t median = test_iterations % 2 == 0
                     ? (test_durations[test_iterations / 2 - 1] + test_durations[test_iterations / 2]) / 2
                     : test_durations[test_iterations / 2];

    double median_frame_usec = (double) median / iterations;

    LOG_INFO("%s/%s/%s/%d => %fus",
        YPU_MACHINE,
        ypu_device,
        "orca",
        iterations,
        median_frame_usec);

    cJSON_AddNumberToObject(results_orca, "test_sentence", median_frame_usec);

    pv_orca_delete(object);
}

#ifdef __PV_YPU_CPU_SUPPORT__

static void test_performance_pv_orca_cpu_impl(void) {
    if (pv_getenv("YPU_IGNORE_CPU", ENVIRONMENT) != NULL) {
        LOG_INFO_SIMPLE("    -> Skipping (ignored)...");
        return;
    }

    int32_t max_threads = 1;
    pv_status_t status = pv_ypu_cpu_utils_get_max_threads(&max_threads);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "pv_ypu_cpu_utils_get_max_threads should have returned with %s, got %s",
            pv_status_to_string(PV_STATUS_SUCCESS),
            pv_status_to_string(status));
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

#if defined(__PV_TARGET_PLATFORM_ANDROID__) || defined(__PV_TARGET_PLATFORM_IOS__) || defined(__PV_TARGET_PLATFORM_WASM__)

    max_threads = pv_min_int32(max_threads, 2);

#else

    max_threads = pv_min_int32(max_threads / 2, 8);

#endif

    for (int32_t i = 1; i <= max_threads; i *= 2) {
        pv_ypu_t *ypu = NULL;
        status = pv_ypu_init_cpu(i, &ypu);
        pv_test_true(
                status == PV_STATUS_SUCCESS,
                "pv_ypu_init_cpu should have returned with %s, got %s",
                pv_status_to_string(PV_STATUS_SUCCESS),
                pv_status_to_string(status));
        if (status != PV_STATUS_SUCCESS) {
            return;
        }

        char ypu_device[8];
        snprintf(ypu_device, sizeof(ypu_device), "cpu:%d", i);
        test_performance_helper(ypu, ypu_device);
    }
}

static void test_memory_pv_orca_cpu_impl(void) {
    if (pv_getenv("YPU_IGNORE_CPU", ENVIRONMENT) != NULL) {
        LOG_INFO_SIMPLE("    -> Skipping (ignored)...");
        return;
    }

    pv_ypu_t *ypu = NULL;
    pv_status_t status = pv_ypu_init_cpu(1, &ypu);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "pv_ypu_init_cpu should have returned with %s, got %s",
            pv_status_to_string(PV_STATUS_SUCCESS),
            pv_status_to_string(status));
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    test_memory_helper(ypu, "cpu:1");
}

#ifdef __PV_TARGET_PLATFORM_LINUX__

static void test_memory_pv_orca_streaming_cpu_impl(void) {
    if (pv_getenv("YPU_IGNORE_CPU", ENVIRONMENT) != NULL) {
        LOG_INFO_SIMPLE("    -> Skipping (ignored)...");
        return;
    }

    pv_ypu_t *ypu = NULL;
    pv_status_t status = pv_ypu_init_cpu(1, &ypu);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "pv_ypu_init_cpu should have returned with %s, got %s",
            pv_status_to_string(PV_STATUS_SUCCESS),
            pv_status_to_string(status));
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    test_memory_streaming_helper(ypu, "cpu:1");
}

#endif

#endif

#ifdef __PV_YPU_CUDA_SUPPORT__

static void test_performance_pv_orca_cuda_impl(void) {
    if (pv_getenv("YPU_IGNORE_CUDA", ENVIRONMENT) != NULL) {
        LOG_INFO_SIMPLE("    -> Skipping (ignored)...");
        return;
    }

    pv_ypu_t *ypu = NULL;
    pv_status_t status = pv_ypu_init_cuda(0, &ypu);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "pv_ypu_init_cuda should have returned with %s, got %s",
            pv_status_to_string(PV_STATUS_SUCCESS),
            pv_status_to_string(status));
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    test_performance_helper(ypu, "cuda");
}

#endif

#ifdef __PV_YPU_MCU_SUPPORT__

static void test_performance_pv_orca_mcu_impl(void) {
    if (pv_getenv("YPU_IGNORE_MCU", ENVIRONMENT) != NULL) {
        LOG_INFO_SIMPLE("    -> Skipping (ignored)...");
        return;
    }

    pv_memory_t *memory = NULL;
    pv_status_t status = pv_memory_init(&memory);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "pv_memory_init should have returned with %s, got %s",
            pv_status_to_string(PV_STATUS_SUCCESS),
            pv_status_to_string(status));
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    pv_ypu_t *ypu = NULL;
    status = pv_ypu_init_mcu(memory, &ypu);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "pv_ypu_init_mcu should have returned with %s, got %s",
            pv_status_to_string(PV_STATUS_SUCCESS),
            pv_status_to_string(status));
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    test_performance_helper(ypu, "mcu");
}

#endif

#ifdef __PV_YPU_DIRECTX_SUPPORT__

static void test_performance_pv_orca_directx_impl(void) {
    if (pv_getenv("YPU_IGNORE_DIRECTX", ENVIRONMENT) != NULL) {
        LOG_INFO_SIMPLE("    -> Skipping (ignored)...");
        return;
    }

    pv_ypu_t *ypu = NULL;
    pv_status_t status = pv_ypu_init_directx(0, &ypu);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "pv_ypu_init_directx should have returned with %s, got %s",
            pv_status_to_string(PV_STATUS_SUCCESS),
            pv_status_to_string(status));
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    test_performance_helper(ypu, "directx");
}

#endif

#ifdef __PV_YPU_METAL_SUPPORT__

static void test_performance_pv_orca_metal_impl(void) {
    if (pv_getenv("YPU_IGNORE_METAL", ENVIRONMENT) != NULL) {
        LOG_INFO_SIMPLE("    -> Skipping (ignored)...");
        return;
    }

    pv_ypu_t *ypu = NULL;
    pv_status_t status = pv_ypu_init_metal(0, &ypu);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "pv_ypu_init_metal should have returned with %s, got %s",
            pv_status_to_string(PV_STATUS_SUCCESS),
            pv_status_to_string(status));
    if (status != PV_STATUS_SUCCESS) {
        return;
    }

    test_performance_helper(ypu, "metal");
}

#endif

static const pv_test_case_t PERFORMANCE_PV_ORCA_TEST_CASES[] = {

#ifdef __PV_YPU_CPU_SUPPORT__

#if !defined(__PV_TARGET_PLATFORM_WASM__) || defined(__PV_WASM_PTHREAD__)

        {"cpu memory", test_memory_pv_orca_cpu_impl},

#endif

#ifdef __PV_TARGET_PLATFORM_LINUX__

        {"cpu memory streaming", test_memory_pv_orca_streaming_cpu_impl},

#endif

        {"cpu performance", test_performance_pv_orca_cpu_impl},

#endif

#ifdef __PV_YPU_CUDA_SUPPORT__

        {"cuda", test_performance_pv_orca_cuda_impl},

#endif

#ifdef __PV_YPU_MCU_SUPPORT__

        {"mcu", test_performance_pv_orca_mcu_impl},

#endif

#ifdef __PV_YPU_DIRECTX_SUPPORT__

        {"directx", test_performance_pv_orca_directx_impl},

#endif

#ifdef __PV_YPU_METAL_SUPPORT__

        {"metal", test_performance_pv_orca_metal_impl},

#endif

};

const pv_test_suite_t PERFORMANCE_PV_ORCA_TEST_SUITE = {
        .name = "orca",
        .setup = test_performance_pv_orca_setup,
        .teardown = test_performance_pv_orca_teardown,
        .test_cases = PERFORMANCE_PV_ORCA_TEST_CASES,
        .num_test_cases = PV_ARRAY_LEN(PERFORMANCE_PV_ORCA_TEST_CASES),
};
