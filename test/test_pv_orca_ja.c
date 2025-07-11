#include "../../gatekeeper/test/test_pv_gatekeeper_usage_helper.h"
#include "orca/pv_orca.h"
#include "orca/pv_orca_internal.h"
#include "test/pv_test.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

static const char *MODEL_PATH_JA = "param/orca_params_ja_female.pv";
static pv_orca_t *orca_object_ja = NULL;
static pv_orca_synthesize_params_t *synthesize_params_object_ja = NULL;


static pv_status_t test_pv_orca_setup_helper(
        const char *param_name,
        pv_orca_t **object,
        pv_orca_synthesize_params_t **synthesize_object) {
    char *model_path = pv_test_resource_path(param_name);
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
            object);
    free(access_key);
    free(model_path);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_orca_synthesize_params_init(synthesize_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return status;
}


static pv_status_t test_pv_orca_setup(void) {
    pv_status_t status = test_pv_orca_setup_helper(
            MODEL_PATH_JA,
            &orca_object_ja,
            &synthesize_params_object_ja);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }
    return PV_STATUS_SUCCESS;
}


static void test_pv_orca_teardown(void) {
    pv_orca_synthesize_params_delete(synthesize_params_object_ja);
    pv_orca_delete(orca_object_ja);
}


static void test_pv_orca_ja_normalizable_characters_is_valid_to_ja_hippo_helper(int32_t unicode_val) {
    char character[3 + 1] = {0};
    int32_t num_samples = 0;
    int16_t *pcm = NULL;
    int32_t num_alignments = 0;
    pv_orca_word_alignment_t **alignments = NULL;

    character[0] = 0xE0 | ((unicode_val >> 12) & 0x0F);
    character[1] = 0x80 | ((unicode_val >> 6) & 0x3F);
    character[2] = 0x80 | (unicode_val & 0x3F);
    character[3] = '\0';

    char sentence[1024] = {0};
    // To avoid empty non-verbalizable text input.
    strcpy(sentence, "a");
    strcat(sentence, character);

    pv_status_t status = pv_orca_synthesize_internal(
            orca_object_ja,
            sentence,
            synthesize_params_object_ja,
            true,
            true,
            &num_samples,
            &pcm,
            &num_alignments,
            &alignments);
    pv_test_true(
            status == PV_STATUS_SUCCESS,
            "failed to synthesize character %s; expected `%s` got `%s`",
            character,
            pv_status_to_string(PV_STATUS_SUCCESS),
            pv_status_to_string(status));

    pv_orca_pcm_delete(pcm);
    pv_orca_word_alignments_delete(num_alignments, alignments);
}


static void test_pv_orca_ja_normalizable_characters_is_valid_to_ja_hippo(void) {
    // Japanese punctuations:
    for (int32_t unicode_val = 0x3001; unicode_val <= 0x301F; ++unicode_val) {
        test_pv_orca_ja_normalizable_characters_is_valid_to_ja_hippo_helper(unicode_val);
    }

    // Hiragana:
    for (int32_t unicode_val = 0x3040; unicode_val <= 0x309F; ++unicode_val) {
        test_pv_orca_ja_normalizable_characters_is_valid_to_ja_hippo_helper(unicode_val);
    }

    // Katakana:
    for (int32_t unicode_val = 0x30A0; unicode_val <= 0x30FF; ++unicode_val) {
        test_pv_orca_ja_normalizable_characters_is_valid_to_ja_hippo_helper(unicode_val);
    }

    // Kanji:
    for (int32_t unicode_val = 0x4E00; unicode_val <= 0x9FFF; ++unicode_val) {
        test_pv_orca_ja_normalizable_characters_is_valid_to_ja_hippo_helper(unicode_val);
    }
}


static const pv_test_case_t PV_ORCA_TEST_CASES[] = {

#if (defined(__PV_ENABLE_RELEASE_TESTS__) && (!defined(__PV_TARGET_PLATFORM_WASM__)))

        {"test normalizable characters to JA normalizer is valid to JA hippo", test_pv_orca_ja_normalizable_characters_is_valid_to_ja_hippo},

#endif

};


const pv_test_suite_t PV_ORCA_JA_TEST_SUITE = {
        .name = "orca_ja",
        .setup = test_pv_orca_setup,
        .teardown = test_pv_orca_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_ORCA_TEST_CASES),
        .test_cases = PV_ORCA_TEST_CASES,
};
