// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "coding_parameters.h"
#include "util.h"

#include <algorithm>
#include <cstring>
#include <sstream>
#include <vector>


//
// This file defines the ProcessLine base class, its derivatives and helper functions.
// During coding/decoding, CharLS process one line at a time. The different ProcessLine implementations
// convert the uncompressed format to and from the internal format for encoding.
// Conversions include color transforms, line interleaved vs sample interleaved, masking out unused bits,
// accounting for line padding etc.
// This mechanism could be used to encode/decode images as they are received.
//

namespace charls {

struct process_line
{
    virtual ~process_line() = default;

    virtual void new_line_decoded(const void* source, size_t pixel_count, size_t source_stride) = 0;
    virtual void new_line_requested(void* destination, size_t pixel_count, size_t destination_stride) = 0;

protected:
    process_line() = default;
    process_line(const process_line&) = default;
    process_line(process_line&&) = default;
    process_line& operator=(const process_line&) = default;
    process_line& operator=(process_line&&) = default;
};


class post_process_single_component final : public process_line
{
public:
    post_process_single_component(void* raw_data, const size_t stride, const size_t bytes_per_pixel) noexcept :
        raw_data_{static_cast<uint8_t*>(raw_data)}, bytes_per_pixel_{bytes_per_pixel}, stride_{stride}
    {
        ASSERT(bytes_per_pixel == sizeof(uint8_t) || bytes_per_pixel == sizeof(uint16_t));
    }

    void new_line_requested(void* destination, const size_t pixel_count,
                            size_t /* destination_stride */) noexcept(false) override
    {
        memcpy(destination, raw_data_, pixel_count * bytes_per_pixel_);
        raw_data_ += stride_;
    }

    void new_line_decoded(const void* source, const size_t pixel_count, size_t /* source_stride */) noexcept(false) override
    {
        memcpy(raw_data_, source, pixel_count * bytes_per_pixel_);
        raw_data_ += stride_;
    }

private:
    uint8_t* raw_data_;
    size_t bytes_per_pixel_;
    size_t stride_;
};


class post_process_single_component_masked final : public process_line
{
public:
    post_process_single_component_masked(void* raw_data, const size_t stride, const size_t bytes_per_pixel,
                                         const uint32_t bits_per_pixel) noexcept :
        raw_data_{raw_data},
        bytes_per_pixel_{bytes_per_pixel},
        stride_{stride},
        mask_{(1U << bits_per_pixel) - 1U},
        single_byte_pixel_{bytes_per_pixel_ == sizeof(uint8_t)}
    {
        ASSERT(bytes_per_pixel == sizeof(uint8_t) || bytes_per_pixel == sizeof(uint16_t));
    }

    void new_line_requested(void* destination, const size_t pixel_count,
                            size_t /* destination_stride */) noexcept(false) override
    {
        if (single_byte_pixel_)
        {
            const auto* pixel_source{static_cast<uint8_t*>(raw_data_)};
            auto* pixel_destination{static_cast<uint8_t*>(destination)};
            for (size_t i{}; i < pixel_count; ++i)
            {
                pixel_destination[i] = static_cast<uint8_t>(pixel_source[i] & mask_);
            }
        }
        else
        {
            const auto* pixel_source{static_cast<uint16_t*>(raw_data_)};
            auto* pixel_destination{static_cast<uint16_t*>(destination)};
            for (size_t i{}; i < pixel_count; ++i)
            {
                pixel_destination[i] = static_cast<uint16_t>(pixel_source[i] & mask_);
            }
        }

        raw_data_ = static_cast<uint8_t*>(raw_data_) + stride_;
    }

    void new_line_decoded(const void* source, const size_t pixel_count, size_t /* source_stride */) noexcept(false) override
    {
        memcpy(raw_data_, source, pixel_count * bytes_per_pixel_);
        raw_data_ = static_cast<uint8_t*>(raw_data_) + stride_;
    }

private:
    void* raw_data_;
    size_t bytes_per_pixel_;
    size_t stride_;
    uint32_t mask_;
    bool single_byte_pixel_;
};


template<typename Transform, typename PixelType>
void transform_line_to_quad(const PixelType* source, const size_t pixel_stride_in, quad<PixelType>* destination,
                            const size_t pixel_stride, Transform& transform) noexcept
{
    const auto pixel_count{std::min(pixel_stride, pixel_stride_in)};

    for (size_t i{}; i < pixel_count; ++i)
    {
        const quad<PixelType> pixel(transform(source[i], source[i + pixel_stride_in], source[i + 2 * pixel_stride_in]),
                                    source[i + 3 * pixel_stride_in]);
        destination[i] = pixel;
    }
}


template<typename Transform, typename PixelType>
void transform_quad_to_line(const quad<PixelType>* source, const size_t pixel_stride_in, PixelType* destination,
                            const size_t pixel_stride, Transform& transform) noexcept
{
    const auto pixel_count{std::min(pixel_stride, pixel_stride_in)};

    for (size_t i{}; i < pixel_count; ++i)
    {
        const quad<PixelType> color{source[i]};
        const quad<PixelType> color_transformed(transform(color.v1, color.v2, color.v3), color.v4);

        destination[i] = color_transformed.v1;
        destination[i + pixel_stride] = color_transformed.v2;
        destination[i + 2 * pixel_stride] = color_transformed.v3;
        destination[i + 3 * pixel_stride] = color_transformed.v4;
    }
}


template<typename Transform, typename PixelType>
void transform_quad_to_line(const quad<PixelType>* source, const size_t pixel_stride_in, PixelType* destination,
                            const size_t pixel_stride, Transform& transform, const uint32_t mask) noexcept
{
    const auto pixel_count{std::min(pixel_stride, pixel_stride_in)};

    for (size_t i{}; i < pixel_count; ++i)
    {
        const quad<PixelType> color{source[i]};
        const quad<PixelType> color_transformed(transform(color.v1 & mask, color.v2 & mask, color.v3 & mask),
                                                color.v4 & mask);

        destination[i] = color_transformed.v1;
        destination[i + pixel_stride] = color_transformed.v2;
        destination[i + 2 * pixel_stride] = color_transformed.v3;
        destination[i + 3 * pixel_stride] = color_transformed.v4;
    }
}


template<typename T>
void transform_rgb_to_bgr(T* buffer, size_t samples_per_pixel, const size_t pixel_count) noexcept
{
    for (size_t i{}; i < pixel_count; ++i)
    {
        std::swap(buffer[0], buffer[2]);
        buffer += samples_per_pixel;
    }
}


template<typename Transform, typename PixelType>
void transform_line(triplet<PixelType>* destination, const triplet<PixelType>* source, const size_t pixel_count,
                    Transform& transform) noexcept
{
    for (size_t i{}; i < pixel_count; ++i)
    {
        destination[i] = transform(source[i].v1, source[i].v2, source[i].v3);
    }
}


template<typename Transform, typename PixelType>
void transform_line(triplet<PixelType>* destination, const triplet<PixelType>* source, const size_t pixel_count,
                    Transform& transform, const uint32_t mask) noexcept
{
    for (size_t i{}; i < pixel_count; ++i)
    {
        destination[i] = transform(source[i].v1 & mask, source[i].v2 & mask, source[i].v3 & mask);
    }
}


template<typename Transform, typename PixelType>
void transform_line(quad<PixelType>* destination, const quad<PixelType>* source, const size_t pixel_count,
                    Transform& transform) noexcept
{
    for (size_t i{}; i < pixel_count; ++i)
    {
        destination[i] = quad<PixelType>(transform(source[i].v1, source[i].v2, source[i].v3), source[i].v4);
    }
}


template<typename Transform, typename PixelType>
void transform_line(quad<PixelType>* destination, const quad<PixelType>* source, const size_t pixel_count,
                    Transform& transform, const uint32_t mask) noexcept
{
    for (size_t i{}; i < pixel_count; ++i)
    {
        destination[i] =
            quad<PixelType>(transform(source[i].v1 & mask, source[i].v2 & mask, source[i].v3 & mask), source[i].v4 & mask);
    }
}


template<typename Transform, typename PixelType>
void transform_line_to_triplet(const PixelType* source, const size_t pixel_stride_in, triplet<PixelType>* destination,
                               const size_t pixel_stride, Transform& transform) noexcept
{
    const auto pixel_count{std::min(pixel_stride, pixel_stride_in)};
    triplet<PixelType>* type_buffer = destination;

    for (size_t i{}; i < pixel_count; ++i)
    {
        type_buffer[i] = transform(source[i], source[i + pixel_stride_in], source[i + 2 * pixel_stride_in]);
    }
}


template<typename Transform, typename PixelType>
void transform_triplet_to_line(const triplet<PixelType>* source, const size_t pixel_stride_in, PixelType* destination,
                               const size_t pixel_stride, Transform& transform) noexcept
{
    const auto pixel_count{std::min(pixel_stride, pixel_stride_in)};
    const triplet<PixelType>* type_buffer_in{source};

    for (size_t i{}; i < pixel_count; ++i)
    {
        const triplet<PixelType> color{type_buffer_in[i]};
        const triplet<PixelType> color_transformed{transform(color.v1, color.v2, color.v3)};

        destination[i] = color_transformed.v1;
        destination[i + pixel_stride] = color_transformed.v2;
        destination[i + 2 * pixel_stride] = color_transformed.v3;
    }
}


template<typename Transform, typename PixelType>
void transform_triplet_to_line(const triplet<PixelType>* source, const size_t pixel_stride_in, PixelType* destination,
                               const size_t pixel_stride, Transform& transform, const uint32_t mask) noexcept
{
    const auto pixel_count{std::min(pixel_stride, pixel_stride_in)};
    const triplet<PixelType>* type_buffer_in{source};

    for (size_t i{}; i < pixel_count; ++i)
    {
        const triplet<PixelType> color{type_buffer_in[i]};
        const triplet<PixelType> color_transformed{transform(color.v1 & mask, color.v2 & mask, color.v3 & mask)};

        destination[i] = color_transformed.v1;
        destination[i + pixel_stride] = color_transformed.v2;
        destination[i + 2 * pixel_stride] = color_transformed.v3;
    }
}


template<typename TransformType>
class process_transformed final : public process_line
{
public:
    process_transformed(byte_span source_pixels, const size_t stride, const frame_info& info,
                        const coding_parameters& parameters, TransformType transform) :
        frame_info_{info},
        parameters_{parameters},
        stride_{stride},
        temp_line_(static_cast<size_t>(info.component_count) * info.width),
        buffer_(static_cast<size_t>(info.component_count) * info.width * sizeof(size_type)),
        transform_{transform},
        inverse_transform_{transform},
        raw_pixels_{source_pixels},
        mask_{(1U << info.bits_per_sample) - 1U}
    {
    }

    void new_line_requested(void* destination, const size_t pixel_count,
                            const size_t destination_stride) noexcept(false) override
    {
        encode_transform(raw_pixels_.data, destination, pixel_count, destination_stride);
        raw_pixels_.data += stride_;
    }

    void encode_transform(const void* source, void* destination, const size_t pixel_count,
                          const size_t destination_stride) noexcept
    {
        if (parameters_.output_bgr)
        {
            memcpy(temp_line_.data(), source, sizeof(triplet<size_type>) * pixel_count);
            transform_rgb_to_bgr(temp_line_.data(), frame_info_.component_count, pixel_count);
            source = temp_line_.data();
        }

        if (frame_info_.component_count == 3)
        {
            if (parameters_.interleave_mode == interleave_mode::sample)
            {
                transform_line(static_cast<triplet<size_type>*>(destination), static_cast<const triplet<size_type>*>(source),
                               pixel_count, transform_, mask_);
            }
            else
            {
                transform_triplet_to_line(static_cast<const triplet<size_type>*>(source), pixel_count,
                                          static_cast<size_type*>(destination), destination_stride, transform_, mask_);
            }
        }
        else if (frame_info_.component_count == 4)
        {
            if (parameters_.interleave_mode == interleave_mode::sample)
            {
                transform_line(static_cast<quad<size_type>*>(destination), static_cast<const quad<size_type>*>(source),
                               pixel_count, transform_, mask_);
            }
            else if (parameters_.interleave_mode == interleave_mode::line)
            {
                transform_quad_to_line(static_cast<const quad<size_type>*>(source), pixel_count,
                                       static_cast<size_type*>(destination), destination_stride, transform_, mask_);
            }
        }
    }

    void decode_transform(const void* source, void* destination, const size_t pixel_count, const size_t byte_stride) noexcept
    {
        if (frame_info_.component_count == 3)
        {
            if (parameters_.interleave_mode == interleave_mode::sample)
            {
                transform_line(static_cast<triplet<size_type>*>(destination), static_cast<const triplet<size_type>*>(source),
                               pixel_count, inverse_transform_);
            }
            else
            {
                transform_line_to_triplet(static_cast<const size_type*>(source), byte_stride,
                                          static_cast<triplet<size_type>*>(destination), pixel_count, inverse_transform_);
            }
        }
        else if (frame_info_.component_count == 4)
        {
            if (parameters_.interleave_mode == interleave_mode::sample)
            {
                transform_line(static_cast<quad<size_type>*>(destination), static_cast<const quad<size_type>*>(source),
                               pixel_count, inverse_transform_);
            }
            else if (parameters_.interleave_mode == interleave_mode::line)
            {
                transform_line_to_quad(static_cast<const size_type*>(source), byte_stride,
                                       static_cast<quad<size_type>*>(destination), pixel_count, inverse_transform_);
            }
        }

        if (parameters_.output_bgr)
        {
            transform_rgb_to_bgr(static_cast<size_type*>(destination), frame_info_.component_count, pixel_count);
        }
    }

    void new_line_decoded(const void* source, const size_t pixel_count, const size_t source_stride) noexcept(false) override
    {
        decode_transform(source, raw_pixels_.data, pixel_count, source_stride);
        raw_pixels_.data += stride_;
    }

private:
    using size_type = typename TransformType::size_type;

    const frame_info& frame_info_;
    const coding_parameters& parameters_;
    const size_t stride_;
    std::vector<size_type> temp_line_;
    std::vector<uint8_t> buffer_;
    TransformType transform_;
    typename TransformType::inverse inverse_transform_;
    byte_span raw_pixels_;
    uint32_t mask_;
};

} // namespace charls
