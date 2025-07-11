#include <stdlib.h>

#include "math/pv_math.h"
#include "nn/pv_activation.h"
#include "orca/pv_affine.h"
#include "orca/pv_buffer.h"
#include "orca/pv_cnn.h"
#include "orca/pv_orca_duration_predictor.h"
#include "orca/pv_profiler.h"
#include "util/pv_check_status.h"
#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

#define DURATION_PREDICTOR_MAX_PHONEME_LENGTH (100)

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_orca_duration_predictor_param_serialize)(
        const pv_orca_duration_predictor_param_t *param,
        FILE *file) {
    PV_ASSERT(param);
    PV_ASSERT(file);

    pv_status_t status = pv_cnn_param_serialize(param->conv_1_param, file);
    PV_CHECK_STATUS(status);

    status = pv_cnn_param_serialize(param->conv_2_param, file);
    PV_CHECK_STATUS(status);

    status = pv_layer_norm_param_serialize(param->layer_norm_1_param, file);
    PV_CHECK_STATUS(status);

    status = pv_layer_norm_param_serialize(param->layer_norm_2_param, file);
    PV_CHECK_STATUS(status);

    status = pv_cnn_param_serialize(param->conv_proj_param, file);
    PV_CHECK_STATUS(status);

    status = pv_affine_param_serialize(param->affine_pre_param, file);
    PV_CHECK_STATUS(status);

    status = pv_affine_param_serialize(param->affine_post_param, file);
    PV_CHECK_STATUS(status);

    return PV_STATUS_SUCCESS;
}

#endif

pv_status_t PV_MOCKABLE(pv_orca_duration_predictor_param_load)(FILE *f, pv_orca_duration_predictor_param_t **param) {
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_orca_duration_predictor_param_t *p = calloc(1, sizeof(pv_orca_duration_predictor_param_t));
    PV_CHECK_ALLOC(p);

    pv_status_t status = pv_cnn_param_load(f, (pv_cnn_param_t **) &(p->conv_1_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_duration_predictor_param_delete(p);
        return status;
    }

    status = pv_cnn_param_load(f, (pv_cnn_param_t **) &(p->conv_2_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_duration_predictor_param_delete(p);
        return status;
    }

    status = pv_layer_norm_param_load(f, (pv_layer_norm_param_t **) &(p->layer_norm_1_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_duration_predictor_param_delete(p);
        return status;
    }

    status = pv_layer_norm_param_load(f, (pv_layer_norm_param_t **) &(p->layer_norm_2_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_duration_predictor_param_delete(p);
        return status;
    }

    status = pv_cnn_param_load(f, (pv_cnn_param_t **) &(p->conv_proj_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_duration_predictor_param_delete(p);
        return status;
    }

    status = pv_affine_param_load(f, (pv_affine_param_t **) &(p->affine_pre_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_duration_predictor_param_delete(p);
        return status;
    }

    status = pv_affine_param_load(f, (pv_affine_param_t **) &(p->affine_post_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_duration_predictor_param_delete(p);
        return status;
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}


void PV_MOCKABLE(pv_orca_duration_predictor_param_delete)(pv_orca_duration_predictor_param_t *param) {
    if (param) {
        pv_affine_param_delete((pv_affine_param_t *) (param->affine_post_param));

        pv_affine_param_delete((pv_affine_param_t *) (param->affine_pre_param));

        pv_cnn_param_delete((pv_cnn_param_t *) (param->conv_proj_param));

        pv_layer_norm_param_delete((pv_layer_norm_param_t *) (param->layer_norm_2_param));

        pv_layer_norm_param_delete((pv_layer_norm_param_t *) (param->layer_norm_1_param));

        pv_cnn_param_delete((pv_cnn_param_t *) (param->conv_2_param));

        pv_cnn_param_delete((pv_cnn_param_t *) (param->conv_1_param));

        free(param);
    }
}

bool PV_MOCKABLE(pv_orca_duration_predictor_param_is_equal)(
        const pv_orca_duration_predictor_param_t *object,
        const pv_orca_duration_predictor_param_t *other) {
    PV_ASSERT(object);
    PV_ASSERT(other);

    if (!pv_cnn_param_is_equal(object->conv_1_param, other->conv_1_param)) {
        return false;
    }

    if (!pv_cnn_param_is_equal(object->conv_2_param, other->conv_2_param)) {
        return false;
    }

    if (!pv_layer_norm_param_is_equal(object->layer_norm_1_param, other->layer_norm_1_param)) {
        return false;
    }

    if (!pv_layer_norm_param_is_equal(object->layer_norm_2_param, other->layer_norm_2_param)) {
        return false;
    }

    if (!pv_cnn_param_is_equal(object->conv_proj_param, other->conv_proj_param)) {
        return false;
    }

    if (!pv_affine_param_is_equal(object->affine_pre_param, other->affine_pre_param)) {
        return false;
    }

    if (!pv_affine_param_is_equal(object->affine_post_param, other->affine_post_param)) {
        return false;
    }

    return true;
}

int32_t PV_MOCKABLE(pv_orca_duration_predictor_param_receptive_field)(
        const pv_orca_duration_predictor_param_t *object) {
    PV_ASSERT(object);

    int32_t receptive_field = 0;
    receptive_field += pv_cnn_param_receptive_field(object->conv_1_param);
    receptive_field += pv_cnn_param_receptive_field(object->conv_2_param);
    return receptive_field;
}

struct pv_orca_duration_predictor {
    const pv_orca_duration_predictor_param_t *param;

    pv_cnn_t *conv_1;
    pv_cnn_t *conv_2;
    pv_layer_norm_t *layer_norm_1;
    pv_layer_norm_t *layer_norm_2;
    pv_cnn_t *conv_proj;
    pv_affine_t *affine_post;
    pv_affine_t *affine_pre;

    pv_buffer_t *buffer_1;
    pv_buffer_t *buffer_2;
    pv_buffer_t *buffer_3;
};

pv_status_t PV_MOCKABLE(pv_orca_duration_predictor_init)(
        const pv_orca_duration_predictor_param_t *param,
        pv_orca_duration_predictor_t **object) {
    PV_ASSERT(param);
    PV_ASSERT(object);

    *object = NULL;

    pv_orca_duration_predictor_t *o = calloc(1, sizeof(pv_orca_duration_predictor_t));
    if (!o) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->param = param;

    pv_status_t status = pv_cnn_init(param->conv_1_param, &(o->conv_1));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_duration_predictor_delete(o);
        return status;
    }

    status = pv_cnn_init(param->conv_2_param, &(o->conv_2));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_duration_predictor_delete(o);
        return status;
    }

    status = pv_layer_norm_init(param->layer_norm_1_param, &(o->layer_norm_1));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_duration_predictor_delete(o);
        return status;
    }

    status = pv_layer_norm_init(param->layer_norm_2_param, &(o->layer_norm_2));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_duration_predictor_delete(o);
        return status;
    }

    status = pv_cnn_init(param->conv_proj_param, &(o->conv_proj));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_duration_predictor_delete(o);
        return status;
    }

    status = pv_affine_init(param->affine_pre_param, &(o->affine_pre));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_duration_predictor_delete(o);
        return status;
    }

    status = pv_affine_init(param->affine_post_param, &(o->affine_post));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_duration_predictor_delete(o);
        return status;
    }

    status = pv_buffer_init(pv_cnn_output_channels(o->conv_1), &(o->buffer_1));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_duration_predictor_delete(o);
        return status;
    }

    status = pv_buffer_init(pv_cnn_output_channels(o->conv_2), &(o->buffer_2));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_duration_predictor_delete(o);
        return status;
    }

    int32_t num_channels = pv_cnn_output_channels(o->conv_proj);
    status = pv_buffer_init(num_channels, &(o->buffer_3));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_duration_predictor_delete(o);
        return status;
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_orca_duration_predictor_delete)(pv_orca_duration_predictor_t *object) {
    if (object) {
        pv_buffer_delete(object->buffer_3);
        pv_buffer_delete(object->buffer_2);
        pv_buffer_delete(object->buffer_1);

        pv_affine_delete(object->affine_post);
        pv_affine_delete(object->affine_pre);
        pv_cnn_delete(object->conv_proj);
        pv_layer_norm_delete(object->layer_norm_2);
        pv_layer_norm_delete(object->layer_norm_1);
        pv_cnn_delete(object->conv_2);
        pv_cnn_delete(object->conv_1);

        free(object);
    }
}

pv_status_t PV_MOCKABLE(pv_orca_duration_predictor_forward)(
        pv_orca_duration_predictor_t *object,
        float speech_rate,
        int32_t num_tokens,
        const float *x,
        int32_t *durations_token) {
    PV_ASSERT(object);
    PV_ASSERT(speech_rate > 0);
    PV_ASSERT(num_tokens);
    PV_ASSERT(x);
    PV_ASSERT(durations_token);
    PV_ORCA_PROFILER_START("duration_predictor");

    float *buffer_1 = pv_buffer_get(object->buffer_1, num_tokens, false);
    if (!buffer_1) {
        return PV_STATUS_OUT_OF_MEMORY;
    }
    pv_status_t status = pv_cnn_forward(object->conv_1, num_tokens, x, buffer_1);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_activation_relu_float(num_tokens * pv_cnn_output_channels(object->conv_1), buffer_1);

    status = pv_layer_norm_forward(object->layer_norm_1, num_tokens, buffer_1, buffer_1);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    float *buffer_2 = pv_buffer_get(object->buffer_2, num_tokens, false);
    if (!buffer_2) {
        return PV_STATUS_OUT_OF_MEMORY;
    }
    status = pv_cnn_forward(object->conv_2, num_tokens, buffer_1, buffer_2);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }
    pv_buffer_free(object->buffer_1);

    pv_activation_relu_float(num_tokens * pv_cnn_output_channels(object->conv_2), buffer_2);

    status = pv_layer_norm_forward(object->layer_norm_2, num_tokens, buffer_2, buffer_2);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_affine_forward(object->affine_pre, num_tokens, buffer_2, buffer_2);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    float *buffer_3 = pv_buffer_get(object->buffer_3, num_tokens, true);
    if (!buffer_3) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    status = pv_cnn_forward(object->conv_proj, num_tokens, buffer_2, buffer_3);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }
    pv_buffer_free(object->buffer_2);

    status = pv_affine_forward(object->affine_post, num_tokens, buffer_3, buffer_3);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    const int32_t num_output_channels = pv_affine_num_channels(object->affine_post);
    for (int32_t i = 0; i < num_tokens; i++) {
        const int32_t offset = i * num_output_channels;
        durations_token[i] = pv_orca_duration_predictor_duration(num_output_channels, buffer_3 + offset, speech_rate);
    }
    pv_buffer_free(object->buffer_3);

    PV_ORCA_PROFILER_STOP("duration_predictor");

    return PV_STATUS_SUCCESS;
}

int32_t PV_MOCKABLE(pv_orca_duration_predictor_duration)(int32_t num_channels, float *x, float speech_rate) {
    PV_ASSERT(num_channels > 0);
    PV_ASSERT(x);
    PV_ASSERT(speech_rate > 0);

    float predicted_duration_float = 0;
    for (int32_t i = 0; i < num_channels; i++) {
        x[i] = 1.f / (1.f + expf(-x[i]));
        predicted_duration_float += x[i];
    }

    int32_t predicted_duration_int = (int32_t) roundf(predicted_duration_float / speech_rate);
    predicted_duration_int = predicted_duration_int < 1 ? 1 : predicted_duration_int;
    return predicted_duration_int;
}
