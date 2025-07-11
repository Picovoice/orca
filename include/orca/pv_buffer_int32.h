#ifndef PV_BUFFER_INT32_H
#define PV_BUFFER_INT32_H

#include <stdbool.h>

#include "core/picovoice.h"
#include "core/pv_type.h"

typedef struct pv_buffer_int32 pv_buffer_int32_t;

pv_status_t PV_MOCKABLE(pv_buffer_int32_init)(int32_t chunk_size, pv_buffer_int32_t **object);

void PV_MOCKABLE(pv_buffer_int32_delete)(pv_buffer_int32_t *object);

void PV_MOCKABLE(pv_buffer_int32_free)(pv_buffer_int32_t *object);

pv_status_t PV_MOCKABLE(pv_buffer_int32_alloc)(pv_buffer_int32_t *object, int32_t requested_num_chunks, bool clear);

pv_status_t PV_MOCKABLE(pv_buffer_int32_append)(
        pv_buffer_int32_t *object,
        int32_t num_elements,
        const int32_t *elements);

pv_status_t PV_MOCKABLE(pv_buffer_int32_remove_from_start)(pv_buffer_int32_t *object, int32_t num_elements_remove);

int32_t *PV_MOCKABLE(pv_buffer_int32_get)(pv_buffer_int32_t *object);

int32_t PV_MOCKABLE(pv_buffer_int32_length)(pv_buffer_int32_t *object);

#endif // PV_BUFFER_INT32_H
