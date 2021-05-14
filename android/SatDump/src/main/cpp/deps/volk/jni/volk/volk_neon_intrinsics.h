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
 * Copyright (c) 2016-2019 ARM Limited.
 *
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * _vtaylor_polyq_f32
 * _vlogq_f32
 *
 */

/* Copyright (C) 2011  Julien Pommier

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  (this is the zlib license)

  _vsincosq_f32

*/

/*
 * This file is intended to hold NEON intrinsics of intrinsics.
 * They should be used in VOLK kernels to avoid copy-pasta.
 */

#ifndef INCLUDE_VOLK_VOLK_NEON_INTRINSICS_H_
#define INCLUDE_VOLK_VOLK_NEON_INTRINSICS_H_
#include <arm_neon.h>


/* Magnitude squared for float32x4x2_t */
static inline float32x4_t _vmagnitudesquaredq_f32(float32x4x2_t cmplxValue)
{
    float32x4_t iValue, qValue, result;
    iValue = vmulq_f32(cmplxValue.val[0], cmplxValue.val[0]); // Square the values
    qValue = vmulq_f32(cmplxValue.val[1], cmplxValue.val[1]); // Square the values
    result = vaddq_f32(iValue, qValue);                       // Add the I2 and Q2 values
    return result;
}

/* Inverse square root for float32x4_t */
static inline float32x4_t _vinvsqrtq_f32(float32x4_t x)
{
    float32x4_t sqrt_reciprocal = vrsqrteq_f32(x);
    sqrt_reciprocal = vmulq_f32(
        vrsqrtsq_f32(vmulq_f32(x, sqrt_reciprocal), sqrt_reciprocal), sqrt_reciprocal);
    sqrt_reciprocal = vmulq_f32(
        vrsqrtsq_f32(vmulq_f32(x, sqrt_reciprocal), sqrt_reciprocal), sqrt_reciprocal);

    return sqrt_reciprocal;
}

/* Inverse */
static inline float32x4_t _vinvq_f32(float32x4_t x)
{
    // Newton's method
    float32x4_t recip = vrecpeq_f32(x);
    recip = vmulq_f32(vrecpsq_f32(x, recip), recip);
    recip = vmulq_f32(vrecpsq_f32(x, recip), recip);
    return recip;
}

/* Complex multiplication for float32x4x2_t */
static inline float32x4x2_t _vmultiply_complexq_f32(float32x4x2_t a_val,
                                                    float32x4x2_t b_val)
{
    float32x4x2_t tmp_real;
    float32x4x2_t tmp_imag;
    float32x4x2_t c_val;

    // multiply the real*real and imag*imag to get real result
    // a0r*b0r|a1r*b1r|a2r*b2r|a3r*b3r
    tmp_real.val[0] = vmulq_f32(a_val.val[0], b_val.val[0]);
    // a0i*b0i|a1i*b1i|a2i*b2i|a3i*b3i
    tmp_real.val[1] = vmulq_f32(a_val.val[1], b_val.val[1]);
    // Multiply cross terms to get the imaginary result
    // a0r*b0i|a1r*b1i|a2r*b2i|a3r*b3i
    tmp_imag.val[0] = vmulq_f32(a_val.val[0], b_val.val[1]);
    // a0i*b0r|a1i*b1r|a2i*b2r|a3i*b3r
    tmp_imag.val[1] = vmulq_f32(a_val.val[1], b_val.val[0]);
    // combine the products
    c_val.val[0] = vsubq_f32(tmp_real.val[0], tmp_real.val[1]);
    c_val.val[1] = vaddq_f32(tmp_imag.val[0], tmp_imag.val[1]);
    return c_val;
}

/* From ARM Compute Library, MIT license */
static inline float32x4_t _vtaylor_polyq_f32(float32x4_t x, const float32x4_t coeffs[8])
{
    float32x4_t cA = vmlaq_f32(coeffs[0], coeffs[4], x);
    float32x4_t cB = vmlaq_f32(coeffs[2], coeffs[6], x);
    float32x4_t cC = vmlaq_f32(coeffs[1], coeffs[5], x);
    float32x4_t cD = vmlaq_f32(coeffs[3], coeffs[7], x);
    float32x4_t x2 = vmulq_f32(x, x);
    float32x4_t x4 = vmulq_f32(x2, x2);
    float32x4_t res = vmlaq_f32(vmlaq_f32(cA, cB, x2), vmlaq_f32(cC, cD, x2), x4);
    return res;
}

/* Natural logarithm.
 * From ARM Compute Library, MIT license */
static inline float32x4_t _vlogq_f32(float32x4_t x)
{
    const float32x4_t log_tab[8] = {
        vdupq_n_f32(-2.29561495781f), vdupq_n_f32(-2.47071170807f),
        vdupq_n_f32(-5.68692588806f), vdupq_n_f32(-0.165253549814f),
        vdupq_n_f32(5.17591238022f),  vdupq_n_f32(0.844007015228f),
        vdupq_n_f32(4.58445882797f),  vdupq_n_f32(0.0141278216615f),
    };

    const int32x4_t CONST_127 = vdupq_n_s32(127);             // 127
    const float32x4_t CONST_LN2 = vdupq_n_f32(0.6931471805f); // ln(2)

    // Extract exponent
    int32x4_t m = vsubq_s32(
        vreinterpretq_s32_u32(vshrq_n_u32(vreinterpretq_u32_f32(x), 23)), CONST_127);
    float32x4_t val =
        vreinterpretq_f32_s32(vsubq_s32(vreinterpretq_s32_f32(x), vshlq_n_s32(m, 23)));

    // Polynomial Approximation
    float32x4_t poly = _vtaylor_polyq_f32(val, log_tab);

    // Reconstruct
    poly = vmlaq_f32(poly, vcvtq_f32_s32(m), CONST_LN2);

    return poly;
}

/* Evaluation of 4 sines & cosines at once.
 * Optimized from here (zlib license)
 * http://gruntthepeon.free.fr/ssemath/ */
static inline float32x4x2_t _vsincosq_f32(float32x4_t x)
{
    const float32x4_t c_minus_cephes_DP1 = vdupq_n_f32(-0.78515625);
    const float32x4_t c_minus_cephes_DP2 = vdupq_n_f32(-2.4187564849853515625e-4);
    const float32x4_t c_minus_cephes_DP3 = vdupq_n_f32(-3.77489497744594108e-8);
    const float32x4_t c_sincof_p0 = vdupq_n_f32(-1.9515295891e-4);
    const float32x4_t c_sincof_p1 = vdupq_n_f32(8.3321608736e-3);
    const float32x4_t c_sincof_p2 = vdupq_n_f32(-1.6666654611e-1);
    const float32x4_t c_coscof_p0 = vdupq_n_f32(2.443315711809948e-005);
    const float32x4_t c_coscof_p1 = vdupq_n_f32(-1.388731625493765e-003);
    const float32x4_t c_coscof_p2 = vdupq_n_f32(4.166664568298827e-002);
    const float32x4_t c_cephes_FOPI = vdupq_n_f32(1.27323954473516); // 4 / M_PI

    const float32x4_t CONST_1 = vdupq_n_f32(1.f);
    const float32x4_t CONST_1_2 = vdupq_n_f32(0.5f);
    const float32x4_t CONST_0 = vdupq_n_f32(0.f);
    const uint32x4_t CONST_2 = vdupq_n_u32(2);
    const uint32x4_t CONST_4 = vdupq_n_u32(4);

    uint32x4_t emm2;

    uint32x4_t sign_mask_sin, sign_mask_cos;
    sign_mask_sin = vcltq_f32(x, CONST_0);
    x = vabsq_f32(x);
    // scale by 4/pi
    float32x4_t y = vmulq_f32(x, c_cephes_FOPI);

    // store the integer part of y in mm0
    emm2 = vcvtq_u32_f32(y);
    /* j=(j+1) & (~1) (see the cephes sources) */
    emm2 = vaddq_u32(emm2, vdupq_n_u32(1));
    emm2 = vandq_u32(emm2, vdupq_n_u32(~1));
    y = vcvtq_f32_u32(emm2);

    /* get the polynom selection mask
     there is one polynom for 0 <= x <= Pi/4
     and another one for Pi/4<x<=Pi/2
     Both branches will be computed. */
    const uint32x4_t poly_mask = vtstq_u32(emm2, CONST_2);

    // The magic pass: "Extended precision modular arithmetic"
    x = vmlaq_f32(x, y, c_minus_cephes_DP1);
    x = vmlaq_f32(x, y, c_minus_cephes_DP2);
    x = vmlaq_f32(x, y, c_minus_cephes_DP3);

    sign_mask_sin = veorq_u32(sign_mask_sin, vtstq_u32(emm2, CONST_4));
    sign_mask_cos = vtstq_u32(vsubq_u32(emm2, CONST_2), CONST_4);

    /* Evaluate the first polynom  (0 <= x <= Pi/4) in y1,
     and the second polynom      (Pi/4 <= x <= 0) in y2 */
    float32x4_t y1, y2;
    float32x4_t z = vmulq_f32(x, x);

    y1 = vmlaq_f32(c_coscof_p1, z, c_coscof_p0);
    y1 = vmlaq_f32(c_coscof_p2, z, y1);
    y1 = vmulq_f32(y1, z);
    y1 = vmulq_f32(y1, z);
    y1 = vmlsq_f32(y1, z, CONST_1_2);
    y1 = vaddq_f32(y1, CONST_1);

    y2 = vmlaq_f32(c_sincof_p1, z, c_sincof_p0);
    y2 = vmlaq_f32(c_sincof_p2, z, y2);
    y2 = vmulq_f32(y2, z);
    y2 = vmlaq_f32(x, x, y2);

    /* select the correct result from the two polynoms */
    const float32x4_t ys = vbslq_f32(poly_mask, y1, y2);
    const float32x4_t yc = vbslq_f32(poly_mask, y2, y1);

    float32x4x2_t sincos;
    sincos.val[0] = vbslq_f32(sign_mask_sin, vnegq_f32(ys), ys);
    sincos.val[1] = vbslq_f32(sign_mask_cos, yc, vnegq_f32(yc));

    return sincos;
}

static inline float32x4_t _vsinq_f32(float32x4_t x)
{
    const float32x4x2_t sincos = _vsincosq_f32(x);
    return sincos.val[0];
}

static inline float32x4_t _vcosq_f32(float32x4_t x)
{
    const float32x4x2_t sincos = _vsincosq_f32(x);
    return sincos.val[1];
}

static inline float32x4_t _vtanq_f32(float32x4_t x)
{
    const float32x4x2_t sincos = _vsincosq_f32(x);
    return vmulq_f32(sincos.val[0], _vinvq_f32(sincos.val[1]));
}

static inline float32x4_t _neon_accumulate_square_sum_f32(float32x4_t sq_acc,
                                                          float32x4_t acc,
                                                          float32x4_t val,
                                                          float32x4_t rec,
                                                          float32x4_t aux)
{
    aux = vmulq_f32(aux, val);
    aux = vsubq_f32(aux, acc);
    aux = vmulq_f32(aux, aux);
    return vfmaq_f32(sq_acc, aux, rec);
}

#endif /* INCLUDE_VOLK_VOLK_NEON_INTRINSICS_H_ */