#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "core/pv_error_messages.h"
#include "orca/pv_cnn.h"
#include "orca/pv_orca_lfm_condition_fuser.h"
#include "orca/pv_profiler.h"
#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

struct pv_orca_lfm_condition_fuser {
    const pv_orca_lfm_condition_fuser_param_t *param;

    pv_cnn_t *conv_1;
    pv_cnn_t *conv_2;
};

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_orca_lfm_condition_fuser_param_serialize)(
        pv_ypu_t *ypu,
        const pv_orca_lfm_condition_fuser_param_t *param,
        FILE *file) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(file);

    pv_status_t status = pv_cnn_param_serialize(
            ypu,
            param->conv_1_param,
            file);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_param_serialize,
                pv_status_to_string(status));
        return status;
    }

    status = pv_cnn_param_serialize(
            ypu,
            param->conv_2_param,
            file);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_param_serialize,
                pv_status_to_string(status));
        return status;
    }

    return PV_STATUS_SUCCESS;
}

#endif

pv_status_t PV_MOCKABLE(pv_orca_lfm_condition_fuser_param_load)(
        pv_ypu_t *ypu,
        FILE *f,
        pv_orca_lfm_condition_fuser_param_t **param) {
    PV_ASSERT(ypu);
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_orca_lfm_condition_fuser_param_t *p = pv_ypu_host_alloc(
            ypu,
            sizeof(pv_orca_lfm_condition_fuser_param_t));
    if (!p) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_host_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(p, 0, sizeof(pv_orca_lfm_condition_fuser_param_t));

    pv_status_t status = pv_cnn_param_load(
            ypu,
            f,
            (pv_cnn_param_t **) &(p->conv_1_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_param_load,
                pv_status_to_string(status));
        pv_orca_lfm_condition_fuser_param_delete(ypu, p);
        return status;
    }

    status = pv_cnn_param_load(
            ypu,
            f,
            (pv_cnn_param_t **) &(p->conv_2_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_param_load,
                pv_status_to_string(status));
        pv_orca_lfm_condition_fuser_param_delete(ypu, p);
        return status;
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_orca_lfm_condition_fuser_param_delete)(
        pv_ypu_t *ypu,
        pv_orca_lfm_condition_fuser_param_t *param) {
    PV_ASSERT(ypu);

    if (param) {
        pv_cnn_param_delete(ypu, (pv_cnn_param_t *) (param->conv_2_param));

        pv_cnn_param_delete(ypu, (pv_cnn_param_t *) (param->conv_1_param));

        pv_ypu_host_free(ypu, param);
    }
}

bool PV_MOCKABLE(pv_orca_lfm_condition_fuser_param_is_equal)(
        const pv_orca_lfm_condition_fuser_param_t *object,
        const pv_orca_lfm_condition_fuser_param_t *other) {
    PV_ASSERT(object);
    PV_ASSERT(other);

    if (!pv_cnn_param_is_equal(
                object->conv_1_param,
                other->conv_1_param)) {
        return false;
    }

    if (!pv_cnn_param_is_equal(
                object->conv_2_param,
                other->conv_2_param)) {
        return false;
    }

    return true;
}

pv_status_t PV_MOCKABLE(pv_orca_lfm_condition_fuser_init)(
        pv_ypu_t *ypu,
        const pv_orca_lfm_condition_fuser_param_t *param,
        pv_orca_lfm_condition_fuser_t **object) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(object);

    *object = NULL;

    pv_orca_lfm_condition_fuser_t *o = pv_ypu_host_alloc(
            ypu,
            sizeof(pv_orca_lfm_condition_fuser_t));
    if (!o) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_host_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(o, 0, sizeof(pv_orca_lfm_condition_fuser_t));

    o->param = param;

    pv_status_t status = pv_cnn_init(
            ypu,
            param->conv_1_param,
            &(o->conv_1));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_init,
                pv_status_to_string(status));
        pv_orca_lfm_condition_fuser_delete(ypu, o);
        return status;
    }

    status = pv_cnn_init(
            ypu,
            param->conv_2_param,
            &(o->conv_2));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_init,
                pv_status_to_string(status));
        pv_orca_lfm_condition_fuser_delete(ypu, o);
        return status;
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_orca_lfm_condition_fuser_delete)(
        pv_ypu_t *ypu,
        pv_orca_lfm_condition_fuser_t *object) {
    PV_ASSERT(ypu);

    if (object) {
        pv_cnn_delete(ypu, object->conv_2);
        pv_cnn_delete(ypu, object->conv_1);

        pv_ypu_host_free(ypu, object);
    }
}

pv_status_t PV_MOCKABLE(pv_orca_lfm_condition_fuser_forward)(
        pv_ypu_t *ypu,
        pv_orca_lfm_condition_fuser_t *object,
        int32_t n,
        pv_ypu_mem_t *content_condition_ypu,
        pv_ypu_mem_t *time_condition_ypu,
        pv_ypu_mem_t *fused_condition_ypu,
        int32_t content_condition_offset,
        int32_t time_condition_offset,
        int32_t fused_condition_offset) {
    PV_ASSERT(ypu);
    PV_ASSERT(object);
    PV_ASSERT(n > 0);
    PV_ASSERT(content_condition_ypu);
    PV_ASSERT(time_condition_ypu);
    PV_ASSERT(fused_condition_ypu);

    PV_ORCA_PROFILER_START("\torca_lfm_condition_fuser_forward");

    const int32_t input_channels = object->param->conv_1_param->input_channels;
    const int32_t content_condition_channels = input_channels / 2;
    const int32_t time_condition_channels = input_channels / 2;

    pv_ypu_mem_t *buffer_ypu = pv_ypu_buffer_get(
            ypu,
            n * input_channels * (int32_t) sizeof(float),
            false);
    if (!buffer_ypu) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("buffer_ypu"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    for (int32_t i = 0; i < n; i++) {
        const int32_t i_offset_fused = i * input_channels;

        pv_ypu_op_memcpy_args_t memcpy_args0 = {
                .output = buffer_ypu,
                .input = content_condition_ypu,
                .size_bytes = content_condition_channels * (int32_t) sizeof(float),
                .output_offset = i_offset_fused * (int32_t) sizeof(float),
                .input_offset = content_condition_offset + (i * content_condition_channels * (int32_t) sizeof(float)),
        };
        pv_status_t status = pv_ypu_operator_execute(
                ypu,
                PV_YPU_OPERATOR_MEMCPY,
                &memcpy_args0);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT(
                    &pv_error_msg_ypu_operator_fail,
                    PV_ERROR_ARGS_PUBLIC("execute"),
                    PV_ERROR_ARGS_PRIVATE(
                            "execute",
                            pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_MEMCPY),
                            pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                            pv_status_to_string(status)));
            return status;
        }

        pv_ypu_op_memcpy_args_t memcpy_args1 = {
                .output = buffer_ypu,
                .input = time_condition_ypu,
                .size_bytes = time_condition_channels * (int32_t) sizeof(float),
                .output_offset = (i_offset_fused + content_condition_channels) * (int32_t) sizeof(float),
                .input_offset = time_condition_offset,
        };
        status = pv_ypu_operator_execute(
                ypu,
                PV_YPU_OPERATOR_MEMCPY,
                &memcpy_args1);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT(
                    &pv_error_msg_ypu_operator_fail,
                    PV_ERROR_ARGS_PUBLIC("execute"),
                    PV_ERROR_ARGS_PRIVATE(
                            "execute",
                            pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_MEMCPY),
                            pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                            pv_status_to_string(status)));
            return status;
        }
    }
    
    const int32_t num_intermediate_channels = object->param->conv_1_param->output_channels;
    pv_ypu_mem_t *buffer_intermediate_ypu = pv_ypu_buffer_get(
            ypu,
            n * num_intermediate_channels * (int32_t) sizeof(float),
            false);
    if (!buffer_intermediate_ypu) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("buffer_intermediate_ypu"));
        pv_ypu_buffer_release(ypu, buffer_ypu);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_status_t status = pv_cnn_forward(
            ypu,
            object->conv_1,
            n,
            buffer_ypu,
            buffer_intermediate_ypu,
            0,
            0);
    pv_ypu_buffer_release(ypu, buffer_ypu);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_forward,
                pv_status_to_string(status));
        pv_ypu_buffer_release(ypu, buffer_intermediate_ypu);
        return status;
    }

    pv_ypu_op_elementwise_args_t silu_args = {
            .output = buffer_intermediate_ypu,
            .input = buffer_intermediate_ypu,
            .length = n * num_intermediate_channels,
            .output_offset = 0,
            .input_offset = 0
    };
    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_SILU,
            &silu_args);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_SILU),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        pv_ypu_buffer_release(ypu, buffer_intermediate_ypu);
        return status;
    }

    status = pv_cnn_forward(
            ypu,
            object->conv_2,
            n,
            buffer_intermediate_ypu,
            fused_condition_ypu,
            0,
            fused_condition_offset);
    pv_ypu_buffer_release(ypu, buffer_intermediate_ypu);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_forward,
                pv_status_to_string(status));
        return status;
    }

    PV_ORCA_PROFILER_STOP("\torca_lfm_condition_fuser_forward");

    return PV_STATUS_SUCCESS;
}
