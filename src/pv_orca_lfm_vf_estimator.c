#include <stdlib.h>
#include <string.h>

#include "core/pv_error_messages.h"
#include "core/pv_type.h"
#include "orca/pv_affine.h"
#include "orca/pv_cnn.h"
#include "orca/pv_convnext_film.h"
#include "orca/pv_layer_norm.h"
#include "orca/pv_orca_lfm_vf_estimator.h"
#include "orca/pv_orca_lfm_vf_estimator_param.h"
#include "orca/pv_orca_stream_state.h"

#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

#define FILM_SHARING_FACTOR (4)
#define NUM_GATES (3 * FILM_SHARING_FACTOR + 2)

struct pv_orca_lfm_vf_estimator {
    const pv_orca_lfm_vf_estimator_param_t *param;

    pv_cnn_t *conv_pre;
    pv_cnn_t *adanorm_linear;
    pv_convnext_film_t **convnext_blocks;
    pv_layer_norm_t *layer_norm_out;
    pv_cnn_t *conv_out;
};

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_orca_lfm_vf_estimator_param_serialize)(
        pv_ypu_t *ypu,
        const pv_orca_lfm_vf_estimator_param_t *param,
        FILE *file) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(file);

    size_t count = fwrite(&(param->num_blocks), sizeof(int32_t), 1, file);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

    count = fwrite(&(param->dimension), sizeof(int32_t), 1, file);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

    count = fwrite(&(param->out_dimension), sizeof(int32_t), 1, file);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

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

    status = pv_cnn_param_serialize(
            ypu,
            param->adanorm_linear_param,
            file);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_param_serialize,
                pv_status_to_string(status));
        return status;
    }

    for (int32_t i = 0; i < param->num_blocks; i++) {
        status = pv_convnext_film_param_serialize(
                ypu,
                param->convnext_blocks_param[i],
                file);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_convnext_film_param_serialize,
                    pv_status_to_string(status));
            return status;
        }
    }

    status = pv_layer_norm_param_serialize(
            ypu,
            param->layer_norm_out_param,
            false,
            file);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_layer_norm_param_serialize,
                pv_status_to_string(status));
        return status;
    }

    status = pv_cnn_param_serialize(
            ypu,
            param->conv_out_param,
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

pv_status_t PV_MOCKABLE(pv_orca_lfm_vf_estimator_param_load)(
        pv_ypu_t *ypu,
        FILE *f,
        pv_orca_lfm_vf_estimator_param_t **param) {
    PV_ASSERT(ypu);
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_orca_lfm_vf_estimator_param_t *p = pv_ypu_host_alloc(
            ypu,
            sizeof(pv_orca_lfm_vf_estimator_param_t));
    if (!p) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_host_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(p, 0, sizeof(pv_orca_lfm_vf_estimator_param_t));

    size_t count = pv_fread(&(p->num_blocks), sizeof(int32_t), 1, f);
    if (count != 1) {
        PV_ERROR_REPORT(
                &pv_error_msg_fread_param_failure,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("count", f));
        pv_orca_lfm_vf_estimator_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->num_blocks <= 0) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p->num_blocks"));
        pv_orca_lfm_vf_estimator_param_delete(ypu, p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    count = pv_fread(&(p->dimension), sizeof(int32_t), 1, f);
    if (count != 1) {
        PV_ERROR_REPORT(
                &pv_error_msg_fread_param_failure,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("count", f));
        pv_orca_lfm_vf_estimator_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->dimension <= 0) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p->dimension"));
        pv_orca_lfm_vf_estimator_param_delete(ypu, p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    count = pv_fread(&(p->out_dimension), sizeof(int32_t), 1, f);
    if (count != 1) {
        PV_ERROR_REPORT(
                &pv_error_msg_fread_param_failure,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("count", f));
        pv_orca_lfm_vf_estimator_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->out_dimension <= 0) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p->out_dimension"));
        pv_orca_lfm_vf_estimator_param_delete(ypu, p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    pv_status_t status = pv_cnn_param_load(
            ypu,
            f,
            (pv_cnn_param_t **) &(p->conv_pre_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_param_load,
                pv_status_to_string(status));
        pv_orca_lfm_vf_estimator_param_delete(ypu, p);
        return status;
    }

    status = pv_cnn_param_load(
            ypu,
            f,
            (pv_cnn_param_t **) &(p->adanorm_linear_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_param_load,
                pv_status_to_string(status));
        pv_orca_lfm_vf_estimator_param_delete(ypu, p);
        return status;
    }

    p->convnext_blocks_param = pv_ypu_host_alloc(
            ypu,
            p->num_blocks * ((int32_t) sizeof(pv_convnext_film_param_t *)));
    if (!p->convnext_blocks_param) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_host_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p->convnext_blocks_param"));
        pv_orca_lfm_vf_estimator_param_delete(ypu, p);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(p->convnext_blocks_param, 0, p->num_blocks * sizeof(pv_convnext_film_param_t *));

    for (int32_t i = 0; i < p->num_blocks; i++) {
        status = pv_convnext_film_param_load(
                ypu,
                f,
                (pv_convnext_film_param_t **) &(p->convnext_blocks_param[i]));
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_convnext_film_param_load,
                    pv_status_to_string(status));
            pv_orca_lfm_vf_estimator_param_delete(ypu, p);
            return status;
        }
    }

    status = pv_layer_norm_param_load(
            ypu,
            f,
            false,
            (pv_layer_norm_param_t **) &(p->layer_norm_out_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_layer_norm_param_load,
                pv_status_to_string(status));
        pv_orca_lfm_vf_estimator_param_delete(ypu, p);
        return status;
    }

    status = pv_cnn_param_load(
            ypu,
            f,
            (pv_cnn_param_t **) &(p->conv_out_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_param_load,
                pv_status_to_string(status));
        pv_orca_lfm_vf_estimator_param_delete(ypu, p);
        return status;
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_orca_lfm_vf_estimator_param_delete)(
        pv_ypu_t *ypu,
        pv_orca_lfm_vf_estimator_param_t *param) {
    PV_ASSERT(ypu);

    if (param) {
        pv_cnn_param_delete(ypu, (pv_cnn_param_t *) (param->conv_out_param));

        pv_layer_norm_param_delete(ypu, (pv_layer_norm_param_t *) (param->layer_norm_out_param));

        if (param->convnext_blocks_param) {
            for (int32_t i = param->num_blocks - 1; i >= 0; --i) {
                pv_convnext_film_param_delete(ypu, (pv_convnext_film_param_t *) param->convnext_blocks_param[i]);
            }

            pv_ypu_host_free(ypu, param->convnext_blocks_param);
        }

        pv_cnn_param_delete(ypu, (pv_cnn_param_t *) (param->adanorm_linear_param));

        pv_cnn_param_delete(ypu, (pv_cnn_param_t *) (param->conv_pre_param));

        pv_ypu_host_free(ypu, param);
    }
}

bool PV_MOCKABLE(pv_orca_lfm_vf_estimator_param_is_equal)(
        const pv_orca_lfm_vf_estimator_param_t *object,
        const pv_orca_lfm_vf_estimator_param_t *other) {
    PV_ASSERT(object);
    PV_ASSERT(other);

    if (object->num_blocks != other->num_blocks) {
        return false;
    }

    if (object->dimension != other->dimension) {
        return false;
    }

    if (object->out_dimension != other->out_dimension) {
        return false;
    }

    if (!pv_cnn_param_is_equal(object->conv_pre_param, other->conv_pre_param)) {
        return false;
    }

    if (!pv_cnn_param_is_equal(object->adanorm_linear_param, other->adanorm_linear_param)) {
        return false;
    }

    for (int32_t i = 0; i < object->num_blocks; i++) {
        if (!pv_convnext_film_param_is_equal(object->convnext_blocks_param[i], other->convnext_blocks_param[i])) {
            return false;
        }
    }

    if (!pv_layer_norm_param_is_equal(object->layer_norm_out_param, other->layer_norm_out_param)) {
        return false;
    }

    if (!pv_cnn_param_is_equal(object->conv_out_param, other->conv_out_param)) {
        return false;
    }

    return true;
}

pv_status_t PV_MOCKABLE(pv_orca_lfm_vf_estimator_init)(
        pv_ypu_t *ypu,
        const pv_orca_lfm_vf_estimator_param_t *param,
        pv_orca_lfm_vf_estimator_t **object) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(object);

    *object = NULL;

    pv_orca_lfm_vf_estimator_t *o = pv_ypu_host_alloc(
            ypu,
            sizeof(pv_orca_lfm_vf_estimator_t));
    if (!o) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_host_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(o, 0, sizeof(pv_orca_lfm_vf_estimator_t));

    o->param = param;

    pv_status_t status = pv_cnn_init(
            ypu,
            param->conv_pre_param,
            &(o->conv_pre),
            false);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_init,
                pv_status_to_string(status));
        pv_orca_lfm_vf_estimator_delete(ypu, o);
        return status;
    }

    status = pv_cnn_init(
            ypu,
            param->adanorm_linear_param,
            &(o->adanorm_linear),
            false);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_init,
                pv_status_to_string(status));
        pv_orca_lfm_vf_estimator_delete(ypu, o);
        return status;
    }

    o->convnext_blocks = pv_ypu_host_alloc(
            ypu,
            param->num_blocks * ((int32_t) sizeof(pv_convnext_film_t *)));
    if (!(o->convnext_blocks)) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_host_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o->convnext_blocks"));
        pv_orca_lfm_vf_estimator_delete(ypu, o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(o->convnext_blocks, 0, param->num_blocks * sizeof(pv_convnext_film_t *));

    for (int32_t i = 0; i < param->num_blocks; i++) {
        status = pv_convnext_film_init(
                ypu,
                param->convnext_blocks_param[i],
                &(o->convnext_blocks[i]));
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_convnext_film_init,
                    pv_status_to_string(status));
            pv_orca_lfm_vf_estimator_delete(ypu, o);
            return status;
        }
    }

    status = pv_layer_norm_init(
            ypu,
            param->layer_norm_out_param,
            &(o->layer_norm_out));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_layer_norm_init,
                pv_status_to_string(status));
        pv_orca_lfm_vf_estimator_delete(ypu, o);
        return status;
    }

    status = pv_cnn_init(
            ypu,
            param->conv_out_param,
            &(o->conv_out),
            false);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_init,
                pv_status_to_string(status));
        pv_orca_lfm_vf_estimator_delete(ypu, o);
        return status;
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_orca_lfm_vf_estimator_delete)(
        pv_ypu_t *ypu,
        pv_orca_lfm_vf_estimator_t *object) {
    PV_ASSERT(ypu);

    if (object) {
        pv_cnn_delete(ypu, object->conv_out);

        pv_layer_norm_delete(ypu, object->layer_norm_out);

        if (object->convnext_blocks) {
            for (int32_t i = object->param->num_blocks - 1; i >= 0; --i) {
                pv_convnext_film_delete(ypu, object->convnext_blocks[i]);
            }

            pv_ypu_host_free(ypu, object->convnext_blocks);
        }

        pv_cnn_delete(ypu, object->adanorm_linear);

        pv_cnn_delete(ypu, object->conv_pre);

        pv_ypu_host_free(ypu, object);
    }
}

pv_status_t PV_MOCKABLE(pv_orca_lfm_vf_estimator_forward)(
        pv_ypu_t *ypu,
        pv_orca_lfm_vf_estimator_t *object,
        pv_orca_stream_state_t *state,
        int32_t n,
        pv_ypu_mem_t *x_ypu,
        pv_ypu_mem_t *c_ypu,
        pv_ypu_mem_t *y_ypu,
        int32_t x_offset,
        int32_t c_offset,
        int32_t y_offset) {
    PV_ASSERT(object);
    PV_ASSERT(n > 0);
    PV_ASSERT(x_ypu);
    PV_ASSERT(c_ypu);
    PV_ASSERT(y_ypu);

    const int32_t dimension = object->param->dimension;

    pv_ypu_mem_t *buffer_conv_pre_ypu = pv_ypu_buffer_get(
            ypu,
            n * dimension * (int32_t) sizeof(float),
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

    pv_ypu_mem_t *buffer_c_ypu = pv_ypu_buffer_get(
            ypu,
            n * dimension * (int32_t) sizeof(float),
            false);
    if (!buffer_c_ypu) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("buffer_c_ypu"));
        pv_ypu_buffer_release(ypu, buffer_conv_pre_ypu);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    // WARNING: Below <buffer_adanorm_linear_total> can technically be broken up into chunks,
    // that is, we don't have to compute the entirety of it in one go!
    // If it causes memory issue, then we should break it apart and only compute what's needed at a time.
    pv_ypu_mem_t *buffer_adanorm_linear_total_ypu = pv_ypu_buffer_get(
            ypu,
            n * object->param->adanorm_linear_param->output_channels * (int32_t) sizeof(float),
            false);
    if (!buffer_adanorm_linear_total_ypu) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("buffer_adanorm_linear_total_ypu"));
        pv_ypu_buffer_release(ypu, buffer_c_ypu);
        pv_ypu_buffer_release(ypu, buffer_conv_pre_ypu);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_op_elementwise_args_t silu_args = {
            .output = buffer_c_ypu,
            .input = c_ypu,
            .length = n * dimension,
            .output_offset = 0,
            .input_offset = c_offset,
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
        pv_ypu_buffer_release(ypu, buffer_adanorm_linear_total_ypu);
        pv_ypu_buffer_release(ypu, buffer_c_ypu);
        pv_ypu_buffer_release(ypu, buffer_conv_pre_ypu);
        return status;
    }

    status = pv_cnn_forward(
            ypu,
            object->adanorm_linear,
            n,
            buffer_c_ypu,
            buffer_adanorm_linear_total_ypu,
            0,
            0);
    pv_ypu_buffer_release(ypu, buffer_c_ypu);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_forward,
                pv_status_to_string(status));
        pv_ypu_buffer_release(ypu, buffer_adanorm_linear_total_ypu);
        pv_ypu_buffer_release(ypu, buffer_conv_pre_ypu);
        return status;
    }

    pv_ypu_mem_t *buffer_gates_list_ypu = pv_ypu_buffer_get(
            ypu,
            n * NUM_GATES * dimension * (int32_t) sizeof(float),
            false);
    if (!buffer_gates_list_ypu) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("buffer_gates_list_ypu"));
        pv_ypu_buffer_release(ypu, buffer_adanorm_linear_total_ypu);
        pv_ypu_buffer_release(ypu, buffer_conv_pre_ypu);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_op_transpose_args_t transpose_args = {
            .output = buffer_gates_list_ypu,
            .input = buffer_adanorm_linear_total_ypu,
            .b = 1,
            .m = NUM_GATES,
            .n = n,
            .k = dimension * (int32_t) sizeof(float),
            .output_offset = 0,
            .input_offset = 0,
    };
    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_TRANSPOSE,
            &transpose_args);
    pv_ypu_buffer_release(ypu, buffer_adanorm_linear_total_ypu);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_TRANSPOSE),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        pv_ypu_buffer_release(ypu, buffer_gates_list_ypu);
        pv_ypu_buffer_release(ypu, buffer_conv_pre_ypu);
        return status;
    }

    for (int32_t i = 0; i < object->param->num_blocks; i++) {
        const int32_t i_grouped = i / FILM_SHARING_FACTOR;

        int32_t n_i = n;
        int32_t offset_i = 0;
        if (state->status == PV_ORCA_STREAM_STATUS_ACTIVE) {
            const int32_t lookahead_lookback_i = 2 * i;
            n_i -= (pv_min_int32(lookahead_lookback_i, state->T_domain_garbage_lookback_offset) +
                    pv_min_int32(lookahead_lookback_i, state->T_domain_garbage_lookahead_offset));
            offset_i = (pv_min_int32(lookahead_lookback_i, state->T_domain_garbage_lookback_offset)) * object->param->dimension;
        }

        status = pv_convnext_film_forward(
                ypu,
                object->convnext_blocks[i],
                n_i,
                buffer_conv_pre_ypu,
                buffer_gates_list_ypu,
                buffer_gates_list_ypu,
                buffer_gates_list_ypu,
                buffer_conv_pre_ypu,
                offset_i * (int32_t) sizeof(float),
                ((i_grouped * 3) * n * dimension + offset_i) * (int32_t) sizeof(float),
                ((i_grouped * 3 + 1) * n * dimension + offset_i) * (int32_t) sizeof(float),
                ((i_grouped * 3 + 2) * n * dimension + offset_i) * (int32_t) sizeof(float),
                offset_i * (int32_t) sizeof(float));
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_convnext_film_forward,
                    pv_status_to_string(status));
            pv_ypu_buffer_release(ypu, buffer_gates_list_ypu);
            pv_ypu_buffer_release(ypu, buffer_conv_pre_ypu);
            return status;
        }
    }

    pv_ypu_op_layer_norm_non_affine_args_t layer_norm_args = {
            .output = buffer_conv_pre_ypu,
            .input = buffer_conv_pre_ypu,
            .m = n,
            .n = object->param->layer_norm_out_param->num_channels,
            .eps = object->param->layer_norm_out_param->eps,
            .output_offset = 0,
            .input_offset = 0,
    };
    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_LAYER_NORM_NON_AFFINE,
            &layer_norm_args);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_LAYER_NORM_NON_AFFINE),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        pv_ypu_buffer_release(ypu, buffer_gates_list_ypu);
        pv_ypu_buffer_release(ypu, buffer_conv_pre_ypu);
        return status;
    }

    status = pv_affine_execute(
            ypu,
            1,
            n * object->param->layer_norm_out_param->num_channels,
            buffer_conv_pre_ypu,
            1.0f,
            1.0f,
            buffer_gates_list_ypu,
            buffer_gates_list_ypu,
            buffer_conv_pre_ypu,
            0,
            (NUM_GATES - 2) * n * dimension * (int32_t) sizeof(float),
            (NUM_GATES - 1) * n * dimension * (int32_t) sizeof(float),
            0);
    pv_ypu_buffer_release(ypu, buffer_gates_list_ypu);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_affine_execute,
                pv_status_to_string(status));
        pv_ypu_buffer_release(ypu, buffer_conv_pre_ypu);
        return status;
    }

    status = pv_cnn_forward(
            ypu,
            object->conv_out,
            n,
            buffer_conv_pre_ypu,
            y_ypu,
            0,
            y_offset);
    pv_ypu_buffer_release(ypu, buffer_conv_pre_ypu);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_forward,
                pv_status_to_string(status));
        return status;
    }

    return PV_STATUS_SUCCESS;
}
