// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "constants.h"
#include "util.h"

#include <cstdint>

namespace charls {

// Optimized trait classes for lossless compression of 8 bit color and 8/16 bit monochrome images.
// This class assumes MaximumSampleValue correspond to a whole number of bits, and no custom ResetValue is set when encoding.
// The point of this is to have the most optimized code for the most common and most demanding scenario.
template<typename SampleType, int32_t BitsPerPixel>
struct lossless_traits_impl
{
    using sample_type = SampleType;

    static constexpr int32_t maximum_sample_value{(1U << BitsPerPixel) - 1};
    static constexpr int32_t near_lossless{};
    static constexpr int32_t quantized_bits_per_pixel{BitsPerPixel};
    static constexpr int32_t range{compute_range_parameter(maximum_sample_value, near_lossless)};
    static constexpr int32_t bits_per_pixel{BitsPerPixel};
    static constexpr int32_t limit{compute_limit_parameter(BitsPerPixel)};
    static constexpr int32_t reset_threshold{default_reset_value};

    FORCE_INLINE constexpr static int32_t compute_error_value(const int32_t d) noexcept
    {
        return modulo_range(d);
    }

    FORCE_INLINE constexpr static bool is_near(const int32_t lhs, const int32_t rhs) noexcept
    {
        return lhs == rhs;
    }

    FORCE_INLINE constexpr static int32_t
    modulo_range(const int32_t error_value) noexcept
    {
        return (static_cast<int32_t>(static_cast<uint32_t>(error_value) << (int32_t_bit_count - bits_per_pixel))) >>
               (int32_t_bit_count - bits_per_pixel);
    }

    FORCE_INLINE static SampleType compute_reconstructed_sample(const int32_t predicted_value,
                                                                const int32_t error_value) noexcept
    {
        return static_cast<SampleType>(maximum_sample_value & (predicted_value + error_value));
    }

    FORCE_INLINE static int32_t correct_prediction(const int32_t predicted) noexcept
    {
        if ((predicted & maximum_sample_value) == predicted)
            return predicted;

        return (~(predicted >> (int32_t_bit_count - 1))) & maximum_sample_value;
    }

#ifndef NDEBUG
    static bool is_valid() noexcept
    {
        return true;
    }
#endif
};


template<typename PixelType, int32_t BitsPerPixel>
struct lossless_traits final : lossless_traits_impl<PixelType, BitsPerPixel>
{
    using pixel_type = PixelType;
};


template<>
struct lossless_traits<uint8_t, 8> final : lossless_traits_impl<uint8_t, 8>
{
    using pixel_type = sample_type;

    FORCE_INLINE constexpr static signed char mod_range(const int32_t error_value) noexcept
    {
        return static_cast<signed char>(error_value);
    }

    FORCE_INLINE constexpr static int32_t compute_error_value(const int32_t d) noexcept
    {
        return static_cast<signed char>(d);
    }

    FORCE_INLINE constexpr static uint8_t compute_reconstructed_sample(const int32_t predicted_value,
                                                                       const int32_t error_value) noexcept
    {
        return static_cast<uint8_t>(predicted_value + error_value);
    }
};


template<>
struct lossless_traits<uint16_t, 16> final : lossless_traits_impl<uint16_t, 16>
{
    using pixel_type = sample_type;

    FORCE_INLINE constexpr static short mod_range(const int32_t error_value) noexcept
    {
        return static_cast<short>(error_value);
    }

    FORCE_INLINE constexpr static int32_t compute_error_value(const int32_t d) noexcept
    {
        return static_cast<short>(d);
    }

    FORCE_INLINE constexpr static sample_type compute_reconstructed_sample(const int32_t predicted_value,
                                                                           const int32_t error_value) noexcept
    {
        return static_cast<sample_type>(predicted_value + error_value);
    }
};


template<typename PixelType, int32_t BitsPerPixel>
struct lossless_traits<triplet<PixelType>, BitsPerPixel> final : lossless_traits_impl<PixelType, BitsPerPixel>
{
    using pixel_type = triplet<PixelType>;

    FORCE_INLINE constexpr static bool is_near(const int32_t lhs, const int32_t rhs) noexcept
    {
        return lhs == rhs;
    }

    FORCE_INLINE static bool is_near(pixel_type lhs, pixel_type rhs) noexcept
    {
        return lhs == rhs;
    }

    FORCE_INLINE static PixelType compute_reconstructed_sample(const int32_t predicted_value,
                                                               const int32_t error_value) noexcept
    {
        return static_cast<PixelType>(predicted_value + error_value);
    }
};


template<typename PixelType, int32_t BitsPerPixel>
struct lossless_traits<quad<PixelType>, BitsPerPixel> final : lossless_traits_impl<PixelType, BitsPerPixel>
{
    using pixel_type = quad<PixelType>;

    FORCE_INLINE constexpr static bool is_near(const int32_t lhs, const int32_t rhs) noexcept
    {
        return lhs == rhs;
    }

    FORCE_INLINE static bool is_near(pixel_type lhs, pixel_type rhs) noexcept
    {
        return lhs == rhs;
    }

    FORCE_INLINE static PixelType compute_reconstructed_sample(const int32_t predicted_value,
                                                               const int32_t error_value) noexcept
    {
        return static_cast<PixelType>(predicted_value + error_value);
    }
};

} // namespace charls
