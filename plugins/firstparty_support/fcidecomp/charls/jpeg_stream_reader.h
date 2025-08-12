// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "public_types.h"

#include "byte_span.h"
#include "coding_parameters.h"
#include "util.h"

#include <cstdint>
#include <vector>

namespace charls {

enum class jpeg_marker_code : uint8_t;

// Purpose: minimal implementation to read a JPEG byte stream.
class jpeg_stream_reader final
{
public:
    jpeg_stream_reader() = default;
    ~jpeg_stream_reader() = default;

    jpeg_stream_reader(const jpeg_stream_reader&) = delete;
    jpeg_stream_reader& operator=(const jpeg_stream_reader&) = delete;
    jpeg_stream_reader(jpeg_stream_reader&&) = default;
    jpeg_stream_reader& operator=(jpeg_stream_reader&&) = default;

    void source(const_byte_span source) noexcept;

    const charls::frame_info& frame_info() const noexcept
    {
        return frame_info_;
    }

    const coding_parameters& parameters() const noexcept
    {
        return parameters_;
    }

    const jpegls_pc_parameters& preset_coding_parameters() const noexcept
    {
        return preset_coding_parameters_;
    }

    void output_bgr(const bool value) noexcept
    {
        parameters_.output_bgr = value;
    }

    void rect(const JlsRect& rect) noexcept
    {
        rect_ = rect;
    }

    void at_comment(const callback_function<at_comment_handler> at_comment_callback) noexcept
    {
        at_comment_callback_ = at_comment_callback;
    }

    void at_application_data(const callback_function<at_application_data_handler> at_application_data_callback) noexcept
    {
        at_application_data_callback_ = at_application_data_callback;
    }

    void read_header(spiff_header* header = nullptr, bool* spiff_header_found = nullptr);
    void decode(byte_span destination, size_t stride);
    void read_end_of_image();

private:
    void advance_position(const size_t count) noexcept
    {
        ASSERT(position_ + count <= end_position_);
        position_ += count;
    }

    CHARLS_CHECK_RETURN uint8_t read_byte_checked();
    CHARLS_CHECK_RETURN uint16_t read_uint16_checked();

    CHARLS_CHECK_RETURN uint8_t read_byte() noexcept;
    void skip_byte() noexcept;

    CHARLS_CHECK_RETURN uint8_t read_uint8() noexcept
    {
        return read_byte();
    }

    CHARLS_CHECK_RETURN uint16_t read_uint16() noexcept;
    CHARLS_CHECK_RETURN uint32_t read_uint24() noexcept;
    CHARLS_CHECK_RETURN uint32_t read_uint32() noexcept;
    CHARLS_CHECK_RETURN const_byte_span read_bytes(size_t byte_count) noexcept;
    void read_segment_size();
    void check_minimal_segment_size(size_t minimum_size) const;
    void check_segment_size(size_t expected_size) const;
    void read_next_start_of_scan();
    CHARLS_CHECK_RETURN jpeg_marker_code read_next_marker_code();
    void validate_marker_code(jpeg_marker_code marker_code) const;
    CHARLS_CHECK_RETURN jpegls_pc_parameters get_validated_preset_coding_parameters() const;
    void read_marker_segment(jpeg_marker_code marker_code, spiff_header* header = nullptr,
                             bool* spiff_header_found = nullptr);
    void read_spiff_directory_entry(jpeg_marker_code marker_code);
    void read_start_of_frame_segment();
    void read_start_of_scan_segment();
    void read_comment_segment();
    void read_application_data_segment(jpeg_marker_code marker_code);
    void read_preset_parameters_segment();
    void read_preset_coding_parameters();
    void oversize_image_dimension();
    void read_define_restart_interval_segment();
    void try_read_application_data8_segment(spiff_header* header, bool* spiff_header_found);
    void try_read_spiff_header_segment(CHARLS_OUT spiff_header& header, CHARLS_OUT bool& spiff_header_found);
    void try_read_hp_color_transform_segment();
    void add_component(uint8_t component_id);
    void check_parameter_coherent() const;
    void check_interleave_mode(interleave_mode mode) const;
    CHARLS_CHECK_RETURN uint32_t maximum_sample_value() const noexcept;
    void skip_remaining_segment_data() noexcept;
    void check_frame_info() const;
    void frame_info_height(uint32_t height);
    void frame_info_width(uint32_t width);
    void call_application_data_callback(jpeg_marker_code marker_code) const;

    enum class state
    {
        before_start_of_image,
        header_section,
        spiff_header_section,
        image_section,
        frame_section,
        scan_section,
        bit_stream_section,
        after_end_of_image
    };

    const_byte_span::iterator position_{};
    const_byte_span::iterator end_position_{};
    const_byte_span segment_data_;
    charls::frame_info frame_info_{};
    coding_parameters parameters_{};
    jpegls_pc_parameters preset_coding_parameters_{};
    JlsRect rect_{};
    std::vector<uint8_t> component_ids_;
    state state_{};
    callback_function<at_comment_handler> at_comment_callback_{};
    callback_function<at_application_data_handler> at_application_data_callback_{};
};

} // namespace charls
