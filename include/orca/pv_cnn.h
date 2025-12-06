#ifndef PV_CNN_H
#define PV_CNN_H

#include "core/pv_type.h"
#include "util/pv_file.h"
#include "ypu/pv_ypu.h"

typedef struct {
    int32_t input_channels;
    int32_t output_channels;
    int32_t kernel_size;
    int32_t stride;
    int32_t padding;
    int32_t dilation;
    pv_ypu_config_mem_t *weight;
    pv_ypu_config_mem_t *bias;
} pv_cnn_param_t;

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_cnn_param_serialize_buffer)(
        pv_ypu_t *ypu,
        const pv_cnn_param_t *param,
        size_t *length,
        void **buffer);

pv_status_t PV_MOCKABLE(pv_cnn_param_serialize)(
        pv_ypu_t *ypu,
        const pv_cnn_param_t *param,
        FILE *file);

#endif

void PV_MOCKABLE(pv_cnn_param_delete)(
        pv_ypu_t *ypu,
        pv_cnn_param_t *param);

pv_status_t PV_MOCKABLE(pv_cnn_param_load)(
        pv_ypu_t *ypu,
        FILE *f,
        pv_cnn_param_t **param);

bool PV_MOCKABLE(pv_cnn_param_is_equal)(
        const pv_cnn_param_t *object,
        const pv_cnn_param_t *other);

typedef struct {
    int32_t num_channels;
    int32_t kernel_size;
    int32_t stride;
    int32_t padding;
    int32_t dilation;
    pv_ypu_config_mem_t *weight;
    pv_ypu_config_mem_t *bias;
} pv_cnn_depthwise_param_t;

int32_t PV_MOCKABLE(pv_cnn_param_receptive_field)(
        const pv_cnn_param_t *param);

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_cnn_depthwise_param_serialize_buffer)(
        pv_ypu_t *ypu,
        const pv_cnn_depthwise_param_t *param,
        size_t *length,
        void **buffer);

pv_status_t PV_MOCKABLE(pv_cnn_depthwise_param_serialize)(
        pv_ypu_t *ypu,
        const pv_cnn_depthwise_param_t *param,
        FILE *file);

#endif

void PV_MOCKABLE(pv_cnn_depthwise_param_delete)(
        pv_ypu_t *ypu,
        pv_cnn_depthwise_param_t *param);

pv_status_t PV_MOCKABLE(pv_cnn_depthwise_param_load)(
        pv_ypu_t *ypu,
        FILE *f,
        pv_cnn_depthwise_param_t **param);

bool PV_MOCKABLE(pv_cnn_depthwise_param_is_equal)(
        const pv_cnn_depthwise_param_t *object,
        const pv_cnn_depthwise_param_t *other);

typedef struct pv_cnn pv_cnn_t;

pv_status_t PV_MOCKABLE(pv_cnn_init)(
        pv_ypu_t *ypu,
        const pv_cnn_param_t *param,
        pv_cnn_t **object);

void PV_MOCKABLE(pv_cnn_delete)(pv_ypu_t *ypu, pv_cnn_t *object);

pv_status_t PV_MOCKABLE(pv_cnn_forward)(
        pv_ypu_t *ypu,
        pv_cnn_t *object,
        int32_t n,
        pv_ypu_mem_t *x,
        pv_ypu_mem_t *y,
        int32_t x_offset,
        int32_t y_offset);

pv_status_t PV_MOCKABLE(pv_cnn_forward_to_q510)(
        pv_ypu_t *ypu,
        pv_cnn_t *object,
        int32_t n,
        pv_ypu_mem_t *x,
        pv_ypu_mem_t *y,
        int32_t x_offset,
        int32_t y_offset);

pv_status_t PV_MOCKABLE(pv_cnn_forward_from_q510)(
        pv_ypu_t *ypu,
        pv_cnn_t *object,
        int32_t n,
        pv_ypu_mem_t *x,
        pv_ypu_mem_t *y,
        int32_t x_offset,
        int32_t y_offset);

int32_t PV_MOCKABLE(pv_cnn_output_channels)(const pv_cnn_t *object);

int32_t PV_MOCKABLE(pv_cnn_input_channels)(const pv_cnn_t *object);

int32_t PV_MOCKABLE(pv_cnn_kernel_size)(const pv_cnn_t *object);

int32_t PV_MOCKABLE(pv_cnn_padding)(const pv_cnn_t *object);

int32_t PV_MOCKABLE(pv_cnn_dilation)(const pv_cnn_t *object);

int32_t PV_MOCKABLE(pv_cnn_stride)(const pv_cnn_t *object);

pv_ypu_mem_t *PV_MOCKABLE(pv_cnn_get_weight)(const pv_cnn_t *object);

typedef struct pv_cnn_depthwise pv_cnn_depthwise_t;

pv_status_t PV_MOCKABLE(pv_cnn_depthwise_init)(
        pv_ypu_t *ypu,
        const pv_cnn_depthwise_param_t *param,
        pv_cnn_depthwise_t **object);

void PV_MOCKABLE(pv_cnn_depthwise_delete)(
        pv_ypu_t *ypu,
        pv_cnn_depthwise_t *object);

pv_status_t PV_MOCKABLE(pv_cnn_depthwise_forward)(
        pv_ypu_t *ypu,
        pv_cnn_depthwise_t *object,
        int32_t n,
        pv_ypu_mem_t *x,
        pv_ypu_mem_t *y, int32_t x_offset, int32_t y_offset);

int32_t PV_MOCKABLE(pv_cnn_depthwise_num_channels)(const pv_cnn_depthwise_t *object);

int32_t PV_MOCKABLE(pv_cnn_depthwise_kernel_size)(const pv_cnn_depthwise_t *object);

int32_t PV_MOCKABLE(pv_cnn_depthwise_stride)(const pv_cnn_depthwise_t *object);

pv_ypu_mem_t *PV_MOCKABLE(pv_cnn_depthwise_get_weight)(const pv_cnn_depthwise_t *object);

#endif // PV_CNN_H
