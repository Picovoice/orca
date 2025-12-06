#include <stdlib.h>
#include <string.h>

#include "orca/pv_affine.h"
#include "orca/pv_profiler.h"
#include "util/pv_check_status.h"
#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"
#include "ypu/mock/pv_ypu_mock.h"

#endif

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_affine_param_serialize_buffer)(
        pv_ypu_t *ypu,
        const pv_affine_param_t *param,
        size_t *length,
        void **buffer) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);

    *length =
            sizeof(int32_t) +
            sizeof(int32_t) +
            (sizeof(float) * param->num_channels) +
            (sizeof(float) * param->num_channels);
    *buffer = NULL;

    void *b = pv_ypu_host_alloc(ypu, *length);
    PV_CHECK_ALLOC(b);
    *buffer = b;

    memcpy(b, &(param->num_channels), sizeof(int32_t));
    b += sizeof(int32_t);

    memcpy(b, &(param->scale_offset), sizeof(int32_t));
    b += sizeof(int32_t);

    memcpy(b, param->bias->data, sizeof(float) * param->num_channels);
    b += sizeof(float) * param->num_channels;

    memcpy(b, param->scale->data, sizeof(float) * param->num_channels);

    return PV_STATUS_SUCCESS;
}


pv_status_t PV_MOCKABLE(pv_affine_param_serialize)(
        pv_ypu_t *ypu,
        const pv_affine_param_t *param,
        FILE *file) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(file);

    size_t length = 0;
    void *buffer = NULL;

    pv_status_t status = pv_affine_param_serialize_buffer(ypu, param, &length, &buffer);
    PV_CHECK_STATUS(status);

    const size_t count = fwrite(buffer, 1, length, file);
    pv_ypu_host_free(ypu, buffer);

    return (count == length) ? PV_STATUS_SUCCESS : PV_STATUS_IO_ERROR;
}

#endif

pv_status_t PV_MOCKABLE(pv_affine_param_load)(
        pv_ypu_t *ypu,
        FILE *f,
        pv_affine_param_t **param) {
    PV_ASSERT(ypu);
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_affine_param_t *p = pv_ypu_host_alloc(ypu, sizeof(pv_affine_param_t));
    PV_CHECK_ALLOC(p);

    memset(p, 0, sizeof(pv_affine_param_t));

    size_t count = pv_fread(&(p->num_channels), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_affine_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->num_channels <= 0) {
        pv_affine_param_delete(ypu, p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    count = pv_fread(&(p->scale_offset), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_affine_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }

    const size_t length = (size_t) p->num_channels;

    p->bias = pv_ypu_config_mem_alloc(
            ypu,
            (int32_t) (sizeof(float) * length),
            PV_YPU_DEVICE_MEM_FLAG_STATIC);
    if (!p->bias) {
        pv_affine_param_delete(ypu, p);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    count = pv_fread(p->bias->data, sizeof(float), length, f);
    if (count != length) {
        pv_affine_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }

    p->scale = pv_ypu_config_mem_alloc(
            ypu,
            (int32_t) (sizeof(float) * length),
            PV_YPU_DEVICE_MEM_FLAG_STATIC);
    if (!p->scale) {
        pv_affine_param_delete(ypu, p);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    count = pv_fread(p->scale->data, sizeof(float), length, f);
    if (count != length) {
        pv_affine_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_affine_param_delete)(
        pv_ypu_t *ypu,
        pv_affine_param_t *param) {
    PV_ASSERT(ypu);

    if (param) {
        pv_ypu_config_mem_free(ypu, param->scale);
        pv_ypu_config_mem_free(ypu, param->bias);
        pv_ypu_host_free(ypu, param);
    }
}

bool PV_MOCKABLE(pv_affine_param_is_equal)(const pv_affine_param_t *object, const pv_affine_param_t *other) {
    PV_ASSERT(object);
    PV_ASSERT(other);

    if (object->num_channels != other->num_channels) {
        return false;
    }

    if (object->scale_offset != other->scale_offset) {
        return false;
    }

    if (!pv_ypu_config_mem_is_equal(object->bias, other->bias)) {
        return false;
    }

    if (!pv_ypu_config_mem_is_equal(object->scale, other->scale)) {
        return false;
    }

    return true;
}

struct pv_affine {
    const pv_affine_param_t *param;

    pv_ypu_mem_t *scale;
    pv_ypu_mem_t *bias;
    pv_ypu_mem_t *scale_with_offset;
};

pv_status_t PV_MOCKABLE(pv_affine_init)(
        pv_ypu_t *ypu,
        const pv_affine_param_t *param,
        pv_affine_t **object) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(object);

    *object = NULL;

    pv_affine_t *o = pv_ypu_host_alloc(ypu, sizeof(pv_affine_t));
    if (!o) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(o, 0, sizeof(pv_affine_t));

    o->param = param;

    const int32_t num_channels = o->param->num_channels;

    o->scale = pv_ypu_mem_from_config(ypu, param->scale);
    if (!o->scale) {
        pv_affine_delete(ypu, o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->bias = pv_ypu_mem_from_config(ypu, param->bias);
    if (!o->bias) {
        pv_affine_delete(ypu, o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->scale_with_offset = pv_ypu_mem_alloc(
            ypu,
            num_channels * (int32_t) sizeof(float),
            PV_YPU_DEVICE_MEM_FLAG_NONE);
    if (!o->scale_with_offset) {
        pv_affine_delete(ypu, o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_op_elementwise_scalar_args_t args = {
            .output = o->scale_with_offset,
            .input = o->scale,
            .scalar.f32 = o->param->scale_offset,
            .length = num_channels,
            .output_offset = 0,
            .input_offset = 0,
    };

    pv_status_t status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_ADDSV,
            &args);
    if (status != PV_STATUS_SUCCESS) {
        pv_affine_delete(ypu, o);
        return status;
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_affine_delete)(
        pv_ypu_t *ypu,
        pv_affine_t *object) {
    PV_ASSERT(ypu);

    if (object) {
        pv_ypu_mem_free(ypu, object->scale_with_offset);
        pv_ypu_mem_free(ypu, object->bias);
        pv_ypu_mem_free(ypu, object->scale);
        pv_ypu_host_free(ypu, object);
    }
}

int32_t PV_MOCKABLE(pv_affine_num_channels)(const pv_affine_t *object) {
    PV_ASSERT(object);

    return object->param->num_channels;
}

pv_status_t PV_MOCKABLE(pv_affine_forward)(
        pv_ypu_t *ypu,
        pv_affine_t *object,
        int32_t n,
        pv_ypu_mem_t *x_ypu_mem,
        pv_ypu_mem_t *y_ypu_mem,
        int32_t x_offset,
        int32_t y_offset) {
    PV_ASSERT(ypu);
    PV_ASSERT(object);
    PV_ASSERT(n);
    PV_ASSERT(x_ypu_mem);
    PV_ASSERT(y_ypu_mem);
    PV_ORCA_PROFILER_START("affine");

    pv_ypu_op_pairwise_broadcast_args_t args0 = {
            .output = y_ypu_mem,
            .lhs = x_ypu_mem,
            .rhs = object->scale_with_offset,
            .m = n,
            .n = object->param->num_channels,
            .output_offset = y_offset,
            .lhs_offset = x_offset,
            .rhs_offset = 0,
    };

    pv_status_t status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_MULMV,
            &args0);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_op_pairwise_broadcast_args_t args1 = {
            .output = y_ypu_mem,
            .lhs = y_ypu_mem,
            .rhs = object->bias,
            .m = n,
            .n = object->param->num_channels,
            .output_offset = y_offset,
            .lhs_offset = y_offset,
            .rhs_offset = 0,
    };

    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_ADDMV,
            &args1);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    PV_ORCA_PROFILER_STOP("affine");
    return PV_STATUS_SUCCESS;
}
