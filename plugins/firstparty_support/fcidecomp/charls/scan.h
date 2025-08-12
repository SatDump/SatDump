// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "coding_parameters.h"
#include "color_transform.h"
#include "context_regular_mode.h"
#include "context_run_mode.h"
#include "jpeg_marker_code.h"
#include "lookup_table.h"
#include "process_line.h"

#include <array>
#include <sstream>
#include <limits>

// This file contains the code for handling a "scan". Usually an image is encoded as a single scan.
// Note: the functions in this header could be moved into jpegls.cpp as they are only used in that file.

namespace charls {

class decoder_strategy;
class encoder_strategy;

extern const std::array<golomb_code_table, max_k_value> decoding_tables;
extern const std::vector<int8_t> quantization_lut_lossless_8;
extern const std::vector<int8_t> quantization_lut_lossless_10;
extern const std::vector<int8_t> quantization_lut_lossless_12;
extern const std::vector<int8_t> quantization_lut_lossless_16;

// Used to determine how large runs should be encoded at a time. Defined by the JPEG-LS standard, A.2.1., Initialization
// step 3.
constexpr std::array<int, 32> J{
    {0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 9, 10, 11, 12, 13, 14, 15}};

constexpr int32_t apply_sign(const int32_t i, const int32_t sign) noexcept
{
    return (sign ^ i) - sign;
}


inline int32_t get_predicted_value(const int32_t ra, const int32_t rb, const int32_t rc) noexcept
{
    // sign trick reduces the number of if statements (branches)
    const int32_t sign{bit_wise_sign(rb - ra)};

    // is Ra between Rc and Rb?
    if ((sign ^ (rc - ra)) < 0)
    {
        return rb;
    }
    if ((sign ^ (rb - rc)) < 0)
    {
        return ra;
    }

    // default case, valid if Rc element of [Ra,Rb]
    return ra + rb - rc;
}


/// <summary>
/// This is the optimized inverse algorithm of ISO/IEC 14495-1, A.5.2, Code Segment A.11 (second else branch)
/// It will map unsigned values back to signed values.
/// </summary>
CHARLS_CONSTEXPR int32_t unmap_error_value(const int32_t mapped_error) noexcept
{
    const int32_t sign{static_cast<int32_t>(static_cast<uint32_t>(mapped_error) << (int32_t_bit_count - 1)) >>
                       (int32_t_bit_count - 1)};
    return sign ^ (mapped_error >> 1);
}


/// <summary>
/// This is the algorithm of ISO/IEC 14495-1, A.5.2, Code Segment A.11 (second else branch)
/// It will map signed values to unsigned values. It has been optimized to prevent branching.
/// </summary>
CHARLS_CONSTEXPR int32_t map_error_value(const int32_t error_value) noexcept
{
    ASSERT(error_value <= std::numeric_limits<int32_t>::max() / 2);

    const int32_t mapped_error{(error_value >> (int32_t_bit_count - 2)) ^ (2 * error_value)};
    return mapped_error;
}


constexpr int32_t compute_context_id(const int32_t q1, const int32_t q2, const int32_t q3) noexcept
{
    return (q1 * 9 + q2) * 9 + q3;
}


template<typename Traits, typename Strategy>
class jls_codec final : public Strategy
{
public:
    using pixel_type = typename Traits::pixel_type;
    using sample_type = typename Traits::sample_type;

    jls_codec(Traits traits, const frame_info& frame_info, const coding_parameters& parameters) noexcept :
        Strategy{update_component_count(frame_info, parameters), parameters},
        traits_{std::move(traits)},
        width_{frame_info.width}
    {
        ASSERT((parameters.interleave_mode == interleave_mode::none && this->frame_info().component_count == 1) ||
               parameters.interleave_mode != interleave_mode::none);
        ASSERT(traits_.is_valid());
    }

    // Factory function for ProcessLine objects to copy/transform un encoded pixels to/from our scan line buffers.
    std::unique_ptr<process_line> create_process_line(byte_span info, const size_t stride) override
    {
        if (!is_interleaved())
        {
            if (frame_info().bits_per_sample == sizeof(sample_type) * 8)
            {
                return std::make_unique<post_process_single_component>(info.data, stride,
                                                                       sizeof(typename Traits::pixel_type));
            }

            return std::make_unique<post_process_single_component_masked>(
                info.data, stride, sizeof(typename Traits::pixel_type), frame_info().bits_per_sample);
        }

        if (parameters().transformation == color_transformation::none)
            return std::make_unique<process_transformed<transform_none<typename Traits::sample_type>>>(
                info, stride, frame_info(), parameters(), transform_none<sample_type>());

        if (frame_info().bits_per_sample == sizeof(sample_type) * 8)
        {
            switch (parameters().transformation)
            {
            case color_transformation::hp1:
                return std::make_unique<process_transformed<transform_hp1<sample_type>>>(
                    info, stride, frame_info(), parameters(), transform_hp1<sample_type>());
            case color_transformation::hp2:
                return std::make_unique<process_transformed<transform_hp2<sample_type>>>(
                    info, stride, frame_info(), parameters(), transform_hp2<sample_type>());
            case color_transformation::hp3:
                return std::make_unique<process_transformed<transform_hp3<sample_type>>>(
                    info, stride, frame_info(), parameters(), transform_hp3<sample_type>());
            default:
                impl::throw_jpegls_error(jpegls_errc::color_transform_not_supported);
            }
        }

        impl::throw_jpegls_error(jpegls_errc::bit_depth_for_transform_not_supported);
    }

private:
    void set_presets(const jpegls_pc_parameters& presets, const uint32_t restart_interval) override
    {
        initialize_parameters(presets.threshold1, presets.threshold2, presets.threshold3, presets.reset_value);
        restart_interval_ = restart_interval;
    }

    bool is_interleaved() noexcept
    {
        ASSERT((parameters().interleave_mode == interleave_mode::none && frame_info().component_count == 1) ||
               parameters().interleave_mode != interleave_mode::none);

        return parameters().interleave_mode != interleave_mode::none;
    }

    const coding_parameters& parameters() const noexcept
    {
        return Strategy::parameters_;
    }

    const charls::frame_info& frame_info() const noexcept
    {
        return Strategy::frame_info_;
    }

    int8_t quantize_gradient_org(const int32_t di) const noexcept
    {
        if (di <= -t3_)
            return -4;
        if (di <= -t2_)
            return -3;
        if (di <= -t1_)
            return -2;
        if (di < -traits_.near_lossless)
            return -1;
        if (di <= traits_.near_lossless)
            return 0;
        if (di < t1_)
            return 1;
        if (di < t2_)
            return 2;
        if (di < t3_)
            return 3;

        return 4;
    }

    FORCE_INLINE int32_t quantize_gradient(const int32_t di) const noexcept
    {
        ASSERT(quantize_gradient_org(di) == *(quantization_ + di));
        return *(quantization_ + di);
    }

    // C4127 = conditional expression is constant (caused by some template methods that are not fully specialized) [VS2017]
    // C6326 = Potential comparison of a constant with another constant. (false warning, triggered by template construction
    // in Checked build)
    // C26814 = The const variable 'RANGE' can be computed at compile-time. [incorrect warning, VS 16.3.0 P3]
    MSVC_WARNING_SUPPRESS(4127 6326 26814)

    void initialize_quantization_lut()
    {
        // for lossless mode with default parameters, we have precomputed the look up table for bit counts 8, 10, 12 and 16.
        if (traits_.near_lossless == 0 && traits_.maximum_sample_value == (1 << traits_.bits_per_pixel) - 1)
        {
            const jpegls_pc_parameters presets{compute_default(traits_.maximum_sample_value, traits_.near_lossless)};
            if (presets.threshold1 == t1_ && presets.threshold2 == t2_ && presets.threshold3 == t3_)
            {
                if (traits_.bits_per_pixel == 8)
                {
                    quantization_ = &quantization_lut_lossless_8[quantization_lut_lossless_8.size() / 2];
                    return;
                }
                if (traits_.bits_per_pixel == 10)
                {
                    quantization_ = &quantization_lut_lossless_10[quantization_lut_lossless_10.size() / 2];
                    return;
                }
                if (traits_.bits_per_pixel == 12)
                {
                    quantization_ = &quantization_lut_lossless_12[quantization_lut_lossless_12.size() / 2];
                    return;
                }
                if (traits_.bits_per_pixel == 16)
                {
                    quantization_ = &quantization_lut_lossless_16[quantization_lut_lossless_16.size() / 2];
                    return;
                }
            }
        }

        // Initialize the quantization lookup table dynamic.
        const int32_t range{1 << traits_.bits_per_pixel};
        quantization_lut_.resize(static_cast<size_t>(range) * 2);
        for (size_t i{}; i < quantization_lut_.size(); ++i)
        {
            quantization_lut_[i] = quantize_gradient_org(-range + static_cast<int32_t>(i));
        }

        quantization_ = &quantization_lut_[range];
    }
    MSVC_WARNING_UNSUPPRESS()

    int32_t decode_value(const int32_t k, const int32_t limit, const int32_t quantized_bits_per_pixel)
    {
        const int32_t high_bits{Strategy::read_high_bits()};

        if (high_bits >= limit - (quantized_bits_per_pixel + 1))
            return Strategy::read_value(quantized_bits_per_pixel) + 1;

        if (k == 0)
            return high_bits;

        return (high_bits << k) + Strategy::read_value(k);
    }

    FORCE_INLINE void encode_mapped_value(const int32_t k, const int32_t mapped_error, const int32_t limit)
    {
        int32_t high_bits{mapped_error >> k};

        if (high_bits < limit - traits_.quantized_bits_per_pixel - 1)
        {
            if (high_bits + 1 > 31)
            {
                Strategy::append_to_bit_stream(0, high_bits / 2);
                high_bits = high_bits - high_bits / 2;
            }
            Strategy::append_to_bit_stream(1, high_bits + 1);
            Strategy::append_to_bit_stream((mapped_error & ((1 << k) - 1)), k);
            return;
        }

        if (limit - traits_.quantized_bits_per_pixel > 31)
        {
            Strategy::append_to_bit_stream(0, 31);
            Strategy::append_to_bit_stream(1, limit - traits_.quantized_bits_per_pixel - 31);
        }
        else
        {
            Strategy::append_to_bit_stream(1, limit - traits_.quantized_bits_per_pixel);
        }
        Strategy::append_to_bit_stream((mapped_error - 1) & ((1 << traits_.quantized_bits_per_pixel) - 1),
                                       traits_.quantized_bits_per_pixel);
    }

    void increment_run_index() noexcept
    {
        run_index_ = std::min(31, run_index_ + 1);
    }

    void decrement_run_index() noexcept
    {
        run_index_ = std::max(0, run_index_ - 1);
    }

    FORCE_INLINE sample_type do_regular(const int32_t qs, int32_t /*x*/, const int32_t predicted,
                                        decoder_strategy* /*template_selector*/)
    {
        const int32_t sign{bit_wise_sign(qs)};
        context_regular_mode& context{contexts_[apply_sign(qs, sign)]};
        const int32_t k{context.get_golomb_coding_parameter()};
        const int32_t predicted_value{traits_.correct_prediction(predicted + apply_sign(context.c(), sign))};

        int32_t error_value;
        const golomb_code& code = decoding_tables[k].get(Strategy::peek_byte());
        if (code.length() != 0)
        {
            Strategy::skip(code.length());
            error_value = code.value();
            ASSERT(std::abs(error_value) < 65535);
        }
        else
        {
            error_value = unmap_error_value(decode_value(k, traits_.limit, traits_.quantized_bits_per_pixel));
            if (UNLIKELY(std::abs(error_value) > 65535))
                impl::throw_jpegls_error(jpegls_errc::invalid_encoded_data);
        }
        if (k == 0)
        {
            error_value = error_value ^ context.get_error_correction(traits_.near_lossless);
        }
        context.update_variables_and_bias(error_value, traits_.near_lossless, traits_.reset_threshold);
        error_value = apply_sign(error_value, sign);
        return traits_.compute_reconstructed_sample(predicted_value, error_value);
    }

    FORCE_INLINE sample_type do_regular(const int32_t qs, const int32_t x, const int32_t predicted,
                                        encoder_strategy* /*template_selector*/)
    {
        const int32_t sign{bit_wise_sign(qs)};
        context_regular_mode& context{contexts_[apply_sign(qs, sign)]};
        const int32_t k{context.get_golomb_coding_parameter()};
        const int32_t predicted_value{traits_.correct_prediction(predicted + apply_sign(context.c(), sign))};
        const int32_t error_value{traits_.compute_error_value(apply_sign(x - predicted_value, sign))};

        encode_mapped_value(k, map_error_value(context.get_error_correction(k | traits_.near_lossless) ^ error_value),
                            traits_.limit);
        context.update_variables_and_bias(error_value, traits_.near_lossless, traits_.reset_threshold);
        ASSERT(traits_.is_near(traits_.compute_reconstructed_sample(predicted_value, apply_sign(error_value, sign)), x));
        return static_cast<sample_type>(
            traits_.compute_reconstructed_sample(predicted_value, apply_sign(error_value, sign)));
    }

    /// <summary>Encodes/Decodes a scan line of samples</summary>
    FORCE_INLINE void do_line(sample_type* /*template_selector*/)
    {
        int32_t index{};
        int32_t rb{previous_line_[index - 1]};
        int32_t rd{previous_line_[index]};

        while (static_cast<uint32_t>(index) < width_)
        {
            const int32_t ra{current_line_[index - 1]};
            const int32_t rc{rb};
            rb = rd;
            rd = previous_line_[index + 1];

            const int32_t qs{
                compute_context_id(quantize_gradient(rd - rb), quantize_gradient(rb - rc), quantize_gradient(rc - ra))};

            if (qs != 0)
            {
                current_line_[index] =
                    do_regular(qs, current_line_[index], get_predicted_value(ra, rb, rc), static_cast<Strategy*>(nullptr));
                ++index;
            }
            else
            {
                index += do_run_mode(index, static_cast<Strategy*>(nullptr));
                rb = previous_line_[index - 1];
                rd = previous_line_[index];
            }
        }
    }

    /// <summary>Encodes/Decodes a scan line of triplets in ILV_SAMPLE mode</summary>
    void do_line(triplet<sample_type>* /*template_selector*/)
    {
        int32_t index{};
        while (static_cast<uint32_t>(index) < width_)
        {
            const triplet<sample_type> ra{current_line_[index - 1]};
            const triplet<sample_type> rc{previous_line_[index - 1]};
            const triplet<sample_type> rb{previous_line_[index]};
            const triplet<sample_type> rd{previous_line_[index + 1]};

            const int32_t qs1{compute_context_id(quantize_gradient(rd.v1 - rb.v1), quantize_gradient(rb.v1 - rc.v1),
                                                 quantize_gradient(rc.v1 - ra.v1))};
            const int32_t qs2{compute_context_id(quantize_gradient(rd.v2 - rb.v2), quantize_gradient(rb.v2 - rc.v2),
                                                 quantize_gradient(rc.v2 - ra.v2))};
            const int32_t qs3{compute_context_id(quantize_gradient(rd.v3 - rb.v3), quantize_gradient(rb.v3 - rc.v3),
                                                 quantize_gradient(rc.v3 - ra.v3))};

            if (qs1 == 0 && qs2 == 0 && qs3 == 0)
            {
                index += do_run_mode(index, static_cast<Strategy*>(nullptr));
            }
            else
            {
                triplet<sample_type> rx;
                rx.v1 = do_regular(qs1, current_line_[index].v1, get_predicted_value(ra.v1, rb.v1, rc.v1),
                                   static_cast<Strategy*>(nullptr));
                rx.v2 = do_regular(qs2, current_line_[index].v2, get_predicted_value(ra.v2, rb.v2, rc.v2),
                                   static_cast<Strategy*>(nullptr));
                rx.v3 = do_regular(qs3, current_line_[index].v3, get_predicted_value(ra.v3, rb.v3, rc.v3),
                                   static_cast<Strategy*>(nullptr));
                current_line_[index] = rx;
                ++index;
            }
        }
    }

    // Setup codec for encoding and calls do_scan
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#pragma clang diagnostic ignored "-Wsuggest-override"
#endif

    MSVC_WARNING_SUPPRESS(26433) // C.128: Virtual functions should specify exactly one of virtual, override, or final

    // Note: depending on the base class encode_scan OR decode_scan will be virtual and abstract,
    // cannot use override in all cases.
    // clang-format off
    // NOLINTNEXTLINE(cppcoreguidelines-explicit-virtual-functions, hicpp-use-override, modernize-use-override,clang-diagnostic-suggest-override)
    size_t encode_scan(std::unique_ptr<process_line> process_line, byte_span destination)
    {
        Strategy::process_line_ = std::move(process_line);

        Strategy::initialize(destination);
        encode_lines();

        return Strategy::get_length();
    }

    // NOLINTNEXTLINE(cppcoreguidelines-explicit-virtual-functions, hicpp-use-override, modernize-use-override, clang-diagnostic-suggest-override)
    size_t decode_scan(std::unique_ptr<process_line> process_line, const JlsRect& rect, const_byte_span encoded_source)
    {
        Strategy::process_line_ = std::move(process_line);

        const auto* scan_begin{encoded_source.begin()};
        rect_ = rect;

        Strategy::initialize(encoded_source);

        // Process images without a restart interval, as 1 large restart interval.
        if (restart_interval_ == 0)
        {
            restart_interval_ = frame_info().height;
        }

        decode_lines();

        return Strategy::get_cur_byte_pos() - scan_begin;
    }

    // clang-format on
    MSVC_WARNING_UNSUPPRESS()

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

    void initialize_parameters(const int32_t t1, const int32_t t2, const int32_t t3, const int32_t reset_threshold)
    {
        t1_ = t1;
        t2_ = t2;
        t3_ = t3;
        reset_threshold_ = static_cast<uint8_t>(reset_threshold);

        initialize_quantization_lut();
        reset_parameters();
    }

    void reset_parameters() noexcept
    {
        const context_regular_mode context_initial_value(traits_.range);
        for (auto& context : contexts_)
        {
            context = context_initial_value;
        }

        context_run_mode_[0] = context_run_mode(0, traits_.range);
        context_run_mode_[1] = context_run_mode(1, traits_.range);
        run_index_ = 0;
    }

    static charls::frame_info update_component_count(charls::frame_info frame, const coding_parameters& parameters) noexcept
    {
        if (parameters.interleave_mode == interleave_mode::none)
        {
            frame.component_count = 1;
        }

        return frame;
    }

    // In ILV_SAMPLE mode, multiple components are handled in do_line
    // In ILV_LINE mode, a call to do_line is made for every component
    // In ILV_NONE mode, do_scan is called for each component
    void encode_lines()
    {
        const uint32_t pixel_stride{width_ + 4U};
        const size_t component_count{
            parameters().interleave_mode == interleave_mode::line ? static_cast<size_t>(frame_info().component_count) : 1U};

        std::vector<pixel_type> line_buffer(component_count * pixel_stride * 2);
        std::vector<int32_t> run_index(component_count);

        for (uint32_t line{}; line < frame_info().height; ++line)
        {
            previous_line_ = &line_buffer[1];
            current_line_ = &line_buffer[1 + static_cast<size_t>(component_count) * pixel_stride];
            if ((line & 1) == 1)
            {
                std::swap(previous_line_, current_line_);
            }

            Strategy::on_line_begin(current_line_, width_, pixel_stride);

            for (size_t component{}; component < component_count; ++component)
            {
                run_index_ = run_index[component];

                // initialize edge pixels used for prediction
                previous_line_[width_] = previous_line_[width_ - 1];
                current_line_[-1] = previous_line_[0];
                do_line(static_cast<pixel_type*>(nullptr)); // dummy argument for overload resolution

                run_index[component] = run_index_;
                previous_line_ += pixel_stride;
                current_line_ += pixel_stride;
            }
        }

        Strategy::end_scan();
    }

    void decode_lines()
    {
        const uint32_t pixel_stride{width_ + 4U};
        const size_t component_count{
            parameters().interleave_mode == interleave_mode::line ? static_cast<size_t>(frame_info().component_count) : 1U};

        std::vector<pixel_type> line_buffer(component_count * pixel_stride * 2);
        std::vector<int32_t> run_index(component_count);

        for (uint32_t line{};;)
        {
            const uint32_t lines_in_interval{std::min(frame_info().height - line, restart_interval_)};

            for (uint32_t mcu{}; mcu < lines_in_interval; ++mcu, ++line)
            {
                previous_line_ = &line_buffer[1];
                current_line_ = &line_buffer[1 + static_cast<size_t>(component_count) * pixel_stride];
                if ((line & 1) == 1)
                {
                    std::swap(previous_line_, current_line_);
                }

                for (size_t component{}; component < component_count; ++component)
                {
                    run_index_ = run_index[component];

                    // initialize edge pixels used for prediction
                    previous_line_[width_] = previous_line_[width_ - 1];
                    current_line_[-1] = previous_line_[0];
                    do_line(static_cast<pixel_type*>(nullptr)); // dummy argument for overload resolution

                    run_index[component] = run_index_;
                    previous_line_ += pixel_stride;
                    current_line_ += pixel_stride;
                }

                // Only copy the line if it is part of the output rectangle.
                if (static_cast<uint32_t>(rect_.Y) <= line && line < static_cast<uint32_t>(rect_.Y + rect_.Height))
                {
                    Strategy::on_line_end(current_line_ + rect_.X - (static_cast<size_t>(component_count) * pixel_stride),
                                          rect_.Width,
                                          pixel_stride);
                }
            }

            if (line == frame_info().height)
                break;

            // At this point in the byte stream a restart marker should be present: process it.
            read_restart_marker();
            restart_interval_counter_ = (restart_interval_counter_ + 1) % jpeg_restart_marker_range;

            // After a restart marker it is required to reset the decoder.
            Strategy::reset();
            std::fill(line_buffer.begin(), line_buffer.end(), pixel_type{});
            std::fill(run_index.begin(), run_index.end(), 0);
            reset_parameters();
        }

        Strategy::end_scan();
    }

    void read_restart_marker()
    {
        auto byte{Strategy::read_byte()};
        if (UNLIKELY(byte != jpeg_marker_start_byte))
            impl::throw_jpegls_error(jpegls_errc::restart_marker_not_found);

        // Read all preceding 0xFF fill values until a non 0xFF value has been found. (see T.81, B.1.1.2)
        do
        {
            byte = Strategy::read_byte();
        } while (byte == jpeg_marker_start_byte);

        if (UNLIKELY(byte != jpeg_restart_marker_base + restart_interval_counter_))
            impl::throw_jpegls_error(jpegls_errc::restart_marker_not_found);
    }

    /// <summary>Encodes/Decodes a scan line of quads in ILV_SAMPLE mode</summary>
    void do_line(quad<sample_type>* /*template_selector*/)
    {
        int32_t index{};
        while (static_cast<uint32_t>(index) < width_)
        {
            const quad<sample_type> ra{current_line_[index - 1]};
            const quad<sample_type> rc{previous_line_[index - 1]};
            const quad<sample_type> rb{previous_line_[index]};
            const quad<sample_type> rd{previous_line_[index + 1]};

            const int32_t qs1{compute_context_id(quantize_gradient(rd.v1 - rb.v1), quantize_gradient(rb.v1 - rc.v1),
                                                 quantize_gradient(rc.v1 - ra.v1))};
            const int32_t qs2{compute_context_id(quantize_gradient(rd.v2 - rb.v2), quantize_gradient(rb.v2 - rc.v2),
                                                 quantize_gradient(rc.v2 - ra.v2))};
            const int32_t qs3{compute_context_id(quantize_gradient(rd.v3 - rb.v3), quantize_gradient(rb.v3 - rc.v3),
                                                 quantize_gradient(rc.v3 - ra.v3))};
            const int32_t qs4{compute_context_id(quantize_gradient(rd.v4 - rb.v4), quantize_gradient(rb.v4 - rc.v4),
                                                 quantize_gradient(rc.v4 - ra.v4))};

            if (qs1 == 0 && qs2 == 0 && qs3 == 0 && qs4 == 0)
            {
                index += do_run_mode(index, static_cast<Strategy*>(nullptr));
            }
            else
            {
                quad<sample_type> rx;
                rx.v1 = do_regular(qs1, current_line_[index].v1, get_predicted_value(ra.v1, rb.v1, rc.v1),
                                   static_cast<Strategy*>(nullptr));
                rx.v2 = do_regular(qs2, current_line_[index].v2, get_predicted_value(ra.v2, rb.v2, rc.v2),
                                   static_cast<Strategy*>(nullptr));
                rx.v3 = do_regular(qs3, current_line_[index].v3, get_predicted_value(ra.v3, rb.v3, rc.v3),
                                   static_cast<Strategy*>(nullptr));
                rx.v4 = do_regular(qs4, current_line_[index].v4, get_predicted_value(ra.v4, rb.v4, rc.v4),
                                   static_cast<Strategy*>(nullptr));
                current_line_[index] = rx;
                ++index;
            }
        }
    }

    int32_t decode_run_interruption_error(context_run_mode& context)
    {
        const int32_t k{context.get_golomb_code()};
        const int32_t e_mapped_error_value{
            decode_value(k, traits_.limit - J[run_index_] - 1, traits_.quantized_bits_per_pixel)};
        const int32_t error_value{context.compute_error_value(e_mapped_error_value + context.run_interruption_type(), k)};
        context.update_variables(error_value, e_mapped_error_value, reset_threshold_);
        return error_value;
    }

    triplet<sample_type> decode_run_interruption_pixel(triplet<sample_type> ra, triplet<sample_type> rb)
    {
        const int32_t error_value1{decode_run_interruption_error(context_run_mode_[0])};
        const int32_t error_value2{decode_run_interruption_error(context_run_mode_[0])};
        const int32_t error_value3{decode_run_interruption_error(context_run_mode_[0])};

        return triplet<sample_type>(traits_.compute_reconstructed_sample(rb.v1, error_value1 * sign(rb.v1 - ra.v1)),
                                    traits_.compute_reconstructed_sample(rb.v2, error_value2 * sign(rb.v2 - ra.v2)),
                                    traits_.compute_reconstructed_sample(rb.v3, error_value3 * sign(rb.v3 - ra.v3)));
    }

    quad<sample_type> decode_run_interruption_pixel(quad<sample_type> ra, quad<sample_type> rb)
    {
        const int32_t error_value1{decode_run_interruption_error(context_run_mode_[0])};
        const int32_t error_value2{decode_run_interruption_error(context_run_mode_[0])};
        const int32_t error_value3{decode_run_interruption_error(context_run_mode_[0])};
        const int32_t error_value4{decode_run_interruption_error(context_run_mode_[0])};

        return quad<sample_type>(
            triplet<sample_type>(traits_.compute_reconstructed_sample(rb.v1, error_value1 * sign(rb.v1 - ra.v1)),
                                 traits_.compute_reconstructed_sample(rb.v2, error_value2 * sign(rb.v2 - ra.v2)),
                                 traits_.compute_reconstructed_sample(rb.v3, error_value3 * sign(rb.v3 - ra.v3))),
            traits_.compute_reconstructed_sample(rb.v4, error_value4 * sign(rb.v4 - ra.v4)));
    }

    sample_type decode_run_interruption_pixel(int32_t ra, int32_t rb)
    {
        if (std::abs(ra - rb) <= traits_.near_lossless)
        {
            const int32_t error_value{decode_run_interruption_error(context_run_mode_[1])};
            return static_cast<sample_type>(traits_.compute_reconstructed_sample(ra, error_value));
        }

        const int32_t error_value{decode_run_interruption_error(context_run_mode_[0])};
        return static_cast<sample_type>(traits_.compute_reconstructed_sample(rb, error_value * sign(rb - ra)));
    }

    int32_t decode_run_pixels(pixel_type ra, pixel_type* start_pos, const int32_t pixel_count)
    {
        int32_t index{};
        while (Strategy::read_bit())
        {
            const int count{std::min(1 << J[run_index_], pixel_count - index)};
            index += count;
            ASSERT(index <= pixel_count);

            if (count == (1 << J[run_index_]))
            {
                increment_run_index();
            }

            if (index == pixel_count)
                break;
        }

        if (index != pixel_count)
        {
            // incomplete run.
            index += (J[run_index_] > 0) ? Strategy::read_value(J[run_index_]) : 0;
        }

        if (UNLIKELY(index > pixel_count))
            impl::throw_jpegls_error(jpegls_errc::invalid_encoded_data);

        for (int32_t i{}; i < index; ++i)
        {
            start_pos[i] = ra;
        }

        return index;
    }

    int32_t do_run_mode(const int32_t start_index, decoder_strategy* /*template_selector*/)
    {
        const pixel_type ra{current_line_[start_index - 1]};

        const int32_t run_length{decode_run_pixels(ra, current_line_ + start_index, width_ - start_index)};
        const uint32_t end_index{static_cast<uint32_t>(start_index + run_length)};

        if (end_index == width_)
            return end_index - start_index;

        // run interruption
        const pixel_type rb{previous_line_[end_index]};
        current_line_[end_index] = decode_run_interruption_pixel(ra, rb);
        decrement_run_index();
        return end_index - start_index + 1;
    }

    void encode_run_interruption_error(context_run_mode& context, const int32_t error_value)
    {
        const int32_t k{context.get_golomb_code()};
        const bool map{context.compute_map(error_value, k)};
        const int32_t e_mapped_error_value{2 * std::abs(error_value) - context.run_interruption_type() -
                                           static_cast<int32_t>(map)};

        ASSERT(error_value == context.compute_error_value(e_mapped_error_value + context.run_interruption_type(), k));
        encode_mapped_value(k, e_mapped_error_value, traits_.limit - J[run_index_] - 1);
        context.update_variables(error_value, e_mapped_error_value, reset_threshold_);
    }

    sample_type encode_run_interruption_pixel(const int32_t x, const int32_t ra, const int32_t rb)
    {
        if (std::abs(ra - rb) <= traits_.near_lossless)
        {
            const int32_t error_value{traits_.compute_error_value(x - ra)};
            encode_run_interruption_error(context_run_mode_[1], error_value);
            return static_cast<sample_type>(traits_.compute_reconstructed_sample(ra, error_value));
        }

        const int32_t error_value{traits_.compute_error_value((x - rb) * sign(rb - ra))};
        encode_run_interruption_error(context_run_mode_[0], error_value);
        return static_cast<sample_type>(traits_.compute_reconstructed_sample(rb, error_value * sign(rb - ra)));
    }

    triplet<sample_type> encode_run_interruption_pixel(const triplet<sample_type> x, const triplet<sample_type> ra,
                                                       const triplet<sample_type> rb)
    {
        const int32_t error_value1{traits_.compute_error_value(sign(rb.v1 - ra.v1) * (x.v1 - rb.v1))};
        encode_run_interruption_error(context_run_mode_[0], error_value1);

        const int32_t error_value2{traits_.compute_error_value(sign(rb.v2 - ra.v2) * (x.v2 - rb.v2))};
        encode_run_interruption_error(context_run_mode_[0], error_value2);

        const int32_t error_value3{traits_.compute_error_value(sign(rb.v3 - ra.v3) * (x.v3 - rb.v3))};
        encode_run_interruption_error(context_run_mode_[0], error_value3);

        return triplet<sample_type>(traits_.compute_reconstructed_sample(rb.v1, error_value1 * sign(rb.v1 - ra.v1)),
                                    traits_.compute_reconstructed_sample(rb.v2, error_value2 * sign(rb.v2 - ra.v2)),
                                    traits_.compute_reconstructed_sample(rb.v3, error_value3 * sign(rb.v3 - ra.v3)));
    }

    quad<sample_type> encode_run_interruption_pixel(const quad<sample_type> x, const quad<sample_type> ra,
                                                    const quad<sample_type> rb)
    {
        const int32_t error_value1{traits_.compute_error_value(sign(rb.v1 - ra.v1) * (x.v1 - rb.v1))};
        encode_run_interruption_error(context_run_mode_[0], error_value1);

        const int32_t error_value2{traits_.compute_error_value(sign(rb.v2 - ra.v2) * (x.v2 - rb.v2))};
        encode_run_interruption_error(context_run_mode_[0], error_value2);

        const int32_t error_value3{traits_.compute_error_value(sign(rb.v3 - ra.v3) * (x.v3 - rb.v3))};
        encode_run_interruption_error(context_run_mode_[0], error_value3);

        const int32_t error_value4{traits_.compute_error_value(sign(rb.v4 - ra.v4) * (x.v4 - rb.v4))};
        encode_run_interruption_error(context_run_mode_[0], error_value4);

        return quad<sample_type>(
            triplet<sample_type>(traits_.compute_reconstructed_sample(rb.v1, error_value1 * sign(rb.v1 - ra.v1)),
                                 traits_.compute_reconstructed_sample(rb.v2, error_value2 * sign(rb.v2 - ra.v2)),
                                 traits_.compute_reconstructed_sample(rb.v3, error_value3 * sign(rb.v3 - ra.v3))),
            traits_.compute_reconstructed_sample(rb.v4, error_value4 * sign(rb.v4 - ra.v4)));
    }

    void encode_run_pixels(int32_t run_length, const bool end_of_line)
    {
        while (run_length >= 1 << J[run_index_])
        {
            Strategy::append_ones_to_bit_stream(1);
            run_length = run_length - (1 << J[run_index_]);
            increment_run_index();
        }

        if (end_of_line)
        {
            if (run_length != 0)
            {
                Strategy::append_ones_to_bit_stream(1);
            }
        }
        else
        {
            Strategy::append_to_bit_stream(run_length, J[run_index_] + 1); // leading 0 + actual remaining length
        }
    }

    int32_t do_run_mode(const int32_t index, encoder_strategy* /*strategy*/)
    {
        const int32_t count_type_remain = width_ - index;
        pixel_type* type_cur_x{current_line_ + index};
        const pixel_type* type_prev_x{previous_line_ + index};

        const pixel_type ra{type_cur_x[-1]};

        int32_t run_length{};
        while (traits_.is_near(type_cur_x[run_length], ra))
        {
            type_cur_x[run_length] = ra;
            ++run_length;

            if (run_length == count_type_remain)
                break;
        }

        encode_run_pixels(run_length, run_length == count_type_remain);

        if (run_length == count_type_remain)
            return run_length;

        type_cur_x[run_length] = encode_run_interruption_pixel(type_cur_x[run_length], ra, type_prev_x[run_length]);
        decrement_run_index();
        return run_length + 1;
    }

    // codec parameters
    Traits traits_;
    JlsRect rect_{};
    uint32_t width_;
    int32_t t1_{};
    int32_t t2_{};
    int32_t t3_{};
    uint8_t reset_threshold_{};
    uint32_t restart_interval_{};
    uint32_t restart_interval_counter_{};

    // compression context
    std::array<context_regular_mode, 365> contexts_;
    std::array<context_run_mode, 2> context_run_mode_;
    int32_t run_index_{};
    pixel_type* previous_line_{};
    pixel_type* current_line_{};

    // quantization lookup table
    const int8_t* quantization_{};
    std::vector<int8_t> quantization_lut_;
};



} // namespace charls
