/* -*- c++ -*- */
/*
 * Copyright 2015 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

/*
 * This file is intended to hold AVX2 intrinsics of intrinsics.
 * They should be used in VOLK kernels to avoid copy-paste.
 */

#ifndef INCLUDE_VOLK_VOLK_AVX2_INTRINSICS_H_
#define INCLUDE_VOLK_VOLK_AVX2_INTRINSICS_H_
#include "volk/volk_avx_intrinsics.h"
#include <immintrin.h>

static inline __m256 _mm256_polar_sign_mask_avx2(__m128i fbits)
{
    const __m128i zeros = _mm_set1_epi8(0x00);
    const __m128i sign_extract = _mm_set1_epi8(0x80);
    const __m256i shuffle_mask = _mm256_setr_epi8(0xff,
                                                  0xff,
                                                  0xff,
                                                  0x00,
                                                  0xff,
                                                  0xff,
                                                  0xff,
                                                  0x01,
                                                  0xff,
                                                  0xff,
                                                  0xff,
                                                  0x02,
                                                  0xff,
                                                  0xff,
                                                  0xff,
                                                  0x03,
                                                  0xff,
                                                  0xff,
                                                  0xff,
                                                  0x04,
                                                  0xff,
                                                  0xff,
                                                  0xff,
                                                  0x05,
                                                  0xff,
                                                  0xff,
                                                  0xff,
                                                  0x06,
                                                  0xff,
                                                  0xff,
                                                  0xff,
                                                  0x07);
    __m256i sign_bits = _mm256_setzero_si256();

    fbits = _mm_cmpgt_epi8(fbits, zeros);
    fbits = _mm_and_si128(fbits, sign_extract);
    sign_bits = _mm256_insertf128_si256(sign_bits, fbits, 0);
    sign_bits = _mm256_insertf128_si256(sign_bits, fbits, 1);
    sign_bits = _mm256_shuffle_epi8(sign_bits, shuffle_mask);

    return _mm256_castsi256_ps(sign_bits);
}

static inline __m256
_mm256_polar_fsign_add_llrs_avx2(__m256 src0, __m256 src1, __m128i fbits)
{
    // prepare sign mask for correct +-
    __m256 sign_mask = _mm256_polar_sign_mask_avx2(fbits);

    __m256 llr0, llr1;
    _mm256_polar_deinterleave(&llr0, &llr1, src0, src1);

    // calculate result
    llr0 = _mm256_xor_ps(llr0, sign_mask);
    __m256 dst = _mm256_add_ps(llr0, llr1);
    return dst;
}

static inline __m256 _mm256_magnitudesquared_ps_avx2(const __m256 cplxValue0,
                                                     const __m256 cplxValue1)
{
    const __m256i idx = _mm256_set_epi32(7, 6, 3, 2, 5, 4, 1, 0);
    const __m256 squared0 = _mm256_mul_ps(cplxValue0, cplxValue0); // Square the values
    const __m256 squared1 = _mm256_mul_ps(cplxValue1, cplxValue1); // Square the Values
    const __m256 complex_result = _mm256_hadd_ps(squared0, squared1);
    return _mm256_permutevar8x32_ps(complex_result, idx);
}

static inline __m256 _mm256_scaled_norm_dist_ps_avx2(const __m256 symbols0,
                                                     const __m256 symbols1,
                                                     const __m256 points0,
                                                     const __m256 points1,
                                                     const __m256 scalar)
{
    /*
     * Calculate: |y - x|^2 * SNR_lin
     * Consider 'symbolsX' and 'pointsX' to be complex float
     * 'symbolsX' are 'y' and 'pointsX' are 'x'
     */
    const __m256 diff0 = _mm256_sub_ps(symbols0, points0);
    const __m256 diff1 = _mm256_sub_ps(symbols1, points1);
    const __m256 norms = _mm256_magnitudesquared_ps_avx2(diff0, diff1);
    return _mm256_mul_ps(norms, scalar);
}

/*
 * The function below vectorizes the inner loop of the following code:
 *
 * float max_values[8] = {0.f};
 * unsigned max_indices[8] = {0};
 * unsigned current_indices[8] = {0, 1, 2, 3, 4, 5, 6, 7};
 * for (unsigned i = 0; i < num_points / 8; ++i) {
 *     for (unsigned j = 0; j < 8; ++j) {
 *         float abs_squared = real(src0) * real(src0) + imag(src0) * imag(src1)
 *         bool compare = abs_squared > max_values[j];
 *         max_values[j] = compare ? abs_squared : max_values[j];
 *         max_indices[j] = compare ? current_indices[j] > max_indices[j]
 *         current_indices[j] += 8; // update for next outer loop iteration
 *         ++src0;
 *     }
 * }
 */
static inline void vector_32fc_index_max_variant0(__m256 in0,
                                                  __m256 in1,
                                                  __m256* max_values,
                                                  __m256i* max_indices,
                                                  __m256i* current_indices,
                                                  __m256i indices_increment)
{
    in0 = _mm256_mul_ps(in0, in0);
    in1 = _mm256_mul_ps(in1, in1);

    /*
     * Given the vectors a = (a_7, a_6, …, a_1, a_0) and b = (b_7, b_6, …, b_1, b_0)
     * hadd_ps(a, b) computes
     * (b_7 + b_6,
     *  b_5 + b_4,
     *  ---------
     *  a_7 + b_6,
     *  a_5 + a_4,
     *  ---------
     *  b_3 + b_2,
     *  b_1 + b_0,
     *  ---------
     *  a_3 + a_2,
     *  a_1 + a_0).
     * The result is the squared absolute value of complex numbers at index
     * offsets (7, 6, 3, 2, 5, 4, 1, 0). This must be the initial value of
     * current_indices!
     */
    __m256 abs_squared = _mm256_hadd_ps(in0, in1);

    /*
     * Compare the recently computed squared absolute values with the
     * previously determined maximum values. cmp_ps(a, b) determines
     * a > b ? 0xFFFFFFFF for each element in the vectors =>
     * compare_mask = abs_squared > max_values ? 0xFFFFFFFF : 0
     *
     * If either operand is NaN, 0 is returned as an “ordered” comparision is
     * used => the blend operation will select the value from *max_values.
     */
    __m256 compare_mask = _mm256_cmp_ps(abs_squared, *max_values, _CMP_GT_OS);

    /* Select maximum by blending. This is the only line which differs from variant1 */
    *max_values = _mm256_blendv_ps(*max_values, abs_squared, compare_mask);

    /*
     * Updates indices: blendv_ps(a, b, mask) determines mask ? b : a for
     * each element in the vectors =>
     * max_indices = compare_mask ? current_indices : max_indices
     *
     * Note: The casting of data types is required to make the compiler happy
     * and does not change values.
     */
    *max_indices =
        _mm256_castps_si256(_mm256_blendv_ps(_mm256_castsi256_ps(*max_indices),
                                             _mm256_castsi256_ps(*current_indices),
                                             compare_mask));

    /* compute indices of complex numbers which will be loaded in the next iteration */
    *current_indices = _mm256_add_epi32(*current_indices, indices_increment);
}

/* See _variant0 for details */
static inline void vector_32fc_index_max_variant1(__m256 in0,
                                                  __m256 in1,
                                                  __m256* max_values,
                                                  __m256i* max_indices,
                                                  __m256i* current_indices,
                                                  __m256i indices_increment)
{
    in0 = _mm256_mul_ps(in0, in0);
    in1 = _mm256_mul_ps(in1, in1);

    __m256 abs_squared = _mm256_hadd_ps(in0, in1);
    __m256 compare_mask = _mm256_cmp_ps(abs_squared, *max_values, _CMP_GT_OS);

    /*
     * This is the only line which differs from variant0. Using maxps instead of
     * blendvps is faster on Intel CPUs (on the ones tested with).
     *
     * Note: The order of arguments matters if a NaN is encountered in which
     * case the value of the second argument is selected. This is consistent
     * with the “ordered” comparision and the blend operation: The comparision
     * returns false if a NaN is encountered and the blend operation
     * consequently selects the value from max_indices.
     */
    *max_values = _mm256_max_ps(abs_squared, *max_values);

    *max_indices =
        _mm256_castps_si256(_mm256_blendv_ps(_mm256_castsi256_ps(*max_indices),
                                             _mm256_castsi256_ps(*current_indices),
                                             compare_mask));

    *current_indices = _mm256_add_epi32(*current_indices, indices_increment);
}

#endif /* INCLUDE_VOLK_VOLK_AVX2_INTRINSICS_H_ */
