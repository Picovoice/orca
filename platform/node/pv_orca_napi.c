#include <assert.h>
#include <inttypes.h>
#include <node_api.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/picovoice.h"
#include "core/picovoice_internal.h"
#include "orca/pv_orca.h"

napi_value napi_orca_init(napi_env env, napi_callback_info info) {
    size_t argc = 3;
    napi_value args[argc];
    napi_status status = napi_get_cb_info(env, info, &argc, args, NULL, NULL);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                "Unable to get input arguments");
        return NULL;
    }

    size_t length = 0;
    status = napi_get_value_string_utf8(env, args[0], NULL, 0, &length);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_INVALID_ARGUMENT),
                "Unable to get the AccessKey");
        return NULL;
    }
    char *access_key = (char *) calloc(length + 1, sizeof(char));
    if (!access_key) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_OUT_OF_MEMORY),
                "Unable to allocate memory");
        return NULL;
    }
    status = napi_get_value_string_utf8(env, args[0], access_key, length + 1, &length);
    if (status != napi_ok) {
        free(access_key);
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_INVALID_ARGUMENT),
                "Unable to get the AccessKey");
        return NULL;
    }

    length = 0;
    status = napi_get_value_string_utf8(env, args[1], NULL, 0, &length);
    if (status != napi_ok) {
        free(access_key);
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_INVALID_ARGUMENT),
                "Unable to get the path to the model file");
        return NULL;
    }
    char *model_path = (char *) calloc(length + 1, sizeof(char));
    if (!model_path) {
        free(access_key);
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_OUT_OF_MEMORY),
                "Unable to allocate memory");
        return NULL;
    }
    status = napi_get_value_string_utf8(env, args[1], model_path, length + 1, &length);
    if (status != napi_ok) {
        free(model_path);
        free(access_key);
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_INVALID_ARGUMENT),
                "Unable to get the path to the model file");
        return NULL;
    }

    length = 0;
    status = napi_get_value_string_utf8(env, args[2], NULL, 0, &length);
    if (status != napi_ok) {
        free(model_path);
        free(access_key);
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_INVALID_ARGUMENT),
                "Unable to get the device");
        return NULL;
    }
    char *device = (char *) calloc(length + 1, sizeof(char));
    if (!device) {
        free(model_path);
        free(access_key);
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_OUT_OF_MEMORY),
                "Unable to allocate memory");
        return NULL;
    }
    status = napi_get_value_string_utf8(env, args[2], device, length + 1, &length);
    if (status != napi_ok) {
        free(device);
        free(model_path);
        free(access_key);
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_INVALID_ARGUMENT),
                "Unable to get the device");
        return NULL;
    }

    pv_orca_t *handle = NULL;
    pv_status_t pv_status = pv_orca_init(
            access_key,
            model_path,
            device,
            &handle);
    free(access_key);
    free(model_path);

    if (pv_status != PV_STATUS_SUCCESS) {
        handle = NULL;
    }

    napi_value result_object = NULL;
    napi_value handle_js = NULL;
    napi_value status_js = NULL;
    const char *ERROR_MSG = "Unable to allocate memory for the constructed instance of Orca";
    status = napi_create_object(env, &result_object);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }

    status = napi_create_int32(env, pv_status, &status_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }
    status = napi_set_named_property(env, result_object, "status", status_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }

    status = napi_create_bigint_uint64(env, ((uint64_t) (uintptr_t) handle), &handle_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }
    status = napi_set_named_property(env, result_object, "handle", handle_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }

    return result_object;
}

napi_value napi_orca_delete(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[argc];
    napi_status status = napi_get_cb_info(env, info, &argc, args, NULL, NULL);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                "Unable to get input arguments");
        return NULL;
    }

    uint64_t object_id = 0;
    bool lossless = false;
    status = napi_get_value_bigint_uint64(env, args[0], &object_id, &lossless);
    if ((status != napi_ok) || !lossless) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_INVALID_ARGUMENT),
                "Unable to get the address of the instance of Orca properly");
        return NULL;
    }

    pv_orca_delete((pv_orca_t *) (uintptr_t) object_id);

    return NULL;
}

napi_value napi_orca_valid_characters(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[argc];
    napi_status status = napi_get_cb_info(env, info, &argc, args, NULL, NULL);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                "Unable to get input arguments");
        return NULL;
    }

    uint64_t object_id = 0;
    bool lossless = false;
    status = napi_get_value_bigint_uint64(env, args[0], &object_id, &lossless);
    if ((status != napi_ok) || !lossless) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_INVALID_ARGUMENT),
                "Unable to get the address of the instance of Orca properly");
        return NULL;
    }

    int32_t num_characters = 0;
    const char *const *characters = NULL;
    const pv_status_t pv_status = pv_orca_valid_characters(
            (pv_orca_t *) (uintptr_t) object_id,
            &num_characters,
            &characters);

    if (pv_status != PV_STATUS_SUCCESS) {
        num_characters = 0;
        characters = NULL;
    }

    napi_value object_js = NULL;
    napi_value status_js = NULL;
    napi_value characters_js = NULL;
    const char *ERROR_MSG = "Unable to allocate memory for valid characters";

    status = napi_create_object(env, &object_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }

    status = napi_create_int32(env, pv_status, &status_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }
    status = napi_set_named_property(env, object_js, "status", status_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }

    status = napi_create_array_with_length(env, num_characters, &characters_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }

    for (int32_t i = 0; i < num_characters; i++) {
        napi_value character;
        status = napi_create_string_utf8(env, characters[i], NAPI_AUTO_LENGTH, &character);
        if (status != napi_ok) {
            napi_throw_error(
                    env,
                    pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                    ERROR_MSG);
            return NULL;
        }
        status = napi_set_element(env, characters_js, i, character);
        if (status != napi_ok) {
            napi_throw_error(
                    env,
                    pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                    ERROR_MSG);
            return NULL;
        }
    }

    status = napi_set_named_property(env, object_js, "valid_characters", characters_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }

    pv_orca_valid_characters_delete(characters);

    return object_js;
}

napi_value napi_orca_sample_rate(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[argc];
    napi_status status = napi_get_cb_info(env, info, &argc, args, NULL, NULL);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                "Unable to get input arguments");
        return NULL;
    }

    uint64_t object_id = 0;
    bool lossless = false;
    status = napi_get_value_bigint_uint64(env, args[0], &object_id, &lossless);
    if ((status != napi_ok) || !lossless) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_INVALID_ARGUMENT),
                "Unable to get the address of the instance of Orca properly");
        return NULL;
    }

    int32_t sample_rate = 0;
    const pv_status_t pv_status = pv_orca_sample_rate(
            (pv_orca_t *) (uintptr_t) object_id,
            &sample_rate);

    if (pv_status != PV_STATUS_SUCCESS) {
        sample_rate = 0;
    }

    napi_value object_js = NULL;
    napi_value status_js = NULL;
    napi_value sample_rate_js = NULL;
    const char *ERROR_MSG = "Unable to allocate memory for sample rate";

    status = napi_create_object(env, &object_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }

    status = napi_create_int32(env, pv_status, &status_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }
    status = napi_set_named_property(env, object_js, "status", status_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }

    status = napi_create_int32(env, sample_rate, &sample_rate_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }
    status = napi_set_named_property(env, object_js, "sample_rate", sample_rate_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }

    return object_js;
}

napi_value napi_orca_max_character_limit(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[argc];
    napi_status status = napi_get_cb_info(env, info, &argc, args, NULL, NULL);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                "Unable to get input arguments");
        return NULL;
    }

    uint64_t object_id = 0;
    bool lossless = false;
    status = napi_get_value_bigint_uint64(env, args[0], &object_id, &lossless);
    if ((status != napi_ok) || !lossless) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_INVALID_ARGUMENT),
                "Unable to get the address of the instance of Orca properly");
        return NULL;
    }

    int32_t max_character_limit = 0;
    const pv_status_t pv_status = pv_orca_max_character_limit(
            (pv_orca_t *) (uintptr_t) object_id,
            &max_character_limit);

    if (pv_status != PV_STATUS_SUCCESS) {
        max_character_limit = 0;
    }

    napi_value object_js = NULL;
    napi_value status_js = NULL;
    napi_value max_character_limit_js = NULL;
    const char *ERROR_MSG = "Unable to allocate memory for max character limit";

    status = napi_create_object(env, &object_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }

    status = napi_create_int32(env, pv_status, &status_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }
    status = napi_set_named_property(env, object_js, "status", status_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }

    status = napi_create_int32(env, max_character_limit, &max_character_limit_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }
    status = napi_set_named_property(env, object_js, "max_character_limit", max_character_limit_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }

    return object_js;
}

napi_value napi_orca_synthesize_params_init(napi_env env, napi_callback_info info) {
    size_t argc = 0;
    napi_value args[argc];
    napi_status status = napi_get_cb_info(env, info, &argc, args, NULL, NULL);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                "Unable to get input arguments");
        return NULL;
    }

    pv_orca_synthesize_params_t *synthesize_params = NULL;
    pv_status_t pv_status = pv_orca_synthesize_params_init(&synthesize_params);

    if (pv_status != PV_STATUS_SUCCESS) {
        synthesize_params = NULL;
    }

    napi_value result_object = NULL;
    napi_value synthesize_params_js = NULL;
    napi_value status_js = NULL;
    const char *ERROR_MSG = "Unable to allocate memory for the constructed instance of Synthesize Params";

    status = napi_create_object(env, &result_object);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }

    status = napi_create_int32(env, pv_status, &status_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }
    status = napi_set_named_property(env, result_object, "status", status_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }

    status = napi_create_bigint_uint64(env, ((uint64_t) (uintptr_t) synthesize_params), &synthesize_params_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }
    status = napi_set_named_property(env, result_object, "synthesize_params", synthesize_params_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }

    return result_object;
}

napi_value napi_orca_synthesize_params_delete(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[argc];
    napi_status status = napi_get_cb_info(env, info, &argc, args, NULL, NULL);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                "Unable to get input arguments");
        return NULL;
    }

    uint64_t object_id = 0;
    bool lossless = false;
    status = napi_get_value_bigint_uint64(env, args[0], &object_id, &lossless);
    if ((status != napi_ok) || !lossless) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_INVALID_ARGUMENT),
                "Unable to get the address of the instance of Synthesize Params properly");
        return NULL;
    }

    pv_orca_synthesize_params_delete((pv_orca_synthesize_params_t *) (uintptr_t) object_id);

    return NULL;
}

napi_value napi_orca_synthesize_params_set_speech_rate(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[argc];
    napi_status status = napi_get_cb_info(env, info, &argc, args, NULL, NULL);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                "Unable to get input arguments");
        return NULL;
    }

    uint64_t object_id = 0;
    bool lossless = false;
    status = napi_get_value_bigint_uint64(env, args[0], &object_id, &lossless);
    if ((status != napi_ok) || !lossless) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_INVALID_ARGUMENT),
                "Unable to get the address of the instance of Synthesize Params properly");
        return NULL;
    }

    double speech_rate = 1.0;
    status = napi_get_value_double(env, args[1], &speech_rate);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_INVALID_ARGUMENT),
                "Unable to get 'speech_rate'");
        return NULL;
    }

    const pv_status_t pv_status = pv_orca_synthesize_params_set_speech_rate(
            (pv_orca_synthesize_params_t *) (uintptr_t) object_id,
            speech_rate);

    napi_value status_js = NULL;
    status = napi_create_int32(env, pv_status, &status_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                "Unable to allocate memory");
        return NULL;
    }

    return status_js;
}

napi_value napi_orca_synthesize_params_get_speech_rate(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[argc];
    napi_status status = napi_get_cb_info(env, info, &argc, args, NULL, NULL);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                "Unable to get input arguments");
        return NULL;
    }

    uint64_t object_id = 0;
    bool lossless = false;
    status = napi_get_value_bigint_uint64(env, args[0], &object_id, &lossless);
    if ((status != napi_ok) || !lossless) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_INVALID_ARGUMENT),
                "Unable to get the address of the instance of Synthesize Params properly");
        return NULL;
    }

    float speech_rate = -1;
    const pv_status_t pv_status = pv_orca_synthesize_params_get_speech_rate(
            (pv_orca_synthesize_params_t *) (uintptr_t) object_id,
            &speech_rate);

    if (pv_status != PV_STATUS_SUCCESS) {
        speech_rate = -1;
    }

    napi_value object_js = NULL;
    napi_value status_js = NULL;
    napi_value speech_rate_js = NULL;
    const char *ERROR_MSG = "Unable to allocate memory for speech rate";

    status = napi_create_object(env, &object_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }

    status = napi_create_int32(env, pv_status, &status_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }
    status = napi_set_named_property(env, object_js, "status", status_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }

    // TODO: arguments get corrupted when first loaded from the dll
    // https://lists.gnu.org/archive/html/bug-binutils/2022-05/msg00099.html
    napi_create_double(NULL, 0.0, NULL);
    status = napi_create_double(env, speech_rate, &speech_rate_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }
    status = napi_set_named_property(env, object_js, "speech_rate", speech_rate_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }

    return object_js;
}

napi_value napi_orca_synthesize_params_set_random_state(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[argc];
    napi_status status = napi_get_cb_info(env, info, &argc, args, NULL, NULL);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                "Unable to get input arguments");
        return NULL;
    }

    uint64_t object_id = 0;
    bool lossless = false;
    status = napi_get_value_bigint_uint64(env, args[0], &object_id, &lossless);
    if ((status != napi_ok) || !lossless) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_INVALID_ARGUMENT),
                "Unable to get the address of the instance of Synthesize Params properly");
        return NULL;
    }

    int64_t random_state = -1;
    status = napi_get_value_int64(env, args[1], &random_state);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_INVALID_ARGUMENT),
                "Unable to get 'random_state'");
        return NULL;
    }

    const pv_status_t pv_status = pv_orca_synthesize_params_set_random_state(
            (pv_orca_synthesize_params_t *) (uintptr_t) object_id,
            random_state);

    napi_value status_js = NULL;
    status = napi_create_int32(env, pv_status, &status_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                "Unable to allocate memory");
        return NULL;
    }

    return status_js;
}

napi_value napi_orca_synthesize_params_get_random_state(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[argc];
    napi_status status = napi_get_cb_info(env, info, &argc, args, NULL, NULL);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                "Unable to get input arguments");
        return NULL;
    }

    uint64_t object_id = 0;
    bool lossless = false;
    status = napi_get_value_bigint_uint64(env, args[0], &object_id, &lossless);
    if ((status != napi_ok) || !lossless) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_INVALID_ARGUMENT),
                "Unable to get the address of the instance of Synthesize Params properly");
        return NULL;
    }

    int64_t random_state = -1;
    const pv_status_t pv_status = pv_orca_synthesize_params_get_random_state(
            (pv_orca_synthesize_params_t *) (uintptr_t) object_id,
            &random_state);

    if (pv_status != PV_STATUS_SUCCESS) {
        random_state = -1;
    }

    napi_value object_js = NULL;
    napi_value status_js = NULL;
    napi_value random_state_js = NULL;
    const char *ERROR_MSG = "Unable to allocate memory for random state";

    status = napi_create_object(env, &object_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }

    status = napi_create_int32(env, pv_status, &status_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }
    status = napi_set_named_property(env, object_js, "status", status_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }

    status = napi_create_int64(env, random_state, &random_state_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }
    status = napi_set_named_property(env, object_js, "random_state", random_state_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }

    return object_js;
}

static napi_value pack_pcm(
        napi_env env,
        int32_t num_samples,
        void *pcm,
        const char *error_msg) {

    napi_value pcm_js = NULL;
    napi_value array_buffer = NULL;
    void *data = NULL;
    size_t byte_size = num_samples * sizeof(int16_t);

    napi_status status = napi_create_arraybuffer(env, byte_size, &data, &array_buffer);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                NULL,
                error_msg);
        return NULL;
    }
    memcpy(data, pcm, byte_size);

    status = napi_create_typedarray(env, napi_int16_array, num_samples, array_buffer, 0, &pcm_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                NULL,
                error_msg);
        return NULL;
    }

    return pcm_js;
}

static napi_value pack_alignments(
        napi_env env,
        int32_t num_alignments,
        pv_orca_word_alignment_t **alignments,
        const char *error_msg) {

    napi_value alignments_js = NULL;
    napi_value phonemes_js = NULL;

    napi_status status = napi_create_array_with_length(env, num_alignments, &alignments_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                error_msg);
        return NULL;
    }

    // TODO: arguments get corrupted when first loaded from the dll
    // https://lists.gnu.org/archive/html/bug-binutils/2022-05/msg00099.html
    napi_create_double(NULL, 0.0, NULL);

    for (int32_t i = 0; i < num_alignments; i++) {
        napi_value alignment_js = NULL;
        status = napi_create_object(env, &alignment_js);
        if (status != napi_ok) {
            napi_throw_error(
                    env,
                    pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                    error_msg);
            return NULL;
        }

        napi_value token_js = NULL;
        status = napi_create_string_utf8(env, alignments[i] -> word, NAPI_AUTO_LENGTH, &token_js);
        if (status != napi_ok) {
            napi_throw_error(
                    env,
                    pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                    error_msg);
            return NULL;
        }

        napi_value start_sec_js = NULL;
        status = napi_create_double(env, (double) alignments[i] -> start_sec, &start_sec_js);
        if (status != napi_ok) {
            napi_throw_error(
                    env,
                    pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                    error_msg);
            return NULL;
        }

        napi_value end_sec_js = NULL;
        status = napi_create_double(env, (double) alignments[i] -> end_sec, &end_sec_js);
        if (status != napi_ok) {
            napi_throw_error(
                    env,
                    pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                    error_msg);
            return NULL;
        }

        status = napi_create_array_with_length(env, alignments[i] -> num_phonemes, &phonemes_js);
        if (status != napi_ok) {
            napi_throw_error(
                    env,
                    pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                    error_msg);
            return NULL;
        }

        for (int32_t j = 0; j < alignments[i] -> num_phonemes; j++) {
            napi_value phoneme_js = NULL;
            status = napi_create_object(env, &phoneme_js);
            if (status != napi_ok) {
                napi_throw_error(
                        env,
                        pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                        error_msg);
                return NULL;
            }

            napi_value token_js = NULL;
            status = napi_create_string_utf8(env, alignments[i] -> phonemes[j] -> phoneme, NAPI_AUTO_LENGTH, &token_js);
            if (status != napi_ok) {
                napi_throw_error(
                        env,
                        pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                        error_msg);
                return NULL;
            }

            napi_value start_sec_js = NULL;
            status = napi_create_double(env, (double) alignments[i] -> phonemes[j] -> start_sec, &start_sec_js);
            if (status != napi_ok) {
                napi_throw_error(
                        env,
                        pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                        error_msg);
                return NULL;
            }

            napi_value end_sec_js = NULL;
            status = napi_create_double(env, (double) alignments[i] -> phonemes[j] -> end_sec, &end_sec_js);
            if (status != napi_ok) {
                napi_throw_error(
                        env,
                        pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                        error_msg);
                return NULL;
            }

            status = napi_set_named_property(env, phoneme_js, "phoneme", token_js);
            if (status != napi_ok) {
                napi_throw_error(
                        env,
                        pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                        error_msg);
                return NULL;
            }

            status = napi_set_named_property(env, phoneme_js, "startSec", start_sec_js);
            if (status != napi_ok) {
                napi_throw_error(
                        env,
                        pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                        error_msg);
                return NULL;
            }

            status = napi_set_named_property(env, phoneme_js, "endSec", end_sec_js);
            if (status != napi_ok) {
                napi_throw_error(
                        env,
                        pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                        error_msg);
                return NULL;
            }

            status = napi_set_element(env, phonemes_js, j, phoneme_js);
            if (status != napi_ok) {
                napi_throw_error(
                        env,
                        pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                        error_msg);
                return NULL;
            }
        }

        status = napi_set_named_property(env, alignment_js, "word", token_js);
        if (status != napi_ok) {
            napi_throw_error(
                    env,
                    pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                    error_msg);
            return NULL;
        }

        status = napi_set_named_property(env, alignment_js, "startSec", start_sec_js);
        if (status != napi_ok) {
            napi_throw_error(
                    env,
                    pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                    error_msg);
            return NULL;
        }

        status = napi_set_named_property(env, alignment_js, "endSec", end_sec_js);
        if (status != napi_ok) {
            napi_throw_error(
                    env,
                    pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                    error_msg);
            return NULL;
        }

        status = napi_set_named_property(env, alignment_js, "phonemes", phonemes_js);
        if (status != napi_ok) {
            napi_throw_error(
                    env,
                    pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                    error_msg);
            return NULL;
        }

        status = napi_set_element(env, alignments_js, i, alignment_js);
        if (status != napi_ok) {
            napi_throw_error(
                    env,
                    pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                    error_msg);
            return NULL;
        }
    }

    return alignments_js;
}

napi_value napi_orca_synthesize(napi_env env, napi_callback_info info) {
    size_t argc = 3;
    napi_value args[argc];
    napi_status status = napi_get_cb_info(env, info, &argc, args, NULL, NULL);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                "Unable to get input arguments");
        return NULL;
    }

    uint64_t object_id = 0;
    bool lossless = false;
    status = napi_get_value_bigint_uint64(env, args[0], &object_id, &lossless);
    if ((status != napi_ok) || !lossless) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_INVALID_ARGUMENT),
                "Unable to get the address of the instance of Orca properly");
        return NULL;
    }

    size_t length = 0;
    status = napi_get_value_string_utf8(env, args[1], NULL, 0, &length);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_INVALID_ARGUMENT),
                "Unable to get the text");
        return NULL;
    }
    char *text = (char *) calloc(length + 1, sizeof(char));
    if (!text) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_OUT_OF_MEMORY),
                "Unable to allocate memory");
        return NULL;
    }
    status = napi_get_value_string_utf8(env, args[1], text, length + 1, &length);
    if (status != napi_ok) {
        free(text);
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_INVALID_ARGUMENT),
                "Unable to get the text");
        return NULL;
    }

    uint64_t synthesize_params_id = 0;
    status = napi_get_value_bigint_uint64(env, args[2], &synthesize_params_id, &lossless);
    if ((status != napi_ok) || !lossless) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_INVALID_ARGUMENT),
                "Unable to get the address of the instance of Synthesize Params properly");
        return NULL;
    }

    void *pcm = NULL;
    int32_t num_samples = 0;
    int32_t num_alignments = 0;
    pv_orca_word_alignment_t **alignments = NULL;
    const pv_status_t pv_status = pv_orca_synthesize(
            (pv_orca_t *) (uintptr_t) object_id,
            text,
            (pv_orca_synthesize_params_t *) (uintptr_t) synthesize_params_id,
            &num_samples,
            (int16_t **) &pcm,
            &num_alignments,
            &alignments);

    if (pv_status != PV_STATUS_SUCCESS) {
        num_samples = 0;
        pcm = NULL;
        num_alignments = 0;
        alignments = NULL;
    }

    napi_value object_js = NULL;
    napi_value status_js = NULL;
    napi_value pcm_js = NULL;
    napi_value alignments_js = NULL;
    const char *ERROR_MSG = "Unable to allocate memory for the synthesize result";

    status = napi_create_object(env, &object_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }

    status = napi_create_int32(env, pv_status, &status_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }
    status = napi_set_named_property(env, object_js, "status", status_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }

    pcm_js = pack_pcm(env, num_samples, pcm, ERROR_MSG);
    status = napi_set_named_property(env, object_js, "pcm", pcm_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }
    pv_orca_pcm_delete(pcm);

    alignments_js = pack_alignments(env, num_alignments, alignments, ERROR_MSG);
    status = napi_set_named_property(env, object_js, "alignments", alignments_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }
    pv_orca_word_alignments_delete(num_alignments, alignments);

    return object_js;
}

napi_value napi_orca_synthesize_to_file(napi_env env, napi_callback_info info) {
    size_t argc = 4;
    napi_value args[argc];
    napi_status status = napi_get_cb_info(env, info, &argc, args, NULL, NULL);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                "Unable to get input arguments");
        return NULL;
    }

    uint64_t object_id = 0;
    bool lossless = false;
    status = napi_get_value_bigint_uint64(env, args[0], &object_id, &lossless);
    if ((status != napi_ok) || !lossless) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_INVALID_ARGUMENT),
                "Unable to get the address of the instance of Orca properly");
        return NULL;
    }

    size_t length = 0;
    status = napi_get_value_string_utf8(env, args[1], NULL, 0, &length);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_INVALID_ARGUMENT),
                "Unable to get the text");
        return NULL;
    }
    char *text = (char *) calloc(length + 1, sizeof(char));
    if (!text) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_OUT_OF_MEMORY),
                "Unable to allocate memory");
        return NULL;
    }
    status = napi_get_value_string_utf8(env, args[1], text, length + 1, &length);
    if (status != napi_ok) {
        free(text);
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_INVALID_ARGUMENT),
                "Unable to get the text");
        return NULL;
    }

    uint64_t synthesize_params_id = 0;
    status = napi_get_value_bigint_uint64(env, args[2], &synthesize_params_id, &lossless);
    if ((status != napi_ok) || !lossless) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_INVALID_ARGUMENT),
                "Unable to get the address of the instance of Synthesize Params properly");
        return NULL;
    }

    size_t output_path_length = 0;
    status = napi_get_value_string_utf8(env, args[3], NULL, 0, &output_path_length);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_INVALID_ARGUMENT),
                "Unable to get the output path");
        return NULL;
    }
    char *output_path = (char *) calloc(output_path_length + 1, sizeof(char));
    if (!output_path) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_OUT_OF_MEMORY),
                "Unable to allocate memory");
        return NULL;
    }
    status = napi_get_value_string_utf8(env, args[3], output_path, output_path_length + 1, &output_path_length);
    if (status != napi_ok) {
        free(output_path);
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_INVALID_ARGUMENT),
                "Unable to get the output path");
        return NULL;
    }

    int32_t num_alignments = 0;
    pv_orca_word_alignment_t **alignments = NULL;
    const pv_status_t pv_status = pv_orca_synthesize_to_file(
            (pv_orca_t *) (uintptr_t) object_id,
            text,
            (pv_orca_synthesize_params_t *) (uintptr_t) synthesize_params_id,
            output_path,
            &num_alignments,
            &alignments);

    if (pv_status != PV_STATUS_SUCCESS) {
        num_alignments = 0;
        alignments = NULL;
    }

    napi_value object_js = NULL;
    napi_value status_js = NULL;
    napi_value alignments_js = NULL;
    const char *ERROR_MSG = "Unable to allocate memory for the synthesize to file result";

    status = napi_create_object(env, &object_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }

    status = napi_create_int32(env, pv_status, &status_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }
    status = napi_set_named_property(env, object_js, "status", status_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }

    alignments_js = pack_alignments(env, num_alignments, alignments, ERROR_MSG);
    status = napi_set_named_property(env, object_js, "alignments", alignments_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }
    pv_orca_word_alignments_delete(num_alignments, alignments);

    return object_js;
}

napi_value napi_orca_stream_open(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[argc];
    napi_status status = napi_get_cb_info(env, info, &argc, args, NULL, NULL);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                "Unable to get input arguments");
        return NULL;
    }

    uint64_t object_id = 0;
    bool lossless = false;
    status = napi_get_value_bigint_uint64(env, args[0], &object_id, &lossless);
    if ((status != napi_ok) || !lossless) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_INVALID_ARGUMENT),
                "Unable to get the address of the instance of Orca properly");
        return NULL;
    }

    uint64_t synthesize_params_id = 0;
    status = napi_get_value_bigint_uint64(env, args[1], &synthesize_params_id, &lossless);
    if ((status != napi_ok) || !lossless) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_INVALID_ARGUMENT),
                "Unable to get the address of the instance of Synthesize Params properly");
        return NULL;
    }

    pv_orca_stream_t *stream = NULL;
    pv_status_t pv_status = pv_orca_stream_open(
            (pv_orca_t *) (uintptr_t) object_id,
            (pv_orca_synthesize_params_t *) (uintptr_t) synthesize_params_id,
            &stream);

    if (pv_status != PV_STATUS_SUCCESS) {
        stream = NULL;
    }

    napi_value result_object = NULL;
    napi_value stream_js = NULL;
    napi_value status_js = NULL;
    const char *ERROR_MSG = "Unable to allocate memory for the constructed instance of Orca Stream";

    status = napi_create_object(env, &result_object);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }

    status = napi_create_int32(env, pv_status, &status_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }
    status = napi_set_named_property(env, result_object, "status", status_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }

    status = napi_create_bigint_uint64(env, ((uint64_t) (uintptr_t) stream), &stream_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }
    status = napi_set_named_property(env, result_object, "stream", stream_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }

    return result_object;
}

napi_value napi_orca_stream_synthesize(napi_env env, napi_callback_info info) {
    size_t argc = 2;
    napi_value args[argc];
    napi_status status = napi_get_cb_info(env, info, &argc, args, NULL, NULL);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                "Unable to get input arguments");
        return NULL;
    }

    uint64_t object_id = 0;
    bool lossless = false;
    status = napi_get_value_bigint_uint64(env, args[0], &object_id, &lossless);
    if ((status != napi_ok) || !lossless) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_INVALID_ARGUMENT),
                "Unable to get the address of the instance of OrcaStream properly");
        return NULL;
    }

    size_t length = 0;
    status = napi_get_value_string_utf8(env, args[1], NULL, 0, &length);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_INVALID_ARGUMENT),
                "Unable to get the text");
        return NULL;
    }
    char *text = (char *) calloc(length + 1, sizeof(char));
    if (!text) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_OUT_OF_MEMORY),
                "Unable to allocate memory");
        return NULL;
    }
    status = napi_get_value_string_utf8(env, args[1], text, length + 1, &length);
    if (status != napi_ok) {
        free(text);
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_INVALID_ARGUMENT),
                "Unable to get the text");
        return NULL;
    }

    void *pcm = NULL;
    int32_t num_samples = 0;
    const pv_status_t pv_status = pv_orca_stream_synthesize(
            (pv_orca_stream_t *) (uintptr_t) object_id,
            text,
            &num_samples,
            (int16_t **) &pcm);

    if (pv_status != PV_STATUS_SUCCESS) {
        pcm = NULL;
        num_samples = 0;
    }

    napi_value object_js = NULL;
    napi_value status_js = NULL;
    napi_value pcm_js = NULL;
    const char *ERROR_MSG = "Unable to allocate memory for the stream synthesize result";

    status = napi_create_object(env, &object_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }

    status = napi_create_int32(env, pv_status, &status_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }
    status = napi_set_named_property(env, object_js, "status", status_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }

    pcm_js = pack_pcm(env, num_samples, pcm, ERROR_MSG);
    status = napi_set_named_property(env, object_js, "pcm", pcm_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }
    pv_orca_pcm_delete(pcm);

    return object_js;
}

napi_value napi_orca_stream_flush(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[argc];
    napi_status status = napi_get_cb_info(env, info, &argc, args, NULL, NULL);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                "Unable to get input arguments");
        return NULL;
    }

    uint64_t object_id = 0;
    bool lossless = false;
    status = napi_get_value_bigint_uint64(env, args[0], &object_id, &lossless);
    if ((status != napi_ok) || !lossless) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_INVALID_ARGUMENT),
                "Unable to get the address of the instance of OrcaStream properly");
        return NULL;
    }

    void *pcm = NULL;
    int32_t num_samples = 0;
    const pv_status_t pv_status = pv_orca_stream_flush(
            (pv_orca_stream_t *) (uintptr_t) object_id,
            &num_samples,
            (int16_t **) &pcm);

    if (pv_status != PV_STATUS_SUCCESS) {
        pcm = NULL;
        num_samples = 0;
    }

    napi_value object_js = NULL;
    napi_value status_js = NULL;
    napi_value pcm_js = NULL;
    const char *ERROR_MSG = "Unable to allocate memory for the stream flush result";

    status = napi_create_object(env, &object_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }

    status = napi_create_int32(env, pv_status, &status_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }
    status = napi_set_named_property(env, object_js, "status", status_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }

    pcm_js = pack_pcm(env, num_samples, pcm, ERROR_MSG);
    status = napi_set_named_property(env, object_js, "pcm", pcm_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }
    pv_orca_pcm_delete(pcm);

    return object_js;
}

napi_value napi_orca_stream_close(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[argc];
    napi_status status = napi_get_cb_info(env, info, &argc, args, NULL, NULL);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                "Unable to get input arguments");
        return NULL;
    }

    uint64_t object_id = 0;
    bool lossless = false;
    status = napi_get_value_bigint_uint64(env, args[0], &object_id, &lossless);
    if ((status != napi_ok) || !lossless) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_INVALID_ARGUMENT),
                "Unable to get the address of the instance of OrcaStream properly");
        return NULL;
    }

    pv_orca_stream_close((pv_orca_stream_t *) (uintptr_t) object_id);

    return NULL;
}

napi_value napi_orca_version(napi_env env, napi_callback_info info) {
    napi_value result = NULL;
    napi_status status = napi_create_string_utf8(env, pv_orca_version(), NAPI_AUTO_LENGTH, &result);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                "Unable to get version");
        return NULL;
    }

    (void) info;
    return result;
}

napi_value napi_orca_set_sdk(napi_env env, napi_callback_info info) {
    size_t argc = 1;
    napi_value args[argc];
    napi_status status = napi_get_cb_info(env, info, &argc, args, NULL, NULL);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                "Unable to get input arguments");
        return NULL;
    }

    size_t length = 0;
    status = napi_get_value_string_utf8(env, args[0], NULL, 0, &length);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_INVALID_ARGUMENT),
                "Unable to get the sdk");
        return NULL;
    }
    char *sdk = (char *) calloc(length + 1, sizeof(char));
    if (!sdk) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_OUT_OF_MEMORY),
                "Unable to allocate memory");
        return NULL;
    }
    status = napi_get_value_string_utf8(env, args[0], sdk, length + 1, &length);
    if (status != napi_ok) {
        free(sdk);
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_INVALID_ARGUMENT),
                "Unable to get the sdk");
        return NULL;
    }

    pv_set_sdk(sdk);
    free(sdk);

    (void) info;
    return NULL;
}

napi_value napi_orca_get_error_stack(napi_env env, napi_callback_info info) {
    int32_t message_stack_depth = 0;
    char **message_stack = NULL;
    pv_status_t pv_status = pv_get_error_stack(&message_stack, &message_stack_depth);

    napi_value object_js = NULL;
    napi_value status_js = NULL;
    napi_value message_stack_js = NULL;
    const char *ERROR_MSG = "Unable to allocate memory for error object";

    napi_status status = napi_create_object(env, &object_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }

    status = napi_create_int32(env, pv_status, &status_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }
    status = napi_set_named_property(env, object_js, "status", status_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }

    if (pv_status == PV_STATUS_SUCCESS) {
        status = napi_create_array_with_length(env, message_stack_depth, &message_stack_js);
        if (status != napi_ok) {
            napi_throw_error(
                    env,
                    pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                    ERROR_MSG);
            return NULL;
        }

        for (int32_t i = 0; i < message_stack_depth; i++) {
            napi_value message;
            status = napi_create_string_utf8(env, message_stack[i], NAPI_AUTO_LENGTH, &message);
            if (status != napi_ok) {
                napi_throw_error(
                        env,
                        pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                        ERROR_MSG);
                return NULL;
            }

            status = napi_set_element(env, message_stack_js, i, message);
            if (status != napi_ok) {
                napi_throw_error(
                        env,
                        pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                        ERROR_MSG);
                return NULL;
            }
        }

        pv_free_error_stack(message_stack);

        status = napi_set_named_property(env, object_js, "message_stack", message_stack_js);
        if (status != napi_ok) {
            napi_throw_error(
                    env,
                    pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                    ERROR_MSG);
            return NULL;
        }
    }

    (void) info;
    return object_js;
}

napi_value napi_orca_list_hardware_devices(napi_env env, napi_callback_info info) {
    char **hardware_devices = NULL;
    int32_t num_hardware_devices = 0;
    const pv_status_t pv_status = pv_orca_list_hardware_devices(
            &hardware_devices,
            &num_hardware_devices);
    if (pv_status != PV_STATUS_SUCCESS) {
        hardware_devices = NULL;
        num_hardware_devices = 0;
    }

    napi_value result_object = NULL;
    napi_value hardware_devices_js = NULL;
    napi_value status_js = NULL;
    const char *ERROR_MSG = "Unable to allocate memory for the constructed return value";
    napi_status status = napi_create_object(env, &result_object);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }

    status = napi_create_array_with_length(env, num_hardware_devices, &hardware_devices_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }

    for (int32_t i = 0; i < num_hardware_devices; i++) {
        napi_value hardware_device_js = NULL;
        status = napi_create_string_utf8(env, hardware_devices[i], NAPI_AUTO_LENGTH, &hardware_device_js);
        if (status != napi_ok) {
            napi_throw_error(
                    env,
                    pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                    ERROR_MSG);
            return NULL;
        }

        status = napi_set_element(env, hardware_devices_js, i, hardware_device_js);
        if (status != napi_ok) {
            napi_throw_error(
                    env,
                    pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                    ERROR_MSG);
            return NULL;
        }
    }
    status = napi_set_named_property(env, result_object, "hardware_devices", hardware_devices_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }

    status = napi_create_int32(env, pv_status, &status_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }
    status = napi_set_named_property(env, result_object, "status", status_js);
    if (status != napi_ok) {
        napi_throw_error(
                env,
                pv_status_to_string(PV_STATUS_RUNTIME_ERROR),
                ERROR_MSG);
        return NULL;
    }

    pv_orca_free_hardware_devices(hardware_devices, num_hardware_devices);

    (void) info;
    return result_object;
}

#define DECLARE_NAPI_METHOD(name, func) (napi_property_descriptor){ name, 0, func, 0, 0, 0, napi_default, 0 }

napi_value Init(napi_env env, napi_value exports) {
    napi_property_descriptor desc = DECLARE_NAPI_METHOD("init", napi_orca_init);
    napi_status status = napi_define_properties(env, exports, 1, &desc);
    assert(status == napi_ok);

    desc = DECLARE_NAPI_METHOD("delete", napi_orca_delete);
    status = napi_define_properties(env, exports, 1, &desc);
    assert(status == napi_ok);

    desc = DECLARE_NAPI_METHOD("valid_characters", napi_orca_valid_characters);
    status = napi_define_properties(env, exports, 1, &desc);
    assert(status == napi_ok);

    desc = DECLARE_NAPI_METHOD("sample_rate", napi_orca_sample_rate);
    status = napi_define_properties(env, exports, 1, &desc);
    assert(status == napi_ok);

    desc = DECLARE_NAPI_METHOD("max_character_limit", napi_orca_max_character_limit);
    status = napi_define_properties(env, exports, 1, &desc);
    assert(status == napi_ok);

    desc = DECLARE_NAPI_METHOD("synthesize_params_init", napi_orca_synthesize_params_init);
    status = napi_define_properties(env, exports, 1, &desc);
    assert(status == napi_ok);

    desc = DECLARE_NAPI_METHOD("synthesize_params_delete", napi_orca_synthesize_params_delete);
    status = napi_define_properties(env, exports, 1, &desc);
    assert(status == napi_ok);

    desc = DECLARE_NAPI_METHOD("synthesize_params_set_speech_rate", napi_orca_synthesize_params_set_speech_rate);
    status = napi_define_properties(env, exports, 1, &desc);
    assert(status == napi_ok);

    desc = DECLARE_NAPI_METHOD("synthesize_params_get_speech_rate", napi_orca_synthesize_params_get_speech_rate);
    status = napi_define_properties(env, exports, 1, &desc);
    assert(status == napi_ok);

    desc = DECLARE_NAPI_METHOD("synthesize_params_set_random_state", napi_orca_synthesize_params_set_random_state);
    status = napi_define_properties(env, exports, 1, &desc);
    assert(status == napi_ok);

    desc = DECLARE_NAPI_METHOD("synthesize_params_get_random_state", napi_orca_synthesize_params_get_random_state);
    status = napi_define_properties(env, exports, 1, &desc);
    assert(status == napi_ok);

    desc = DECLARE_NAPI_METHOD("synthesize", napi_orca_synthesize);
    status = napi_define_properties(env, exports, 1, &desc);
    assert(status == napi_ok);

    desc = DECLARE_NAPI_METHOD("synthesize_to_file", napi_orca_synthesize_to_file);
    status = napi_define_properties(env, exports, 1, &desc);
    assert(status == napi_ok);

    desc = DECLARE_NAPI_METHOD("stream_open", napi_orca_stream_open);
    status = napi_define_properties(env, exports, 1, &desc);
    assert(status == napi_ok);

    desc = DECLARE_NAPI_METHOD("stream_synthesize", napi_orca_stream_synthesize);
    status = napi_define_properties(env, exports, 1, &desc);
    assert(status == napi_ok);

    desc = DECLARE_NAPI_METHOD("stream_flush", napi_orca_stream_flush);
    status = napi_define_properties(env, exports, 1, &desc);
    assert(status == napi_ok);

    desc = DECLARE_NAPI_METHOD("stream_close", napi_orca_stream_close);
    status = napi_define_properties(env, exports, 1, &desc);
    assert(status == napi_ok);

    desc = DECLARE_NAPI_METHOD("version", napi_orca_version);
    status = napi_define_properties(env, exports, 1, &desc);
    assert(status == napi_ok);

    desc = DECLARE_NAPI_METHOD("set_sdk", napi_orca_set_sdk);
    status = napi_define_properties(env, exports, 1, &desc);
    assert(status == napi_ok);

    desc = DECLARE_NAPI_METHOD("get_error_stack", napi_orca_get_error_stack);
    status = napi_define_properties(env, exports, 1, &desc);
    assert(status == napi_ok);

    desc = DECLARE_NAPI_METHOD("list_hardware_devices", napi_orca_list_hardware_devices);
    status = napi_define_properties(env, exports, 1, &desc);
    assert(status == napi_ok);


    (void) status;
    return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)
