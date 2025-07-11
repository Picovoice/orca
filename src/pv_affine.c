#include <stdlib.h>
#include <string.h>

#include "orca/pv_affine.h"
#include "orca/pv_profiler.h"
#include "util/pv_check_status.h"
#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_affine_param_serialize_buffer)(
        const pv_affine_param_t *param,
        size_t *length,
        void **buffer) {
    PV_ASSERT(param);

    *length =
            sizeof(int32_t) +
            sizeof(int32_t) +
            (sizeof(float) * param->num_channels) +
            (sizeof(float) * param->num_channels);
    *buffer = NULL;

    void *b = malloc(*length);
    PV_CHECK_ALLOC(b);
    *buffer = b;

    memcpy(b, &(param->num_channels), sizeof(int32_t));
    b += sizeof(int32_t);

    memcpy(b, &(param->scale_offset), sizeof(int32_t));
    b += sizeof(int32_t);

    memcpy(b, param->bias, sizeof(float) * param->num_channels);
    b += sizeof(float) * param->num_channels;

    memcpy(b, param->scale, sizeof(float) * param->num_channels);

    return PV_STATUS_SUCCESS;
}


pv_status_t PV_MOCKABLE(pv_affine_param_serialize)(const pv_affine_param_t *param, FILE *file) {
    PV_ASSERT(param);
    PV_ASSERT(file);

    size_t length = 0;
    void *buffer = NULL;

    pv_status_t status = pv_affine_param_serialize_buffer(param, &length, &buffer);
    PV_CHECK_STATUS(status);

    const size_t count = fwrite(buffer, 1, length, file);
    free(buffer);

    return (count == length) ? PV_STATUS_SUCCESS : PV_STATUS_IO_ERROR;
}

#endif

pv_status_t PV_MOCKABLE(pv_affine_param_load)(FILE *f, pv_affine_param_t **param) {
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_affine_param_t *p = calloc(1, sizeof(pv_affine_param_t));
    PV_CHECK_ALLOC(p);

    size_t count = pv_fread(&(p->num_channels), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_affine_param_delete(p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->num_channels <= 0) {
        pv_affine_param_delete(p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    count = pv_fread(&(p->scale_offset), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_affine_param_delete(p);
        return PV_STATUS_IO_ERROR;
    }

    const size_t length = (size_t) p->num_channels;

    p->bias = malloc(sizeof(float) * length);
    if (!p->bias) {
        pv_affine_param_delete(p);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    count = pv_fread((q7_t *) p->bias, sizeof(float), length, f);
    if (count != length) {
        pv_affine_param_delete(p);
        return PV_STATUS_IO_ERROR;
    }

    p->scale = malloc(sizeof(float) * length);
    if (!p->scale) {
        pv_affine_param_delete(p);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    count = pv_fread((q7_t *) (p->scale), sizeof(float), length, f);
    if (count != length) {
        pv_affine_param_delete(p);
        return PV_STATUS_IO_ERROR;
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_affine_param_delete)(pv_affine_param_t *param) {
    if (param) {
        free((float *) (param->scale));
        free((float *) (param->bias));

        free(param);
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

    for (int32_t i = 0; i < object->num_channels; i++) {
        if (object->bias[i] != other->bias[i]) {
            return false;
        }
    }

    for (int32_t i = 0; i < object->num_channels; i++) {
        if (object->scale[i] != other->scale[i]) {
            return false;
        }
    }

    return true;
}

struct pv_affine {
    const pv_affine_param_t *param;

    float *scale_with_offset;
};

pv_status_t PV_MOCKABLE(pv_affine_init)(const pv_affine_param_t *param, pv_affine_t **object) {
    PV_ASSERT(param);
    PV_ASSERT(object);

    *object = NULL;

    pv_affine_t *o = calloc(1, sizeof(pv_affine_t));
    if (!o) {
        pv_affine_delete(o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->param = param;

    const int32_t num_channels = o->param->num_channels;

    o->scale_with_offset = malloc(num_channels * sizeof(float));
    if (!o->scale_with_offset) {
        return PV_STATUS_OUT_OF_MEMORY;
    }
    memcpy(o->scale_with_offset, o->param->scale, num_channels * sizeof(float));

    for (int32_t j = 0; j < num_channels; j++) {
        o->scale_with_offset[j] += (float) o->param->scale_offset;
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_affine_delete)(pv_affine_t *object) {
    if (object) {
        if (object->scale_with_offset) {
            free(object->scale_with_offset);
        }

        free(object);
    }
}

int32_t PV_MOCKABLE(pv_affine_num_channels)(const pv_affine_t *object) {
    PV_ASSERT(object);

    return object->param->num_channels;
}

pv_status_t PV_MOCKABLE(pv_affine_forward)(pv_affine_t *object, int32_t n, const float *x, float *y) {
    PV_ASSERT(object);
    PV_ASSERT(n);
    PV_ASSERT(x);
    PV_ASSERT(y);
    PV_ORCA_PROFILER_START("affine");

    const int32_t num_channels = object->param->num_channels;

    const float *scale = object->scale_with_offset;
    const float *bias = object->param->bias;

    for (int32_t i = 0; i < n; i++) {
        const int32_t offset = i * num_channels;
        for (int32_t j = 0; j < num_channels; j++) {
            y[offset + j] = (x[offset + j] * scale[j]) + bias[j];
        }
    }

    PV_ORCA_PROFILER_STOP("affine");
    return PV_STATUS_SUCCESS;
}
