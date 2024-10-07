// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "jpeg_stream_reader.h"

#include "constants.h"
#include "decoder_strategy.h"
#include "jls_codec_factory.h"
#include "jpeg_marker_code.h"
#include "jpegls_preset_coding_parameters.h"
#include "jpegls_preset_parameters_type.h"
#include "util.h"

#include <algorithm>
#include <array>
#include <memory>
#include <tuple>

namespace charls {

using impl::throw_jpegls_error;
using std::array;
using std::equal;
using std::find;
using std::unique_ptr;

namespace {

constexpr bool is_restart_marker_code(const jpeg_marker_code marker_code) noexcept
{
    return static_cast<uint8_t>(marker_code) >= jpeg_restart_marker_base &&
           static_cast<uint8_t>(marker_code) < jpeg_restart_marker_base + jpeg_restart_marker_range;
}

constexpr int32_t to_application_data_id(const jpeg_marker_code marker_code) noexcept
{
    return static_cast<int32_t>(marker_code) - static_cast<int32_t>(jpeg_marker_code::application_data0);
}

} // namespace


void jpeg_stream_reader::source(const const_byte_span source) noexcept
{
    ASSERT(state_ == state::before_start_of_image);

    position_ = source.begin();
    end_position_ = source.end();
}


void jpeg_stream_reader::read_header(spiff_header* header, bool* spiff_header_found)
{
    ASSERT(state_ != state::scan_section);

    if (state_ == state::before_start_of_image)
    {
        if (UNLIKELY(read_next_marker_code() != jpeg_marker_code::start_of_image))
            throw_jpegls_error(jpegls_errc::start_of_image_marker_not_found);

        component_ids_.reserve(4); // expect 4 components or less.
        state_ = state::header_section;
    }

    for (;;)
    {
        const jpeg_marker_code marker_code{read_next_marker_code()};
        validate_marker_code(marker_code);
        read_segment_size();

        switch (state_)
        {
        case state::spiff_header_section:
            read_spiff_directory_entry(marker_code);
            break;

        default:
            read_marker_segment(marker_code, header, spiff_header_found);
            break;
        }

        ASSERT(segment_data_.end() - position_ == 0); // All segment data should be processed.

        if (state_ == state::header_section && spiff_header_found && *spiff_header_found)
        {
            state_ = state::spiff_header_section;
            return;
        }

        if (state_ == state::bit_stream_section)
        {
            check_frame_info();
            return;
        }
    }
}


void jpeg_stream_reader::decode(byte_span destination, size_t stride)
{
    ASSERT(state_ == state::bit_stream_section);

    check_parameter_coherent();

    if (rect_.Width <= 0)
    {
        rect_.Width = static_cast<int32_t>(frame_info_.width);
        rect_.Height = static_cast<int32_t>(frame_info_.height);
    }

    // Compute the stride for the uncompressed destination buffer.
    const uint32_t width{rect_.Width != 0 ? static_cast<uint32_t>(rect_.Width) : frame_info_.width};
    const size_t components_in_plane_count{
        parameters_.interleave_mode == interleave_mode::none ? 1U : static_cast<size_t>(frame_info_.component_count)};
    const size_t minimum_stride{components_in_plane_count * width * bit_to_byte_count(frame_info_.bits_per_sample)};

    if (stride == auto_calculate_stride)
    {
        stride = minimum_stride;
    }
    else
    {
        if (UNLIKELY(stride < minimum_stride))
            throw_jpegls_error(jpegls_errc::invalid_argument_stride);
    }

    // Compute the layout of the destination buffer.
    const size_t bytes_per_plane{stride * rect_.Height};
    const size_t plane_count{parameters_.interleave_mode == interleave_mode::none ? frame_info_.component_count : 1U};
    const size_t minimum_destination_size = bytes_per_plane * plane_count - (stride - minimum_stride);
    if (UNLIKELY(destination.size < minimum_destination_size))
        throw_jpegls_error(jpegls_errc::destination_buffer_too_small);

    for (size_t i{}; i < plane_count; ++i)
    {
        if (state_ == state::scan_section)
        {
            read_next_start_of_scan();
            skip_bytes(destination, bytes_per_plane);
        }

        const unique_ptr<decoder_strategy> codec{jls_codec_factory<decoder_strategy>().create_codec(
            frame_info_, parameters_, get_validated_preset_coding_parameters())};
        unique_ptr<process_line> process_line(codec->create_process_line(destination, stride));
        const size_t bytes_read{codec->decode_scan(std::move(process_line), rect_, const_byte_span{position_, end_position_})};
        advance_position(bytes_read);
        state_ = state::scan_section;
    }
}


void jpeg_stream_reader::read_end_of_image()
{
    ASSERT(state_ == state::scan_section);

    const jpeg_marker_code marker_code{read_next_marker_code()};
    if (UNLIKELY(marker_code != jpeg_marker_code::end_of_image))
        throw_jpegls_error(jpegls_errc::end_of_image_marker_not_found);

#ifndef NDEBUG
    state_ = state::after_end_of_image;
#endif
}


void jpeg_stream_reader::read_next_start_of_scan()
{
    ASSERT(state_ == state::scan_section);

    do
    {
        const jpeg_marker_code marker_code{read_next_marker_code()};
        validate_marker_code(marker_code);
        read_segment_size();
        read_marker_segment(marker_code);

        ASSERT(segment_data_.end() - position_ == 0); // All segment data should be processed.
    } while (state_ == state::scan_section);
}


USE_DECL_ANNOTATIONS jpeg_marker_code jpeg_stream_reader::read_next_marker_code()
{
    auto byte{read_byte_checked()};
    if (UNLIKELY(byte != jpeg_marker_start_byte))
        throw_jpegls_error(jpegls_errc::jpeg_marker_start_byte_not_found);

    // Read all preceding 0xFF fill values until a non 0xFF value has been found. (see ISO/IEC 10918-1, B.1.1.2)
    do
    {
        byte = read_byte_checked();
    } while (byte == jpeg_marker_start_byte);

    return static_cast<jpeg_marker_code>(byte);
}


void jpeg_stream_reader::validate_marker_code(const jpeg_marker_code marker_code) const
{
    // ISO/IEC 14495-1, C.1.1. defines the following markers as valid for a JPEG-LS byte stream:
    // SOF55, LSE, SOI, EOI, SOS, DNL, DRI, RSTm, APPn and COM.
    // All other markers shall not be present.
    switch (marker_code)
    {
    case jpeg_marker_code::start_of_scan:
        if (UNLIKELY(state_ != state::scan_section))
            throw_jpegls_error(jpegls_errc::unexpected_marker_found);

        return;

    case jpeg_marker_code::start_of_frame_jpegls:
        if (UNLIKELY(state_ == state::scan_section))
            throw_jpegls_error(jpegls_errc::duplicate_start_of_frame_marker);

        return;

    case jpeg_marker_code::define_restart_interval:
    case jpeg_marker_code::jpegls_preset_parameters:
    case jpeg_marker_code::comment:
    case jpeg_marker_code::application_data0:
    case jpeg_marker_code::application_data1:
    case jpeg_marker_code::application_data2:
    case jpeg_marker_code::application_data3:
    case jpeg_marker_code::application_data4:
    case jpeg_marker_code::application_data5:
    case jpeg_marker_code::application_data6:
    case jpeg_marker_code::application_data7:
    case jpeg_marker_code::application_data8:
    case jpeg_marker_code::application_data9:
    case jpeg_marker_code::application_data10:
    case jpeg_marker_code::application_data11:
    case jpeg_marker_code::application_data12:
    case jpeg_marker_code::application_data13:
    case jpeg_marker_code::application_data14:
    case jpeg_marker_code::application_data15:
        return;

    // Check explicit for one of the other common JPEG encodings.
    case jpeg_marker_code::start_of_frame_baseline_jpeg:
    case jpeg_marker_code::start_of_frame_extended_sequential:
    case jpeg_marker_code::start_of_frame_progressive:
    case jpeg_marker_code::start_of_frame_lossless:
    case jpeg_marker_code::start_of_frame_differential_sequential:
    case jpeg_marker_code::start_of_frame_differential_progressive:
    case jpeg_marker_code::start_of_frame_differential_lossless:
    case jpeg_marker_code::start_of_frame_extended_arithmetic:
    case jpeg_marker_code::start_of_frame_progressive_arithmetic:
    case jpeg_marker_code::start_of_frame_lossless_arithmetic:
    case jpeg_marker_code::start_of_frame_jpegls_extended:
        throw_jpegls_error(jpegls_errc::encoding_not_supported);

    case jpeg_marker_code::start_of_image:
        throw_jpegls_error(jpegls_errc::duplicate_start_of_image_marker);

    case jpeg_marker_code::end_of_image:
        throw_jpegls_error(jpegls_errc::unexpected_end_of_image_marker);
    }

    if (is_restart_marker_code(marker_code))
        throw_jpegls_error(jpegls_errc::unexpected_restart_marker);

    throw_jpegls_error(jpegls_errc::unknown_jpeg_marker_found);
}


USE_DECL_ANNOTATIONS jpegls_pc_parameters jpeg_stream_reader::get_validated_preset_coding_parameters() const
{
    jpegls_pc_parameters preset_coding_parameters;

    if (UNLIKELY(!is_valid(preset_coding_parameters_, calculate_maximum_sample_value(frame_info_.bits_per_sample),
                           parameters_.near_lossless, &preset_coding_parameters)))
        throw_jpegls_error(jpegls_errc::invalid_parameter_jpegls_preset_parameters);

    return preset_coding_parameters;
}


void jpeg_stream_reader::read_marker_segment(const jpeg_marker_code marker_code, spiff_header* header,
                                             bool* spiff_header_found)
{
    switch (marker_code)
    {
    case jpeg_marker_code::start_of_frame_jpegls:
        read_start_of_frame_segment();
        break;

    case jpeg_marker_code::start_of_scan:
        read_start_of_scan_segment();
        break;

    case jpeg_marker_code::jpegls_preset_parameters:
        read_preset_parameters_segment();
        break;

    case jpeg_marker_code::define_restart_interval:
        read_define_restart_interval_segment();
        break;

    case jpeg_marker_code::application_data8:
        try_read_application_data8_segment(header, spiff_header_found);
        break;

    case jpeg_marker_code::comment:
        read_comment_segment();
        break;

    case jpeg_marker_code::application_data0:
    case jpeg_marker_code::application_data1:
    case jpeg_marker_code::application_data2:
    case jpeg_marker_code::application_data3:
    case jpeg_marker_code::application_data4:
    case jpeg_marker_code::application_data5:
    case jpeg_marker_code::application_data6:
    case jpeg_marker_code::application_data7:
    case jpeg_marker_code::application_data9:
    case jpeg_marker_code::application_data10:
    case jpeg_marker_code::application_data11:
    case jpeg_marker_code::application_data12:
    case jpeg_marker_code::application_data13:
    case jpeg_marker_code::application_data14:
    case jpeg_marker_code::application_data15:
        read_application_data_segment(marker_code);
        break;

    // Other tags not supported (among which DNL)
    default:
        ASSERT(false);
        break;
    }
}

void jpeg_stream_reader::read_spiff_directory_entry(const jpeg_marker_code marker_code)
{
    if (UNLIKELY(marker_code != jpeg_marker_code::application_data8))
        throw_jpegls_error(jpegls_errc::missing_end_of_spiff_directory);

    check_minimal_segment_size(4);
    const uint32_t spiff_directory_type{read_uint32()};
    if (spiff_directory_type == spiff_end_of_directory_entry_type)
    {
        check_segment_size(6); // 4 + 2 for dummy SOI.
        state_ = state::image_section;
    }

    skip_remaining_segment_data();
}


void jpeg_stream_reader::read_start_of_frame_segment()
{
    // A JPEG-LS Start of Frame (SOF) segment is documented in ISO/IEC 14495-1, C.2.2
    // This section references ISO/IEC 10918-1, B.2.2, which defines the normal JPEG SOF,
    // with some modifications.
    check_minimal_segment_size(6);

    frame_info_.bits_per_sample = read_uint8();
    if (UNLIKELY(frame_info_.bits_per_sample < minimum_bits_per_sample ||
                 frame_info_.bits_per_sample > maximum_bits_per_sample))
        throw_jpegls_error(jpegls_errc::invalid_parameter_bits_per_sample);

    frame_info_height(read_uint16());
    frame_info_width(read_uint16());

    frame_info_.component_count = read_uint8();
    if (UNLIKELY(frame_info_.component_count == 0))
        throw_jpegls_error(jpegls_errc::invalid_parameter_component_count);

    check_segment_size((static_cast<size_t>(frame_info_.component_count) * 3) + 6);
    for (int32_t i{}; i != frame_info_.component_count; ++i)
    {
        // Component specification parameters
        add_component(read_byte()); // Ci = Component identifier
        const uint8_t horizontal_vertical_sampling_factor{
            read_byte()}; // Hi + Vi = Horizontal sampling factor + Vertical sampling factor
        if (UNLIKELY(horizontal_vertical_sampling_factor != 0x11))
            throw_jpegls_error(jpegls_errc::parameter_value_not_supported);

        skip_byte(); // Tqi = Quantization table destination selector (reserved for JPEG-LS, should be set to 0)
    }

    state_ = state::scan_section;
}


void jpeg_stream_reader::read_comment_segment()
{
    if (at_comment_callback_.handler &&
        UNLIKELY(static_cast<bool>(at_comment_callback_.handler(segment_data_.empty() ? nullptr : position_,
                                                                segment_data_.size(), at_comment_callback_.user_context))))
        throw_jpegls_error(jpegls_errc::callback_failed);

    skip_remaining_segment_data();
}


void jpeg_stream_reader::read_application_data_segment(const jpeg_marker_code marker_code)
{
    call_application_data_callback(marker_code);
    skip_remaining_segment_data();
}


void jpeg_stream_reader::read_preset_parameters_segment()
{
    check_minimal_segment_size(1);
    const auto type{static_cast<jpegls_preset_parameters_type>(read_byte())};
    switch (type)
    {
    case jpegls_preset_parameters_type::preset_coding_parameters:
        read_preset_coding_parameters();
        return;

    case jpegls_preset_parameters_type::oversize_image_dimension:
        oversize_image_dimension();
        return;

    case jpegls_preset_parameters_type::mapping_table_specification:
    case jpegls_preset_parameters_type::mapping_table_continuation:
        throw_jpegls_error(jpegls_errc::parameter_value_not_supported);

    case jpegls_preset_parameters_type::coding_method_specification:
    case jpegls_preset_parameters_type::near_lossless_error_re_specification:
    case jpegls_preset_parameters_type::visually_oriented_quantization_specification:
    case jpegls_preset_parameters_type::extended_prediction_specification:
    case jpegls_preset_parameters_type::start_of_fixed_length_coding:
    case jpegls_preset_parameters_type::end_of_fixed_length_coding:
    case jpegls_preset_parameters_type::extended_preset_coding_parameters:
    case jpegls_preset_parameters_type::inverse_color_transform_specification:
        throw_jpegls_error(jpegls_errc::jpegls_preset_extended_parameter_type_not_supported);
    }

    throw_jpegls_error(jpegls_errc::invalid_jpegls_preset_parameter_type);
}


void jpeg_stream_reader::read_preset_coding_parameters()
{
    check_segment_size(1 + (5 * sizeof(uint16_t)));

    // Note: validation will be done, just before decoding as more info is needed for validation.
    preset_coding_parameters_.maximum_sample_value = read_uint16();
    preset_coding_parameters_.threshold1 = read_uint16();
    preset_coding_parameters_.threshold2 = read_uint16();
    preset_coding_parameters_.threshold3 = read_uint16();
    preset_coding_parameters_.reset_value = read_uint16();
}


void jpeg_stream_reader::oversize_image_dimension()
{
    // Note: The JPEG-LS standard supports a 2,3 or 4 bytes for the size.
    check_minimal_segment_size(2);
    const uint8_t dimension_size{read_uint8()};

    uint32_t height;
    uint32_t width;
    switch (dimension_size)
    {
    case 2:
        check_segment_size(sizeof(uint16_t) * 2 + 2);
        height = read_uint16();
        width = read_uint16();
        break;

    case 3:
        check_segment_size((sizeof(uint16_t) + 1) * 2 + 2);
        height = read_uint24();
        width = read_uint24();
        break;

    case 4:
        check_segment_size(sizeof(uint32_t) * 2 + 2);
        height = read_uint32();
        width = read_uint32();
        break;

    default:
        throw_jpegls_error(jpegls_errc::invalid_parameter_jpegls_preset_parameters);
    }

    frame_info_height(height);
    frame_info_width(width);
}


void jpeg_stream_reader::read_define_restart_interval_segment()
{
    // Note: The JPEG-LS standard supports a 2,3 or 4 byte restart interval (see ISO/IEC 14495-1, C.2.5)
    //       The original JPEG standard only supports 2 bytes (16 bit big endian).
    switch (segment_data_.size())
    {
    case 2:
        parameters_.restart_interval = read_uint16();
        break;

    case 3:
        parameters_.restart_interval = read_uint24();
        break;

    case 4:
        parameters_.restart_interval = read_uint32();
        break;

    default:
        throw_jpegls_error(jpegls_errc::invalid_marker_segment_size);
    }
}


void jpeg_stream_reader::read_start_of_scan_segment()
{
    check_minimal_segment_size(1);
    const size_t component_count_in_scan{read_uint8()};

    // ISO 10918-1, B2.3. defines the limits for the number of image components parameter in a SOS.
    if (UNLIKELY(component_count_in_scan < 1U || component_count_in_scan > 4U ||
                 component_count_in_scan > static_cast<size_t>(frame_info_.component_count)))
        throw_jpegls_error(jpegls_errc::invalid_parameter_component_count);

    if (UNLIKELY(component_count_in_scan != 1 &&
                 component_count_in_scan != static_cast<size_t>(frame_info_.component_count)))
        throw_jpegls_error(jpegls_errc::parameter_value_not_supported);

    check_segment_size((component_count_in_scan * 2) + 4);

    for (size_t i{}; i != component_count_in_scan; ++i)
    {
        skip_byte(); // Skip scan component selector
        const uint8_t mapping_table_selector{read_uint8()};
        if (UNLIKELY(mapping_table_selector != 0))
            throw_jpegls_error(jpegls_errc::parameter_value_not_supported);
    }

    parameters_.near_lossless = read_uint8(); // Read NEAR parameter
    if (UNLIKELY(parameters_.near_lossless > compute_maximum_near_lossless(static_cast<int>(maximum_sample_value()))))
        throw_jpegls_error(jpegls_errc::invalid_parameter_near_lossless);

    const auto mode{static_cast<interleave_mode>(read_byte())}; // Read ILV parameter
    check_interleave_mode(mode);
    parameters_.interleave_mode = mode;

    if (UNLIKELY((read_byte() & 0xFU) != 0)) // Read Ah (no meaning) and Al (point transform).
        throw_jpegls_error(jpegls_errc::parameter_value_not_supported);

    state_ = state::bit_stream_section;
}


USE_DECL_ANNOTATIONS uint8_t jpeg_stream_reader::read_byte_checked()
{
    if (UNLIKELY(position_ == end_position_))
        throw_jpegls_error(jpegls_errc::source_buffer_too_small);

    return read_byte();
}


USE_DECL_ANNOTATIONS uint16_t jpeg_stream_reader::read_uint16_checked()
{
    if (UNLIKELY(position_ + sizeof(uint16_t) > end_position_))
        throw_jpegls_error(jpegls_errc::source_buffer_too_small);

    return read_uint16();
}


USE_DECL_ANNOTATIONS uint8_t jpeg_stream_reader::read_byte() noexcept
{
    ASSERT(position_ != end_position_);

    const uint8_t value{*position_};
    advance_position(1);
    return value;
}


void jpeg_stream_reader::skip_byte() noexcept
{
    advance_position(1);
}


USE_DECL_ANNOTATIONS uint16_t jpeg_stream_reader::read_uint16() noexcept
{
    ASSERT(position_ + sizeof(uint16_t) <= end_position_);

    const auto value{read_big_endian_unaligned<uint16_t>(position_)};
    advance_position(2);
    return value;
}


USE_DECL_ANNOTATIONS uint32_t jpeg_stream_reader::read_uint24() noexcept
{
    const uint32_t value{static_cast<uint32_t>(read_uint8()) << 16U};
    return value + read_uint16();
}


USE_DECL_ANNOTATIONS uint32_t jpeg_stream_reader::read_uint32() noexcept
{
    ASSERT(position_ + sizeof(uint32_t) <= end_position_);

    const auto value{read_big_endian_unaligned<uint32_t>(position_)};
    advance_position(4);
    return value;
}


USE_DECL_ANNOTATIONS const_byte_span jpeg_stream_reader::read_bytes(const size_t byte_count) noexcept
{
    ASSERT(position_ + byte_count <= end_position_);

    const const_byte_span bytes{position_, byte_count};
    advance_position(byte_count);
    return bytes;
}


void jpeg_stream_reader::read_segment_size()
{
    constexpr size_t segment_length{2}; // The segment size also includes the length of the segment length bytes.
    const size_t segment_size{read_uint16_checked()};
    segment_data_ = {position_, segment_size - segment_length};

    if (UNLIKELY(segment_size < segment_length || position_ + segment_data_.size() > end_position_))
        throw_jpegls_error(jpegls_errc::invalid_marker_segment_size);
}


void jpeg_stream_reader::check_minimal_segment_size(const size_t minimum_size) const
{
    if (UNLIKELY(minimum_size > segment_data_.size()))
        throw_jpegls_error(jpegls_errc::invalid_marker_segment_size);
}


void jpeg_stream_reader::check_segment_size(const size_t expected_size) const
{
    if (UNLIKELY(expected_size != segment_data_.size()))
        throw_jpegls_error(jpegls_errc::invalid_marker_segment_size);
}


void jpeg_stream_reader::try_read_application_data8_segment(spiff_header* header, bool* spiff_header_found)
{
    call_application_data_callback(jpeg_marker_code::application_data8);

    if (spiff_header_found)
    {
        ASSERT(header);
        *spiff_header_found = false;
    }

    if (segment_data_.size() == 5)
    {
        try_read_hp_color_transform_segment();
    }
    else if (header && spiff_header_found && segment_data_.size() >= 30)
    {
        try_read_spiff_header_segment(*header, *spiff_header_found);
    }

    skip_remaining_segment_data();
}


void jpeg_stream_reader::try_read_hp_color_transform_segment()
{
    ASSERT(segment_data_.size() == 5);

    const array<uint8_t, 4> mrfx_tag{'m', 'r', 'f', 'x'}; // mrfx = xfrm (in big endian) = colorXFoRM
    if (!equal(mrfx_tag.cbegin(), mrfx_tag.cend(), read_bytes(4).begin()))
        return;

    const auto transformation{read_byte()};
    switch (transformation)
    {
    case static_cast<uint8_t>(color_transformation::none):
    case static_cast<uint8_t>(color_transformation::hp1):
    case static_cast<uint8_t>(color_transformation::hp2):
    case static_cast<uint8_t>(color_transformation::hp3):
        parameters_.transformation = static_cast<color_transformation>(transformation);
        return;

    case 4: // RgbAsYuvLossy (The standard lossy RGB to YCbCr transform used in JPEG.)
    case 5: // Matrix (transformation is controlled using a matrix that is also stored in the segment.
        throw_jpegls_error(jpegls_errc::color_transform_not_supported);

    default:
        throw_jpegls_error(jpegls_errc::invalid_encoded_data);
    }
}


USE_DECL_ANNOTATIONS void jpeg_stream_reader::try_read_spiff_header_segment(spiff_header& header, bool& spiff_header_found)
{
    ASSERT(segment_data_.size() >= 30);

    const array<uint8_t, 6> spiff_tag{'S', 'P', 'I', 'F', 'F', 0};
    if (!equal(spiff_tag.cbegin(), spiff_tag.cend(), read_bytes(6).begin()))
    {
        header = {};
        spiff_header_found = false;
        return;
    }

    const auto high_version{read_byte()};
    if (high_version > spiff_major_revision_number)
    {
        header = {};
        spiff_header_found = false;
        return; // Treat unknown versions as if the SPIFF header doesn't exists.
    }
    skip_byte(); // low version

    header.profile_id = static_cast<spiff_profile_id>(read_byte());
    header.component_count = read_uint8();
    header.height = read_uint32();
    header.width = read_uint32();
    header.color_space = static_cast<spiff_color_space>(read_byte());
    header.bits_per_sample = read_uint8();
    header.compression_type = static_cast<spiff_compression_type>(read_byte());
    header.resolution_units = static_cast<spiff_resolution_units>(read_byte());
    header.vertical_resolution = read_uint32();
    header.horizontal_resolution = read_uint32();

    spiff_header_found = true;
}


void jpeg_stream_reader::add_component(const uint8_t component_id)
{
    if (UNLIKELY(find(component_ids_.cbegin(), component_ids_.cend(), component_id) != component_ids_.cend()))
        throw_jpegls_error(jpegls_errc::duplicate_component_id_in_sof_segment);

    component_ids_.push_back(component_id);
}


void jpeg_stream_reader::check_parameter_coherent() const
{
    switch (frame_info_.component_count)
    {
    case 4:
    case 3:
        break;
    default:
        if (UNLIKELY(parameters_.interleave_mode != interleave_mode::none))
            throw_jpegls_error(jpegls_errc::parameter_value_not_supported);

        break;
    }
}


void jpeg_stream_reader::check_interleave_mode(const interleave_mode mode) const
{
    constexpr auto errc{jpegls_errc::invalid_parameter_interleave_mode};
    charls::check_interleave_mode(mode, errc);
    if (UNLIKELY(frame_info_.component_count == 1 && mode != interleave_mode::none))
        throw_jpegls_error(errc);
}


USE_DECL_ANNOTATIONS uint32_t jpeg_stream_reader::maximum_sample_value() const noexcept
{
    if (preset_coding_parameters_.maximum_sample_value != 0)
        return static_cast<uint32_t>(preset_coding_parameters_.maximum_sample_value);

    return static_cast<uint32_t>(calculate_maximum_sample_value(frame_info_.bits_per_sample));
}


void jpeg_stream_reader::skip_remaining_segment_data() noexcept
{
    const auto bytes_still_to_read{segment_data_.end() - position_};
    advance_position(bytes_still_to_read);
}


void jpeg_stream_reader::check_frame_info() const
{
    if (UNLIKELY(frame_info_.height < 1))
        throw_jpegls_error(jpegls_errc::parameter_value_not_supported);

    if (UNLIKELY(frame_info_.width < 1))
        throw_jpegls_error(jpegls_errc::invalid_parameter_width);
}


void jpeg_stream_reader::frame_info_height(const uint32_t height)
{
    if (height == 0)
        return;

    if (UNLIKELY(frame_info_.height != 0))
        throw_jpegls_error(jpegls_errc::invalid_parameter_height);

    frame_info_.height = height;
}


void jpeg_stream_reader::frame_info_width(const uint32_t width)
{
    if (width == 0)
        return;

    if (UNLIKELY(frame_info_.width != 0))
        throw_jpegls_error(jpegls_errc::invalid_parameter_width);

    frame_info_.width = width;
}


void jpeg_stream_reader::call_application_data_callback(const jpeg_marker_code marker_code) const
{
    if (at_application_data_callback_.handler &&
        UNLIKELY(static_cast<bool>(at_application_data_callback_.handler(
            to_application_data_id(marker_code), segment_data_.empty() ? nullptr : position_, segment_data_.size(),
            at_application_data_callback_.user_context))))
        throw_jpegls_error(jpegls_errc::callback_failed);
}

} // namespace charls
