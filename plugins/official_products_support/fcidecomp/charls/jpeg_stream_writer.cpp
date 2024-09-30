// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "jpeg_stream_writer.h"

#include "constants.h"
#include "jpeg_marker_code.h"
#include "jpegls_preset_parameters_type.h"
#include "util.h"

#include <array>

namespace charls {

using std::array;
using std::numeric_limits;

jpeg_stream_writer::jpeg_stream_writer(const byte_span destination) noexcept : destination_{destination}
{
}


void jpeg_stream_writer::write_start_of_image()
{
    write_segment_without_data(jpeg_marker_code::start_of_image);
}


void jpeg_stream_writer::write_end_of_image(const bool even_destination_size)
{
    if (even_destination_size && bytes_written() % 2 != 0)
    {
        // Write an additional 0xFF byte to ensure that the encoded bit stream has an even size.
        write_uint8(jpeg_marker_start_byte);
    }

    write_segment_without_data(jpeg_marker_code::end_of_image);
}


void jpeg_stream_writer::write_spiff_header_segment(const spiff_header& header)
{
    ASSERT(header.height > 0);
    ASSERT(header.width > 0);

    static constexpr array<uint8_t, 6> spiff_magic_id{{'S', 'P', 'I', 'F', 'F', '\0'}};

    // Create a JPEG APP8 segment in Still Picture Interchange File Format (SPIFF), v2.0
    write_segment_header(jpeg_marker_code::application_data8, 30);
    write_bytes(spiff_magic_id.data(), spiff_magic_id.size());
    write_uint8(spiff_major_revision_number);
    write_uint8(spiff_minor_revision_number);
    write_uint8(to_underlying_type(header.profile_id));
    write_uint8(header.component_count);
    write_uint32(header.height);
    write_uint32(header.width);
    write_uint8(to_underlying_type(header.color_space));
    write_uint8(header.bits_per_sample);
    write_uint8(to_underlying_type(header.compression_type));
    write_uint8(to_underlying_type(header.resolution_units));
    write_uint32(header.vertical_resolution);
    write_uint32(header.horizontal_resolution);
}


USE_DECL_ANNOTATIONS void jpeg_stream_writer::write_spiff_directory_entry(const uint32_t entry_tag, const void* entry_data,
                                                                          const size_t entry_data_size_bytes)
{
    write_segment_header(jpeg_marker_code::application_data8, sizeof(uint32_t) + entry_data_size_bytes);
    write_uint32(entry_tag);
    write_bytes(entry_data, entry_data_size_bytes);
}


void jpeg_stream_writer::write_spiff_end_of_directory_entry()
{
    // Note: ISO/IEC 10918-3, Annex F.2.2.3 documents that the EOD entry segment should have a length of 8
    // but only 6 data bytes. This approach allows to wrap existing bit streams\encoders with a SPIFF header.
    // In this implementation the SOI marker is added as data bytes to simplify the design.
    static constexpr array<uint8_t, 6> spiff_end_of_directory{
        {0, 0, 0, spiff_end_of_directory_entry_type, 0xFF, to_underlying_type(charls::jpeg_marker_code::start_of_image)}};

    write_segment_header(jpeg_marker_code::application_data8, spiff_end_of_directory.size());
    write_bytes(spiff_end_of_directory.data(), spiff_end_of_directory.size());
}


bool jpeg_stream_writer::write_start_of_frame_segment(const frame_info& frame)
{
    ASSERT(frame.width > 0);
    ASSERT(frame.height > 0);
    ASSERT(frame.bits_per_sample >= minimum_bits_per_sample && frame.bits_per_sample <= maximum_bits_per_sample);
    ASSERT(frame.component_count > 0 && frame.component_count <= numeric_limits<uint8_t>::max());

    // Create a Frame Header as defined in ISO/IEC 14495-1, C.2.2 and T.81, B.2.2
    const size_t data_size{6 + (static_cast<size_t>(frame.component_count) * 3)};
    write_segment_header(jpeg_marker_code::start_of_frame_jpegls, data_size);
    write_uint8(frame.bits_per_sample); // P = Sample precision

    const bool oversized_image{frame.width > numeric_limits<uint16_t>::max() ||
                               frame.height > numeric_limits<uint16_t>::max()};
    write_uint16(oversized_image ? 0 : frame.height); // Y = Number of lines
    write_uint16(oversized_image ? 0 : frame.width);  // X = Number of samples per line

    // Components
    write_uint8(frame.component_count); // Nf = Number of image components in frame

    // Use by default 1 as the start component identifier to remain compatible with the
    // code sample of ISO/IEC 14495-1, H.4 and the JPEG-LS ISO conformance sample files.
    for (auto component_id{1}; component_id <= frame.component_count; ++component_id)
    {
        // Component Specification parameters
        write_uint8(component_id); // Ci = Component identifier
        write_uint8(0x11);         // Hi + Vi = Horizontal sampling factor + Vertical sampling factor
        write_uint8(0); // Tqi = Quantization table destination selector (reserved for JPEG-LS, should be set to 0)
    }

    return oversized_image;
}


void jpeg_stream_writer::write_color_transform_segment(const color_transformation transformation)
{
    const array<uint8_t, 5> segment{'m', 'r', 'f', 'x', static_cast<uint8_t>(transformation)};

    write_segment_header(jpeg_marker_code::application_data8, segment.size());
    write_bytes(segment.data(), segment.size());
}


void jpeg_stream_writer::write_comment_segment(const const_byte_span comment)
{
    write_segment_header(jpeg_marker_code::comment, comment.size());
    write_bytes(comment);
}


void jpeg_stream_writer::write_application_data_segment(const int32_t application_data_id, const const_byte_span application_data)
{
    ASSERT(application_data_id >= minimum_application_data_id && application_data_id <= maximum_application_data_id);

    write_segment_header(
        static_cast<jpeg_marker_code>(static_cast<int32_t>(jpeg_marker_code::application_data0) + application_data_id),
        application_data.size());
    write_bytes(application_data);
}


void jpeg_stream_writer::write_jpegls_preset_parameters_segment(const jpegls_pc_parameters& preset_coding_parameters)
{
    write_segment_header(jpeg_marker_code::jpegls_preset_parameters, 1 + 5 * sizeof(uint16_t));
    write_uint8(to_underlying_type(jpegls_preset_parameters_type::preset_coding_parameters));
    write_uint16(preset_coding_parameters.maximum_sample_value);
    write_uint16(preset_coding_parameters.threshold1);
    write_uint16(preset_coding_parameters.threshold2);
    write_uint16(preset_coding_parameters.threshold3);
    write_uint16(preset_coding_parameters.reset_value);
}


void jpeg_stream_writer::write_jpegls_preset_parameters_segment(const uint32_t height, const uint32_t width)
{
    // Format is defined in ISO/IEC 14495-1, C.2.4.1.4
    write_segment_header(jpeg_marker_code::jpegls_preset_parameters, sizeof(uint32_t) * 2 + 1 + 1);
    write_uint8(to_underlying_type(jpegls_preset_parameters_type::oversize_image_dimension));
    write_uint8(sizeof(uint32_t)); // Wxy: number of bytes used to represent Ye and Xe [2..4]. Always 4 for simplicity.
    write_uint32(height);          // Ye: number of lines in the image.
    write_uint32(width);           // Xe: number of columns in the image.
}


void jpeg_stream_writer::write_start_of_scan_segment(const int32_t component_count, const int32_t near_lossless,
                                                     const interleave_mode interleave_mode)
{
    ASSERT(component_count > 0 && component_count <= numeric_limits<uint8_t>::max());
    ASSERT(near_lossless >= 0 && near_lossless <= numeric_limits<uint8_t>::max());
    ASSERT(interleave_mode == interleave_mode::none || interleave_mode == interleave_mode::line ||
           interleave_mode == interleave_mode::sample);

    // Create a Scan Header as defined in T.87, C.2.3 and T.81, B.2.3
    write_segment_header(jpeg_marker_code::start_of_scan, 1 + (static_cast<size_t>(component_count) * 2) + 3);
    write_uint8(component_count);

    for (int32_t i{}; i != component_count; ++i)
    {
        write_uint8(component_id_);
        write_uint8(0); // Mapping table selector (0 = no table)
        ++component_id_;
    }

    write_uint8(near_lossless);                       // NEAR parameter
    write_uint8(to_underlying_type(interleave_mode)); // ILV parameter
    write_uint8(0);                                   // transformation
}


void jpeg_stream_writer::write_segment_header(const jpeg_marker_code marker_code, const size_t data_size)
{
    ASSERT(data_size <= segment_max_data_size);

    // Check if there is enough room in the destination to write the complete segment.
    // Other methods assume that the checking in done here and don't check again.
    constexpr size_t marker_code_size{2};
    const size_t total_segment_size{marker_code_size + segment_length_size + data_size};
    if (UNLIKELY(byte_offset_ + total_segment_size > destination_.size))
        impl::throw_jpegls_error(jpegls_errc::destination_buffer_too_small);

    write_marker(marker_code);
    write_uint16(static_cast<uint16_t>(segment_length_size + data_size));
}

} // namespace charls
