#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/picovoice.h"
#include "core/pv_type.h"
#include "io/pv_log.h"
#include "orca/pv_buffer.h"
#include "orca/pv_orca_internal.h"
#include "orca/pv_orca_metric_internal.h"
#include "orca/pv_orca_phonemizer.h"
#include "orca/pv_orca_stream_state.h"
#include "test/pv_test.h"
#include "util/pv_time.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

static pv_ypu_t *ypu = NULL;
static pv_orca_synthesizer_t *orca_synthesizer_object = NULL;
static pv_orca_synthesize_params_t *synthesize_params_object = NULL;
static pv_orca_synthesizer_param_t *synthesizer_param_object = NULL;
static pv_orca_stream_state_t *stream_state_object = NULL;


static struct init_args {
    const pv_orca_synthesizer_param_t *param;
    const pv_orca_stream_state_t *state;
    pv_orca_synthesizer_t *object;
} default_init_args = {};

static struct forward_args {
    pv_orca_synthesizer_t *object;
    const pv_orca_synthesize_params_t *synthesize_params;
    bool no_random_latents;
    int32_t num_tokens;
    const int32_t *tokens;
    int32_t *token_durations;
    int32_t num_samples;
    float *spec;
    int16_t *pcm;
} default_forward_args = {
        .no_random_latents = false,
        .num_tokens = 3,
        .tokens = (int32_t[]){2, 3, 4},
        .num_samples = 0,
};

static pv_status_t test_pv_orca_synthesizer_setup_helper(
        const char *param_name,
        pv_orca_synthesize_params_t **synthesize_params,
        pv_orca_synthesizer_param_t **synthesizer_param,
        pv_orca_synthesizer_t **object) {
    char *model_path = pv_test_module_res_path(param_name);
    pv_test_true(
            model_path != NULL,
            "failed to open file with '%s'",
            pv_status_to_string(PV_STATUS_OUT_OF_MEMORY));
    if (!model_path) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    FILE *f = pv_fopen(model_path, "rb");
    free(model_path);
    pv_test_true(f != NULL, "failed to open model path");
    if (!f) {
        return PV_STATUS_IO_ERROR;
    }

    pv_orca_phonemizer_param_t *phonemizer_param = NULL;
    pv_status_t status = pv_orca_internal_param_load(ypu, f, &phonemizer_param, synthesizer_param);
    pv_test_true(status == PV_STATUS_SUCCESS, "failed to load params with `%s`", pv_status_to_string(status));
    if (status != PV_STATUS_SUCCESS) {
        fclose(f);
        return status;
    }
    pv_orca_phonemizer_param_delete(phonemizer_param);

    int32_t eos_punctuation_indices[1] = {-1};
    status = pv_orca_stream_state_init(
            ypu,
            *synthesizer_param,
            1,
            eos_punctuation_indices,
            1,
            eos_punctuation_indices,
            200,
            &stream_state_object);
    if (status != PV_STATUS_SUCCESS) {
        fclose(f);
        return status;
    }

    // Ted: Set to streaming active and is_flush for mock test.
    stream_state_object->status = PV_ORCA_STREAM_STATUS_ACTIVE;
    stream_state_object->is_flush = true;

    status = pv_orca_synthesizer_init(ypu, *synthesizer_param, stream_state_object, object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_orca_synthesize_params_init(synthesize_params);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static pv_status_t test_pv_orca_synthesizer_setup(void) {
    pv_status_t status = pv_ypu_init_cpu(1, &ypu);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = test_pv_orca_synthesizer_setup_helper(
            "param/orca_params_en_female.pv",
            &synthesize_params_object,
            &synthesizer_param_object,
            &orca_synthesizer_object);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    default_forward_args.object = orca_synthesizer_object;
    default_forward_args.synthesize_params = synthesize_params_object;
    default_forward_args.pcm = NULL;
    default_forward_args.token_durations = NULL;

    default_init_args.object = NULL;
    default_init_args.param = synthesizer_param_object;
    default_init_args.state = stream_state_object;

    return PV_STATUS_SUCCESS;
}

static void test_pv_orca_synthesizer_teardown(void) {
    pv_orca_synthesizer_delete(ypu, orca_synthesizer_object);
    pv_orca_synthesizer_param_delete(ypu, (pv_orca_synthesizer_param_t *) synthesizer_param_object);
    pv_orca_synthesize_params_delete(synthesize_params_object);
    pv_ypu_delete(ypu);
}

#ifdef __PV_MOCKS__

static void test_pv_orca_synthesizer_forward_helper(
        struct forward_args *args,
        pv_status_t expected,
        const char *expected_public_error_message_regex,
        const char *expected_private_error_message_regex) {
    pv_log_disable();

    pv_status_t status = pv_orca_synthesizer_forward(
            ypu,
            args->object,
            args->synthesize_params,
            args->no_random_latents,
            args->num_tokens,
            args->tokens,
            &args->token_durations,
            &args->num_samples,
            &args->pcm);
    pv_log_enable();
    reset_mocks();
    pv_test_true(
            status == expected,
            "forward error, got '%s' expected '%s'",
            pv_status_to_string(status),
            pv_status_to_string(expected));

    if (status != PV_STATUS_SUCCESS) {
        const char *expected_message = expected_public_error_message_regex;

#ifdef __PV_ERROR_SHOW_PRIVATE_MSGS__

        if (expected_private_error_message_regex) {
            expected_message = expected_private_error_message_regex;
        }

#endif

        pv_test_error_message(
                expected_public_error_message_regex,
                expected_private_error_message_regex,
                true,
                "forward error message mismatch, expected '%s'",
                expected_message);
    }

    free(args->pcm);
    args->pcm = NULL;
}

static pv_status_t pv_orca_stream_state_update_t_domain_mock(
        pv_ypu_t *ypu,
        pv_orca_stream_state_t *object,
        int32_t *num_to_T_domain) {
    (void) ypu;
    (void) object;
    *num_to_T_domain = 1;

    return PV_STATUS_SUCCESS;
}

static void test_pv_orca_synthesizer_init_helper(
        struct init_args *args,
        pv_status_t expected,
        const char *expected_public_error_message_regex,
        const char *expected_private_error_message_regex) {
    pv_log_disable();
    pv_status_t status = pv_orca_synthesizer_init(
            ypu,
            args->param,
            (pv_orca_stream_state_t *) args->state,
            &(args->object));
    pv_log_enable();
    reset_mocks();
    pv_test_true(
            status == expected,
            "init internal error, expected '%s' got '%s'",
            pv_status_to_string(expected),
            pv_status_to_string(status));

    if (status != PV_STATUS_SUCCESS) {
        const char *expected_message = expected_public_error_message_regex;

#ifdef __PV_ERROR_SHOW_PRIVATE_MSGS__

        if (expected_private_error_message_regex) {
            expected_message = expected_private_error_message_regex;
        }

#endif

        pv_test_error_message(
                expected_public_error_message_regex,
                expected_private_error_message_regex,
                true,
                "init error message mismatch, expected '%s'",
                expected_message);
    }

    if (status == PV_STATUS_SUCCESS) {
        pv_orca_synthesizer_delete(ypu, args->object);
    }
}

static void test_pv_orca_synthesizer_init_prior_encoder_film_generator_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_prior_encoder_film_generator_init, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_synthesizer_init_helper(
            &default_init_args,
            PV_STATUS_OUT_OF_MEMORY,
            NULL,
            "`prior_encoder_film_generator` failed to init with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_synthesizer_init_prior_encoder_flow_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_prior_encoder_flow_init, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_synthesizer_init_helper(
            &default_init_args,
            PV_STATUS_OUT_OF_MEMORY,
            NULL,
            "`prior_encoder_flow` failed to init with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_synthesizer_init_duration_predictor_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_duration_predictor_init, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_synthesizer_init_helper(
            &default_init_args,
            PV_STATUS_OUT_OF_MEMORY,
            NULL,
            "`duration_predictor` failed to init with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_synthesizer_init_gaussian_upsampler_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_gaussian_upsampler_init, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_synthesizer_init_helper(
            &default_init_args,
            PV_STATUS_OUT_OF_MEMORY,
            NULL,
            "`gaussian_upsampler` failed to init with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_synthesizer_init_lfm_film_generator_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_lfm_film_generator_init, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_synthesizer_init_helper(
            &default_init_args,
            PV_STATUS_OUT_OF_MEMORY,
            NULL,
            "`lfm_film_generator` failed to init with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_synthesizer_init_lfm_condition_fuser_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_lfm_condition_fuser_init, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_synthesizer_init_helper(
            &default_init_args,
            PV_STATUS_OUT_OF_MEMORY,
            NULL,
            "`lfm_condition_fuser` failed to init with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_synthesizer_init_lfm_vf_estimator_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_lfm_vf_estimator_init, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_synthesizer_init_helper(
            &default_init_args,
            PV_STATUS_OUT_OF_MEMORY,
            NULL,
            "`lfm_vf_estimator` failed to init with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_synthesizer_init_vocoder_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_vocoder_init, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_synthesizer_init_helper(
            &default_init_args,
            PV_STATUS_OUT_OF_MEMORY,
            NULL,
            "`vocoder` failed to init with status `OUT_OF_MEMORY`\\.");
}


static void test_pv_orca_synthesizer_forward_get_random_state_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_synthesize_params_get_random_state_valid, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_synthesizer_forward_helper(&default_forward_args, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_orca_synthesize_params_get_random_state_valid` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_synthesizer_forward_sufficient_context_n_domain_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_stream_state_is_sufficient_context_n_domain, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_synthesizer_forward_helper(&default_forward_args, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_orca_stream_state_is_sufficient_context_n_domain` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_synthesizer_forward_update_n_domain_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_stream_state_update_n_domain, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_synthesizer_forward_helper(&default_forward_args, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_orca_stream_state_update_n_domain` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_synthesizer_forward_sample_gaussian_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_util_sample_standard_gaussian_with_temperature, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_synthesizer_forward_helper(&default_forward_args, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_orca_util_sample_standard_gaussian_with_temperature` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_synthesizer_forward_prior_encoder_film_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_stream_state_update_n_domain, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_orca_prior_encoder_film_generator_forward, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_synthesizer_forward_helper(&default_forward_args, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_orca_prior_encoder_film_generator_forward` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_synthesizer_forward_prior_encoder_flow_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_stream_state_update_n_domain, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_orca_prior_encoder_flow_forward, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_synthesizer_forward_helper(&default_forward_args, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_orca_prior_encoder_flow_forward` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_synthesizer_forward_get_speech_rate_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_stream_state_update_n_domain, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_orca_synthesize_params_get_speech_rate, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_synthesizer_forward_helper(&default_forward_args, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_orca_synthesize_params_get_speech_rate` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_synthesizer_forward_duration_predictor_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_stream_state_update_n_domain, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_orca_duration_predictor_forward, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_synthesizer_forward_helper(&default_forward_args, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_orca_duration_predictor_forward` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_synthesizer_forward_gaussian_upsampler_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_stream_state_update_n_domain, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_orca_gaussian_upsampler_forward, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_synthesizer_forward_helper(&default_forward_args, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_orca_gaussian_upsampler_forward` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_synthesizer_forward_sample_gaussian_failure_2(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_stream_state_update_n_domain, PV_STATUS_SUCCESS)
    pv_status_t custom_rets_orca_lfm_condition_fuser_forward[] = {
            PV_STATUS_SUCCESS,
            PV_STATUS_OUT_OF_MEMORY,
    };
    PV_SET_MOCK_RETURN_SEQ(pv_orca_util_sample_standard_gaussian_with_temperature, custom_rets_orca_lfm_condition_fuser_forward)

    test_pv_orca_synthesizer_forward_helper(&default_forward_args, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_orca_util_sample_standard_gaussian_with_temperature` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_synthesizer_forward_sufficient_context_t_domain_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_stream_state_update_n_domain, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_orca_stream_state_is_sufficient_context_t_domain, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_synthesizer_forward_helper(&default_forward_args, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_orca_stream_state_is_sufficient_context_t_domain` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_synthesizer_forward_update_t_domain_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_stream_state_update_n_domain, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_orca_stream_state_update_t_domain, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_synthesizer_forward_helper(&default_forward_args, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_orca_stream_state_update_t_domain` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_synthesizer_forward_lfm_film_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_stream_state_update_n_domain, PV_STATUS_SUCCESS)
    PV_SET_MOCK_CUSTOM_FUNC(pv_orca_stream_state_update_t_domain, pv_orca_stream_state_update_t_domain_mock)
    PV_SET_MOCK_RETURN_VAL(pv_orca_lfm_film_generator_forward, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_synthesizer_forward_helper(&default_forward_args, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_orca_lfm_film_generator_forward` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_synthesizer_forward_lfm_condition_fuser_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_stream_state_update_n_domain, PV_STATUS_SUCCESS)
    PV_SET_MOCK_CUSTOM_FUNC(pv_orca_stream_state_update_t_domain, pv_orca_stream_state_update_t_domain_mock)
    PV_SET_MOCK_RETURN_VAL(pv_orca_lfm_condition_fuser_forward, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_synthesizer_forward_helper(&default_forward_args, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_orca_lfm_condition_fuser_forward` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_synthesizer_forward_lfm_vf_estimator_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_stream_state_update_n_domain, PV_STATUS_SUCCESS)
    PV_SET_MOCK_CUSTOM_FUNC(pv_orca_stream_state_update_t_domain, pv_orca_stream_state_update_t_domain_mock)
    PV_SET_MOCK_RETURN_VAL(pv_orca_lfm_vf_estimator_forward, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_synthesizer_forward_helper(&default_forward_args, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_orca_lfm_vf_estimator_forward` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_synthesizer_forward_vocoder_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_stream_state_update_n_domain, PV_STATUS_SUCCESS)
    PV_SET_MOCK_CUSTOM_FUNC(pv_orca_stream_state_update_t_domain, pv_orca_stream_state_update_t_domain_mock)
    PV_SET_MOCK_RETURN_VAL(pv_orca_vocoder_forward_with_cache, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_synthesizer_forward_helper(&default_forward_args, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_orca_vocoder_forward_with_cache` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_synthesizer_param_load_helper(
        pv_status_t expected,
        const char *expected_public_error_message_regex,
        const char *expected_private_error_message_regex) {
    FILE *dummy_file = malloc(sizeof(FILE *));
    pv_test_true(dummy_file != NULL, "failed create dummy file");
    if (!dummy_file) {
        return;
    }

    pv_orca_synthesizer_param_t *param = NULL;

    pv_status_t status = pv_orca_synthesizer_param_load(ypu, dummy_file, "1.0", &param);
    reset_mocks();
    pv_test_true(
            status == expected,
            "param load error, got '%s' expected '%s'",
            pv_status_to_string(status),
            pv_status_to_string(expected));

    if (expected != PV_STATUS_SUCCESS) {
        const char *expected_message = expected_public_error_message_regex;

#ifdef __PV_ERROR_SHOW_PRIVATE_MSGS__

        if (expected_private_error_message_regex) {
            expected_message = expected_private_error_message_regex;
        }

#endif

        pv_test_error_message(
                expected_public_error_message_regex,
                expected_private_error_message_regex,
                true,
                "param load error message mismatch, expected '%s'",
                expected_message);
    }

    free(dummy_file);
}

#endif

static const pv_test_case_t PV_ORCA_SYNTHESIZER_TEST_CASES[] = {

#ifdef __PV_MOCKS__

        {"forward get random state failure", test_pv_orca_synthesizer_forward_get_random_state_failure},
        {"forward sufficient context n domain failure", test_pv_orca_synthesizer_forward_sufficient_context_n_domain_failure},
        {"forward update n domain failure", test_pv_orca_synthesizer_forward_update_n_domain_failure},
        {"forward sample gaussian failure", test_pv_orca_synthesizer_forward_sample_gaussian_failure},
        {"forward prior encoder film failure", test_pv_orca_synthesizer_forward_prior_encoder_film_failure},
        {"forward prior encoder flow failure", test_pv_orca_synthesizer_forward_prior_encoder_flow_failure},
        {"forward get speech rate failure", test_pv_orca_synthesizer_forward_get_speech_rate_failure},
        {"forward duration predictor failure", test_pv_orca_synthesizer_forward_duration_predictor_failure},
        {"forward gaussian upsampler failure", test_pv_orca_synthesizer_forward_gaussian_upsampler_failure},
        {"forward sample gaussian failure 2", test_pv_orca_synthesizer_forward_sample_gaussian_failure_2},
        {"forward sufficient context t domain failure", test_pv_orca_synthesizer_forward_sufficient_context_t_domain_failure},
        {"forward update t domain failure", test_pv_orca_synthesizer_forward_update_t_domain_failure},
        {"forward lfm film failure", test_pv_orca_synthesizer_forward_lfm_film_failure},
        {"forward lfm condition fuser failure", test_pv_orca_synthesizer_forward_lfm_condition_fuser_failure},
        {"forward lfm vf estimator failure", test_pv_orca_synthesizer_forward_lfm_vf_estimator_failure},

#if !defined(__PV_TARGET_PLATFORM_WASM__)

        {"forward vocoder failure", test_pv_orca_synthesizer_forward_vocoder_failure},

#endif

        {"init prior_encoder_film_generator failure", test_pv_orca_synthesizer_init_prior_encoder_film_generator_failure},
        {"init prior_encoder_flow failure", test_pv_orca_synthesizer_init_prior_encoder_flow_failure},
        {"init gaussian_upsampler failure", test_pv_orca_synthesizer_init_gaussian_upsampler_failure},
        {"init duration_predictor failure", test_pv_orca_synthesizer_init_duration_predictor_failure},
        {"init lfm_film_generator failure", test_pv_orca_synthesizer_init_lfm_film_generator_failure},
        {"init lfm_condition_fuser failure", test_pv_orca_synthesizer_init_lfm_condition_fuser_failure},
        {"init lfm_vf_estimator failure", test_pv_orca_synthesizer_init_lfm_vf_estimator_failure},
        {"init vocoder failure", test_pv_orca_synthesizer_init_vocoder_failure},

#endif

};

const pv_test_suite_t PV_ORCA_SYNTHESIZER_TEST_SUITE = {
        .name = "orca_synthesizer",
        .setup = test_pv_orca_synthesizer_setup,
        .teardown = test_pv_orca_synthesizer_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_ORCA_SYNTHESIZER_TEST_CASES),
        .test_cases = PV_ORCA_SYNTHESIZER_TEST_CASES,
};
