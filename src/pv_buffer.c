#include <memory.h>
#include <stdio.h>
#include <stdlib.h>

#include "core/pv_type.h"
#include "orca/pv_buffer.h"
#include "orca/pv_profiler.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

struct pv_buffer {
    int32_t dimension;
    int32_t length;
    void *buffer;
};

pv_status_t PV_MOCKABLE(pv_buffer_init)(int32_t dimension, pv_buffer_t **object) {
    PV_ASSERT(dimension > 0);
    PV_ASSERT(object);

    *object = NULL;

    pv_buffer_t *o = calloc(1, sizeof(pv_buffer_t));
    if (!o) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->dimension = dimension;
    o->length = 0;
    o->buffer = NULL;

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_buffer_delete)(pv_buffer_t *object) {
    if (object) {
        free(object->buffer);
        free(object);
    }
}

void PV_MOCKABLE(pv_buffer_free)(pv_buffer_t *object) {
    if (object) {
        free(object->buffer);
        object->buffer = NULL;
        object->length = 0;
    }
}

void *PV_MOCKABLE(pv_buffer_get)(pv_buffer_t *object, int32_t requested_length, bool clear) {
    PV_ASSERT(object);
    PV_ASSERT(requested_length > 0);

    if (requested_length <= object->length) {
        return clear ? memset(object->buffer, 0, requested_length * object->dimension) : object->buffer;
    }

    if (object->length > 0) {
        free(object->buffer);
        object->buffer = NULL;
        object->length = 0;
    }

    if (clear) {
        object->buffer = calloc(requested_length * object->dimension, sizeof(char));
    } else {
        object->buffer = malloc(requested_length * object->dimension);
    }
    if (!object->buffer) {
        return NULL;
    }
    object->length = requested_length;

    return object->buffer;
}

int32_t PV_MOCKABLE(pv_buffer_dimension)(const pv_buffer_t *object) {
    PV_ASSERT(object);

    return object->dimension;
}

int32_t PV_MOCKABLE(pv_buffer_length)(const pv_buffer_t *object) {
    PV_ASSERT(object);

    return object->length;
}

pv_status_t PV_MOCKABLE(pv_buffer_concat)(
        pv_buffer_t *object,
        const void *src,
        int32_t n) {
    PV_ASSERT(object);
    if (n > 0) {
        PV_ASSERT(src);
    }
    PV_ASSERT(n >= 0);

    if (n == 0) {
        return PV_STATUS_SUCCESS;
    }

    void *result = malloc((object->length + n) * object->dimension);
    if (!result) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    if (object->length > 0) {
        memcpy(
                result,
                object->buffer,
                object->length * object->dimension);
    }

    memcpy(
            result + object->length * object->dimension,
            src,
            n * object->dimension);

    free(object->buffer);
    object->buffer = result;
    object->length = object->length + n;

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_buffer_replace)(
        pv_buffer_t *object,
        const void *src,
        int32_t n) {
    PV_ASSERT(object);
    if (n > 0) {
        PV_ASSERT(src);
    }
    PV_ASSERT(n >= 0);

    pv_buffer_free(object);

    if (n == 0) {
        return PV_STATUS_SUCCESS;
    }

    float *result = malloc(n * object->dimension);
    if (!result) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memcpy(
            result,
            src,
            n * object->dimension);

    object->buffer = result;
    object->length = n;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_buffer_copy_to)(
        const pv_buffer_t *object,
        void *dest,
        int32_t n) {
    PV_ASSERT(object);
    PV_ASSERT(dest);
    PV_ASSERT(n > 0);

    memcpy(
            dest,
            object->buffer,
            n * object->dimension);
}
