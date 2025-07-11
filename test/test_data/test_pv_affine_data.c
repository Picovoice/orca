#ifndef TEST_PV_AFFINE_DATA_H
#define TEST_PV_AFFINE_DATA_H

#include "orca/pv_affine.h"

static const float TEST_AFFINE_SCALE[] = {0.1f, 0.2f, 0.1f, 0.5f};

static const float TEST_AFFINE_BIAS[] = {1.0f, 0.5f, 1.5f, 0.7f};

static const pv_affine_param_t TEST_AFFINE_PARAM = {
        .num_channels = 4,
        .scale = TEST_AFFINE_SCALE,
        .bias = TEST_AFFINE_BIAS,
};

const float TEST_AFFINE_INPUT[12] = {
        0.1f, 0.2f, 0.3f, 0.4f,
        0.5f, 0.6f, 0.7f, 0.8f,
        0.9f, 1.0f, 1.1f, 1.2f,
};

const float TEST_AFFINE_TARGET[12] = {
        1.01f, 0.54f, 1.53f, 0.9f,
        1.05f, 0.62f, 1.57f, 1.1f,
        1.09f, 0.70f, 1.61f, 1.3f,
};

const int32_t TEST_AFFINE_SEQUENCE_LENGTH = 3;

#endif
