// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "constants.h"
#include "util.h"

#include <cassert>
#include <cmath>
#include <cstdlib>

// Default traits that support all JPEG LS parameters: custom limit, near, maxval (not power of 2)

// This traits class is used to initialize a coder/decoder.
// The coder/decoder also delegates some functions to the traits class.
// This is to allow the traits class to replace the default implementation here with optimized specific implementations.
// This is done for lossless coding/decoding: see lossless_traits.h

namespace charls {

template<typename SampleType, typename PixelType>
struct default_traits final
{
    using sample_type = SampleType;
    using pixel_type = PixelType;

    int32_t maximum_sample_value;
    const int32_t near_lossless;
    const int32_t range;
    const int32_t quantized_bits_per_pixel;
    const int32_t bits_per_pixel;
    const int32_t limit;
    const int32_t reset_threshold;

    default_traits(const int32_t arg_maximum_sample_value, const int32_t arg_near_lossless,
                   const int32_t reset = default_reset_value) noexcept :
        maximum_sample_value{arg_maximum_sample_value},
        near_lossless{arg_near_lossless},
        range{compute_range_parameter(maximum_sample_value, near_lossless)},
        quantized_bits_per_pixel{log2_ceil(range)},
        bits_per_pixel{log2_ceil(maximum_sample_value)},
        limit{compute_limit_parameter(bits_per_pixel)},
        reset_threshold{reset}
    {
    }

    default_traits() = delete;
    default_traits(const default_traits&) noexcept = default;
    default_traits(default_traits&&) noexcept = default;
    ~default_traits() = default;
    default_traits& operator=(const default_traits&) = delete;
    default_traits& operator=(default_traits&&) = delete;

    FORCE_INLINE int32_t compute_error_value(const int32_t e) const noexcept
    {
        return modulo_range(quantize(e));
    }

    FORCE_INLINE SampleType compute_reconstructed_sample(const int32_t predicted_value,
                                                         const int32_t error_value) const noexcept
    {
        return fix_reconstructed_value(predicted_value + dequantize(error_value));
    }

    FORCE_INLINE bool is_near(const int32_t lhs, const int32_t rhs) const noexcept
    {
        return std::abs(lhs - rhs) <= near_lossless;
    }

    bool is_near(const triplet<SampleType> lhs, const triplet<SampleType> rhs) const noexcept
    {
        return std::abs(lhs.v1 - rhs.v1) <= near_lossless && std::abs(lhs.v2 - rhs.v2) <= near_lossless &&
               std::abs(lhs.v3 - rhs.v3) <= near_lossless;
    }

    bool is_near(const quad<SampleType> lhs, const quad<SampleType> rhs) const noexcept
    {
        return std::abs(lhs.v1 - rhs.v1) <= near_lossless && std::abs(lhs.v2 - rhs.v2) <= near_lossless &&
               std::abs(lhs.v3 - rhs.v3) <= near_lossless && std::abs(lhs.v4 - rhs.v4) <= near_lossless;
    }

    FORCE_INLINE int32_t correct_prediction(const int32_t predicted) const noexcept
    {
        if ((predicted & maximum_sample_value) == predicted)
            return predicted;

        return (~(predicted >> (int32_t_bit_count - 1))) & maximum_sample_value;
    }

    /// <summary>
    /// Returns the value of errorValue modulo RANGE. ITU.T.87, A.4.5 (code segment A.9)
    /// This ensures the error is reduced to the range (-⌊RANGE/2⌋ .. ⌈RANGE/2⌉-1)
    /// </summary>
    FORCE_INLINE int32_t modulo_range(int32_t error_value) const noexcept
    {
        ASSERT(std::abs(error_value) <= range);

        if (error_value < 0)
        {
            error_value += range;
        }

        if (error_value >= (range + 1) / 2)
        {
            error_value -= range;
        }

        ASSERT(-range / 2 <= error_value && error_value <= ((range + 1) / 2) - 1);
        return error_value;
    }

#ifndef NDEBUG
    bool is_valid() const noexcept
    {
        if (maximum_sample_value < 1 || maximum_sample_value > std::numeric_limits<uint16_t>::max())
            return false;

        if (bits_per_pixel < 1 || bits_per_pixel > 16)
            return false;

        return true;
    }
#endif

private:
    int32_t quantize(const int32_t error_value) const noexcept
    {
        if (error_value > 0)
            return (error_value + near_lossless) / (2 * near_lossless + 1);

        return -(near_lossless - error_value) / (2 * near_lossless + 1);
    }

    FORCE_INLINE int32_t dequantize(const int32_t error_value) const noexcept
    {
        return error_value * (2 * near_lossless + 1);
    }

    FORCE_INLINE SampleType fix_reconstructed_value(int32_t value) const noexcept
    {
        if (value < -near_lossless)
        {
            value = value + range * (2 * near_lossless + 1);
        }
        else if (value > maximum_sample_value + near_lossless)
        {
            value = value - range * (2 * near_lossless + 1);
        }

        return static_cast<SampleType>(correct_prediction(value));
    }
};

} // namespace charls
