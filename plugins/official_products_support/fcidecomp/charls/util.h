// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "annotations.h"
#include "jpegls_error.h"

#include "byte_span.h"
#include "constants.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <type_traits>


// Use an uppercase alias for assert to make it clear that ASSERT is a pre-processor macro.
#ifdef _MSC_VER
// C26493 = Don't use C-style casts
#define ASSERT(expression) \
    __pragma(warning(push)) __pragma(warning(disable : 26493)) assert(expression) __pragma(warning(pop))
#else
#define ASSERT(expression) assert(expression)
#endif

// Use forced inline for supported C++ compilers in release builds.
// Note: usage of FORCE_INLINE may be reduced in the future as the latest generation of C++ compilers
// can handle optimization by themselves.
#ifndef FORCE_INLINE
#ifdef NDEBUG
#ifdef _MSC_VER
#define FORCE_INLINE __forceinline
#elif defined(__GNUC__)
// C++ Compilers that support the GCC extensions (GCC, clang, Intel, etc.)
#define FORCE_INLINE __attribute__((always_inline))
#else
// Unknown C++ compiler, fallback to auto inline.
#define FORCE_INLINE
#endif
#else
// Do not force inline in debug builds.
#define FORCE_INLINE
#endif
#endif


#ifdef _MSC_VER
#define MSVC_WARNING_SUPPRESS(x) \
    __pragma(warning(push)) __pragma(warning(disable : x)) // NOLINT(misc-macro-parentheses, bugprone-macro-parentheses)
#define MSVC_WARNING_UNSUPPRESS() __pragma(warning(pop))

#define MSVC_WARNING_SUPPRESS_NEXT_LINE(x) \
    __pragma(warning(suppress \
                     : x)) // NOLINT(misc-macro-parentheses, bugprone-macro-parentheses, cppcoreguidelines-macro-usage)

// Helper macro for SAL annotations.
#define USE_DECL_ANNOTATIONS _Use_decl_annotations_

#else
#define MSVC_WARNING_SUPPRESS(x)
#define MSVC_WARNING_UNSUPPRESS()
#define MSVC_WARNING_SUPPRESS_NEXT_LINE(x)
#define USE_DECL_ANNOTATIONS
#endif

// C++20 has support for [[likely]] and [[unlikely]]. Use for now the GCC\Clang extension.
// MSVC has in C++14\C++17 mode no alternative for it.
#ifdef __GNUC__
#define UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define UNLIKELY(x) (x)
#endif

// C++20 provides std::endian, use for now compiler macros.
#ifdef _MSC_VER
#define LITTLE_ENDIAN_ARCHITECTURE // MSVC++ compiler support only little endian platforms.
#elif __GNUC__
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#define LITTLE_ENDIAN_ARCHITECTURE
#endif
#else
#error "Unknown compiler"
#endif

// Turn A into a string literal without expanding macro definitions
// (however, if invoked from a macro, macro arguments are expanded).
#define TO_STRING_NX(A) #A // NOLINT(cppcoreguidelines-macro-usage)

// Turn A into a string literal after macro-expanding it.
#define TO_STRING(A) TO_STRING_NX(A) // NOLINT(cppcoreguidelines-macro-usage)


namespace charls {

inline jpegls_errc to_jpegls_errc() noexcept
{
    try
    {
        // re-trow the exception.
        throw;
    }
    catch (const jpegls_error& error)
    {
        return static_cast<jpegls_errc>(error.code().value());
    }
    catch (const std::bad_alloc&)
    {
        return jpegls_errc::not_enough_memory;
    }
    catch (...)
    {
        return jpegls_errc::unexpected_failure;
    }
}

inline void clear_error_message(CHARLS_OUT_OPT char* error_message) noexcept
{
    if (error_message)
    {
        error_message[0] = 0;
    }
}


/// <summary>
/// Cross platform safe version of strcpy.
/// </summary>
inline void string_copy(CHARLS_IN_Z const char* source, CHARLS_OUT_WRITES_Z(size_in_bytes) char* destination,
                        const size_t size_in_bytes) noexcept
{
    ASSERT(strlen(source) < size_in_bytes && "String will be truncated");

#if defined(__STDC_SECURE_LIB__) && defined(__STDC_WANT_SECURE_LIB__) && __STDC_WANT_SECURE_LIB__ == 1
    constexpr size_t truncate{static_cast<size_t>(-1)};
    strncpy_s(destination, size_in_bytes, source, truncate);
#else
    strncpy(destination, source, size_in_bytes);
    destination[size_in_bytes - 1] = 0;
#endif
}

inline jpegls_errc set_error_message(const jpegls_errc error,
                                     CHARLS_OUT_WRITES_Z(ErrorMessageSize) char* error_message) noexcept
{
    if (error_message)
    {
        string_copy(charls_get_error_message(error), error_message, ErrorMessageSize);
    }

    return error;
}


/// <Remarks>
/// Bits per sample can be computed if range is passed as argument.
/// </Remarks>
CHARLS_CONSTEXPR int32_t log2_ceil(const int32_t n) noexcept
{
    ASSERT(n >= 0);
    ASSERT(static_cast<uint32_t>(n) <= std::numeric_limits<uint32_t>::max() >> 2); // otherwise 1 << x becomes negative.

    int32_t x{};
    while (n > (1 << x))
    {
        ++x;
    }
    return x;
}


constexpr int32_t sign(const int32_t n) noexcept
{
    return (n >> (int32_t_bit_count - 1)) | 1;
}


constexpr int32_t bit_wise_sign(const int32_t i) noexcept
{
    return i >> (int32_t_bit_count - 1);
}


/// <summary>
/// Computes the parameter RANGE. When NEAR = 0, RANGE = MAXVAL + 1. (see ISO/IEC 14495-1, A.2.1)
/// </summary>
constexpr int32_t compute_range_parameter(const int32_t maximum_sample_value, const int32_t near_lossless) noexcept
{
    return (maximum_sample_value + 2 * near_lossless) / (2 * near_lossless + 1) + 1;
}


/// <summary>
/// Computes the parameter LIMIT. (see ISO/IEC 14495-1, A.2.1)
/// </summary>
constexpr int32_t compute_limit_parameter(const int32_t bits_per_pixel)
{
    return 2 * (bits_per_pixel + std::max(8, bits_per_pixel));
}


template<typename SampleType>
struct triplet
{
    triplet() noexcept : v1{}, v2{}, v3{}
    {
    }

    triplet(int32_t x1, int32_t x2, int32_t x3) noexcept :
        v1(static_cast<SampleType>(x1)), v2(static_cast<SampleType>(x2)), v3(static_cast<SampleType>(x3))
    {
    }

    union
    {
        SampleType v1;
        SampleType R;
    };
    union
    {
        SampleType v2;
        SampleType G;
    };
    union
    {
        SampleType v3;
        SampleType B;
    };
};


inline bool operator==(const triplet<uint8_t>& lhs, const triplet<uint8_t>& rhs) noexcept
{
    return lhs.v1 == rhs.v1 && lhs.v2 == rhs.v2 && lhs.v3 == rhs.v3;
}


inline bool operator!=(const triplet<uint8_t>& lhs, const triplet<uint8_t>& rhs) noexcept
{
    return !(lhs == rhs);
}


template<typename SampleType>
struct quad final : triplet<SampleType>
{
    MSVC_WARNING_SUPPRESS(26495) // false warning that v4 is uninitialized [VS 2017 15.9.4]
    quad() noexcept : triplet<SampleType>(), v4{}
    {
    }
    MSVC_WARNING_UNSUPPRESS()

    MSVC_WARNING_SUPPRESS(26495) // false warning that v4 is uninitialized [VS 2017 15.9.4]
    quad(triplet<SampleType> triplet_value, int32_t alpha) noexcept :
        triplet<SampleType>(triplet_value), A(static_cast<SampleType>(alpha))
    {
    }
    MSVC_WARNING_UNSUPPRESS()

    union
    {
        SampleType v4;
        SampleType A;
    };
};


template<typename Callback>
struct callback_function final
{
    Callback handler;
    void* user_context;
};


// C++23 comes with std::byteswap. Use our own byte_swap implementation for now.

// A simple overload with uint64_t\uint32_t doesn't work for macOS. size_t is not the same type as uint64_t.
template<int Bits, typename T>
constexpr bool is_uint_v = sizeof(T) == (Bits / 8) && std::is_integral<T>::value && !std::is_signed<T>::value;

template<typename T>
CHARLS_CHECK_RETURN auto byte_swap(const T value) noexcept -> std::enable_if_t<is_uint_v<16, T>, uint16_t>
{
#ifdef _MSC_VER
    return _byteswap_ushort(value);
#else
    // Note: GCC and Clang will optimize this pattern to a built-in intrinsic.
    return static_cast<uint16_t>(value << 8 | value >> 8);
#endif
}

template<typename T>
CHARLS_CHECK_RETURN auto byte_swap(const T value) noexcept -> std::enable_if_t<is_uint_v<32, T>, uint32_t>
{
#ifdef _MSC_VER
    return _byteswap_ulong(value);
#else
    // Note: GCC and Clang will optimize this pattern to a built-in intrinsic.
    return value >> 24 | (value & 0x00FF0000) >> 8 | (value & 0x0000FF00) << 8 | value << 24;
#endif
}

template<typename T>
CHARLS_CHECK_RETURN auto byte_swap(const T value) noexcept -> std::enable_if_t<is_uint_v<64, T>, uint64_t>
{
#ifdef _MSC_VER
    return _byteswap_uint64(value);
#else
    // Note: GCC and Clang will optimize this pattern to a built-in intrinsic.
    return (value << 56) | ((value << 40) & 0x00FF'0000'0000'0000) | ((value << 24) & 0x0000'FF00'0000'0000) |
           ((value << 8) & 0x0000'00FF'0000'0000) | ((value >> 8) & 0x0000'0000'FF00'0000) |
           ((value >> 24) & 0x0000'0000'00FF'0000) | ((value >> 40) & 0x0000'0000'0000'FF00) | (value >> 56);
#endif
}


template<typename T>
T read_unaligned(const void* buffer) noexcept
{
    // Note: MSVC, GCC and clang will replace this with a direct register read if the CPU architecture allows it
    // On x86, x64 and ARM64 this will just be 1 register load.
    T value;
    memcpy(&value, buffer, sizeof(T));
    return value;
}

#ifdef __EMSCRIPTEN__

// Note: WebAssembly (emcc 3.1.1) will fail with the default read_unaligned.

template<typename T>
T read_big_endian_unaligned(const void* /*buffer*/) noexcept;

template<>
inline uint16_t read_big_endian_unaligned<uint16_t>(const void* buffer) noexcept
{
    const uint8_t* p{static_cast<const uint8_t*>(buffer)};
    return (static_cast<uint32_t>(p[0]) << 8U) + static_cast<uint32_t>(p[1]);
}

template<>
inline uint32_t read_big_endian_unaligned<uint32_t>(const void* buffer) noexcept
{
    const uint8_t* p{static_cast<const uint8_t*>(buffer)};
    return (static_cast<uint32_t>(p[0]) << 24U) + (static_cast<uint32_t>(p[1]) << 16U) +
           (static_cast<uint32_t>(p[2]) << 8U) + static_cast<uint32_t>(p[3]);
}

template<>
inline size_t read_big_endian_unaligned<size_t>(const void* buffer) noexcept
{
    static_assert(sizeof(size_t) == sizeof(uint32_t), "wasm32 only");
    return read_big_endian_unaligned<uint32_t>(buffer);
}

#else

template<typename T>
T read_big_endian_unaligned(const void* buffer) noexcept
{
#ifdef LITTLE_ENDIAN_ARCHITECTURE
    return byte_swap(read_unaligned<T>(buffer));
#else
    return read_unaligned<T>(buffer);
#endif
}

#endif


inline void skip_bytes(byte_span& stream_info, const size_t count) noexcept
{
    ASSERT(count <= stream_info.size);

    stream_info.data += count;
    stream_info.size -= count;
}


template<typename T>
T* check_pointer(T* pointer)
{
    if (UNLIKELY(!pointer))
        impl::throw_jpegls_error(jpegls_errc::invalid_argument);

    return pointer;
}


/// <summary>
/// Validates the boolean 'expression'.
/// </summary>
/// <exception cref="charls::jpegls_error">Throws jpegls_errc::invalid_operation if 'expression' is false.</exception>
inline void check_operation(const bool expression)
{
    if (UNLIKELY(!expression))
        impl::throw_jpegls_error(jpegls_errc::invalid_operation);
}


/// <summary>
/// Validates the boolean 'expression'.
/// </summary>
/// <exception cref="charls::jpegls_error">Throws jpegls_errc if 'expression' is false.</exception>
inline void check_argument(const bool expression, const jpegls_errc error_value = jpegls_errc::invalid_argument)
{
    if (UNLIKELY(!expression))
        impl::throw_jpegls_error(error_value);
}


inline void check_interleave_mode(const charls::interleave_mode mode, const jpegls_errc error_value)
{
    if (UNLIKELY(!(mode == interleave_mode::none || mode == interleave_mode::line || mode == interleave_mode::sample)))
        impl::throw_jpegls_error(error_value);
}


CHARLS_CONSTEXPR int32_t calculate_maximum_sample_value(const int32_t bits_per_sample)
{
    ASSERT(bits_per_sample > 0 && bits_per_sample <= 16);
    return static_cast<int32_t>((1U << bits_per_sample) - 1);
}


/// <summary>
/// Computes how many bytes are needed to hold the number of bits.
/// </summary>
constexpr uint32_t bit_to_byte_count(const int32_t bit_count) noexcept
{
    return static_cast<uint32_t>((bit_count + 7) / 8);
}


/// <summary>
/// Converts an enumeration to its underlying type. Equivalent to C++23 std::to_underlying
/// </summary>
template<typename Enum>
constexpr auto to_underlying_type(Enum e) noexcept
{
    return static_cast<std::underlying_type_t<Enum>>(e);
}

#ifdef _MSC_VER
#if defined(_M_X64) || defined(_M_ARM64)
/// <summary>
/// Custom implementation of C++20 std::countl_zero (for uint64_t)
/// </summary>
inline int countl_zero(const uint64_t value) noexcept
{
    if (value == 0)
        return 64;

    unsigned long index;
    _BitScanReverse64(&index, value);
    return static_cast<int>(63U - index);
}
#endif

/// <summary>
/// Custom implementation of C++20 std::countl_zero (for uint32_t)
/// </summary>
inline int countl_zero(const uint32_t value) noexcept
{
    if (value == 0)
        return 32;

    unsigned long index;
    _BitScanReverse(&index, value);

    return static_cast<int>(31U - index);
}
#endif

#ifdef __GNUC__

// A simple overload with uint64_t\uint32_t doesn't work for macOS. size_t is not the same type as uint64_t.

/// <summary>
/// Custom implementation of C++20 std::countl_zero (for uint64_t)
/// </summary>
template<typename T>
auto countl_zero(const T value) noexcept -> std::enable_if_t<is_uint_v<64, T>, int>
{
    if (value == 0)
        return 64;

    return __builtin_clzll(value);
}

/// <summary>
/// Custom implementation of C++20 std::countl_zero (for uint32_t)
/// </summary>
template<typename T>
auto countl_zero(const T value) noexcept -> std::enable_if_t<is_uint_v<32, T>, int>
{
    if (value == 0)
        return 32;

    return __builtin_clz(value);
}

#endif


#if INTPTR_MAX == INT64_MAX
constexpr size_t checked_mul(const size_t a, const size_t b) noexcept
{
    return a * b;
}
#elif INTPTR_MAX == INT32_MAX
inline size_t checked_mul(const size_t a, const size_t b)
{
    const size_t result{a * b};
    if (UNLIKELY(result < a || result < b)) // check for unsigned integer overflow.
        impl::throw_jpegls_error(jpegls_errc::parameter_value_not_supported);
    return result;
}
#endif


} // namespace charls
