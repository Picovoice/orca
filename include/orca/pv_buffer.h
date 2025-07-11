#ifndef PV_BUFFER_H
#define PV_BUFFER_H

#include <stdbool.h>

#include "core/picovoice.h"
#include "core/pv_type.h"

typedef struct pv_buffer pv_buffer_t;

pv_status_t PV_MOCKABLE(pv_buffer_init)(int32_t dimension, pv_buffer_t **object);

void PV_MOCKABLE(pv_buffer_delete)(pv_buffer_t *object);

void PV_MOCKABLE(pv_buffer_free)(pv_buffer_t *object);

float *PV_MOCKABLE(pv_buffer_get)(pv_buffer_t *object, int32_t requested_length, bool clear);

int32_t PV_MOCKABLE(pv_buffer_dimension)(const pv_buffer_t *object);

int32_t PV_MOCKABLE(pv_buffer_length)(const pv_buffer_t *object);

#endif // PV_BUFFER_H
