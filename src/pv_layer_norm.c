#include <stdlib.h>
#include <string.h>

#include "core/pv_error_messages.h"
#include "math/pv_math.h"
#include "orca/pv_affine.h"
#include "orca/pv_layer_norm.h"
#include "orca/pv_profiler.h"
#include "util/pv_check_status.h"
#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_layer_norm_param_serialize_buffer)(
        pv_ypu_t *ypu,
        const pv_layer_norm_param_t *param,
        size_t *length,
        bool elementwise_affine,
        void **buffer) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);

    *length = sizeof(int32_t) + sizeof(float);
    if (elementwise_affine) {
        *length += (sizeof(float) * param->num_channels) + (sizeof(float) * param->num_channels);
    }
    *buffer = NULL;

    void *b = pv_ypu_host_alloc(ypu, (int32_t) (*length));
    if (!b) {
        return PV_STATUS_OUT_OF_MEMORY;
    }
    *buffer = b;

    memcpy(b, &(param->num_channels), sizeof(int32_t));
    b += sizeof(int32_t);

    memcpy(b, &(param->eps), sizeof(float));
    b += sizeof(float);

    if (elementwise_affine) {
        memcpy(b, param->weight->data, sizeof(float) * param->num_channels);
        b += sizeof(float) * param->num_channels;

        memcpy(b, param->bias->data, sizeof(float) * param->num_channels);
    }

    return PV_STATUS_SUCCESS;
}


pv_status_t PV_MOCKABLE(pv_layer_norm_param_serialize)(
        pv_ypu_t *ypu,
        const pv_layer_norm_param_t *param,
        bool elementwise_affine,
        FILE *file) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(file);

    size_t length = 0;
    void *buffer = NULL;

    pv_status_t status = pv_layer_norm_param_serialize_buffer(
            ypu,
            param,
            &length,
            elementwise_affine,
            &buffer);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                pv_layer_norm_param_serialize_buffer,
                pv_status_to_string(status));
        return status;
    }

    const size_t count = fwrite(buffer, 1, length, file);
    pv_ypu_host_free(ypu, buffer);

    return (count == length) ? PV_STATUS_SUCCESS : PV_STATUS_IO_ERROR;
}

#endif

pv_status_t PV_MOCKABLE(pv_layer_norm_param_load)(
        pv_ypu_t *ypu,
        FILE *f,
        bool elementwise_affine,
        pv_layer_norm_param_t **param) {
    PV_ASSERT(ypu);
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_layer_norm_param_t *p = pv_ypu_host_alloc(ypu, sizeof(pv_layer_norm_param_t));
    if (!p) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(p, 0, sizeof(pv_layer_norm_param_t));

    size_t count = pv_fread(&(p->num_channels), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_layer_norm_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->num_channels <= 0) {
        pv_layer_norm_param_delete(ypu, p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    count = pv_fread(&(p->eps), sizeof(float), 1, f);
    if (count != 1) {
        pv_layer_norm_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->eps <= 0) {
        pv_layer_norm_param_delete(ypu, p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    p->elementwise_affine = elementwise_affine;

    p->weight = NULL;
    p->bias = NULL;
    if (elementwise_affine) {
        p->weight = pv_ypu_config_mem_alloc(
                ypu,
                ((int32_t) sizeof(float)) * p->num_channels,
                PV_YPU_DEVICE_MEM_FLAG_STATIC);
        if (!p->weight) {
            pv_layer_norm_param_delete(ypu, p);
            return PV_STATUS_OUT_OF_MEMORY;
        }
        count = pv_fread((float *) (p->weight->data), sizeof(float), p->num_channels, f);
        if ((int32_t) count != (p->num_channels)) {
            pv_layer_norm_param_delete(ypu, p);
            return PV_STATUS_IO_ERROR;
        }

        p->bias = pv_ypu_config_mem_alloc(
                ypu,
                ((int32_t) sizeof(float)) * p->num_channels,
                PV_YPU_DEVICE_MEM_FLAG_STATIC);
        if (!p->bias) {
            pv_layer_norm_param_delete(ypu, p);
            return PV_STATUS_OUT_OF_MEMORY;
        }
        count = pv_fread((float *) (p->bias->data), sizeof(float), p->num_channels, f);
        if ((int32_t) count != p->num_channels) {
            pv_layer_norm_param_delete(ypu, p);
            return PV_STATUS_IO_ERROR;
        }
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}


void PV_MOCKABLE(pv_layer_norm_param_delete)(
        pv_ypu_t *ypu,
        pv_layer_norm_param_t *param) {
    PV_ASSERT(ypu);

    if (param) {
        if (param->elementwise_affine) {
            pv_ypu_config_mem_free(ypu, param->weight);
            pv_ypu_config_mem_free(ypu, param->bias);
        }
        pv_ypu_host_free(ypu, param);
    }
}

bool PV_MOCKABLE(pv_layer_norm_param_is_equal)(
        const pv_layer_norm_param_t *object,
        const pv_layer_norm_param_t *other) {
    PV_ASSERT(object);
    PV_ASSERT(other);

    if (object->num_channels != other->num_channels) {
        return false;
    }

    if (object->eps != other->eps) {
        return false;
    }

    if ((object->bias != NULL) && (other->bias != NULL)) {
        if (!pv_ypu_config_mem_is_equal(object->bias, other->bias)) {
            return false;
        }
    } else if ((object->bias != NULL) || (other->bias != NULL)){
        return false;
    }

    if ((object->weight != NULL) && (other->weight != NULL)) {
        if (!pv_ypu_config_mem_is_equal(object->weight, other->weight)) {
            return false;
        }
    } else if ((object->weight != NULL) || (other->weight != NULL)){
        return false;
    }

    return true;
}

struct pv_layer_norm {
    const pv_layer_norm_param_t *param;

    pv_ypu_mem_t *weight;
    pv_ypu_mem_t *bias;
};

pv_status_t PV_MOCKABLE(pv_layer_norm_init)(
        pv_ypu_t *ypu,
        const pv_layer_norm_param_t *param,
        pv_layer_norm_t **object) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(object);

    *object = NULL;

    pv_layer_norm_t *o = pv_ypu_host_alloc(ypu, sizeof(pv_layer_norm_t));
    if (!o) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(o, 0, sizeof(pv_layer_norm_t));

    o->param = param;

    o->weight = NULL;
    o->bias = NULL;
    if (param->elementwise_affine) {
        o->weight = pv_ypu_mem_from_config(
                ypu,
                param->weight);
        if (!o->weight) {
            pv_layer_norm_delete(ypu, o);
            return PV_STATUS_OUT_OF_MEMORY;
        }

        o->bias = pv_ypu_mem_from_config(
                ypu,
                param->bias);
        if (!o->bias) {
            pv_layer_norm_delete(ypu, o);
            return PV_STATUS_OUT_OF_MEMORY;
        }
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_layer_norm_delete)(
        pv_ypu_t *ypu,
        pv_layer_norm_t *object) {
    PV_ASSERT(ypu);

    if (object) {
        if (object->param->elementwise_affine) {
            pv_ypu_mem_free(ypu, object->weight);
            pv_ypu_mem_free(ypu, object->bias);
        }
        pv_ypu_host_free(ypu, object);
    }
}

int32_t PV_MOCKABLE(pv_layer_norm_num_channels)(const pv_layer_norm_t *object) {
    PV_ASSERT(object);

    return object->param->num_channels;
}

pv_status_t PV_MOCKABLE(pv_layer_norm_forward)(
        pv_ypu_t *ypu,
        pv_layer_norm_t *object,
        int32_t n,
        pv_ypu_mem_t *x_ypu,
        pv_ypu_mem_t *y_ypu) {
    PV_ASSERT(ypu);
    PV_ASSERT(object);
    PV_ASSERT(n);
    PV_ASSERT(x_ypu);
    PV_ASSERT(y_ypu);

    const int32_t num_channels = object->param->num_channels;

    pv_ypu_op_layer_norm_non_affine_args_t layer_norm_args = {
            .output = y_ypu,
            .input = x_ypu,
            .m = n,
            .n = num_channels,
            .eps = object->param->eps,
            .output_offset = 0,
            .input_offset = 0
    };
    pv_status_t status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_LAYER_NORM_NON_AFFINE,
            &layer_norm_args);
    if (status != PV_STATUS_SUCCESS) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_operator_fail,
                PV_ERROR_ARGS_PUBLIC("execute"),
                PV_ERROR_ARGS_PRIVATE(
                        "execute",
                        pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_LAYER_NORM_NON_AFFINE),
                        pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                        pv_status_to_string(status)));
        return status;
    }

    if (object->param->elementwise_affine) {
        status = pv_affine_execute(
                ypu,
                n,
                num_channels,
                y_ypu,
                1.0f,
                1.0f,
                object->weight,
                object->bias,
                y_ypu,
                0,
                0,
                0,
                0);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_affine_execute,
                    pv_status_to_string(status));
            return status;
        }
    }

    return PV_STATUS_SUCCESS;
}
