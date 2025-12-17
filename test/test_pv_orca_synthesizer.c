#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/picovoice.h"
#include "core/pv_type.h"
#include "io/pv_log.h"
#include "orca/pv_orca_internal.h"
#include "orca/pv_orca_metric_internal.h"
#include "orca/pv_orca_phonemizer.h"
#include "orca/pv_orca_stream_state.h"
#include "test/pv_test.h"
#include "util/pv_time.h"

#include "test_data/test_pv_orca_synthesizer_data_en.c"

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
    pv_test_true(f != NULL, "failed to load params");
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
            -1,
            200,
            &stream_state_object);
    if (status != PV_STATUS_SUCCESS) {
        fclose(f);
        return status;
    }

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

static void test_pv_orca_synthesizer_forward_success(void) {
    test_pv_orca_synthesizer_forward_helper(&default_forward_args, PV_STATUS_SUCCESS, NULL, NULL);
}

__attribute__((unused)) static void test_pv_orca_synthesizer_forward_performance(void) {
    int64_t start = pv_time();
    default_forward_args.num_tokens = TEST_ORCA_SYNTHESIZER_EN_SEQUENCE_LENGTH;
    default_forward_args.tokens = TEST_ORCA_SYNTHESIZER_EN_INPUT;
    int32_t num_calls = 100;
    for (int32_t i = 0; i < num_calls; i++) {
        test_pv_orca_synthesizer_forward_helper(&default_forward_args, PV_STATUS_SUCCESS, NULL, NULL);
    }
    int64_t end = pv_time();
    int32_t duration = (int32_t) (end - start);

    LOG_INFO("Time taken for %d synthesis calls: %d s", num_calls, duration);
    pv_test_true(
            duration < 30,
            "Time taken for %d synthesis calls is too high, got `%d s`, expected less than 30 s",
            num_calls,
            duration);
}

static pv_ypu_mem_t *pv_ypu_buffer_get_return_null(pv_ypu_t *ypu, int32_t size_bytes, bool exact_size) {
    (void) ypu;
    (void) size_bytes;
    (void) exact_size;
    return NULL;
}

static pv_status_t pv_orca_duration_predictor_forward_mock(
        pv_ypu_t *ypu,
        pv_orca_duration_predictor_t *object,
        float speech_rate,
        int32_t n,
        pv_ypu_mem_t *x,
        int32_t *durations_token,
        int32_t x_offset) {
    (void) ypu;
    (void) object;
    (void) speech_rate;
    (void) n;
    (void) x;
    (void) x_offset;
    for (int32_t i = 0; i < n; i++) {
        durations_token[i] = 1;
    }
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

static void test_pv_orca_synthesizer_init_alloc_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_ypu_host_alloc, NULL)

    test_pv_orca_synthesizer_init_helper(&default_init_args, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `o`\\.");
}

static void test_pv_orca_synthesizer_init_text_encoder_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_text_encoder_init, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_synthesizer_init_helper(&default_init_args, PV_STATUS_OUT_OF_MEMORY, "Picovoice Error", "`text_encoder` failed to init with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_synthesizer_init_duration_predictor_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_text_encoder_init, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_orca_duration_predictor_init, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_synthesizer_init_helper(&default_init_args, PV_STATUS_OUT_OF_MEMORY, "Picovoice Error", "`duration_predictor` failed to init with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_synthesizer_init_flow_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_text_encoder_init, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_orca_duration_predictor_init, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_orca_flow_init, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_synthesizer_init_helper(&default_init_args, PV_STATUS_OUT_OF_MEMORY, "Picovoice Error", "`flow` failed to init with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_synthesizer_init_vocoder_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_text_encoder_init, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_orca_duration_predictor_init, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_orca_flow_init, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_orca_vocoder_init, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_synthesizer_init_helper(&default_init_args, PV_STATUS_OUT_OF_MEMORY, "Picovoice Error", "`vocoder` failed to init with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_synthesizer_forward_text_encoder_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_text_encoder_forward, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_synthesizer_forward_helper(&default_forward_args, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_orca_text_encoder_forward` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_synthesizer_forward_duration_predictor_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_text_encoder_forward, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_orca_duration_predictor_forward, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_synthesizer_forward_helper(&default_forward_args, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_orca_duration_predictor_forward` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_synthesizer_forward_flow_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_text_encoder_forward, PV_STATUS_SUCCESS)
    PV_SET_MOCK_CUSTOM_FUNC(pv_orca_duration_predictor_forward, pv_orca_duration_predictor_forward_mock)
    PV_SET_MOCK_RETURN_VAL(pv_orca_flow_forward, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_synthesizer_forward_helper(&default_forward_args, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_orca_flow_forward` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_synthesizer_forward_vocoder_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_text_encoder_forward, PV_STATUS_SUCCESS)
    PV_SET_MOCK_CUSTOM_FUNC(pv_orca_duration_predictor_forward, pv_orca_duration_predictor_forward_mock)
    PV_SET_MOCK_RETURN_VAL(pv_orca_flow_forward, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_orca_vocoder_forward, PV_STATUS_OUT_OF_MEMORY)
    test_pv_orca_synthesizer_forward_helper(&default_forward_args, PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_orca_vocoder_forward` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_synthesizer_forward_1st_buffer_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_ypu_buffer_get, NULL)
    test_pv_orca_synthesizer_forward_helper(&default_forward_args, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `pv_ypu_buffer_get`\\.");
}

static void test_pv_orca_synthesizer_forward_2nd_buffer_failure(void) {
    pv_ypu_mem_t *(*custom_funcs[])(pv_ypu_t *, int32_t, bool) = {
            pv_ypu_buffer_get_real,
            pv_ypu_buffer_get_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(pv_ypu_buffer_get, custom_funcs)
    test_pv_orca_synthesizer_forward_helper(&default_forward_args, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `pv_ypu_buffer_get`\\.");
}

static void test_pv_orca_synthesizer_forward_3rd_buffer_failure(void) {
    pv_ypu_mem_t *(*custom_funcs[])(pv_ypu_t *, int32_t, bool) = {
            pv_ypu_buffer_get_real,
            pv_ypu_buffer_get_real,
            pv_ypu_buffer_get_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(pv_ypu_buffer_get, custom_funcs)
    test_pv_orca_synthesizer_forward_helper(&default_forward_args, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `pv_ypu_buffer_get`\\.");
}

static void test_pv_orca_synthesizer_forward_4th_buffer_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_text_encoder_forward, PV_STATUS_SUCCESS)
    PV_SET_MOCK_CUSTOM_FUNC(pv_orca_duration_predictor_forward, pv_orca_duration_predictor_forward_mock)
    pv_ypu_mem_t *(*custom_funcs[])(pv_ypu_t *, int32_t, bool) = {
            pv_ypu_buffer_get_real,
            pv_ypu_buffer_get_real,
            pv_ypu_buffer_get_real,
            pv_ypu_buffer_get_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(pv_ypu_buffer_get, custom_funcs)
    test_pv_orca_synthesizer_forward_helper(&default_forward_args, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `pv_ypu_buffer_get`\\.");
}

static void test_pv_orca_synthesizer_forward_5th_buffer_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_text_encoder_forward, PV_STATUS_SUCCESS)
    PV_SET_MOCK_CUSTOM_FUNC(pv_orca_duration_predictor_forward, pv_orca_duration_predictor_forward_mock)
    PV_SET_MOCK_RETURN_VAL(pv_orca_flow_forward, PV_STATUS_SUCCESS)
    pv_ypu_mem_t *(*custom_funcs[])(pv_ypu_t *, int32_t, bool) = {
            pv_ypu_buffer_get_real,
            pv_ypu_buffer_get_real,
            pv_ypu_buffer_get_real,
            pv_ypu_buffer_get_real,
            pv_ypu_buffer_get_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(pv_ypu_buffer_get, custom_funcs)
    test_pv_orca_synthesizer_forward_helper(&default_forward_args, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `pv_ypu_buffer_get`\\.");
}

static void test_pv_orca_synthesizer_forward_6th_buffer_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_text_encoder_forward, PV_STATUS_SUCCESS)
    PV_SET_MOCK_CUSTOM_FUNC(pv_orca_duration_predictor_forward, pv_orca_duration_predictor_forward_mock)
    pv_ypu_mem_t *(*custom_funcs[])(pv_ypu_t *, int32_t, bool) = {
            pv_ypu_buffer_get_real,
            pv_ypu_buffer_get_real,
            pv_ypu_buffer_get_real,
            pv_ypu_buffer_get_real,
            pv_ypu_buffer_get_real,
            pv_ypu_buffer_get_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(pv_ypu_buffer_get, custom_funcs)
    test_pv_orca_synthesizer_forward_helper(&default_forward_args, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `pv_ypu_buffer_get`\\.");
}

static void test_pv_orca_synthesizer_forward_7th_buffer_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_text_encoder_forward, PV_STATUS_SUCCESS)
    PV_SET_MOCK_CUSTOM_FUNC(pv_orca_duration_predictor_forward, pv_orca_duration_predictor_forward_mock)
    pv_ypu_mem_t *(*custom_funcs[])(pv_ypu_t *, int32_t, bool) = {
            pv_ypu_buffer_get_real,
            pv_ypu_buffer_get_real,
            pv_ypu_buffer_get_real,
            pv_ypu_buffer_get_real,
            pv_ypu_buffer_get_real,
            pv_ypu_buffer_get_real,
            pv_ypu_buffer_get_return_null,
    };
    PV_SET_MOCK_CUSTOM_FUNC_SEQ(pv_ypu_buffer_get, custom_funcs)
    test_pv_orca_synthesizer_forward_helper(&default_forward_args, PV_STATUS_OUT_OF_MEMORY, "Failed to allocate, out of memory\\.", "Failed to allocate memory for `pv_ypu_buffer_get`\\.");
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
                "param load error message mismatch, expected '%s'",
                expected_message);
    }

    free(dummy_file);
}

static void test_pv_orca_synthesizer_param_load_text_encoder_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_fread, 1)
    PV_SET_MOCK_RETURN_VAL(pv_orca_phonemizer_param_load, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_orca_text_encoder_param_load, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_synthesizer_param_load_helper(PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_orca_text_encoder_param_load` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_synthesizer_param_load_duration_predictor_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_fread, 1)
    PV_SET_MOCK_RETURN_VAL(pv_orca_phonemizer_param_load, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_orca_text_encoder_param_load, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_orca_duration_predictor_param_load, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_synthesizer_param_load_helper(PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_orca_duration_predictor_param_load` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_synthesizer_param_load_flow_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_fread, 1)
    PV_SET_MOCK_RETURN_VAL(pv_orca_phonemizer_param_load, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_orca_text_encoder_param_load, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_orca_duration_predictor_param_load, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_orca_flow_param_load, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_synthesizer_param_load_helper(PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_orca_flow_param_load` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_synthesizer_param_load_vocoder_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_fread, 1)
    PV_SET_MOCK_RETURN_VAL(pv_orca_phonemizer_param_load, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_orca_text_encoder_param_load, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_orca_duration_predictor_param_load, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_orca_flow_param_load, PV_STATUS_SUCCESS)
    PV_SET_MOCK_RETURN_VAL(pv_orca_vocoder_param_load, PV_STATUS_OUT_OF_MEMORY)

    test_pv_orca_synthesizer_param_load_helper(PV_STATUS_OUT_OF_MEMORY, pv_test_function_hash_regex(), "`pv_orca_vocoder_param_load` failed with status `OUT_OF_MEMORY`\\.");
}

static void test_pv_orca_synthesizer_param_is_equal_helper(bool expected) {
    bool is_equal = pv_orca_synthesizer_param_is_equal(synthesizer_param_object, synthesizer_param_object);
    pv_test_true(is_equal == expected, "param load error, got '%d' expected '%d'", is_equal, expected);
}

static void test_pv_orca_synthesizer_param_is_equal_success(void) {
    test_pv_orca_synthesizer_param_is_equal_helper(true);
}

static void test_pv_orca_synthesizer_param_is_equal_text_encoder_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_text_encoder_param_is_equal, false)

    test_pv_orca_synthesizer_param_is_equal_helper(false);
}

static void test_pv_orca_synthesizer_param_is_equal_duration_predictor_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_text_encoder_param_is_equal, true)
    PV_SET_MOCK_RETURN_VAL(pv_orca_duration_predictor_param_is_equal, false)

    test_pv_orca_synthesizer_param_is_equal_helper(false);
}

static void test_pv_orca_synthesizer_param_is_equal_flow_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_text_encoder_param_is_equal, true)
    PV_SET_MOCK_RETURN_VAL(pv_orca_duration_predictor_param_is_equal, true)
    PV_SET_MOCK_RETURN_VAL(pv_orca_flow_param_is_equal, false)

    test_pv_orca_synthesizer_param_is_equal_helper(false);
}

static void test_pv_orca_synthesizer_param_is_equal_vocoder_failure(void) {
    PV_SET_MOCK_RETURN_VAL(pv_orca_text_encoder_param_is_equal, true)
    PV_SET_MOCK_RETURN_VAL(pv_orca_duration_predictor_param_is_equal, true)
    PV_SET_MOCK_RETURN_VAL(pv_orca_flow_param_is_equal, true)
    PV_SET_MOCK_RETURN_VAL(pv_orca_vocoder_param_is_equal, false)

    test_pv_orca_synthesizer_param_is_equal_helper(false);
}

#endif

static const pv_test_case_t PV_ORCA_SYNTHESIZER_TEST_CASES[] = {

#ifdef __PV_MOCKS__

        {"forward text_encoder failure", test_pv_orca_synthesizer_forward_text_encoder_failure},
        {"forward duration_predictor failure", test_pv_orca_synthesizer_forward_duration_predictor_failure},
        {"forward flow failure", test_pv_orca_synthesizer_forward_flow_failure},
        {"forward vocoder failure", test_pv_orca_synthesizer_forward_vocoder_failure},
        {"forward 1st buffer failure", test_pv_orca_synthesizer_forward_1st_buffer_failure},
        {"forward 2nd buffer failure", test_pv_orca_synthesizer_forward_2nd_buffer_failure},
        {"forward 3rd buffer failure", test_pv_orca_synthesizer_forward_3rd_buffer_failure},
        {"forward 4th buffer failure", test_pv_orca_synthesizer_forward_4th_buffer_failure},
        {"forward 5th buffer failure", test_pv_orca_synthesizer_forward_5th_buffer_failure},
        {"forward 6th buffer failure", test_pv_orca_synthesizer_forward_6th_buffer_failure},
        {"forward 7th buffer failure", test_pv_orca_synthesizer_forward_7th_buffer_failure},

        {"init alloc failure", test_pv_orca_synthesizer_init_alloc_failure},
        {"init text_encoder failure", test_pv_orca_synthesizer_init_text_encoder_failure},
        {"init duration_predictor failure", test_pv_orca_synthesizer_init_duration_predictor_failure},
        {"init flow failure", test_pv_orca_synthesizer_init_flow_failure},
        {"init vocoder failure", test_pv_orca_synthesizer_init_vocoder_failure},

        {"param load text_encoder failure", test_pv_orca_synthesizer_param_load_text_encoder_failure},
        {"param load duration_predictor failure", test_pv_orca_synthesizer_param_load_duration_predictor_failure},
        {"param load flow failure", test_pv_orca_synthesizer_param_load_flow_failure},
        {"param load vocoder failure", test_pv_orca_synthesizer_param_load_vocoder_failure},

        {"param is_equal success", test_pv_orca_synthesizer_param_is_equal_success},
        {"param is_equal text_encoder failure", test_pv_orca_synthesizer_param_is_equal_text_encoder_failure},
        {"param is_equal duration_predictor failure",
         test_pv_orca_synthesizer_param_is_equal_duration_predictor_failure},
        {"param is_equal flow failure", test_pv_orca_synthesizer_param_is_equal_flow_failure},
        {"param is_equal vocoder failure", test_pv_orca_synthesizer_param_is_equal_vocoder_failure},

        {"forward success", test_pv_orca_synthesizer_forward_success},

        /*
            Commented out because it slows down pipeline and is only useful in release mode.
            Kept in to be able to test locally for different platforms.
        */
        //         {"forward performance", test_pv_orca_synthesizer_forward_performance},

        #endif
};

const pv_test_suite_t PV_ORCA_SYNTHESIZER_TEST_SUITE = {
        .name = "orca_synthesizer",
        .setup = test_pv_orca_synthesizer_setup,
        .teardown = test_pv_orca_synthesizer_teardown,
        .num_test_cases = PV_ARRAY_LEN(PV_ORCA_SYNTHESIZER_TEST_CASES),
        .test_cases = PV_ORCA_SYNTHESIZER_TEST_CASES,
};
