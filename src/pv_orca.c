#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "audio/pv_audio_file.h"
#include "audio/pv_container_wav.h"
#include "core/pv_error_messages.h"
#include "gatekeeper/pv_gatekeeper.h"
#include "gatekeeper/pv_gatekeeper_usage_animal.h"
#include "hippo/pv_hippo_internal.h"
#include "io/pv_log.h"
#include "lm/pv_dict.h"
#include "lm/pv_noun_gender_dict.h"
#include "orca/normalizer/pv_normalizer.h"
#include "orca/normalizer/pv_normalizer_language_data.h"
#include "orca/normalizer/pv_normalizer_stream.h"
#include "orca/normalizer/pv_normalizer_token.h"
#include "orca/pv_orca.h"
#include "orca/pv_orca_internal.h"
#include "orca/pv_orca_phonemizer.h"
#include "orca/pv_orca_stream_state.h"
#include "orca/pv_orca_synthesizer.h"
#include "orca/pv_profiler.h"
#include "util/pv_check_status.h"
#include "util/pv_file.h"
#include "util/pv_time.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

static const float PV_ORCA_DEFAULT_SPEECH_RATE = 1.0f;
static const float PV_ORCA_MIN_SPEECH_RATE = 0.7f;
static const float PV_ORCA_MAX_SPEECH_RATE = 1.3f;

static const int64_t PV_ORCA_DEFAULT_RANDOM_STATE = -1;

static const int32_t PV_ORCA_STREAM_STATE_TEXT_BUFFER_SIZE = 2000;

static pv_status_t pv_orca_get_eos_punctuation_indices(
        const pv_normalizer_language_t language,
        const pv_orca_phonemizer_t *phonemizer,
        int32_t *num_eos_punctuation_indices,
        int32_t **eos_punctuation_indices) {
    PV_ASSERT(language >= 0);
    PV_ASSERT(phonemizer);
    PV_ASSERT(num_eos_punctuation_indices);
    PV_ASSERT(eos_punctuation_indices);

    *num_eos_punctuation_indices = 0;
    *eos_punctuation_indices = NULL;

    const char **eos_punctuation_characters = NULL;

    switch (language) {
        case PV_NORMALIZER_LANGUAGE_EN: {
            *num_eos_punctuation_indices = PV_ORCA_STREAM_EN_NUM_EOS_PUNCTUATIONS;
            eos_punctuation_characters = PV_ORCA_STREAM_EN_EOS_PUNCTUATIONS;
        } break;
        case PV_NORMALIZER_LANGUAGE_DE: {
            *num_eos_punctuation_indices = PV_ORCA_STREAM_DE_NUM_EOS_PUNCTUATIONS;
            eos_punctuation_characters = PV_ORCA_STREAM_DE_EOS_PUNCTUATIONS;
        } break;
        case PV_NORMALIZER_LANGUAGE_FR: {
            *num_eos_punctuation_indices = PV_ORCA_STREAM_FR_NUM_EOS_PUNCTUATIONS;
            eos_punctuation_characters = PV_ORCA_STREAM_FR_EOS_PUNCTUATIONS;
        } break;
        case PV_NORMALIZER_LANGUAGE_ES: {
            *num_eos_punctuation_indices = PV_ORCA_STREAM_ES_NUM_EOS_PUNCTUATIONS;
            eos_punctuation_characters = PV_ORCA_STREAM_ES_EOS_PUNCTUATIONS;
        } break;
        case PV_NORMALIZER_LANGUAGE_IT: {
            *num_eos_punctuation_indices = PV_ORCA_STREAM_IT_NUM_EOS_PUNCTUATIONS;
            eos_punctuation_characters = PV_ORCA_STREAM_IT_EOS_PUNCTUATIONS;
        } break;
        case PV_NORMALIZER_LANGUAGE_PT: {
            *num_eos_punctuation_indices = PV_ORCA_STREAM_PT_NUM_EOS_PUNCTUATIONS;
            eos_punctuation_characters = PV_ORCA_STREAM_PT_EOS_PUNCTUATIONS;
        } break;
        case PV_NORMALIZER_LANGUAGE_KO: {
            *num_eos_punctuation_indices = PV_ORCA_STREAM_KO_NUM_EOS_PUNCTUATIONS;
            eos_punctuation_characters = PV_ORCA_STREAM_KO_EOS_PUNCTUATIONS;
        } break;
        case PV_NORMALIZER_LANGUAGE_JA: {
            *num_eos_punctuation_indices = PV_ORCA_STREAM_JA_NUM_EOS_PUNCTUATIONS;
            eos_punctuation_characters = PV_ORCA_STREAM_JA_EOS_PUNCTUATIONS;
        } break;
        default:
            PV_ERROR_REPORT(
                    &pv_error_msg_invalid_argument_internal,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("language"));
            return PV_STATUS_INVALID_ARGUMENT;
    }

    pv_status_t status = pv_orca_phonemizer_get_punctuation_end_indices(
            phonemizer,
            *num_eos_punctuation_indices,
            eos_punctuation_characters,
            eos_punctuation_indices);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_phonemizer_get_punctuation_end_indices,
                pv_status_to_string(status));
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static pv_status_t pv_orca_get_fallback_cutoff_characters_indices(
        const pv_normalizer_language_t language,
        const pv_orca_phonemizer_t *phonemizer,
        int32_t *num_fallback_cutoff_characters_indices,
        int32_t **fallback_cutoff_characters_indices) {
    PV_ASSERT(language >= 0);
    PV_ASSERT(phonemizer);
    PV_ASSERT(num_fallback_cutoff_characters_indices);
    PV_ASSERT(fallback_cutoff_characters_indices);

    *num_fallback_cutoff_characters_indices = 0;
    *fallback_cutoff_characters_indices = NULL;

    const char **fallback_cutoff_characters = NULL;

    switch (language) {
        case PV_NORMALIZER_LANGUAGE_EN: {
            *num_fallback_cutoff_characters_indices = PV_ORCA_STREAM_EN_NUM_FALLBACK_CUTOFF_CHARACTERS;
            fallback_cutoff_characters = PV_ORCA_STREAM_EN_FALLBACK_CUTOFF_CHARACTERS;
        } break;
        case PV_NORMALIZER_LANGUAGE_DE: {
            *num_fallback_cutoff_characters_indices = PV_ORCA_STREAM_DE_NUM_FALLBACK_CUTOFF_CHARACTERS;
            fallback_cutoff_characters = PV_ORCA_STREAM_DE_FALLBACK_CUTOFF_CHARACTERS;
        } break;
        case PV_NORMALIZER_LANGUAGE_FR: {
            *num_fallback_cutoff_characters_indices = PV_ORCA_STREAM_FR_NUM_FALLBACK_CUTOFF_CHARACTERS;
            fallback_cutoff_characters = PV_ORCA_STREAM_FR_FALLBACK_CUTOFF_CHARACTERS;
        } break;
        case PV_NORMALIZER_LANGUAGE_ES: {
            *num_fallback_cutoff_characters_indices = PV_ORCA_STREAM_ES_NUM_FALLBACK_CUTOFF_CHARACTERS;
            fallback_cutoff_characters = PV_ORCA_STREAM_ES_FALLBACK_CUTOFF_CHARACTERS;
        } break;
        case PV_NORMALIZER_LANGUAGE_IT: {
            *num_fallback_cutoff_characters_indices = PV_ORCA_STREAM_IT_NUM_FALLBACK_CUTOFF_CHARACTERS;
            fallback_cutoff_characters = PV_ORCA_STREAM_IT_FALLBACK_CUTOFF_CHARACTERS;
        } break;
        case PV_NORMALIZER_LANGUAGE_PT: {
            *num_fallback_cutoff_characters_indices = PV_ORCA_STREAM_PT_NUM_FALLBACK_CUTOFF_CHARACTERS;
            fallback_cutoff_characters = PV_ORCA_STREAM_PT_FALLBACK_CUTOFF_CHARACTERS;
        } break;
        case PV_NORMALIZER_LANGUAGE_KO: {
            *num_fallback_cutoff_characters_indices = PV_ORCA_STREAM_KO_NUM_FALLBACK_CUTOFF_CHARACTERS;
            fallback_cutoff_characters = PV_ORCA_STREAM_KO_FALLBACK_CUTOFF_CHARACTERS;
        } break;
        case PV_NORMALIZER_LANGUAGE_JA: {
            *num_fallback_cutoff_characters_indices = PV_ORCA_STREAM_JA_NUM_FALLBACK_CUTOFF_CHARACTERS;
            fallback_cutoff_characters = PV_ORCA_STREAM_JA_FALLBACK_CUTOFF_CHARACTERS;
        } break;
        default:
            PV_ERROR_REPORT(
                    &pv_error_msg_invalid_argument_internal,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("language"));
            return PV_STATUS_INVALID_ARGUMENT;
    }

    pv_status_t status = pv_orca_phonemizer_get_punctuation_end_indices(
            phonemizer,
            *num_fallback_cutoff_characters_indices,
            fallback_cutoff_characters,
            fallback_cutoff_characters_indices);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_phonemizer_get_punctuation_end_indices,
                pv_status_to_string(status));
        return status;
    }

    return PV_STATUS_SUCCESS;
}

struct pv_orca {
    pv_orca_synthesizer_t *synthesizer;
    pv_orca_synthesizer_param_t *synthesizer_param;

    pv_orca_phonemizer_param_t *phonemizer_param;

    pv_language_info_t *language_info;
    pv_normalizer_t *normalizer;
    pv_orca_phonemizer_t *phonemizer;
    pv_hippo_t *hippo;

    pv_lexicon_t *lexicon;
    pv_dict_t *dict;
    pv_noun_gender_dict_t *noun_gender_dict;
    pv_heteronym_tree_t *heteronym_tree;

    pv_orca_stream_state_t *stream_state;

    int32_t max_character_limit;

    pv_gatekeeper_usage_animal_t *gatekeeper;
};

PV_API pv_status_t PV_MOCKABLE(pv_orca_init)(
        const char *access_key,
        const char *model_path,
        pv_orca_t **object) {
    pv_error_prepare();

    pv_https_client_factory_t *https_client_factory = calloc(1, sizeof(pv_https_client_factory_t));
    if (!https_client_factory) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("https_client_factory"));
        return PV_STATUS_OUT_OF_MEMORY;
    }
    https_client_factory->create = pv_gatekeeper_get_https_client;

    pv_status_t status = pv_orca_internal_init(
            access_key,
            https_client_factory,
            model_path,
            object);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_internal_init,
                pv_status_to_string(status));
        free(https_client_factory);
    }

    return status;
}

pv_status_t PV_MOCKABLE(pv_orca_internal_init)(
        const char *access_key,
        pv_https_client_factory_t *https_client_factory,
        const char *model_path,
        pv_orca_t **object) {
    pv_error_prepare();

    if (!access_key) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_null,
                PV_ERROR_ARGS_PUBLIC("access_key"),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }
    if (!object) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_null,
                PV_ERROR_ARGS_PUBLIC("object"),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }
    if (!model_path) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_null,
                PV_ERROR_ARGS_PUBLIC("model_path"),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }

    *object = NULL;

    pv_orca_t *o = calloc(1, sizeof(pv_orca_t));
    if (!o) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    FILE *f = pv_fopen(model_path, "rb");
    if (!f) {
        PV_ERROR_REPORT(
                &pv_error_msg_fopen_failure,
                PV_ERROR_ARGS_PUBLIC(model_path),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_IO_ERROR;
    }

    pv_orca_phonemizer_param_t *phonemizer_param = NULL;
    pv_orca_synthesizer_param_t *synthesizer_param = NULL;
    pv_status_t status = pv_orca_internal_param_load(f, &phonemizer_param, &synthesizer_param);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_internal_param_load,
                pv_status_to_string(status));
        (void) fclose(f);
        pv_orca_delete(o);
        return status;
    }

    o->phonemizer_param = phonemizer_param;
    o->synthesizer_param = synthesizer_param;

    size_t count = pv_fread(&(o->max_character_limit), sizeof(int32_t), 1, f);
    if (count != 1) {
        PV_ERROR_REPORT(
                &pv_error_msg_fread_failure,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE(f));
        (void) fclose(f);
        pv_orca_delete(o);
        return PV_STATUS_IO_ERROR;
    }
    if (o->max_character_limit <= 0) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_min,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o->max_character_limit", o->max_character_limit, 0));
        (void) fclose(f);
        pv_orca_delete(o);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    status = pv_serialized_section_unpack(
            f,
            pv_language_info_serialized_context(),
            (void **) &(o->language_info));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_serialized_section_unpack,
                pv_status_to_string(status));
        (void) fclose(f);
        pv_orca_delete(o);
        return status;
    }

    status = pv_hippo_init2(f, &(o->hippo));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_module_init_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE(
                        "pv_hippo",
                        pv_status_to_string(status)));
        (void) fclose(f);
        pv_orca_delete(o);
        return status;
    }

    int32_t curr = (int32_t) ftell(f);
    if (curr == -1) {
        PV_ERROR_REPORT(
                &pv_error_msg_ftell_failure,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE(f));
        (void) fclose(f);
        pv_orca_delete(o);
        return PV_STATUS_IO_ERROR;
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        PV_ERROR_REPORT(
                &pv_error_msg_fseek_failure,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE(f));
        (void) fclose(f);
        pv_orca_delete(o);
        return PV_STATUS_IO_ERROR;
    }

    int32_t end = (int32_t) ftell(f);
    if (end == -1) {
        PV_ERROR_REPORT(
                &pv_error_msg_ftell_failure,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE(f));
        (void) fclose(f);
        pv_orca_delete(o);
        return PV_STATUS_IO_ERROR;
    }

    const int32_t length = end - curr;
    void *buffer = malloc(length);
    if (!buffer) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o"));
        (void) fclose(f);
        pv_orca_delete(o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    if (fseek(f, curr, SEEK_SET) != 0) {
        PV_ERROR_REPORT(
                &pv_error_msg_fseek_failure,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE(f));
        free(buffer);
        (void) fclose(f);
        pv_orca_delete(o);
        return PV_STATUS_IO_ERROR;
    }

    if (pv_fread(buffer, 1, length, f) != (size_t) length) {
        PV_ERROR_REPORT(
                &pv_error_msg_fread_failure,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE(f));
        free(buffer);
        (void) fclose(f);
        pv_orca_delete(o);
        return PV_STATUS_IO_ERROR;
    }

    if (fclose(f) != 0) {
        PV_ERROR_REPORT(
                &pv_error_msg_fclose_failure,
                PV_ERROR_ARGS_PUBLIC(model_path),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_IO_ERROR;
    }
    const void *shadow = buffer;

    pv_language_info_t *language_info_hippo = pv_hippo_language_info(o->hippo);
    status = pv_lexicon_deserialize(&shadow, language_info_hippo, &(o->lexicon));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_lexicon_deserialize,
                pv_status_to_string(status));
        free(buffer);
        pv_orca_delete(o);
        return status;
    }

    status = pv_dict_deserialize(o->lexicon, &shadow, &(o->dict));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_dict_deserialize,
                pv_status_to_string(status));
        free(buffer);
        pv_orca_delete(o);
        return status;
    }

    status = pv_heteronym_tree_deserialize(o->lexicon, &shadow, &(o->heteronym_tree));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_heteronym_tree_deserialize,
                pv_status_to_string(status));
        free(buffer);
        pv_orca_delete(o);
        return status;
    }

    status = pv_noun_gender_dict_deserialize(&shadow, &(o->noun_gender_dict));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_noun_gender_dict_deserialize,
                pv_status_to_string(status));
        free(buffer);
        pv_orca_delete(o);
        return status;
    }

    status = pv_normalizer_init(o->language_info, o->noun_gender_dict, &shadow, &(o->normalizer));
    free(buffer);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_module_init_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE(
                        "pv_normalizer_init",
                        pv_status_to_string(status)));
        pv_orca_delete(o);
        return status;
    }

    status = pv_orca_phonemizer_init(
            o->phonemizer_param,
            o->hippo,
            o->lexicon,
            o->dict,
            o->heteronym_tree,
            o->language_info,
            &(o->phonemizer));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_module_init_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE(
                        "pv_orca_phonemizer_init",
                        pv_status_to_string(status)));
        pv_orca_delete(o);
        return status;
    }

    pv_normalizer_language_t language = 0;
    status = pv_normalizer_util_infer_language_from_language_info(o->language_info, &language);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_util_infer_language_from_language_info,
                pv_status_to_string(status));
        pv_orca_delete(o);
        return status;
    }

    int32_t num_eos_punctuation_indices = 0;
    int32_t *eos_punctuation_indices = NULL;
    status = pv_orca_get_eos_punctuation_indices(
            language,
            o->phonemizer,
            &num_eos_punctuation_indices,
            &eos_punctuation_indices);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_get_eos_punctuation_indices,
                pv_status_to_string(status));
        pv_orca_delete(o);
        return status;
    }

    int32_t num_fallback_cutoff_characters_indices = 0;
    int32_t *fallback_cutoff_characters_indices = NULL;
    status = pv_orca_get_fallback_cutoff_characters_indices(
            language,
            o->phonemizer,
            &num_fallback_cutoff_characters_indices,
            &fallback_cutoff_characters_indices);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_get_fallback_cutoff_characters_indices,
                pv_status_to_string(status));
        pv_orca_delete(o);
        return status;
    }

    status = pv_orca_stream_state_init(
            o->synthesizer_param,
            num_eos_punctuation_indices,
            eos_punctuation_indices,
            num_fallback_cutoff_characters_indices,
            fallback_cutoff_characters_indices,
            pv_orca_phonemizer_get_word_boundary_index(o->phonemizer),
            PV_ORCA_STREAM_STATE_TEXT_BUFFER_SIZE,
            &(o->stream_state));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_module_init_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE(
                        "pv_orca_stream_state_init",
                        pv_status_to_string(status)));
        pv_orca_delete(o);
        return status;
    }

    status = pv_orca_synthesizer_init(o->synthesizer_param, o->stream_state, &(o->synthesizer));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_module_init_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE(
                        "pv_orca_synthesizer",
                        pv_status_to_string(status)));
        pv_orca_delete(o);
        return status;
    }

    pv_gatekeeper_client_info_t *client_info = NULL;
    status = pv_gatekeeper_client_info_init(
            "orca",
            pv_language_info_language_code(o->language_info),
            pv_orca_version(),
            o->synthesizer_param->version,
            NULL,
            &client_info);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_module_init_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE(
                        "pv_gatekeeper_client_info_init",
                        pv_status_to_string(status)));
        pv_orca_delete(o);
        return status;
    }

    status = pv_gatekeeper_usage_animal_init(
            https_client_factory,
            client_info,
            access_key,
            &(o->gatekeeper));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_module_init_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE(
                        "pv_gatekeeper_usage_animal",
                        pv_status_to_string(status)));
        pv_gatekeeper_client_info_delete(client_info);
        pv_orca_delete(o);
        return status;
    }

    *object = o;

    return status;
}

pv_orca_stream_state_t *PV_MOCKABLE(pv_orca_stream_state_get)(const pv_orca_t *object) {
    PV_ASSERT(object);

    return object->stream_state;
}

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_orca_internal_param_serialize)(
        const pv_orca_phonemizer_param_t *phonemizer_param,
        const pv_orca_synthesizer_param_t *synthesizer_param,
        const char *path) {
    PV_ASSERT(synthesizer_param);
    PV_ASSERT(path);

    FILE *file = pv_fopen(path, "wb");
    if (!file) {
        return PV_STATUS_IO_ERROR;
    }

    const char *product = PV_ORCA_MAGIC_TOKEN;
    const size_t product_length = strlen(product);
    size_t count = fwrite(product, sizeof(char), product_length, file);
    if (count != product_length) {
        return PV_STATUS_IO_ERROR;
    }

    const size_t version_length = strlen(PV_ORCA_VERSION);
    count = fwrite(PV_ORCA_VERSION, sizeof(char), version_length, file);
    if (count != version_length) {
        return PV_STATUS_IO_ERROR;
    }

    pv_status_t status = pv_orca_phonemizer_param_serialize(phonemizer_param, file);
    PV_CHECK_STATUS(status);

    status = pv_orca_synthesizer_param_serialize(synthesizer_param, file);
    PV_CHECK_STATUS(status);

    return (fclose(file) == 0) ? PV_STATUS_SUCCESS : PV_STATUS_IO_ERROR;
}

#endif

pv_normalizer_t *PV_MOCKABLE(pv_orca_get_normalizer)(const pv_orca_t *object) {
    PV_ASSERT(object);

    return object->normalizer;
}

pv_status_t PV_MOCKABLE(pv_orca_internal_param_load)(
        FILE *f,
        pv_orca_phonemizer_param_t **phonemizer_param,
        pv_orca_synthesizer_param_t **synthesizer_param) {
    PV_ASSERT(f);
    PV_ASSERT(phonemizer_param);
    PV_ASSERT(synthesizer_param);

    *phonemizer_param = NULL;
    *synthesizer_param = NULL;

    const size_t product_length = strlen(PV_ORCA_MAGIC_TOKEN);
    char loaded_product[product_length + 1];
    loaded_product[product_length] = '\0';
    size_t count = pv_fread(loaded_product, sizeof(char), product_length, f);
    if (count != product_length) {
        PV_ERROR_REPORT(
                &pv_error_msg_fread_failure,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE(f));
        return PV_STATUS_IO_ERROR;
    }
    if (strcmp(loaded_product, PV_ORCA_MAGIC_TOKEN) != 0) {
        PV_ERROR_REPORT(
                &pv_error_msg_incorrect_file,
                PV_ERROR_ARGS_PUBLIC("Orca model (.pv)"),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }

    const size_t version_length = strlen(PV_ORCA_VERSION);
    char loaded_version[version_length + 1];
    loaded_version[version_length] = '\0';
    count = pv_fread(loaded_version, sizeof(char), version_length, f);
    if (count != version_length) {
        PV_ERROR_REPORT(
                &pv_error_msg_fread_failure,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE(f));
        return PV_STATUS_IO_ERROR;
    }
    if (strcmp(loaded_version, PV_ORCA_VERSION) != 0) {
        PV_ERROR_REPORT(
                &pv_error_msg_incorrect_file_version,
                PV_ERROR_ARGS_PUBLIC(
                        "Orca model (.pv)",
                        loaded_version,
                        PV_ORCA_VERSION),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }

    pv_status_t status = pv_orca_phonemizer_param_load(f, phonemizer_param);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_phonemizer_param_load,
                pv_status_to_string(status));
        return status;
    }

    status = pv_orca_synthesizer_param_load(f, loaded_version, synthesizer_param);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_synthesizer_param_load,
                pv_status_to_string(status));
        return status;
    }

    return status;
}

PV_API void PV_MOCKABLE(pv_orca_delete)(pv_orca_t *object) {
    if (object) {
        if (object->gatekeeper) {
            pv_gatekeeper_usage_animal_finish_reports(object->gatekeeper);
        }
        pv_gatekeeper_usage_animal_delete(object->gatekeeper);

        pv_orca_phonemizer_delete(object->phonemizer);
        pv_dict_delete(object->dict);
        pv_heteronym_tree_delete(object->heteronym_tree);
        pv_noun_gender_dict_delete(object->noun_gender_dict);
        pv_lexicon_delete(object->lexicon);
        pv_hippo_delete(object->hippo);

        pv_normalizer_delete(object->normalizer);
        pv_language_info_delete(object->language_info);

        pv_orca_synthesizer_delete(object->synthesizer);

        pv_orca_stream_state_delete(object->stream_state);

        pv_orca_phonemizer_param_delete(object->phonemizer_param);

        pv_orca_synthesizer_param_delete(object->synthesizer_param);

        free(object);
    }
}

PV_API pv_status_t PV_MOCKABLE(pv_orca_valid_characters)(
        const pv_orca_t *object,
        int32_t *num_characters,
        const char *const **characters) {
    pv_error_prepare();

    if (!object) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_null,
                PV_ERROR_ARGS_PUBLIC("object"),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }
    if (!num_characters) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_null,
                PV_ERROR_ARGS_PUBLIC("num_characters"),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }
    if (!characters) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_null,
                PV_ERROR_ARGS_PUBLIC("characters"),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }

    *num_characters = 0;
    *characters = NULL;

    int32_t num_characters_internal = 0;
    const char *const *all_characters = NULL;

    pv_status_t status = pv_normalizer_get_characters(
            object->normalizer,
            &num_characters_internal,
            &all_characters);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_get_characters,
                pv_status_to_string(status));
        return status;
    }

    const char **characters_internal = malloc(sizeof(char *) * num_characters_internal);
    if (!characters_internal) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("characters_internal"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    for (int32_t i = 0; i < num_characters_internal; i++) {
        characters_internal[i] = all_characters[i];
    }

    *num_characters = num_characters_internal;
    *characters = characters_internal;

    return PV_STATUS_SUCCESS;
}

PV_API pv_status_t PV_MOCKABLE(pv_orca_sample_rate)(const pv_orca_t *object, int32_t *sample_rate) {
    pv_error_prepare();
    if (!object) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_null,
                PV_ERROR_ARGS_PUBLIC("object"),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }
    if (!sample_rate) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_null,
                PV_ERROR_ARGS_PUBLIC("sample_rate"),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }

    *sample_rate = pv_orca_synthesizer_sample_rate(object->synthesizer);

    return PV_STATUS_SUCCESS;
}

static float pv_orca_frame_to_sec(int32_t frame, int32_t sample_rate) {
    PV_ASSERT(frame >= 0);
    PV_ASSERT(sample_rate > 0);

    const int32_t frame_length = PV_ORCA_WINDOW_SHIFT;
    const float res = (float) (frame * frame_length) / (float) sample_rate;

    return roundf(res * 1000.f) / 1000.f;
}

pv_status_t PV_MOCKABLE(pv_orca_phoneme_alignment_init)(
        const char *phoneme,
        float start_sec,
        float end_sec,
        pv_orca_phoneme_alignment_t **object) {
    PV_ASSERT(phoneme);
    PV_ASSERT(start_sec >= 0);
    PV_ASSERT(end_sec >= 0);
    PV_ASSERT(object);

    *object = NULL;

    pv_orca_phoneme_alignment_t *o = calloc(1, sizeof(pv_orca_phoneme_alignment_t));
    if (!o) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->start_sec = start_sec;
    o->end_sec = end_sec;

    o->phoneme = calloc(strlen(phoneme) + 1, sizeof(char));
    if (!o->phoneme) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o->phoneme"));
        free(o);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    memcpy(o->phoneme, phoneme, strlen(phoneme) + 1);

    *object = o;

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_orca_phoneme_alignment_copy)(
        pv_orca_phoneme_alignment_t *source,
        pv_orca_phoneme_alignment_t **destination) {
    PV_ASSERT(source);
    PV_ASSERT(destination);

    *destination = NULL;

    pv_orca_phoneme_alignment_t *o = calloc(1, sizeof(pv_orca_phoneme_alignment_t));
    if (!o) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->start_sec = source->start_sec;
    o->end_sec = source->end_sec;

    o->phoneme = calloc(strlen(source->phoneme) + 1, sizeof(char));
    if (!o->phoneme) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o->phoneme"));
        free(o);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    memcpy(o->phoneme, source->phoneme, strlen(source->phoneme) + 1);

    *destination = o;

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_orca_phoneme_alignment_delete)(pv_orca_phoneme_alignment_t *object) {
    if (object) {
        free(object->phoneme);
        free(object);
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_orca_word_alignment_init)(
        const char *word,
        float start_sec,
        float end_sec,
        int32_t num_phonemes,
        pv_orca_phoneme_alignment_t **phonemes,
        pv_orca_word_alignment_t **object) {
    PV_ASSERT(word);
    PV_ASSERT(start_sec >= 0);
    PV_ASSERT(end_sec >= 0);
    PV_ASSERT(num_phonemes >= 0);
    PV_ASSERT(phonemes);
    PV_ASSERT(object);

    *object = NULL;

    pv_orca_word_alignment_t *o = calloc(1, sizeof(pv_orca_word_alignment_t));
    if (!o) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->start_sec = start_sec;
    o->end_sec = end_sec;

    o->word = calloc(strlen(word) + 1, sizeof(char));
    if (!o->word) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o->word"));
        free(o);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    memcpy(o->word, word, strlen(word) + 1);

    o->num_phonemes = num_phonemes;
    o->phonemes = phonemes;

    *object = o;

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_orca_word_alignment_delete)(pv_orca_word_alignment_t *object) {
    if (object) {
        for (int32_t i = 0; i < object->num_phonemes; i++) {
            pv_orca_phoneme_alignment_delete(object->phonemes[i]);
        }
        free((pv_orca_phoneme_alignment_t *) object->phonemes);
        free(object->word);
        free(object);
    }

    return PV_STATUS_SUCCESS;
}

static int32_t encoded_phoneme_to_phoneme(const pv_orca_t *object, int32_t encoded_phoneme_index) {
    PV_ASSERT(object);
    PV_ASSERT(encoded_phoneme_index >= 0);

    int32_t multiplier = object->phonemizer_param->num_phoneme_multiplier;
    return (encoded_phoneme_index + multiplier) / multiplier;
}

pv_status_t PV_MOCKABLE(pv_orca_phonemize_text)(
        const pv_orca_t *object,
        const char *text,
        bool is_flush,
        bool no_text_additions,
        int32_t *num_text_tokens,
        pv_normalizer_token_t ***text_tokens,
        int32_t *num_encoded_phonemes,
        int32_t **encoded_phonemes,
        int32_t **text_tokens_num_encoded_phonemes) {
    PV_ASSERT(object);
    PV_ASSERT(text);
    PV_ASSERT(num_text_tokens);
    PV_ASSERT(text_tokens);
    PV_ASSERT(num_encoded_phonemes);
    PV_ASSERT(encoded_phonemes);
    PV_ASSERT(text_tokens_num_encoded_phonemes);

    *num_text_tokens = 0;
    *text_tokens = NULL;
    *num_encoded_phonemes = 0;
    *encoded_phonemes = NULL;
    *text_tokens_num_encoded_phonemes = NULL;

    PV_ORCA_PROFILER_START("pv_orca_phonemize_text");

    bool preserve_word_boundary = true;
    bool remove_unknown_characters = false;
    pv_normalizer_token_list_t *text_token_list_internal = NULL;
    pv_status_t status = pv_normalizer_normalize(
            object->normalizer,
            text,
            preserve_word_boundary,
            remove_unknown_characters,
            NULL,
            &text_token_list_internal);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_normalize,
                pv_status_to_string(status));
        return status;
    }

#ifdef __ORCA_LOG_LEVEL_VERBOSE__

    ORCA_LOG_VERBOSE_SIMPLE("Normalized text tokens:");
    pv_normalizer_token_t *current_ = text_token_list_internal->head;
    while (current_) {
        ORCA_LOG_VERBOSE(
                "Token: string=`%s`, original=`%s`, verbalized=`%s`, pron=`%s`, tag=`%d`",
                current_->string,
                current_->original_string,
                current_->verbalized,
                current_->pronunciation,
                current_->tag);
        current_ = current_->next;
    }

#endif

    if (!text_token_list_internal) {
        PV_ERROR_REPORT_SIMPLE(&pv_error_msg_orca_invalid_text);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    pv_normalizer_token_list_remove_space_tokens(text_token_list_internal);

    if (text_token_list_internal->size == 0) {
        pv_normalizer_token_list_delete(text_token_list_internal);
        PV_ERROR_REPORT_SIMPLE(&pv_error_msg_orca_invalid_text);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    int32_t num_text_tokens_internal = text_token_list_internal->size;

    pv_normalizer_token_t **text_tokens_internal = NULL;
    status = pv_normalizer_token_list_to_token_array(
            text_token_list_internal,
            &text_tokens_internal);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_token_list_to_token_array,
                pv_status_to_string(status));
        pv_normalizer_token_list_delete(text_token_list_internal);
        return status;
    }
    free(text_token_list_internal);

    bool append_terminator = false;
    bool allow_prepend_bos = false;
    bool allow_append_eos = false;
    if (no_text_additions) {
        // When testing for streaming and batching PCM match, then need to turn off the EOS, BOS, and terminator, because otherwise streaming and batching will naturally disagree.
        append_terminator = false;
        allow_prepend_bos = false;
        allow_append_eos = false;
    } else {
        append_terminator = is_flush ? object->phonemizer_param->add_eos_punctuation : false;
        allow_prepend_bos = true;
        allow_append_eos = true;
        if (object->stream_state->status == PV_ORCA_STREAM_STATUS_ACTIVE) {
            allow_prepend_bos = object->stream_state->is_first_chunk ? true : false;
            allow_append_eos = object->stream_state->is_flush ? true : false;
        }
    }

    int32_t num_encoded_phonemes_internal = 0;
    int32_t *encoded_phonemes_internal = NULL;
    int32_t *text_tokens_num_encoded_phonemes_internal = NULL;
    status = pv_orca_phonemizer_phonemize(
            object->phonemizer,
            num_text_tokens_internal,
            (const pv_normalizer_token_t **) text_tokens_internal,
            0,
            NULL,
            append_terminator,
            allow_prepend_bos,
            allow_append_eos,
            &num_encoded_phonemes_internal,
            &encoded_phonemes_internal,
            &text_tokens_num_encoded_phonemes_internal);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_phonemizer_phonemize,
                pv_status_to_string(status));
        for (int32_t i = 0; i < num_text_tokens_internal; i++) {
            pv_normalizer_token_delete(text_tokens_internal[i]);
        }
        free(text_tokens_internal);
        return status;
    }

    *num_text_tokens = num_text_tokens_internal;
    *text_tokens = text_tokens_internal;
    *num_encoded_phonemes = num_encoded_phonemes_internal;
    *encoded_phonemes = encoded_phonemes_internal;
    *text_tokens_num_encoded_phonemes = text_tokens_num_encoded_phonemes_internal;

    PV_ORCA_PROFILER_STOP("pv_orca_phonemize_text");

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_orca_merge_word_alignments)(
        int32_t num_alignments,
        pv_orca_word_alignment_t **alignments_orig,
        const int32_t *indices_merged,
        int32_t *num_alignments_merged,
        pv_orca_word_alignment_t ***alignments_merged) {
    PV_ASSERT(num_alignments > 0);
    PV_ASSERT(alignments_orig);
    PV_ASSERT(indices_merged);
    PV_ASSERT(num_alignments_merged);
    PV_ASSERT(alignments_merged);

    *num_alignments_merged = 0;
    *alignments_merged = NULL;

    int32_t num_alignments_merged_internal = indices_merged[num_alignments - 1] + 1;

    pv_orca_word_alignment_t **alignments_merged_internal = calloc(
            num_alignments_merged_internal,
            sizeof(pv_orca_word_alignment_t *));
    if (!alignments_merged_internal) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("alignments_merged_internal"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    int32_t index_merged = 0;
    int32_t index_orig = 0;
    while (index_orig < num_alignments) {

        int32_t num_phonemes = 0;

        int32_t start_index_original = index_orig;
        int32_t current_index_merged = indices_merged[index_orig];
        while ((index_orig < num_alignments) && (indices_merged[index_orig] == current_index_merged)) {
            pv_orca_word_alignment_t *current = alignments_orig[index_orig];
            num_phonemes += current->num_phonemes;
            index_orig++;
        }
        int32_t end_index_original = index_orig - 1;

        pv_orca_phoneme_alignment_t **phonemes = calloc(num_phonemes, sizeof(pv_orca_phoneme_alignment_t *));
        if (!phonemes) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("phonemes"));
            for (int32_t k = 0; k < index_merged; k++) {
                pv_orca_word_alignment_delete(alignments_merged_internal[k]);
            }
            free(alignments_merged_internal);
            return PV_STATUS_OUT_OF_MEMORY;
        }

        int32_t index_new_phonemes = 0;
        for (int32_t i = start_index_original; i <= end_index_original; i++) {
            pv_orca_word_alignment_t *current = alignments_orig[i];
            for (int32_t j = 0; j < current->num_phonemes; j++) {
                pv_status_t status = pv_orca_phoneme_alignment_copy(
                        current->phonemes[j],
                        &phonemes[index_new_phonemes]);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_orca_phoneme_alignment_copy,
                            pv_status_to_string(status));
                    for (int32_t k = 0; k < num_phonemes; k++) {
                        pv_orca_phoneme_alignment_delete(phonemes[k]);
                    }
                    free(phonemes);
                    for (int32_t k = 0; k < index_merged; k++) {
                        pv_orca_word_alignment_delete(alignments_merged_internal[k]);
                    }
                    free(alignments_merged_internal);
                    return status;
                }

                index_new_phonemes++;
            }
        }

        pv_orca_word_alignment_t *first = alignments_orig[start_index_original];
        pv_orca_word_alignment_t *last = alignments_orig[end_index_original];
        pv_status_t status = pv_orca_word_alignment_init(
                first->word,
                first->start_sec,
                last->end_sec,
                num_phonemes,
                phonemes,
                &alignments_merged_internal[index_merged]);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_orca_word_alignment_init,
                    pv_status_to_string(status));
            for (int32_t k = 0; k < num_phonemes; k++) {
                pv_orca_phoneme_alignment_delete(phonemes[k]);
            }
            free(phonemes);
            for (int32_t k = 0; k < index_merged; k++) {
                pv_orca_word_alignment_delete(alignments_merged_internal[k]);
            }
            free(alignments_merged_internal);
            return status;
        }

        ORCA_LOG_VERBOSE(
                "NEW MERGED WORD ALIGNMENT: `%s`, start, end: [%.3f, %.3f]",
                first->word,
                first->start_sec,
                last->end_sec);

        index_merged++;
    }

    *num_alignments_merged = num_alignments_merged_internal;
    *alignments_merged = alignments_merged_internal;

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_orca_create_word_alignments)(
        const pv_orca_t *object,
        int32_t num_text_tokens,
        pv_normalizer_token_t **text_tokens,
        const int32_t *text_tokens_num_encoded_phonemes,
        const int32_t *encoded_phonemes,
        const int32_t *encoded_phonemes_durations,
        int32_t *num_alignments,
        pv_orca_word_alignment_t ***alignments) {
    PV_ASSERT(object);
    PV_ASSERT(num_text_tokens > 0);
    PV_ASSERT(text_tokens);
    PV_ASSERT(text_tokens_num_encoded_phonemes);
    PV_ASSERT(encoded_phonemes);
    PV_ASSERT(encoded_phonemes_durations);
    PV_ASSERT(num_alignments);
    PV_ASSERT(alignments);

    *num_alignments = 0;
    *alignments = NULL;

    int32_t sample_rate = 0;
    pv_status_t status = pv_orca_sample_rate(object, &sample_rate);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_sample_rate,
                pv_status_to_string(status));
        return status;
    }

    pv_orca_word_alignment_t **aligns = calloc(num_text_tokens, sizeof(pv_orca_word_alignment_t *));
    if (!aligns) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("aligns"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    int32_t *indices_merged = calloc(num_text_tokens, sizeof(int32_t));
    if (!indices_merged) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("indices_merged"));
        free(aligns);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    int32_t index_merged = 0;
    bool merge_alignments = false;

    int32_t num_phonemes =
            pv_language_info_num_phonemes(object->language_info) *
            object->phonemizer_param->num_phoneme_multiplier;

    int32_t start_frame_word = 0;
    int32_t end_frame_word = 0;

    int32_t start_frame_phoneme = 0;
    int32_t end_frame_phoneme = 0;

    int32_t token_index = 0;
    for (int32_t word_index = 0; word_index < num_text_tokens; word_index++) {
        start_frame_word = end_frame_word;

        int32_t num_true_phonemes = 0;
        int32_t token_index_dry_run = token_index;
        for (int32_t phoneme_index = 0; phoneme_index < text_tokens_num_encoded_phonemes[word_index]; phoneme_index++) {
            int32_t encoded_phoneme_index = encoded_phonemes[token_index_dry_run];

            bool special_character = (encoded_phoneme_index >= num_phonemes);
            int32_t multiplier = object->phonemizer_param->num_phoneme_multiplier;
            if (!special_character && ((encoded_phoneme_index % multiplier) == 0)) {
                num_true_phonemes += 1;
            }
            token_index_dry_run += 1;
        }

        pv_orca_phoneme_alignment_t **phonemes = calloc(num_true_phonemes, sizeof(pv_orca_phoneme_alignment_t *));
        if (!phonemes) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("phonemes"));
            free(indices_merged);
            for (int32_t k = 0; k < word_index; k++) {
                pv_orca_word_alignment_delete(aligns[k]);
            }
            free(aligns);
            return PV_STATUS_OUT_OF_MEMORY;
        }

        int32_t prev_phoneme_index = -1;
        int32_t num_added_phonemes = 0;
        for (int32_t encoded_index = 0; encoded_index < text_tokens_num_encoded_phonemes[word_index]; encoded_index++) {

            bool last_phoneme_in_word = (encoded_index == (text_tokens_num_encoded_phonemes[word_index] - 1));
            bool special_character = (encoded_phonemes[token_index] >= num_phonemes);
            if (special_character && !last_phoneme_in_word) { // special token
                end_frame_phoneme += encoded_phonemes_durations[token_index];
                token_index += 1;
                continue;
            }

            if (prev_phoneme_index == -1) {
                prev_phoneme_index = encoded_phoneme_to_phoneme(object, encoded_phonemes[token_index]);
                end_frame_phoneme += encoded_phonemes_durations[token_index];
                token_index += 1;
                continue;
            }

            int32_t phoneme_index = encoded_phoneme_to_phoneme(object, encoded_phonemes[token_index]);

            bool skip_possible =
                    (encoded_phonemes[token_index] % object->phonemizer_param->num_phoneme_multiplier) != 0;
            if ((phoneme_index != prev_phoneme_index) || last_phoneme_in_word || !skip_possible) {
                if (last_phoneme_in_word) {
                    end_frame_phoneme += encoded_phonemes_durations[token_index];
                }

                const char *phoneme = NULL;
                status = pv_language_info_phoneme_index_to_string(object->language_info, prev_phoneme_index, &phoneme);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_language_info_phoneme_index_to_string,
                            pv_status_to_string(status));
                    free(indices_merged);
                    for (int32_t k = 0; k < num_added_phonemes; k++) {
                        pv_orca_phoneme_alignment_delete(phonemes[k]);
                    }
                    free(phonemes);
                    for (int32_t k = 0; k < word_index; k++) {
                        pv_orca_word_alignment_delete(aligns[k]);
                    }
                    free(aligns);
                    return status;
                }

                float start_second_phoneme = pv_orca_frame_to_sec(start_frame_phoneme, sample_rate);
                float end_second_phoneme = pv_orca_frame_to_sec(end_frame_phoneme, sample_rate);

                status = pv_orca_phoneme_alignment_init(
                        phoneme,
                        start_second_phoneme,
                        end_second_phoneme,
                        &phonemes[num_added_phonemes]);
                if (status != PV_STATUS_SUCCESS) {
                    PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                            pv_orca_phoneme_alignment_init,
                            pv_status_to_string(status));
                    free(indices_merged);
                    for (int32_t k = 0; k < encoded_index; k++) {
                        pv_orca_phoneme_alignment_delete(phonemes[k]);
                    }
                    free(phonemes);
                    for (int32_t k = 0; k < word_index; k++) {
                        pv_orca_word_alignment_delete(aligns[k]);
                    }
                    free(aligns);
                    return status;
                }

                num_added_phonemes += 1;
                start_frame_phoneme = end_frame_phoneme;
            }

            if (!last_phoneme_in_word) {
                int32_t multiplier = object->phonemizer_param->num_phoneme_multiplier;
                if ((encoded_phonemes[token_index] % multiplier) == 0) {
                    prev_phoneme_index = encoded_phoneme_to_phoneme(object, encoded_phonemes[token_index]);
                }
                end_frame_phoneme += encoded_phonemes_durations[token_index];
            }

            token_index += 1;
        }

        PV_ASSERT(num_added_phonemes == num_true_phonemes);

        end_frame_word = end_frame_phoneme;

        float start_second_word = pv_orca_frame_to_sec(start_frame_word, sample_rate);
        float end_second_word = pv_orca_frame_to_sec(end_frame_word, sample_rate);

        ORCA_LOG_VERBOSE(
                "NEW WORD ALIGNMENT: `%s`, start, end: [%.3f, %.3f]",
                text_tokens[word_index]->original_string,
                start_second_word,
                end_second_word);

        status = pv_orca_word_alignment_init(
                text_tokens[word_index]->original_string,
                start_second_word,
                end_second_word,
                num_true_phonemes,
                phonemes,
                &aligns[word_index]);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_orca_word_alignment_init,
                    pv_status_to_string(status));
            free(indices_merged);
            for (int32_t k = 0; k < num_added_phonemes; k++) {
                pv_orca_phoneme_alignment_delete(phonemes[k]);
            }
            free(phonemes);
            for (int32_t k = 0; k < word_index; k++) {
                pv_orca_word_alignment_delete(aligns[k]);
            }
            free(aligns);
            return status;
        }

        bool same_original_string =
                (word_index > 0) &&
                (strcmp(text_tokens[word_index - 1]->original_string, text_tokens[word_index]->original_string) == 0);

        bool to_be_merged =
                same_original_string && (text_tokens[word_index]->length_past_context > 0);

        if (to_be_merged) {
            merge_alignments = true;
            indices_merged[word_index] = index_merged - 1;
        } else {
            indices_merged[word_index] = index_merged++;
        }
    }

    if (merge_alignments) {
        int32_t num_aligns_merged = 0;
        pv_orca_word_alignment_t **aligns_merged = NULL;
        status = pv_orca_merge_word_alignments(
                num_text_tokens,
                aligns,
                indices_merged,
                &num_aligns_merged,
                &aligns_merged);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_orca_merge_word_alignments,
                    pv_status_to_string(status));
            free(indices_merged);
            for (int32_t k = 0; k < num_text_tokens; k++) {
                pv_orca_word_alignment_delete(aligns[k]);
            }
            free(aligns);
            return status;
        }
        pv_orca_word_alignments_delete(num_text_tokens, aligns);

        *num_alignments = num_aligns_merged;
        *alignments = aligns_merged;
    } else {
        *num_alignments = num_text_tokens;
        *alignments = aligns;
    }

    free(indices_merged);

    return PV_STATUS_SUCCESS;
}

PV_API pv_status_t PV_MOCKABLE(pv_orca_synthesize)(
        const pv_orca_t *object,
        const char *text,
        const pv_orca_synthesize_params_t *synthesize_params,
        int32_t *num_samples,
        int16_t **pcm,
        int32_t *num_alignments,
        pv_orca_word_alignment_t ***alignments) {
    pv_error_prepare();

    return pv_orca_synthesize_internal(
            object,
            text,
            synthesize_params,
            false,
            false,
            num_samples,
            pcm,
            num_alignments,
            alignments);
}

pv_status_t PV_MOCKABLE(pv_orca_synthesize_internal)(
        const pv_orca_t *object,
        const char *text,
        const pv_orca_synthesize_params_t *synthesize_params,
        bool no_random_latents,
        bool no_text_additions,
        int32_t *num_samples,
        int16_t **pcm,
        int32_t *num_alignments,
        pv_orca_word_alignment_t ***alignments) {
    pv_error_prepare();

    if (!object) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_null,
                PV_ERROR_ARGS_PUBLIC("object"),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }
    if (!text) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_null,
                PV_ERROR_ARGS_PUBLIC("text"),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }
    if (!synthesize_params) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_null,
                PV_ERROR_ARGS_PUBLIC("synthesize_params"),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }
    if (!num_samples) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_null,
                PV_ERROR_ARGS_PUBLIC("num_samples"),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }
    if (!pcm) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_null,
                PV_ERROR_ARGS_PUBLIC("pcm"),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }
    if (!num_alignments) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_null,
                PV_ERROR_ARGS_PUBLIC("num_alignments"),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }
    if (!alignments) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_null,
                PV_ERROR_ARGS_PUBLIC("alignments"),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }

    size_t num_characters = strlen(text);
    if (num_characters == 0) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument,
                PV_ERROR_ARGS_PUBLIC("text"),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }
    if ((int32_t) num_characters > object->max_character_limit) {
        PV_ERROR_REPORT(
                &pv_error_msg_orca_max_character_limit_exceeded,
                PV_ERROR_ARGS_PUBLIC(object->max_character_limit),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }

    if (object->stream_state->status != PV_ORCA_STREAM_STATUS_INACTIVE) {
        PV_ERROR_REPORT_SIMPLE(&pv_error_msg_orca_stream_busy);
        return PV_STATUS_INVALID_STATE;
    }

    *num_samples = 0;
    *pcm = NULL;
    *num_alignments = 0;
    *alignments = NULL;

    int32_t num_text_tokens = 0;
    pv_normalizer_token_t **text_tokens = NULL;
    int32_t num_encoded_phonemes = 0;
    int32_t *encoded_phonemes = NULL;
    int32_t *text_tokens_num_encoded_phonemes = NULL;
    pv_status_t status = pv_orca_phonemize_text(
            object,
            text,
            true,
            no_text_additions,
            &num_text_tokens,
            &text_tokens,
            &num_encoded_phonemes,
            &encoded_phonemes,
            &text_tokens_num_encoded_phonemes);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_phonemize_text,
                pv_status_to_string(status));
        return status;
    }

#ifdef __ORCA_LOG_LEVEL_VERBOSE__

    ORCA_LOG_VERBOSE("num_encoded_phonemes: `%d`", num_encoded_phonemes);
    for (int32_t i = 0; i < num_encoded_phonemes; i++) {
        ORCA_LOG_VERBOSE_INLINE("%d ", encoded_phonemes[i]);
        if (i == (num_encoded_phonemes - 1)) {
            ORCA_LOG_VERBOSE_INLINE_SIMPLE("\n");
        }
    }

#endif

    int32_t num_samples_internal = 0;
    int16_t *pcm_internal = NULL;
    int32_t *encoded_phonemes_durations = NULL;
    status = pv_orca_synthesizer_forward(
            object->synthesizer,
            synthesize_params,
            no_random_latents,
            num_encoded_phonemes,
            (const int32_t *) encoded_phonemes,
            &encoded_phonemes_durations,
            &num_samples_internal,
            &pcm_internal);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_synthesizer_forward,
                pv_status_to_string(status));
        free(encoded_phonemes);
        free(text_tokens_num_encoded_phonemes);
        for (int32_t i = 0; i < num_text_tokens; i++) {
            pv_normalizer_token_delete(text_tokens[i]);
        }
        free(text_tokens);
        return status;
    }

    int32_t num_alignments_internal = 0;
    pv_orca_word_alignment_t **alignments_internal = NULL;
    status = pv_orca_create_word_alignments(
            object,
            num_text_tokens,
            text_tokens,
            text_tokens_num_encoded_phonemes,
            (const int32_t *) encoded_phonemes,
            encoded_phonemes_durations,
            &num_alignments_internal,
            &alignments_internal);
    free(encoded_phonemes_durations);
    free(encoded_phonemes);
    free(text_tokens_num_encoded_phonemes);
    for (int32_t i = 0; i < num_text_tokens; i++) {
        pv_normalizer_token_delete(text_tokens[i]);
    }
    free(text_tokens);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_create_word_alignments,
                pv_status_to_string(status));
        free(pcm_internal);
        return status;
    }

    status = pv_gatekeeper_usage_animal_validate_license(object->gatekeeper, (float) num_characters);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_gatekeeper_usage_animal_validate_license,
                pv_status_to_string(status));
        pv_orca_word_alignments_delete(num_text_tokens, alignments_internal);
        free(pcm_internal);
        return status;
    }

    *num_samples = num_samples_internal;
    *pcm = pcm_internal;
    *num_alignments = num_alignments_internal;
    *alignments = alignments_internal;

    return PV_STATUS_SUCCESS;
}

PV_API pv_status_t PV_MOCKABLE(pv_orca_synthesize_to_file)(
        const pv_orca_t *object,
        const char *text,
        const pv_orca_synthesize_params_t *synthesize_params,
        const char *output_path,
        int32_t *num_alignments,
        pv_orca_word_alignment_t ***alignments) {
    pv_error_prepare();

#if defined(__PV_TARGET_NO_FILE_INTERFACE__)

    (void) object;
    (void) text;
    (void) synthesize_params;
    (void) output_path;
    (void) num_alignments;
    (void) alignments;
    LOG_ERROR_SIMPLE(
            "This compilation of Orca does not support audio file output. "
            "Use pv_orca_synthesize and handle the raw PCM.");
    return PV_STATUS_IO_ERROR;

#else

    return pv_orca_synthesize_to_file_internal(
            object,
            text,
            synthesize_params,
            output_path,
            num_alignments,
            alignments);

#endif
}

#if !defined(__PV_TARGET_NO_FILE_INTERFACE__) && !defined(__PV_TARGET_NO_FILE_SYSTEM__)

pv_status_t PV_MOCKABLE(pv_orca_synthesize_to_file_internal)(
        const pv_orca_t *object,
        const char *text,
        const pv_orca_synthesize_params_t *synthesize_params,
        const char *output_path,
        int32_t *num_alignments,
        pv_orca_word_alignment_t ***alignments) {
    pv_error_prepare();

    if (!object) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_null,
                PV_ERROR_ARGS_PUBLIC("object"),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }
    if (!text) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_null,
                PV_ERROR_ARGS_PUBLIC("text"),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }
    if (!synthesize_params) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_null,
                PV_ERROR_ARGS_PUBLIC("synthesize_params"),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }
    if (!output_path || strlen(output_path) == 0) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_null,
                PV_ERROR_ARGS_PUBLIC("output_path"),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }

    int32_t sample_rate = 0;
    pv_orca_sample_rate(object, &sample_rate);

    pv_writer_wav_t *output_file;
    pv_status_t status = pv_writer_wav_init(output_path, sample_rate, &output_file);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_writer_wav_init,
                pv_status_to_string(status));
        return status;
    }

    int32_t num_samples = 0;
    int16_t *pcm = NULL;
    status = pv_orca_synthesize_internal(
            object,
            text,
            synthesize_params,
            false,
            false,
            &num_samples,
            &pcm,
            num_alignments,
            alignments);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_synthesize_internal,
                pv_status_to_string(status));
        pv_writer_wav_delete(output_file);
        return status;
    }

    status = pv_writer_wav_write(output_file, num_samples, pcm);
    pv_writer_wav_delete(output_file);
    free(pcm);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_writer_wav_write,
                pv_status_to_string(status));
        return status;
    }

    return PV_STATUS_SUCCESS;
}

#endif

PV_API void PV_MOCKABLE(pv_orca_pcm_delete)(int16_t *pcm) {
    free(pcm);
}

PV_API pv_status_t PV_MOCKABLE(pv_orca_word_alignments_delete)(
        const int32_t num_alignments,
        pv_orca_word_alignment_t **alignments) {
    if (num_alignments < 0) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_min,
                PV_ERROR_ARGS_PUBLIC("num_alignments", 0),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }

    if (alignments) {
        for (int32_t i = 0; i < num_alignments; i++) {
            pv_orca_word_alignment_delete(alignments[i]);
        }
        free(alignments);
    }

    return PV_STATUS_SUCCESS;
}

struct pv_orca_synthesize_params {
    float speech_rate;
    int64_t random_state;
};

PV_API pv_status_t PV_MOCKABLE(pv_orca_synthesize_params_init)(pv_orca_synthesize_params_t **object) {
    pv_error_prepare();

    *object = NULL;

    pv_orca_synthesize_params_t *o = calloc(1, sizeof(pv_orca_synthesize_params_t));
    PV_CHECK_ALLOC(o);

    o->speech_rate = PV_ORCA_DEFAULT_SPEECH_RATE;
    o->random_state = PV_ORCA_DEFAULT_RANDOM_STATE;

    *object = o;

    return PV_STATUS_SUCCESS;
}

PV_API void PV_MOCKABLE(pv_orca_synthesize_params_delete)(pv_orca_synthesize_params_t *object) {
    free(object);
}

pv_status_t PV_MOCKABLE(pv_orca_synthesize_params_copy)(
        const pv_orca_synthesize_params_t *source,
        pv_orca_synthesize_params_t *destination) {
    PV_ASSERT(source);
    PV_ASSERT(destination);

    destination->speech_rate = source->speech_rate;
    destination->random_state = source->random_state;

    return PV_STATUS_SUCCESS;
}

PV_API pv_status_t PV_MOCKABLE(pv_orca_synthesize_params_set_speech_rate)(
        pv_orca_synthesize_params_t *object,
        float speech_rate) {
    pv_error_prepare();

    if (!object) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_null,
                PV_ERROR_ARGS_PUBLIC("object"),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }
    if ((speech_rate < PV_ORCA_MIN_SPEECH_RATE) || (speech_rate > PV_ORCA_MAX_SPEECH_RATE)) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_range_float,
                PV_ERROR_ARGS_PUBLIC(
                        "speech_rate",
                        speech_rate,
                        PV_ORCA_MIN_SPEECH_RATE,
                        PV_ORCA_MAX_SPEECH_RATE),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }

    object->speech_rate = speech_rate;

    return PV_STATUS_SUCCESS;
}

PV_API pv_status_t PV_MOCKABLE(pv_orca_synthesize_params_get_speech_rate)(
        const pv_orca_synthesize_params_t *object,
        float *speech_rate) {
    pv_error_prepare();

    if (!object) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_null,
                PV_ERROR_ARGS_PUBLIC("object"),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }
    if (!speech_rate) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_null,
                PV_ERROR_ARGS_PUBLIC("speech_rate"),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }

    *speech_rate = object->speech_rate;

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_orca_synthesize_params_set_default_random_state)(
        pv_orca_synthesize_params_t *object) {
    if (!object) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_null,
                PV_ERROR_ARGS_PUBLIC("object"),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }

    object->random_state = PV_ORCA_DEFAULT_RANDOM_STATE;

    return PV_STATUS_SUCCESS;
}

PV_API pv_status_t PV_MOCKABLE(pv_orca_synthesize_params_set_random_state)(
        pv_orca_synthesize_params_t *object,
        int64_t random_state) {
    pv_error_prepare();

    if (!object) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_null,
                PV_ERROR_ARGS_PUBLIC("object"),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }
    if (random_state < 0) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_min,
                PV_ERROR_ARGS_PUBLIC(
                        "random_state",
                        (int32_t) random_state,
                        0),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }

    object->random_state = random_state;

    return PV_STATUS_SUCCESS;
}

PV_API pv_status_t PV_MOCKABLE(pv_orca_synthesize_params_get_random_state)(
        const pv_orca_synthesize_params_t *object,
        int64_t *random_state) {
    pv_error_prepare();

    if (!object) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_null,
                PV_ERROR_ARGS_PUBLIC("object"),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }
    if (!random_state) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_null,
                PV_ERROR_ARGS_PUBLIC("random_state"),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }

    *random_state = object->random_state;

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_orca_synthesize_params_get_random_state_valid)(
        const pv_orca_synthesize_params_t *object,
        int64_t *random_state) {
    PV_ASSERT(object);
    PV_ASSERT(random_state);

    pv_status_t status = pv_orca_synthesize_params_get_random_state(object, random_state);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_synthesize_params_get_random_state,
                pv_status_to_string(status));
        return status;
    }

    if (*random_state == PV_ORCA_DEFAULT_RANDOM_STATE) {
        *random_state = (int64_t) pv_time();
    }

    return PV_STATUS_SUCCESS;
}

PV_API void PV_MOCKABLE(pv_orca_valid_characters_delete)(const char *const *characters) {
    if (characters) {
        free((void *) characters);
    }
}

PV_API const char *PV_MOCKABLE(pv_orca_version)(void) {
    return PV_ORCA_VERSION;
}

PV_API pv_status_t PV_MOCKABLE(pv_orca_max_character_limit)(const pv_orca_t *object, int32_t *max_character_limit) {
    pv_error_prepare();
    if (!object) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_null,
                PV_ERROR_ARGS_PUBLIC("object"),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }
    if (!max_character_limit) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_null,
                PV_ERROR_ARGS_PUBLIC("max_character_limit"),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }

    *max_character_limit = object->max_character_limit;

    return PV_STATUS_SUCCESS;
}

struct pv_orca_stream {
    const pv_orca_t *orca;
    pv_orca_stream_state_t *stream_state;
    pv_normalizer_stream_t *normalizer_stream;
    pv_normalizer_token_t **prev_verbalized_tokens;
    int32_t prev_verbalized_token_count;
};

PV_API pv_status_t PV_MOCKABLE(pv_orca_stream_open)(
        pv_orca_t *object,
        const pv_orca_synthesize_params_t *synthesize_params,
        pv_orca_stream_t **stream) {
    pv_error_prepare();

    if (!object) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_null,
                PV_ERROR_ARGS_PUBLIC("object"),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }
    if (!synthesize_params) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_null,
                PV_ERROR_ARGS_PUBLIC("synthesize_params"),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }

    if (object->stream_state->status != PV_ORCA_STREAM_STATUS_INACTIVE) {
        PV_ERROR_REPORT_SIMPLE(&pv_error_msg_orca_stream_busy);
        return PV_STATUS_INVALID_STATE;
    }

    *stream = NULL;

    pv_orca_stream_t *s = calloc(1, sizeof(pv_orca_stream_t));
    if (!s) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("pv_orca_stream_t"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    s->orca = object;
    s->stream_state = object->stream_state;

    pv_status_t status = pv_orca_stream_state_open(s->stream_state, synthesize_params);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_stream_state_open,
                pv_status_to_string(status));
        free(s);
        return status;
    }

    status = pv_normalizer_stream_open(object->normalizer, &(s->normalizer_stream));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_normalizer_stream_open,
                pv_status_to_string(status));
        pv_orca_stream_state_close(s->stream_state);
        free(s);
        return status;
    }

    s->prev_verbalized_token_count = 0;
    s->prev_verbalized_tokens = NULL;

    *stream = s;

    return PV_STATUS_SUCCESS;
}

PV_API void PV_MOCKABLE(pv_orca_stream_close)(pv_orca_stream_t *object) {
    PV_ASSERT(object);

    if (object) {
        pv_normalizer_stream_close(object->normalizer_stream);
        pv_orca_stream_state_close(object->stream_state);
        for (int32_t i = 0; i < object->prev_verbalized_token_count; i++) {
            pv_normalizer_token_delete(object->prev_verbalized_tokens[i]);
        }
        free(object->prev_verbalized_tokens);
        free((pv_orca_stream_t *) object);
    }
}

PV_API pv_status_t PV_MOCKABLE(pv_orca_stream_synthesize)(
        pv_orca_stream_t *object,
        const char *text,
        int32_t *num_samples,
        int16_t **pcm) {
    pv_error_prepare();

    return pv_orca_stream_synthesize_internal(
            object,
            false,
            false,
            text,
            false,
            num_samples,
            pcm);
}

pv_status_t pv_orca_stream_append_token_arrays(
        pv_orca_stream_t *object,
        int32_t num_text_tokens,
        pv_normalizer_token_t **text_tokens) {
    PV_ASSERT(object);
    PV_ASSERT(num_text_tokens > 0);
    PV_ASSERT(text_tokens);

    int32_t new_count = object->prev_verbalized_token_count + num_text_tokens;
    pv_normalizer_token_t **new_prev_verbalized_token_array = malloc(new_count * sizeof(pv_normalizer_token_t *));
    if (!new_prev_verbalized_token_array) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("new_prev_verbalized_token_array"));
        return PV_STATUS_OUT_OF_MEMORY;
    }
    for (int32_t i = 0; i < object->prev_verbalized_token_count; i++) {
        new_prev_verbalized_token_array[i] = object->prev_verbalized_tokens[i];
    }
    for (int32_t i = 0; i < num_text_tokens; i++) {
        new_prev_verbalized_token_array[object->prev_verbalized_token_count + i] = text_tokens[i];
    }

    free(object->prev_verbalized_tokens);
    object->prev_verbalized_tokens = new_prev_verbalized_token_array;
    object->prev_verbalized_token_count = new_count;

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_orca_stream_synthesize_internal)(
        pv_orca_stream_t *object,
        bool no_random_latents,
        bool no_synthesis,
        const char *text,
        bool no_text_additions,
        int32_t *num_samples,
        int16_t **pcm) {
    pv_error_prepare();

    if (!object) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_null,
                PV_ERROR_ARGS_PUBLIC("object"),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }
    if (!text) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_null,
                PV_ERROR_ARGS_PUBLIC("text"),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }
    if (!num_samples) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_null,
                PV_ERROR_ARGS_PUBLIC("num_samples"),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }
    if (!pcm) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_null,
                PV_ERROR_ARGS_PUBLIC("pcm"),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }

    *num_samples = 0;
    *pcm = NULL;

    pv_orca_stream_state_t *state = object->stream_state;

    if (state->status != PV_ORCA_STREAM_STATUS_ACTIVE) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("stream_state->status"));
        return PV_STATUS_INVALID_STATE;
    }

    ORCA_LOG_VERBOSE_INLINE_SIMPLE("\n");
    ORCA_LOG_VERBOSE(
            "[ENTERING PV_ORCA_STREAM_SYNTHESIZE] New input text=`%s`, is_flush=`%d`",
            text,
            state->is_flush);

    pv_orca_stream_state_count_num_characters(state, text);

    pv_normalizer_token_list_t *new_token_list_to_phonemize = NULL;
    if (!state->is_flush) {
        pv_status_t status = pv_normalizer_stream_add(
                object->normalizer_stream,
                text,
                &new_token_list_to_phonemize);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_stream_add,
                    pv_status_to_string(status));
            return status;
        }
    } else {
        pv_status_t status = pv_normalizer_stream_flush(object->normalizer_stream, &new_token_list_to_phonemize);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_stream_flush,
                    pv_status_to_string(status));
            return status;
        }
    }

    if (!state->is_flush && !new_token_list_to_phonemize) {
        ORCA_LOG_VERBOSE_SIMPLE("[RETURN] No valid tokens to synthesize");
        return PV_STATUS_SUCCESS;
    }

    if ((new_token_list_to_phonemize != NULL) && (new_token_list_to_phonemize->size > 0)) {
        pv_status_t status = pv_orca_phonemizer_remove_invalid_custom_pronunciation_tokens(
                object->orca->phonemizer,
                new_token_list_to_phonemize);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_orca_phonemizer_remove_invalid_custom_pronunciation_tokens,
                    pv_status_to_string(status));
            pv_normalizer_token_list_delete(new_token_list_to_phonemize);
            return status;
        }

        if (new_token_list_to_phonemize->size == 0) {
            pv_normalizer_token_list_delete(new_token_list_to_phonemize);
            return PV_STATUS_SUCCESS;
        }

        int32_t num_text_tokens = new_token_list_to_phonemize->size;

        pv_normalizer_token_t **text_tokens = NULL;
        status = pv_normalizer_token_list_to_token_array(new_token_list_to_phonemize, &text_tokens);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_normalizer_token_list_to_token_array,
                    pv_status_to_string(status));
            pv_normalizer_token_list_delete(new_token_list_to_phonemize);
            return status;
        }


        bool append_terminator = false;
        bool allow_prepend_bos = false;
        bool allow_append_eos = false;
        if (no_text_additions) {
            // When testing for streaming and batching PCM match, then need to turn off the EOS, BOS, and terminator, because otherwise streaming and batching will naturally disagree.
            append_terminator = false;
            allow_prepend_bos = false;
            allow_append_eos = false;
        } else {
            append_terminator = state->is_flush ? object->orca->phonemizer_param->add_eos_punctuation : false;
            allow_prepend_bos = object->stream_state->is_first_chunk ? true : false;
            allow_append_eos = object->stream_state->is_flush ? true : false;
        }

        int32_t new_num_encoded_phonemes = 0;
        int32_t *new_encoded_phonemes = NULL;
        int32_t *text_tokens_num_encoded_phonemes = NULL;
        status = pv_orca_phonemizer_phonemize(
                object->orca->phonemizer,
                num_text_tokens,
                (const pv_normalizer_token_t **) text_tokens,
                object->prev_verbalized_token_count,
                (const pv_normalizer_token_t **) object->prev_verbalized_tokens,
                append_terminator,
                allow_prepend_bos,
                allow_append_eos,
                &new_num_encoded_phonemes,
                &new_encoded_phonemes,
                &text_tokens_num_encoded_phonemes);
        if (status != PV_STATUS_SUCCESS) {
            for (int32_t i = 0; i < num_text_tokens; i++) {
                pv_normalizer_token_delete(text_tokens[i]);
            }
            free(text_tokens);
            free(text_tokens_num_encoded_phonemes);
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_orca_phonemizer_phonemize,
                    pv_status_to_string(status));
            return status;
        }

        status = pv_orca_stream_append_token_arrays(object, num_text_tokens, text_tokens);
        free(text_tokens);
        free(text_tokens_num_encoded_phonemes);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_orca_phonemizer_phonemize,
                    pv_status_to_string(status));
            return status;
        }

        if (new_num_encoded_phonemes > 0) {
            status = pv_orca_stream_state_append_encoded_phonemes(
                    state,
                    new_num_encoded_phonemes,
                    new_encoded_phonemes);
            free(new_encoded_phonemes);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                        pv_orca_stream_state_append_encoded_phonemes,
                        pv_status_to_string(status));
                return status;
            }
        }
    }

    free(new_token_list_to_phonemize);

#ifdef __ORCA_LOG_LEVEL_VERBOSE__

    ORCA_LOG_VERBOSE(
            "Num encoded phonemes `%d`",
            pv_buffer_int32_length(state->buffer_encoded_phonemes));
    for (int32_t i = 0; i < pv_buffer_int32_length(state->buffer_encoded_phonemes); i++) {
        ORCA_LOG_VERBOSE_INLINE("%d ", pv_buffer_int32_get(state->buffer_encoded_phonemes)[i]);
        if (i == (pv_buffer_int32_length(state->buffer_encoded_phonemes) - 1)) {
            ORCA_LOG_VERBOSE_INLINE_SIMPLE("\n");
        }
    }

#endif

    if (pv_buffer_int32_length(state->buffer_encoded_phonemes) == 0) {
        ORCA_LOG_VERBOSE_SIMPLE("[RETURN] No phonemes to synthesize audio");
        return PV_STATUS_SUCCESS;
    }

    bool is_sufficient_context = pv_orca_stream_state_is_sufficient_context(state);
    if (!is_sufficient_context && !state->is_flush) {
        ORCA_LOG_VERBOSE_SIMPLE("[RETURN] Not enough context to synthesize audio");
        return PV_STATUS_SUCCESS;
    }

    if (no_synthesis) {
        ORCA_LOG_VERBOSE_SIMPLE("[RETURN] No synthesis requested.");
        int32_t num_encoded_phonemes_to_dp = 0;
        int32_t num_encoded_phonemes_to_flow = 0;
        pv_orca_stream_state_update_chunk_state(state);
        pv_orca_stream_state_update_encoder_state(
                state,
                pv_buffer_int32_length(state->buffer_encoded_phonemes),
                &num_encoded_phonemes_to_dp,
                &num_encoded_phonemes_to_flow);
        return PV_STATUS_SUCCESS;
    }

    ORCA_LOG_VERBOSE("[STREAM_SYNTHESIZE] Call synthesizer_forward with flush=`%d`", state->is_flush);

    int32_t *encoded_phonemes_durations = NULL;
    int32_t num_samples_internal = 0;
    int16_t *pcm_internal = NULL;
    pv_status_t status = pv_orca_synthesizer_forward(
            object->orca->synthesizer,
            state->synthesize_params,
            no_random_latents,
            pv_buffer_int32_length(state->buffer_encoded_phonemes),
            (const int32_t *) (pv_buffer_int32_get(state->buffer_encoded_phonemes)),
            &encoded_phonemes_durations,
            &num_samples_internal,
            &pcm_internal);
    free(encoded_phonemes_durations); // only used in non-streaming version
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_synthesizer_forward,
                pv_status_to_string(status));
        return status;
    }

    status = pv_gatekeeper_usage_animal_validate_license(
            object->orca->gatekeeper,
            (float) pv_orca_stream_state_flush_num_characters(state));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_gatekeeper_usage_animal_validate_license,
                pv_status_to_string(status));
        free(pcm_internal);
        return status;
    }

    pv_orca_stream_state_update_chunk_state(state);

    *num_samples = num_samples_internal;
    *pcm = pcm_internal;

    return PV_STATUS_SUCCESS;
}

PV_API pv_status_t PV_MOCKABLE(pv_orca_stream_flush)(
        pv_orca_stream_t *object,
        int32_t *num_samples,
        int16_t **pcm) {
    pv_error_prepare();

    return pv_orca_stream_flush_internal(
            object,
            false,
            false,
            false,
            num_samples,
            pcm);
}

pv_status_t PV_MOCKABLE(pv_orca_stream_flush_internal)(
        pv_orca_stream_t *object,
        bool no_random_latents,
        bool no_synthesis,
        bool no_text_additions,
        int32_t *num_samples,
        int16_t **pcm) {
    pv_error_prepare();

    if (!object) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_null,
                PV_ERROR_ARGS_PUBLIC("object"),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }
    if (!num_samples) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_null,
                PV_ERROR_ARGS_PUBLIC("num_samples"),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }
    if (!pcm) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_null,
                PV_ERROR_ARGS_PUBLIC("pcm"),
                PV_ERROR_ARGS_PRIVATE_EMPTY());
        return PV_STATUS_INVALID_ARGUMENT;
    }

    pv_orca_stream_state_t *state = object->stream_state;

    if (state->status != PV_ORCA_STREAM_STATUS_ACTIVE) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("stream_state->status"));
        return PV_STATUS_INVALID_STATE;
    }

    *num_samples = 0;
    *pcm = NULL;

    state->is_flush = true;

    int32_t num_samples_internal = 0;
    int16_t *pcm_internal = NULL;
    pv_status_t status = pv_orca_stream_synthesize_internal(
            object,
            no_random_latents,
            no_synthesis,
            "",
            no_text_additions,
            &num_samples_internal,
            &pcm_internal);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_stream_synthesize_internal,
                pv_status_to_string(status));
        return status;
    }

    pv_orca_stream_state_refresh(object->stream_state);
    pv_normalizer_stream_reset(object->normalizer_stream);

    *num_samples = num_samples_internal;
    *pcm = pcm_internal;

    return PV_STATUS_SUCCESS;
}
