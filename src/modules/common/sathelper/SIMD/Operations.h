/*
 * Operations.h
 *
 *  Created on: 08/01/2017
 *      Author: Lucas Teske
 */

#pragma once

#include "SIMD.h"

#if defined(_M_IX86) || defined(_M_X86) || defined(__i386__) || defined(__x86_64) || defined(_M_X64)
#define MEMORY_OP_X86
#if defined __GNUC__
#include <x86intrin.h>
#else
#include <immintrin.h>
#endif
#elif defined(__arm__) || defined(_M_ARM) || defined(__aarch64__)
#include <arm_neon.h>
#ifdef FORCE_NEON_CODE
#define MEMORY_OP_ARM
#else
#define MEMORY_OP_GENERIC
#endif
#else
#define MEMORY_OP_GENERIC
#endif

#include <complex>

namespace sathelper
{

  class Operations
  {
  public:
#ifdef MEMORY_OP_X86
    static inline void dotProduct(std::complex<float> *result, const std::complex<float> *input, const float *taps, unsigned int length)
    {
      const unsigned int runs = length / 8; // number of complexes.

      float res[2];
      SIMD_ALIGNED(16)
      float vectorResult[4];

      __m128 c0, c1, c2, c3;
      __m128 tap0, tap1, tap2, tap3;
      __m128 tmp0, tmp1, tmp2, tmp3;

      __m128 t0, t1, t2, t3; // Interleaved Tap
      __m128 r0, r1, r2, r3; // Dot Product Result

      // Initialize results with 0
      r0 = _mm_setzero_ps();
      r1 = _mm_setzero_ps();
      r2 = _mm_setzero_ps();
      r3 = _mm_setzero_ps();

      float *iPtr = (float *)input;
      float *tPtr = (float *)taps;

      for (unsigned int i = 0; i < runs; i++)
      {

        // Load inputs to SIMD registers
        tmp0 = _mm_loadu_ps(iPtr + 0);
        tmp1 = _mm_loadu_ps(iPtr + 4);
        tmp2 = _mm_loadu_ps(iPtr + 8);
        tmp3 = _mm_loadu_ps(iPtr + 12);

        // Load taps to SIMD registers
        tap0 = _mm_load_ps(tPtr + 0);
        tap1 = _mm_load_ps(tPtr + 0); // Same as tap0
        tap2 = _mm_load_ps(tPtr + 4);
        tap3 = _mm_load_ps(tPtr + 4); // Same as tap2

        // Interleave taps to multiply
        // Basicaly we have in tapN a value like:
        //      v0, v1, v2, v3 ...
        // And since we have a complex input, we should have
        //      v0, v0, v1, v1 ...
        // So we "unpack" to t0, t1, t2, t3.
        // tap0 == tap1 and tap2 == tap3. So the
        // unpack will be v0, v0, v1, v1 etc...

        t0 = _mm_unpacklo_ps(tap0, tap1);
        t2 = _mm_unpacklo_ps(tap2, tap3);
        t1 = _mm_unpackhi_ps(tap0, tap1);
        t3 = _mm_unpackhi_ps(tap2, tap3);

        // Multiply taps for IQ
        c0 = _mm_mul_ps(tmp0, t0);
        c1 = _mm_mul_ps(tmp1, t1);
        c2 = _mm_mul_ps(tmp2, t2);
        c3 = _mm_mul_ps(tmp3, t3);

        // Add the result of multiplication to the result vectors.
        r0 = _mm_add_ps(c0, r0);
        r1 = _mm_add_ps(c1, r1);
        r2 = _mm_add_ps(c2, r2);
        r3 = _mm_add_ps(c3, r3);

        iPtr += 16;
        tPtr += 8;
      }

      // Combine all results into one
      r0 = _mm_add_ps(r0, r1);
      r0 = _mm_add_ps(r0, r2);
      r0 = _mm_add_ps(r0, r3);

      // Get the values from r0 to a float array.
      _mm_store_ps(vectorResult, r0);

      // Sum up to result
      res[0] = vectorResult[0] + vectorResult[2]; // I
      res[1] = vectorResult[1] + vectorResult[3]; // Q

      // If the length not a multiple of 16, we need to do the remaining ones without SIMD.
      const unsigned int run2 = runs * 8;
      for (unsigned int i = run2; i < length; i++)
      {
        res[0] += ((*iPtr++) * (*tPtr));
        res[1] += ((*iPtr++) * (*tPtr++));
      }

      result->real(res[0]);
      result->imag(res[1]);
    }
#elif defined(MEMORY_OP_ARM)
    static inline void dotProduct(std::complex<float> *result, const std::complex<float> *input, const float *taps, unsigned int length)
    {
      const float *iPtr = (float *)input;
      const float *tPtr = (float *)taps;
      const unsigned int runs = length / 8;
      const float zeros[4] = {0, 0, 0, 0};
      float res[2];
      float accReal[4];
      float accImag[4];

      float32x4x2_t iVal0, iVal1;
      float32x4_t tVal0, tVal1, rAcc0, rAcc1, iAcc0, iAcc1;
      float32x4_t tmpR0, tmpR1, tmpI0, tmpI1;

      rAcc0 = vld1q_f32(zeros);
      rAcc1 = vld1q_f32(zeros);
      iAcc0 = vld1q_f32(zeros);
      iAcc1 = vld1q_f32(zeros);

      for (unsigned int i = 0; i < runs; i++)
      {
        // Load taps
        tVal0 = vld1q_f32(tPtr);
        tVal1 = vld1q_f32(tPtr + 4);

        // Load Complex
        iVal0 = vld2q_f32(iPtr);
        iVal1 = vld2q_f32(iPtr + 8);

        // Multiply first complex group by taps
        tmpR0 = vmulq_f32(tVal0, iVal0.val[0]);
        tmpI0 = vmulq_f32(tVal0, iVal0.val[1]);

        // Multiply second complex group by taps
        tmpR1 = vmulq_f32(tVal1, iVal1.val[0]);
        tmpI1 = vmulq_f32(tVal1, iVal1.val[1]);

        // Sum First complex group to accumulators
        rAcc0 = vaddq_f32(rAcc0, tmpR0);
        iAcc0 = vaddq_f32(iAcc0, tmpI0);

        // Sum Second complex group to accumulators
        rAcc1 = vaddq_f32(rAcc1, tmpR1);
        iAcc1 = vaddq_f32(iAcc1, tmpI1);

        tPtr += 8;
        iPtr += 16;
      }

      // Sum um two complex group results
      rAcc0 = vaddq_f32(rAcc0, rAcc1);
      iAcc0 = vaddq_f32(iAcc0, iAcc1);

      // Get back values to memory from register
      vst1q_f32(accReal, rAcc0);
      vst1q_f32(accImag, iAcc0);

      // Set result values
      res[0] = accReal[0] + accReal[1] + accReal[2] + accReal[3];
      res[1] = accImag[0] + accImag[1] + accImag[2] + accImag[3];

      // Continue if length % 8 != 0
      const unsigned int run2 = runs * 8;
      for (unsigned int i = run2; i < length; i++)
      {
        res[0] += ((*iPtr++) * (*tPtr));
        res[1] += ((*iPtr++) * (*tPtr++));
      }

      result->real(res[0]);
      result->imag(res[1]);
    }
#else
    static inline void dotProduct(std::complex<float> *result, const std::complex<float> *input, const float *taps, unsigned int length)
    {
      float res[2] = {0, 0};

      float *iPtr = (float *)input;
      float *tPtr = (float *)taps;

      for (unsigned int i = 0; i < length; i++)
      {
        res[0] += ((*iPtr++) * (*tPtr));
        res[1] += ((*iPtr++) * (*tPtr++));
      }

      result->real(res[0]);
      result->imag(res[1]);
    }
#endif
  };
} // namespace sathelper
