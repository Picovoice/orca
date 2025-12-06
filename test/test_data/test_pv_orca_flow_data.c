#ifndef TEST_PV_ORCA_FLOW_DATA_C
#define TEST_PV_ORCA_FLOW_DATA_C

#include "orca/pv_orca_flow.h"

static const q7_t FLOW_0_CONV_PRE_WEIGHT[] = {
    -10, 8, 12, -1, 10, 7, 0, 8, -12, 10, 8, -4, -7, 12, 4, -7, 0, -8, -8, 9, 9, -12, -3, 6, 6, 5, 12, 7, 8, -8, 11, 
    -8, 13, -4, -13, 3, -6, 0, 6, -11, -13, 6, 6, -10, 11, -7, 6, 11, 5, -13, 4, -1, 9, 10, -4, 12, 5, 7, 10, 8, 10, 
    10, 5, 12};

static const q7_t FLOW_0_CONV_PRE_BIAS[] = {
    0, 0, 0, 0, 0, 0, 0, 0};

static const pv_ypu_config_mem_t FLOW_0_CONV_PRE_WEIGHT_CONFIG = {
    .size_bytes = sizeof(FLOW_0_CONV_PRE_WEIGHT),
    .flags = PV_YPU_DEVICE_MEM_FLAG_STATIC,
    .data = (void *) FLOW_0_CONV_PRE_WEIGHT,
};

static const pv_ypu_config_mem_t FLOW_0_CONV_PRE_BIAS_CONFIG = {
    .size_bytes = sizeof(FLOW_0_CONV_PRE_BIAS),
    .flags = PV_YPU_DEVICE_MEM_FLAG_STATIC,
    .data = (void *) FLOW_0_CONV_PRE_BIAS,
};

static const pv_cnn_param_t FLOW_0_CONV_PRE_PARAM = {
        .input_channels = 8,
        .output_channels = 8,
        .kernel_size = 1,
        .stride = 1,
        .padding = 0,
        .dilation = 1,
        .weight = (pv_ypu_config_mem_t *) &FLOW_0_CONV_PRE_WEIGHT_CONFIG,
        .bias = (pv_ypu_config_mem_t *) &FLOW_0_CONV_PRE_BIAS_CONFIG,
};

static const q7_t FLOW_0_CONV_POST_WEIGHT[] = {
    6, 7, 9, 10, 11, 12, 3, 1, 8, 9, 2, 2, 10, 11, 8, 11, 0, 12, 7, 6, 2, 8, 7, 6, 12, 5, 8, 3, 12, 5, 5, 2, 7, 2, 7, 
    4, 1, 13, 2, 9, 2, 10, 4, 2, 6, 7, 5, 12, 1, 3, 5, 1, 12, 11, 1, 5, 10, 5, 2, 10, 12, 10, 12, 8};

static const q7_t FLOW_0_CONV_POST_BIAS[] = {
    0, 0, 0, 0, 0, 0, 0, 0};

static const pv_ypu_config_mem_t FLOW_0_CONV_POST_WEIGHT_CONFIG = {
    .size_bytes = sizeof(FLOW_0_CONV_POST_WEIGHT),
    .flags = PV_YPU_DEVICE_MEM_FLAG_STATIC,
    .data = (void *) FLOW_0_CONV_POST_WEIGHT,
};

static const pv_ypu_config_mem_t FLOW_0_CONV_POST_BIAS_CONFIG = {
    .size_bytes = sizeof(FLOW_0_CONV_POST_BIAS),
    .flags = PV_YPU_DEVICE_MEM_FLAG_STATIC,
    .data = (void *) FLOW_0_CONV_POST_BIAS,
};

static const pv_cnn_param_t FLOW_0_CONV_POST_PARAM = {
        .input_channels = 8,
        .output_channels = 8,
        .kernel_size = 1,
        .stride = 1,
        .padding = 0,
        .dilation = 1,
        .weight = (pv_ypu_config_mem_t *) &FLOW_0_CONV_POST_WEIGHT_CONFIG,
        .bias = (pv_ypu_config_mem_t *) &FLOW_0_CONV_POST_BIAS_CONFIG,
};

static const q7_t FLOW_0_WAVENET_RESBLOCKS_0_ENC_CONV_WEIGHT[] = {
    5, 1, -5, 10, -4, -3, -4, -1, 4, -1, 5, 10, -8, -9, -8, 3, 0, 3, -3, -6, 12, 2, 1, -6, 8, -10, 10, -10, -5, 9, 11, 
    12, 12, 5, 0, -3, -6, -11, 6, 10, 11, -1, -2, 2, -3, 6, 1, 4, -8, -6, 5, 0, -6, 9, -10, 4, -1, 10, -2, 11, 9, 7, 
    -7, 3, 11, 4, 3, 6, 9, 12, 0, 3, 6, 11, -9, 6, 6, -3, -8, 4, 10, 9, 7, 9, -8, -5, 6, 1, 2, -10, 11, 7, 8, 8, -5, 
    -2, -3, -7, 7, 4, 5, -12, -11, -11, 2, 6, -12, 4, 11, 1, 3, -2, -3, 1, -1, 8, -11, 12, -4, 2, -5, -3, 4, -10, 13, 
    7, 1, -7, -8, -3, 9, -6, 2, 11, 0, 6, -3, -7, -12, 1, 11, -8, 1, 8, -5, 2, 6, -11, 5, -12, -7, -1, -11, -6, -8, 
    -10, 9, 7, -4, -7, 9, 6, -7, 6, -3, -2, 8, -4, -2, 1, -1, -5, -4, -5, 6, 6, -11, 7, -5, -5, 6, 8, 1, -6, 3, -7, 
    -12, 2, -3, 5, -5, -7, 2, 9, 1, 10, 8, 4, 12, -1, 0, -1, -2, 0, 9, 8, 11, -1, 11, 1, -12, 0, -3, -12, -3, 4, -6, 3, 
    -6, -2, 8, 9, -1, 9, 12, 13, -11, 8, 10, -6, -7, 7, -3, 7, -1, 8, 7, 3, -5, -3, 4, 3, -12, -4, -8, 11, 4, 6, 6, -1, 
    2, 9, 11, -9, 11, 4, 13, 8, -10, 4, -4, 3, -3, 3, -7, -9, 5, -7, 0, -13, -11, 3, 11, -8, 9, -6, -4, -10, -11, -10, 
    -3, 11, 0, 6, 12, 11, -4, -9, -3, 4, -6, -10, -2, 5, -2, 3, 1, -5, -7, -3, 8, -9, -11, 3, 1, -1, -9, -4, -9, 9, -5, 
    -5, 11, -5, 8, 2, -7, -3, -8, -12, -4, 10, 8, 0, -3, -1, -3, 3, -13, -6, -2, 8, -5, 9, -1, 9, -7, -11, 2, 3, 6, 2, 
    6, 1, 12, -2, -5, -10, -12, -10, 0, 3, -4, 4, 10, 8, -11, -6, -4, -2, 11, -11, -3, -8, 11, 12, -6, 4, 2, 12, 12, 
    -11, 1, -4, -12, 6, 12, -12, -7, -7, -13, -3, 1, -1, 5, -8, 12, -3, 4, -3, 7, 10, 8, -1, 10, 2, -12, -3, -8, 7, 
    -11, 10, 1, 11, 8, 2, -11, 8, 1, -9, -6, -1, 9, 7, -3, -12, -7, -1, 11, 10, -8, 10, -7, -7, 6, -1, 7, -1, -5, 9, 
    -3, -4, -5, -8, 2, 13, 10, 0, -10, 3, -4, -5, -9, 4, -6, -12, -12, 2, 0, 7, -8, -6, -6, -3, -6, -4, -10, -11, -1, 
    11, 2, -3, -4, -5, -11, -9, -7, 8, 10, 7, 8, -12, -6, 1, 13, 9, -7, -9, 8, -9, -9, 10, 7, 10, 1, 9, 3, 0, 4, -9, 
    -11, -5, 2, -11, 1, 10, -5, 2, 6, 12, 11, 7, -2, 0, 12, -5, -11, 0, 10, -1, -2, 7, 8, -4, -11, 10, -2, 2, 13, -9, 
    3, -10, 11, -6, -9, 9, 8, -10, 0, 10, 11, 6, 3, 5, -5, -2, 9, -10, 10, 7, -4, 7, 1, -4, -8, 10, 11, 1, 8, 12, -12, 
    -10, -4, 7, 8, 0, 5, -2, 10, 9, 5, -7, 8, 8, -5, 1, 12, 4, 2, -3, -5, 3, -8, 0, -12, 2, 6, -11, 13, 0, -4, -6, 10, 
    -7, 6, -2, 1, -3, 0, -4, -13, 2, 12, -6, -12, 13, 4, -5, -9, -12, -6, 4, -9, 4, 7, -12, -1, 3, -5, -4, 1, 1, -12, 
    -7, -12, -7, -3, -4, 0, -5, 5, 6, -5, 11, 9, 6, -4, 1, 11, 12, -7, -3, -7, -2, 11, -9, 12, 8, 3, 7};

static const q7_t FLOW_0_WAVENET_RESBLOCKS_0_ENC_CONV_BIAS[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static const pv_ypu_config_mem_t FLOW_0_WAVENET_RESBLOCKS_0_ENC_CONV_WEIGHT_CONFIG = {
    .size_bytes = sizeof(FLOW_0_WAVENET_RESBLOCKS_0_ENC_CONV_WEIGHT),
    .flags = PV_YPU_DEVICE_MEM_FLAG_STATIC,
    .data = (void *) FLOW_0_WAVENET_RESBLOCKS_0_ENC_CONV_WEIGHT,
};

static const pv_ypu_config_mem_t FLOW_0_WAVENET_RESBLOCKS_0_ENC_CONV_BIAS_CONFIG = {
    .size_bytes = sizeof(FLOW_0_WAVENET_RESBLOCKS_0_ENC_CONV_BIAS),
    .flags = PV_YPU_DEVICE_MEM_FLAG_STATIC,
    .data = (void *) FLOW_0_WAVENET_RESBLOCKS_0_ENC_CONV_BIAS,
};

static const pv_cnn_param_t FLOW_0_WAVENET_RESBLOCKS_0_ENC_CONV_PARAM = {
        .input_channels = 8,
        .output_channels = 16,
        .kernel_size = 5,
        .stride = 1,
        .padding = 2,
        .dilation = 1,
        .weight = (pv_ypu_config_mem_t *) &FLOW_0_WAVENET_RESBLOCKS_0_ENC_CONV_WEIGHT_CONFIG,
        .bias = (pv_ypu_config_mem_t *) &FLOW_0_WAVENET_RESBLOCKS_0_ENC_CONV_BIAS_CONFIG,
};

static const q7_t FLOW_0_WAVENET_RESBLOCKS_0_ENC_CONV_SKIP_WEIGHT[] = {
    11, -1, -7, 6, -5, -11, -5, 0, -7, -10, -11, -9, -12, 9, 12, -3, 5, -1, -3, 8, 2, 5, 0, -8, 7, 0, 11, 0, 12, 2, 6, 
    -1, 0, -2, -11, -7, -1, 9, 8, 10, -7, 9, -3, -9, 13, -11, 8, 12, -6, 2, -2, 10, 8, -6, -12, -12, 0, -9, 1, -5, 12, 
    2, 0, 11, 10, -7, 2, 10, -3, 4, -9, -11, -3, 1, -5, -8, 0, -8, -6, 4, 0, 12, 2, 8, 4, 12, 7, -3, 1, -3, -10, -7, 2, 
    -12, -1, -12, -5, -4, 12, -10, -6, -10, -2, 2, 0, 1, -1, -2, 3, -12, -9, 3, -6, 5, -11, -8, 13, -4, 5, 4, 4, 0, -8, 
    -5, -13, 0, -3, -5};

static const q7_t FLOW_0_WAVENET_RESBLOCKS_0_ENC_CONV_SKIP_BIAS[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static const pv_ypu_config_mem_t FLOW_0_WAVENET_RESBLOCKS_0_ENC_CONV_SKIP_WEIGHT_CONFIG = {
    .size_bytes = sizeof(FLOW_0_WAVENET_RESBLOCKS_0_ENC_CONV_SKIP_WEIGHT),
    .flags = PV_YPU_DEVICE_MEM_FLAG_STATIC,
    .data = (void *) FLOW_0_WAVENET_RESBLOCKS_0_ENC_CONV_SKIP_WEIGHT,
};

static const pv_ypu_config_mem_t FLOW_0_WAVENET_RESBLOCKS_0_ENC_CONV_SKIP_BIAS_CONFIG = {
    .size_bytes = sizeof(FLOW_0_WAVENET_RESBLOCKS_0_ENC_CONV_SKIP_BIAS),
    .flags = PV_YPU_DEVICE_MEM_FLAG_STATIC,
    .data = (void *) FLOW_0_WAVENET_RESBLOCKS_0_ENC_CONV_SKIP_BIAS,
};

static const pv_cnn_param_t FLOW_0_WAVENET_RESBLOCKS_0_ENC_CONV_SKIP_PARAM = {
        .input_channels = 8,
        .output_channels = 16,
        .kernel_size = 1,
        .stride = 1,
        .padding = 0,
        .dilation = 1,
        .weight = (pv_ypu_config_mem_t *) &FLOW_0_WAVENET_RESBLOCKS_0_ENC_CONV_SKIP_WEIGHT_CONFIG,
        .bias = (pv_ypu_config_mem_t *) &FLOW_0_WAVENET_RESBLOCKS_0_ENC_CONV_SKIP_BIAS_CONFIG,
};

static const pv_wavenet_resblock_param_t FLOW_0_WAVENET_RESBLOCKS_0_PARAM = {
        .conv_param = &FLOW_0_WAVENET_RESBLOCKS_0_ENC_CONV_PARAM,
        .conv_skip_param = &FLOW_0_WAVENET_RESBLOCKS_0_ENC_CONV_SKIP_PARAM,
};

static const q7_t FLOW_0_WAVENET_RESBLOCKS_1_ENC_CONV_WEIGHT[] = {
    6, -3, 7, -12, -7, -1, -12, -10, -4, 4, 9, 3, -5, 12, -10, -7, -1, 7, -4, -13, -11, -10, 8, -5, 12, 3, -8, -6, -3, 
    -1, 9, 13, 0, -6, 12, -8, -5, 7, 9, -5, -9, -5, -13, 1, 11, -8, 6, 11, -2, -12, -11, 4, 13, -5, 0, 3, 1, -12, -7, 
    10, -6, 7, -7, 11, 11, 12, 5, -1, 5, -9, -11, 0, -2, 3, 4, -11, -2, -11, -6, -5, -9, 13, 3, 12, 9, -1, 12, 8, 5, 5, 
    -9, -9, -11, -8, -2, 1, 12, -10, -10, -10, -4, 6, -7, 10, 11, -3, -11, 7, 5, -8, -2, 8, 10, 3, 3, 2, -11, -8, -10, 
    10, -3, -4, 5, 2, -5, 3, -7, 10, -1, 0, -8, 1, -11, -5, 5, -12, 12, -10, -9, 12, 12, -8, 5, 2, 0, -6, 8, 3, -4, 10, 
    7, 12, 11, -3, 8, 10, 9, 6, -1, -1, -1, -1, -5, 12, 5, 7, -12, 11, 8, -9, -9, -8, 8, 8, -10, 2, -11, -3, 0, 0, 12, 
    2, 10, -5, -3, 9, -3, -8, -11, 5, -12, 3, -12, -7, -8, -9, -5, 9, 10, 7, 1, -6, 5, 12, -10, 2, -2, 7, 7, -7, -12, 
    9, -5, -13, 11, 2, -10, 1, -1, 8, 4, 11, 3, 4, -8, 2, -1, -8, -12, 9, -6, -11, 0, 10, 12, -10, 7, -11, -4, 0, 5, 
    13, -5, -6, -1, 1, -7, 4, 2, 11, -3, -3, -2, 2, -1, 1, 6, 3, -10, -8, -1, -11, 5, -5, -10, -11, -10, 2, 10, -11, 
    -1, -2, 6, 5, 3, -9, -4, 6, 3, 5, -4, -1, -8, 1, 10, 6, -12, -4, -3, 9, 8, 7, 8, -11, -7, 6, -11, 9, 9, -7, 5, 12, 
    -4, -11, -12, 2, 2, 4, 6, 7, -9, -12, -8, -7, -3, 0, 7, 8, 5, 3, 1, 12, 10, 5, 12, -8, 11, -13, 7, -10, 3, 9, 10, 
    1, 11, 7, -8, 7, 4, -8, -12, -5, 10, 9, 4, -2, 5, 1, -10, -7, -9, 8, 3, 8, 7, -2, -2, 0, -13, 9, 7, 11, 0, 2, 2, 
    -1, -7, 5, 10, -3, -1, 2, 0, 6, -11, -8, -9, -10, -5, 0, 9, -1, -5, 10, 5, -9, 2, -7, -11, 7, 5, -4, 0, -1, 1, 3, 
    9, 9, -11, -9, 4, -8, 11, -11, 9, -6, -7, 7, 4, 1, 7, 5, 9, 9, 10, -11, 11, -11, 10, 4, 9, -1, -11, -10, 4, -7, 0, 
    4, 0, -7, 5, -11, 1, 3, 11, 9, 7, 12, -12, -5, -8, -7, 2, -3, 10, -8, 0, -3, 11, -2, -7, -2, 5, 10, 11, 9, 1, 9, 
    -1, -2, -6, 4, 9, 2, 9, 8, -5, 13, 5, 2, -2, -4, 3, -9, 7, -9, -3, 10, -8, 0, 9, -7, -12, 6, -10, -9, 0, -7, 5, -8, 
    2, 8, 7, 11, 3, -8, 12, 9, 9, 7, -1, -9, -4, -2, 11, 13, 8, 0, 3, 10, -8, -12, -3, 11, -7, 7, 5, 3, -3, 8, -4, 7, 
    -4, -10, -2, -8, 12, 10, 11, 10, 1, -11, 3, 6, 0, 3, -2, 0, 0, -13, 11, -8, -9, 10, 3, -8, 8, 4, 10, 10, -9, 10, 
    13, 7, -10, -9, -12, -11, 3, 6, 5, -5, 10, -6, 1, 7, -11, 13, 5, 8, -2, 7, -6, 3, 9, -7, 1, 6, -3, 2, -2, 1, 0, 4, 
    -1, -5, 11, -8, -1, 9, 7, -10, -7, -7, 4, -2, -2, -2, 12, 7, 11, -1, 3, 5, -4, -12, 13, -10, -11, -5, -6, -4, -2, 
    -5, -9, -4, -12, 9, 5, 9, 0, -3, -11, 4, 12, 4, -9, 5, 8, -6, 10, -5, -10, -8, 5, -9, 9, 2, -8, 11};

static const q7_t FLOW_0_WAVENET_RESBLOCKS_1_ENC_CONV_BIAS[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static const pv_ypu_config_mem_t FLOW_0_WAVENET_RESBLOCKS_1_ENC_CONV_WEIGHT_CONFIG = {
    .size_bytes = sizeof(FLOW_0_WAVENET_RESBLOCKS_1_ENC_CONV_WEIGHT),
    .flags = PV_YPU_DEVICE_MEM_FLAG_STATIC,
    .data = (void *) FLOW_0_WAVENET_RESBLOCKS_1_ENC_CONV_WEIGHT,
};

static const pv_ypu_config_mem_t FLOW_0_WAVENET_RESBLOCKS_1_ENC_CONV_BIAS_CONFIG = {
    .size_bytes = sizeof(FLOW_0_WAVENET_RESBLOCKS_1_ENC_CONV_BIAS),
    .flags = PV_YPU_DEVICE_MEM_FLAG_STATIC,
    .data = (void *) FLOW_0_WAVENET_RESBLOCKS_1_ENC_CONV_BIAS,
};

static const pv_cnn_param_t FLOW_0_WAVENET_RESBLOCKS_1_ENC_CONV_PARAM = {
        .input_channels = 8,
        .output_channels = 16,
        .kernel_size = 5,
        .stride = 1,
        .padding = 2,
        .dilation = 1,
        .weight = (pv_ypu_config_mem_t *) &FLOW_0_WAVENET_RESBLOCKS_1_ENC_CONV_WEIGHT_CONFIG,
        .bias = (pv_ypu_config_mem_t *) &FLOW_0_WAVENET_RESBLOCKS_1_ENC_CONV_BIAS_CONFIG,
};

static const q7_t FLOW_0_WAVENET_RESBLOCKS_1_ENC_CONV_SKIP_WEIGHT[] = {
    -8, -4, 10, 4, 0, 4, 4, 11, 2, 4, 9, 6, 3, 0, 4, 1, 12, 12, 5, 9, 7, -8, 12, 4, -10, 0, 6, 6, 3, 9, 0, 10, 7, 9, 
    11, -9, 7, 8, 0, 7, 0, -4, 13, -9, 0, -7, -8, 8, 8, 3, -12, -7, -9, 4, -8, 11, 12, 9, -3, 0, -5, 3, 0, -5};

static const q7_t FLOW_0_WAVENET_RESBLOCKS_1_ENC_CONV_SKIP_BIAS[] = {
    0, 0, 0, 0, 0, 0, 0, 0};

static const pv_ypu_config_mem_t FLOW_0_WAVENET_RESBLOCKS_1_ENC_CONV_SKIP_WEIGHT_CONFIG = {
    .size_bytes = sizeof(FLOW_0_WAVENET_RESBLOCKS_1_ENC_CONV_SKIP_WEIGHT),
    .flags = PV_YPU_DEVICE_MEM_FLAG_STATIC,
    .data = (void *) FLOW_0_WAVENET_RESBLOCKS_1_ENC_CONV_SKIP_WEIGHT,
};

static const pv_ypu_config_mem_t FLOW_0_WAVENET_RESBLOCKS_1_ENC_CONV_SKIP_BIAS_CONFIG = {
    .size_bytes = sizeof(FLOW_0_WAVENET_RESBLOCKS_1_ENC_CONV_SKIP_BIAS),
    .flags = PV_YPU_DEVICE_MEM_FLAG_STATIC,
    .data = (void *) FLOW_0_WAVENET_RESBLOCKS_1_ENC_CONV_SKIP_BIAS,
};

static const pv_cnn_param_t FLOW_0_WAVENET_RESBLOCKS_1_ENC_CONV_SKIP_PARAM = {
        .input_channels = 8,
        .output_channels = 8,
        .kernel_size = 1,
        .stride = 1,
        .padding = 0,
        .dilation = 1,
        .weight = (pv_ypu_config_mem_t *) &FLOW_0_WAVENET_RESBLOCKS_1_ENC_CONV_SKIP_WEIGHT_CONFIG,
        .bias = (pv_ypu_config_mem_t *) &FLOW_0_WAVENET_RESBLOCKS_1_ENC_CONV_SKIP_BIAS_CONFIG,
};

static const pv_wavenet_resblock_param_t FLOW_0_WAVENET_RESBLOCKS_1_PARAM = {
        .conv_param = &FLOW_0_WAVENET_RESBLOCKS_1_ENC_CONV_PARAM,
        .conv_skip_param = &FLOW_0_WAVENET_RESBLOCKS_1_ENC_CONV_SKIP_PARAM,
};

static const pv_wavenet_resblock_param_t *FLOW_0_WAVENET_RESBLOCKS[] = {
        &FLOW_0_WAVENET_RESBLOCKS_0_PARAM,
        &FLOW_0_WAVENET_RESBLOCKS_1_PARAM,
};

static const pv_residual_coupling_param_t FLOW_0_PARAM = {
        .num_wavenet_resblocks = 2,
        .conv_pre_param = &FLOW_0_CONV_PRE_PARAM,
        .conv_post_param = &FLOW_0_CONV_POST_PARAM,
        .wavenet_resblocks_param = FLOW_0_WAVENET_RESBLOCKS,
};

static const q7_t FLOW_1_CONV_PRE_WEIGHT[] = {
    9, 13, 5, -12, 11, -11, 6, -10, 11, -7, -3, 12, -2, 3, -8, 2, 7, -10, 5, -11, 11, -7, 5, 10, -11, 13, -11, -7, 12, 
    -9, 0, -7, 10, 1, -5, -10, 10, -6, -7, 5, 3, 8, -9, -7, 7, -1, -5, 4, -10, 11, -3, -11, 13, -2, 8, -4, 12, -2, -3, 
    -10, -7, 8, 4, 10};

static const q7_t FLOW_1_CONV_PRE_BIAS[] = {
    0, 0, 0, 0, 0, 0, 0, 0};

static const pv_ypu_config_mem_t FLOW_1_CONV_PRE_WEIGHT_CONFIG = {
    .size_bytes = sizeof(FLOW_1_CONV_PRE_WEIGHT),
    .flags = PV_YPU_DEVICE_MEM_FLAG_STATIC,
    .data = (void *) FLOW_1_CONV_PRE_WEIGHT,
};

static const pv_ypu_config_mem_t FLOW_1_CONV_PRE_BIAS_CONFIG = {
    .size_bytes = sizeof(FLOW_1_CONV_PRE_BIAS),
    .flags = PV_YPU_DEVICE_MEM_FLAG_STATIC,
    .data = (void *) FLOW_1_CONV_PRE_BIAS,
};

static const pv_cnn_param_t FLOW_1_CONV_PRE_PARAM = {
        .input_channels = 8,
        .output_channels = 8,
        .kernel_size = 1,
        .stride = 1,
        .padding = 0,
        .dilation = 1,
        .weight = (pv_ypu_config_mem_t *) &FLOW_1_CONV_PRE_WEIGHT_CONFIG,
        .bias = (pv_ypu_config_mem_t *) &FLOW_1_CONV_PRE_BIAS_CONFIG,
};

static const q7_t FLOW_1_CONV_POST_WEIGHT[] = {
    4, 10, 4, 1, 10, 10, 10, 1, 5, 4, 6, 2, 9, 11, 9, 11, 7, 12, 8, 12, 6, 5, 12, 3, 7, 7, 10, 8, 3, 2, 11, 9, 6, 2, 2, 
    11, 12, 7, 6, 7, 12, 1, 2, 2, 6, 7, 6, 4, 3, 2, 12, 0, 8, 5, 6, 11, 7, 4, 3, 11, 11, 12, 5, 9};

static const q7_t FLOW_1_CONV_POST_BIAS[] = {
    0, 0, 0, 0, 0, 0, 0, 0};

static const pv_ypu_config_mem_t FLOW_1_CONV_POST_WEIGHT_CONFIG = {
    .size_bytes = sizeof(FLOW_1_CONV_POST_WEIGHT),
    .flags = PV_YPU_DEVICE_MEM_FLAG_STATIC,
    .data = (void *) FLOW_1_CONV_POST_WEIGHT,
};

static const pv_ypu_config_mem_t FLOW_1_CONV_POST_BIAS_CONFIG = {
    .size_bytes = sizeof(FLOW_1_CONV_POST_BIAS),
    .flags = PV_YPU_DEVICE_MEM_FLAG_STATIC,
    .data = (void *) FLOW_1_CONV_POST_BIAS,
};

static const pv_cnn_param_t FLOW_1_CONV_POST_PARAM = {
        .input_channels = 8,
        .output_channels = 8,
        .kernel_size = 1,
        .stride = 1,
        .padding = 0,
        .dilation = 1,
        .weight = (pv_ypu_config_mem_t *) &FLOW_1_CONV_POST_WEIGHT_CONFIG,
        .bias = (pv_ypu_config_mem_t *) &FLOW_1_CONV_POST_BIAS_CONFIG,
};

static const q7_t FLOW_1_WAVENET_RESBLOCKS_0_ENC_CONV_WEIGHT[] = {
    -5, 0, 5, 4, -6, 8, -6, 10, -2, 2, 11, -9, -3, -9, 5, 2, -12, 1, 9, 9, -13, 1, -8, -2, 0, -6, 8, 6, -9, 6, 1, 11, 
    -3, -12, 5, -2, 10, -1, 9, 11, -12, -11, -2, 4, 0, -9, -6, 10, -11, -11, -4, -2, 12, -3, -6, 8, -12, 10, -8, 8, 7, 
    4, 2, 11, 10, -7, 5, 12, -9, 10, -4, -3, 3, 12, -12, 4, -4, -12, -7, 10, -7, -5, 9, -5, 0, -7, 12, 9, -11, -10, -5, 
    0, -5, -3, 1, 3, -12, -5, 10, 7, 9, 12, -6, 3, 8, -3, -3, -1, 10, 7, 5, -7, 6, -7, 12, 8, -10, -5, 5, 12, 12, -1, 
    -8, -8, -9, 13, -12, -11, -10, -12, -10, -9, 7, 9, -10, 1, 10, 3, -2, -9, 0, -4, 11, -7, -12, 5, 4, -11, 6, -2, -6, 
    8, -6, -11, 5, 11, 7, -7, 1, -5, -5, -6, 0, -4, 12, 9, 10, 3, -2, 8, 7, 8, 4, 12, -5, -1, 7, 1, -3, -5, 5, 11, 10, 
    10, -6, -7, -2, -12, 6, 9, 2, 6, 10, 7, 3, -3, -1, 4, 1, -12, 1, 7, 0, 3, -9, -5, -8, 9, 11, 11, 8, -11, 2, -10, 
    -4, -2, 8, -2, -5, -10, 2, 4, -2, -1, -9, -1, -10, 5, -10, 10, 4, -2, -12, -5, 3, 8, 5, -1, 5, 12, -7, 6, -1, 10, 
    -1, -2, 2, 12, 11, -5, -10, -13, -7, -12, -9, -7, -2, 6, 11, -10, 3, 4, 8, 7, -7, -9, 9, 3, 0, 12, -13, -8, 1, -1, 
    -8, 2, -6, 13, -11, 12, -4, -2, 4, -2, -11, 4, -9, 6, -7, 9, 12, 10, -1, -11, 5, 10, -2, 10, 2, -4, -12, 11, 4, 0, 
    7, 3, -12, -5, 13, 11, 2, -8, -12, -3, -1, 8, -8, 1, 10, 8, -4, 0, -1, -5, -3, 4, -5, 6, 8, 4, -4, -7, 12, 7, -9, 
    9, -8, -1, 2, -3, 9, 0, 4, 12, 3, -1, 6, 13, 1, 4, 10, 9, -3, 12, 6, 8, -11, -8, -1, -12, 8, -10, 12, -1, 2, 5, 8, 
    7, 13, 8, -12, -1, 4, 6, -8, 10, 6, 1, -7, -5, -2, -11, 13, -5, -4, 12, 7, 12, 4, 2, -10, -12, -8, 6, -8, -1, 7, 
    -8, 0, 2, -4, 8, -11, 12, 6, 13, 10, 2, 2, -7, -1, -6, 12, 0, -5, 11, -7, -6, 13, 12, 4, 11, -11, 5, 6, -5, -1, -1, 
    -6, 2, 10, 4, 3, 4, 6, -4, 5, -2, -13, -7, 11, 5, 12, 6, 6, 7, -7, 12, -3, -10, 2, 1, 7, 2, 4, -8, 2, 8, 1, 4, -5, 
    -12, -4, 5, 4, -7, 5, 11, -10, 7, -6, 1, 9, -11, -12, 0, -4, -7, -5, -12, 7, 4, -6, -2, 11, 8, -11, -8, 10, -7, 11, 
    2, 9, 1, -8, -7, 12, -1, 4, 8, 11, -8, 4, -2, 0, 11, 6, -3, 11, -11, 6, 1, -2, -8, 12, 5, -5, 1, -6, -13, 5, 2, 9, 
    -4, 5, -1, 4, 9, 2, 3, -9, -7, 5, -12, -10, -1, 4, 12, -2, 4, 4, -8, 6, 10, -12, -10, -5, -1, 5, 6, -6, -4, -12, 
    -7, -8, 0, -2, -5, -9, -2, -4, 10, -8, -11, -3, 13, -4, 11, 7, 12, 6, -8, -4, 7, -4, -10, 4, 4, 4, -7, -6, 13, 9, 
    -2, 11, -1, 11, 4, 11, -2, 7, -7, -6, -1, -11, 3, -1, 2, 12, 11, 10, -1, -3, 10, -7, 1, 1, -4, 0, 8, 7, -13, -9, 
    -6, 3, 2, -4, -7, 6, -2, 2, -3, -8, -4, -1, -10, -11, -8, 0, 1, 3, 2, -6, -10, -4, 4, -11, -12, -9, 12};

static const q7_t FLOW_1_WAVENET_RESBLOCKS_0_ENC_CONV_BIAS[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static const pv_ypu_config_mem_t FLOW_1_WAVENET_RESBLOCKS_0_ENC_CONV_WEIGHT_CONFIG = {
    .size_bytes = sizeof(FLOW_1_WAVENET_RESBLOCKS_0_ENC_CONV_WEIGHT),
    .flags = PV_YPU_DEVICE_MEM_FLAG_STATIC,
    .data = (void *) FLOW_1_WAVENET_RESBLOCKS_0_ENC_CONV_WEIGHT,
};

static const pv_ypu_config_mem_t FLOW_1_WAVENET_RESBLOCKS_0_ENC_CONV_BIAS_CONFIG = {
    .size_bytes = sizeof(FLOW_1_WAVENET_RESBLOCKS_0_ENC_CONV_BIAS),
    .flags = PV_YPU_DEVICE_MEM_FLAG_STATIC,
    .data = (void *) FLOW_1_WAVENET_RESBLOCKS_0_ENC_CONV_BIAS,
};

static const pv_cnn_param_t FLOW_1_WAVENET_RESBLOCKS_0_ENC_CONV_PARAM = {
        .input_channels = 8,
        .output_channels = 16,
        .kernel_size = 5,
        .stride = 1,
        .padding = 2,
        .dilation = 1,
        .weight = (pv_ypu_config_mem_t *) &FLOW_1_WAVENET_RESBLOCKS_0_ENC_CONV_WEIGHT_CONFIG,
        .bias = (pv_ypu_config_mem_t *) &FLOW_1_WAVENET_RESBLOCKS_0_ENC_CONV_BIAS_CONFIG,
};

static const q7_t FLOW_1_WAVENET_RESBLOCKS_0_ENC_CONV_SKIP_WEIGHT[] = {
    -2, -3, 7, -13, -12, 4, -11, 11, 3, 5, 3, 5, -8, -7, 4, 4, -10, 10, -1, 4, 9, -8, 7, 4, -11, 1, -10, 4, 2, 8, -11, 
    12, -8, -12, -3, -10, 8, 9, 8, -13, 2, -1, -7, 7, 3, -8, -8, -4, 8, 3, -12, 5, -9, 12, -6, 4, 8, 6, -10, -3, -7, 
    -9, 13, 8, -1, -8, 10, 6, 13, -4, 9, -10, -10, -6, 8, 3, -6, 12, -11, -6, -3, 5, 8, 0, -12, 5, 7, -7, -13, 1, 5, 3, 
    7, -5, -6, -5, -3, 2, -11, 10, 8, 1, 10, -10, 8, -11, -2, -1, -5, 3, -3, 9, -2, -1, 1, -1, -9, 10, 4, 2, -9, 5, -1, 
    6, 6, 8, -10, -6};

static const q7_t FLOW_1_WAVENET_RESBLOCKS_0_ENC_CONV_SKIP_BIAS[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static const pv_ypu_config_mem_t FLOW_1_WAVENET_RESBLOCKS_0_ENC_CONV_SKIP_WEIGHT_CONFIG = {
    .size_bytes = sizeof(FLOW_1_WAVENET_RESBLOCKS_0_ENC_CONV_SKIP_WEIGHT),
    .flags = PV_YPU_DEVICE_MEM_FLAG_STATIC,
    .data = (void *) FLOW_1_WAVENET_RESBLOCKS_0_ENC_CONV_SKIP_WEIGHT,
};

static const pv_ypu_config_mem_t FLOW_1_WAVENET_RESBLOCKS_0_ENC_CONV_SKIP_BIAS_CONFIG = {
    .size_bytes = sizeof(FLOW_1_WAVENET_RESBLOCKS_0_ENC_CONV_SKIP_BIAS),
    .flags = PV_YPU_DEVICE_MEM_FLAG_STATIC,
    .data = (void *) FLOW_1_WAVENET_RESBLOCKS_0_ENC_CONV_SKIP_BIAS,
};

static const pv_cnn_param_t FLOW_1_WAVENET_RESBLOCKS_0_ENC_CONV_SKIP_PARAM = {
        .input_channels = 8,
        .output_channels = 16,
        .kernel_size = 1,
        .stride = 1,
        .padding = 0,
        .dilation = 1,
        .weight = (pv_ypu_config_mem_t *) &FLOW_1_WAVENET_RESBLOCKS_0_ENC_CONV_SKIP_WEIGHT_CONFIG,
        .bias = (pv_ypu_config_mem_t *) &FLOW_1_WAVENET_RESBLOCKS_0_ENC_CONV_SKIP_BIAS_CONFIG,
};

static const pv_wavenet_resblock_param_t FLOW_1_WAVENET_RESBLOCKS_0_PARAM = {
        .conv_param = &FLOW_1_WAVENET_RESBLOCKS_0_ENC_CONV_PARAM,
        .conv_skip_param = &FLOW_1_WAVENET_RESBLOCKS_0_ENC_CONV_SKIP_PARAM,
};

static const q7_t FLOW_1_WAVENET_RESBLOCKS_1_ENC_CONV_WEIGHT[] = {
    -4, -5, 10, -4, -5, -4, -11, -6, -9, 5, -3, 10, 3, -11, 3, -10, -5, 10, -10, 9, 2, 12, -6, -5, 4, 7, -9, 0, 2, -12, 
    0, -10, -9, 8, 3, 9, -6, -12, -9, 9, -13, -11, 2, -4, -3, 3, -11, -5, 0, -10, -12, -4, -5, -13, 6, 5, -9, 0, -13, 
    -6, -8, 12, 2, -7, -11, 7, 0, 7, -7, 0, 11, 3, 7, -8, 0, 12, -12, 4, -8, -8, -7, -1, 0, 1, 3, -6, -3, -2, -11, 9, 
    -5, 3, -2, -8, -13, 3, 12, -5, -5, -6, 3, -11, -2, -2, 0, -13, 0, -7, -6, -2, 4, 5, -10, 12, -12, 9, -13, 1, -4, 7, 
    -9, 10, -6, -3, 3, 3, 10, 3, -12, 7, 1, -7, 11, 7, 12, -8, 5, 11, 2, 4, 1, 12, -1, 10, -12, -4, 3, 2, 2, -7, -8, 
    11, -9, 13, -5, -5, 12, -10, 11, -11, 12, 11, 11, -1, -8, 8, -2, 3, 8, 0, -2, 9, -4, 1, 2, 10, 7, 1, 7, -9, -12, 
    -10, -10, -1, -7, -4, 6, 12, 10, 4, -2, 0, 11, -3, 11, -11, -12, -5, 12, -5, 1, 10, -1, 11, -2, -6, 6, 7, 1, 11, 
    -5, 5, 7, -11, -6, -7, -12, 1, -7, 7, 0, -10, 6, 11, -9, 0, 4, -6, -3, -12, 0, 0, 2, -8, -6, 2, 10, -9, -3, 6, 0, 
    -12, -7, 11, -1, -9, -3, -5, -5, 7, 5, 6, -12, 3, 12, -3, 0, 13, -11, 12, -2, 11, 5, 2, 7, 5, -3, 9, 3, 11, 6, -13, 
    7, -10, -5, 8, 5, 3, 4, -8, 4, 1, 11, -12, 2, 6, -3, 2, -8, -3, -5, 3, 10, -1, 9, -10, -2, 8, -11, 11, -9, 9, 3, 3, 
    -1, -4, -7, -11, 8, 1, 2, -3, 0, -12, -10, -10, -4, -10, -9, 8, -11, -12, -11, -11, -9, -5, -5, 9, 11, 4, 0, -2, 9, 
    1, -12, -6, -7, -5, -4, 6, -7, 12, -10, 6, 1, -3, 7, 13, 2, -8, 1, 2, -4, 8, -5, 3, 8, 2, -2, 0, 2, -5, 0, -6, 8, 
    3, 10, -6, 3, 7, 0, -4, 1, -1, -12, -8, -9, 13, 11, -1, 12, 1, -1, 7, -10, -1, 11, -12, 9, -11, -1, -11, 9, -12, 
    -8, 5, 3, 1, 11, -4, -9, -4, 7, 10, 5, -8, 0, -7, -13, 8, -7, -13, -1, 9, -6, 5, -1, -1, 9, 4, -9, -2, -3, 1, -2, 
    6, -2, 3, 0, 3, -4, 0, -5, -12, -5, -9, 11, -13, -3, -2, 12, -3, 6, 10, 7, -7, -3, -2, -8, -2, -10, 6, -10, -11, 
    11, -10, 0, 0, -10, -7, 6, 8, -3, -10, 9, -12, -2, -13, 10, 1, -1, -4, -9, -5, -11, 6, 2, -6, 8, 11, -3, -10, 2, 
    -11, 5, 6, 12, -2, 7, -1, 6, 9, 6, -1, 0, -12, -4, 2, 13, 11, -9, -4, -11, 0, -9, 3, 5, -9, -1, 12, -8, -3, -6, 8, 
    11, 0, 8, -5, 9, 11, 9, 5, -2, -12, -5, 4, 8, -3, -11, 8, -8, -2, 7, 11, -2, -11, -9, -4, 9, 0, 2, 1, 7, 9, 3, -4, 
    -5, -9, 2, -1, -5, 9, 0, 0, 6, 9, -4, 10, 11, -1, 10, 11, -1, 7, -12, -7, -2, 2, -1, 8, 8, -10, 8, 4, 2, 8, 1, -9, 
    -4, 12, 3, 1, 5, -9, -5, -8, 2, 12, 5, -2, 5, -12, -5, 6, 10, 8, 9, 9, -7, 2, 7, 8, -9, -13, 2, -11, 7, 2, -8, -12, 
    1, -11, 1, 1, 7, 10, 4, 3, 8, -6, 12, -13, -8, 7, -12, -13, -9, 1, -13, 0, 6, 5, -11, 7, 8, 4, 0, 3, 8, 4};

static const q7_t FLOW_1_WAVENET_RESBLOCKS_1_ENC_CONV_BIAS[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

static const pv_ypu_config_mem_t FLOW_1_WAVENET_RESBLOCKS_1_ENC_CONV_WEIGHT_CONFIG = {
    .size_bytes = sizeof(FLOW_1_WAVENET_RESBLOCKS_1_ENC_CONV_WEIGHT),
    .flags = PV_YPU_DEVICE_MEM_FLAG_STATIC,
    .data = (void *) FLOW_1_WAVENET_RESBLOCKS_1_ENC_CONV_WEIGHT,
};

static const pv_ypu_config_mem_t FLOW_1_WAVENET_RESBLOCKS_1_ENC_CONV_BIAS_CONFIG = {
    .size_bytes = sizeof(FLOW_1_WAVENET_RESBLOCKS_1_ENC_CONV_BIAS),
    .flags = PV_YPU_DEVICE_MEM_FLAG_STATIC,
    .data = (void *) FLOW_1_WAVENET_RESBLOCKS_1_ENC_CONV_BIAS,
};

static const pv_cnn_param_t FLOW_1_WAVENET_RESBLOCKS_1_ENC_CONV_PARAM = {
        .input_channels = 8,
        .output_channels = 16,
        .kernel_size = 5,
        .stride = 1,
        .padding = 2,
        .dilation = 1,
        .weight = (pv_ypu_config_mem_t *) &FLOW_1_WAVENET_RESBLOCKS_1_ENC_CONV_WEIGHT_CONFIG,
        .bias = (pv_ypu_config_mem_t *) &FLOW_1_WAVENET_RESBLOCKS_1_ENC_CONV_BIAS_CONFIG,
};

static const q7_t FLOW_1_WAVENET_RESBLOCKS_1_ENC_CONV_SKIP_WEIGHT[] = {
    3, -11, 13, -5, -4, -7, -1, -2, 12, 3, 6, -12, 7, -1, 10, -10, 11, -8, 10, 4, 12, -12, -6, 12, -1, -3, -4, -12, 2, 
    -3, 7, 10, 6, -9, 4, 1, 12, -2, 7, 4, 7, 0, -5, -10, -1, -9, -2, 13, 7, 9, -8, -4, -8, -5, 1, 12, -8, 5, 11, 1, 12, 
    -3, 7, 11};

static const q7_t FLOW_1_WAVENET_RESBLOCKS_1_ENC_CONV_SKIP_BIAS[] = {
    0, 0, 0, 0, 0, 0, 0, 0};

static const pv_ypu_config_mem_t FLOW_1_WAVENET_RESBLOCKS_1_ENC_CONV_SKIP_WEIGHT_CONFIG = {
    .size_bytes = sizeof(FLOW_1_WAVENET_RESBLOCKS_1_ENC_CONV_SKIP_WEIGHT),
    .flags = PV_YPU_DEVICE_MEM_FLAG_STATIC,
    .data = (void *) FLOW_1_WAVENET_RESBLOCKS_1_ENC_CONV_SKIP_WEIGHT,
};

static const pv_ypu_config_mem_t FLOW_1_WAVENET_RESBLOCKS_1_ENC_CONV_SKIP_BIAS_CONFIG = {
    .size_bytes = sizeof(FLOW_1_WAVENET_RESBLOCKS_1_ENC_CONV_SKIP_BIAS),
    .flags = PV_YPU_DEVICE_MEM_FLAG_STATIC,
    .data = (void *) FLOW_1_WAVENET_RESBLOCKS_1_ENC_CONV_SKIP_BIAS,
};

static const pv_cnn_param_t FLOW_1_WAVENET_RESBLOCKS_1_ENC_CONV_SKIP_PARAM = {
        .input_channels = 8,
        .output_channels = 8,
        .kernel_size = 1,
        .stride = 1,
        .padding = 0,
        .dilation = 1,
        .weight = (pv_ypu_config_mem_t *) &FLOW_1_WAVENET_RESBLOCKS_1_ENC_CONV_SKIP_WEIGHT_CONFIG,
        .bias = (pv_ypu_config_mem_t *) &FLOW_1_WAVENET_RESBLOCKS_1_ENC_CONV_SKIP_BIAS_CONFIG,
};

static const pv_wavenet_resblock_param_t FLOW_1_WAVENET_RESBLOCKS_1_PARAM = {
        .conv_param = &FLOW_1_WAVENET_RESBLOCKS_1_ENC_CONV_PARAM,
        .conv_skip_param = &FLOW_1_WAVENET_RESBLOCKS_1_ENC_CONV_SKIP_PARAM,
};

static const pv_wavenet_resblock_param_t *FLOW_1_WAVENET_RESBLOCKS[] = {
        &FLOW_1_WAVENET_RESBLOCKS_0_PARAM,
        &FLOW_1_WAVENET_RESBLOCKS_1_PARAM,
};

static const pv_residual_coupling_param_t FLOW_1_PARAM = {
        .num_wavenet_resblocks = 2,
        .conv_pre_param = &FLOW_1_CONV_PRE_PARAM,
        .conv_post_param = &FLOW_1_CONV_POST_PARAM,
        .wavenet_resblocks_param = FLOW_1_WAVENET_RESBLOCKS,
};

static const pv_residual_coupling_param_t *FLOWS_PARAM[] = {
        &FLOW_0_PARAM,
        &FLOW_1_PARAM,
};

static const pv_orca_flow_param_t FLOW_PARAM = {
        .num_flows = 2,
        .flows_param = FLOWS_PARAM,
};

const float TEST_ORCA_FLOW_INPUT[176] = {
    0.9943054914474487f, 0.9879740476608276f, -0.9130532741546631f, 0.8137416243553162f, -0.06260286271572113f, 1.0993751287460327f, -0.2768777906894684f, -1.2882760763168335f, 0.4366651475429535f, 0.4912671446800232f, -2.0337271690368652f, -0.1539347916841507f, 0.5338360071182251f, -0.8478547930717468f, 1.5186269283294678f, -1.2994645833969116f, 1.0700186491012573f, 0.23360590636730194f, 0.3357304334640503f, -0.003312170971184969f, -0.6252195239067078f, 1.4669550657272339f, -0.11580804735422134f, -0.5103858113288879f, 0.9791077971458435f, 1.267769694328308f, -0.9672501683235168f, -0.6656705141067505f, -1.6262092590332031f, -0.0689484179019928f, 1.192818284034729f, -1.963201880455017f, 1.495693325996399f, 0.5901704430580139f, -0.48331424593925476f, 0.008636483922600746f, 0.911597728729248f, 1.136540412902832f, 1.3350532054901123f, -0.8681640028953552f, 0.19334541261196136f, 0.5388903617858887f, -2.2533669471740723f, -0.552861213684082f, -0.8340277671813965f, -0.05342057719826698f, 0.26439031958580017f, -1.4291630983352661f, 0.03587142378091812f, 1.5761041641235352f, -0.6924213767051697f, -0.11940276622772217f, -0.6619770526885986f, 1.6215412616729736f, 1.5566482543945312f, -0.7096378207206726f, 0.33112233877182007f, -0.45140448212623596f, -1.4523650407791138f, -0.00654512457549572f, -0.4215202331542969f, 0.2664007842540741f, 0.9734750390052795f, -1.8458890914916992f, 0.7955874800682068f, 0.4821157455444336f, 0.3051069974899292f, 0.9616941213607788f, -0.13465826213359833f, 1.025089979171753f, 0.19403566420078278f, -1.3850281238555908f, 1.098765254020691f, 0.7744066715240479f, -1.6302570104599f, 0.8141222596168518f, -1.6424096822738647f, -1.0710971355438232f, 0.6928704380989075f, -1.280344009399414f, 0.377902090549469f, 0.631513774394989f, -0.5301932096481323f, 0.12101497501134872f, -1.2427160739898682f, 1.3188272714614868f, 1.0748326778411865f, -0.4321218430995941f, 0.7750934958457947f, 1.0912315845489502f, -1.7617721557617188f, 0.6275743246078491f, -1.6328192949295044f, -0.01912583038210869f, 0.918734073638916f, -1.3179757595062256f, 0.4809340834617615f, 0.9894796013832092f, -1.5010924339294434f, -0.053151149302721024f, 0.5302069187164307f, -0.24166058003902435f, 1.5110899209976196f, -0.0671372264623642f, 0.2475588023662567f, 0.8046465516090393f, -0.8293840289115906f, 0.439230352640152f, -1.298856496810913f, -1.5071974992752075f, 1.7028404474258423f, -1.2075072526931763f, -0.08131337910890579f, 1.152830719947815f, 0.730475902557373f, 1.0900079011917114f, -0.15501908957958221f, 0.7116581797599792f, -0.07538794726133347f, -0.6168003678321838f, 0.021680070087313652f, 0.07992421090602875f, -2.4476211071014404f, 0.32075604796409607f, -0.1397242397069931f, -0.5462788939476013f, 1.6516762971878052f, -1.6968640089035034f, 0.060130469501018524f, 1.0881179571151733f, -0.6278327703475952f, -0.2637239098548889f, -1.5781176090240479f, 0.6042597889900208f, 1.0166839361190796f, 0.5266656279563904f, 1.322942852973938f, 0.6547505259513855f, -2.1581168174743652f, 0.6903097033500671f, -0.6852370500564575f, -0.7493147253990173f, 1.0066237449645996f, -0.9081417322158813f, 0.6356406807899475f, 0.49783802032470703f, 1.101321816444397f, 0.7317166924476624f, 0.1540747880935669f, 0.34965601563453674f, 0.009950275532901287f, -2.4930636882781982f, 0.6382780075073242f, 1.5950117111206055f, -1.7286664247512817f, -0.8367001414299011f, -0.24169780313968658f, -0.7050041556358337f, 0.1601262390613556f, 0.13151755928993225f, -0.15408998727798462f, 0.1891770213842392f, 0.14545969665050507f, 0.6272910833358765f, -1.1517337560653687f, 1.2584232091903687f, 1.1111936569213867f, 0.06351090967655182f, 1.3506011962890625f, 1.4917845726013184f, -1.8797943592071533f, -0.8040680885314941f, -1.2570685148239136f, -1.1003576517105103f, 0.4731837809085846f, -0.36351248621940613f
};

const float TEST_ORCA_FLOW_TARGET[176] = {
    0.9941050410270691f, 0.9880252480506897f, -0.9120306968688965f, 0.8140797019004822f, -0.06308095902204514f, 1.1001653671264648f, -0.27641361951828003f, -1.2885817289352417f, 0.4357585310935974f, 0.49077892303466797f, -2.034505605697632f, -0.15434163808822632f, 0.5339333415031433f, -0.8479501605033875f, 1.518578290939331f, -1.299668550491333f, 1.0690135955810547f, 0.2331876903772354f, 0.3360726237297058f, -0.0034422262106090784f, -0.6257767677307129f, 1.4671789407730103f, -0.11625033617019653f, -0.5113188624382019f, 0.9803109169006348f, 1.2694240808486938f, -0.9656375646591187f, -0.6643646359443665f, -1.624681830406189f, -0.06882712244987488f, 1.1938605308532715f, -1.9613398313522339f, 1.4946575164794922f, 0.5894762277603149f, -0.4825963079929352f, 0.008963285945355892f, 0.9110714793205261f, 1.13710618019104f, 1.3348077535629272f, -0.8689866662025452f, 0.19391627609729767f, 0.5398760437965393f, -2.2523601055145264f, -0.5519657731056213f, -0.8329166769981384f, -0.05293143540620804f, 0.26502007246017456f, -1.4278135299682617f, 0.034975145012140274f, 1.5747889280319214f, -0.6925660967826843f, -0.11929798126220703f, -0.6629098057746887f, 1.6215969324111938f, 1.555937647819519f, -0.710663914680481f, 0.32985517382621765f, -0.45201700925827026f, -1.4527583122253418f, -0.006713485345244408f, -0.42139723896980286f, 0.2658843994140625f, 0.9729004502296448f, -1.8457781076431274f, 0.7946397066116333f, 0.48049724102020264f, 0.3051128089427948f, 0.9612166881561279f, -0.1359715610742569f, 1.0253913402557373f, 0.19312015175819397f, -1.386376976966858f, 1.0974581241607666f, 0.7736296057701111f, -1.631232738494873f, 0.8135582804679871f, -1.6421782970428467f, -1.0714021921157837f, 0.6925538778305054f, -1.280651330947876f, 0.3774701654911041f, 0.631256639957428f, -0.5297808051109314f, 0.12129674106836319f, -1.2427955865859985f, 1.319329023361206f, 1.074837565422058f, -0.4321775436401367f, 0.7761088013648987f, 1.092776894569397f, -1.7607715129852295f, 0.6284447908401489f, -1.6314870119094849f, -0.01876867562532425f, 0.9197267889976501f, -1.3164297342300415f, 0.48023903369903564f, 0.9886336922645569f, -1.5010219812393188f, -0.05301715061068535f, 0.530005156993866f, -0.24111145734786987f, 1.5106018781661987f, -0.06750866770744324f, 0.24630524218082428f, 0.8036407828330994f, -0.830163836479187f, 0.4385495185852051f, -1.2991915941238403f, -1.5075733661651611f, 1.701747179031372f, -1.2077974081039429f, -0.08174265921115875f, 1.1516436338424683f, 0.7306167483329773f, 1.0903223752975464f, -0.15531213581562042f, 0.7122766375541687f, -0.07575114816427231f, -0.6168427467346191f, 0.021827995777130127f, 0.07970543950796127f, -2.446922540664673f, 0.3209165036678314f, -0.13958121836185455f, -0.5465177297592163f, 1.650999665260315f, -1.6967185735702515f, 0.06023966148495674f, 1.087827205657959f, -0.627530038356781f, -0.2628595530986786f, -1.5774189233779907f, 0.604863166809082f, 1.0168043375015259f, 0.5275322794914246f, 1.3215036392211914f, 0.6544431447982788f, -2.1591601371765137f, 0.6897897720336914f, -0.6845777034759521f, -0.7493141889572144f, 1.0065876245498657f, -0.9076966643333435f, 0.635145366191864f, 0.49732574820518494f, 1.1009633541107178f, 0.7315782904624939f, 0.15374250710010529f, 0.34962984919548035f, 0.009625139646232128f, -2.492913007736206f, 0.6365798115730286f, 1.5932883024215698f, -1.7300442457199097f, -0.8379350304603577f, -0.24221229553222656f, -0.7054530382156372f, 0.15870791673660278f, 0.13051246106624603f, -0.15647980570793152f, 0.18780820071697235f, 0.14427337050437927f, 0.6266481280326843f, -1.1528109312057495f, 1.2576788663864136f, 1.1093755960464478f, 0.062403760850429535f, 1.350033164024353f, 1.4916470050811768f, -1.8803696632385254f, -0.8046022653579712f, -1.2565054893493652f, -1.1006675958633423f, 0.47292566299438477f, -0.3631381392478943f
};

const int32_t TEST_ORCA_FLOW_SEQUENCE_LENGTH = 11;

#endif
