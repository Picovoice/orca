#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "core/pv_error_messages.h"
#include "math/pv_math.h"
#include "orca/pv_orca_gaussian_upsampler.h"
#include "orca/pv_rope_transformer.h"
#include "orca/pv_orca_util.h"
#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_orca_gaussian_upsampler_param_serialize)(
        pv_ypu_t *ypu,
        const pv_orca_gaussian_upsampler_param_t *param,
        FILE *file) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(file);

    size_t count = fwrite(&(param->dimension), sizeof(int32_t), 1, file);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

    count = fwrite(&(param->num_lookaheads_gaussian_upsampling), sizeof(int32_t), 1, file);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

    count = fwrite(&(param->num_lookbacks_gaussian_upsampling), sizeof(int32_t), 1, file);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

    pv_status_t status = pv_rope_transformer_param_serialize(
            ypu,
            param->transformer_param,
            file);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_rope_transformer_param_serialize,
                pv_status_to_string(status));
        return status;
    }

    return PV_STATUS_SUCCESS;
}

#endif

pv_status_t PV_MOCKABLE(pv_orca_gaussian_upsampler_param_load)(
        pv_ypu_t *ypu,
        FILE *f,
        pv_orca_gaussian_upsampler_param_t **param) {
    PV_ASSERT(ypu);
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_orca_gaussian_upsampler_param_t *p = pv_ypu_host_alloc(
            ypu,
            sizeof(pv_orca_gaussian_upsampler_param_t));
    if (!p) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_host_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(p, 0, sizeof(pv_orca_gaussian_upsampler_param_t));

    size_t count = pv_fread(&(p->dimension), sizeof(int32_t), 1, f);
    if (count != 1) {
        PV_ERROR_REPORT(
                &pv_error_msg_fread_param_failure,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("count", f));
        pv_orca_gaussian_upsampler_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->dimension <= 0) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p->dimension"));
        pv_orca_gaussian_upsampler_param_delete(ypu, p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    count = pv_fread(&(p->num_lookaheads_gaussian_upsampling), sizeof(int32_t), 1, f);
    if (count != 1) {
        PV_ERROR_REPORT(
                &pv_error_msg_fread_param_failure,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("count", f));
        pv_orca_gaussian_upsampler_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->num_lookaheads_gaussian_upsampling < 0) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p->num_lookaheads_gaussian_upsampling"));
        pv_orca_gaussian_upsampler_param_delete(ypu, p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    count = pv_fread(&(p->num_lookbacks_gaussian_upsampling), sizeof(int32_t), 1, f);
    if (count != 1) {
        PV_ERROR_REPORT(
                &pv_error_msg_fread_param_failure,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("count", f));
        pv_orca_gaussian_upsampler_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->num_lookbacks_gaussian_upsampling < 0) {
        PV_ERROR_REPORT(
                &pv_error_msg_invalid_argument_internal,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p->num_lookbacks_gaussian_upsampling"));
        pv_orca_gaussian_upsampler_param_delete(ypu, p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    pv_status_t status = pv_rope_transformer_param_load(
            ypu,
            f,
            (pv_rope_transformer_param_t **) &(p->transformer_param));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_rope_transformer_param_load,
                pv_status_to_string(status));
        pv_orca_gaussian_upsampler_param_delete(ypu, p);
        return status;
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_orca_gaussian_upsampler_param_delete)(
        pv_ypu_t *ypu,
        pv_orca_gaussian_upsampler_param_t *param) {
    PV_ASSERT(ypu);

    if (param) {
        pv_rope_transformer_param_delete(ypu, (pv_rope_transformer_param_t *) (param->transformer_param));

        pv_ypu_host_free(ypu, param);
    }
}

bool PV_MOCKABLE(pv_orca_gaussian_upsampler_param_is_equal)(
        const pv_orca_gaussian_upsampler_param_t *object,
        const pv_orca_gaussian_upsampler_param_t *other) {
    PV_ASSERT(object);
    PV_ASSERT(other);

    if (object->dimension != other->dimension) {
        return false;
    }

    if (object->num_lookaheads_gaussian_upsampling != other->num_lookaheads_gaussian_upsampling) {
        return false;
    }

    if (object->num_lookbacks_gaussian_upsampling != other->num_lookbacks_gaussian_upsampling) {
        return false;
    }

    if (!pv_rope_transformer_param_is_equal(
                object->transformer_param,
                other->transformer_param)) {
            return false;
    }

    return true;
}

pv_status_t PV_MOCKABLE(pv_orca_gaussian_upsampler_init)(
        pv_ypu_t *ypu,
        const pv_orca_gaussian_upsampler_param_t *param,
        pv_orca_gaussian_upsampler_t **object) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(object);

    *object = NULL;

    pv_orca_gaussian_upsampler_t *o = pv_ypu_host_alloc(
            ypu,
            sizeof(pv_orca_gaussian_upsampler_t));
    if (!o) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_host_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(o, 0, sizeof(pv_orca_gaussian_upsampler_t));

    o->param = param;

    pv_status_t status = pv_rope_transformer_init(
            ypu,
            param->transformer_param,
            &(o->transformer));
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_rope_transformer_init,
                pv_status_to_string(status));
        pv_orca_gaussian_upsampler_delete(ypu, o);
        return status;
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_orca_gaussian_upsampler_delete)(
        pv_ypu_t *ypu,
        pv_orca_gaussian_upsampler_t *object) {
    PV_ASSERT(ypu);

    if (object) {
        pv_rope_transformer_delete(ypu, object->transformer);

        pv_ypu_host_free(ypu, object);
    }
}


pv_status_t PV_MOCKABLE(pv_orca_gaussian_upsampler_attention)(
        pv_ypu_t *ypu,
        pv_orca_gaussian_upsampler_t *object,
        int32_t n,
        int32_t T,
        const int32_t *d,
        pv_ypu_mem_t *buffer_gaussian_center_ypu,
        pv_ypu_mem_t *std_ypu,
        pv_ypu_mem_t *v_ypu,
        pv_ypu_mem_t *y_ypu) {
    PV_ASSERT(ypu);
    PV_ASSERT(object);
    PV_ASSERT(n > 0);
    PV_ASSERT(T > 0);
    PV_ASSERT(d);
    PV_ASSERT(buffer_gaussian_center_ypu);
    PV_ASSERT(std_ypu);
    PV_ASSERT(v_ypu);
    PV_ASSERT(y_ypu);

    const int32_t dimension = object->param->dimension;
    const int32_t num_lookaheads = object->param->num_lookaheads_gaussian_upsampling;
    const int32_t num_lookbacks = object->param->num_lookbacks_gaussian_upsampling;

    pv_ypu_mem_t *bucket_ypu = pv_ypu_buffer_get(
            ypu,
            T * (int32_t) sizeof(int32_t),
            false);
    if (!bucket_ypu) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_orca_util_generate_bucket(
            ypu,
            n,
            0,
            d,
            bucket_ypu);

    pv_ypu_op_gaussian_attention_args_t gaussian_attention_args = {
            .output = y_ypu,
            .v = v_ypu,
            .std = std_ypu,
            .gaussian_center = buffer_gaussian_center_ypu,
            .bucket = bucket_ypu,
            .m = T,
            .n = n,
            .k = dimension,
            .num_lookaheads = num_lookaheads,
            .num_lookbacks = num_lookbacks
    };
    pv_status_t status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_GAUSSIAN_ATTENTION,
            &gaussian_attention_args);
    pv_ypu_buffer_release(ypu, bucket_ypu);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_GAUSSIAN_ATTENTION),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        return status;
    }
    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_orca_gaussian_upsampler_forward)(
        pv_ypu_t *ypu,
        pv_orca_gaussian_upsampler_t *object,
        int32_t n,
        int32_t T,
        pv_ypu_mem_t *x,
        int32_t *d,
        pv_ypu_mem_t *std,
        pv_ypu_mem_t *y) {
    PV_ASSERT(ypu);
    PV_ASSERT(object);
    PV_ASSERT(n > 0);
    PV_ASSERT(T > 0);
    PV_ASSERT(x);
    PV_ASSERT(d);
    PV_ASSERT(std);
    PV_ASSERT(y);

    pv_status_t status = pv_rope_transformer_forward(
            ypu,
            object->transformer,
            n,
            x,
            NULL,
            x);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_rope_transformer_forward,
                pv_status_to_string(status));
        return status;
    }

    pv_ypu_mem_t *buffer_gaussian_center_ypu = pv_ypu_buffer_get(
            ypu,
            n * (int32_t) sizeof(float),
            false);
    if (!buffer_gaussian_center_ypu) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_device_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("buffer_gaussian_center"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    float *buffer_gaussian_center = pv_ypu_mem_get_host_view(ypu, buffer_gaussian_center_ypu, false);

    float duration_cumulative_sum = 0.0f;
    for (int32_t i = 0; i < n; i++) {
        buffer_gaussian_center[i] = duration_cumulative_sum + (((float) d[i]) / 2.0f);
        duration_cumulative_sum += ((float ) d[i]);
    }

    pv_ypu_mem_release_host_view(ypu, buffer_gaussian_center_ypu, true);

    status = pv_orca_gaussian_upsampler_attention(
            ypu,
            object,
            n,
            T,
            d,
            buffer_gaussian_center_ypu,
            std,
            x,
            y);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_rope_transformer_forward,
                pv_status_to_string(status));
        pv_ypu_buffer_release(ypu, buffer_gaussian_center_ypu);
        return status;
    }

    pv_ypu_buffer_release(ypu, buffer_gaussian_center_ypu);

    return PV_STATUS_SUCCESS;
}
