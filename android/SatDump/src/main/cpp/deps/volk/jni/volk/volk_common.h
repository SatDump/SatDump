#ifndef INCLUDED_LIBVOLK_COMMON_H
#define INCLUDED_LIBVOLK_COMMON_H

////////////////////////////////////////////////////////////////////////
// Cross-platform attribute macros
////////////////////////////////////////////////////////////////////////
#if _MSC_VER
#define __VOLK_ATTR_ALIGNED(x) __declspec(align(x))
#define __VOLK_ATTR_UNUSED
#define __VOLK_ATTR_INLINE __forceinline
#define __VOLK_ATTR_DEPRECATED __declspec(deprecated)
#define __VOLK_ATTR_EXPORT __declspec(dllexport)
#define __VOLK_ATTR_IMPORT __declspec(dllimport)
#define __VOLK_PREFETCH(addr)
#define __VOLK_ASM __asm
#define __VOLK_VOLATILE
#elif defined(__clang__)
// AppleClang also defines __GNUC__, so do this check first.  These
// will probably be the same as for __GNUC__, but let's keep them
// separate just to be safe.
#define __VOLK_ATTR_ALIGNED(x) __attribute__((aligned(x)))
#define __VOLK_ATTR_UNUSED __attribute__((unused))
#define __VOLK_ATTR_INLINE __attribute__((always_inline))
#define __VOLK_ATTR_DEPRECATED __attribute__((deprecated))
#define __VOLK_ASM __asm__
#define __VOLK_VOLATILE __volatile__
#define __VOLK_ATTR_EXPORT __attribute__((visibility("default")))
#define __VOLK_ATTR_IMPORT __attribute__((visibility("default")))
#define __VOLK_PREFETCH(addr) __builtin_prefetch(addr)
#elif defined __GNUC__
#define __VOLK_ATTR_ALIGNED(x) __attribute__((aligned(x)))
#define __VOLK_ATTR_UNUSED __attribute__((unused))
#define __VOLK_ATTR_INLINE __attribute__((always_inline))
#define __VOLK_ATTR_DEPRECATED __attribute__((deprecated))
#define __VOLK_ASM __asm__
#define __VOLK_VOLATILE __volatile__
#if __GNUC__ >= 4
#define __VOLK_ATTR_EXPORT __attribute__((visibility("default")))
#define __VOLK_ATTR_IMPORT __attribute__((visibility("default")))
#else
#define __VOLK_ATTR_EXPORT
#define __VOLK_ATTR_IMPORT
#endif
#define __VOLK_PREFETCH(addr) __builtin_prefetch(addr)
#elif _MSC_VER
#define __VOLK_ATTR_ALIGNED(x) __declspec(align(x))
#define __VOLK_ATTR_UNUSED
#define __VOLK_ATTR_INLINE __forceinline
#define __VOLK_ATTR_DEPRECATED __declspec(deprecated)
#define __VOLK_ATTR_EXPORT __declspec(dllexport)
#define __VOLK_ATTR_IMPORT __declspec(dllimport)
#define __VOLK_PREFETCH(addr)
#define __VOLK_ASM __asm
#define __VOLK_VOLATILE
#else
#define __VOLK_ATTR_ALIGNED(x)
#define __VOLK_ATTR_UNUSED
#define __VOLK_ATTR_INLINE
#define __VOLK_ATTR_DEPRECATED
#define __VOLK_ATTR_EXPORT
#define __VOLK_ATTR_IMPORT
#define __VOLK_PREFETCH(addr)
#define __VOLK_ASM __asm__
#define __VOLK_VOLATILE __volatile__
#endif

////////////////////////////////////////////////////////////////////////
// Ignore annoying warnings in MSVC
////////////////////////////////////////////////////////////////////////
#if defined(_MSC_VER)
#pragma warning(disable : 4244) //'conversion' conversion from 'type1' to 'type2',
                                // possible loss of data
#pragma warning(disable : 4305) //'identifier' : truncation from 'type1' to 'type2'
#endif

////////////////////////////////////////////////////////////////////////
// C-linkage declaration macros
// FIXME: due to the usage of complex.h, require gcc for c-linkage
////////////////////////////////////////////////////////////////////////
#if defined(__cplusplus) && (__GNUC__)
#define __VOLK_DECL_BEGIN extern "C" {
#define __VOLK_DECL_END }
#else
#define __VOLK_DECL_BEGIN
#define __VOLK_DECL_END
#endif

////////////////////////////////////////////////////////////////////////
// Define VOLK_API for library symbols
// http://gcc.gnu.org/wiki/Visibility
////////////////////////////////////////////////////////////////////////
#ifdef volk_EXPORTS
#define VOLK_API __VOLK_ATTR_EXPORT
#else
#define VOLK_API __VOLK_ATTR_IMPORT
#endif

////////////////////////////////////////////////////////////////////////
// The bit128 union used by some
////////////////////////////////////////////////////////////////////////
#include <stdint.h>

#ifdef LV_HAVE_SSE
#ifdef _WIN32
#include <intrin.h>
#else
#include <x86intrin.h>
#endif
#endif

union bit128 {
    uint8_t i8[16];
    uint16_t i16[8];
    uint32_t i[4];
    float f[4];
    double d[2];

#ifdef LV_HAVE_SSE
    __m128 float_vec;
#endif

#ifdef LV_HAVE_SSE2
    __m128i int_vec;
    __m128d double_vec;
#endif
};

union bit256 {
    uint8_t i8[32];
    uint16_t i16[16];
    uint32_t i[8];
    float f[8];
    double d[4];

#ifdef LV_HAVE_AVX
    __m256 float_vec;
    __m256i int_vec;
    __m256d double_vec;
#endif
};

#define bit128_p(x) ((union bit128*)(x))
#define bit256_p(x) ((union bit256*)(x))

////////////////////////////////////////////////////////////////////////
// log2f
////////////////////////////////////////////////////////////////////////
#include <math.h>
// +-Inf -> +-127.0f in order to match the behaviour of the SIMD kernels
static inline float log2f_non_ieee(float f)
{
    float const result = log2f(f);
    return isinf(result) ? copysignf(127.0f, result) : result;
}

////////////////////////////////////////////////////////////////////////
// Constant used to do log10 calculations as faster log2
////////////////////////////////////////////////////////////////////////
// precalculated 10.0 / log2f_non_ieee(10.0) to allow for constexpr
#define volk_log2to10factor 3.01029995663981209120

#endif /*INCLUDED_LIBVOLK_COMMON_H*/
