#include <stdlib.h>
#include <string.h>

#include "math/pv_math.h"
#include "orca/pv_layer_norm.h"
#include "orca/pv_profiler.h"
#include "util/pv_check_status.h"
#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_layer_norm_param_serialize_buffer)(
        const pv_layer_norm_param_t *param,
        size_t *length,
        void **buffer) {
    PV_ASSERT(param);

    *length =
            sizeof(int32_t) +
            (sizeof(q7_t) * param->num_channels) +
            (sizeof(q7_t) * param->num_channels);
    *buffer = NULL;

    void *b = malloc(*length);
    PV_CHECK_ALLOC(b);
    *buffer = b;

    memcpy(b, &(param->num_channels), sizeof(int32_t));
    b += sizeof(int32_t);

    memcpy(b, param->bias, sizeof(q7_t) * param->num_channels);
    b += sizeof(q7_t) * param->num_channels;

    memcpy(b, param->weight, sizeof(q7_t) * param->num_channels);

    return PV_STATUS_SUCCESS;
}


pv_status_t PV_MOCKABLE(pv_layer_norm_param_serialize)(const pv_layer_norm_param_t *param, FILE *file) {
    PV_ASSERT(param);
    PV_ASSERT(file);

    size_t length = 0;
    void *buffer = NULL;

    pv_status_t status = pv_layer_norm_param_serialize_buffer(param, &length, &buffer);
    PV_CHECK_STATUS(status);

    const size_t count = fwrite(buffer, 1, length, file);
    free(buffer);

    return (count == length) ? PV_STATUS_SUCCESS : PV_STATUS_IO_ERROR;
}

#endif

pv_status_t PV_MOCKABLE(pv_layer_norm_param_load)(FILE *f, pv_layer_norm_param_t **param) {
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_layer_norm_param_t *p = calloc(1, sizeof(pv_layer_norm_param_t));
    if (!p) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    size_t count = pv_fread(&(p->num_channels), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_layer_norm_param_delete(p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->num_channels <= 0) {
        pv_layer_norm_param_delete(p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    p->bias = malloc(sizeof(q7_t) * p->num_channels);
    if (!p->bias) {
        pv_layer_norm_param_delete(p);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    count = pv_fread((q7_t *) (p->bias), sizeof(q7_t), p->num_channels, f);
    if (count != (size_t) p->num_channels) {
        pv_layer_norm_param_delete(p);
        return PV_STATUS_IO_ERROR;
    }

    p->weight = malloc(sizeof(q7_t) * p->num_channels);
    if (!p->weight) {
        pv_layer_norm_param_delete(p);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    count = pv_fread((q7_t *) (p->weight), sizeof(q7_t), p->num_channels, f);
    if ((int32_t) count != (p->num_channels)) {
        pv_layer_norm_param_delete(p);
        return PV_STATUS_IO_ERROR;
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}


void PV_MOCKABLE(pv_layer_norm_param_delete)(pv_layer_norm_param_t *param) {
    if (param) {
        free((q7_t *) (param->weight));
        free((q7_t *) (param->bias));

        free(param);
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

    for (int32_t i = 0; i < object->num_channels; i++) {
        if (object->bias[i] != other->bias[i]) {
            return false;
        }
    }

    for (int32_t i = 0; i < object->num_channels; i++) {
        if (object->weight[i] != other->weight[i]) {
            return false;
        }
    }

    return true;
}

struct pv_layer_norm {
    const pv_layer_norm_param_t *param;
};

pv_status_t PV_MOCKABLE(pv_layer_norm_init)(
        const pv_layer_norm_param_t *param,
        pv_layer_norm_t **object) {
    PV_ASSERT(param);
    PV_ASSERT(object);

    *object = NULL;

    pv_layer_norm_t *o = calloc(1, sizeof(pv_layer_norm_t));
    if (!o) {
        pv_layer_norm_delete(o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->param = param;

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_layer_norm_delete)(pv_layer_norm_t *object) {
    if (object) {
        free(object);
    }
}

int32_t PV_MOCKABLE(pv_layer_norm_num_channels)(const pv_layer_norm_t *object) {
    PV_ASSERT(object);

    return object->param->num_channels;
}

pv_status_t PV_MOCKABLE(pv_layer_norm_forward)(pv_layer_norm_t *object, int32_t n, const float *x, float *y) {
    PV_ASSERT(object);
    PV_ASSERT(n);
    PV_ASSERT(x);
    PV_ASSERT(y);
    PV_ORCA_PROFILER_START("layer_norm");

    const int32_t num_channels = object->param->num_channels;
    const q7_t *weight = object->param->weight;
    const q7_t *bias = object->param->bias;

    const float eps = object->param->eps;
    const float norm_mean = (1.f / (float) num_channels);

    for (int32_t i = 0; i < n; i++) {
        const float *xx = x + (i * num_channels);

        float mean_x = 0;
        float mean_x2 = 0;
        for (int32_t j = 0; j < num_channels; j++) {
            mean_x += xx[j];
            mean_x2 += (xx[j] * xx[j]);
        }
        mean_x *= norm_mean;
        mean_x2 *= norm_mean;

        const float var = mean_x2 - (mean_x * mean_x);
        const float norm = 1.f / sqrtf(var + eps);

        float *yy = y + (i * num_channels);
        for (int32_t j = 0; j < num_channels; j++) {
            yy[j] = (((xx[j] - mean_x) * norm) * (1 + (((float) weight[j]) / 128.f))) + (((float) bias[j]) / 128.f);
        }
    }

    PV_ORCA_PROFILER_STOP("layer_norm");
    return PV_STATUS_SUCCESS;
}
