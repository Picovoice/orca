#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "core/pv_error_messages.h"
#include "orca/pv_cnn.h"
#include "orca/pv_orca_util.h"
#include "orca/pv_profiler.h"
#include "orca/pv_rope_attention.h"
#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_rope_attention_param_serialize)(
        pv_ypu_t *ypu,
        const pv_rope_attention_param_t *param,
        FILE *file) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(file);

    size_t count = fwrite(&(param->dimension), sizeof(int32_t), 1, file);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

    count = fwrite(&(param->head_dimension), sizeof(int32_t), 1, file);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

    count = fwrite(&(param->num_heads), sizeof(int32_t), 1, file);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

    count = fwrite(&(param->ffn_intermediate_dimension), sizeof(int32_t), 1, file);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

    count = fwrite(&(param->num_lookaheads), sizeof(int32_t), 1, file);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

    count = fwrite(&(param->num_lookbacks), sizeof(int32_t), 1, file);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

    count = fwrite(&(param->rope_base), sizeof(float), 1, file);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

    count = fwrite(&(param->sdpa_downsample_factor), sizeof(int32_t), 1, file);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

    pv_status_t status = pv_cnn_param_serialize(
            ypu,
            param->conv_q_param,
            file);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_param_serialize,
                pv_status_to_string(status));
        return status;
    }

    status = pv_cnn_param_serialize(
            ypu,
            param->conv_k_param,
            file);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_param_serialize,
                pv_status_to_string(status));
        return status;
    }

    status = pv_cnn_param_serialize(
            ypu,
            param->conv_v_param,
            file);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_param_serialize,
                pv_status_to_string(status));
        return status;
    }

    status = pv_cnn_param_serialize(
            ypu,
            param->conv_o_param,
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

pv_status_t PV_MOCKABLE(pv_rope_attention_param_load)(
        pv_ypu_t *ypu,
        FILE *f,
        pv_rope_attention_param_t **param) {
    PV_ASSERT(ypu);
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_rope_attention_param_t *p = pv_ypu_host_alloc(
            ypu,
            sizeof(pv_rope_attention_param_t));
    if (!p) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_host_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(p, 0, sizeof(pv_rope_attention_param_t));

    size_t count = pv_fread(&(p->dimension), sizeof(int32_t), 1, f);
    if (count != 1) {
        PV_ERROR_REPORT(
                &pv_error_msg_fread_param_failure,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("count", f));
        pv_rope_attention_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->dimension <= 0) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p->dimension"));
        pv_rope_attention_param_delete(ypu, p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    count = pv_fread(&(p->head_dimension), sizeof(int32_t), 1, f);
    if (count != 1) {
        PV_ERROR_REPORT(
                &pv_error_msg_fread_param_failure,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("count", f));
        pv_rope_attention_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->head_dimension <= 0) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p->head_dimension"));
        pv_rope_attention_param_delete(ypu, p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    count = pv_fread(&(p->num_heads), sizeof(int32_t), 1, f);
    if (count != 1) {
        PV_ERROR_REPORT(
                &pv_error_msg_fread_param_failure,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("count", f));
        pv_rope_attention_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->num_heads <= 0) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p->num_heads"));
        pv_rope_attention_param_delete(ypu, p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    count = pv_fread(&(p->ffn_intermediate_dimension), sizeof(int32_t), 1, f);
    if (count != 1) {
        PV_ERROR_REPORT(
                &pv_error_msg_fread_param_failure,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("count", f));
        pv_rope_attention_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->ffn_intermediate_dimension <= 0) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p->ffn_intermediate_dimension"));
        pv_rope_attention_param_delete(ypu, p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    count = pv_fread(&(p->num_lookaheads), sizeof(int32_t), 1, f);
    if (count != 1) {
        PV_ERROR_REPORT(
                &pv_error_msg_fread_param_failure,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("count", f));
        pv_rope_attention_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->num_lookaheads < 0) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p->num_lookaheads"));
        pv_rope_attention_param_delete(ypu, p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    count = pv_fread(&(p->num_lookbacks), sizeof(int32_t), 1, f);
    if (count != 1) {
        PV_ERROR_REPORT(
                &pv_error_msg_fread_param_failure,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("count", f));
        pv_rope_attention_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->num_lookbacks < 0) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p->num_lookbacks"));
        pv_rope_attention_param_delete(ypu, p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    count = pv_fread(&(p->rope_base), sizeof(float), 1, f);
    if (count != 1) {
        PV_ERROR_REPORT(
                &pv_error_msg_fread_param_failure,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("count", f));
        pv_rope_attention_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->rope_base <= 0.0f) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p->rope_base"));
        pv_rope_attention_param_delete(ypu, p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    count = pv_fread(&(p->sdpa_downsample_factor), sizeof(int32_t), 1, f);
    if (count != 1) {
        PV_ERROR_REPORT(
                &pv_error_msg_fread_param_failure,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("count", f));
        pv_rope_attention_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->sdpa_downsample_factor <= 0) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p->sdpa_downsample_factor"));
        pv_rope_attention_param_delete(ypu, p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    pv_status_t status = pv_cnn_param_load(
            ypu,
            f,
            (pv_cnn_param_t **) &(p->conv_q_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_param_load,
                pv_status_to_string(status));
        pv_rope_attention_param_delete(ypu, p);
        return status;
    }

    status = pv_cnn_param_load(
            ypu,
            f,
            (pv_cnn_param_t **) &(p->conv_k_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_param_load,
                pv_status_to_string(status));
        pv_rope_attention_param_delete(ypu, p);
        return status;
    }

    status = pv_cnn_param_load(
            ypu,
            f,
            (pv_cnn_param_t **) &(p->conv_v_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_param_load,
                pv_status_to_string(status));
        pv_rope_attention_param_delete(ypu, p);
        return status;
    }

    status = pv_cnn_param_load(
            ypu,
            f,
            (pv_cnn_param_t **) &(p->conv_o_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_param_load,
                pv_status_to_string(status));
        pv_rope_attention_param_delete(ypu, p);
        return status;
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_rope_attention_param_delete)(
        pv_ypu_t *ypu,
        pv_rope_attention_param_t *param) {
    PV_ASSERT(ypu);
    if (param) {
        pv_cnn_param_delete(ypu, (pv_cnn_param_t *) (param->conv_o_param));

        pv_cnn_param_delete(ypu, (pv_cnn_param_t *) (param->conv_v_param));

        pv_cnn_param_delete(ypu, (pv_cnn_param_t *) (param->conv_k_param));

        pv_cnn_param_delete(ypu, (pv_cnn_param_t *) (param->conv_q_param));

        pv_ypu_host_free(ypu, param);
    }
}

bool PV_MOCKABLE(pv_rope_attention_param_is_equal)(
        const pv_rope_attention_param_t *object,
        const pv_rope_attention_param_t *other) {
    PV_ASSERT(object);
    PV_ASSERT(other);

    if (object->dimension != other->dimension) {
        return false;
    }

    if (object->head_dimension != other->head_dimension) {
        return false;
    }

    if (object->num_heads != other->num_heads) {
        return false;
    }

    if (object->ffn_intermediate_dimension != other->ffn_intermediate_dimension) {
        return false;
    }

    if (object->num_lookaheads != other->num_lookaheads) {
        return false;
    }

    if (object->num_lookbacks != other->num_lookbacks) {
        return false;
    }

    if (object->rope_base != other->rope_base) {
        return false;
    }

    if (object->sdpa_downsample_factor != other->sdpa_downsample_factor) {
        return false;
    }

    if (!pv_cnn_param_is_equal(object->conv_q_param, other->conv_q_param)) {
        return false;
    }

    if (!pv_cnn_param_is_equal(object->conv_k_param, other->conv_k_param)) {
        return false;
    }

    if (!pv_cnn_param_is_equal(object->conv_v_param, other->conv_v_param)) {
        return false;
    }

    if (!pv_cnn_param_is_equal(object->conv_o_param, other->conv_o_param)) {
        return false;
    }

    return true;
}

pv_status_t PV_MOCKABLE(pv_rope_attention_init)(
        pv_ypu_t *ypu,
        const pv_rope_attention_param_t *param,
        pv_rope_attention_t **object) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(object);

    *object = NULL;

    pv_rope_attention_t *o = pv_ypu_host_alloc(
            ypu,
            sizeof(pv_rope_attention_t));
    if (!o) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_host_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(o, 0, sizeof(pv_rope_attention_t));

    o->param = param;

    pv_status_t status = pv_cnn_init(
            ypu,
            param->conv_q_param,
            &(o->conv_q));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_init,
                pv_status_to_string(status));
        pv_rope_attention_delete(ypu, o);
        return status;
    }

    status = pv_cnn_init(
            ypu,
            param->conv_k_param,
            &(o->conv_k));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_init,
                pv_status_to_string(status));
        pv_rope_attention_delete(ypu, o);
        return status;
    }

    status = pv_cnn_init(
            ypu,
            param->conv_v_param,
            &(o->conv_v));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_init,
                pv_status_to_string(status));
        pv_rope_attention_delete(ypu, o);
        return status;
    }

    status = pv_cnn_init(
            ypu,
            param->conv_o_param,
            &(o->conv_o));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_init,
                pv_status_to_string(status));
        pv_rope_attention_delete(ypu, o);
        return status;
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_rope_attention_delete)(
        pv_ypu_t *ypu,
        pv_rope_attention_t *object) {
    PV_ASSERT(ypu);

    if (object) {
        pv_cnn_delete(ypu, object->conv_o);
        pv_cnn_delete(ypu, object->conv_v);
        pv_cnn_delete(ypu, object->conv_k);
        pv_cnn_delete(ypu, object->conv_q);

        pv_ypu_host_free(ypu, object);
    }
}

pv_status_t PV_MOCKABLE(pv_rope)(
        pv_ypu_t *ypu,
        pv_rope_attention_t *object,
        int32_t n,
        pv_ypu_mem_t *x,
        pv_ypu_mem_t *y) {
    PV_ASSERT(ypu);
    PV_ASSERT(object);
    PV_ASSERT(n > 0);
    PV_ASSERT(x);
    PV_ASSERT(y);

    PV_ASSERT(object->param->head_dimension % 2 == 0);
    const int32_t head_dimension_half = object->param->head_dimension / 2;
    const float rope_constant = -(logf(object->param->rope_base) / ((float) (head_dimension_half - 1)));

    pv_ypu_op_rope_args_t args_rope = {
            .output = y,
            .input = x,
            .b = 1,
            .m = n,
            .n = object->param->num_heads,
            .k = object->param->head_dimension,
            .position = 0,
            .rope_constant = rope_constant
    };
    pv_status_t status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_ROPE,
            &args_rope);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_ROPE),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        return status;
    }

    return PV_STATUS_SUCCESS;
}

static void pv_rope_attention_generate_mask(
        pv_ypu_t *ypu,
        pv_rope_attention_t *object,
        int32_t n,
        pv_ypu_mem_t *mask_indices_ypu) {
    int32_t *mask_indices = (int32_t *) pv_ypu_mem_get_host_view(ypu, mask_indices_ypu, false);
    for (int32_t i = 0; i < n; i++) {
        int start = (i - object->param->num_lookbacks > 0) ? (i - object->param->num_lookbacks) : 0;
        mask_indices[i * 2] = start;

        int end = (i + object->param->num_lookaheads < n) ? (i + object->param->num_lookaheads) : n - 1;
        mask_indices[i * 2 + 1] = end;
    }
    pv_ypu_mem_release_host_view(ypu, mask_indices_ypu, true);
}

static void pv_rope_attention_generate_mask_with_bucket(
        pv_ypu_t *ypu,
        pv_rope_attention_t *object,
        int32_t n,
        pv_ypu_mem_t *bucket_ypu,
        pv_ypu_mem_t *mask_indices_ypu) {
    int32_t *bucket = bucket_ypu ? (int32_t *) pv_ypu_mem_get_host_view(ypu, bucket_ypu, true) : NULL;
    int32_t *mask_indices = (int32_t *) pv_ypu_mem_get_host_view(ypu, mask_indices_ypu, false);
    for (int32_t i = 0; i < n; i++) {
        int start = (i - object->param->num_lookbacks > 0) ? (i - object->param->num_lookbacks) : 0;
        mask_indices[i * 2] = start;

        const int32_t q_phoneme = bucket[(i + 1) * object->param->sdpa_downsample_factor - 1];

        int32_t end = start;
        for (int32_t j = start; j < n; j++) {
            const int32_t kv_phoneme = bucket[j * object->param->sdpa_downsample_factor];
            if (q_phoneme + object->param->num_lookaheads < kv_phoneme) {
                break;
            }
            end = j;
        }
        mask_indices[i * 2 + 1] = end;
    }
    pv_ypu_mem_release_host_view(ypu, bucket_ypu, false);
    pv_ypu_mem_release_host_view(ypu, mask_indices_ypu, true);
}

pv_status_t PV_MOCKABLE(pv_rope_attention_forward)(
        pv_ypu_t *ypu,
        pv_rope_attention_t *object,
        int32_t n,
        pv_ypu_mem_t *x,
        pv_ypu_mem_t *bucket,
        pv_ypu_mem_t *y) {
    PV_ASSERT(ypu);
    PV_ASSERT(object);
    PV_ASSERT(n > 0);
    PV_ASSERT(x);
    PV_ASSERT(y);

    const int32_t head_dimension = object->param->head_dimension;
    const int32_t num_heads = object->param->num_heads;
    PV_ASSERT(object->param->sdpa_downsample_factor == 1);

    pv_ypu_mem_t *buffer_q = pv_ypu_buffer_get(
            ypu,
            n * num_heads * head_dimension * (int32_t) sizeof(float),
            false);
    if (!buffer_q) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("buffer_q_ypu"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    // # TODO (Ted): Might be faster to merge Q, K, V
    pv_status_t status = pv_cnn_forward(
            ypu,
            object->conv_q,
            n,
            x,
            buffer_q,
            0,
            0);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_forward,
                pv_status_to_string(status));
    	pv_ypu_buffer_release(ypu, buffer_q);
        return status;
    }

    pv_ypu_mem_t *buffer_k = pv_ypu_buffer_get(
            ypu,
            n * num_heads * head_dimension * (int32_t) sizeof(float),
            false);
    if (!buffer_k) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("buffer_k_ypu"));
        pv_ypu_buffer_release(ypu, buffer_q);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    status = pv_cnn_forward(
            ypu,
            object->conv_k,
            n,
            x,
            buffer_k,
            0,
            0);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_forward,
                pv_status_to_string(status));
        pv_ypu_buffer_release(ypu, buffer_k);
    	pv_ypu_buffer_release(ypu, buffer_q);
        return status;
    }

    pv_ypu_mem_t *buffer_v = pv_ypu_buffer_get(
            ypu,
            n * num_heads * head_dimension * (int32_t) sizeof(float),
            false);
    if (!buffer_v) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("buffer_v_ypu"));
        pv_ypu_buffer_release(ypu, buffer_k);
        pv_ypu_buffer_release(ypu, buffer_q);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    status = pv_cnn_forward(
            ypu,
            object->conv_v,
            n,
            x,
            buffer_v,
            0,
            0);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_forward,
                pv_status_to_string(status));
        pv_ypu_buffer_release(ypu, buffer_v);
        pv_ypu_buffer_release(ypu, buffer_k);
    	pv_ypu_buffer_release(ypu, buffer_q);
        return status;
    }

    pv_ypu_mem_t *buffer_o = pv_ypu_buffer_get(
            ypu,
            n * num_heads * head_dimension * (int32_t) sizeof(float),
            false);
    if (!buffer_o) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("buffer_o_ypu"));
        pv_ypu_buffer_release(ypu, buffer_v);
        pv_ypu_buffer_release(ypu, buffer_k);
        pv_ypu_buffer_release(ypu, buffer_q);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_mem_t *buffer_q_rope = pv_ypu_buffer_get(
            ypu,
            n * num_heads * head_dimension * (int32_t) sizeof(float),
            false);
    if (!buffer_q_rope) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("buffer_q_rope_ypu"));
        pv_ypu_buffer_release(ypu, buffer_o);
        pv_ypu_buffer_release(ypu, buffer_v);
        pv_ypu_buffer_release(ypu, buffer_k);
        pv_ypu_buffer_release(ypu, buffer_q);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    status = pv_rope(
            ypu,
            object,
            n,
            buffer_q,
            buffer_q_rope);
    pv_ypu_buffer_release(ypu, buffer_q);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_rope,
                pv_status_to_string(status));
        pv_ypu_buffer_release(ypu, buffer_q_rope);
        pv_ypu_buffer_release(ypu, buffer_o);
        pv_ypu_buffer_release(ypu, buffer_v);
        pv_ypu_buffer_release(ypu, buffer_k);
        return status;
    }

    pv_ypu_mem_t *buffer_k_rope = pv_ypu_buffer_get(
            ypu,
            n * num_heads * head_dimension * (int32_t) sizeof(float),
            false);
    if (!buffer_k_rope) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("buffer_k_rope_ypu"));
        pv_ypu_buffer_release(ypu, buffer_q_rope);
        pv_ypu_buffer_release(ypu, buffer_o);
        pv_ypu_buffer_release(ypu, buffer_v);
        pv_ypu_buffer_release(ypu, buffer_k);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    status = pv_rope(
            ypu,
            object,
            n,
            buffer_k,
            buffer_k_rope);
    pv_ypu_buffer_release(ypu, buffer_k);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_rope,
                pv_status_to_string(status));
        pv_ypu_buffer_release(ypu, buffer_k_rope);
        pv_ypu_buffer_release(ypu, buffer_q_rope);
        pv_ypu_buffer_release(ypu, buffer_o);
        pv_ypu_buffer_release(ypu, buffer_v);
        return status;
    }

    pv_ypu_mem_t *buffer_scores = pv_ypu_buffer_get(
            ypu,
            num_heads * n * n * (int32_t) sizeof(float),
            false);
    if (!buffer_scores) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("buffer_scores"));
        pv_ypu_buffer_release(ypu, buffer_k_rope);
        pv_ypu_buffer_release(ypu, buffer_q_rope);
        pv_ypu_buffer_release(ypu, buffer_o);
        pv_ypu_buffer_release(ypu, buffer_v);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_mem_t *mask_indices = pv_ypu_buffer_get(
            ypu,
            n * 2 * (int32_t) sizeof(float),
            false);
    if (!mask_indices) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("mask_indices"));
        pv_ypu_buffer_release(ypu, buffer_k_rope);
        pv_ypu_buffer_release(ypu, buffer_q_rope);
        pv_ypu_buffer_release(ypu, buffer_o);
        pv_ypu_buffer_release(ypu, buffer_v);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    if (bucket) {
        pv_rope_attention_generate_mask_with_bucket(
                ypu,
                object,
                n,
                bucket,
                mask_indices);
    }
    else {
        pv_rope_attention_generate_mask(
                ypu,
                object,
                n,
                mask_indices);
    }
    pv_ypu_op_sdpa_masked_args_t args_sdpa = {
            .output = buffer_o,
            .query = buffer_q_rope,
            .key = buffer_k_rope,
            .value = buffer_v,
            .scores = buffer_scores,
            .mask_indices = mask_indices,
            .batch_size = 1,
            .query_size = n,
            .key_value_size = n,
            .num_heads = num_heads,
            .num_kv_heads = num_heads,
            .head_dim = head_dimension,
    };
    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_SDPA_MASKED,
            &args_sdpa);
    pv_ypu_buffer_release(ypu, mask_indices);
    pv_ypu_buffer_release(ypu, buffer_scores);
    pv_ypu_buffer_release(ypu, buffer_k_rope);
    pv_ypu_buffer_release(ypu, buffer_q_rope);
    pv_ypu_buffer_release(ypu, buffer_v);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_SDPA_MASKED),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        pv_ypu_buffer_release(ypu, buffer_o);
        return status;
    }

    status = pv_cnn_forward(
            ypu,
            object->conv_o,
            n,
            buffer_o,
            y,
            0,
            0);
    pv_ypu_buffer_release(ypu, buffer_o);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_cnn_forward,
                pv_status_to_string(status));
        return status;
    }

    return PV_STATUS_SUCCESS;
}
