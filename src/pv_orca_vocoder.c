#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "nn/pv_activation.h"
#include "orca/pv_buffer.h"
#include "orca/pv_cnn.h"
#include "orca/pv_convnext_transposed.h"
#include "orca/pv_orca_istft.h"
#include "orca/pv_orca_vocoder.h"
#include "orca/pv_profiler.h"
#include "util/pv_check_status.h"
#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_orca_vocoder_param_serialize)(const pv_orca_vocoder_param_t *param, FILE *file) {
    PV_ASSERT(param);
    PV_ASSERT(file);

    pv_status_t status = pv_cnn_param_serialize(param->conv_pre_param, file);
    PV_CHECK_STATUS(status);

    status = pv_convnext_transposed_param_serialize(param->convnext_transposed_0_param, file);
    PV_CHECK_STATUS(status);

    status = pv_convnext_transposed_param_serialize(param->convnext_transposed_1_param, file);
    PV_CHECK_STATUS(status);

    status = pv_vocos_backbone_param_serialize(param->backbone_0_param, file);
    PV_CHECK_STATUS(status);

    status = pv_vocos_backbone_param_serialize(param->backbone_1_param, file);
    PV_CHECK_STATUS(status);

    status = pv_cnn_param_serialize(param->conv_proj_param, file);
    PV_CHECK_STATUS(status);

    size_t count = fwrite(&(param->pcm_normalization_factor), sizeof(float), 1, file);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

    return PV_STATUS_SUCCESS;
}

#endif

pv_status_t PV_MOCKABLE(pv_orca_vocoder_param_load)(FILE *f, pv_orca_vocoder_param_t **param) {
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_orca_vocoder_param_t *p = calloc(1, sizeof(pv_orca_vocoder_param_t));
    PV_CHECK_ALLOC(p);

    pv_status_t status = pv_cnn_param_load(f, (pv_cnn_param_t **) &(p->conv_pre_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_vocoder_param_delete(p);
        return status;
    }

    status = pv_convnext_transposed_param_load(
            f, (pv_convnext_transposed_param_t **) &(p->convnext_transposed_0_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_vocoder_param_delete(p);
        return status;
    }

    status = pv_convnext_transposed_param_load(
            f, (pv_convnext_transposed_param_t **) &(p->convnext_transposed_1_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_vocoder_param_delete(p);
        return status;
    }

    status = pv_vocos_backbone_param_load(f, (pv_vocos_backbone_param_t **) &(p->backbone_0_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_vocoder_param_delete(p);
        return status;
    }

    status = pv_vocos_backbone_param_load(f, (pv_vocos_backbone_param_t **) &(p->backbone_1_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_vocoder_param_delete(p);
        return status;
    }

    status = pv_cnn_param_load(f, (pv_cnn_param_t **) &(p->conv_proj_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_vocoder_param_delete(p);
        return status;
    }

    size_t count = pv_fread(&(p->pcm_normalization_factor), sizeof(float), 1, f);
    if (count != 1) {
        pv_orca_vocoder_param_delete(p);
        return PV_STATUS_IO_ERROR;
    }
    if ((p->pcm_normalization_factor <= 0) || (p->pcm_normalization_factor > 1.0)) {
        pv_orca_vocoder_param_delete(p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_orca_vocoder_param_delete)(pv_orca_vocoder_param_t *param) {
    if (param) {
        pv_cnn_param_delete((pv_cnn_param_t *) (param->conv_proj_param));

        pv_vocos_backbone_param_delete((pv_vocos_backbone_param_t *) (param->backbone_1_param));
        pv_vocos_backbone_param_delete((pv_vocos_backbone_param_t *) (param->backbone_0_param));

        pv_convnext_transposed_param_delete((pv_convnext_transposed_param_t *) (param->convnext_transposed_1_param));
        pv_convnext_transposed_param_delete((pv_convnext_transposed_param_t *) (param->convnext_transposed_0_param));

        pv_cnn_param_delete((pv_cnn_param_t *) (param->conv_pre_param));

        free(param);
    }
}

bool PV_MOCKABLE(pv_orca_vocoder_param_is_equal)(
        const pv_orca_vocoder_param_t *object,
        const pv_orca_vocoder_param_t *other) {
    PV_ASSERT(object);
    PV_ASSERT(other);

    if (!pv_cnn_param_is_equal(object->conv_pre_param, other->conv_pre_param)) {
        return false;
    }

    if (!pv_convnext_transposed_param_is_equal(
            object->convnext_transposed_0_param, other->convnext_transposed_0_param)) {
        return false;
    }

    if (!pv_convnext_transposed_param_is_equal(
            object->convnext_transposed_1_param, other->convnext_transposed_1_param)) {
        return false;
    }

    if (!pv_vocos_backbone_param_is_equal(object->backbone_0_param, other->backbone_0_param)) {
        return false;
    }

    if (!pv_vocos_backbone_param_is_equal(object->backbone_1_param, other->backbone_1_param)) {
        return false;
    }

    if (!pv_cnn_param_is_equal(object->conv_proj_param, other->conv_proj_param)) {
        return false;
    }

    if (object->pcm_normalization_factor != other->pcm_normalization_factor) {
        return false;
    }

    return true;
}

int32_t PV_MOCKABLE(pv_orca_vocoder_param_receptive_field)(const pv_orca_vocoder_param_t *object) {
    PV_ASSERT(object);

    int32_t conv_pre_receptive_field = pv_cnn_param_receptive_field(object->conv_pre_param);

    const pv_cnn_transposed_depthwise_param_t *conv_transpose_0_param =
            object->convnext_transposed_0_param->conv_transposed_depthwise_param;
    const pv_cnn_transposed_depthwise_param_t *conv_transpose_1_param =
            object->convnext_transposed_1_param->conv_transposed_depthwise_param;

    float num_upsample_0 = (float) conv_transpose_0_param->stride;
    float num_upsample_1 = (float) conv_transpose_1_param->stride;

    int32_t convnext_transposed_0_receptive_field =
            pv_cnn_transposed_depthwise_param_receptive_field(conv_transpose_0_param);

    float convnext_transposed_1_receptive_field =
            (float) pv_cnn_transposed_depthwise_param_receptive_field(conv_transpose_0_param) / (float) num_upsample_0;

    float backbone_0_receptive_field =
            (float) pv_vocos_backbone_param_receptive_field(object->backbone_0_param) / (float) num_upsample_0;

    float backbone_1_receptive_field =
            (float) pv_vocos_backbone_param_receptive_field(object->backbone_1_param) /
            (float) (num_upsample_0 * num_upsample_1);

    float conv_proj_receptive_field =
            (float) pv_cnn_param_receptive_field(object->conv_proj_param) / (float) (num_upsample_0 * num_upsample_1);

    float istft_contribution = 3 / (float) (num_upsample_0 * num_upsample_1);

    float fractional_contribution =
            convnext_transposed_1_receptive_field +
            backbone_0_receptive_field +
            backbone_1_receptive_field +
            conv_proj_receptive_field +
            istft_contribution;
    int32_t rounded_fractional_contribution = (int32_t) ceilf(fractional_contribution);

    return conv_pre_receptive_field + convnext_transposed_0_receptive_field + rounded_fractional_contribution;
}

struct pv_orca_vocoder {
    const pv_orca_vocoder_param_t *param;

    pv_cnn_t *conv_pre;
    pv_convnext_transposed_t *convnext_transposed_0;
    pv_convnext_transposed_t *convnext_transposed_1;
    pv_vocos_backbone_t *backbone_0;
    pv_vocos_backbone_t *backbone_1;
    pv_cnn_t *conv_proj;

    pv_orca_istft_t *istft;

    pv_buffer_t *buffer_conv_pre;
    pv_buffer_t *buffer_convnext_transposed;
    pv_buffer_t *buffer_backbone;
    pv_buffer_t *buffer_spec_all_subbands;
};

pv_status_t PV_MOCKABLE(pv_orca_vocoder_init)(
        const pv_orca_vocoder_param_t *param,
        pv_orca_vocoder_t **object) {
    PV_ASSERT(param);
    PV_ASSERT(object);

    *object = NULL;

    pv_orca_vocoder_t *o = calloc(1, sizeof(pv_orca_vocoder_t));
    if (!o) {
        pv_orca_vocoder_delete(o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->param = param;

    pv_status_t status = pv_cnn_init(param->conv_pre_param, &(o->conv_pre));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_vocoder_delete(o);
        return status;
    }

    status = pv_convnext_transposed_init(param->convnext_transposed_0_param, &(o->convnext_transposed_0));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_vocoder_delete(o);
        return status;
    }

    status = pv_convnext_transposed_init(param->convnext_transposed_1_param, &(o->convnext_transposed_1));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_vocoder_delete(o);
        return status;
    }

    status = pv_vocos_backbone_init(param->backbone_0_param, &(o->backbone_0));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_vocoder_delete(o);
        return status;
    }

    status = pv_vocos_backbone_init(param->backbone_1_param, &(o->backbone_1));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_vocoder_delete(o);
        return status;
    }

    status = pv_cnn_init(param->conv_proj_param, &(o->conv_proj));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_vocoder_delete(o);
        return status;
    }

    int32_t num_channels_conv_pre = o->param->conv_pre_param->output_channels;
    status = pv_buffer_init(num_channels_conv_pre, &(o->buffer_conv_pre));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_vocoder_delete(o);
        return status;
    }

    int32_t num_channels_backbone_0 = o->param->backbone_0_param->layer_norm_pre_param->num_channels;
    int32_t num_channels_backbone_1 = o->param->backbone_1_param->layer_norm_pre_param->num_channels;
    int32_t num_upsample_0 = o->param->convnext_transposed_0_param->conv_transposed_depthwise_param->stride;
    PV_ASSERT((num_channels_backbone_1 * num_upsample_0) > num_channels_backbone_0);
    status = pv_buffer_init(num_channels_backbone_1, &(o->buffer_backbone));  // take larger buffer for both backbones
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_vocoder_delete(o);
        return status;
    }

    int32_t num_channels_convnext_transposed_1 = o->param->convnext_transposed_1_param->conv_2_param->output_channels;
    // take larger buffer for both upsample layers
    int32_t num_upsample_1 =
            num_upsample_0 * o->param->convnext_transposed_1_param->conv_transposed_depthwise_param->stride;
    PV_ASSERT(
            num_upsample_1 * o->param->convnext_transposed_1_param->conv_2_param->output_channels >
            num_upsample_0 * o->param->convnext_transposed_1_param->conv_2_param->output_channels);
    status = pv_buffer_init(num_channels_convnext_transposed_1, &(o->buffer_convnext_transposed));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_vocoder_delete(o);
        return status;
    }

    (void) num_upsample_0;
    (void) num_upsample_1;
    (void) num_channels_backbone_0;

    int32_t num_channels_spec = pv_cnn_output_channels(o->conv_proj);
    PV_ASSERT(num_channels_spec == (PV_ORCA_VOCODER_NUM_SUBBANDS * (PV_ORCA_VOCODER_NUM_FFT + 2)));
    status = pv_buffer_init(num_channels_spec, &(o->buffer_spec_all_subbands));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_vocoder_delete(o);
        return status;
    }

    status = pv_orca_istft_init(
            PV_ORCA_VOCODER_WINDOW_LENGTH,
            PV_ORCA_VOCODER_WINDOW_SHIFT,
            PV_ORCA_VOCODER_NUM_FFT,
            PV_ORCA_VOCODER_NUM_SUBBANDS,
            o->param->pcm_normalization_factor,
            &(o->istft));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_vocoder_delete(o);
        return status;
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_orca_vocoder_delete)(pv_orca_vocoder_t *object) {
    if (object) {
        pv_buffer_delete(object->buffer_convnext_transposed);
        pv_buffer_delete(object->buffer_conv_pre);
        pv_buffer_delete(object->buffer_backbone);
        pv_buffer_delete(object->buffer_spec_all_subbands);

        pv_orca_istft_delete(object->istft);

        pv_cnn_delete(object->conv_proj);
        pv_vocos_backbone_delete(object->backbone_1);
        pv_vocos_backbone_delete(object->backbone_0);
        pv_convnext_transposed_delete(object->convnext_transposed_1);
        pv_convnext_transposed_delete(object->convnext_transposed_0);
        pv_cnn_delete(object->conv_pre);

        free(object);
    }
}

pv_status_t PV_MOCKABLE(pv_orca_vocoder_forward)(
        pv_orca_vocoder_t *object,
        int32_t n,
        const float *x,
        int16_t *pcm) {
    PV_ASSERT(object);
    PV_ASSERT(n);
    PV_ASSERT(x);
    PV_ASSERT(pcm);
    PV_ORCA_PROFILER_START("vocoder");

    int32_t n_upsampled_0 = pv_convnext_transposed_num_output_frames(object->convnext_transposed_0, n);
    int32_t n_upsampled_1 = pv_convnext_transposed_num_output_frames(object->convnext_transposed_1, n_upsampled_0);

    float *buffer_conv_pre = pv_buffer_get(object->buffer_conv_pre, n, false);
    if (!buffer_conv_pre) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_status_t status = pv_cnn_forward(object->conv_pre, n, x, buffer_conv_pre);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    PV_ORCA_PROFILER_START("gelu");
    pv_activation_gelu_float_approx(n * pv_cnn_output_channels(object->conv_pre), buffer_conv_pre);
    PV_ORCA_PROFILER_STOP("gelu");

    float *buffer_convnext_transposed = pv_buffer_get(object->buffer_convnext_transposed, n_upsampled_1, false);
    if (!buffer_convnext_transposed) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    status = pv_convnext_transposed_forward(
            object->convnext_transposed_0,
            n,
            buffer_conv_pre,
            buffer_convnext_transposed);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_buffer_free(object->buffer_conv_pre);

    float *buffer_backbone = pv_buffer_get(object->buffer_backbone, n_upsampled_1, false);
    if (!buffer_backbone) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    status = pv_vocos_backbone_forward(
            object->backbone_0,
            n_upsampled_0,
            buffer_convnext_transposed,
            buffer_backbone);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    PV_ORCA_PROFILER_START("gelu");
    pv_activation_gelu_float_approx(
            n_upsampled_0 * pv_vocos_backbone_output_channels(object->backbone_0), buffer_backbone);
    PV_ORCA_PROFILER_STOP("gelu");

    status = pv_convnext_transposed_forward(
            object->convnext_transposed_1,
            n_upsampled_0,
            buffer_backbone,
            buffer_convnext_transposed);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    status = pv_vocos_backbone_forward(
            object->backbone_1,
            n_upsampled_1,
            buffer_convnext_transposed,
            buffer_backbone);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_buffer_free(object->buffer_convnext_transposed);

    float *buffer_spec_all_subbands = pv_buffer_get(object->buffer_spec_all_subbands, n_upsampled_1, false);
    if (!buffer_spec_all_subbands) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    status = pv_cnn_forward(object->conv_proj, n_upsampled_1, buffer_backbone, buffer_spec_all_subbands);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }
    pv_buffer_free(object->buffer_backbone);

    status = pv_orca_istft_multiband_forward(object->istft, n_upsampled_1, buffer_spec_all_subbands, pcm);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_buffer_free(object->buffer_spec_all_subbands);

    PV_ORCA_PROFILER_STOP("vocoder");
    return PV_STATUS_SUCCESS;
}
