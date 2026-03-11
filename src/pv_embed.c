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

pv_status_t PV_MOCKABLE(pv_embed_param_serialize_buffer)(
        pv_ypu_t *ypu,
        const pv_embed_param_t *param,
        size_t *length,
        void **buffer) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);

    *length =
            sizeof(int32_t) +
            sizeof(int32_t) +
            (sizeof(float) * param->num_embeddings * param->output_channels);
    *buffer = NULL;

    void *b = pv_ypu_host_alloc(ypu, (int32_t) (*length));
    if (!b) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_host_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("b"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    *buffer = b;

    memcpy(b, &(param->num_embeddings), sizeof(int32_t));
    b += sizeof(int32_t);

    memcpy(b, &(param->output_channels), sizeof(int32_t));
    b += sizeof(int32_t);

    memcpy(b, param->weight, sizeof(float) * param->num_embeddings * param->output_channels);

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_embed_param_serialize)(pv_ypu_t *ypu, const pv_embed_param_t *param, FILE *file) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(file);

    size_t length = 0;
    void *buffer = NULL;

    pv_status_t status = pv_embed_param_serialize_buffer(ypu, param, &length, &buffer);
    PV_CHECK_STATUS(status);

    const size_t count = fwrite(buffer, 1, length, file);
    pv_ypu_host_free(ypu, buffer);

    return (count == length) ? PV_STATUS_SUCCESS : PV_STATUS_IO_ERROR;
}

#endif

pv_status_t PV_MOCKABLE(pv_embed_param_load)(pv_ypu_t *ypu, FILE *f, pv_embed_param_t **param) {
    PV_ASSERT(ypu);
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_embed_param_t *p = pv_ypu_host_alloc(ypu, sizeof(pv_embed_param_t));
    if (!p) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_host_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(p, 0, sizeof(pv_embed_param_t));

    size_t count = pv_fread(&(p->num_embeddings), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_embed_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->num_embeddings <= 0) {
        pv_embed_param_delete(ypu, p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    count = pv_fread(&(p->output_channels), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_embed_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->output_channels <= 0) {
        pv_embed_param_delete(ypu, p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    const size_t num_weight_params = p->num_embeddings * p->output_channels;
    p->weight = pv_ypu_host_alloc(ypu, (int32_t) (sizeof(float) * num_weight_params));
    if (!p->weight) {
        pv_embed_param_delete(ypu, p);
        return PV_STATUS_OUT_OF_MEMORY;
    }
    count = pv_fread((float *) (p->weight), sizeof(float), num_weight_params, f);
    if (count != (size_t) num_weight_params) {
        pv_embed_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}


void PV_MOCKABLE(pv_embed_param_delete)(pv_ypu_t *ypu, pv_embed_param_t *param) {
    PV_ASSERT(ypu);

    if (param) {
        pv_ypu_host_free(ypu, (float *) (param->weight));
        pv_ypu_host_free(ypu, param);
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

pv_status_t PV_MOCKABLE(pv_embed_init)(pv_ypu_t *ypu, const pv_embed_param_t *param, pv_embed_t **object) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(object);

    *object = NULL;

    pv_embed_t *o = pv_ypu_host_alloc(ypu, sizeof(pv_embed_t));
    if (o == NULL) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(o, 0, sizeof(pv_embed_t));

    o->param = param;

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_embed_delete)(pv_ypu_t *ypu, pv_embed_t *object) {
    PV_ASSERT(ypu);

    if (object) {
        pv_ypu_host_free(ypu, object);
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

pv_status_t PV_MOCKABLE(pv_embed_get)(
        pv_ypu_t *ypu,
        const pv_embed_t *object,
        int32_t index,
        pv_ypu_mem_t *y_ypu_mem,
        int32_t y_offset) {
    PV_ASSERT(ypu);
    PV_ASSERT(object);
    PV_ASSERT(index >= 0);
    PV_ASSERT(y_ypu_mem);
    PV_ASSERT(index < object->param->num_embeddings);

    int32_t dimension = object->param->output_channels;
    pv_status_t status = pv_ypu_mem_copy_to(
            ypu,
            y_ypu_mem,
            object->param->weight + (index * dimension),
            y_offset,
            (int32_t) sizeof(float) * dimension);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_embed_forward)(
        pv_ypu_t *ypu,
        const pv_embed_t *object,
        int32_t n,
        const int32_t *x,
        pv_ypu_mem_t *y_ypu_mem,
        int32_t y_offset) {
    PV_ASSERT(ypu);
    PV_ASSERT(object);
    PV_ASSERT(n);
    PV_ASSERT(x);
    PV_ASSERT(y_ypu_mem);

    int32_t dimension = object->param->output_channels;
    for (int32_t i = 0; i < n; i++) {
        pv_status_t status = pv_embed_get(
                ypu,
                object,
                x[i],
                y_ypu_mem,
                y_offset + ((i * dimension) * (int32_t) sizeof(float)));
        if (status != PV_STATUS_SUCCESS) {
            return status;
        }
    }

    return PV_STATUS_SUCCESS;
}
