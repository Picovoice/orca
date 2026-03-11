#include <string.h>

#include "core/pv_error_messages.h"
#include "orca/pv_additive_coupling.h"
#include "orca/pv_orca_prior_encoder_flow.h"
#include "orca/pv_orca_util.h"
#include "orca/pv_profiler.h"
#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

struct pv_orca_prior_encoder_flow {
    const pv_orca_prior_encoder_flow_param_t *param;

    pv_additive_coupling_t **flows;
};

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_orca_prior_encoder_flow_param_serialize)(
        pv_ypu_t *ypu,
        const pv_orca_prior_encoder_flow_param_t *param, FILE *file) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(file);

    size_t count = fwrite(&(param->num_flows), sizeof(int32_t), 1, file);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

    count = fwrite(&(param->dimension), sizeof(int32_t), 1, file);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

    for (int32_t i = 0; i < param->num_flows; i++) {
        pv_status_t status = pv_additive_coupling_param_serialize(
                ypu,
                param->flows_param[i],
                file);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_additive_coupling_param_serialize,
                    pv_status_to_string(status));
            return status;
        }
    }

    return PV_STATUS_SUCCESS;
}

#endif

pv_status_t PV_MOCKABLE(pv_orca_prior_encoder_flow_param_load)(
        pv_ypu_t *ypu,
        FILE *f,
        pv_orca_prior_encoder_flow_param_t **param) {
    PV_ASSERT(ypu);
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_orca_prior_encoder_flow_param_t *p = pv_ypu_host_alloc(
            ypu,
            sizeof(pv_orca_prior_encoder_flow_param_t));
    if (!p) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_host_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(p, 0, sizeof(pv_orca_prior_encoder_flow_param_t));

    size_t count = pv_fread(&(p->num_flows), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_orca_prior_encoder_flow_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->num_flows <= 0) {
        pv_orca_prior_encoder_flow_param_delete(ypu, p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    count = pv_fread(&(p->dimension), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_orca_prior_encoder_flow_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->dimension <= 0) {
        pv_orca_prior_encoder_flow_param_delete(ypu, p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    p->flows_param = pv_ypu_host_alloc(
            ypu,
            p->num_flows * ((int32_t) sizeof(pv_additive_coupling_param_t *)));
    if (!(p->flows_param)) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_host_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("p->flows_param"));
        pv_orca_prior_encoder_flow_param_delete(ypu, p);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(p->flows_param, 0, p->num_flows * sizeof(pv_additive_coupling_param_t *));

    pv_status_t status;
    for (int32_t i = 0; i < p->num_flows; i++) {
        status = pv_additive_coupling_param_load(
                ypu,
                f,
                (pv_additive_coupling_param_t **) &(p->flows_param[i]));
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_additive_coupling_param_load,
                    pv_status_to_string(status));
            pv_orca_prior_encoder_flow_param_delete(ypu, p);
            return status;
        }
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_orca_prior_encoder_flow_param_delete)(
        pv_ypu_t *ypu,
        pv_orca_prior_encoder_flow_param_t *param) {
    PV_ASSERT(ypu);

    if (param) {
        if (param->flows_param) {
            for (int32_t i = param->num_flows - 1; i >= 0; --i) {
                pv_additive_coupling_param_delete(ypu, (pv_additive_coupling_param_t *) param->flows_param[i]);
            }

            pv_ypu_host_free(ypu, param->flows_param);
        }

        pv_ypu_host_free(ypu, param);
    }
}

bool PV_MOCKABLE(pv_orca_prior_encoder_flow_param_is_equal)(
        const pv_orca_prior_encoder_flow_param_t *object,
        const pv_orca_prior_encoder_flow_param_t *other) {
    PV_ASSERT(object);
    PV_ASSERT(other);

    if (object->num_flows != other->num_flows) {
        return false;
    }

    if (object->dimension != other->dimension) {
        return false;
    }

    for (int32_t i = 0; i < object->num_flows; i++) {
        if (!pv_additive_coupling_param_is_equal(object->flows_param[i], other->flows_param[i])) {
            return false;
        }
    }

    return true;
}

pv_status_t PV_MOCKABLE(pv_orca_prior_encoder_flow_init)(
        pv_ypu_t *ypu,
        const pv_orca_prior_encoder_flow_param_t *param,
        pv_orca_prior_encoder_flow_t **object) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(object);

    *object = NULL;

    pv_orca_prior_encoder_flow_t *o = pv_ypu_host_alloc(
            ypu,
            sizeof(pv_orca_prior_encoder_flow_t));
    if (!o) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_host_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(o, 0, sizeof(pv_orca_prior_encoder_flow_t));

    o->param = param;

    o->flows = pv_ypu_host_alloc(
            ypu,
            param->num_flows * ((int32_t) sizeof(pv_orca_prior_encoder_flow_t *)));
    if (!(o->flows)) {
        PV_ERROR_REPORT(
                &pv_error_msg_ypu_host_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("o->flows"));
        pv_orca_prior_encoder_flow_delete(ypu, o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(o->flows, 0, param->num_flows * sizeof(pv_orca_prior_encoder_flow_t *));

    pv_status_t status;
    for (int32_t i = 0; i < param->num_flows; i++) {
        status = pv_additive_coupling_init(
                ypu,
                param->flows_param[i],
                &(o->flows[i]));
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_additive_coupling_init,
                    pv_status_to_string(status));
            pv_orca_prior_encoder_flow_delete(ypu, o);
            return status;
        }
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_orca_prior_encoder_flow_delete)(
        pv_ypu_t *ypu,
        pv_orca_prior_encoder_flow_t *object) {
    PV_ASSERT(ypu);

    if (object) {
        if (object->flows) {
            for (int32_t i = object->param->num_flows - 1; i >= 0; --i) {
                pv_additive_coupling_delete(ypu, object->flows[i]);
            }

            pv_ypu_host_free(ypu, object->flows);
        }

        pv_ypu_host_free(ypu, object);
    }
}

pv_status_t PV_MOCKABLE(pv_orca_prior_encoder_flow_forward)(
        pv_ypu_t *ypu,
        pv_orca_prior_encoder_flow_t *object,
        int32_t n,
        pv_ypu_mem_t *c,
        pv_ypu_mem_t *x,
        pv_ypu_mem_t *y) {
    PV_ASSERT(ypu);
    PV_ASSERT(object);
    PV_ASSERT(n > 0);
    PV_ASSERT(c);
    PV_ASSERT(x);
    PV_ASSERT(y);

    PV_ORCA_PROFILER_START("\torca_prior_encoder_flow_forward"); 

    const int32_t dimension = object->param->dimension;
    const int32_t num_flows = object->param->num_flows;

    for (int32_t i = 0; i < num_flows; i++) {
        pv_status_t status = pv_additive_coupling_forward(
                ypu,
                object->flows[i],
                n,
                c,
                (i == 0) ? x : y,
                y);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT_MODULE_FUNCTION_STATUS_INTERNAL_HELPER(
                    pv_additive_coupling_forward,
                    pv_status_to_string(status));
            return status;
        }

        pv_ypu_op_elementwise_broadcast_args_t flip_args = {
                .output = y,
                .input = y,
                .m = n,
                .n = dimension,
                .output_offset = 0,
                .input_offset = 0
        };
        status = pv_ypu_operator_execute(
                ypu,
                PV_YPU_OPERATOR_FLIP,
                &flip_args);
        if (status != PV_STATUS_SUCCESS) {
            PV_ERROR_REPORT(
                    &pv_error_msg_ypu_operator_fail,
                    PV_ERROR_ARGS_PUBLIC("execute"),
                    PV_ERROR_ARGS_PRIVATE(
                            "execute",
                            pv_ypu_operator_type_to_string(PV_YPU_OPERATOR_FLIP),
                            pv_ypu_device_type_to_string(pv_ypu_device_type(ypu)),
                            pv_status_to_string(status)));
            return status;
        }
    }

    PV_ORCA_PROFILER_STOP("\torca_prior_encoder_flow_forward"); 

    return PV_STATUS_SUCCESS;
}
