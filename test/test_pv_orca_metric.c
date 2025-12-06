#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "core/picovoice.h"
#include "core/pv_type.h"
#include "test/pv_test.h"
#include "orca/pv_orca_metric_internal.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

static const char *CLASSIFIER_MODEL_PATH = "metric/orca_metric_classifier_params_en.pv";

extern const int16_t SHUT_OFF_LIGHTS[];
extern const int32_t SHUT_OFF_LIGHTS_LENGTH;
static const char *TARGET_PHONEMES[] = {"SH", "AH", "T", "AO", "F", "L", "AY", "T", "S"};
static const int32_t NUM_TARGET_PHONEMES = 9;
static const float PHONEME_ERROR_RATE_REFERENCE = 0.0f;
static const int32_t SAMPLING_RATE = 16000;

static pv_ypu_t *ypu = NULL;
static pv_orca_metric_t *orca_metric_object = NULL;

static pv_status_t test_pv_orca_metric_setup(void) {
    pv_status_t status = pv_ypu_init_cpu(1, &ypu);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    char *model_path = pv_test_module_res_path(CLASSIFIER_MODEL_PATH);
    pv_test_true(
            model_path != NULL,
            "failed to open file with '%s'",
            pv_status_to_string(PV_STATUS_OUT_OF_MEMORY));
    if (!model_path) {
        return PV_STATUS_IO_ERROR;
    }

    status = pv_orca_metric_init(ypu, model_path, SAMPLING_RATE, &orca_metric_object);
    free(model_path);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void test_pv_orca_metric_teardown(void) {
    pv_orca_metric_delete(ypu, orca_metric_object);
}

static void test_pv_orca_metric_process(void) {
    float per = 0.f;
    pv_status_t status = pv_orca_metric_process(
            ypu,
            orca_metric_object,
            SHUT_OFF_LIGHTS_LENGTH,
            SHUT_OFF_LIGHTS,
            NUM_TARGET_PHONEMES,
            TARGET_PHONEMES,
            &per);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to initialize orca metric");
    pv_test_close_float(
            per,
            PHONEME_ERROR_RATE_REFERENCE,
            0.0001f,
            "PER too high, got `%f`, expected `%f`",
            per,
            PHONEME_ERROR_RATE_REFERENCE);
}

static const pv_test_case_t PV_ORCA_METRIC_TEST_CASES[] = {
        {"process", test_pv_orca_metric_process},
};

const pv_test_suite_t PV_ORCA_METRIC_TEST_SUITE = {
        .name = "orca_metric",
        .setup = test_pv_orca_metric_setup,
        .teardown = test_pv_orca_metric_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_ORCA_METRIC_TEST_CASES),
        .test_cases = PV_ORCA_METRIC_TEST_CASES,
};
