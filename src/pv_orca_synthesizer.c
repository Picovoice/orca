#include <string.h>

#include "core/pv_error_messages.h"
#include "core/pv_type.h"
#include "io/pv_log.h"
#include "orca/pv_orca_duration_predictor.h"
#include "orca/pv_orca_gaussian_upsampler.h"
#include "orca/pv_orca_internal.h"
#include "orca/pv_orca_lfm_condition_fuser.h"
#include "orca/pv_orca_lfm_film_generator.h"
#include "orca/pv_orca_lfm_vf_estimator.h"
#include "orca/pv_orca_lfm_vf_estimator_param.h"
#include "orca/pv_orca_prior_encoder_film_generator.h"
#include "orca/pv_orca_prior_encoder_flow.h"
#include "orca/pv_orca_stream_state.h"
#include "orca/pv_orca_synthesizer.h"
#include "orca/pv_orca_util.h"
#include "orca/pv_orca_vocoder.h"
#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

static const float PV_ORCA_SYNTHESIZER_DEFAULT_TEMPERATURE = 0.0f;
static const int32_t NFE = 10;
static const float DT = 0.1f;

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

    count = fwrite(&(param->lfm_time_embedding_dim), sizeof(int32_t), 1, f);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

    pv_status_t status = pv_orca_prior_encoder_film_generator_param_serialize(
            ypu,
            param->prior_encoder_film_generator_param,
            f);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_prior_encoder_film_generator_param_serialize,
                pv_status_to_string(status));
        return status;
    }

    status = pv_orca_prior_encoder_flow_param_serialize(
            ypu,
            param->prior_encoder_flow_param,
            f);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_prior_encoder_flow_param_serialize,
                pv_status_to_string(status));
        return status;
    }

    status = pv_orca_duration_predictor_param_serialize(
            ypu,
            param->duration_predictor_param,
            f);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_duration_predictor_param_serialize,
                pv_status_to_string(status));
        return status;
    }

    status = pv_orca_gaussian_upsampler_param_serialize(
            ypu,
            param->gaussian_upsampler_param,
            f);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_gaussian_upsampler_param_serialize,
                pv_status_to_string(status));
        return status;
    }

    status = pv_orca_lfm_film_generator_param_serialize(
            ypu,
            param->lfm_film_generator_param,
            f);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_lfm_film_generator_param_serialize,
                pv_status_to_string(status));
        return status;
    }

    count = fwrite(
            param->lfm_time_embedding_param->data,
            sizeof(float),
            NFE * param->lfm_time_embedding_dim,
            f);
    if (count != (size_t) NFE * param->lfm_time_embedding_dim) {
        return PV_STATUS_IO_ERROR;
    }

    status = pv_orca_lfm_condition_fuser_param_serialize(
            ypu,
            param->lfm_condition_fuser_param,
            f);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_lfm_condition_fuser_param_serialize,
                pv_status_to_string(status));
        return status;
    }

    status = pv_orca_lfm_vf_estimator_param_serialize(
            ypu,
            param->lfm_vf_estimator_param,
            f);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_lfm_vf_estimator_param_serialize,
                pv_status_to_string(status));
        return status;
    }

    status = pv_orca_vocoder_param_serialize(
            ypu,
            param->vocoder_param,
            f);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_vocoder_param_serialize,
                pv_status_to_string(status));
        return status;
    }

    count = fwrite(&(param->N_domain_lookahead), sizeof(int32_t), 1, f);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

    count = fwrite(&(param->N_domain_lookback), sizeof(int32_t), 1, f);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

    count = fwrite(&(param->lfm_film_generator_lookahead), sizeof(int32_t), 1, f);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

    count = fwrite(&(param->lfm_film_generator_lookback), sizeof(int32_t), 1, f);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

    count = fwrite(&(param->lfm_vf_estimator_lookahead), sizeof(int32_t), 1, f);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

    count = fwrite(&(param->lfm_vf_estimator_lookback), sizeof(int32_t), 1, f);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

    count = fwrite(&(param->vocoder_lookahead), sizeof(int32_t), 1, f);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

    count = fwrite(&(param->vocoder_lookback), sizeof(int32_t), 1, f);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

    return PV_STATUS_SUCCESS;
}

#endif

pv_status_t PV_MOCKABLE(pv_orca_synthesizer_param_load)(
        pv_ypu_t *ypu,
        FILE *f,
        const char *version,
        pv_orca_synthesizer_param_t **param) {
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_orca_synthesizer_param_t *p = pv_ypu_host_alloc(ypu, sizeof(pv_orca_synthesizer_param_t));
    if (!p) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_host_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("pv_orca_synthesizer_param_t"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(p, 0, sizeof(pv_orca_synthesizer_param_t));

    const size_t version_length = strlen(version);
    p->version = pv_ypu_host_alloc(ypu, (int32_t) ((version_length + 1) * sizeof(char)));
    if (!(p->version)) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_host_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p->version"));
        return PV_STATUS_OUT_OF_MEMORY;
    }
    memcpy((char *) (p->version), version, version_length + 1);

    size_t count = pv_fread((int32_t *) &(p->sample_rate), sizeof(int32_t), 1, f);
    if (count != 1) {
        PV_ERROR_REPORT(
                &pv_error_msg_fread_param_failure,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p->sample_rate", f));
        pv_orca_synthesizer_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }

    count = pv_fread((int32_t *) &(p->lfm_time_embedding_dim), sizeof(int32_t), 1, f);
    if (count != 1) {
        PV_ERROR_REPORT(
                &pv_error_msg_fread_param_failure,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p->lfm_time_embedding_dim", f));
        pv_orca_synthesizer_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }

    pv_status_t status = pv_orca_prior_encoder_film_generator_param_load(
            ypu,
            f,
            (pv_orca_prior_encoder_film_generator_param_t **) &(p->prior_encoder_film_generator_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_prior_encoder_film_generator_param_load,
                pv_status_to_string(status));
        pv_orca_synthesizer_param_delete(ypu, p);
        return status;
    }

    status = pv_orca_prior_encoder_flow_param_load(
            ypu,
            f,
            (pv_orca_prior_encoder_flow_param_t **) &(p->prior_encoder_flow_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_prior_encoder_flow_param_load,
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

    status = pv_orca_gaussian_upsampler_param_load(
            ypu,
            f,
            (pv_orca_gaussian_upsampler_param_t **) &(p->gaussian_upsampler_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_gaussian_upsampler_param_load,
                pv_status_to_string(status));
        pv_orca_synthesizer_param_delete(ypu, p);
        return status;
    }

    status = pv_orca_lfm_film_generator_param_load(
            ypu,
            f,
            (pv_orca_lfm_film_generator_param_t **) &(p->lfm_film_generator_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_lfm_film_generator_param_load,
                pv_status_to_string(status));
        pv_orca_synthesizer_param_delete(ypu, p);
        return status;
    }

    p->lfm_time_embedding_param = pv_ypu_config_mem_alloc(
            ypu,
            ((int32_t) sizeof(float)) * NFE * p->lfm_time_embedding_dim,
            PV_YPU_DEVICE_MEM_FLAG_STATIC);
    if (!(p->lfm_time_embedding_param)) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p->lfm_time_embedding_param"));
        pv_orca_synthesizer_param_delete(ypu, p);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    count = pv_fread((float *) p->lfm_time_embedding_param->data, sizeof(float), NFE * p->lfm_time_embedding_dim, f);
    if (count != (size_t) NFE * p->lfm_time_embedding_dim) {
        PV_ERROR_REPORT(
                &pv_error_msg_fread_param_failure,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p->lfm_time_embedding_param", f));
        pv_orca_synthesizer_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }

    status = pv_orca_lfm_condition_fuser_param_load(
            ypu,
            f,
            (pv_orca_lfm_condition_fuser_param_t **) &(p->lfm_condition_fuser_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_lfm_condition_fuser_param_load,
                pv_status_to_string(status));
        pv_orca_synthesizer_param_delete(ypu, p);
        return status;
    }

    status = pv_orca_lfm_vf_estimator_param_load(
            ypu,
            f,
            (pv_orca_lfm_vf_estimator_param_t **) &(p->lfm_vf_estimator_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_lfm_vf_estimator_param_load,
                pv_status_to_string(status));
        pv_orca_synthesizer_param_delete(ypu, p);
        return status;
    }

    status = pv_orca_vocoder_param_load(
            ypu,
            f,
            (pv_orca_vocoder_param_t **) &(p->vocoder_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_vocoder_param_load,
                pv_status_to_string(status));
        pv_orca_synthesizer_param_delete(ypu, p);
        return status;
    }

    count = pv_fread((int32_t *) &(p->N_domain_lookahead), sizeof(int32_t), 1, f);
    if (count != 1) {
        PV_ERROR_REPORT(
                &pv_error_msg_fread_param_failure,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p->N_domain_lookahead", f));
        pv_orca_synthesizer_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }

    count = pv_fread((int32_t *) &(p->N_domain_lookback), sizeof(int32_t), 1, f);
    if (count != 1) {
        PV_ERROR_REPORT(
                &pv_error_msg_fread_param_failure,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p->N_domain_lookback", f));
        pv_orca_synthesizer_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }

    count = pv_fread((int32_t *) &(p->lfm_film_generator_lookahead), sizeof(int32_t), 1, f);
    if (count != 1) {
        PV_ERROR_REPORT(
                &pv_error_msg_fread_param_failure,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p->lfm_film_generator_lookahead", f));
        pv_orca_synthesizer_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }

    count = pv_fread((int32_t *) &(p->lfm_film_generator_lookback), sizeof(int32_t), 1, f);
    if (count != 1) {
        PV_ERROR_REPORT(
                &pv_error_msg_fread_param_failure,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p->lfm_film_generator_lookback", f));
        pv_orca_synthesizer_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }

    count = pv_fread((int32_t *) &(p->lfm_vf_estimator_lookahead), sizeof(int32_t), 1, f);
    if (count != 1) {
        PV_ERROR_REPORT(
                &pv_error_msg_fread_param_failure,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p->lfm_vf_estimator_lookahead", f));
        pv_orca_synthesizer_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }

    count = pv_fread((int32_t *) &(p->lfm_vf_estimator_lookback), sizeof(int32_t), 1, f);
    if (count != 1) {
        PV_ERROR_REPORT(
                &pv_error_msg_fread_param_failure,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p->lfm_vf_estimator_lookback", f));
        pv_orca_synthesizer_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }

    count = pv_fread((int32_t *) &(p->vocoder_lookahead), sizeof(int32_t), 1, f);
    if (count != 1) {
        PV_ERROR_REPORT(
                &pv_error_msg_fread_param_failure,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p->vocoder_lookahead", f));
        pv_orca_synthesizer_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }

    count = pv_fread((int32_t *) &(p->vocoder_lookback), sizeof(int32_t), 1, f);
    if (count != 1) {
        PV_ERROR_REPORT(
                &pv_error_msg_fread_param_failure,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p->vocoder_lookback", f));
        pv_orca_synthesizer_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_orca_synthesizer_param_delete)(pv_ypu_t *ypu, pv_orca_synthesizer_param_t *param) {
    PV_ASSERT(ypu);

    if (param) {
        pv_orca_vocoder_param_delete(ypu, (pv_orca_vocoder_param_t *) (param->vocoder_param));

        pv_orca_lfm_vf_estimator_param_delete(ypu, (pv_orca_lfm_vf_estimator_param_t *) (param->lfm_vf_estimator_param));
        pv_orca_lfm_condition_fuser_param_delete(ypu, (pv_orca_lfm_condition_fuser_param_t *) (param->lfm_condition_fuser_param));

        pv_ypu_config_mem_free(ypu, param->lfm_time_embedding_param);

        pv_orca_lfm_film_generator_param_delete(ypu, (pv_orca_lfm_film_generator_param_t *) (param->lfm_film_generator_param));
        pv_orca_duration_predictor_param_delete(ypu, (pv_orca_duration_predictor_param_t *) (param->duration_predictor_param));
        pv_orca_gaussian_upsampler_param_delete(ypu, (pv_orca_gaussian_upsampler_param_t *) (param->gaussian_upsampler_param));
        pv_orca_prior_encoder_flow_param_delete(ypu, (pv_orca_prior_encoder_flow_param_t *) (param->prior_encoder_flow_param));
        pv_orca_prior_encoder_film_generator_param_delete(ypu, (pv_orca_prior_encoder_film_generator_param_t *) (param->prior_encoder_film_generator_param));

        pv_ypu_host_free(ypu, (char *) (param->version));

        pv_ypu_host_free(ypu, param);
    }
}

bool PV_MOCKABLE(pv_orca_synthesizer_param_is_equal)(
        const pv_orca_synthesizer_param_t *object,
        const pv_orca_synthesizer_param_t *other) {
    PV_ASSERT(object);
    PV_ASSERT(other);

    if (!pv_orca_prior_encoder_film_generator_param_is_equal(object->prior_encoder_film_generator_param, other->prior_encoder_film_generator_param)) {
        return false;
    }

    if (!pv_orca_prior_encoder_flow_param_is_equal(object->prior_encoder_flow_param, other->prior_encoder_flow_param)) {
        return false;
    }

    if (!pv_orca_gaussian_upsampler_param_is_equal(object->gaussian_upsampler_param, other->gaussian_upsampler_param)) {
        return false;
    }

    if (!pv_orca_duration_predictor_param_is_equal(object->duration_predictor_param, other->duration_predictor_param)) {
        return false;
    }

    if (!pv_orca_lfm_film_generator_param_is_equal(object->lfm_film_generator_param, other->lfm_film_generator_param)) {
        return false;
    }

    if (object->lfm_time_embedding_dim != other->lfm_time_embedding_dim) {
        return false;
    }

    if (!pv_ypu_config_mem_is_equal(object->lfm_time_embedding_param, other->lfm_time_embedding_param)) {
        return false;
    }

    if (object->N_domain_lookahead != other->N_domain_lookahead) {
        return false;
    }

    if (object->N_domain_lookback != other->N_domain_lookback) {
        return false;
    }

    if (object->lfm_film_generator_lookahead != other->lfm_film_generator_lookahead) {
        return false;
    }

    if (object->lfm_film_generator_lookback != other->lfm_film_generator_lookback) {
        return false;
    }

    if (object->lfm_vf_estimator_lookahead != other->lfm_vf_estimator_lookahead) {
        return false;
    }

    if (object->lfm_vf_estimator_lookback != other->lfm_vf_estimator_lookback) {
        return false;
    }

    if (object->vocoder_lookahead != other->vocoder_lookahead) {
        return false;
    }

    if (object->vocoder_lookback != other->vocoder_lookback) {
        return false;
    }

    if (!pv_orca_lfm_condition_fuser_param_is_equal(object->lfm_condition_fuser_param, other->lfm_condition_fuser_param)) {
        return false;
    }

    if (!pv_orca_lfm_vf_estimator_param_is_equal(object->lfm_vf_estimator_param, other->lfm_vf_estimator_param)) {
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

    pv_orca_prior_encoder_film_generator_t *prior_encoder_film_generator;
    pv_orca_prior_encoder_flow_t *prior_encoder_flow;
    pv_orca_gaussian_upsampler_t *gaussian_upsampler;
    pv_orca_duration_predictor_t *duration_predictor;
    pv_orca_lfm_film_generator_t *lfm_film_generator;
    pv_orca_lfm_condition_fuser_t *lfm_condition_fuser;
    pv_orca_lfm_vf_estimator_t *lfm_vf_estimator;
    pv_orca_vocoder_t *vocoder;

    pv_ypu_mem_t *lfm_time_embedding;
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
                &pv_error_msg_ypu_host_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(o, 0, sizeof(pv_orca_synthesizer_t));

    o->param = param;
    o->stream_state = stream_state;

    o->lfm_time_embedding = pv_ypu_mem_from_config(ypu, param->lfm_time_embedding_param);
    if (!o->lfm_time_embedding) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o->lfm_time_embedding"));
        pv_orca_synthesizer_delete(ypu, o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_status_t status = pv_orca_prior_encoder_film_generator_init(
            ypu,
            o->param->prior_encoder_film_generator_param,
            &(o->prior_encoder_film_generator));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_module_init_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE(
                        "prior_encoder_film_generator",
                        pv_status_to_string(status)));
        pv_orca_synthesizer_delete(ypu, o);
        return status;
    }

    status = pv_orca_prior_encoder_flow_init(
            ypu,
            o->param->prior_encoder_flow_param,
            &(o->prior_encoder_flow));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_module_init_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE(
                        "prior_encoder_flow",
                        pv_status_to_string(status)));
        pv_orca_synthesizer_delete(ypu, o);
        return status;
    }

    status = pv_orca_gaussian_upsampler_init(
            ypu,
            o->param->gaussian_upsampler_param,
            &(o->gaussian_upsampler));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_module_init_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE(
                        "gaussian_upsampler",
                        pv_status_to_string(status)));
        pv_orca_synthesizer_delete(ypu, o);
        return status;
    }

    status = pv_orca_duration_predictor_init(
            ypu,
            o->param->duration_predictor_param,
            &(o->duration_predictor));
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

    status = pv_orca_lfm_film_generator_init(
            ypu,
            o->param->lfm_film_generator_param,
            &(o->lfm_film_generator));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_module_init_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE(
                        "lfm_film_generator",
                        pv_status_to_string(status)));
        pv_orca_synthesizer_delete(ypu, o);
        return status;
    }

    status = pv_orca_lfm_condition_fuser_init(
            ypu,
            o->param->lfm_condition_fuser_param,
            &(o->lfm_condition_fuser));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_module_init_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE(
                        "lfm_condition_fuser",
                        pv_status_to_string(status)));
        pv_orca_synthesizer_delete(ypu, o);
        return status;
    }

    status = pv_orca_lfm_vf_estimator_init(
            ypu,
            o->param->lfm_vf_estimator_param,
            &(o->lfm_vf_estimator));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_module_init_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE(
                        "lfm_vf_estimator",
                        pv_status_to_string(status)));
        pv_orca_synthesizer_delete(ypu, o);
        return status;
    }

    status = pv_orca_vocoder_init(
            ypu,
            o->param->vocoder_param,
            &(o->vocoder));
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
        pv_orca_lfm_vf_estimator_delete(ypu, object->lfm_vf_estimator);
        pv_orca_lfm_condition_fuser_delete(ypu, object->lfm_condition_fuser);
        pv_orca_lfm_film_generator_delete(ypu, object->lfm_film_generator);
        pv_orca_duration_predictor_delete(ypu, object->duration_predictor);
        pv_orca_gaussian_upsampler_delete(ypu, object->gaussian_upsampler);
        pv_orca_prior_encoder_flow_delete(ypu, object->prior_encoder_flow);
        pv_orca_prior_encoder_film_generator_delete(ypu, object->prior_encoder_film_generator);

        pv_ypu_mem_free(ypu, object->lfm_time_embedding);

        pv_ypu_host_free(ypu, object);
    }
}

pv_status_t PV_MOCKABLE(pv_orca_synthesizer_reset_cache)(
        pv_ypu_t *ypu,
        pv_orca_synthesizer_t *object) {
    PV_ASSERT(ypu);
    PV_ASSERT(object);

    return pv_orca_vocoder_reset_cache(ypu, object->vocoder);
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
    PV_ASSERT(num_encoded_phonemes >= 0);
    if (num_encoded_phonemes > 0) {
        PV_ASSERT(encoded_phonemes);
    } else {
        PV_ASSERT(encoded_phonemes == NULL);
    }
    PV_ASSERT(encoded_phonemes_durations);
    PV_ASSERT(num_samples);
    PV_ASSERT(pcm);

    (void) no_random_latents;

    *encoded_phonemes_durations = NULL;
    *num_samples = 0;
    *pcm = NULL;

    int64_t random_state = 0;
    pv_status_t status = pv_orca_synthesize_params_get_random_state_valid(synthesize_params, &random_state);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_synthesize_params_get_random_state_valid,
                pv_status_to_string(status));
        return status;
    }

    pv_orca_stream_state_t *state = object->stream_state;
    PV_ASSERT(num_encoded_phonemes > 0 || object->stream_state->is_flush);

    int32_t num_to_N_domain = num_encoded_phonemes;

    const int32_t *buffer_N_domain_ptr = encoded_phonemes;
    if (state->status == PV_ORCA_STREAM_STATUS_ACTIVE) {
        bool is_sufficient_context_N_domain = false;
        status = pv_orca_stream_state_is_sufficient_context_n_domain(
                state,
                num_encoded_phonemes,
                encoded_phonemes,
                &is_sufficient_context_N_domain);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_orca_stream_state_is_sufficient_context_n_domain,
                    pv_status_to_string(status));
            return status;
        }
        if (state->status == PV_ORCA_STREAM_STATUS_ACTIVE) {
            state->is_first_chunk = false;
        }
        if (!is_sufficient_context_N_domain && (state->status == PV_ORCA_STREAM_STATUS_ACTIVE) && !(state->is_flush)) {
            ORCA_LOG_VERBOSE_SIMPLE("[RETURN] Not enough N-domain context to synthesize audio");
            return PV_STATUS_SUCCESS;
        }

        status = pv_orca_stream_state_update_n_domain(
                state,
                &num_to_N_domain);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_orca_stream_state_update_n_domain,
                    pv_status_to_string(status));
            return status;
        }

        PV_ASSERT(num_to_N_domain >= 0);
        if (num_to_N_domain > 0) {
            buffer_N_domain_ptr = pv_buffer_get(
                    state->buffer_N_domain_concat,
                    num_to_N_domain * ((int32_t) sizeof(int32_t)),
                    false);
            if (!buffer_N_domain_ptr) {
                PV_ERROR_REPORT(
                        &pv_error_msg_alloc,
                        PV_ERROR_ARGS_PUBLIC_EMPTY(),
                        PV_ERROR_ARGS_PRIVATE("buffer_N_domain_float"));
                return PV_STATUS_OUT_OF_MEMORY;
            }
        }
    }

    const int32_t N = num_to_N_domain;

    PV_ASSERT(N >= 0);

    int32_t *buffer_duration = NULL;
    pv_ypu_mem_t *buffer_gaussian_upsampled_ypu = NULL;
    pv_ypu_mem_t *buffer_bucket_non_streaming_ypu = NULL;
    pv_ypu_mem_t *buffer_lfm_x_t_raw_ypu = NULL;

    int32_t T = 0;
    int32_t N_to_T_garbage_lookahead_offset = 0;
    int32_t N_to_T_garbage_lookback_offset = 0;

    if (N > 0) {
        pv_ypu_mem_t *buffer_z_prior_ypu = pv_ypu_buffer_get(
                ypu,
                N * object->param->prior_encoder_flow_param->dimension * ((int32_t) sizeof(float)),
                false);
        if (!buffer_z_prior_ypu) {
            PV_ERROR_REPORT(
                    &pv_error_msg_ypu_device_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("buffer_z_prior"));
            return PV_STATUS_OUT_OF_MEMORY;
        }

        float temperature = PV_ORCA_SYNTHESIZER_DEFAULT_TEMPERATURE;
        (void) temperature;
        PV_ASSERT(temperature == 0.0f);

        status = pv_orca_util_sample_standard_gaussian_with_temperature(
                ypu,
                N,
                object->param->prior_encoder_flow_param->dimension,
                no_random_latents ? 0.0f : temperature,
                random_state,
                buffer_z_prior_ypu);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_orca_util_sample_standard_gaussian_with_temperature,
                    pv_status_to_string(status));
            pv_ypu_buffer_release(ypu, buffer_z_prior_ypu);
            return status;
        }

        ORCA_LOG_VERBOSE(
                "[SYNTHESIZE] Encoding `%d` tokens",
                N);

        pv_ypu_mem_t *buffer_cond_ypu = pv_ypu_buffer_get(
                ypu,
                N * object->param->prior_encoder_film_generator_param->dimension * (int32_t) sizeof(float),
                false);
        if (!buffer_cond_ypu) {
            PV_ERROR_REPORT(
                    &pv_error_msg_ypu_device_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("buffer_cond"));
            pv_ypu_buffer_release(ypu, buffer_z_prior_ypu);
            return PV_STATUS_OUT_OF_MEMORY;
        }

        status = pv_orca_prior_encoder_film_generator_forward(
                ypu,
                object->prior_encoder_film_generator,
                N,
                buffer_N_domain_ptr,
                buffer_cond_ypu);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_orca_prior_encoder_film_generator_forward,
                    pv_status_to_string(status));
            pv_ypu_buffer_release(ypu, buffer_cond_ypu);
            pv_ypu_buffer_release(ypu, buffer_z_prior_ypu);
            return status;
        }

        status = pv_orca_prior_encoder_flow_forward(
                ypu,
                object->prior_encoder_flow,
                N,
                buffer_cond_ypu,
                buffer_z_prior_ypu,
                buffer_z_prior_ypu);
        pv_ypu_buffer_release(ypu, buffer_cond_ypu);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_orca_prior_encoder_flow_forward,
                    pv_status_to_string(status));
            pv_ypu_buffer_release(ypu, buffer_z_prior_ypu);
            return status;
        }

        float speech_rate = 1.0f;
        status = pv_orca_synthesize_params_get_speech_rate(synthesize_params, &speech_rate);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_orca_synthesize_params_get_speech_rate,
                    pv_status_to_string(status));
            pv_ypu_buffer_release(ypu, buffer_z_prior_ypu);
            return status;
        }

        buffer_duration = pv_ypu_host_alloc(
                ypu,
                N * (int32_t) sizeof(int32_t));
        if (!buffer_duration) {
            PV_ERROR_REPORT(
                    &pv_error_msg_ypu_host_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("buffer_duration"));
            pv_ypu_buffer_release(ypu, buffer_z_prior_ypu);
            return PV_STATUS_OUT_OF_MEMORY;
        }

        pv_ypu_mem_t *buffer_std_ypu = pv_ypu_buffer_get(
                ypu,
                N * (int32_t) sizeof(float),
                false);
        if (!buffer_std_ypu) {
            PV_ERROR_REPORT(
                    &pv_error_msg_ypu_device_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("buffer_std"));
            pv_ypu_host_free(ypu, buffer_duration);
            pv_ypu_buffer_release(ypu, buffer_z_prior_ypu);
            return PV_STATUS_OUT_OF_MEMORY;
        }

        status = pv_orca_duration_predictor_forward(
                ypu,
                object->duration_predictor,
                N,
                speech_rate,
                buffer_z_prior_ypu,
                buffer_duration,
                buffer_std_ypu);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_orca_duration_predictor_forward,
                    pv_status_to_string(status));
            pv_ypu_buffer_release(ypu, buffer_std_ypu);
            pv_ypu_buffer_release(ypu, buffer_z_prior_ypu);
            return status;
        }

        for (int32_t i = 0; i < N; i++) {
            T += buffer_duration[i];
        }
        const int32_t sdpa_downsample_factor = 1;
        PV_ASSERT(sdpa_downsample_factor == 1);
        const int32_t T_pre_rounded = T;
        T = ((T + (sdpa_downsample_factor - 1)) / sdpa_downsample_factor) * sdpa_downsample_factor;
        PV_ASSERT((T - T_pre_rounded) < sdpa_downsample_factor);
        buffer_duration[N - 1] += (T - T_pre_rounded);

        // First compute N->T domain conversion garbage to be thrown out on both sides:
        for (int32_t i = 0; i < state->N_domain_garbage_lookahead_offset; i++) {
            PV_ASSERT(N - 1 - i >= 0);
            N_to_T_garbage_lookahead_offset += buffer_duration[N - 1 - i];
        }
        for (int32_t i = 0; i < state->N_domain_garbage_lookback_offset; i++) {
            PV_ASSERT(i <= N - 1);
            N_to_T_garbage_lookback_offset += buffer_duration[i];
        }

        ORCA_LOG_VERBOSE(
                "[SYNTHESIZE] Synthesizing %d frames and %.3f seconds",
                T,
                (float) T * 256.f / 22050.f);

        buffer_gaussian_upsampled_ypu = pv_ypu_mem_alloc(
                ypu,
                T * object->param->gaussian_upsampler_param->dimension * (int32_t) sizeof(float),
                PV_YPU_DEVICE_MEM_FLAG_NONE);
        if (!buffer_gaussian_upsampled_ypu) {
            PV_ERROR_REPORT(
                    &pv_error_msg_ypu_device_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("buffer_gaussian_upsampled"));
            pv_ypu_host_free(ypu, buffer_duration);
            pv_ypu_buffer_release(ypu, buffer_std_ypu);
            pv_ypu_buffer_release(ypu, buffer_z_prior_ypu);
            return PV_STATUS_OUT_OF_MEMORY;
        }

        status = pv_orca_gaussian_upsampler_forward(
                ypu,
                object->gaussian_upsampler,
                N,
                T,
                buffer_z_prior_ypu,
                buffer_duration,
                buffer_std_ypu,
                buffer_gaussian_upsampled_ypu);
        pv_ypu_buffer_release(ypu, buffer_z_prior_ypu);
        pv_ypu_buffer_release(ypu, buffer_std_ypu);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_orca_gaussian_upsampler_forward,
                    pv_status_to_string(status));
            pv_ypu_mem_free(ypu, buffer_gaussian_upsampled_ypu);
            pv_ypu_host_free(ypu, buffer_duration);
            return status;
        }

        buffer_bucket_non_streaming_ypu = pv_ypu_mem_alloc(
                ypu,
                T * (int32_t) sizeof(int32_t),
                PV_YPU_DEVICE_MEM_FLAG_NONE);
        if (!buffer_bucket_non_streaming_ypu) {
            PV_ERROR_REPORT(
                    &pv_error_msg_ypu_device_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("buffer_bucket_non_streaming"));
            pv_ypu_mem_free(ypu, buffer_gaussian_upsampled_ypu);
            pv_ypu_host_free(ypu, buffer_duration);
            return PV_STATUS_OUT_OF_MEMORY;
        }

        const int32_t bucket_offset =
                state->status == PV_ORCA_STREAM_STATUS_ACTIVE
                        ? state->bucket_offset - state->N_domain_garbage_lookback_offset
                        : 0;
        pv_orca_util_generate_bucket(
                ypu,
                N,
                bucket_offset,
                buffer_duration,
                buffer_bucket_non_streaming_ypu);

        if (state->status == PV_ORCA_STREAM_STATUS_ACTIVE) {
            PV_ASSERT(N - state->N_domain_garbage_lookback_offset - state->N_domain_garbage_lookahead_offset >= 0);
            state->bucket_offset += (N - state->N_domain_garbage_lookback_offset - state->N_domain_garbage_lookahead_offset);
        }

        buffer_lfm_x_t_raw_ypu = pv_ypu_mem_alloc(
                ypu,
                T * object->param->lfm_vf_estimator_param->out_dimension * ((int32_t) sizeof(float)),
                PV_YPU_DEVICE_MEM_FLAG_NONE);
        if (!buffer_lfm_x_t_raw_ypu) {
            PV_ERROR_REPORT(
                    &pv_error_msg_ypu_device_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("buffer_lfm_x_t_raw"));
            pv_ypu_mem_free(ypu, buffer_bucket_non_streaming_ypu);
            pv_ypu_mem_free(ypu, buffer_gaussian_upsampled_ypu);
            pv_ypu_host_free(ypu, buffer_duration);
            return PV_STATUS_OUT_OF_MEMORY;
        }

        status = pv_orca_util_sample_standard_gaussian_with_temperature(
                ypu,
                T,
                object->param->lfm_vf_estimator_param->out_dimension,
                no_random_latents ? 0.0f : 1.0f,
                random_state,
                buffer_lfm_x_t_raw_ypu);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_orca_util_sample_standard_gaussian_with_temperature,
                    pv_status_to_string(status));
            pv_ypu_mem_free(ypu, buffer_bucket_non_streaming_ypu);
            pv_ypu_mem_free(ypu, buffer_gaussian_upsampled_ypu);
            pv_ypu_host_free(ypu, buffer_duration);
            return status;
        }
    }

    int32_t num_to_T_domain = T;
    pv_ypu_mem_t *buffer_T_domain_ypu = buffer_gaussian_upsampled_ypu;
    pv_ypu_mem_t *buffer_bucket_ypu = buffer_bucket_non_streaming_ypu;
    pv_ypu_mem_t *buffer_lfm_x_t_ypu = buffer_lfm_x_t_raw_ypu;

    if (state->status == PV_ORCA_STREAM_STATUS_ACTIVE) {
        bool is_sufficient_context_T_domain = false;
        status = pv_orca_stream_state_is_sufficient_context_t_domain(
                ypu,
                state,
                T,
                N_to_T_garbage_lookback_offset,
                N_to_T_garbage_lookahead_offset,
                buffer_gaussian_upsampled_ypu,
                buffer_bucket_non_streaming_ypu,
                buffer_lfm_x_t_raw_ypu,
                &is_sufficient_context_T_domain);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_orca_stream_state_is_sufficient_context_t_domain,
                    pv_status_to_string(status));
            pv_ypu_mem_free(ypu, buffer_lfm_x_t_raw_ypu);
            pv_ypu_mem_free(ypu, buffer_bucket_non_streaming_ypu);
            pv_ypu_mem_free(ypu, buffer_gaussian_upsampled_ypu);
            pv_ypu_host_free(ypu, buffer_duration);
            return status;
        }
        if (!is_sufficient_context_T_domain && (state->status == PV_ORCA_STREAM_STATUS_ACTIVE) && !(state->is_flush)) {
            ORCA_LOG_VERBOSE_SIMPLE("[RETURN] Not enough T-domain context to synthesize audio");
            pv_ypu_mem_free(ypu, buffer_lfm_x_t_raw_ypu);
            pv_ypu_mem_free(ypu, buffer_bucket_non_streaming_ypu);
            pv_ypu_mem_free(ypu, buffer_gaussian_upsampled_ypu);
            pv_ypu_host_free(ypu, buffer_duration);
            return PV_STATUS_SUCCESS;
        }

        status = pv_orca_stream_state_update_t_domain(
                ypu,
                state,
                &num_to_T_domain);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_orca_stream_state_update_t_domain,
                    pv_status_to_string(status));
            pv_ypu_mem_free(ypu, buffer_lfm_x_t_raw_ypu);
            pv_ypu_mem_free(ypu, buffer_bucket_non_streaming_ypu);
            pv_ypu_mem_free(ypu, buffer_gaussian_upsampled_ypu);
            pv_ypu_host_free(ypu, buffer_duration);
            return status;
        }

        PV_ASSERT(num_to_T_domain > 0);
        buffer_T_domain_ypu = pv_buffer_ypu_get(ypu, state->buffer_T_domain_concat, num_to_T_domain, false);
        if (!buffer_T_domain_ypu) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("buffer_T_domain"));
            pv_ypu_mem_free(ypu, buffer_lfm_x_t_raw_ypu);
            pv_ypu_mem_free(ypu, buffer_bucket_non_streaming_ypu);
            pv_ypu_mem_free(ypu, buffer_gaussian_upsampled_ypu);
            pv_ypu_host_free(ypu, buffer_duration);
            return PV_STATUS_OUT_OF_MEMORY;
        }

        buffer_bucket_ypu = pv_buffer_ypu_get(ypu, state->buffer_bucket_concat, num_to_T_domain, false);
        if (!buffer_bucket_ypu) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("buffer_bucket"));
            pv_ypu_mem_free(ypu, buffer_lfm_x_t_raw_ypu);
            pv_ypu_mem_free(ypu, buffer_bucket_non_streaming_ypu);
            pv_ypu_mem_free(ypu, buffer_gaussian_upsampled_ypu);
            pv_ypu_host_free(ypu, buffer_duration);
            return PV_STATUS_OUT_OF_MEMORY;
        }

        buffer_lfm_x_t_ypu = pv_buffer_ypu_get(ypu, state->buffer_lfm_x_t_concat, num_to_T_domain, false);
        if (!buffer_lfm_x_t_ypu) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("buffer_lfm_x_t"));
            pv_ypu_mem_free(ypu, buffer_lfm_x_t_raw_ypu);
            pv_ypu_mem_free(ypu, buffer_bucket_non_streaming_ypu);
            pv_ypu_mem_free(ypu, buffer_gaussian_upsampled_ypu);
            pv_ypu_host_free(ypu, buffer_duration);
            return PV_STATUS_OUT_OF_MEMORY;
        }
    }

    T = num_to_T_domain;

    PV_ASSERT(T > 0);

    pv_ypu_mem_t *buffer_content_condition_ypu = pv_ypu_mem_alloc(
            ypu,
            T * object->param->lfm_film_generator_param->dimension * (int32_t) sizeof(float),
            PV_YPU_DEVICE_MEM_FLAG_NONE);
    if (!buffer_content_condition_ypu) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("buffer_content_condition"));
        pv_ypu_mem_free(ypu, buffer_lfm_x_t_raw_ypu);
        pv_ypu_mem_free(ypu, buffer_bucket_non_streaming_ypu);
        pv_ypu_mem_free(ypu, buffer_gaussian_upsampled_ypu);
        pv_ypu_host_free(ypu, buffer_duration);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    status = pv_orca_lfm_film_generator_forward(
            ypu,
            object->lfm_film_generator,
            T,
            buffer_T_domain_ypu,
            buffer_bucket_ypu,
            buffer_content_condition_ypu);
    pv_ypu_mem_free(ypu, buffer_gaussian_upsampled_ypu);
    pv_ypu_mem_free(ypu, buffer_bucket_non_streaming_ypu);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_lfm_film_generator_forward,
                pv_status_to_string(status));
        pv_ypu_mem_free(ypu, buffer_content_condition_ypu);
        pv_ypu_mem_free(ypu, buffer_lfm_x_t_raw_ypu);
        pv_ypu_host_free(ypu, buffer_duration);
        return status;
    }

    // Update pre-vocoder T-domain so to let vocoder avoids redundant re-computation.
    int32_t T_to_lfm = T;
    int32_t lfm_offset = 0;
    if (state->status == PV_ORCA_STREAM_STATUS_ACTIVE) {
        T_to_lfm -= (pv_max_int32(state->T_domain_garbage_lookback_offset - state->config->lfm_vf_estimator_lookback - state->config->vocoder_lookback, 0) +
                     pv_max_int32(state->T_domain_garbage_lookahead_offset - state->config->lfm_vf_estimator_lookahead - state->config->vocoder_lookahead, 0));
        lfm_offset = (pv_max_int32(state->T_domain_garbage_lookback_offset - state->config->lfm_vf_estimator_lookback - state->config->vocoder_lookback, 0));
    }

    for (int32_t i = 0; i < NFE; i++) {
        pv_ypu_mem_t *buffer_lfm_condition_ypu = pv_ypu_mem_alloc(
                ypu,
                T * object->param->lfm_vf_estimator_param->dimension * (int32_t) sizeof(float),
                PV_YPU_DEVICE_MEM_FLAG_NONE);
        if (!buffer_lfm_condition_ypu) {
            PV_ERROR_REPORT(
                    &pv_error_msg_ypu_device_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("buffer_lfm_condition"));
            pv_ypu_mem_free(ypu, buffer_content_condition_ypu);
            pv_ypu_mem_free(ypu, buffer_lfm_x_t_raw_ypu);
            pv_ypu_host_free(ypu, buffer_duration);
            return PV_STATUS_OUT_OF_MEMORY;
        }

        status = pv_orca_lfm_condition_fuser_forward(
                ypu,
                object->lfm_condition_fuser,
                T_to_lfm,
                buffer_content_condition_ypu,
                object->lfm_time_embedding,
                buffer_lfm_condition_ypu,
                lfm_offset * object->param->lfm_film_generator_param->dimension * (int32_t) sizeof(float),
                i * object->param->lfm_time_embedding_dim * (int32_t) sizeof(float),
                lfm_offset * object->param->lfm_vf_estimator_param->dimension * (int32_t) sizeof(float));
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_orca_lfm_condition_fuser_forward,
                    pv_status_to_string(status));
            pv_ypu_mem_free(ypu, buffer_lfm_condition_ypu);
            pv_ypu_mem_free(ypu, buffer_content_condition_ypu);
            pv_ypu_mem_free(ypu, buffer_lfm_x_t_raw_ypu);
            pv_ypu_host_free(ypu, buffer_duration);
            return status;
        }

        pv_ypu_mem_t *buffer_lfm_velocity_pred_ypu = pv_ypu_mem_alloc(
                ypu,
                T * object->param->lfm_vf_estimator_param->out_dimension * (int32_t) sizeof(float),
                PV_YPU_DEVICE_MEM_FLAG_NONE);
        if (!buffer_lfm_velocity_pred_ypu) {
            PV_ERROR_REPORT(
                    &pv_error_msg_ypu_device_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("buffer_lfm_velocity_pred"));
            pv_ypu_mem_free(ypu, buffer_lfm_condition_ypu);
            pv_ypu_mem_free(ypu, buffer_content_condition_ypu);
            pv_ypu_mem_free(ypu, buffer_lfm_x_t_raw_ypu);
            pv_ypu_host_free(ypu, buffer_duration);
            return PV_STATUS_OUT_OF_MEMORY;
        }

        status = pv_orca_lfm_vf_estimator_forward(
                ypu,
                object->lfm_vf_estimator,
                state,
                T_to_lfm,
                buffer_lfm_x_t_ypu,
                buffer_lfm_condition_ypu,
                buffer_lfm_velocity_pred_ypu,
                lfm_offset * object->param->lfm_vf_estimator_param->out_dimension * (int32_t) sizeof(float),
                lfm_offset * object->param->lfm_vf_estimator_param->dimension * (int32_t) sizeof(float),
                lfm_offset * object->param->lfm_vf_estimator_param->out_dimension * (int32_t) sizeof(float));
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_orca_lfm_vf_estimator_forward,
                    pv_status_to_string(status));
            pv_ypu_mem_free(ypu, buffer_lfm_velocity_pred_ypu);
            pv_ypu_mem_free(ypu, buffer_lfm_condition_ypu);
            pv_ypu_mem_free(ypu, buffer_content_condition_ypu);
            pv_ypu_mem_free(ypu, buffer_lfm_x_t_raw_ypu);
            pv_ypu_host_free(ypu, buffer_duration);
            return status;
        }

        const int32_t lfm_buffers_offset = lfm_offset * object->param->lfm_vf_estimator_param->out_dimension * (int32_t) sizeof(float);
        pv_ypu_op_elementwise_scalar_args_t args_mulsv = {
                .output = buffer_lfm_velocity_pred_ypu,
                .input = buffer_lfm_velocity_pred_ypu,
                .scalar.f32 = DT,
                .length = T_to_lfm * object->param->lfm_vf_estimator_param->out_dimension,
                .output_offset = lfm_buffers_offset,
                .input_offset = lfm_buffers_offset,
        };
        status = pv_ypu_operator_execute(
                ypu,
                PV_YPU_OPERATOR_MULSV,
                &args_mulsv);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT(
                    &pv_error_msg_ypu_operator_fail,
                    PV_ERROR_ARGS_PUBLIC("execute"),
                    PV_ERROR_ARGS_PRIVATE(
                            "execute",
                            pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_MULSV),
                            pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                            pv_status_to_string(status)));
            pv_ypu_mem_free(ypu, buffer_lfm_velocity_pred_ypu);
            pv_ypu_mem_free(ypu, buffer_lfm_condition_ypu);
            pv_ypu_mem_free(ypu, buffer_content_condition_ypu);
            pv_ypu_mem_free(ypu, buffer_lfm_x_t_raw_ypu);
            pv_ypu_host_free(ypu, buffer_duration);
            return status;
        }

        pv_ypu_op_pairwise_args_t args_add = {
                .output = buffer_lfm_x_t_ypu,
                .lhs = buffer_lfm_x_t_ypu,
                .rhs = buffer_lfm_velocity_pred_ypu,
                .length = T_to_lfm * object->param->lfm_vf_estimator_param->out_dimension,
                .output_offset = lfm_buffers_offset,
                .lhs_offset = lfm_buffers_offset,
                .rhs_offset = lfm_buffers_offset,
        };
        status = pv_ypu_operator_execute(
                ypu,
                PV_YPU_OPERATOR_ADD,
                &args_add);
        pv_ypu_mem_free(ypu, buffer_lfm_velocity_pred_ypu);
        pv_ypu_mem_free(ypu, buffer_lfm_condition_ypu);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT(
                    &pv_error_msg_ypu_operator_fail,
                    PV_ERROR_ARGS_PUBLIC("execute"),
                    PV_ERROR_ARGS_PRIVATE(
                            "execute",
                            pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_ADD),
                            pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                            pv_status_to_string(status)));
            pv_ypu_mem_free(ypu, buffer_content_condition_ypu);
            pv_ypu_mem_free(ypu, buffer_lfm_x_t_raw_ypu);
            pv_ypu_host_free(ypu, buffer_duration);
            return status;
        }
    }

    pv_ypu_mem_free(ypu, buffer_content_condition_ypu);

    // Update pre-vocoder T-domain so to let vocoder do less redundant re-computation.
    int32_t T_to_vocoder = T;
    int32_t vocoder_offset = 0;
    if (state->status == PV_ORCA_STREAM_STATUS_ACTIVE) {
        T_to_vocoder -= (pv_max_int32(state->T_domain_garbage_lookback_offset - state->config->vocoder_lookback, 0) +
                         pv_max_int32(state->T_domain_garbage_lookahead_offset - state->config->vocoder_lookahead, 0));
        vocoder_offset = (pv_max_int32(state->T_domain_garbage_lookback_offset - state->config->vocoder_lookback, 0)) *
                         object->param->lfm_vf_estimator_param->out_dimension;
    }

    int32_t num_samples_internal = 0;
    if ((state->status == PV_ORCA_STREAM_STATUS_ACTIVE) && (state->is_flush)) {
        num_samples_internal = PV_ORCA_WINDOW_SHIFT * (T_to_vocoder + 13 * 2);
    } else {
        num_samples_internal = PV_ORCA_WINDOW_SHIFT * T_to_vocoder;
    }

    int16_t *pcm_internal = malloc(num_samples_internal * sizeof(int16_t));
    if (!pcm_internal) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("pcm_internal"));
        pv_ypu_mem_free(ypu, buffer_lfm_x_t_raw_ypu);
        pv_ypu_host_free(ypu, buffer_duration);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    int32_t num_pcm_internal_out = 0;
    status = pv_orca_vocoder_forward_with_cache(
            ypu,
            object->vocoder,
            T_to_vocoder,
            buffer_lfm_x_t_ypu,
            pcm_internal,
            vocoder_offset * (int32_t) sizeof(float),
            (state->status == PV_ORCA_STREAM_STATUS_ACTIVE) ? state->is_flush : true,
            &num_pcm_internal_out);
    pv_ypu_mem_free(ypu, buffer_lfm_x_t_raw_ypu);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_vocoder_forward_with_cache,
                pv_status_to_string(status));
        free(pcm_internal);
        pv_ypu_host_free(ypu, buffer_duration);
        return status;
    }

    int32_t num_samples_chunk = 0;
    int32_t pcm_offset = 0;

    if (state->status == PV_ORCA_STREAM_STATUS_ACTIVE) {
        num_samples_chunk = num_pcm_internal_out;
    } else {
        num_samples_chunk = num_samples_internal;
    }

    if (num_samples_chunk != num_samples_internal) {
        int16_t *pcm_truncated = malloc(num_samples_chunk * sizeof(int16_t));
        if (!pcm_truncated) {
            PV_ERROR_REPORT(
                    &pv_error_msg_alloc,
                    PV_ERROR_ARGS_PUBLIC_EMPTY(),
                    PV_ERROR_ARGS_PRIVATE("pcm_truncated"));
            free(pcm_internal);
            pv_ypu_host_free(ypu, buffer_duration);
            return PV_STATUS_OUT_OF_MEMORY;
        }
        memcpy(pcm_truncated, pcm_internal + pcm_offset, num_samples_chunk * sizeof(int16_t));
        free(pcm_internal);
        *pcm = pcm_truncated;
    } else {
        *pcm = pcm_internal;
    }

    *num_samples = num_samples_chunk;
    *encoded_phonemes_durations = buffer_duration;

    return PV_STATUS_SUCCESS;
}
