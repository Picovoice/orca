#include <stdlib.h>
#include <string.h>

#include "math/pv_math.h"
#include "orca/pv_layer_norm.h"
#include "orca/pv_profiler.h"
#include "util/pv_check_status.h"
#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"
#include "ypu/mock/pv_ypu_mock.h"

#endif

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_layer_norm_param_serialize_buffer)(
        pv_ypu_t *ypu,
        const pv_layer_norm_param_t *param,
        size_t *length,
        void **buffer) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);

    *length =
            sizeof(int32_t) +
            (sizeof(q7_t) * param->num_channels) +
            (sizeof(q7_t) * param->num_channels);
    *buffer = NULL;

    void *b = pv_ypu_host_alloc(ypu, *length);
    PV_CHECK_ALLOC(b);
    *buffer = b;

    memcpy(b, &(param->num_channels), sizeof(int32_t));
    b += sizeof(int32_t);

    memcpy(b, param->bias->data, sizeof(q7_t) * param->num_channels);
    b += sizeof(q7_t) * param->num_channels;

    memcpy(b, param->weight->data, sizeof(q7_t) * param->num_channels);

    return PV_STATUS_SUCCESS;
}


pv_status_t PV_MOCKABLE(pv_layer_norm_param_serialize)(pv_ypu_t *ypu, const pv_layer_norm_param_t *param, FILE *file) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(file);

    size_t length = 0;
    void *buffer = NULL;

    pv_status_t status = pv_layer_norm_param_serialize_buffer(ypu, param, &length, &buffer);
    PV_CHECK_STATUS(status);

    const size_t count = fwrite(buffer, 1, length, file);
    pv_ypu_host_free(ypu, buffer);

    return (count == length) ? PV_STATUS_SUCCESS : PV_STATUS_IO_ERROR;
}

#endif

pv_status_t PV_MOCKABLE(pv_layer_norm_param_load)(pv_ypu_t *ypu, FILE *f, pv_layer_norm_param_t **param) {
    PV_ASSERT(ypu);
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_layer_norm_param_t *p = pv_ypu_host_alloc(ypu, sizeof(pv_layer_norm_param_t));
    if (!p) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(p, 0, sizeof(pv_layer_norm_param_t));

    size_t count = pv_fread(&(p->num_channels), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_layer_norm_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->num_channels <= 0) {
        pv_layer_norm_param_delete(ypu, p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    p->bias = pv_ypu_config_mem_alloc(
            ypu,
            sizeof(q7_t) * p->num_channels,
            PV_YPU_DEVICE_MEM_FLAG_STATIC);
    if (!p->bias) {
        pv_layer_norm_param_delete(ypu, p);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    count = pv_fread(p->bias->data, sizeof(q7_t), p->num_channels, f);
    if (count != (size_t) p->num_channels) {
        pv_layer_norm_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }

    p->weight = pv_ypu_config_mem_alloc(
            ypu,
            sizeof(q7_t) * p->num_channels,
            PV_YPU_DEVICE_MEM_FLAG_STATIC);
    if (!p->weight) {
        pv_layer_norm_param_delete(ypu, p);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    count = pv_fread(p->weight->data, sizeof(q7_t), p->num_channels, f);
    if ((int32_t) count != (p->num_channels)) {
        pv_layer_norm_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}


void PV_MOCKABLE(pv_layer_norm_param_delete)(pv_ypu_t *ypu, pv_layer_norm_param_t *param) {
    PV_ASSERT(ypu);

    if (param) {
        pv_ypu_config_mem_free(ypu, param->weight);
        pv_ypu_config_mem_free(ypu, param->bias);
        pv_ypu_host_free(ypu, param);
    }
}

bool PV_MOCKABLE(pv_layer_norm_param_is_equal)(
        const pv_layer_norm_param_t *object,
        const pv_layer_norm_param_t *other) {
    PV_ASSERT(object);
    PV_ASSERT(other);

    if (object->num_channels != other->num_channels) {
        return false;
    }

    if (!pv_ypu_config_mem_is_equal(object->bias, other->bias)) {
        return false;
    }

    if (!pv_ypu_config_mem_is_equal(object->weight, other->weight)) {
        return false;
    }

    return true;
}

struct pv_layer_norm {
    const pv_layer_norm_param_t *param;

    pv_ypu_mem_t *weight;
    pv_ypu_mem_t *bias;
};

pv_status_t PV_MOCKABLE(pv_layer_norm_init)(
        pv_ypu_t *ypu,
        const pv_layer_norm_param_t *param,
        pv_layer_norm_t **object) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(object);

    *object = NULL;

    pv_layer_norm_t *o = pv_ypu_host_alloc(ypu, sizeof(pv_layer_norm_t));
    if (!o) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(o, 0, sizeof(pv_layer_norm_t));

    o->param = param;

    o->weight = pv_ypu_mem_from_config(
            ypu,
            param->weight);
    if (!o->weight) {
        pv_layer_norm_delete(ypu, o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->bias = pv_ypu_mem_from_config(
            ypu,
            param->bias);
    if (!o->bias) {
        pv_layer_norm_delete(ypu, o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_layer_norm_delete)(pv_ypu_t *ypu, pv_layer_norm_t *object) {
    PV_ASSERT(ypu);

    if (object) {
        pv_ypu_mem_free(ypu, object->weight);
        pv_ypu_mem_free(ypu, object->bias);
        pv_ypu_host_free(ypu, object);
    }
}

int32_t PV_MOCKABLE(pv_layer_norm_num_channels)(const pv_layer_norm_t *object) {
    PV_ASSERT(object);

    return object->param->num_channels;
}

pv_status_t PV_MOCKABLE(pv_layer_norm_forward)(
        pv_ypu_t *ypu,
        pv_layer_norm_t *object,
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

    const int32_t num_channels = object->param->num_channels;

    pv_ypu_op_layer_norm_args_t args = {
            .output = y_ypu_mem,
            .input = x_ypu_mem,
            .weight = object->weight,
            .bias = object->bias,
            .eps = object->param->eps,
            .m = n,
            .n = num_channels,
            .output_offset = y_offset,
            .input_offset = x_offset,
            .weight_offset = 0,
            .bias_offset = 0,
    };

    pv_status_t status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_LAYER_NORM_F32_F32_Q7_Q7,
            &args);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}
