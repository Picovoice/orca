#if (defined(__PV_ARM_NEON__) || defined(__PV_ARM_NEON_64__))

#include <arm_neon.h>

#endif

#if defined(__PV_TARGET_PLATFORM_ANDROID__) && defined(__PV_ARM_NEON__)

#include <cpu-features.h>

#endif

#if defined(__PV_TARGET_PLATFORM_WASM__) && defined(__PV_WASM_SIMD__)

#include "wasm_simd128.h"

#endif

#include "core/pv_type.h"
#include "orca/pv_profiler.h"

#if ((!(defined(__PV_ARM_NEON__) || defined(__PV_ARM_NEON_64__)) || defined(__PV_TARGET_PLATFORM_ANDROID__))) && !defined(__PV_WASM_SIMD__)

static void pv_cnn_kernel_gt1_q510_generic(
        const int32_t in_channels,
        const int32_t out_channels,
        const int32_t kernel_size,
        const q7_t *weight,
        const int32_t n,
        const q510_t *x,
        int32_t *y) {

    for (int32_t frame = 0; frame < n; frame++) {
        const q7_t *w = weight;
        const int32_t frame_offset_out = frame * out_channels;
        const int32_t frame_offset_in = frame * in_channels;

        for (int32_t oc = 0; oc < out_channels; oc++) {

            int32_t sum = 0;
            for (int32_t ke = 0; ke < kernel_size; ke++) {
                const int32_t kernel_offset = ke * in_channels;

                for (int32_t ic = 0; ic < in_channels; ic++) {
                    sum += (int32_t) x[frame_offset_in + kernel_offset + ic] * ((int32_t) *w++);
                }

            }

            y[frame_offset_out + oc] = sum;
        }
    }
}

#endif

#if defined(__PV_TARGET_PLATFORM_WASM__) && defined(__PV_WASM_SIMD__)

static void pv_cnn_kernel_gt1_q510_wasm(
        const int32_t in_channels,
        const int32_t out_channels,
        const int32_t kernel_size,
        const q7_t *weight,
        const int32_t n,
        const q510_t *x,
        int32_t *y) {

    for (int32_t frame = 0; frame < n; frame++) {
        const q7_t *w = weight;
        const q510_t *xx = x + (frame * in_channels);

        const int32_t frame_offset_out = frame * out_channels;

        for (int32_t oc = 0; oc < out_channels; oc += 4) {
            const q7_t *w_1 = w + (oc * in_channels * kernel_size);
            const q7_t *w_2 = w + ((oc + 1) * in_channels * kernel_size);
            const q7_t *w_3 = w + ((oc + 2) * in_channels * kernel_size);
            const q7_t *w_4 = w + ((oc + 3) * in_channels * kernel_size);

            int32_t sum_1 = 0;
            int32_t sum_2 = 0;
            int32_t sum_3 = 0;
            int32_t sum_4 = 0;
            for (int32_t ke = 0; ke < kernel_size; ke++) {
                const q510_t *xxx = xx + (ke * in_channels);

                v128_t sum_vec_1 = wasm_i32x4_splat(0);
                v128_t sum_vec_2 = wasm_i32x4_splat(0);
                v128_t sum_vec_3 = wasm_i32x4_splat(0);
                v128_t sum_vec_4 = wasm_i32x4_splat(0);

                int32_t i = in_channels >> 3;
                while (i) {
                    v128_t weight_vec_1 = wasm_i16x8_load8x8(w_1);
                    v128_t weight_vec_2 = wasm_i16x8_load8x8(w_2);
                    v128_t weight_vec_3 = wasm_i16x8_load8x8(w_3);
                    v128_t weight_vec_4 = wasm_i16x8_load8x8(w_4);

                    v128_t x_vec = wasm_v128_load(xxx);

                    sum_vec_1 = wasm_i32x4_add(sum_vec_1, wasm_i32x4_dot_i16x8(weight_vec_1, x_vec));
                    sum_vec_2 = wasm_i32x4_add(sum_vec_2, wasm_i32x4_dot_i16x8(weight_vec_2, x_vec));
                    sum_vec_3 = wasm_i32x4_add(sum_vec_3, wasm_i32x4_dot_i16x8(weight_vec_3, x_vec));
                    sum_vec_4 = wasm_i32x4_add(sum_vec_4, wasm_i32x4_dot_i16x8(weight_vec_4, x_vec));

                    xxx += 8;

                    w_1 += 8;
                    w_2 += 8;
                    w_3 += 8;
                    w_4 += 8;

                    i--;
                }

                i = in_channels & 7;
                while (i) {
                    sum_1 += (int32_t) *xxx * (int32_t) *w_1++;
                    sum_2 += (int32_t) *xxx * (int32_t) *w_2++;
                    sum_3 += (int32_t) *xxx * (int32_t) *w_3++;
                    sum_4 += (int32_t) *xxx++ * (int32_t) *w_4++;

                    i--;
                }

                sum_1 += wasm_i32x4_extract_lane(sum_vec_1, 0) + wasm_i32x4_extract_lane(sum_vec_1, 1) + wasm_i32x4_extract_lane(sum_vec_1, 2) + wasm_i32x4_extract_lane(sum_vec_1, 3);
                sum_2 += wasm_i32x4_extract_lane(sum_vec_2, 0) + wasm_i32x4_extract_lane(sum_vec_2, 1) + wasm_i32x4_extract_lane(sum_vec_2, 2) + wasm_i32x4_extract_lane(sum_vec_2, 3);
                sum_3 += wasm_i32x4_extract_lane(sum_vec_3, 0) + wasm_i32x4_extract_lane(sum_vec_3, 1) + wasm_i32x4_extract_lane(sum_vec_3, 2) + wasm_i32x4_extract_lane(sum_vec_3, 3);
                sum_4 += wasm_i32x4_extract_lane(sum_vec_4, 0) + wasm_i32x4_extract_lane(sum_vec_4, 1) + wasm_i32x4_extract_lane(sum_vec_4, 2) + wasm_i32x4_extract_lane(sum_vec_4, 3);
            }

            y[frame_offset_out + oc] = sum_1;
            y[frame_offset_out + (oc + 1)] = sum_2;
            y[frame_offset_out + (oc + 2)] = sum_3;
            y[frame_offset_out + (oc + 3)] = sum_4;
        }
    }

}

#endif

#if (defined(__PV_ARM_NEON__) || defined(__PV_ARM_NEON_64__))

static void pv_cnn_kernel_gt1_q510_neon(
        const int32_t in_channels,
        const int32_t out_channels,
        const int32_t kernel_size,
        const q7_t *weight,
        const int32_t n,
        const q510_t *x,
        int32_t *y) {

    for (int32_t frame = 0; frame < n; frame++) {
        const q7_t *w = weight;
        const q510_t *xx = x + (frame * in_channels);

        const int32_t frame_offset_out = frame * out_channels;

        for (int32_t oc = 0; oc < out_channels; oc += 8) {
            const q7_t *w_1 = w + (oc * in_channels * kernel_size);
            const q7_t *w_2 = w + ((oc + 1) * in_channels * kernel_size);
            const q7_t *w_3 = w + ((oc + 2) * in_channels * kernel_size);
            const q7_t *w_4 = w + ((oc + 3) * in_channels * kernel_size);
            const q7_t *w_5 = w + ((oc + 4) * in_channels * kernel_size);
            const q7_t *w_6 = w + ((oc + 5) * in_channels * kernel_size);
            const q7_t *w_7 = w + ((oc + 6) * in_channels * kernel_size);
            const q7_t *w_8 = w + ((oc + 7) * in_channels * kernel_size);

            int32_t sum_1 = 0;
            int32_t sum_2 = 0;
            int32_t sum_3 = 0;
            int32_t sum_4 = 0;
            int32_t sum_5 = 0;
            int32_t sum_6 = 0;
            int32_t sum_7 = 0;
            int32_t sum_8 = 0;

            for (int32_t ke = 0; ke < kernel_size; ke++) {
                const q510_t *xxx = xx + (ke * in_channels);

                int32x4_t sum_vec_1 = vdupq_n_s32(0);
                int32x4_t sum_vec_2 = vdupq_n_s32(0);
                int32x4_t sum_vec_3 = vdupq_n_s32(0);
                int32x4_t sum_vec_4 = vdupq_n_s32(0);
                int32x4_t sum_vec_5 = vdupq_n_s32(0);
                int32x4_t sum_vec_6 = vdupq_n_s32(0);
                int32x4_t sum_vec_7 = vdupq_n_s32(0);
                int32x4_t sum_vec_8 = vdupq_n_s32(0);

                int32_t i = in_channels >> 3;
                while (i) {
                    int16x8_t weight_vec_1 = vmovl_s8(vld1_s8(w_1));
                    int16x8_t weight_vec_2 = vmovl_s8(vld1_s8(w_2));
                    int16x8_t weight_vec_3 = vmovl_s8(vld1_s8(w_3));
                    int16x8_t weight_vec_4 = vmovl_s8(vld1_s8(w_4));
                    int16x8_t weight_vec_5 = vmovl_s8(vld1_s8(w_5));
                    int16x8_t weight_vec_6 = vmovl_s8(vld1_s8(w_6));
                    int16x8_t weight_vec_7 = vmovl_s8(vld1_s8(w_7));
                    int16x8_t weight_vec_8 = vmovl_s8(vld1_s8(w_8));

                    int16x8_t x_vec = vld1q_s16(xxx);

#ifdef __PV_ARM_NEON_64__

                    sum_vec_1 = vmlal_s16(sum_vec_1, vget_low_s16(weight_vec_1), vget_low_s16(x_vec));
                    sum_vec_2 = vmlal_s16(sum_vec_2, vget_low_s16(weight_vec_2), vget_low_s16(x_vec));
                    sum_vec_3 = vmlal_s16(sum_vec_3, vget_low_s16(weight_vec_3), vget_low_s16(x_vec));
                    sum_vec_4 = vmlal_s16(sum_vec_4, vget_low_s16(weight_vec_4), vget_low_s16(x_vec));
                    sum_vec_5 = vmlal_s16(sum_vec_5, vget_low_s16(weight_vec_5), vget_low_s16(x_vec));
                    sum_vec_6 = vmlal_s16(sum_vec_6, vget_low_s16(weight_vec_6), vget_low_s16(x_vec));
                    sum_vec_7 = vmlal_s16(sum_vec_7, vget_low_s16(weight_vec_7), vget_low_s16(x_vec));
                    sum_vec_8 = vmlal_s16(sum_vec_8, vget_low_s16(weight_vec_8), vget_low_s16(x_vec));

                    sum_vec_1 = vmlal_high_s16(sum_vec_1, weight_vec_1, x_vec);
                    sum_vec_2 = vmlal_high_s16(sum_vec_2, weight_vec_2, x_vec);
                    sum_vec_3 = vmlal_high_s16(sum_vec_3, weight_vec_3, x_vec);
                    sum_vec_4 = vmlal_high_s16(sum_vec_4, weight_vec_4, x_vec);
                    sum_vec_5 = vmlal_high_s16(sum_vec_5, weight_vec_5, x_vec);
                    sum_vec_6 = vmlal_high_s16(sum_vec_6, weight_vec_6, x_vec);
                    sum_vec_7 = vmlal_high_s16(sum_vec_7, weight_vec_7, x_vec);
                    sum_vec_8 = vmlal_high_s16(sum_vec_8, weight_vec_8, x_vec);

#else

                    sum_vec_1 = vmlal_s16(sum_vec_1, vget_low_s16(weight_vec_1), vget_low_s16(x_vec));
                    sum_vec_1 = vmlal_s16(sum_vec_1, vget_high_s16(weight_vec_1), vget_high_s16(x_vec));
                    sum_vec_2 = vmlal_s16(sum_vec_2, vget_low_s16(weight_vec_2), vget_low_s16(x_vec));
                    sum_vec_2 = vmlal_s16(sum_vec_2, vget_high_s16(weight_vec_2), vget_high_s16(x_vec));
                    sum_vec_3 = vmlal_s16(sum_vec_3, vget_low_s16(weight_vec_3), vget_low_s16(x_vec));
                    sum_vec_3 = vmlal_s16(sum_vec_3, vget_high_s16(weight_vec_3), vget_high_s16(x_vec));
                    sum_vec_4 = vmlal_s16(sum_vec_4, vget_low_s16(weight_vec_4), vget_low_s16(x_vec));
                    sum_vec_4 = vmlal_s16(sum_vec_4, vget_high_s16(weight_vec_4), vget_high_s16(x_vec));
                    sum_vec_5 = vmlal_s16(sum_vec_5, vget_low_s16(weight_vec_5), vget_low_s16(x_vec));
                    sum_vec_5 = vmlal_s16(sum_vec_5, vget_high_s16(weight_vec_5), vget_high_s16(x_vec));
                    sum_vec_6 = vmlal_s16(sum_vec_6, vget_low_s16(weight_vec_6), vget_low_s16(x_vec));
                    sum_vec_6 = vmlal_s16(sum_vec_6, vget_high_s16(weight_vec_6), vget_high_s16(x_vec));
                    sum_vec_7 = vmlal_s16(sum_vec_7, vget_low_s16(weight_vec_7), vget_low_s16(x_vec));
                    sum_vec_7 = vmlal_s16(sum_vec_7, vget_high_s16(weight_vec_7), vget_high_s16(x_vec));
                    sum_vec_8 = vmlal_s16(sum_vec_8, vget_low_s16(weight_vec_8), vget_low_s16(x_vec));
                    sum_vec_8 = vmlal_s16(sum_vec_8, vget_high_s16(weight_vec_8), vget_high_s16(x_vec));

#endif

                    xxx += 8;

                    w_1 += 8;
                    w_2 += 8;
                    w_3 += 8;
                    w_4 += 8;
                    w_5 += 8;
                    w_6 += 8;
                    w_7 += 8;
                    w_8 += 8;

                    i--;
                }

                i = in_channels & 7;
                while (i) {
                    sum_1 += (int32_t) *xxx * (int32_t) *w_1++;
                    sum_2 += (int32_t) *xxx * (int32_t) *w_2++;
                    sum_3 += (int32_t) *xxx * (int32_t) *w_3++;
                    sum_4 += (int32_t) *xxx * (int32_t) *w_4++;
                    sum_5 += (int32_t) *xxx * (int32_t) *w_5++;
                    sum_6 += (int32_t) *xxx * (int32_t) *w_6++;
                    sum_7 += (int32_t) *xxx * (int32_t) *w_7++;
                    sum_8 += (int32_t) (*xxx++) * (int32_t) *w_8++;

                    i--;
                }

#ifdef __PV_ARM_NEON_64__

                sum_1 += vaddvq_s32(sum_vec_1);
                sum_2 += vaddvq_s32(sum_vec_2);
                sum_3 += vaddvq_s32(sum_vec_3);
                sum_4 += vaddvq_s32(sum_vec_4);
                sum_5 += vaddvq_s32(sum_vec_5);
                sum_6 += vaddvq_s32(sum_vec_6);
                sum_7 += vaddvq_s32(sum_vec_7);
                sum_8 += vaddvq_s32(sum_vec_8);

#else

                sum_1 += vgetq_lane_s32(sum_vec_1, 0) + vgetq_lane_s32(sum_vec_1, 1) + vgetq_lane_s32(sum_vec_1, 2) + vgetq_lane_s32(sum_vec_1, 3);
                sum_2 += vgetq_lane_s32(sum_vec_2, 0) + vgetq_lane_s32(sum_vec_2, 1) + vgetq_lane_s32(sum_vec_2, 2) + vgetq_lane_s32(sum_vec_2, 3);
                sum_3 += vgetq_lane_s32(sum_vec_3, 0) + vgetq_lane_s32(sum_vec_3, 1) + vgetq_lane_s32(sum_vec_3, 2) + vgetq_lane_s32(sum_vec_3, 3);
                sum_4 += vgetq_lane_s32(sum_vec_4, 0) + vgetq_lane_s32(sum_vec_4, 1) + vgetq_lane_s32(sum_vec_4, 2) + vgetq_lane_s32(sum_vec_4, 3);
                sum_5 += vgetq_lane_s32(sum_vec_5, 0) + vgetq_lane_s32(sum_vec_5, 1) + vgetq_lane_s32(sum_vec_5, 2) + vgetq_lane_s32(sum_vec_5, 3);
                sum_6 += vgetq_lane_s32(sum_vec_6, 0) + vgetq_lane_s32(sum_vec_6, 1) + vgetq_lane_s32(sum_vec_6, 2) + vgetq_lane_s32(sum_vec_6, 3);
                sum_7 += vgetq_lane_s32(sum_vec_7, 0) + vgetq_lane_s32(sum_vec_7, 1) + vgetq_lane_s32(sum_vec_7, 2) + vgetq_lane_s32(sum_vec_7, 3);
                sum_8 += vgetq_lane_s32(sum_vec_8, 0) + vgetq_lane_s32(sum_vec_8, 1) + vgetq_lane_s32(sum_vec_8, 2) + vgetq_lane_s32(sum_vec_8, 3);

#endif
            }

            y[frame_offset_out + oc] = sum_1;
            y[frame_offset_out + oc + 1] = sum_2;
            y[frame_offset_out + oc + 2] = sum_3;
            y[frame_offset_out + oc + 3] = sum_4;
            y[frame_offset_out + oc + 4] = sum_5;
            y[frame_offset_out + oc + 5] = sum_6;
            y[frame_offset_out + oc + 6] = sum_7;
            y[frame_offset_out + oc + 7] = sum_8;
        }
    }
}

#endif

static void pv_cnn_kernel_gt1_q510(
        const int32_t in_channels,
        const int32_t out_channels,
        const int32_t kernel_size,
        const q7_t *weight,
        const int32_t n,
        const q510_t *x,
        int32_t *y) {
    PV_ASSERT(in_channels > 0);
    PV_ASSERT(out_channels > 0);
    PV_ASSERT(kernel_size > 0);
    PV_ASSERT(weight);
    PV_ASSERT(n > 0);
    PV_ASSERT(x);
    PV_ASSERT(y);

#if (defined(__PV_ARM_NEON__) || defined(__PV_ARM_NEON_64__))

#ifdef __PV_TARGET_PLATFORM_ANDROID__

    if (android_getCpuFeatures() & ANDROID_CPU_ARM_FEATURE_NEON) {
        pv_cnn_kernel_gt1_q510_neon(in_channels, out_channels, kernel_size, weight, n, x, y);
    } else {
        pv_cnn_kernel_gt1_q510_generic(in_channels, out_channels, kernel_size, weight, n, x, y);
    }

#else

    pv_cnn_kernel_gt1_q510_neon(in_channels, out_channels, kernel_size, weight, n, x, y);

#endif

#elif defined(__PV_TARGET_PLATFORM_WASM__) && defined(__PV_WASM_SIMD__)

    pv_cnn_kernel_gt1_q510_wasm(in_channels, out_channels, kernel_size, weight, n, x, y);

#else

    PV_ORCA_PROFILER_START("cnn_kernel_gt1");
    pv_cnn_kernel_gt1_q510_generic(in_channels, out_channels, kernel_size, weight, n, x, y);
    PV_ORCA_PROFILER_STOP("cnn_kernel_gt1");

#endif

}

#if ((!(defined(__PV_ARM_NEON__) || defined(__PV_ARM_NEON_64__)) || defined(__PV_TARGET_PLATFORM_ANDROID__))) && !defined(__PV_WASM_SIMD__)

static void pv_cnn_kernel_1_q510_generic(
        const int32_t in_channels,
        const int32_t out_channels,
        const q7_t *weight,
        const int32_t n,
        const q510_t *x,
        int32_t *y) {

    for (int32_t frame = 0; frame < n; frame++) {
        const q7_t *w = weight;
        const int32_t frame_offset_in = frame * in_channels;
        const int32_t frame_offset_out = frame * out_channels;

        for (int32_t oc = 0; oc < out_channels; oc++) {

            int32_t sum = 0;
            for (int32_t ic = 0; ic < in_channels; ic++) {
                sum += (int32_t) x[frame_offset_in + ic] * ((int32_t) *w++);
            }

            y[frame_offset_out + oc] = sum;
        }
    }
}

#endif

#if defined(__PV_TARGET_PLATFORM_WASM__) && defined(__PV_WASM_SIMD__)

static void pv_cnn_kernel_1_q510_wasm(
        const int32_t in_channels,
        const int32_t out_channels,
        const q7_t *weight,
        const int32_t n,
        const q510_t *x,
        int32_t *y) {

    for (int32_t frame = 0; frame < n; frame++) {
        const q510_t *xx = x + (frame * in_channels);
        int32_t *yy = y + (frame * out_channels);
        const q7_t *w_1 = weight;

        int32_t o = out_channels;
        while (o > 3) {
            int32_t sum_1 = 0;
            v128_t sum_vec_1 = wasm_i32x4_splat(0);
            int32_t sum_2 = 0;
            v128_t sum_vec_2 = wasm_i32x4_splat(0);
            int32_t sum_3 = 0;
            v128_t sum_vec_3 = wasm_i32x4_splat(0);
            int32_t sum_4 = 0;
            v128_t sum_vec_4 = wasm_i32x4_splat(0);

            const q7_t *w_2 = w_1 + in_channels;
            const q7_t *w_3 = w_2 + in_channels;
            const q7_t *w_4 = w_3 + in_channels;

            int32_t i = in_channels >> 3;
            while (i) {
                v128_t weight_vec_1 = wasm_i16x8_load8x8(w_1);
                v128_t weight_vec_2 = wasm_i16x8_load8x8(w_2);
                v128_t weight_vec_3 = wasm_i16x8_load8x8(w_3);
                v128_t weight_vec_4 = wasm_i16x8_load8x8(w_4);

                v128_t x_vec = wasm_v128_load(xx);

                sum_vec_1 = wasm_i32x4_add(sum_vec_1, wasm_i32x4_dot_i16x8(weight_vec_1, x_vec));
                sum_vec_2 = wasm_i32x4_add(sum_vec_2, wasm_i32x4_dot_i16x8(weight_vec_2, x_vec));
                sum_vec_3 = wasm_i32x4_add(sum_vec_3, wasm_i32x4_dot_i16x8(weight_vec_3, x_vec));
                sum_vec_4 = wasm_i32x4_add(sum_vec_4, wasm_i32x4_dot_i16x8(weight_vec_4, x_vec));

                xx += 8;
                w_1 += 8;
                w_2 += 8;
                w_3 += 8;
                w_4 += 8;

                i--;
            }

            i = in_channels & 7;

            while (i) {
                sum_1 += (int32_t) *xx * (int32_t) *w_1++;
                sum_2 += (int32_t) *xx * (int32_t) *w_2++;
                sum_3 += (int32_t) *xx * (int32_t) *w_3++;
                sum_4 += (int32_t) (*xx++) * (int32_t) *w_4++;

                i--;
            }

            xx -= in_channels;
            w_1 = w_4;

            sum_1 += wasm_i32x4_extract_lane(sum_vec_1, 0) + wasm_i32x4_extract_lane(sum_vec_1, 1) + wasm_i32x4_extract_lane(sum_vec_1, 2) + wasm_i32x4_extract_lane(sum_vec_1, 3);
            sum_2 += wasm_i32x4_extract_lane(sum_vec_2, 0) + wasm_i32x4_extract_lane(sum_vec_2, 1) + wasm_i32x4_extract_lane(sum_vec_2, 2) + wasm_i32x4_extract_lane(sum_vec_2, 3);
            sum_3 += wasm_i32x4_extract_lane(sum_vec_3, 0) + wasm_i32x4_extract_lane(sum_vec_3, 1) + wasm_i32x4_extract_lane(sum_vec_3, 2) + wasm_i32x4_extract_lane(sum_vec_3, 3);
            sum_4 += wasm_i32x4_extract_lane(sum_vec_4, 0) + wasm_i32x4_extract_lane(sum_vec_4, 1) + wasm_i32x4_extract_lane(sum_vec_4, 2) + wasm_i32x4_extract_lane(sum_vec_4, 3);

            *yy++ = sum_1;
            *yy++ = sum_2;
            *yy++ = sum_3;
            *yy++ = sum_4;

            o -= 4;
        }

        while (o) {
            v128_t sum_vec = wasm_i32x4_splat(0);

            int32_t i = in_channels >> 3;
            while (i) {
                v128_t weight_vec = wasm_i16x8_load8x8(w_1);
                v128_t x_vec = wasm_v128_load(xx);
                sum_vec = wasm_i32x4_add(sum_vec, wasm_i32x4_dot_i16x8(weight_vec, x_vec));

                xx += 8;
                w_1 += 8;
                i--;
            }

            i = in_channels & 7;

            int32_t sum = 0;
            while (i) {
                sum += (int32_t) (*xx++) * (int32_t) *w_1++;

                i--;
            }

            xx -= in_channels;

            sum += wasm_i32x4_extract_lane(sum_vec, 0) + wasm_i32x4_extract_lane(sum_vec, 1) + wasm_i32x4_extract_lane(sum_vec, 2) + wasm_i32x4_extract_lane(sum_vec, 3);

            *yy++ = sum;

            o--;
        }
    }
}

#endif

#if (defined(__PV_ARM_NEON__) || defined(__PV_ARM_NEON_64__))

static void pv_cnn_kernel_1_q510_neon(
        const int32_t in_channels,
        const int32_t out_channels,
        const q7_t *weight,
        const int32_t n,
        const q510_t *x,
        int32_t *y) {

    for (int32_t frame = 0; frame < n; frame++) {
        const q510_t *xx = x + (frame * in_channels);
        int32_t *yy = y + (frame * out_channels);
        const q7_t *w_1 = weight;

        int32_t o = out_channels;
        while (o > 7) {
            int32_t sum_1 = 0;
            int32x4_t sum_vec_1 = vdupq_n_s32(0);
            int32_t sum_2 = 0;
            int32x4_t sum_vec_2 = vdupq_n_s32(0);
            int32_t sum_3 = 0;
            int32x4_t sum_vec_3 = vdupq_n_s32(0);
            int32_t sum_4 = 0;
            int32x4_t sum_vec_4 = vdupq_n_s32(0);
            int32_t sum_5 = 0;
            int32x4_t sum_vec_5 = vdupq_n_s32(0);
            int32_t sum_6 = 0;
            int32x4_t sum_vec_6 = vdupq_n_s32(0);
            int32_t sum_7 = 0;
            int32x4_t sum_vec_7 = vdupq_n_s32(0);
            int32_t sum_8 = 0;
            int32x4_t sum_vec_8 = vdupq_n_s32(0);

            const q7_t *w_2 = w_1 + in_channels;
            const q7_t *w_3 = w_2 + in_channels;
            const q7_t *w_4 = w_3 + in_channels;
            const q7_t *w_5 = w_4 + in_channels;
            const q7_t *w_6 = w_5 + in_channels;
            const q7_t *w_7 = w_6 + in_channels;
            const q7_t *w_8 = w_7 + in_channels;

            int32_t i = in_channels >> 3;
            while (i) {
                int16x8_t weight_vec_1 = vmovl_s8(vld1_s8(w_1));
                int16x8_t weight_vec_2 = vmovl_s8(vld1_s8(w_2));
                int16x8_t weight_vec_3 = vmovl_s8(vld1_s8(w_3));
                int16x8_t weight_vec_4 = vmovl_s8(vld1_s8(w_4));
                int16x8_t weight_vec_5 = vmovl_s8(vld1_s8(w_5));
                int16x8_t weight_vec_6 = vmovl_s8(vld1_s8(w_6));
                int16x8_t weight_vec_7 = vmovl_s8(vld1_s8(w_7));
                int16x8_t weight_vec_8 = vmovl_s8(vld1_s8(w_8));

                int16x8_t x_vec = vld1q_s16(xx);

#ifdef __PV_ARM_NEON_64__

                    sum_vec_1 = vmlal_s16(sum_vec_1, vget_low_s16(weight_vec_1), vget_low_s16(x_vec));
                    sum_vec_2 = vmlal_s16(sum_vec_2, vget_low_s16(weight_vec_2), vget_low_s16(x_vec));
                    sum_vec_3 = vmlal_s16(sum_vec_3, vget_low_s16(weight_vec_3), vget_low_s16(x_vec));
                    sum_vec_4 = vmlal_s16(sum_vec_4, vget_low_s16(weight_vec_4), vget_low_s16(x_vec));
                    sum_vec_5 = vmlal_s16(sum_vec_5, vget_low_s16(weight_vec_5), vget_low_s16(x_vec));
                    sum_vec_6 = vmlal_s16(sum_vec_6, vget_low_s16(weight_vec_6), vget_low_s16(x_vec));
                    sum_vec_7 = vmlal_s16(sum_vec_7, vget_low_s16(weight_vec_7), vget_low_s16(x_vec));
                    sum_vec_8 = vmlal_s16(sum_vec_8, vget_low_s16(weight_vec_8), vget_low_s16(x_vec));

                    sum_vec_1 = vmlal_high_s16(sum_vec_1, weight_vec_1, x_vec);
                    sum_vec_2 = vmlal_high_s16(sum_vec_2, weight_vec_2, x_vec);
                    sum_vec_3 = vmlal_high_s16(sum_vec_3, weight_vec_3, x_vec);
                    sum_vec_4 = vmlal_high_s16(sum_vec_4, weight_vec_4, x_vec);
                    sum_vec_5 = vmlal_high_s16(sum_vec_5, weight_vec_5, x_vec);
                    sum_vec_6 = vmlal_high_s16(sum_vec_6, weight_vec_6, x_vec);
                    sum_vec_7 = vmlal_high_s16(sum_vec_7, weight_vec_7, x_vec);
                    sum_vec_8 = vmlal_high_s16(sum_vec_8, weight_vec_8, x_vec);

#else

                sum_vec_1 = vmlal_s16(sum_vec_1, vget_low_s16(weight_vec_1), vget_low_s16(x_vec));
                sum_vec_1 = vmlal_s16(sum_vec_1, vget_high_s16(weight_vec_1), vget_high_s16(x_vec));
                sum_vec_2 = vmlal_s16(sum_vec_2, vget_low_s16(weight_vec_2), vget_low_s16(x_vec));
                sum_vec_2 = vmlal_s16(sum_vec_2, vget_high_s16(weight_vec_2), vget_high_s16(x_vec));
                sum_vec_3 = vmlal_s16(sum_vec_3, vget_low_s16(weight_vec_3), vget_low_s16(x_vec));
                sum_vec_3 = vmlal_s16(sum_vec_3, vget_high_s16(weight_vec_3), vget_high_s16(x_vec));
                sum_vec_4 = vmlal_s16(sum_vec_4, vget_low_s16(weight_vec_4), vget_low_s16(x_vec));
                sum_vec_4 = vmlal_s16(sum_vec_4, vget_high_s16(weight_vec_4), vget_high_s16(x_vec));
                sum_vec_5 = vmlal_s16(sum_vec_5, vget_low_s16(weight_vec_5), vget_low_s16(x_vec));
                sum_vec_5 = vmlal_s16(sum_vec_5, vget_high_s16(weight_vec_5), vget_high_s16(x_vec));
                sum_vec_6 = vmlal_s16(sum_vec_6, vget_low_s16(weight_vec_6), vget_low_s16(x_vec));
                sum_vec_6 = vmlal_s16(sum_vec_6, vget_high_s16(weight_vec_6), vget_high_s16(x_vec));
                sum_vec_7 = vmlal_s16(sum_vec_7, vget_low_s16(weight_vec_7), vget_low_s16(x_vec));
                sum_vec_7 = vmlal_s16(sum_vec_7, vget_high_s16(weight_vec_7), vget_high_s16(x_vec));
                sum_vec_8 = vmlal_s16(sum_vec_8, vget_low_s16(weight_vec_8), vget_low_s16(x_vec));
                sum_vec_8 = vmlal_s16(sum_vec_8, vget_high_s16(weight_vec_8), vget_high_s16(x_vec));

#endif

                xx += 8;
                w_1 += 8;
                w_2 += 8;
                w_3 += 8;
                w_4 += 8;
                w_5 += 8;
                w_6 += 8;
                w_7 += 8;
                w_8 += 8;

                i--;
            }

            i = in_channels & 7;

            while (i) {
                sum_1 += (int32_t) *xx * (int32_t) *w_1++;
                sum_2 += (int32_t) *xx * (int32_t) *w_2++;
                sum_3 += (int32_t) *xx * (int32_t) *w_3++;
                sum_4 += (int32_t) *xx * (int32_t) *w_4++;
                sum_5 += (int32_t) *xx * (int32_t) *w_5++;
                sum_6 += (int32_t) *xx * (int32_t) *w_6++;
                sum_7 += (int32_t) *xx * (int32_t) *w_7++;
                sum_8 += (int32_t) (*xx++) * (int32_t) *w_8++;

                i--;
            }

            xx -= in_channels;
            w_1 = w_8;

#ifdef __PV_ARM_NEON_64__

            sum_1 += vaddvq_s32(sum_vec_1);
            sum_2 += vaddvq_s32(sum_vec_2);
            sum_3 += vaddvq_s32(sum_vec_3);
            sum_4 += vaddvq_s32(sum_vec_4);
            sum_5 += vaddvq_s32(sum_vec_5);
            sum_6 += vaddvq_s32(sum_vec_6);
            sum_7 += vaddvq_s32(sum_vec_7);
            sum_8 += vaddvq_s32(sum_vec_8);

#else

            sum_1 += vgetq_lane_s32(sum_vec_1, 0) + vgetq_lane_s32(sum_vec_1, 1) + vgetq_lane_s32(sum_vec_1, 2) + vgetq_lane_s32(sum_vec_1, 3);
            sum_2 += vgetq_lane_s32(sum_vec_2, 0) + vgetq_lane_s32(sum_vec_2, 1) + vgetq_lane_s32(sum_vec_2, 2) + vgetq_lane_s32(sum_vec_2, 3);
            sum_3 += vgetq_lane_s32(sum_vec_3, 0) + vgetq_lane_s32(sum_vec_3, 1) + vgetq_lane_s32(sum_vec_3, 2) + vgetq_lane_s32(sum_vec_3, 3);
            sum_4 += vgetq_lane_s32(sum_vec_4, 0) + vgetq_lane_s32(sum_vec_4, 1) + vgetq_lane_s32(sum_vec_4, 2) + vgetq_lane_s32(sum_vec_4, 3);
            sum_5 += vgetq_lane_s32(sum_vec_5, 0) + vgetq_lane_s32(sum_vec_5, 1) + vgetq_lane_s32(sum_vec_5, 2) + vgetq_lane_s32(sum_vec_5, 3);
            sum_6 += vgetq_lane_s32(sum_vec_6, 0) + vgetq_lane_s32(sum_vec_6, 1) + vgetq_lane_s32(sum_vec_6, 2) + vgetq_lane_s32(sum_vec_6, 3);
            sum_7 += vgetq_lane_s32(sum_vec_7, 0) + vgetq_lane_s32(sum_vec_7, 1) + vgetq_lane_s32(sum_vec_7, 2) + vgetq_lane_s32(sum_vec_7, 3);
            sum_8 += vgetq_lane_s32(sum_vec_8, 0) + vgetq_lane_s32(sum_vec_8, 1) + vgetq_lane_s32(sum_vec_8, 2) + vgetq_lane_s32(sum_vec_8, 3);

#endif

            *yy++ = sum_1;
            *yy++ = sum_2;
            *yy++ = sum_3;
            *yy++ = sum_4;
            *yy++ = sum_5;
            *yy++ = sum_6;
            *yy++ = sum_7;
            *yy++ = sum_8;

            o -= 8;
        }

        while (o) {
            int32x4_t sum_vec = vdupq_n_s32(0);

            int32_t i = in_channels >> 3;
            while (i) {
                int16x8_t weight_vec = vmovl_s8(vld1_s8(w_1));
                int16x8_t x_vec = vld1q_s16(xx);

#ifdef __PV_ARM_NEON_64__

                sum_vec = vmlal_s16(sum_vec, vget_low_s16(weight_vec), vget_low_s16(x_vec));
                sum_vec = vmlal_high_s16(sum_vec, weight_vec, x_vec);

#else

                sum_vec = vmlal_s16(sum_vec, vget_low_s16(weight_vec), vget_low_s16(x_vec));
                sum_vec = vmlal_s16(sum_vec, vget_high_s16(weight_vec), vget_high_s16(x_vec));

#endif

                xx += 8;
                w_1 += 8;

                i--;
            }

            i = in_channels & 7;

            int32_t sum = 0;
            while (i) {
                sum += (int32_t) (*xx++) * (int32_t) *w_1++;

                i--;
            }

            xx -= in_channels;

#ifdef __PV_ARM_NEON_64__

            sum += vaddvq_s32(sum_vec);

#else

            sum += vgetq_lane_s32(sum_vec, 0) + vgetq_lane_s32(sum_vec, 1) + vgetq_lane_s32(sum_vec, 2) + vgetq_lane_s32(sum_vec, 3);

#endif

            *yy++ = sum;

            o--;
        }
    }
}

#endif

void pv_cnn_kernel_1_q510(
        const int32_t in_channels,
        const int32_t out_channels,
        const q7_t *weight,
        const int32_t n,
        const q510_t *x,
        int32_t *y) {
    PV_ASSERT(in_channels > 0);
    PV_ASSERT(out_channels > 0);
    PV_ASSERT(weight);
    PV_ASSERT(n > 0);
    PV_ASSERT(x);
    PV_ASSERT(y);

#if (defined(__PV_ARM_NEON__) || defined(__PV_ARM_NEON_64__))

#ifdef __PV_TARGET_PLATFORM_ANDROID__

    if (android_getCpuFeatures() & ANDROID_CPU_ARM_FEATURE_NEON) {
        pv_cnn_kernel_1_q510_neon(in_channels, out_channels, weight, n, x, y);
    } else {
        pv_cnn_kernel_1_q510_generic(in_channels, out_channels, weight, n, x, y);
    }

#else

    pv_cnn_kernel_1_q510_neon(in_channels, out_channels, weight, n, x, y);

#endif

#elif defined(__PV_TARGET_PLATFORM_WASM__) && defined(__PV_WASM_SIMD__)

    pv_cnn_kernel_1_q510_wasm(in_channels, out_channels, weight, n, x, y);

#else

    PV_ORCA_PROFILER_START("cnn_kernel_1");
    pv_cnn_kernel_1_q510_generic(in_channels, out_channels, weight, n, x, y);
    PV_ORCA_PROFILER_STOP("cnn_kernel_1");

#endif

}

void pv_cnn_kernel_3_q510(
        const int32_t in_channels,
        const int32_t out_channels,
        const q7_t *weight,
        const int32_t n,
        const q510_t *x,
        int32_t *y) {
    PV_ORCA_PROFILER_START("cnn_kernel_3");
    pv_cnn_kernel_gt1_q510(in_channels, out_channels, 3, weight, n, x, y);
    PV_ORCA_PROFILER_STOP("cnn_kernel_3");
}

void pv_cnn_kernel_5_q510(
        const int32_t in_channels,
        const int32_t out_channels,
        const q7_t *weight,
        const int32_t n,
        const q510_t *x,
        int32_t *y) {
    PV_ORCA_PROFILER_START("cnn_kernel_5");
    pv_cnn_kernel_gt1_q510(in_channels, out_channels, 5, weight, n, x, y);
    PV_ORCA_PROFILER_STOP("cnn_kernel_5");
}

void pv_cnn_kernel_7_q510(
        const int32_t in_channels,
        const int32_t out_channels,
        const q7_t *weight,
        const int32_t n,
        const q510_t *x,
        int32_t *y) {
    PV_ORCA_PROFILER_START("cnn_kernel_7");
    pv_cnn_kernel_gt1_q510(in_channels, out_channels, 7, weight, n, x, y);
    PV_ORCA_PROFILER_STOP("cnn_kernel_7");
}
