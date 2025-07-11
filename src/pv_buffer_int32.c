#include <stdlib.h>
#include <string.h>

#include "core/pv_error.h"
#include "core/pv_error_messages.h"
#include "core/pv_type.h"
#include "orca/pv_buffer_int32.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

struct pv_buffer_int32 {
    int32_t chunk_size;
    int32_t num_chunks;
    int32_t num_elements;
    int32_t *buffer;
};

pv_status_t PV_MOCKABLE(pv_buffer_int32_init)(int32_t chunk_size, pv_buffer_int32_t **object) {
    PV_ASSERT(chunk_size > 0);
    PV_ASSERT(object);

    *object = NULL;

    pv_buffer_int32_t *o = calloc(1, sizeof(pv_buffer_int32_t));
    if (!o) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->chunk_size = chunk_size;
    o->num_chunks = 0;
    o->num_elements = 0;
    o->buffer = NULL;

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_buffer_int32_delete)(pv_buffer_int32_t *object) {
    if (object) {
        free(object->buffer);
        free(object);
    }
}

void PV_MOCKABLE(pv_buffer_int32_free)(pv_buffer_int32_t *object) {
    if (object) {
        free(object->buffer);
        object->buffer = NULL;
        object->num_chunks = 0;
        object->num_elements = 0;
    }
}

pv_status_t PV_MOCKABLE(pv_buffer_int32_alloc)(pv_buffer_int32_t *object, int32_t requested_num_chunks, bool clear) {
    PV_ASSERT(object);
    PV_ASSERT(requested_num_chunks > 0);

    if (requested_num_chunks <= object->num_chunks) {
        if (clear) {
            memset(object->buffer, 0, object->chunk_size * requested_num_chunks * sizeof(int32_t));
        }
    } else {
        if (!object->buffer) {
            object->buffer = calloc(object->chunk_size * requested_num_chunks, sizeof(int32_t));
            if (!object->buffer) {
                PV_ERROR_REPORT(
                        &pv_error_msg_alloc,
                        PV_ERROR_ARGS_PUBLIC_EMPTY(),
                        PV_ERROR_ARGS_PRIVATE("object->buffer"));
                return PV_STATUS_OUT_OF_MEMORY;
            }
        } else {
            object->buffer = realloc(
                    object->buffer,
                    object->chunk_size * requested_num_chunks * sizeof(int32_t));
            if (!object->buffer) {
                PV_ERROR_REPORT(
                        &pv_error_msg_alloc,
                        PV_ERROR_ARGS_PUBLIC_EMPTY(),
                        PV_ERROR_ARGS_PRIVATE("object->buffer"));
                return PV_STATUS_OUT_OF_MEMORY;
            }

            if (clear) {
                memset(object->buffer, 0, object->chunk_size * requested_num_chunks * sizeof(int32_t));
            }
        }
    }

    object->num_chunks = requested_num_chunks;

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_buffer_int32_append)(
        pv_buffer_int32_t *object,
        int32_t num_elements,
        const int32_t *elements) {
    PV_ASSERT(object);
    PV_ASSERT(num_elements > 0);
    PV_ASSERT(elements);

    while ((object->num_chunks * object->chunk_size) < (object->num_elements + num_elements)) {
        pv_status_t status = pv_buffer_int32_alloc(object, object->num_chunks + 1, false);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_buffer_int32_alloc,
                    pv_status_to_string(status));
            return status;
        }
    }

    memcpy(object->buffer + object->num_elements, elements, num_elements * sizeof(int32_t));

    object->num_elements += num_elements;

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_buffer_int32_remove_from_start)(pv_buffer_int32_t *object, int32_t num_elements_remove) {
    PV_ASSERT(object);
    PV_ASSERT(num_elements_remove > 0);

    int32_t *pruned_elements = calloc(object->chunk_size * object->num_chunks, sizeof(int32_t));
    if (!pruned_elements) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("pruned_elements"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memcpy(
            pruned_elements,
            object->buffer + num_elements_remove,
            (object->num_elements - num_elements_remove) * sizeof(int32_t));

    free(object->buffer);
    object->buffer = pruned_elements;

    object->num_elements -= num_elements_remove;

    return PV_STATUS_SUCCESS;
}

int32_t *PV_MOCKABLE(pv_buffer_int32_get)(pv_buffer_int32_t *object) {
    PV_ASSERT(object);

    return object->buffer;
}

int32_t PV_MOCKABLE(pv_buffer_int32_length)(pv_buffer_int32_t *object) {
    PV_ASSERT(object);

    return object->num_elements;
}
