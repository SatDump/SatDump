/*
 * extensions.h
 *
 *  Created on: 16/09/2017
 *      Author: Lucas Teske
 */

#pragma once

#include <iostream>
#include "SIMD/MemoryOp.h"

namespace sathelper
{
  class Extensions
  {
  public:
    static bool hasFMA;
    static bool hasSSE;
    static bool hasSSE4;
    static bool hasAVX;
    static bool initialized;

#if defined(MEMORY_OP_X86) && defined(OP_X64)
    static inline float FMA(float a, float b, float c)
    {
      if (hasFMA)
      {
#ifdef _MSC_VER
        return fmaf(a, b, c); // Inline not supported for x64 in MSVC :(
#else
        __asm__ __volatile__("vfmadd231ss %[a], %[b], %[c]"
                             : [a] "+x"(a), [b] "+x"(b), [c] "+x"(c)
                             :);
        return c;
#endif
      }
      else
      {
        return a * b + c;
      }
    }
#else
    static inline float FMA(float a, float b, float c)
    {
      return a * b + c;
    }
#endif
  };

} // namespace sathelper