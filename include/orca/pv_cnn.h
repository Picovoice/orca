#ifndef PV_CNN_H
#define PV_CNN_H

#include "util/pv_file.h"
#include "core/pv_type.h"

typedef struct {
    int32_t input_channels;
    int32_t output_channels;
    int32_t kernel_size;
    int32_t stride;
    int32_t padding;
    int32_t dilation;
    const q7_t *weight;
    const q7_t *bias;
} pv_cnn_param_t;

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_cnn_param_serialize_buffer)(const pv_cnn_param_t *param, size_t *length, void **buffer);

pv_status_t PV_MOCKABLE(pv_cnn_param_serialize)(const pv_cnn_param_t *param, FILE *file);

#endif

void PV_MOCKABLE(pv_cnn_param_delete)(pv_cnn_param_t *param);

pv_status_t PV_MOCKABLE(pv_cnn_param_load)(FILE *f, pv_cnn_param_t **param);

bool PV_MOCKABLE(pv_cnn_param_is_equal)(
        const pv_cnn_param_t *object,
        const pv_cnn_param_t *other);

typedef struct {
    int32_t num_channels;
    int32_t kernel_size;
    int32_t stride;
    int32_t padding;
    int32_t dilation;
    const q7_t *weight;
    const q7_t *bias;
} pv_cnn_depthwise_param_t;

int32_t PV_MOCKABLE(pv_cnn_param_receptive_field)(const pv_cnn_param_t *param);

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_cnn_depthwise_param_serialize_buffer)(
        const pv_cnn_depthwise_param_t *param,
        size_t *length,
        void **buffer);

pv_status_t PV_MOCKABLE(pv_cnn_depthwise_param_serialize)(const pv_cnn_depthwise_param_t *param, FILE *file);

#endif

void PV_MOCKABLE(pv_cnn_depthwise_param_delete)(pv_cnn_depthwise_param_t *param);

pv_status_t PV_MOCKABLE(pv_cnn_depthwise_param_load)(FILE *f, pv_cnn_depthwise_param_t **param);

bool PV_MOCKABLE(pv_cnn_depthwise_param_is_equal)(
        const pv_cnn_depthwise_param_t *object,
        const pv_cnn_depthwise_param_t *other);

typedef struct pv_cnn pv_cnn_t;

pv_status_t PV_MOCKABLE(pv_cnn_init)(
        const pv_cnn_param_t *param,
        pv_cnn_t **object);

void PV_MOCKABLE(pv_cnn_delete)(pv_cnn_t *object);

pv_status_t PV_MOCKABLE(pv_cnn_transpose_weight)(pv_cnn_t *object, q7_t **weight);

pv_status_t PV_MOCKABLE(pv_cnn_forward)(pv_cnn_t *object, int32_t n, const float *x, float *y);

pv_status_t PV_MOCKABLE(pv_cnn_forward_to_q510)(pv_cnn_t *object, int32_t n, const float *x, q510_t *y);

pv_status_t PV_MOCKABLE(pv_cnn_forward_from_q510)(pv_cnn_t *object, int32_t n, const q510_t *x, float *y);

int32_t PV_MOCKABLE(pv_cnn_output_channels)(const pv_cnn_t *object);

int32_t PV_MOCKABLE(pv_cnn_input_channels)(const pv_cnn_t *object);

int32_t PV_MOCKABLE(pv_cnn_kernel_size)(const pv_cnn_t *object);

int32_t PV_MOCKABLE(pv_cnn_padding)(const pv_cnn_t *object);

int32_t PV_MOCKABLE(pv_cnn_dilation)(const pv_cnn_t *object);

int32_t PV_MOCKABLE(pv_cnn_stride)(const pv_cnn_t *object);

const q7_t *PV_MOCKABLE(pv_cnn_get_weight)(const pv_cnn_t *object);

typedef struct pv_cnn_depthwise pv_cnn_depthwise_t;

pv_status_t PV_MOCKABLE(pv_cnn_depthwise_init)(
        const pv_cnn_depthwise_param_t *param,
        pv_cnn_depthwise_t **object);

void PV_MOCKABLE(pv_cnn_depthwise_delete)(pv_cnn_depthwise_t *object);

pv_status_t PV_MOCKABLE(pv_cnn_depthwise_forward)(pv_cnn_depthwise_t *object, int32_t n, const float *x, float *y);

int32_t PV_MOCKABLE(pv_cnn_depthwise_num_channels)(const pv_cnn_depthwise_t *object);

int32_t PV_MOCKABLE(pv_cnn_depthwise_kernel_size)(const pv_cnn_depthwise_t *object);

int32_t PV_MOCKABLE(pv_cnn_depthwise_stride)(const pv_cnn_depthwise_t *object);

const q7_t *PV_MOCKABLE(pv_cnn_depthwise_get_weight)(const pv_cnn_depthwise_t *object);

#endif // PV_CNN_H
