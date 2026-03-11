#include <stdlib.h>
#include <string.h>

#include "core/pv_error_messages.h"
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

pv_status_t PV_MOCKABLE(pv_orca_vocoder_param_serialize)(
        pv_ypu_t *ypu,
        const pv_orca_vocoder_param_t *param,
        FILE *file) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(file);

    pv_status_t status = pv_cnn_param_serialize(
            ypu,
            param->conv_pre_param,
            file);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_param_serialize,
                pv_status_to_string(status));
        return status;
    }

    status = pv_convnext_transposed_param_serialize(
            ypu,
            param->convnext_transposed_0_param,
            file);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_convnext_transposed_param_serialize,
                pv_status_to_string(status));
        return status;
    }

    status = pv_convnext_transposed_param_serialize(
            ypu,
            param->convnext_transposed_1_param,
            file);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_convnext_transposed_param_serialize,
                pv_status_to_string(status));
        return status;
    }

    status = pv_vocos_backbone_param_serialize(
            ypu,
            param->backbone_0_param,
            file);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_vocos_backbone_param_serialize,
                pv_status_to_string(status));
        return status;
    }

    status = pv_vocos_backbone_param_serialize(
            ypu,
            param->backbone_1_param,
            file);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_vocos_backbone_param_serialize,
                pv_status_to_string(status));
        return status;
    }

    status = pv_cnn_param_serialize(
            ypu,
            param->conv_proj_param,
            file);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_param_serialize,
                pv_status_to_string(status));
        return status;
    }

    size_t count = fwrite(&(param->pcm_normalization_factor), sizeof(float), 1, file);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

    return PV_STATUS_SUCCESS;
}

#endif

pv_status_t PV_MOCKABLE(pv_orca_vocoder_param_load)(
        pv_ypu_t *ypu,
        FILE *f,
        pv_orca_vocoder_param_t **param) {
    PV_ASSERT(ypu);
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_orca_vocoder_param_t *p = pv_ypu_host_alloc(
            ypu,
            sizeof(pv_orca_vocoder_param_t));
    if (!p) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_host_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(p, 0, sizeof(pv_orca_vocoder_param_t));

    pv_status_t status = pv_cnn_param_load(
            ypu,
            f,
            (pv_cnn_param_t **) &(p->conv_pre_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_param_load,
                pv_status_to_string(status));
        pv_orca_vocoder_param_delete(ypu, p);
        return status;
    }

    status = pv_convnext_transposed_param_load(
            ypu,
            f,
            (pv_convnext_transposed_param_t **) &(p->convnext_transposed_0_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_convnext_transposed_param_load,
                pv_status_to_string(status));
        pv_orca_vocoder_param_delete(ypu, p);
        return status;
    }

    status = pv_convnext_transposed_param_load(
            ypu,
            f,
            (pv_convnext_transposed_param_t **) &(p->convnext_transposed_1_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_convnext_transposed_param_load,
                pv_status_to_string(status));
        pv_orca_vocoder_param_delete(ypu, p);
        return status;
    }

    status = pv_vocos_backbone_param_load(
            ypu,
            f,
            (pv_vocos_backbone_param_t **) &(p->backbone_0_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_vocos_backbone_param_load,
                pv_status_to_string(status));
        pv_orca_vocoder_param_delete(ypu, p);
        return status;
    }

    status = pv_vocos_backbone_param_load(
            ypu,
            f,
            (pv_vocos_backbone_param_t **) &(p->backbone_1_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_vocos_backbone_param_load,
                pv_status_to_string(status));
        pv_orca_vocoder_param_delete(ypu, p);
        return status;
    }

    status = pv_cnn_param_load(
            ypu,
            f,
            (pv_cnn_param_t **) &(p->conv_proj_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_param_load,
                pv_status_to_string(status));
        pv_orca_vocoder_param_delete(ypu, p);
        return status;
    }

    size_t count = pv_fread(&(p->pcm_normalization_factor), sizeof(float), 1, f);
    if (count != 1) {
        PV_ERROR_REPORT(
                &pv_error_msg_fread_param_failure,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("count", f));
        pv_orca_vocoder_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }
    if ((p->pcm_normalization_factor <= 0) || (p->pcm_normalization_factor > 1.0)) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p->pcm_normalization_factor"));
        pv_orca_vocoder_param_delete(ypu, p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_orca_vocoder_param_delete)(
        pv_ypu_t *ypu,
        pv_orca_vocoder_param_t *param) {
    PV_ASSERT(ypu);

    if (param) {
        pv_cnn_param_delete(ypu, (pv_cnn_param_t *) (param->conv_proj_param));

        pv_vocos_backbone_param_delete(ypu, (pv_vocos_backbone_param_t *) (param->backbone_1_param));
        pv_vocos_backbone_param_delete(ypu, (pv_vocos_backbone_param_t *) (param->backbone_0_param));

        pv_convnext_transposed_param_delete(ypu, (pv_convnext_transposed_param_t *) (param->convnext_transposed_1_param));
        pv_convnext_transposed_param_delete(ypu, (pv_convnext_transposed_param_t *) (param->convnext_transposed_0_param));

        pv_cnn_param_delete(ypu, (pv_cnn_param_t *) (param->conv_pre_param));

        pv_ypu_host_free(ypu, param);
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

struct pv_orca_vocoder {
    const pv_orca_vocoder_param_t *param;

    pv_cnn_t *conv_pre;
    pv_convnext_transposed_t *convnext_transposed_0;
    pv_convnext_transposed_t *convnext_transposed_1;
    pv_vocos_backbone_t *backbone_0;
    pv_vocos_backbone_t *backbone_1;
    pv_cnn_t *conv_proj;

    pv_orca_istft_t *istft;
};

pv_status_t PV_MOCKABLE(pv_orca_vocoder_init)(
        pv_ypu_t *ypu,
        const pv_orca_vocoder_param_t *param,
        pv_orca_vocoder_t **object) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(object);

    *object = NULL;

    pv_orca_vocoder_t *o = pv_ypu_host_alloc(
            ypu,
            sizeof(pv_orca_vocoder_t));
    if (!o) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_host_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(o, 0, sizeof(pv_orca_vocoder_t));

    o->param = param;

    pv_status_t status = pv_cnn_init(
            ypu,
            param->conv_pre_param,
            &(o->conv_pre));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_init,
                pv_status_to_string(status));
        pv_orca_vocoder_delete(ypu, o);
        return status;
    }

    status = pv_convnext_transposed_init(
            ypu,
            param->convnext_transposed_0_param,
            &(o->convnext_transposed_0));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_convnext_transposed_init,
                pv_status_to_string(status));
        pv_orca_vocoder_delete(ypu, o);
        return status;
    }

    status = pv_convnext_transposed_init(
            ypu,
            param->convnext_transposed_1_param,
            &(o->convnext_transposed_1));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_convnext_transposed_init,
                pv_status_to_string(status));
        pv_orca_vocoder_delete(ypu, o);
        return status;
    }

    status = pv_vocos_backbone_init(
            ypu,
            param->backbone_0_param,
            &(o->backbone_0));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_vocos_backbone_init,
                pv_status_to_string(status));
        pv_orca_vocoder_delete(ypu, o);
        return status;
    }

    status = pv_vocos_backbone_init(
            ypu,
            param->backbone_1_param,
            &(o->backbone_1));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_vocos_backbone_init,
                pv_status_to_string(status));
        pv_orca_vocoder_delete(ypu, o);
        return status;
    }

    status = pv_cnn_init(
            ypu,
            param->conv_proj_param,
            &(o->conv_proj));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_init,
                pv_status_to_string(status));
        pv_orca_vocoder_delete(ypu, o);
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
        pv_orca_vocoder_delete(ypu, o);
        return status;
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_orca_vocoder_delete)(
        pv_ypu_t *ypu,
        pv_orca_vocoder_t *object) {
    PV_ASSERT(ypu);

    if (object) {
        pv_orca_istft_delete(object->istft);

        pv_cnn_delete(ypu, object->conv_proj);
        pv_vocos_backbone_delete(ypu, object->backbone_1);
        pv_vocos_backbone_delete(ypu, object->backbone_0);
        pv_convnext_transposed_delete(ypu, object->convnext_transposed_1);
        pv_convnext_transposed_delete(ypu, object->convnext_transposed_0);
        pv_cnn_delete(ypu, object->conv_pre);

        pv_ypu_host_free(ypu, object);
    }
}

pv_status_t PV_MOCKABLE(pv_orca_vocoder_forward)(
        pv_ypu_t *ypu,
        pv_orca_vocoder_t *object,
        int32_t n,
        pv_ypu_mem_t *x_ypu,
        int16_t *pcm,
        int32_t x_offset) {
    PV_ASSERT(ypu);
    PV_ASSERT(object);
    PV_ASSERT(n);
    PV_ASSERT(x_ypu);
    PV_ASSERT(pcm);

    PV_ORCA_PROFILER_START("\torca_vocoder");

    int32_t n_upsampled_0 = pv_convnext_transposed_num_output_frames(object->convnext_transposed_0, n);
    int32_t n_upsampled_1 = pv_convnext_transposed_num_output_frames(object->convnext_transposed_1, n_upsampled_0);

    const int32_t num_channels_conv_pre = object->param->conv_pre_param->output_channels;
    pv_ypu_mem_t *buffer_conv_pre_ypu = pv_ypu_buffer_get(
            ypu,
            num_channels_conv_pre * n * ((int32_t) sizeof(float)),
            false);
    if (!buffer_conv_pre_ypu) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("buffer_conv_pre_ypu"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_status_t status = pv_cnn_forward(
            ypu,
            object->conv_pre,
            n,
            x_ypu,
            buffer_conv_pre_ypu,
            x_offset,
            0);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_forward,
                pv_status_to_string(status));
        pv_ypu_buffer_release(ypu, buffer_conv_pre_ypu);
        return status;
    }

    PV_ORCA_PROFILER_START("\t\tvocoder_gelu");
    pv_ypu_op_elementwise_args_t gelu_args0 = {
            .output = buffer_conv_pre_ypu,
            .input = buffer_conv_pre_ypu,
            .length = n * pv_cnn_output_channels(object->conv_pre),
            .output_offset = 0,
            .input_offset = 0
    };
    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_GELU_APPROX,
            &gelu_args0);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_GELU_APPROX),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        pv_ypu_buffer_release(ypu, buffer_conv_pre_ypu);
        return status;
    }
    PV_ORCA_PROFILER_STOP("\t\tvocoder_gelu");

    const int32_t num_channels_convnext_transposed = object->param->convnext_transposed_1_param->conv_2_param->output_channels;
    pv_ypu_mem_t *buffer_convnext_transposed_ypu = pv_ypu_buffer_get(
            ypu,
            num_channels_convnext_transposed * n_upsampled_1 * ((int32_t) sizeof(float)),
            false);
    if (!buffer_convnext_transposed_ypu) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("buffer_convnext_transposed_ypu"));
        pv_ypu_buffer_release(ypu, buffer_conv_pre_ypu);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    status = pv_convnext_transposed_forward(
            ypu,
            object->convnext_transposed_0,
            n,
            buffer_conv_pre_ypu,
            buffer_convnext_transposed_ypu);
    pv_ypu_buffer_release(ypu, buffer_conv_pre_ypu);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_convnext_transposed_forward,
                pv_status_to_string(status));
        pv_ypu_buffer_release(ypu, buffer_convnext_transposed_ypu);
        return status;
    }

    const int32_t num_channels_backbone = object->param->backbone_1_param->layer_norm_pre_param->num_channels;
    pv_ypu_mem_t *buffer_backbone_ypu = pv_ypu_buffer_get(
            ypu,
            num_channels_backbone * n_upsampled_1 * ((int32_t) sizeof(float)),
            false);
    if (!buffer_backbone_ypu) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("buffer_backbone_ypu"));
        pv_ypu_buffer_release(ypu, buffer_convnext_transposed_ypu);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    status = pv_vocos_backbone_forward(
            ypu,
            object->backbone_0,
            n_upsampled_0,
            buffer_convnext_transposed_ypu,
            buffer_backbone_ypu);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_vocos_backbone_forward,
                pv_status_to_string(status));
        pv_ypu_buffer_release(ypu, buffer_backbone_ypu);
        pv_ypu_buffer_release(ypu, buffer_convnext_transposed_ypu);
        return status;
    }

    PV_ORCA_PROFILER_START("\t\tvocoder_gelu");
    pv_ypu_op_elementwise_args_t gelu_args1 = {
            .output = buffer_backbone_ypu,
            .input = buffer_backbone_ypu,
            .length = n_upsampled_0 * pv_vocos_backbone_output_channels(object->backbone_0),
            .output_offset = 0,
            .input_offset = 0
    };
    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_GELU_APPROX,
            &gelu_args1);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_GELU_APPROX),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        pv_ypu_buffer_release(ypu, buffer_backbone_ypu);
        pv_ypu_buffer_release(ypu, buffer_convnext_transposed_ypu);
        return status;
    }
    PV_ORCA_PROFILER_STOP("\t\tvocoder_gelu");

    status = pv_convnext_transposed_forward(
            ypu,
            object->convnext_transposed_1,
            n_upsampled_0,
            buffer_backbone_ypu,
            buffer_convnext_transposed_ypu);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_convnext_transposed_forward,
                pv_status_to_string(status));
        pv_ypu_buffer_release(ypu, buffer_backbone_ypu);
        pv_ypu_buffer_release(ypu, buffer_convnext_transposed_ypu);
        return status;
    }

    status = pv_vocos_backbone_forward(
            ypu,
            object->backbone_1,
            n_upsampled_1,
            buffer_convnext_transposed_ypu,
            buffer_backbone_ypu);
    pv_ypu_buffer_release(ypu, buffer_convnext_transposed_ypu);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_vocos_backbone_forward,
                pv_status_to_string(status));
        pv_ypu_buffer_release(ypu, buffer_backbone_ypu);
        return status;
    }

    const int32_t num_channels_spec = pv_cnn_output_channels(object->conv_proj);
    pv_ypu_mem_t *buffer_spec_all_subbands_ypu = pv_ypu_buffer_get(
            ypu,
            num_channels_spec * n_upsampled_1 * ((int32_t) sizeof(float)),
            false);
    if (!buffer_spec_all_subbands_ypu) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("buffer_spec_all_subbands_ypu"));
        pv_ypu_buffer_release(ypu, buffer_backbone_ypu);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    status = pv_cnn_forward(
            ypu,
            object->conv_proj,
            n_upsampled_1,
            buffer_backbone_ypu,
            buffer_spec_all_subbands_ypu,
            0,
            0);
    pv_ypu_buffer_release(ypu, buffer_backbone_ypu);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_forward,
                pv_status_to_string(status));
        pv_ypu_buffer_release(ypu, buffer_spec_all_subbands_ypu);
        return status;
    }

    float *buffer_spec_all_subbands = ((float *) pv_ypu_mem_get_host_view(ypu, buffer_spec_all_subbands_ypu, true));
    status = pv_orca_istft_multiband_forward(
            object->istft,
            n_upsampled_1,
            buffer_spec_all_subbands,
            pcm);
    pv_ypu_buffer_release(ypu, buffer_spec_all_subbands_ypu);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_orca_istft_multiband_forward,
                pv_status_to_string(status));
        return status;
    }
    pv_ypu_mem_release_host_view(ypu, buffer_spec_all_subbands_ypu, false);

    PV_ORCA_PROFILER_STOP("\torca_vocoder");

    return PV_STATUS_SUCCESS;
}
