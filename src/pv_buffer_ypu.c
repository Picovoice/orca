#include <string.h>

#include "core/pv_error_messages.h"
#include "core/pv_type.h"
#include "orca/pv_buffer_ypu.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

struct pv_buffer_ypu {
    int32_t dimension;
    int32_t length;
    pv_ypu_mem_t *buffer;
};

pv_status_t PV_MOCKABLE(pv_buffer_ypu_init)(
        pv_ypu_t *ypu,
        int32_t dimension,
        pv_buffer_ypu_t **object) {
    PV_ASSERT(dimension > 0);
    PV_ASSERT(object);

    *object = NULL;

    (void) ypu;

    pv_buffer_ypu_t *o = pv_ypu_host_alloc(ypu, sizeof(pv_buffer_ypu_t));
    if (!o) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_host_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(o, 0, sizeof(pv_buffer_ypu_t));

    o->dimension = dimension;
    o->length = 0;
    o->buffer = NULL;

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_buffer_ypu_delete)(pv_ypu_t *ypu, pv_buffer_ypu_t *object) {
    if (object) {
        pv_ypu_mem_free(ypu, object->buffer);
        pv_ypu_host_free(ypu, object);
    }
}

void PV_MOCKABLE(pv_buffer_ypu_free)(pv_ypu_t *ypu, pv_buffer_ypu_t *object) {
    if (object) {
        pv_ypu_mem_free(ypu, object->buffer);
        object->buffer = NULL;
        object->length = 0;
    }
}

pv_ypu_mem_t *PV_MOCKABLE(pv_buffer_ypu_get)(pv_ypu_t *ypu, pv_buffer_ypu_t *object, int32_t requested_length, bool clear) {
    PV_ASSERT(object);
    PV_ASSERT(requested_length > 0);

    if (requested_length <= object->length) {
        if (clear) {
            pv_ypu_op_memset_args_t args_memset = {
                    .output = object->buffer,
                    .size_bytes = requested_length * object->dimension * (int32_t) sizeof(float),
                    .output_offset = 0,
            };
            pv_status_t status = pv_ypu_operator_execute(
                    ypu,
                    PV_YPU_OPERATOR_MEMSET,
                    &args_memset);
            if (status != PV_STATUS_SUCCESS) {
                PV_ERROR_REPORT(
                        &pv_error_msg_ypu_operator_fail,
                        PV_ERROR_ARGS_PUBLIC("execute"),
                        PV_ERROR_ARGS_PRIVATE(
                                "execute",
                                pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_MEMSET),
                                pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                                pv_status_to_string(status)));
                return NULL;
            }
        }
        return object->buffer;
    }

    if (object->length > 0) {
        pv_ypu_mem_free(ypu, object->buffer);
        object->buffer = NULL;
        object->length = 0;
    }

    object->buffer = pv_ypu_mem_alloc(
            ypu,
            requested_length * object->dimension * (int32_t) sizeof(float),
            PV_YPU_DEVICE_MEM_FLAG_NONE);
    if (!object->buffer) {
        return NULL;
    }

    if (clear) {
        pv_ypu_op_memset_args_t args_memset = {
                .output = object->buffer,
                .size_bytes = requested_length * object->dimension * (int32_t) sizeof(float),
                .output_offset = 0,
        };
        pv_status_t status = pv_ypu_operator_execute(
                ypu,
                PV_YPU_OPERATOR_MEMSET,
                &args_memset);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT(
                    &pv_error_msg_ypu_operator_fail,
                    PV_ERROR_ARGS_PUBLIC("execute"),
                    PV_ERROR_ARGS_PRIVATE(
                            "execute",
                            pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_MEMSET),
                            pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                            pv_status_to_string(status)));
            return NULL;
        }
    }

    object->length = requested_length;

    return object->buffer;
}

int32_t PV_MOCKABLE(pv_buffer_ypu_dimension)(const pv_buffer_ypu_t *object) {
    PV_ASSERT(object);

    return object->dimension;
}

int32_t PV_MOCKABLE(pv_buffer_ypu_length)(const pv_buffer_ypu_t *object) {
    PV_ASSERT(object);

    return object->length;
}

pv_status_t PV_MOCKABLE(pv_buffer_ypu_concat)(
        pv_ypu_t *ypu,
        pv_buffer_ypu_t *object,
        pv_ypu_mem_t *src_ypu,
        int32_t src_offset,
        int32_t n) {
    PV_ASSERT(object);
    if (n > 0) {
        PV_ASSERT(src_ypu);
    }
    PV_ASSERT(n >= 0);

    if (n == 0) {
        return PV_STATUS_SUCCESS;
    }

    pv_ypu_mem_t *result_ypu = pv_ypu_mem_alloc(
            ypu,
            (object->length + n) * object->dimension * (int32_t) sizeof(float),
            PV_YPU_DEVICE_MEM_FLAG_NONE);
    if (!result_ypu) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    if (object->length > 0) {
        pv_ypu_op_memcpy_args_t memcpy_args = {
                .output = result_ypu,
                .input = object->buffer,
                .size_bytes = object->length * object->dimension * (int32_t) sizeof(float),
                .output_offset = 0,
                .input_offset = 0,
        };
        pv_status_t status = pv_ypu_operator_execute(
                ypu,
                PV_YPU_OPERATOR_MEMCPY,
                &memcpy_args);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT(
                    &pv_error_msg_ypu_operator_fail,
                    PV_ERROR_ARGS_PUBLIC("execute"),
                    PV_ERROR_ARGS_PRIVATE(
                            "execute",
                            pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_MEMCPY),
                            pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                            pv_status_to_string(status)));
            pv_ypu_mem_free(ypu, result_ypu);
            return status;
        }
    }

    pv_ypu_op_memcpy_args_t memcpy_args = {
            .output = result_ypu,
            .input = src_ypu,
            .size_bytes = n * object->dimension * (int32_t) sizeof(float),
            .output_offset = object->length * object->dimension * (int32_t) sizeof(float),
            .input_offset = src_offset * (int32_t) sizeof(float),
    };
    pv_status_t status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_MEMCPY,
            &memcpy_args);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_MEMCPY),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        pv_ypu_mem_free(ypu, result_ypu);
        return status;
    }

    pv_ypu_mem_free(ypu, object->buffer);
    object->buffer = result_ypu;
    object->length = object->length + n;

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_buffer_ypu_replace)(
        pv_ypu_t *ypu,
        pv_buffer_ypu_t *object,
        pv_ypu_mem_t *src_ypu,
        int32_t src_offset,
        int32_t n) {
    PV_ASSERT(object);
    if (n > 0) {
        PV_ASSERT(src_ypu);
    }
    PV_ASSERT(n >= 0);

    pv_buffer_ypu_free(ypu, object);

    if (n == 0) {
        return PV_STATUS_SUCCESS;
    }

    pv_ypu_mem_t *result_ypu = pv_ypu_mem_alloc(
            ypu,
            n * object->dimension * (int32_t) sizeof(float),
            PV_YPU_DEVICE_MEM_FLAG_NONE);
    if (!result_ypu) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_op_memcpy_args_t memcpy_args = {
            .output = result_ypu,
            .input = src_ypu,
            .size_bytes = n * object->dimension * (int32_t) sizeof(float),
            .output_offset = 0,
            .input_offset = src_offset * (int32_t) sizeof(float),
    };
    pv_status_t status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_MEMCPY,
            &memcpy_args);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_MEMCPY),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        pv_ypu_mem_free(ypu, result_ypu);
        return status;
    }

    object->buffer = result_ypu;
    object->length = n;

    return PV_STATUS_SUCCESS;
}

pv_status_t PV_MOCKABLE(pv_buffer_ypu_copy_to)(
        pv_ypu_t *ypu,
        const pv_buffer_ypu_t *object,
        pv_ypu_mem_t *dst_ypu,
        int32_t dst_offset,
        int32_t n) {
    PV_ASSERT(object);
    PV_ASSERT(dst_ypu);
    PV_ASSERT(n > 0);

    pv_ypu_op_memcpy_args_t memcpy_args = {
            .output = dst_ypu,
            .input = object->buffer,
            .size_bytes = n * object->dimension * (int32_t) sizeof(float),
            .output_offset = dst_offset * (int32_t) sizeof(float),
            .input_offset = 0,
    };
    pv_status_t status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_MEMCPY,
            &memcpy_args);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_MEMCPY),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
    }
    return status;
}
