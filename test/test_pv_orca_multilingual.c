#include <errno.h>
#include <string.h>

#include "test/pv_test.h"

#include "../../gatekeeper/test/test_pv_gatekeeper_usage_helper.h"
#include "core/pv_error_messages.h"
#include "gatekeeper/pv_gatekeeper_usage_animal.h"
#include "orca/pv_orca.h"
#include "orca/pv_orca_internal.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif


static pv_status_t test_pv_orca_setup_helper(
        const char *param_name,
        pv_orca_t **orca_object,
        pv_orca_synthesize_params_t **synthesize_params_object) {
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


const char *MODEL_PATHS[] = {
        "param/orca_params_de_male.pv",
        "param/orca_params_en_female.pv",
        "param/orca_params_es_female.pv",
        "param/orca_params_fr_female.pv",
        "param/orca_params_it_female.pv",
        "param/orca_params_pt_female.pv",
        "param/orca_params_ko_female.pv",
        "param/orca_params_en_male_16.pv",
        "param/orca_params_en_female_16.pv",
        "param/orca_params_es_male_16.pv",
        "param/orca_params_es_female_16.pv",
        "param/orca_params_de_male_16.pv",
        "param/orca_params_de_female_16.pv",
        "param/orca_params_fr_male_16.pv",
        "param/orca_params_fr_female_16.pv",
        "param/orca_params_it_male_16.pv",
        "param/orca_params_it_female_16.pv",
        "param/orca_params_pt_male_16.pv",
        "param/orca_params_pt_female_16.pv",
};

static void test_pv_orca_multilingual_init_success(void) {
    for (int32_t i = 0; i < PV_ARRAY_LEN(MODEL_PATHS); ++i) {
        const char *model_path = MODEL_PATHS[i];
        pv_orca_t *orca_object = NULL;
        pv_orca_synthesize_params_t *synthesize_params_object = NULL;

        pv_status_t status = test_pv_orca_setup_helper(model_path, &orca_object, &synthesize_params_object);
        pv_test_true(status == PV_STATUS_SUCCESS, "Failed to set up orca related objects using function `test_pv_orca_setup_helper`");
        if (status != PV_STATUS_SUCCESS) {
            return;
        }

        pv_orca_synthesize_params_delete(synthesize_params_object);
        pv_orca_delete(orca_object);
    }
}

static const pv_test_case_t PV_ORCA_MULTILINGUAL_TEST_CASES[] = {
        {"orca_multilingual_init_success", test_pv_orca_multilingual_init_success},
};


const pv_test_suite_t PV_ORCA_MULTILINGUAL_TEST_SUITE = {
        .name = "orca_multilingual",
        .setup = NULL,
        .teardown = NULL,
        .num_test_cases = PV_ARRAY_LEN(PV_ORCA_MULTILINGUAL_TEST_CASES),
        .test_cases = PV_ORCA_MULTILINGUAL_TEST_CASES,
};
