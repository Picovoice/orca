#include <errno.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "io/pv_log.h"
#include "test/pv_test.h"
#include "cJSON.h"

#include "../../gatekeeper/test/test_pv_gatekeeper_usage_helper.h"
#include "gatekeeper/pv_gatekeeper_usage_animal.h"
#include "hippo/pv_hippo_internal.h"
#include "orca/normalizer/pv_normalizer_token.h"
#include "orca/normalizer/pv_normalizer_util.h"
#include "orca/pv_orca.h"
#include "orca/pv_orca_internal.h"
#include "orca/pv_orca_stream_state.h"
#include "orca/pv_orca_synthesizer.h"
#include "orca/pv_orca_util.h"
#include "orca/normalizer/pv_normalizer_data/pv_normalizer_shared_data.h"
#include "util/pv_string.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif


static const char *MODEL_PATH = "param/orca_params_en_female.pv";
extern const pv_orca_phonemizer_param_t PV_ORCA_PHONEMIZER_PARAM;
extern const pv_orca_synthesizer_param_t PV_ORCA_SYNTHESIZER_PARAM;
static pv_orca_t *orca_object = NULL;
static pv_orca_synthesize_params_t *synthesize_params_object = NULL;

static const char *DEFAULT_SENTENCE = "Orca performance test";

static cJSON *ENVIRONMENT = NULL;
static cJSON *RESULTS = NULL;
static const char *YPU_ITERATIONS = NULL;
static const char *YPU_DURATION = NULL;
static const char *YPU_MACHINE = NULL;
static const char *YPU_DEVICE = NULL;

static int32_t test_iterations = 1;
static int64_t test_duration_usec = 1;
static int64_t *test_durations = NULL;

cJSON *pv_load_json(const char *filename) {
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

const char *pv_getenv(const char *key, const cJSON *env_json) {
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
    char *model_path = pv_test_module_res_path(MODEL_PATH);
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
            &orca_object);
    free(access_key);
    free(model_path);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_orca_synthesize_params_init(&synthesize_params_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    ENVIRONMENT = pv_load_json("environment.json");

    YPU_ITERATIONS = pv_getenv("YPU_ITERATIONS", ENVIRONMENT);
    YPU_DURATION = pv_getenv("YPU_DURATION", ENVIRONMENT);
    YPU_MACHINE = pv_getenv("YPU_MACHINE", ENVIRONMENT);
    YPU_DEVICE = pv_getenv("YPU_DEVICE", ENVIRONMENT);

    test_iterations = YPU_ITERATIONS ? atoi(YPU_ITERATIONS) : 1;
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

    return PV_STATUS_SUCCESS;
}

static void test_performance_pv_orca_teardown(void) {
    pv_orca_synthesize_params_delete(synthesize_params_object);
    pv_orca_delete(orca_object);

    free(test_durations);
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

static void test_performance_helper(const char *ypu_device) {
    cJSON *results_machine = cJSON_GetObjectItemCaseSensitive(RESULTS, YPU_MACHINE);
    cJSON *results_device = cJSON_CreateObject();
    cJSON_AddItemToObject(results_machine, ypu_device, results_device);
    cJSON *results_orca = cJSON_CreateObject();
    cJSON_AddItemToObject(results_device, "orca", results_orca);

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
                    orca_object,
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
        LOG_INFO("orca: %d iterations in %.3f sec\n", iterations, duration_usec / 1e6);

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
                    orca_object,
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
        LOG_INFO("orca: %d iterations in %.3f sec\n", iterations, test_durations[i] / 1e6);

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
}

static void test_performance_pv_orca_cpu_impl(void) {
    test_performance_helper("cpu:1");
}

static const pv_test_case_t PERFORMANCE_PV_ORCA_TEST_CASES[] = {
        {"cpu", test_performance_pv_orca_cpu_impl},
};

const pv_test_suite_t PERFORMANCE_PV_ORCA_TEST_SUITE = {
        .name = "orca",
        .setup = test_performance_pv_orca_setup,
        .teardown = test_performance_pv_orca_teardown,
        .test_cases = PERFORMANCE_PV_ORCA_TEST_CASES,
        .num_test_cases = PV_ARRAY_LEN(PERFORMANCE_PV_ORCA_TEST_CASES),
};
