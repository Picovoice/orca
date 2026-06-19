#include <stdlib.h>

#include "core/picovoice.h"
#include "core/pv_assert.h"
#include "core/pv_error.h"
#include "core/pv_error_messages.h"
#include "core/pv_type.h"
#include "orca/pv_orca_pqmf.h"

#ifdef __PV_MOCKS__

#include "orca/mock/pv_orca_mock.h"

#endif


static const int32_t PQMF_NUM_TAPS = 62;
static const int32_t PQMF_SYNTHESIS_FILTER_KERNEL_SIZE = PQMF_NUM_TAPS + 1;

static const int32_t PQMF_NUM_PADDING = 31;

// generated with `script/orca/generate_pqmf_helper_data.py`
static const int16_t PQMF_SYNTHESIS_FILTER[252] = { // shape: [num_subbands, kernel_size]
        1, 3, 2, -1, -2, 6, 30, 65, 84, 59, -10, -77, -60, 93, 330, 487,
        377, -44, -569, -800, -418, 501, 1370, 1348, -89, -2605, -4906, -5275, -2537, 3113, 9956, 15475,
        17577, 15648, 10813, 5275, 1151, -518, -50, 1348, 2419, 2520, 1783, 800, 134, -9, 214, 487,
        583, 467, 254, 77, 2, 12, 47, 65, 54, 30, 10, 1, -1, 0, 1, -1,
        -2, 1, -1, -6, 17, 54, 15, -71, -50, -2, -136, -169, 265, 583, 114, -320,
        38, -134, -1413, -1192, 1428, 2419, 316, 75, 2208, -1151, -9313, -7225, 8864, 17577, 3631, -14901,
        -13266, 2537, 9313, 3278, -1476, -89, 316, -2051, -2136, 418, 1413, 380, -25, 377, 114, -494,
        -396, 60, 136, 7, 34, 84, 15, -46, -25, 2, 1, -1, 1, 1, -1, -1,
        3, 0, 10, -25, -11, 76, -47, -34, -12, 27, 254, -396, -116, 574, -214, 25,
        -671, 281, 1783, -2136, -481, 1591, 50, 1476, -5787, 1852, 10813, -13266, -3496, 18254, -9956, -8864,
        12755, -1852, -4906, 2208, 18, 1591, -1370, -1428, 2104, -281, -569, 38, -75, 574, -330, -265,
        299, -27, -10, -50, -17, 76, -30, -17, 11, 0, 2, -2, 0, 0, 0, -1,
        -1, 11, -30, 46, -43, 17, 12, 7, -115, 299, -467, 494, -325, 75, -9, 380,
        -1198, 2104, -2520, 2051, -901, -18, -518, 3278, -7895, 12755, -15648, 14901, -10340, 3496, 3113, -7225,
        7895, -5787, 2605, -75, -901, 481, 501, -1192, 1198, -671, 44, 320, -325, 116, 93, -169,
        115, -12, -59, 71, -43, 11, 6, -6, 1, 3, -3, 1};

pv_status_t PV_MOCKABLE(pv_orca_pqmf_synthesis)(
        int32_t num_subbands,
        int32_t n,
        const float *x,
        float *y) {
    PV_ASSERT(num_subbands);
    PV_ASSERT(n);
    PV_ASSERT(x); // [num_subbands, n]
    PV_ASSERT(y);

    const int32_t n_expanded = num_subbands * n;

    const int32_t num_samples = num_subbands * n_expanded;
    const int32_t num_samples_padded = num_samples + (2 * PQMF_NUM_PADDING * num_subbands);

    int32_t *x_padded = calloc(num_samples_padded, sizeof(int32_t));
    if (!x_padded) {
        PV_ERROR_REPORT(
                &pv_error_msg_alloc,
                PV_ERROR_ARGS_PUBLIC_EMPTY(),
                PV_ERROR_ARGS_PRIVATE("x_padded"));
        return PV_STATUS_OUT_OF_MEMORY;
    }

    // TRANSFORM AND PAD
    // interleave with zeroes, add zero padding, new shape: [n + (2 * padding), num_subbands]
    int32_t index = 0;
    for (int32_t i = (num_subbands * PQMF_NUM_PADDING); i < (num_samples + (num_subbands * PQMF_NUM_PADDING)); i++) {
        if (((i + num_subbands) % (num_subbands * num_subbands)) == 0) {
            for (int32_t j = 0; j < num_subbands; j++) {
                int32_t sample = (index / num_subbands);
                int32_t subband = (index % num_subbands);
                x_padded[i + j] = pv_float_to_int32(4 * x[sample + (subband * n)] * (1U << 16));
                index++;
            }
        }
    }

    for (int32_t frame = 0; frame < n_expanded; frame++) {
        const int32_t frame_offset_in = frame * num_subbands;

        int64_t sum = 0;
        for (int32_t ic = 0; ic < num_subbands; ic++) {
            const int32_t subband_offset_filter = ic * PQMF_SYNTHESIS_FILTER_KERNEL_SIZE;

            for (int32_t ke = 0; ke < PQMF_SYNTHESIS_FILTER_KERNEL_SIZE; ke++) {
                int16_t filter_value = PQMF_SYNTHESIS_FILTER[subband_offset_filter + ke];
                int32_t x_value = x_padded[frame_offset_in + (ke * num_subbands) + ic];
                sum += (int64_t) x_value * (int64_t) filter_value;
            }
        }

        y[frame] = (float) sum / 4294967296.f;
    }

    free(x_padded);

    return PV_STATUS_SUCCESS;
}
