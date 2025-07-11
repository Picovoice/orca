#include <memory.h>
#include <stdlib.h>

#include "core/pv_type.h"
#include "orca/pv_buffer_q510.h"
#include "orca/pv_profiler.h"

#ifdef __PV_MOCKS__

#include "core/mock/pv_core_mock.h"

#endif

struct pv_buffer_q510 {
    int32_t dimension;
    int32_t length;
    q510_t *buffer;
};

pv_status_t PV_MOCKABLE(pv_buffer_q510_init)(int32_t dimension, pv_buffer_q510_t **object) {
    PV_ASSERT(dimension > 0);
    PV_ASSERT(object);

    *object = NULL;

    pv_buffer_q510_t *o = calloc(1, sizeof(pv_buffer_q510_t));
    if (!o) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->dimension = dimension;
    o->length = 0;
    o->buffer = NULL;

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_buffer_q510_delete)(pv_buffer_q510_t *object) {
    if (object) {
        free(object->buffer);
        free(object);
    }
}

void PV_MOCKABLE(pv_buffer_q510_free)(pv_buffer_q510_t *object) {
    if (object) {
        free(object->buffer);
        object->buffer = NULL;
        object->length = 0;
    }
}

q510_t *PV_MOCKABLE(pv_buffer_q510_get)(pv_buffer_q510_t *object, int32_t requested_length, bool clear) {
    PV_ASSERT(object);
    PV_ASSERT(requested_length > 0);

    if (requested_length <= object->length) {
        return clear ? memset(object->buffer, 0, requested_length * object->dimension * sizeof(q510_t)) : object->buffer;
    }

    if (object->length > 0) {
        free(object->buffer);
        object->buffer = NULL;
        object->length = 0;
    }

    PV_ORCA_PROFILER_START("alloc");
    if (clear) {
        object->buffer = calloc(requested_length * object->dimension, sizeof(q510_t));
    } else {
        object->buffer = malloc(requested_length * object->dimension * sizeof(q510_t));
    }
    PV_ORCA_PROFILER_STOP("alloc");
    if (!object->buffer) {
        return NULL;
    }
    object->length = requested_length;

    return object->buffer;
}
