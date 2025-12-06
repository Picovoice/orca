#include <stdlib.h>
#include <string.h>

#include "orca/pv_profiler.h"
#include "orca/pv_vocos_backbone.h"
#include "util/pv_check_status.h"
#include "util/pv_file.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_vocos_backbone_param_serialize)(
        pv_ypu_t *ypu,
        const pv_vocos_backbone_param_t *param,
        FILE *file) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(file);

    pv_status_t status = pv_layer_norm_param_serialize(ypu, param->layer_norm_pre_param, file);
    PV_CHECK_STATUS(status);

    status = pv_layer_norm_param_serialize(ypu, param->layer_norm_post_param, file);
    PV_CHECK_STATUS(status);

    const size_t count = fwrite(&(param->num_convnext_layers), sizeof(int32_t), 1, file);
    if (count != 1) {
        return PV_STATUS_IO_ERROR;
    }

    for (int32_t i = 0; i < param->num_convnext_layers; i++) {
        status = pv_convnext_param_serialize(ypu, param->convnext_layers_param[i], file);
        if (status != PV_STATUS_SUCCESS) {
            return status;
        }
    }

    return PV_STATUS_SUCCESS;
}

#endif

pv_status_t PV_MOCKABLE(pv_vocos_backbone_param_load)(pv_ypu_t *ypu, FILE *f, pv_vocos_backbone_param_t **param) {
    PV_ASSERT(ypu);
    PV_ASSERT(f);
    PV_ASSERT(param);

    *param = NULL;

    pv_vocos_backbone_param_t *p = pv_ypu_host_alloc(ypu, sizeof(pv_vocos_backbone_param_t));
    PV_CHECK_ALLOC(p);

    memset(p, 0, sizeof(pv_vocos_backbone_param_t));

    pv_status_t status = pv_layer_norm_param_load(ypu, f, (pv_layer_norm_param_t **) &(p->layer_norm_pre_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_vocos_backbone_param_delete(ypu, p);
        return status;
    }

    status = pv_layer_norm_param_load(ypu, f, (pv_layer_norm_param_t **) &(p->layer_norm_post_param));
    if (status != PV_STATUS_SUCCESS) {
        pv_vocos_backbone_param_delete(ypu, p);
        return status;
    }

    size_t count = pv_fread(&(p->num_convnext_layers), sizeof(int32_t), 1, f);
    if (count != 1) {
        pv_vocos_backbone_param_delete(ypu, p);
        return PV_STATUS_IO_ERROR;
    }
    if (p->num_convnext_layers <= 0) {
        pv_vocos_backbone_param_delete(ypu, p);
        return PV_STATUS_INVALID_ARGUMENT;
    }

    p->convnext_layers_param = pv_ypu_host_alloc(ypu, p->num_convnext_layers * (int32_t) sizeof(pv_convnext_param_t *));
    if (!(p->convnext_layers_param)) {
        pv_vocos_backbone_param_delete(ypu, p);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(p->convnext_layers_param, 0, p->num_convnext_layers * (int32_t) sizeof(pv_convnext_param_t *));

    for (int32_t i = 0; i < p->num_convnext_layers; i++) {
        status = pv_convnext_param_load(ypu, f, (pv_convnext_param_t **) &(p->convnext_layers_param[i]));
        if (status != PV_STATUS_SUCCESS) {
            pv_vocos_backbone_param_delete(ypu, p);
            return status;
        }
    }

    *param = p;

    return PV_STATUS_SUCCESS;
}


void PV_MOCKABLE(pv_vocos_backbone_param_delete)(pv_ypu_t *ypu, pv_vocos_backbone_param_t *param) {
    PV_ASSERT(ypu);

    if (param) {
        if (param->convnext_layers_param) {
            for (int32_t i = param->num_convnext_layers - 1; i >= 0; i--) {
                pv_convnext_param_delete(ypu, (pv_convnext_param_t *) param->convnext_layers_param[i]);
            }

            pv_ypu_host_free(ypu, (pv_convnext_param_t **) (param->convnext_layers_param));
        }

        pv_layer_norm_param_delete(ypu, (pv_layer_norm_param_t *) (param->layer_norm_post_param));

        pv_layer_norm_param_delete(ypu, (pv_layer_norm_param_t *) (param->layer_norm_pre_param));

        pv_ypu_host_free(ypu, param);
    }
}

bool PV_MOCKABLE(pv_vocos_backbone_param_is_equal)(
        const pv_vocos_backbone_param_t *object,
        const pv_vocos_backbone_param_t *other) {
    PV_ASSERT(object);
    PV_ASSERT(other);

    if (!pv_layer_norm_param_is_equal(object->layer_norm_pre_param, other->layer_norm_pre_param)) {
        return false;
    }

    if (!pv_layer_norm_param_is_equal(object->layer_norm_post_param, other->layer_norm_post_param)) {
        return false;
    }

    if (object->num_convnext_layers != other->num_convnext_layers) {
        return false;
    }

    for (int32_t i = 0; i < object->num_convnext_layers; i++) {
        if (!pv_convnext_param_is_equal(object->convnext_layers_param[i], other->convnext_layers_param[i])) {
            return false;
        }
    }

    return true;
}

int32_t PV_MOCKABLE(pv_vocos_backbone_param_receptive_field)(const pv_vocos_backbone_param_t *object) {
    PV_ASSERT(object);

    int32_t num_layers = object->num_convnext_layers;
    int32_t kernel_size = object->convnext_layers_param[0]->conv_depthwise_param->kernel_size;

    PV_ASSERT((kernel_size % 2) == 1);

    return (num_layers * ((kernel_size - 1) / 2));
}

struct pv_vocos_backbone {
    const pv_vocos_backbone_param_t *param;

    pv_layer_norm_t *layer_norm_pre;
    pv_layer_norm_t *layer_norm_post;
    pv_convnext_t **convnext_layers;
};

pv_status_t PV_MOCKABLE(pv_vocos_backbone_init)(
        pv_ypu_t *ypu,
        const pv_vocos_backbone_param_t *param,
        pv_vocos_backbone_t **object) {
    PV_ASSERT(ypu);
    PV_ASSERT(param);
    PV_ASSERT(object);

    *object = NULL;

    pv_vocos_backbone_t *o = pv_ypu_host_alloc(ypu, sizeof(pv_vocos_backbone_t));
    if (!o) {
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(o, 0, sizeof(pv_vocos_backbone_t));

    o->param = param;

    pv_status_t status = pv_layer_norm_init(ypu, param->layer_norm_pre_param, &(o->layer_norm_pre));
    if (status != PV_STATUS_SUCCESS) {
        pv_vocos_backbone_delete(ypu, o);
        return status;
    }

    status = pv_layer_norm_init(ypu, param->layer_norm_post_param, &(o->layer_norm_post));
    if (status != PV_STATUS_SUCCESS) {
        pv_vocos_backbone_delete(ypu, o);
        return status;
    }

    o->convnext_layers = pv_ypu_host_alloc(ypu, param->num_convnext_layers * (int32_t) sizeof(pv_convnext_t *));
    if (!(o->convnext_layers)) {
        pv_vocos_backbone_delete(ypu, o);
        return PV_STATUS_OUT_OF_MEMORY;
    }

    memset(o->convnext_layers, 0, param->num_convnext_layers * (int32_t) sizeof(pv_convnext_t *));

    for (int32_t i = 0; i < param->num_convnext_layers; i++) {
        status = pv_convnext_init(
                ypu,
                param->convnext_layers_param[i],
                &(o->convnext_layers[i]));
        if (status != PV_STATUS_SUCCESS) {
            pv_vocos_backbone_delete(ypu, o);
            return status;
        }
    }

    *object = o;

    return PV_STATUS_SUCCESS;
}

void PV_MOCKABLE(pv_vocos_backbone_delete)(pv_ypu_t *ypu, pv_vocos_backbone_t *object) {
    PV_ASSERT(ypu);

    if (object) {
        if (object->convnext_layers) {
            for (int32_t i = object->param->num_convnext_layers - 1; i >= 0; i--) {
                pv_convnext_delete(ypu, object->convnext_layers[i]);
            }
            pv_ypu_host_free(ypu, object->convnext_layers);
        }

        pv_layer_norm_delete(ypu, object->layer_norm_post);
        pv_layer_norm_delete(ypu, object->layer_norm_pre);

        pv_ypu_host_free(ypu, object);
    }
}

int32_t PV_MOCKABLE(pv_vocos_backbone_output_channels)(const pv_vocos_backbone_t *object) {
    PV_ASSERT(object);

    return pv_layer_norm_num_channels(object->layer_norm_post);
}

pv_status_t PV_MOCKABLE(pv_vocos_backbone_forward)(
        pv_ypu_t *ypu,
        pv_vocos_backbone_t *object,
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

    PV_ORCA_PROFILER_START("vocos_backbone");
    pv_status_t status = pv_layer_norm_forward(
            ypu,
            object->layer_norm_pre,
            n,
            x_ypu_mem,
            y_ypu_mem,
            x_offset,
            y_offset);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    for (int32_t i = 0; i < object->param->num_convnext_layers; i++) {
        status = pv_convnext_forward(
                ypu,
                object->convnext_layers[i],
                n,
                y_ypu_mem,
                y_ypu_mem,
                y_offset,
                y_offset);
        if (status != PV_STATUS_SUCCESS) {
            return status;
        }
    }

    status = pv_layer_norm_forward(
            ypu,
            object->layer_norm_post,
            n,
            y_ypu_mem,
            y_ypu_mem,
            y_offset,
            y_offset);
    if (status != PV_STATUS_SUCCESS) {
        return status;
    }

    PV_ORCA_PROFILER_STOP("vocos_backbone");

    return PV_STATUS_SUCCESS;
}
