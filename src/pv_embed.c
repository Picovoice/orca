#include <stdlib.h>
#include <string.h>

#include "core/pv_error_messages.h"
#include "math/pv_math.h"
#include "orca/pv_embed.h"
#include "util/pv_check_status.h"
#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_embed_param_serialize_buffer)(const pv_embed_param_t *param, size_t *length, void **buffer) {
    PV_ASSERT(param);

    *length =
            sizeof(int32_t) +
            sizeof(int32_t) +
            (sizeof(float) * param->num_embeddings * param->output_channels);
    *buffer = NULL;

    void *b = malloc(*length);
    PV_CHECK_ALLOC(b);
    *buffer = b;

    memcpy(b, &(param->num_embeddings), sizeof(int32_t));
    b += sizeof(int32_t);

    memcpy(b, &(param->output_channels), sizeof(int32_t));
    b += sizeof(int32_t);

    memcpy(b, param->weight, sizeof(float) * param->num_embeddings * param->output_channels);

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_embed_param_serialize)(const pv_embed_param_t *param, FILE *file) {
    PV_ASSERT(param);
    PV_ASSERT(file);

    size_t length = 0;
    void *buffer = NULL;

    pv_status_t status = pv_embed_param_serialize_buffer(param, &length, &buffer);
    PV_CHECK_STATUS(status);

    const size_t count = fwrite(buffer, 1, length, file);
    free(buffer);

    return (count == length) ? PV_STATUS_SUCCESS : PV_STATUS_IO_ERROR;
}

#endif

pv_status_t PV_MOCKABLE(pv_embed_param_load)(FILE *f, pv_embed_param_t **param) {
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_embed_param_t *p = calloc(1, sizeof(pv_embed_param_t));
    PV_CHECK_ALLOC(p);

    size_t count = pv_fread(&(p->num_embeddings), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_embed_param_delete(p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->num_embeddings <= 0) {
        pv_embed_param_delete(p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    count = pv_fread(&(p->output_channels), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_embed_param_delete(p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->output_channels <= 0) {
        pv_embed_param_delete(p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    const size_t num_weight_params = p->num_embeddings * p->output_channels;
    p->weight = malloc(sizeof(float) * num_weight_params);
    if (!p->weight) {
        pv_embed_param_delete(p);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    count = pv_fread((float *) (p->weight), sizeof(float), num_weight_params, f);
    if (count != (size_t) num_weight_params) {
        pv_embed_param_delete(p);
        return PV_STATUS_IO_ERROR;
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}


void PV_MOCKABLE(pv_embed_param_delete)(pv_embed_param_t *param) {
    if (param) {
        free((float *) (param->weight));

        free(param);
    }
}

bool PV_MOCKABLE(pv_embed_param_is_equal)(
        const pv_embed_param_t *object,
        const pv_embed_param_t *other) {
    PV_ASSERT(object);
    PV_ASSERT(other);

    if (object->num_embeddings != other->num_embeddings) {
        return false;
    }

    if (object->output_channels != other->output_channels) {
        return false;
    }

    for (int32_t i = 0; i < object->num_embeddings * object->output_channels; i++) {
        if (object->weight[i] != other->weight[i]) {
            return false;
        }
    }

    return true;
}


struct pv_embed {
    const pv_embed_param_t *param;
};

pv_status_t PV_MOCKABLE(pv_embed_init)(const pv_embed_param_t *param, pv_embed_t **object) {
    PV_ASSERT(param);
    PV_ASSERT(object);

    *object = NULL;

    pv_embed_t *o = calloc(1, sizeof(pv_embed_t));
    if (!o) {
        pv_embed_delete(o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->param = param;

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_embed_delete)(pv_embed_t *object) {
    if (object) {
        free(object);
    }
}

int32_t PV_MOCKABLE(pv_embed_dimension)(const pv_embed_t *object) {
    PV_ASSERT(object);

    return object->param->output_channels;
}

int32_t PV_MOCKABLE(pv_embed_num_embeddings)(const pv_embed_t *object) {
    PV_ASSERT(object);

    return object->param->num_embeddings;
}

void PV_MOCKABLE(pv_embed_get)(const pv_embed_t *object, int32_t index, float *y) {
    PV_ASSERT(object);
    PV_ASSERT(index >= 0);
    PV_ASSERT(y);
    PV_ASSERT(index < object->param->num_embeddings);

    int32_t dimension = object->param->output_channels;
    memcpy(y, object->param->weight + (index * dimension), sizeof(float) * dimension);
}

pv_status_t PV_MOCKABLE(pv_embed_forward)(const pv_embed_t *object, int32_t n, const int32_t *x, float *y) {
    PV_ASSERT(object);
    PV_ASSERT(n);
    PV_ASSERT(x);
    PV_ASSERT(y);

    int32_t dimension = object->param->output_channels;
    for (int32_t i = 0; i < n; i++) {
        pv_embed_get(object, x[i], y + (i * dimension));
    }

    return PV_STATUS_SUCCESS;
}
