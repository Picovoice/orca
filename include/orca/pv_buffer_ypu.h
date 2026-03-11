#ifndef PV_BUFFER_YPU_H
#define PV_BUFFER_YPU_H

#include <stdbool.h>

#include "core/picovoice.h"
#include "core/pv_type.h"
#include "ypu/pv_ypu.h"

typedef struct pv_buffer_ypu pv_buffer_ypu_t;

pv_status_t PV_MOCKABLE(pv_buffer_ypu_init)(
        pv_ypu_t *ypu,
        int32_t dimension,
        pv_buffer_ypu_t **object);

void PV_MOCKABLE(pv_buffer_ypu_delete)(pv_ypu_t *ypu, pv_buffer_ypu_t *object);

void PV_MOCKABLE(pv_buffer_ypu_free)(pv_ypu_t *ypu, pv_buffer_ypu_t *object);

pv_ypu_mem_t *PV_MOCKABLE(pv_buffer_ypu_get)(pv_ypu_t *ypu, pv_buffer_ypu_t *object, int32_t requested_length, bool clear);

int32_t PV_MOCKABLE(pv_buffer_ypu_dimension)(const pv_buffer_ypu_t *object);

int32_t PV_MOCKABLE(pv_buffer_ypu_length)(const pv_buffer_ypu_t *object);

pv_status_t PV_MOCKABLE(pv_buffer_ypu_concat)(
        pv_ypu_t *ypu,
        pv_buffer_ypu_t *object,
        pv_ypu_mem_t *src,
        int32_t src_offset,
        int32_t n);

pv_status_t PV_MOCKABLE(pv_buffer_ypu_replace)(
        pv_ypu_t *ypu,
        pv_buffer_ypu_t *object,
        pv_ypu_mem_t *src,
        int32_t src_offset,
        int32_t n);

pv_status_t PV_MOCKABLE(pv_buffer_ypu_copy_to)(
        pv_ypu_t *ypu,
        const pv_buffer_ypu_t *object,
        pv_ypu_mem_t *dst,
        int32_t dst_offset,
        int32_t n);

#endif // PV_BUFFER_YPU_H
