#include <string.h>

#include "orca/pv_orca_flow.h"
#include "orca/pv_orca_util.h"
#include "orca/pv_profiler.h"
#include "util/pv_check_status.h"
#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_orca_flow_param_serialize)(pv_ypu_t *ypu, const pv_orca_flow_param_t *param, FILE *file) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(file);

    size_t count = fwrite(&(param->num_flows), sizeof(int32_t), 1, file);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

    pv_status_t status;
    for (int32_t i = 0; i < param->num_flows; i++) {
        status = pv_residual_coupling_param_serialize(ypu, param->flows_param[i], file);
        if (status != PV_STATUS_SUCCESS) {
            return status;
        }
    }

    return PV_STATUS_SUCCESS;
}

#endif

pv_status_t PV_MOCKABLE(pv_orca_flow_param_load)(pv_ypu_t *ypu, FILE *f, pv_orca_flow_param_t **param) {
    PV_ASSERT(ypu);
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_orca_flow_param_t *p = pv_ypu_host_alloc(ypu, sizeof(pv_orca_flow_param_t));
    PV_CHECK_ALLOC(p);

    memset(p, 0, sizeof(pv_orca_flow_param_t));

    size_t count = pv_fread(&(p->num_flows), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_orca_flow_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->num_flows <= 0) {
        pv_orca_flow_param_delete(ypu, p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    p->flows_param = pv_ypu_host_alloc(ypu, p->num_flows * (int32_t) sizeof(pv_residual_coupling_param_t *));
    if (!(p->flows_param)) {
        pv_orca_flow_param_delete(ypu, p);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(p->flows_param, 0, p->num_flows * (int32_t) sizeof(pv_residual_coupling_param_t *));

    pv_status_t status;
    for (int32_t i = 0; i < p->num_flows; i++) {
        status = pv_residual_coupling_param_load(ypu, f, (pv_residual_coupling_param_t **) &(p->flows_param[i]));
        if (status != PV_STATUS_SUCCESS) {
            pv_orca_flow_param_delete(ypu, p);
            return status;
        }
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}


void PV_MOCKABLE(pv_orca_flow_param_delete)(pv_ypu_t *ypu, pv_orca_flow_param_t *param) {
    PV_ASSERT(ypu);

    if (param) {
        if (param->flows_param) {
            for (int32_t i = param->num_flows - 1; i >= 0; i--) {
                pv_residual_coupling_param_delete(ypu, (pv_residual_coupling_param_t *) param->flows_param[i]);
            }

            pv_ypu_host_free(ypu, param->flows_param);
        }

        pv_ypu_host_free(ypu, param);
    }
}

bool PV_MOCKABLE(pv_orca_flow_param_is_equal)(
        const pv_orca_flow_param_t *object,
        const pv_orca_flow_param_t *other) {
    PV_ASSERT(object);
    PV_ASSERT(other);

    if (object->num_flows != other->num_flows) {
        return false;
    }

    for (int32_t i = 0; i < object->num_flows; i++) {
        if (!pv_residual_coupling_param_is_equal(object->flows_param[i], other->flows_param[i])) {
            return false;
        }
    }

    return true;
}

struct pv_orca_flow {
    const pv_orca_flow_param_t *param;

    pv_residual_coupling_t **flows;
};

pv_status_t PV_MOCKABLE(pv_orca_flow_init)(
        pv_ypu_t *ypu,
        const pv_orca_flow_param_t *param,
        pv_orca_flow_t **object) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(object);

    *object = NULL;

    pv_orca_flow_t *o = pv_ypu_host_alloc(ypu, sizeof(pv_orca_flow_t));
    if (!o) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(o, 0, sizeof(pv_orca_flow_t));

    o->param = param;

    o->flows = pv_ypu_host_alloc(ypu, param->num_flows * (int32_t) sizeof(pv_orca_flow_t *));
    if (!(o->flows)) {
        pv_orca_flow_delete(ypu, o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(o->flows, 0, param->num_flows * (int32_t) sizeof(pv_orca_flow_t *));

    for (int32_t i = 0; i < param->num_flows; i++) {
        pv_status_t status = pv_residual_coupling_init(
                ypu,
                param->flows_param[i],
                &(o->flows[i]));
        if (status != PV_STATUS_SUCCESS) {
            pv_orca_flow_delete(ypu, o);
            return status;
        }
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_orca_flow_delete)(pv_ypu_t *ypu, pv_orca_flow_t *object) {
    PV_ASSERT(ypu);

    if (object) {
        if (object->flows) {
            for (int32_t i = object->param->num_flows - 1; i >= 0; i--) {
                pv_residual_coupling_delete(ypu, object->flows[i]);
            }

            pv_ypu_host_free(ypu, object->flows);
        }

        pv_ypu_host_free(ypu, object);
    }
}

pv_status_t PV_MOCKABLE(pv_orca_flow_forward)(
        pv_ypu_t *ypu,
        pv_orca_flow_t *object,
        int32_t n,
        pv_ypu_mem_t *x_ypu_mem,
        pv_ypu_mem_t *y_ypu_mem,
        int32_t x_offset,
        int32_t y_offset) {
    PV_ASSERT(ypu);
    PV_ASSERT(object);
    PV_ASSERT(n);
    PV_ASSERT(x_ypu_mem);
    PV_ASSERT(y_ypu_mem);
    PV_ORCA_PROFILER_START("flow");

    const int32_t num_channels = 2 * object->param->flows_param[object->param->num_flows - 1]->conv_post_param->output_channels;

    pv_ypu_mem_t *buffer_1 = pv_ypu_buffer_get(
            ypu,
            n * num_channels * (int32_t) sizeof(float),
            false);
    if (!buffer_1) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_mem_t *buffer_2 = pv_ypu_buffer_get(
            ypu,
            n * num_channels * (int32_t) sizeof(float),
            false);
    if (!buffer_2) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_ypu_op_memcpy_args_t args0 = {
            .output = buffer_1,
            .input = x_ypu_mem,
            .size_bytes = n * num_channels * (int32_t) sizeof(float),
            .output_offset = 0,
            .input_offset = x_offset,
    };

    pv_status_t status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_MEMCPY,
            &args0);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    for (int32_t i = 0; i < object->param->num_flows; i++) {
        pv_ypu_op_elementwise_broadcast_args_t args = {
                .output = buffer_2,
                .input = buffer_1,
                .m = n,
                .n = num_channels,
                .output_offset = 0,
                .input_offset = 0,
        };

        status = pv_ypu_operator_execute(
                ypu,
                PV_YPU_OPERATOR_FLIP,
                &args);
        if (status != PV_STATUS_SUCCESS) {
            return status;
        }

        status = pv_residual_coupling_forward(
                ypu,
                object->flows[i],
                n,
                buffer_2,
                buffer_1,
                0,
                0);
        if (status != PV_STATUS_SUCCESS) {
            return status;
        }
    }

    pv_ypu_op_memcpy_args_t args2 = {
            .output = y_ypu_mem,
            .input = buffer_1,
            .size_bytes = n * num_channels * (int32_t) sizeof(float),
            .output_offset = y_offset,
            .input_offset = 0,
    };

    status = pv_ypu_operator_execute(
            ypu,
            PV_YPU_OPERATOR_MEMCPY,
            &args2);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    pv_ypu_buffer_release(ypu, buffer_2);
    pv_ypu_buffer_release(ypu, buffer_1);

    PV_ORCA_PROFILER_STOP("flow");
    return PV_STATUS_SUCCESS;
}
