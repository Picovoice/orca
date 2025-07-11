#include <stdlib.h>

#include "core/pv_error_messages.h"
#include "orca/pv_cnn_transposed.h"
#include "orca/pv_cnn.h"
#include "orca/pv_cnn_matmul.h"
#include "orca/pv_orca_util.h"
#include "orca/pv_profiler.h"
#include "util/pv_check_status.h"
#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_cnn_transposed_depthwise_param_serialize)(
        const pv_cnn_transposed_depthwise_param_t *param,
        FILE *file) {
    PV_ASSERT(param);
    PV_ASSERT(file);

    return pv_cnn_depthwise_param_serialize((pv_cnn_depthwise_param_t *) param, file);
}

#endif

pv_status_t PV_MOCKABLE(pv_cnn_transposed_depthwise_param_load)(FILE *f, pv_cnn_transposed_depthwise_param_t **param) {
    PV_ASSERT(f);
    PV_ASSERT(param);

    return pv_cnn_depthwise_param_load(f, (pv_cnn_depthwise_param_t **) param);
}

void PV_MOCKABLE(pv_cnn_transposed_depthwise_param_delete)(pv_cnn_transposed_depthwise_param_t *param) {
    pv_cnn_depthwise_param_delete((pv_cnn_depthwise_param_t *) param);
}

bool PV_MOCKABLE(pv_cnn_transposed_depthwise_param_is_equal)(
        const pv_cnn_transposed_depthwise_param_t *object,
        const pv_cnn_transposed_depthwise_param_t *other) {
    PV_ASSERT(object);
    PV_ASSERT(other);

    return pv_cnn_depthwise_param_is_equal(
            (const pv_cnn_depthwise_param_t *) object,
            (const pv_cnn_depthwise_param_t *) other);
}

int32_t PV_MOCKABLE(pv_cnn_transposed_depthwise_param_receptive_field)(
        const pv_cnn_transposed_depthwise_param_t *object) {
    PV_ASSERT(object);

    PV_ASSERT((object->kernel_size - object->stride) % 2 == 0);

    return (object->kernel_size - object->stride) / 2;
}

/* layout-compatible with pv_cnn_depthwise_t */
struct pv_cnn_transposed_depthwise {
    const pv_cnn_transposed_depthwise_param_t *param;

    q7_t *transposed_weight;
};

static pv_status_t pv_cnn_transposed_depthwise_forward_float_to_float(
        pv_cnn_transposed_depthwise_t *object,
        int32_t n,
        const float *x,
        float *y) {
    PV_ASSERT(object);
    PV_ASSERT(n);
    PV_ASSERT(x);
    PV_ASSERT(y);
    PV_ORCA_PROFILER_START("cnn_transposed_depthwise");

    const q7_t *weight = pv_cnn_depthwise_get_weight((pv_cnn_depthwise_t *) object);
    const q7_t *bias = object->param->bias;

    int32_t num_channels = pv_cnn_depthwise_num_channels((pv_cnn_depthwise_t *) object);
    int32_t kernel_size = object->param->kernel_size;
    int32_t padding = object->param->padding;

    const int32_t num_input_elements = n * num_channels;
    float inverse_scale = 0.f;
    q510_t *x_q510 = calloc(num_input_elements, sizeof(q510_t));
    if (!x_q510) {
        return PV_STATUS_OUT_OF_MEMORY;
    }
    pv_orca_util_scale_and_quantize_activation(num_input_elements, 0, x, x_q510, &inverse_scale);

    const int32_t num_output_frames = pv_cnn_transposed_depthwise_num_output_frames(object, n);
    const int32_t num_output_elements_padded = (num_output_frames + 2 * padding) * num_channels;

    int32_t *buffer = calloc(num_output_elements_padded, sizeof(int32_t));
    if (!buffer) {
        free(x_q510);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    int32_t stride = object->param->stride;
    for (int32_t frame = 0; frame < n; frame++) {
        const int32_t frame_offset = frame * num_channels;
        const int32_t frame_offset_strided = frame * num_channels * stride;

        for (int32_t ke = 0; ke < kernel_size; ke++) {
            const int32_t kernel_offset = ke * num_channels;

            for (int32_t nc = 0; nc < num_channels; nc++) {
                buffer[frame_offset_strided + kernel_offset + nc] +=
                        (int32_t) x_q510[frame_offset + nc] * (int32_t) weight[kernel_offset + nc];
            }
        }
    }

    const int32_t padding_numel = padding * num_channels;
    for (int32_t f = 0; f < num_output_frames; f++) {
        const int32_t frame_offset = f * num_channels;

        for (int32_t nc = 0; nc < num_channels; nc++) {
            y[frame_offset + nc] =
                    ((float) buffer[padding_numel + frame_offset + nc] * inverse_scale)  + (((float) bias[nc]) / 128.f);
        }
    }

    free(x_q510);
    free(buffer);

    PV_ORCA_PROFILER_STOP("cnn_transposed_depthwise");
    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_cnn_transposed_depthwise_init)(
        const pv_cnn_transposed_depthwise_param_t *param,
        pv_cnn_transposed_depthwise_t **object) {
    PV_ASSERT(param);
    PV_ASSERT(object);

    return pv_cnn_depthwise_init((const pv_cnn_depthwise_param_t *) param, (pv_cnn_depthwise_t **) object);
}

void PV_MOCKABLE(pv_cnn_transposed_depthwise_delete)(pv_cnn_transposed_depthwise_t *object) {
    return pv_cnn_depthwise_delete((pv_cnn_depthwise_t *) object);
}

pv_status_t PV_MOCKABLE(pv_cnn_transposed_depthwise_forward)(
        pv_cnn_transposed_depthwise_t *object,
        int32_t n,
        const float *x,
        float *y) {
    PV_ASSERT(object);
    PV_ASSERT(n);
    PV_ASSERT(x);
    PV_ASSERT(y);

    return pv_cnn_transposed_depthwise_forward_float_to_float(object, n, x, y);
}

int32_t PV_MOCKABLE(pv_cnn_transposed_depthwise_num_output_frames)(
        const pv_cnn_transposed_depthwise_t *object,
        int32_t n) {
    PV_ASSERT(object);
    PV_ASSERT(n > 0);

    return (((n - 1) * object->param->stride) + object->param->kernel_size) - (2 * object->param->padding);
}
