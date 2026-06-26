#ifndef PV_CNN_TRANSPOSED_DEPTHWISE_H
#define PV_CNN_TRANSPOSED_DEPTHWISE_H

#include "core/pv_type.h"
#include "util/pv_file.h"

/* layout-compatible with pv_cnn_depthwise_param_t */
typedef struct {
    int32_t num_channels;
    int32_t kernel_size;
    int32_t stride;
    int32_t padding;
    int32_t dilation;
    pv_ypu_config_mem_t *weight;
    pv_ypu_config_mem_t *bias;
    pv_ypu_config_mem_t *int8_inverse_scale;
} pv_cnn_transposed_depthwise_param_t;

#ifdef __PV_BUILD_APPS__

pv_status_t PV_MOCKABLE(pv_cnn_transposed_depthwise_param_serialize)(
        pv_ypu_t *ypu,
        const pv_cnn_transposed_depthwise_param_t *param,
        FILE *file);

#endif

void PV_MOCKABLE(pv_cnn_transposed_depthwise_param_delete)(
        pv_ypu_t *ypu,
        pv_cnn_transposed_depthwise_param_t *param);

pv_status_t PV_MOCKABLE(pv_cnn_transposed_depthwise_param_load)(
        pv_ypu_t *ypu,
        FILE *f,
        pv_cnn_transposed_depthwise_param_t **param);

bool PV_MOCKABLE(pv_cnn_transposed_depthwise_param_is_equal)(
        const pv_cnn_transposed_depthwise_param_t *object,
        const pv_cnn_transposed_depthwise_param_t *other);

typedef struct pv_cnn_transposed_depthwise pv_cnn_transposed_depthwise_t;

pv_status_t PV_MOCKABLE(pv_cnn_transposed_depthwise_init)(
        pv_ypu_t *ypu,
        const pv_cnn_transposed_depthwise_param_t *param,
        pv_cnn_transposed_depthwise_t **object);

void PV_MOCKABLE(pv_cnn_transposed_depthwise_delete)(
        pv_ypu_t *ypu,
        pv_cnn_transposed_depthwise_t *object);

pv_status_t PV_MOCKABLE(pv_cnn_transposed_depthwise_reset_cache)(
        pv_ypu_t *ypu,
        pv_cnn_transposed_depthwise_t *object);

pv_status_t PV_MOCKABLE(pv_cnn_transposed_depthwise_forward)(
        pv_ypu_t *ypu,
        pv_cnn_transposed_depthwise_t *object,
        int32_t n,
        pv_ypu_mem_t *x,
        pv_ypu_mem_t *y,
        int32_t x_offset,
        int32_t y_offset);

pv_status_t PV_MOCKABLE(pv_cnn_transposed_depthwise_forward_with_cache)(
        pv_ypu_t *ypu,
        pv_cnn_transposed_depthwise_t *object,
        int32_t n,
        pv_ypu_mem_t *x_ypu,
        pv_ypu_mem_t *y_ypu,
        int32_t x_offset,
        int32_t y_offset,
        bool is_flush,
        int32_t *n_out);

int32_t PV_MOCKABLE(pv_cnn_transposed_depthwise_num_output_frames)(
        const pv_cnn_transposed_depthwise_t *object,
        int32_t n);

int32_t PV_MOCKABLE(pv_cnn_transposed_depthwise_kernel_size)(const pv_cnn_transposed_depthwise_t *object);

int32_t PV_MOCKABLE(pv_cnn_transposed_depthwise_num_channels)(const pv_cnn_transposed_depthwise_t *object);

int32_t PV_MOCKABLE(pv_cnn_transposed_depthwise_padding)(const pv_cnn_transposed_depthwise_t *object);

int32_t PV_MOCKABLE(pv_cnn_transposed_depthwise_cache_length)(const pv_cnn_transposed_depthwise_t *object);

#endif // PV_CNN_TRANSPOSED_DEPTHWISE_H
