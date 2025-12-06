#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "core/pv_error_messages.h"
#include "io/pv_log.h"
#include "math/pv_math.h"
#include "orca/pv_orca_duration_predictor.h"
#include "orca/pv_orca_flow.h"
#include "orca/pv_orca_internal.h"
#include "orca/pv_orca_stream_state.h"
#include "orca/pv_orca_synthesizer.h"
#include "orca/pv_orca_text_encoder.h"
#include "orca/pv_orca_util.h"
#include "orca/pv_orca_vocoder.h"
#include "orca/pv_profiler.h"
#include "util/pv_check_status.h"
#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

static const float PV_ORCA_SYNTHESIZER_DEFAULT_NOISE_SCALE = 0.5f;

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_orca_synthesizer_param_serialize)(
        pv_ypu_t *ypu,
        const pv_orca_synthesizer_param_t *param,
        FILE *f) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(f);

    size_t count = fwrite(&(param->sample_rate), sizeof(int32_t), 1, f);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

    pv_status_t status = pv_orca_text_encoder_param_serialize(ypu, param->text_encoder_param, f);
    PV_CHECK_STATUS(status);

    status = pv_orca_duration_predictor_param_serialize(ypu, param->duration_predictor_param, f);
    PV_CHECK_STATUS(status);

    status = pv_orca_flow_param_serialize(ypu, param->flow_param, f);
    PV_CHECK_STATUS(status);

    status = pv_orca_vocoder_param_serialize(ypu, param->vocoder_param, f);
    PV_CHECK_STATUS(status);

    return PV_STATUS_SUCCESS;
}

#endif

pv_status_t PV_MOCKABLE(pv_orca_synthesizer_param_load)(
        pv_ypu_t *ypu,
        FILE *f,
        const char *version,
        pv_orca_synthesizer_param_t **param) {
    PV_ASSERT(ypu);
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_orca_synthesizer_param_t *p = pv_ypu_host_alloc(ypu, sizeof(pv_orca_synthesizer_param_t));
    if (!p) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("pv_orca_synthesizer_param_t"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(p, 0, sizeof(pv_orca_synthesizer_param_t));

    const size_t version_length = strlen(version);
    p->version = pv_ypu_host_alloc(ypu, (int32_t) ((version_length + 1) * sizeof(char)));
    if (!(p->version)) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p->version"));
        return PV_STATUS_OUT_OF_MEMORY;
    }
    memcpy((char *) (p->version), version, version_length + 1);

    size_t count = pv_fread((int32_t *) &(p->sample_rate), sizeof(int32_t), 1, f);
    if (count != 1) {
        PV_ERROR_REPORT(
                &pv_error_msg_fread_failure,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE(f));
        pv_orca_synthesizer_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }

    pv_status_t status = pv_orca_text_encoder_param_load(
            ypu,
            f,
            (pv_orca_text_encoder_param_t **) &(p->text_encoder_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_text_encoder_param_load,
                pv_status_to_string(status));
        pv_orca_synthesizer_param_delete(ypu, p);
        return status;
    }

    status = pv_orca_duration_predictor_param_load(
            ypu,
            f,
            (pv_orca_duration_predictor_param_t **) &(p->duration_predictor_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_duration_predictor_param_load,
                pv_status_to_string(status));
        pv_orca_synthesizer_param_delete(ypu, p);
        return status;
    }

    status = pv_orca_flow_param_load(ypu, f, (pv_orca_flow_param_t **) &(p->flow_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_flow_param_load,
                pv_status_to_string(status));
        pv_orca_synthesizer_param_delete(ypu, p);
        return status;
    }

    status = pv_orca_vocoder_param_load(ypu, f, (pv_orca_vocoder_param_t **) &(p->vocoder_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_vocoder_param_load,
                pv_status_to_string(status));
        pv_orca_synthesizer_param_delete(ypu, p);
        return status;
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_orca_synthesizer_param_delete)(pv_ypu_t *ypu, pv_orca_synthesizer_param_t *param) {
    PV_ASSERT(ypu);

    if (param) {
        pv_orca_vocoder_param_delete(ypu, (pv_orca_vocoder_param_t *) (param->vocoder_param));

        pv_orca_flow_param_delete(ypu, (pv_orca_flow_param_t *) (param->flow_param));

        pv_orca_duration_predictor_param_delete(
                ypu,
                (pv_orca_duration_predictor_param_t *) (param->duration_predictor_param));

        pv_orca_text_encoder_param_delete(ypu, (pv_orca_text_encoder_param_t *) (param->text_encoder_param));

        pv_ypu_host_free(ypu, (char *) (param->version));

        pv_ypu_host_free(ypu, param);
    }
}

bool PV_MOCKABLE(pv_orca_synthesizer_param_is_equal)(
        const pv_orca_synthesizer_param_t *object,
        const pv_orca_synthesizer_param_t *other) {
    PV_ASSERT(object);
    PV_ASSERT(other);

    if (!pv_orca_text_encoder_param_is_equal(object->text_encoder_param, other->text_encoder_param)) {
        return false;
    }

    if (!pv_orca_duration_predictor_param_is_equal(object->duration_predictor_param, other->duration_predictor_param)) {
        return false;
    }

    if (!pv_orca_flow_param_is_equal(object->flow_param, other->flow_param)) {
        return false;
    }

    if (!pv_orca_vocoder_param_is_equal(object->vocoder_param, other->vocoder_param)) {
        return false;
    }

    return true;
}

struct pv_orca_synthesizer {
    const pv_orca_synthesizer_param_t *param;

    pv_orca_stream_state_t *stream_state;

    pv_orca_text_encoder_t *text_encoder;
    pv_orca_duration_predictor_t *duration_predictor;
    pv_orca_flow_t *flow;
    pv_orca_vocoder_t *vocoder;
};

pv_status_t PV_MOCKABLE(pv_orca_synthesizer_init)(
        pv_ypu_t *ypu,
        const pv_orca_synthesizer_param_t *param,
        pv_orca_stream_state_t *stream_state,
        pv_orca_synthesizer_t **object) {
    pv_error_prepare();
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(object);

    *object = NULL;

    pv_orca_synthesizer_t *o = pv_ypu_host_alloc(ypu, sizeof(pv_orca_synthesizer_t));
    if (!o) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(o, 0, sizeof(pv_orca_synthesizer_t));

    o->param = param;
    o->stream_state = stream_state;

    pv_status_t status = pv_orca_text_encoder_init(ypu, o->param->text_encoder_param, &(o->text_encoder));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_module_init_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE(
                        "text_encoder",
                        pv_status_to_string(status)));
        pv_orca_synthesizer_delete(ypu, o);
        return status;
    }

    status = pv_orca_duration_predictor_init(ypu, o->param->duration_predictor_param, &(o->duration_predictor));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_module_init_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE(
                        "duration_predictor",
                        pv_status_to_string(status)));
        pv_orca_synthesizer_delete(ypu, o);
        return status;
    }

    status = pv_orca_flow_init(ypu, o->param->flow_param, &(o->flow));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_module_init_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE(
                        "flow",
                        pv_status_to_string(status)));
        pv_orca_synthesizer_delete(ypu, o);
        return status;
    }

    status = pv_orca_vocoder_init(ypu, o->param->vocoder_param, &(o->vocoder));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_module_init_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE(
                        "vocoder",
                        pv_status_to_string(status)));
        pv_orca_synthesizer_delete(ypu, o);
        return status;
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_orca_synthesizer_delete)(pv_ypu_t *ypu, pv_orca_synthesizer_t *object) {
    PV_ASSERT(ypu);

    if (object) {
        pv_orca_vocoder_delete(ypu, object->vocoder);
        pv_orca_flow_delete(ypu, object->flow);
        pv_orca_duration_predictor_delete(ypu, object->duration_predictor);
        pv_orca_text_encoder_delete(ypu, object->text_encoder);

        pv_ypu_host_free(ypu, object);
    }
}

int32_t PV_MOCKABLE(pv_orca_synthesizer_sample_rate)(const pv_orca_synthesizer_t *object) {
    PV_ASSERT(object);

    return object->param->sample_rate;
}

pv_status_t PV_MOCKABLE(pv_orca_synthesizer_forward)(
        pv_ypu_t *ypu,
        pv_orca_synthesizer_t *object,
        const pv_orca_synthesize_params_t *synthesize_params,
        bool no_random_latents,
        int32_t num_encoded_phonemes,
        const int32_t *encoded_phonemes,
        int32_t **encoded_phonemes_durations,
        int32_t *num_samples,
        int16_t **pcm) {
    pv_error_prepare();

    PV_ASSERT(ypu);
    PV_ASSERT(object);
    PV_ASSERT(synthesize_params);
    PV_ASSERT(num_encoded_phonemes > 0);
    PV_ASSERT(encoded_phonemes);
    PV_ASSERT(encoded_phonemes_durations);
    PV_ASSERT(num_samples);
    PV_ASSERT(pcm);

    PV_ORCA_PROFILER_START("synthesizer_forward");

    *encoded_phonemes_durations = NULL;
    *num_samples = 0;
    *pcm = NULL;

    pv_orca_stream_state_t *state = object->stream_state;
    const int32_t buffer_encoded_channels = object->param->text_encoder_param->embed_param->output_channels;
    const int32_t text_encoder_output_channels = object->param->text_encoder_param->conv_post_param->output_channels / 2;

    pv_ypu_mem_t *buffer_encoded_tokens = pv_ypu_buffer_get(
            ypu,
            num_encoded_phonemes * buffer_encoded_channels * (int32_t) sizeof(float),
            false);
    if (!buffer_encoded_tokens) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("pv_ypu_buffer_get"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_op_memset_args_t args0 = {
            .output = buffer_encoded_tokens,
            .size_bytes = num_encoded_phonemes * buffer_encoded_channels * (int32_t) sizeof(float),
            .output_offset = 0,
    };

    pv_status_t status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_MEMSET,
            &args0);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_mem_t *buffer_means_enc = pv_ypu_buffer_get(
            ypu,
            num_encoded_phonemes * text_encoder_output_channels * (int32_t) sizeof(float),
            false);
    if (!buffer_means_enc) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("pv_ypu_buffer_get"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_op_memset_args_t args1 = {
            .output = buffer_means_enc,
            .size_bytes = num_encoded_phonemes * text_encoder_output_channels * (int32_t) sizeof(float),
            .output_offset = 0,
    };

    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_MEMSET,
            &args1);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_mem_t *buffer_logs_enc = pv_ypu_buffer_get(
            ypu,
            num_encoded_phonemes * text_encoder_output_channels * (int32_t) sizeof(float),
            false);
    if (!buffer_logs_enc) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("pv_ypu_buffer_get"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_op_memset_args_t args2 = {
            .output = buffer_logs_enc,
            .size_bytes = num_encoded_phonemes * text_encoder_output_channels * (int32_t) sizeof(float),
            .output_offset = 0,
    };

    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_MEMSET,
            &args2);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    ORCA_LOG_VERBOSE(
            "[SYNTHESIZE] Encoding `%d` tokens",
            num_encoded_phonemes);

    status = pv_orca_text_encoder_forward(
            ypu,
            object->text_encoder,
            num_encoded_phonemes,
            encoded_phonemes,
            buffer_encoded_tokens,
            buffer_means_enc,
            buffer_logs_enc,
            0,
            0,
            0);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_text_encoder_forward,
                pv_status_to_string(status));
        return status;
    }

    int32_t num_encoded_phonemes_to_dp = num_encoded_phonemes;
    int32_t num_encoded_phonemes_to_flow = num_encoded_phonemes;

    if (state->status == PV_ORCA_STREAM_STATUS_ACTIVE) {
        pv_orca_stream_state_update_encoder_state(
                state,
                num_encoded_phonemes,
                &num_encoded_phonemes_to_dp,
                &num_encoded_phonemes_to_flow);
    }

    int32_t *encoded_phonemes_durations_internal = pv_ypu_host_alloc(ypu, sizeof(int32_t) * num_encoded_phonemes_to_dp);
    if (!encoded_phonemes_durations_internal) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("pv_ypu_host_alloc"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    float speech_rate = 1.0f;
    status = pv_orca_synthesize_params_get_speech_rate(synthesize_params, &speech_rate);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_synthesize_params_get_speech_rate,
                pv_status_to_string(status));
        pv_ypu_host_free(ypu, encoded_phonemes_durations_internal);
        return status;
    }

    status = pv_orca_duration_predictor_forward(
            ypu,
            object->duration_predictor,
            speech_rate,
            num_encoded_phonemes_to_dp,
            buffer_encoded_tokens,
            encoded_phonemes_durations_internal,
            state->start_index_dp * buffer_encoded_channels * (int32_t) sizeof(float));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_duration_predictor_forward,
                pv_status_to_string(status));
        pv_ypu_host_free(ypu, encoded_phonemes_durations_internal);
        return status;
    }

    pv_ypu_buffer_release(ypu, buffer_encoded_tokens);

    int32_t num_frames = 0;
    for (int32_t i = state->durations_offset; i < num_encoded_phonemes_to_flow + state->durations_offset; i++) {
        num_frames += encoded_phonemes_durations_internal[i];
    }

    pv_ypu_mem_t *buffer_means = pv_ypu_buffer_get(
            ypu,
            num_frames * text_encoder_output_channels * (int32_t) sizeof(float),
            false);
    if (!buffer_means) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("pv_ypu_buffer_get"));
        pv_ypu_host_free(ypu, encoded_phonemes_durations_internal);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    pv_ypu_mem_t *buffer_logs = pv_ypu_buffer_get(
            ypu,
            num_frames * text_encoder_output_channels * (int32_t) sizeof(float),
            false);
    if (!buffer_logs) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("pv_ypu_buffer_get"));
        pv_ypu_host_free(ypu, encoded_phonemes_durations_internal);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    int32_t hidden_channels = pv_orca_text_encoder_output_channels(object->text_encoder);

    status = pv_orca_util_expand_tokens_to_frames(
            ypu,
            num_encoded_phonemes_to_flow,
            hidden_channels,
            encoded_phonemes_durations_internal + state->durations_offset, // truncated to num_encoded_phonemes_to_flow
            buffer_means_enc,
            buffer_logs_enc,
            buffer_means,
            buffer_logs,
            state->start_index_flow * text_encoder_output_channels * (int32_t) sizeof(float),
            state->start_index_flow * text_encoder_output_channels * (int32_t) sizeof(float),
            0,
            0);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_buffer_release(ypu, buffer_means_enc);
    pv_ypu_buffer_release(ypu, buffer_logs_enc);

    ORCA_LOG_VERBOSE(
            "[SYNTHESIZE] Synthesizing %d frames and %.3f seconds",
            num_frames,
            (float) num_frames * 256.f / 22050.f);

    pv_ypu_mem_t *buffer_z_prior = pv_ypu_buffer_get(
            ypu,
            num_frames * text_encoder_output_channels * (int32_t) sizeof(float),
            false);
    if (!buffer_z_prior) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("pv_ypu_buffer_get"));
        pv_ypu_host_free(ypu, encoded_phonemes_durations_internal);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_op_memset_args_t args3 = {
            .output = buffer_z_prior,
            .size_bytes = num_frames * text_encoder_output_channels * (int32_t) sizeof(float),
            .output_offset = 0,
    };

    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_MEMSET,
            &args3);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    int64_t random_state = 0;
    status = pv_orca_synthesize_params_get_random_state_valid(synthesize_params, &random_state);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_synthesize_params_get_random_state,
                pv_status_to_string(status));
        pv_ypu_host_free(ypu, encoded_phonemes_durations_internal);
        return status;
    }

    if (no_random_latents) {
        pv_ypu_op_memcpy_args_t args = {
                .output = buffer_z_prior,
                .input = buffer_means,
                .size_bytes = hidden_channels * num_frames * (int32_t) sizeof(float),
                .output_offset = 0,
                .input_offset = 0,
        };

        pv_status_t status = pv_ypu_operator_execute(
                ypu,
                PV_YPU_OPERATOR_MEMCPY,
                &args);
        if (status != PV_STATUS_SUCCESS) {
            return status;
        }
    } else {
        float noise_scale = PV_ORCA_SYNTHESIZER_DEFAULT_NOISE_SCALE;
        pv_status_t status = pv_orca_util_sample_latents(
                ypu,
                num_frames,
                hidden_channels,
                noise_scale,
                random_state,
                buffer_means,
                buffer_logs,
                buffer_z_prior);
        if (status != PV_STATUS_SUCCESS) {
            return status;
        }
    }

    pv_ypu_buffer_release(ypu, buffer_means);
    pv_ypu_buffer_release(ypu, buffer_logs);

    int32_t num_frames_to_flow = num_frames;

    if (state->status == PV_ORCA_STREAM_STATUS_ACTIVE) {
        status = pv_orca_stream_state_update_z_prior(
                ypu,
                state,
                &buffer_z_prior,
                &num_frames_to_flow);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_orca_stream_state_update_z_prior,
                    pv_status_to_string(status));
            pv_ypu_host_free(ypu, encoded_phonemes_durations_internal);
            return status;
        }
    }

    pv_ypu_mem_t *buffer_z = pv_ypu_buffer_get(
            ypu,
            num_frames_to_flow * text_encoder_output_channels * (int32_t) sizeof(float),
            false);
    if (!buffer_z) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("pv_ypu_buffer_get"));
        pv_ypu_host_free(ypu, encoded_phonemes_durations_internal);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_op_memset_args_t args = {
            .output = buffer_z,
            .size_bytes = num_frames_to_flow * text_encoder_output_channels * (int32_t) sizeof(float),
            .output_offset = 0,
    };

    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_MEMSET,
            &args);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_orca_flow_forward(
            ypu,
            object->flow,
            num_frames_to_flow,
            buffer_z_prior,
            buffer_z,
            0,
            0);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_flow_forward,
                pv_status_to_string(status));
        pv_ypu_host_free(ypu, encoded_phonemes_durations_internal);
        return status;
    }

    pv_ypu_buffer_release(ypu, buffer_z_prior);

    int32_t num_frames_to_voc = num_frames_to_flow;

    if (state->status == PV_ORCA_STREAM_STATUS_ACTIVE) {
        status = pv_orca_stream_state_update_z(
                ypu,
                state,
                num_frames_to_flow,
                &buffer_z,
                &num_frames_to_voc);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_orca_stream_state_update_z,
                    pv_status_to_string(status));
            pv_ypu_host_free(ypu, encoded_phonemes_durations_internal);
            return status;
        }
    }

    int32_t frame_sample = PV_ORCA_WINDOW_SHIFT;
    int32_t num_samples_internal = frame_sample * num_frames_to_voc;
    int16_t *pcm_internal = malloc(num_samples_internal * (int32_t) sizeof(int16_t));
    if (!pcm_internal) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("pcm_internal"));
        pv_ypu_host_free(ypu, encoded_phonemes_durations_internal);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    status = pv_orca_vocoder_forward(ypu, object->vocoder, num_frames_to_voc, buffer_z, pcm_internal, 0);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_duration_predictor_forward,
                pv_status_to_string(status));
        pv_ypu_host_free(ypu, encoded_phonemes_durations_internal);
        free(pcm_internal);
        return status;
    }

    pv_ypu_buffer_release(ypu, buffer_z);

    int32_t num_samples_chunk = 0;
    int32_t pcm_offset = 0;

    if (state->status == PV_ORCA_STREAM_STATUS_ACTIVE) {
        pv_orca_stream_state_update_pcm_chunk(
                state,
                PV_ORCA_WINDOW_SHIFT,
                num_samples_internal,
                &num_samples_chunk,
                &pcm_offset);
    } else {
        num_samples_chunk = num_samples_internal;
    }

    if (state->status == PV_ORCA_STREAM_STATUS_ACTIVE) {
        state->is_first_chunk = false;
    }

    if (num_samples_chunk != num_samples_internal) {
        int16_t *pcm_truncated = malloc(num_samples_chunk * (int32_t) sizeof(int16_t));
        if (!pcm_truncated) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("pcm_truncated"));
            pv_ypu_host_free(ypu, encoded_phonemes_durations_internal);
            pv_ypu_host_free(ypu, pcm_internal);
            return PV_STATUS_OUT_OF_MEMORY;
        }
        memcpy(pcm_truncated, pcm_internal + pcm_offset, num_samples_chunk * (int32_t) sizeof(int16_t));
        free(pcm_internal);
        *pcm = pcm_truncated;
    } else {
        *pcm = pcm_internal;
    }

    *num_samples = num_samples_chunk;
    *encoded_phonemes_durations = encoded_phonemes_durations_internal;

    PV_ORCA_PROFILER_STOP("synthesizer_forward");
    PV_ORCA_PROFILER_PRINT_DATA;

    return PV_STATUS_SUCCESS;
}
