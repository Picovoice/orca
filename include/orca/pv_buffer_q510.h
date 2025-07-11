#ifndef PV_BUFFER_Q510_H
#define PV_BUFFER_Q510_H

#include <stdbool.h>

#include "core/picovoice.h"
#include "core/pv_type.h"

typedef struct pv_buffer_q510 pv_buffer_q510_t;

pv_status_t PV_MOCKABLE(pv_buffer_q510_init)(int32_t dimension, pv_buffer_q510_t **object);

void PV_MOCKABLE(pv_buffer_q510_delete)(pv_buffer_q510_t *object);

void PV_MOCKABLE(pv_buffer_q510_free)(pv_buffer_q510_t *object);

q510_t *PV_MOCKABLE(pv_buffer_q510_get)(pv_buffer_q510_t *object, int32_t requested_length, bool clear);

#endif // PV_BUFFER_Q510_H
