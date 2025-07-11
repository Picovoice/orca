#include <jni.h>
#include <stdlib.h>
#include <string.h>

#include "core/picovoice.h"
#include "core/picovoice_internal.h"
#include "core/pv_assert.h"
#include "io/pv_log.h"
#include "orca/pv_orca.h"
#include "util/pv_string.h"

static pv_status_t pv_get_message_array(JNIEnv *env, jobjectArray *message_array) {
    int32_t message_stack_depth = 0;
    char **message_stack = NULL;
    pv_status_t status = pv_get_error_stack(&message_stack, &message_stack_depth);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    *message_array = (*env)->NewObjectArray(
            env,
            message_stack_depth,
            (*env)->FindClass(env, "java/lang/String"),
            (*env)->NewStringUTF(env, ""));

    for (int32_t i = 0; i < message_stack_depth; i++) {
        (*env)->SetObjectArrayElement(env, *message_array, i, (*env)->NewStringUTF(env, message_stack[i]));
    }

    pv_free_error_stack(message_stack);
    return PV_STATUS_SUCCESS;
}

static jint pv_throw_exception(JNIEnv *env, pv_status_t status, const char *message, bool is_pv_error) {
    const char *java_exception_equivalent = NULL;

    jobjectArray message_array;
    pv_status_t err_status;
    if (is_pv_error) {
        err_status = pv_get_message_array(env, &message_array);
        if (err_status != PV_STATUS_SUCCESS) {
            status = err_status;
            message = "Unable to get Orca error status";
        }
    }

    switch (status) {
        case PV_STATUS_OUT_OF_MEMORY:
            java_exception_equivalent = "ai/picovoice/orca/OrcaMemoryException";
            break;
        case PV_STATUS_IO_ERROR:
            java_exception_equivalent = "ai/picovoice/orca/OrcaIOException";
            break;
        case PV_STATUS_INVALID_ARGUMENT:
            java_exception_equivalent = "ai/picovoice/orca/OrcaInvalidArgumentException";
            break;
        case PV_STATUS_STOP_ITERATION:
            java_exception_equivalent = "ai/picovoice/orca/OrcaStopIterationException";
            break;
        case PV_STATUS_KEY_ERROR:
            java_exception_equivalent = "ai/picovoice/orca/OrcaKeyException";
            break;
        case PV_STATUS_INVALID_STATE:
            java_exception_equivalent = "ai/picovoice/orca/OrcaInvalidStateException";
            break;
        case PV_STATUS_ACTIVATION_ERROR:
            java_exception_equivalent = "ai/picovoice/orca/OrcaActivationException";
            break;
        case PV_STATUS_ACTIVATION_LIMIT_REACHED:
            java_exception_equivalent = "ai/picovoice/orca/OrcaActivationLimitException";
            break;
        case PV_STATUS_ACTIVATION_REFUSED:
            java_exception_equivalent = "ai/picovoice/orca/OrcaActivationRefusedException";
            break;
        case PV_STATUS_ACTIVATION_THROTTLED:
            java_exception_equivalent = "ai/picovoice/orca/OrcaActivationThrottledException";
            break;
        case PV_STATUS_RUNTIME_ERROR:
            java_exception_equivalent = "ai/picovoice/orca/OrcaRuntimeException";
            break;
        default:
            java_exception_equivalent = "ai/picovoice/orca/OrcaException";
            return (*env)->ThrowNew(env, (*env)->FindClass(env, java_exception_equivalent),
                                    pv_string_format(
                                            "%s: %s",
                                            pv_status_to_string(status),
                                            message));
    }

    if (is_pv_error && (err_status == PV_STATUS_SUCCESS) && ((*env)->GetArrayLength(env, message_array) > 0)) {
        jclass exception_class = (*env)->FindClass(env, java_exception_equivalent);
        jstring exception_message = (*env)->NewStringUTF(env, message);
        jmethodID exception_constructor = (*env)->GetMethodID(
                env,
                exception_class,
                "<init>",
                "(Ljava/lang/String;[Ljava/lang/String;)V");
        jobject exception_object = (*env)->NewObject(
                env,
                exception_class,
                exception_constructor,
                exception_message,
                message_array);
        return (*env)->Throw(env, exception_object);
    } else {
        return (*env)->ThrowNew(env, (*env)->FindClass(env, java_exception_equivalent), message);
    }
}

static pv_status_t create_synthesize_params_c_object(
        JNIEnv *env,
        jfloat j_speech_rate,
        jlong j_random_state,
        pv_orca_synthesize_params_t **synthesize_params) {
    pv_status_t status = pv_orca_synthesize_params_init(synthesize_params);
    if (status != PV_STATUS_SUCCESS) {
        pv_throw_exception(
                env,
                status,
                "Failed to init Orca synthesize params object",
                true);
        return status;
    }

    status = pv_orca_synthesize_params_set_speech_rate(*synthesize_params, j_speech_rate);
    if (status != PV_STATUS_SUCCESS) {
        pv_throw_exception(
                env,
                status,
                "Failed to set speech rate",
                true);
        return status;
    }

    if (j_random_state >= 0) {
        status = pv_orca_synthesize_params_set_random_state(*synthesize_params, j_random_state);
        if (status != PV_STATUS_SUCCESS) {
            pv_throw_exception(
                    env,
                    status,
                    "Failed to set random state",
                    true);
            return status;
        }
    }

    return PV_STATUS_SUCCESS;
}

static pv_status_t build_common_inputs(
        JNIEnv *env,
        jlong object_id,
        jfloat j_speech_rate,
        jlong j_random_state,
        pv_orca_synthesize_params_t **synthesize_params,
        pv_orca_t **handle) {
    if (!object_id) {
        pv_throw_exception(env, PV_STATUS_INVALID_ARGUMENT, "Invalid object ID.", false);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    if (!j_speech_rate) {
        pv_throw_exception(env, PV_STATUS_INVALID_ARGUMENT, "synthesize params is 'NULL`.", false);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    if (!j_random_state) {
        pv_throw_exception(env, PV_STATUS_INVALID_ARGUMENT, "random state is 'NULL`.", false);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    if (!synthesize_params) {
        pv_throw_exception(env, PV_STATUS_INVALID_ARGUMENT, "synthesize params is 'NULL`.", false);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    if (!handle) {
        pv_throw_exception(env, PV_STATUS_INVALID_ARGUMENT, "handle is 'NULL`.", false);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    pv_status_t status = create_synthesize_params_c_object(env, j_speech_rate, j_random_state, synthesize_params);
    if (status != PV_STATUS_SUCCESS) {
        pv_throw_exception(
                env,
                status,
                "Failed to create Orca synthesize params object",
                true);
        return status;
    }

    *handle = (pv_orca_t *) (uintptr_t) object_id;

    return PV_STATUS_SUCCESS;
}

static jobject create_orca_audio_object(
        JNIEnv *env,
        const int32_t num_samples,
        int16_t *pcm,
        const int32_t num_alignments,
        pv_orca_word_alignment_t **alignments) {
    PV_ASSERT(env);
    PV_ASSERT((pcm && num_samples > 0) || (!pcm && num_samples == 0));
    PV_ASSERT(num_alignments > 0);
    PV_ASSERT(alignments);

    jclass word_class = (*env)->FindClass(env, "ai/picovoice/orca/OrcaWord");
    if (!word_class) {
        pv_throw_exception(
                env,
                PV_STATUS_RUNTIME_ERROR,
                "Cannot find `OrcaWord` class at `ai/picovoice/orca/OrcaWord`",
                false);
        return NULL;
    }

    jmethodID word_constructor = (*env)->GetMethodID(
            env,
            word_class,
            "<init>",
            "(Ljava/lang/String;FF[Lai/picovoice/orca/OrcaPhoneme;)V");
    if (!word_constructor) {
        pv_throw_exception(
                env,
                PV_STATUS_RUNTIME_ERROR,
                "Cannot find `OrcaWord` class's constructor",
                false);
        return NULL;
    }

    jclass phoneme_class = (*env)->FindClass(env, "ai/picovoice/orca/OrcaPhoneme");
    if (!phoneme_class) {
        pv_throw_exception(
                env,
                PV_STATUS_RUNTIME_ERROR,
                "Cannot find `Phoneme` class at `ai/picovoice/orca/OrcaPhoneme`",
                false);
        return NULL;
    }

    jmethodID phoneme_constructor = (*env)->GetMethodID(
            env,
            phoneme_class,
            "<init>",
            "(Ljava/lang/String;FF)V");
    if (!phoneme_constructor) {
        pv_throw_exception(
                env,
                PV_STATUS_RUNTIME_ERROR,
                "Cannot find `OrcaPhoneme` class's constructor",
                false);
        return NULL;
    }

    jobjectArray j_word_alignments = (*env)->NewObjectArray(env, num_alignments, word_class, NULL);
    if (!j_word_alignments) {
        pv_throw_exception(
                env,
                PV_STATUS_OUT_OF_MEMORY,
                "Unable to allocate memory for word alignments.",
                false);
        return NULL;
    }

    for (int32_t i = 0; i < num_alignments; i++) {
        pv_orca_word_alignment_t *word = alignments[i];

        jobjectArray j_phoneme_alignments = (*env)->NewObjectArray(env, word->num_phonemes, phoneme_class, NULL);
        if (!j_phoneme_alignments) {
            pv_throw_exception(
                    env,
                    PV_STATUS_OUT_OF_MEMORY,
                    "Unable to allocate memory for phoneme alignments.",
                    false);
            return NULL;
        }

        for (int32_t j = 0; j < word->num_phonemes; j++) {
            pv_orca_phoneme_alignment_t *phoneme = word->phonemes[j];

            jstring j_phoneme = (*env)->NewStringUTF(env, phoneme->phoneme);
            if (!j_phoneme) {
                pv_throw_exception(
                        env,
                        PV_STATUS_OUT_OF_MEMORY,
                        "Unable to allocate phoneme string",
                        false);
                return NULL;
            }

            jobject j_phoneme_object = (*env)->NewObject(
                    env,
                    phoneme_class,
                    phoneme_constructor,
                    j_phoneme,
                    phoneme->start_sec,
                    phoneme->end_sec);
            if (!j_phoneme_object) {
                pv_throw_exception(
                        env,
                        PV_STATUS_OUT_OF_MEMORY,
                        "Unable to allocate phoneme object",
                        false);
                return NULL;
            }

            (*env)->SetObjectArrayElement(env, j_phoneme_alignments, j, j_phoneme_object);
        }

        jstring j_word = (*env)->NewStringUTF(env, word->word);
        if (!j_word) {
            pv_throw_exception(
                    env,
                    PV_STATUS_OUT_OF_MEMORY,
                    "Unable to allocate word string",
                    false);
            return NULL;
        }

        jobject j_word_object = (*env)->NewObject(
                env,
                word_class,
                word_constructor,
                j_word,
                word->start_sec,
                word->end_sec,
                j_phoneme_alignments);
        if (!j_word_object) {
            pv_throw_exception(
                    env,
                    PV_STATUS_OUT_OF_MEMORY,
                    "Unable to allocate word object",
                    false);
            return NULL;
        }

        (*env)->SetObjectArrayElement(env, j_word_alignments, i, j_word_object);
    }

    jshortArray j_pcm = NULL;
    if (pcm) {
        j_pcm = (*env)->NewShortArray(env, num_samples);
        if (!j_pcm) {
            pv_throw_exception(
                    env,
                    PV_STATUS_OUT_OF_MEMORY,
                    "Unable to allocate memory for PCM array.",
                    false);
            return NULL;
        }

        (*env)->SetShortArrayRegion(env, j_pcm, 0, num_samples, pcm);
    }

    jclass orca_audio_class = (*env)->FindClass(env, "ai/picovoice/orca/OrcaAudio");
    if (!orca_audio_class) {
        pv_throw_exception(
                env,
                PV_STATUS_RUNTIME_ERROR,
                "Cannot find `OrcaAudio` class at `ai/picovoice/orca/OrcaAudio`",
                false);
        return NULL;
    }

    jmethodID orca_audio_constructor = (*env)->GetMethodID(
            env,
            orca_audio_class,
            "<init>",
            "([S[Lai/picovoice/orca/OrcaWord;)V");
    if (!orca_audio_constructor) {
        pv_throw_exception(
                env,
                PV_STATUS_RUNTIME_ERROR,
                "Cannot find `OrcaAudio` class's constructor",
                false);
        return NULL;
    }

    return (*env)->NewObject(
            env,
            orca_audio_class,
            orca_audio_constructor,
            j_pcm,
            j_word_alignments);
}

JNIEXPORT jlong JNICALL Java_ai_picovoice_orca_OrcaNative_init(
        JNIEnv *env,
        jobject j_object,
        jstring j_access_key,
        jstring j_model_path) {
    (void) j_object;

    if (!j_access_key) {
        pv_throw_exception(env, PV_STATUS_INVALID_ARGUMENT, "AccessKey is 'NULL'.", false);
        return 0;
    }

    if (!j_model_path) {
        pv_throw_exception(env, PV_STATUS_INVALID_ARGUMENT, "Model path is 'NULL'.", false);
        return 0;
    }

    const char *access_key = (*env)->GetStringUTFChars(env, j_access_key, NULL);
    if (!access_key) {
        pv_throw_exception(
                env,
                PV_STATUS_OUT_OF_MEMORY,
                "Failed to transfer AccessKey string.",
                false);
        return 0;
    }

    const char *model_path = (*env)->GetStringUTFChars(env, j_model_path, NULL);
    if (!model_path) {
        pv_throw_exception(
                env,
                PV_STATUS_OUT_OF_MEMORY,
                "Failed to transfer model path string.",
                false);
        return 0;
    }

    LOG_DEBUG("Orca model path : '%s'", model_path);

    pv_orca_t *handle = NULL;
    const pv_status_t status = pv_orca_init(
            access_key,
            model_path,
            &handle);

    (*env)->ReleaseStringUTFChars(env, j_access_key, access_key);
    (*env)->ReleaseStringUTFChars(env, j_model_path, model_path);

    if (status != PV_STATUS_SUCCESS) {
        pv_throw_exception(env, status, "Initialization failed", true);
        return 0;
    }

    return (jlong) (uintptr_t) handle;
}

JNIEXPORT void JNICALL Java_ai_picovoice_orca_OrcaNative_delete(JNIEnv *env, jobject j_object, jlong object_id) {
    (void) env;
    (void) j_object;

    if (!object_id) {
        return;
    }

    pv_orca_delete((pv_orca_t *) (uintptr_t) object_id);
}

JNIEXPORT jcharArray JNICALL Java_ai_picovoice_orca_OrcaNative_getValidCharacters(
        JNIEnv *env,
        jobject j_object,
        jlong object_id) {
    (void) j_object;

    if (!object_id) {
        pv_throw_exception(env, PV_STATUS_INVALID_ARGUMENT, "Invalid object ID.", false);
        return 0;
    }

    pv_orca_t *handle = (pv_orca_t *) (uintptr_t) object_id;

    int32_t num_characters = 0;
    const char *const *characters = NULL;
    pv_status_t status = pv_orca_valid_characters(handle, &num_characters, &characters);
    if (status != PV_STATUS_SUCCESS) {
        pv_throw_exception(env, status, "Failed to get characters", true);
        return 0;
    }

    jobjectArray j_characters = (*env)->NewObjectArray(
            env,
            num_characters,
            (*env)->FindClass(env, "java/lang/String"),
            (*env)->NewStringUTF(env, ""));
    if (!j_characters) {
        pv_throw_exception(
                env,
                PV_STATUS_OUT_OF_MEMORY,
                "Unable to allocate memory for characters.",
                false);
        return NULL;
    }

    for (int32_t i = 0; i < num_characters; i++) {
        (*env)->SetObjectArrayElement(env, j_characters, i, (*env)->NewStringUTF(env, characters[i]));
    }

    pv_orca_valid_characters_delete(characters);

    return j_characters;
}


JNIEXPORT jint JNICALL Java_ai_picovoice_orca_OrcaNative_getSampleRate(
        JNIEnv *env,
        jobject j_object,
        jlong object_id) {
    (void) j_object;

    if (!object_id) {
        pv_throw_exception(env, PV_STATUS_INVALID_ARGUMENT, "Invalid object ID.", false);
        return 0;
    }

    pv_orca_t *handle = (pv_orca_t *) (uintptr_t) object_id;

    int32_t sample_rate = 0;
    pv_status_t status = pv_orca_sample_rate(handle, &sample_rate);
    if (status != PV_STATUS_SUCCESS) {
        pv_throw_exception(env, status, "Failed to get sample rate", true);
        return 0;
    }

    return sample_rate;
}

JNIEXPORT jobject JNICALL Java_ai_picovoice_orca_OrcaNative_synthesize(
        JNIEnv *env,
        jobject j_object,
        jlong object_id,
        jstring j_text,
        jfloat j_speech_rate,
        jlong j_random_state) {
    (void) j_object;

    if (!j_text) {
        pv_throw_exception(env, PV_STATUS_INVALID_ARGUMENT, "text is 'NULL`.", false);
        return NULL;
    }

    pv_orca_synthesize_params_t *synthesize_params = NULL;
    pv_orca_t *handle = NULL;
    pv_status_t status = build_common_inputs(
            env,
            object_id,
            j_speech_rate,
            j_random_state,
            &synthesize_params,
            &handle);
    if (status != PV_STATUS_SUCCESS) {
        pv_throw_exception(env, status, "invalid arguments", true);
        return NULL;
    }

    const char *text = (*env)->GetStringUTFChars(env, j_text, NULL);
    if (!text) {
        pv_throw_exception(env, PV_STATUS_OUT_OF_MEMORY, "Failed to transfer text string.", false);
        return NULL;
    }

    int16_t *pcm = NULL;
    int32_t num_samples = 0;
    int32_t num_alignments = 0;
    pv_orca_word_alignment_t **alignments = NULL;
    status = pv_orca_synthesize(handle, text, synthesize_params, &num_samples, &pcm, &num_alignments, &alignments);

    (*env)->ReleaseStringUTFChars(env, j_text, text);
    pv_orca_synthesize_params_delete(synthesize_params);

    if (status != PV_STATUS_SUCCESS) {
        pv_throw_exception(env, status, "Failed to synthesize text", true);
        return NULL;
    }

    jobject res = create_orca_audio_object(env, num_samples, pcm, num_alignments, alignments);

    pv_orca_pcm_delete(pcm);

    if (alignments) {
        status = pv_orca_word_alignments_delete(num_alignments, alignments);
        if (status != PV_STATUS_SUCCESS) {
            pv_throw_exception(env, status, "Failed to delete word alignments", true);
            return NULL;
        }
    }

    return res;
}

JNIEXPORT jobject JNICALL Java_ai_picovoice_orca_OrcaNative_synthesizeToFile(
        JNIEnv *env,
        jobject j_object,
        jlong object_id,
        jstring j_text,
        jstring j_output_path,
        jfloat j_speech_rate,
        jlong j_random_state) {
    (void) j_object;

    if (!j_text) {
        pv_throw_exception(env, PV_STATUS_INVALID_ARGUMENT, "text is 'NULL`.", false);
        return NULL;
    }

    if (!j_output_path) {
        pv_throw_exception(env, PV_STATUS_INVALID_ARGUMENT, "output path is 'NULL`.", false);
        return NULL;
    }

    pv_orca_synthesize_params_t *synthesize_params = NULL;
    pv_orca_t *handle = NULL;
    pv_status_t status = build_common_inputs(
            env,
            object_id,
            j_speech_rate,
            j_random_state,
            &synthesize_params,
            &handle);
    if (status != PV_STATUS_SUCCESS) {
        pv_throw_exception(env, status, "invalid arguments", true);
        return NULL;
    }

    const char *output_path = (*env)->GetStringUTFChars(env, j_output_path, NULL);
    if (!output_path) {
        pv_throw_exception(env, PV_STATUS_OUT_OF_MEMORY, "Failed to transfer output path string.", false);
        return NULL;
    }

    const char *text = (*env)->GetStringUTFChars(env, j_text, NULL);
    if (!text) {
        pv_throw_exception(env, PV_STATUS_OUT_OF_MEMORY, "Failed to transfer text string.", false);
        return NULL;
    }

    int32_t num_alignments = 0;
    pv_orca_word_alignment_t **alignments = NULL;
    status = pv_orca_synthesize_to_file(handle, text, synthesize_params, output_path, &num_alignments, &alignments);

    (*env)->ReleaseStringUTFChars(env, j_output_path, output_path);
    (*env)->ReleaseStringUTFChars(env, j_text, text);
    pv_orca_synthesize_params_delete(synthesize_params);

    if (status != PV_STATUS_SUCCESS) {
        pv_throw_exception(env, status, "Failed to synthesize text", true);
        return NULL;
    }

    jobject res = create_orca_audio_object(env, 0, NULL, num_alignments, alignments);

    if (alignments) {
        status = pv_orca_word_alignments_delete(num_alignments, alignments);
        if (status != PV_STATUS_SUCCESS) {
            pv_throw_exception(env, status, "Failed to delete word alignments", true);
            return NULL;
        }
    }

    return res;
}

JNIEXPORT jlong JNICALL Java_ai_picovoice_orca_OrcaNative_streamOpen(
        JNIEnv *env,
        jobject j_object,
        jlong object_id,
        jfloat j_speech_rate,
        jlong j_random_state) {
    (void) j_object;

    pv_orca_t *handle = NULL;
    pv_orca_synthesize_params_t *synthesize_params = NULL;
    pv_status_t status = build_common_inputs(
            env,
            object_id,
            j_speech_rate,
            j_random_state,
            &synthesize_params,
            &handle);

    if (status != PV_STATUS_SUCCESS) {
        pv_throw_exception(env, status, "invalid arguments", true);
        return 0;
    }

    pv_orca_stream_t *stream;
    status = pv_orca_stream_open(handle, synthesize_params, &stream);
    pv_orca_synthesize_params_delete(synthesize_params);

    if (status != PV_STATUS_SUCCESS) {
        pv_throw_exception(env, status, "Failed to open stream", true);
        return 0;
    }

    return (jlong) (uintptr_t) stream;
}

JNIEXPORT jobject JNICALL Java_ai_picovoice_orca_OrcaNative_streamSynthesize(
        JNIEnv *env,
        jobject j_object,
        jlong object_id,
        jstring j_text) {
    (void) j_object;

    if (!object_id) {
        pv_throw_exception(env, PV_STATUS_INVALID_ARGUMENT, "Invalid stream ID.", false);
        return NULL;
    }

    const char *text = (*env)->GetStringUTFChars(env, j_text, NULL);
    if (!text) {
        pv_throw_exception(env, PV_STATUS_OUT_OF_MEMORY, "Failed to transfer text string.", false);
        return NULL;
    }

    pv_orca_stream_t *stream = (pv_orca_stream_t *) (uintptr_t) object_id;

    int16_t *pcm = NULL;
    int32_t num_samples = 0;
    pv_status_t status = pv_orca_stream_synthesize(stream, text, &num_samples, &pcm);

    (*env)->ReleaseStringUTFChars(env, j_text, text);

    if (status != PV_STATUS_SUCCESS) {
        pv_throw_exception(env, status, "Failed to synthesize stream", true);
        return NULL;
    }

    jshortArray j_pcm = (*env)->NewShortArray(env, num_samples);
    if (!j_pcm) {
        pv_throw_exception(
                env,
                PV_STATUS_OUT_OF_MEMORY,
                "Unable to allocate memory for enhanced PCM array.",
                false);
        return NULL;
    }
    (*env)->SetShortArrayRegion(env, j_pcm, 0, num_samples, pcm);

    pv_orca_pcm_delete(pcm);

    return j_pcm;
}

JNIEXPORT jobject JNICALL Java_ai_picovoice_orca_OrcaNative_streamFlush(
        JNIEnv *env,
        jobject j_object,
        jlong object_id) {
    (void) j_object;

    if (!object_id) {
        pv_throw_exception(env, PV_STATUS_INVALID_ARGUMENT, "Invalid stream ID.", false);
        return NULL;
    }

    pv_orca_stream_t *stream = (pv_orca_stream_t *) (uintptr_t) object_id;

    int16_t *pcm = NULL;
    int32_t num_samples = 0;
    pv_status_t status = pv_orca_stream_flush(stream, &num_samples, &pcm);

    if (status != PV_STATUS_SUCCESS) {
        pv_throw_exception(env, status, "Failed to flush stream", true);
        return NULL;
    }

    jshortArray j_pcm = (*env)->NewShortArray(env, num_samples);
    if (!j_pcm) {
        pv_throw_exception(
                env,
                PV_STATUS_OUT_OF_MEMORY,
                "Unable to allocate memory for enhanced PCM array.",
                false);
        return NULL;
    }
    (*env)->SetShortArrayRegion(env, j_pcm, 0, num_samples, pcm);

    pv_orca_pcm_delete(pcm);

    return j_pcm;
}

JNIEXPORT void JNICALL Java_ai_picovoice_orca_OrcaNative_streamClose(JNIEnv *env, jobject j_object, jlong object_id) {
    (void) env;
    (void) j_object;

    if (!object_id) {
        return;
    }

    pv_orca_stream_t *stream = (pv_orca_stream_t *) (uintptr_t) object_id;

    pv_orca_stream_close(stream);
}

JNIEXPORT jint JNICALL Java_ai_picovoice_orca_OrcaNative_getMaxCharacterLimit(
        JNIEnv *env,
        jobject j_object,
        jlong object_id) {
    (void) j_object;

    if (!object_id) {
        pv_throw_exception(env, PV_STATUS_INVALID_ARGUMENT, "Invalid object ID.", false);
        return 0;
    }

    pv_orca_t *handle = (pv_orca_t *) (uintptr_t) object_id;

    int32_t max_character_limit = 0;
    pv_status_t status = pv_orca_max_character_limit(handle, &max_character_limit);
    if (status != PV_STATUS_SUCCESS) {
        pv_throw_exception(env, status, "Failed to get max character limit", true);
        return 0;
    }

    return max_character_limit;
}

JNIEXPORT jstring JNICALL Java_ai_picovoice_orca_OrcaNative_getVersion(JNIEnv *env, jobject j_object) {
    (void) j_object;

    jstring j_version = (*env)->NewStringUTF(env, pv_orca_version());
    if (!j_version) {
        pv_throw_exception(env, PV_STATUS_OUT_OF_MEMORY, "Failed to get version string.", false);
        return NULL;
    }

    return j_version;
}

JNIEXPORT void JNICALL Java_ai_picovoice_orca_OrcaNative_setSdk(
        JNIEnv *env,
        jobject j_object,
        jstring j_sdk) {
    (void) j_object;

    if (!j_sdk) {
        pv_throw_exception(env, PV_STATUS_INVALID_ARGUMENT, "SDK string is `NULL`.", false);
        return;
    }

    const char *sdk = (*env)->GetStringUTFChars(env, j_sdk, NULL);
    if (!sdk) {
        pv_throw_exception(
                env,
                PV_STATUS_OUT_OF_MEMORY,
                "Failed to transfer SDK string.",
                false);
        return;
    }

    pv_set_sdk(sdk);

    (*env)->ReleaseStringUTFChars(env, j_sdk, sdk);
}
