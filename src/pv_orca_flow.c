#include <string.h>

#include "orca/pv_buffer.h"
#include "orca/pv_orca_flow.h"
#include "orca/pv_orca_util.h"
#include "orca/pv_profiler.h"
#include "util/pv_check_status.h"
#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_orca_flow_param_serialize)(const pv_orca_flow_param_t *param, FILE *file) {
    PV_ASSERT(param);
    PV_ASSERT(file);

    size_t count = fwrite(&(param->num_flows), sizeof(int32_t), 1, file);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

    pv_status_t status;
    for (int32_t i = 0; i < param->num_flows; i++) {
        status = pv_residual_coupling_param_serialize(param->flows_param[i], file);
        if (status != PV_STATUS_SUCCESS) {
            return status;
        }
    }

    return PV_STATUS_SUCCESS;
}

#endif

pv_status_t PV_MOCKABLE(pv_orca_flow_param_load)(FILE *f, pv_orca_flow_param_t **param) {
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_orca_flow_param_t *p = calloc(1, sizeof(pv_orca_flow_param_t));
    PV_CHECK_ALLOC(p);

    size_t count = pv_fread(&(p->num_flows), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_orca_flow_param_delete(p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->num_flows <= 0) {
        pv_orca_flow_param_delete(p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    p->flows_param = calloc(p->num_flows, sizeof(pv_residual_coupling_param_t *));
    if (!(p->flows_param)) {
        pv_orca_flow_param_delete(p);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    pv_status_t status;
    for (int32_t i = 0; i < p->num_flows; i++) {
        status = pv_residual_coupling_param_load(f, (pv_residual_coupling_param_t **) &(p->flows_param[i]));
        if (status != PV_STATUS_SUCCESS) {
            pv_orca_flow_param_delete(p);
            return status;
        }
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}


void PV_MOCKABLE(pv_orca_flow_param_delete)(pv_orca_flow_param_t *param) {
    if (param) {
        if (param->flows_param) {
            for (int32_t i = param->num_flows - 1; i >= 0; i--) {
                pv_residual_coupling_param_delete((pv_residual_coupling_param_t *) param->flows_param[i]);
            }

            free(param->flows_param);
        }

        free(param);
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

    pv_buffer_t *buffer_1;
    pv_buffer_t *buffer_2;

    pv_buffer_t *buffer_flow_residual_coupling_x0;
    pv_buffer_t *buffer_flow_residual_coupling_x1;
    pv_buffer_t *buffer_flow_residual_coupling_mean;
    pv_buffer_t *buffer_flow_wavenet_in;
    pv_buffer_t *buffer_flow_wavenet_hidden;
    pv_buffer_t *buffer_flow_wavenet_inter;
    pv_buffer_t *buffer_flow_wavenet_inter_out;
    pv_buffer_t *buffer_flow_wavenet_out;
};

pv_status_t PV_MOCKABLE(pv_orca_flow_init)(
        const pv_orca_flow_param_t *param,
        pv_orca_flow_t **object) {
    PV_ASSERT(param);
    PV_ASSERT(object);

    *object = NULL;

    pv_orca_flow_t *o = calloc(1, sizeof(pv_orca_flow_t));
    if (!o) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    o->param = param;

    o->flows = calloc(param->num_flows, sizeof(pv_orca_flow_t *));
    if (!(o->flows)) {
        pv_orca_flow_delete(o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    int32_t flow_channels = o->param->flows_param[0]->conv_pre_param->input_channels;

    pv_status_t status = pv_buffer_init(flow_channels, &(o->buffer_flow_residual_coupling_x0));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_flow_delete(o);
        return status;
    }

    status = pv_buffer_init(flow_channels, &(o->buffer_flow_residual_coupling_x1));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_flow_delete(o);
        return status;
    }

    status = pv_buffer_init(flow_channels, &(o->buffer_flow_residual_coupling_mean));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_flow_delete(o);
        return status;
    }

    int32_t wavenet_in_channels = o->param->flows_param[0]->wavenet_resblocks_param[0]->conv_param->input_channels;
    int32_t wavenet_hidden_channels = o->param->flows_param[0]->wavenet_resblocks_param[0]->conv_param->output_channels;
    PV_ASSERT(wavenet_hidden_channels == 2 * wavenet_in_channels);

    status = pv_buffer_init(wavenet_in_channels, &(o->buffer_flow_wavenet_in));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_flow_delete(o);
        return status;
    }

    status = pv_buffer_init(wavenet_hidden_channels, &(o->buffer_flow_wavenet_hidden));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_flow_delete(o);
        return status;
    }

    status = pv_buffer_init(wavenet_in_channels, &(o->buffer_flow_wavenet_inter));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_flow_delete(o);
        return status;
    }

    status = pv_buffer_init(wavenet_in_channels, &(o->buffer_flow_wavenet_inter_out));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_flow_delete(o);
        return status;
    }

    status = pv_buffer_init(wavenet_in_channels, &(o->buffer_flow_wavenet_out));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_flow_delete(o);
        return status;
    }

    for (int32_t i = 0; i < param->num_flows; i++) {
        status = pv_residual_coupling_init(
                param->flows_param[i],
                o->buffer_flow_residual_coupling_x0,
                o->buffer_flow_residual_coupling_x1,
                o->buffer_flow_residual_coupling_mean,
                o->buffer_flow_wavenet_in,
                o->buffer_flow_wavenet_hidden,
                o->buffer_flow_wavenet_inter,
                o->buffer_flow_wavenet_inter_out,
                o->buffer_flow_wavenet_out,
                &(o->flows[i]));
        if (status != PV_STATUS_SUCCESS) {
            pv_orca_flow_delete(o);
            return status;
        }
    }

    int32_t num_channels = 2 * o->param->flows_param[o->param->num_flows - 1]->conv_post_param->output_channels;
    status = pv_buffer_init(num_channels, &(o->buffer_1));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_flow_delete(o);
        return status;
    }

    status = pv_buffer_init(num_channels, &(o->buffer_2));
    if (status != PV_STATUS_SUCCESS) {
        pv_orca_flow_delete(o);
        return status;
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_orca_flow_delete)(pv_orca_flow_t *object) {
    if (object) {
        pv_buffer_delete(object->buffer_2);
        pv_buffer_delete(object->buffer_1);
        pv_buffer_delete(object->buffer_flow_wavenet_out);
        pv_buffer_delete(object->buffer_flow_wavenet_inter_out);
        pv_buffer_delete(object->buffer_flow_wavenet_inter);
        pv_buffer_delete(object->buffer_flow_wavenet_hidden);
        pv_buffer_delete(object->buffer_flow_wavenet_in);
        pv_buffer_delete(object->buffer_flow_residual_coupling_mean);
        pv_buffer_delete(object->buffer_flow_residual_coupling_x1);
        pv_buffer_delete(object->buffer_flow_residual_coupling_x0);

        if (object->flows) {
            for (int32_t i = object->param->num_flows - 1; i >= 0; i--) {
                pv_residual_coupling_delete(object->flows[i]);
            }

            free(object->flows);
        }

        free(object);
    }
}

pv_status_t PV_MOCKABLE(pv_orca_flow_forward)(pv_orca_flow_t *object, int32_t n, const float *x, float *y) {
    PV_ASSERT(object);
    PV_ASSERT(n);
    PV_ASSERT(x);
    PV_ASSERT(y);
    PV_ORCA_PROFILER_START("flow");

    float *buffer_1 = pv_buffer_get(object->buffer_1, n, false);
    if (!buffer_1) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    float *buffer_2 = pv_buffer_get(object->buffer_2, n, false);
    if (!buffer_2) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memcpy(buffer_1, x, n * pv_buffer_dimension(object->buffer_1) * sizeof(float));

    const int32_t num_channels = pv_buffer_dimension(object->buffer_1);

    for (int32_t i = 0; i < object->param->num_flows; i++) {

        // flip channel dimension
        for (int32_t frame = 0; frame < n; frame++) {
            const int32_t frame_offset = frame * num_channels;

            for (int32_t c = 0; c < num_channels; c++) {
                buffer_2[frame_offset + num_channels - c - 1] = buffer_1[frame_offset + c];
            }
        }

        pv_status_t status = pv_residual_coupling_forward(object->flows[i], n, buffer_2, buffer_1);
        if (status != PV_STATUS_SUCCESS) {
            return status;
        }

    }

    memcpy(y, buffer_1, n * num_channels * sizeof(float));

    pv_buffer_free(object->buffer_2);
    pv_buffer_free(object->buffer_1);
    pv_buffer_free(object->buffer_flow_wavenet_out);
    pv_buffer_free(object->buffer_flow_wavenet_inter_out);
    pv_buffer_free(object->buffer_flow_wavenet_inter);
    pv_buffer_free(object->buffer_flow_wavenet_hidden);
    pv_buffer_free(object->buffer_flow_wavenet_in);
    pv_buffer_free(object->buffer_flow_residual_coupling_mean);
    pv_buffer_free(object->buffer_flow_residual_coupling_x1);
    pv_buffer_free(object->buffer_flow_residual_coupling_x0);

    PV_ORCA_PROFILER_STOP("flow");
    return PV_STATUS_SUCCESS;
}
