#include <stdlib.h>
#include <string.h>

#include "math/pv_math.h"
#include "model/pv_activation.h"
#include "orca/pv_affine.h"
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
        pv_ypu_t *ypu,
        const pv_orca_duration_predictor_param_t *param,
        FILE *file) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(file);

    pv_status_t status = pv_cnn_param_serialize(ypu, param->conv_1_param, file);
    PV_CHECK_STATUS(status);

    status = pv_cnn_param_serialize(ypu, param->conv_2_param, file);
    PV_CHECK_STATUS(status);

    status = pv_layer_norm_param_serialize(ypu, param->layer_norm_1_param, file);
    PV_CHECK_STATUS(status);

    status = pv_layer_norm_param_serialize(ypu, param->layer_norm_2_param, file);
    PV_CHECK_STATUS(status);

    status = pv_cnn_param_serialize(ypu, param->conv_proj_param, file);
    PV_CHECK_STATUS(status);

    status = pv_affine_param_serialize(ypu, param->affine_pre_param, file);
    PV_CHECK_STATUS(status);

    status = pv_affine_param_serialize(ypu, param->affine_post_param, file);
    PV_CHECK_STATUS(status);

    return PV_STATUS_SUCCESS;
}

#endif

pv_status_t PV_MOCKABLE(pv_orca_duration_predictor_param_load)(
        pv_ypu_t *ypu,
        FILE *f,
        pv_orca_duration_predictor_param_t **param) {
    PV_ASSERT(ypu);
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_orca_duration_predictor_param_t *p = pv_ypu_host_alloc(ypu, sizeof(pv_orca_duration_predictor_param_t));
    PV_CHECK_ALLOC(p);

    memset(p, 0, sizeof(pv_orca_duration_predictor_param_t));

    pv_status_t status = pv_cnn_param_load(ypu, f, (pv_cnn_param_t **) &(p->conv_1_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_duration_predictor_param_delete(ypu, p);
        return status;
    }

    status = pv_cnn_param_load(ypu, f, (pv_cnn_param_t **) &(p->conv_2_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_duration_predictor_param_delete(ypu, p);
        return status;
    }

    status = pv_layer_norm_param_load(ypu, f, (pv_layer_norm_param_t **) &(p->layer_norm_1_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_duration_predictor_param_delete(ypu, p);
        return status;
    }

    status = pv_layer_norm_param_load(ypu, f, (pv_layer_norm_param_t **) &(p->layer_norm_2_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_duration_predictor_param_delete(ypu, p);
        return status;
    }

    status = pv_cnn_param_load(ypu, f, (pv_cnn_param_t **) &(p->conv_proj_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_duration_predictor_param_delete(ypu, p);
        return status;
    }

    status = pv_affine_param_load(ypu, f, (pv_affine_param_t **) &(p->affine_pre_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_duration_predictor_param_delete(ypu, p);
        return status;
    }

    status = pv_affine_param_load(ypu, f, (pv_affine_param_t **) &(p->affine_post_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_duration_predictor_param_delete(ypu, p);
        return status;
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}


void PV_MOCKABLE(pv_orca_duration_predictor_param_delete)(pv_ypu_t *ypu, pv_orca_duration_predictor_param_t *param) {
    PV_ASSERT(ypu);

    if (param) {
        pv_affine_param_delete(ypu, (pv_affine_param_t *) (param->affine_post_param));

        pv_affine_param_delete(ypu, (pv_affine_param_t *) (param->affine_pre_param));

        pv_cnn_param_delete(ypu, (pv_cnn_param_t *) (param->conv_proj_param));

        pv_layer_norm_param_delete(ypu, (pv_layer_norm_param_t *) (param->layer_norm_2_param));

        pv_layer_norm_param_delete(ypu, (pv_layer_norm_param_t *) (param->layer_norm_1_param));

        pv_cnn_param_delete(ypu, (pv_cnn_param_t *) (param->conv_2_param));

        pv_cnn_param_delete(ypu, (pv_cnn_param_t *) (param->conv_1_param));

        pv_ypu_host_free(ypu, param);
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
};

pv_status_t PV_MOCKABLE(pv_orca_duration_predictor_init)(
        pv_ypu_t *ypu,
        const pv_orca_duration_predictor_param_t *param,
        pv_orca_duration_predictor_t **object) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(object);

    *object = NULL;

    pv_orca_duration_predictor_t *o = pv_ypu_host_alloc(ypu, sizeof(pv_orca_duration_predictor_t));
    if (!o) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(o, 0, sizeof(pv_orca_duration_predictor_t));

    o->param = param;

    pv_status_t status = pv_cnn_init(ypu, param->conv_1_param, &(o->conv_1));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_duration_predictor_delete(ypu, o);
        return status;
    }

    status = pv_cnn_init(ypu, param->conv_2_param, &(o->conv_2));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_duration_predictor_delete(ypu, o);
        return status;
    }

    status = pv_layer_norm_init(ypu, param->layer_norm_1_param, &(o->layer_norm_1));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_duration_predictor_delete(ypu, o);
        return status;
    }

    status = pv_layer_norm_init(ypu, param->layer_norm_2_param, &(o->layer_norm_2));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_duration_predictor_delete(ypu, o);
        return status;
    }

    status = pv_cnn_init(ypu, param->conv_proj_param, &(o->conv_proj));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_duration_predictor_delete(ypu, o);
        return status;
    }

    status = pv_affine_init(ypu, param->affine_pre_param, &(o->affine_pre));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_duration_predictor_delete(ypu, o);
        return status;
    }

    status = pv_affine_init(ypu, param->affine_post_param, &(o->affine_post));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_duration_predictor_delete(ypu, o);
        return status;
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_orca_duration_predictor_delete)(pv_ypu_t *ypu, pv_orca_duration_predictor_t *object) {
    PV_ASSERT(ypu);

    if (object) {
        pv_affine_delete(ypu, object->affine_post);
        pv_affine_delete(ypu, object->affine_pre);
        pv_cnn_delete(ypu, object->conv_proj);
        pv_layer_norm_delete(ypu, object->layer_norm_2);
        pv_layer_norm_delete(ypu, object->layer_norm_1);
        pv_cnn_delete(ypu, object->conv_2);
        pv_cnn_delete(ypu, object->conv_1);
        pv_ypu_host_free(ypu, object);
    }
}

pv_status_t PV_MOCKABLE(pv_orca_duration_predictor_forward)(
        pv_ypu_t *ypu,
        pv_orca_duration_predictor_t *object,
        float speech_rate,
        int32_t num_tokens,
        pv_ypu_mem_t *x,
        int32_t *durations_token,
        int32_t x_offset) {
    PV_ASSERT(ypu);
    PV_ASSERT(object);
    PV_ASSERT(speech_rate > 0);
    PV_ASSERT(num_tokens);
    PV_ASSERT(x);
    PV_ASSERT(durations_token);
    PV_ORCA_PROFILER_START("duration_predictor");

    pv_ypu_mem_t *buffer_1 = pv_ypu_buffer_get(
            ypu,
            num_tokens * pv_cnn_output_channels(object->conv_1) * (int32_t) sizeof(float),
            false);
    if (!buffer_1) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_status_t status = pv_cnn_forward(
            ypu,
            object->conv_1,
            num_tokens,
            x,
            buffer_1,
            x_offset,
            0);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_activation_relu_float(
            ypu,
            num_tokens * pv_cnn_output_channels(object->conv_1),
            buffer_1,
            0);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_layer_norm_forward(
            ypu,
            object->layer_norm_1,
            num_tokens,
            buffer_1,
            buffer_1,
            0,
            0);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_mem_t *buffer_2 = pv_ypu_buffer_get(
            ypu,
            num_tokens * pv_cnn_output_channels(object->conv_2) * (int32_t) sizeof(float),
            false);
    if (!buffer_2) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    status = pv_cnn_forward(
            ypu,
            object->conv_2,
            num_tokens,
            buffer_1,
            buffer_2,
            0,
            0);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_buffer_release(ypu, buffer_1);

    status = pv_activation_relu_float(
            ypu,
            num_tokens * pv_cnn_output_channels(object->conv_2),
            buffer_2,
            0);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_layer_norm_forward(
            ypu,
            object->layer_norm_2,
            num_tokens,
            buffer_2,
            buffer_2,
            0,
            0);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_affine_forward(
            ypu,
            object->affine_pre,
            num_tokens,
            buffer_2,
            buffer_2,
            0,
            0);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_mem_t *buffer_3 = pv_ypu_buffer_get(
            ypu,
            num_tokens * pv_cnn_output_channels(object->conv_proj) * (int32_t) sizeof(float),
            false);
    if (!buffer_3) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    status = pv_cnn_forward(
            ypu,
            object->conv_proj,
            num_tokens,
            buffer_2,
            buffer_3,
            0,
            0);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_buffer_release(ypu, buffer_2);

    status = pv_affine_forward(
            ypu,
            object->affine_post,
            num_tokens,
            buffer_3,
            buffer_3,
            0,
            0);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    const int32_t num_output_channels = pv_affine_num_channels(object->affine_post);
    for (int32_t i = 0; i < num_tokens; i++) {
        const int32_t offset = i * num_output_channels;
        int32_t duration_token = 0;
        status = pv_orca_duration_predictor_duration(
                ypu,
                num_output_channels,
                buffer_3,
                speech_rate,
                offset * (int32_t) sizeof(float),
                &duration_token);
        if (status != PV_STATUS_SUCCESS) {
            return status;
        }
        durations_token[i] = duration_token;
    }

    pv_ypu_buffer_release(ypu, buffer_3);

    PV_ORCA_PROFILER_STOP("duration_predictor");

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_orca_duration_predictor_duration)(
        pv_ypu_t *ypu,
        int32_t num_channels,
        pv_ypu_mem_t *x_ypu_mem,
        float speech_rate,
        int32_t x_offset,
        int32_t *duration_token) {
    PV_ASSERT(ypu);
    PV_ASSERT(num_channels > 0);
    PV_ASSERT(x_ypu_mem);
    PV_ASSERT(speech_rate > 0);
    PV_ASSERT(duration_token);

    pv_ypu_mem_t *temp = pv_ypu_buffer_get(
            ypu,
            sizeof(float),
            false);
    if (temp == NULL) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_op_elementwise_scalar_args_t args0 = {
            .output = x_ypu_mem,
            .input = x_ypu_mem,
            .scalar.f32 = 0.0f,
            .length = num_channels,
            .output_offset = x_offset,
            .input_offset = x_offset,
    };

    pv_status_t status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_SUBSV,
            &args0);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_op_elementwise_args_t args1 = {
            .output = x_ypu_mem,
            .input = x_ypu_mem,
            .length = num_channels,
            .output_offset = x_offset,
            .input_offset = x_offset,
    };

    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_EXP,
            &args1);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_op_elementwise_scalar_args_t args2 = {
            .output = x_ypu_mem,
            .input = x_ypu_mem,
            .scalar.f32 = 1.0f,
            .length = num_channels,
            .output_offset = x_offset,
            .input_offset = x_offset,
    };

    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_ADDSV,
            &args2);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_op_elementwise_scalar_args_t args3 = {
            .output = x_ypu_mem,
            .input = x_ypu_mem,
            .scalar.f32 = 1.0f,
            .length = num_channels,
            .output_offset = x_offset,
            .input_offset = x_offset,
    };

    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_DIVSV,
            &args3);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_op_elementwise_broadcast_args_t args4 = {
            .output = temp,
            .input = x_ypu_mem,
            .m = 1,
            .n = num_channels,
            .output_offset = 0,
            .input_offset = x_offset,
    };

    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_SUM,
            &args4);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    float predicted_duration_float = 0.0f;
    status = pv_ypu_mem_copy_from(
            ypu,
            temp,
            &predicted_duration_float,
            0,
            sizeof(float));
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_buffer_release(ypu, temp);

    int32_t predicted_duration_int = (int32_t) roundf(predicted_duration_float / speech_rate);
    predicted_duration_int = predicted_duration_int < 1 ? 1 : predicted_duration_int;

    *duration_token = predicted_duration_int;
    return PV_STATUS_SUCCESS;
}
