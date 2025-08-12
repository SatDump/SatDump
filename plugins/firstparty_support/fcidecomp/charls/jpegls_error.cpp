// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "jpegls_error.h"

#include <string>

namespace charls {

using std::error_category;

class jpegls_category final : public error_category
{
public:
    const char* name() const noexcept override
    {
        return "charls::jpegls";
    }

    std::string message(int error_value) const override
    {
        return charls_get_error_message(static_cast<jpegls_errc>(error_value));
    }
};

} // namespace charls

using namespace charls;

const error_category* CHARLS_API_CALLING_CONVENTION charls_get_jpegls_category()
{
    static class jpegls_category instance;
    return &instance;
}

const char* CHARLS_API_CALLING_CONVENTION charls_get_error_message(const charls_jpegls_errc error_value)
{
    switch (error_value)
    {
    case jpegls_errc::success:
        return "Success";

    case jpegls_errc::invalid_argument:
        return "Invalid argument";

    case jpegls_errc::invalid_argument_width:
        return "The width argument is outside the supported range [1, 65535]";

    case jpegls_errc::invalid_argument_height:
        return "The height argument is outside the supported range [1, 65535]";

    case jpegls_errc::invalid_argument_component_count:
        return "The component count argument is outside the range [1, 255]";

    case jpegls_errc::invalid_argument_bits_per_sample:
        return "The bit per sample argument is outside the range [2, 16]";

    case jpegls_errc::invalid_argument_interleave_mode:
        return "The interleave mode is not None, Sample, Line) or invalid in combination with component count";

    case jpegls_errc::invalid_argument_near_lossless:
        return "The near lossless argument is outside the range [0, 255]";

    case jpegls_errc::invalid_argument_size:
        return "The argument for the entry size parameter is outside the valid range";

    case jpegls_errc::invalid_argument_color_transformation:
        return "The argument for the color component is not (None, Hp1, Hp2, Hp3) or invalid in combination with component "
               "count";

    case jpegls_errc::invalid_argument_jpegls_pc_parameters:
        return "The argument for the JPEG-LS preset coding parameters is not valid";

    case jpegls_errc::invalid_argument_stride:
        return "The stride argument does not match with the frame info and buffer size";

    case jpegls_errc::invalid_argument_encoding_options:
        return "The encoding options argument has an invalid value";

    case jpegls_errc::start_of_image_marker_not_found:
        return "Invalid JPEG-LS stream: first JPEG marker is not a Start Of Image (SOI) marker";

    case jpegls_errc::unexpected_marker_found:
        return "Invalid JPEG-LS stream: unexpected marker found";

    case jpegls_errc::invalid_marker_segment_size:
        return "Invalid JPEG-LS stream: segment size of a marker segment is invalid";

    case jpegls_errc::duplicate_start_of_image_marker:
        return "Invalid JPEG-LS stream: more then one Start Of Image (SOI) marker";

    case jpegls_errc::duplicate_start_of_frame_marker:
        return "Invalid JPEG-LS stream: more then one Start Of Frame (SOF) marker";

    case jpegls_errc::duplicate_component_id_in_sof_segment:
        return "Invalid JPEG-LS stream: duplicate component identifier in the (SOF) segment";

    case jpegls_errc::unexpected_end_of_image_marker:
        return "Invalid JPEG-LS stream: unexpected End Of Image (EOI) marker";

    case jpegls_errc::invalid_jpegls_preset_parameter_type:
        return "Invalid JPEG-LS stream: JPEG-LS preset parameters segment contains an invalid type";

    case jpegls_errc::jpegls_preset_extended_parameter_type_not_supported:
        return "Unsupported JPEG-LS stream: JPEG-LS preset parameters segment contains an JPEG-LS Extended (ISO/IEC "
               "14495-2) type";

    case jpegls_errc::missing_end_of_spiff_directory:
        return "Invalid JPEG-LS stream: SPIFF header without End Of Directory (EOD) entry";

    case jpegls_errc::unexpected_restart_marker:
        return "Invalid JPEG-LS stream: restart (RTSm) marker found outside encoded entropy data";

    case jpegls_errc::restart_marker_not_found:
        return "Invalid JPEG-LS stream: missing expected restart (RTSm) marker";

    case jpegls_errc::callback_failed:
        return "Callback function returned a failure";

    case jpegls_errc::end_of_image_marker_not_found:
        return "Invalid JPEG-LS stream: missing End Of Image (EOI) marker";

    case jpegls_errc::invalid_spiff_header:
        return "Invalid JPEG-LS stream: invalid SPIFF header";

    case jpegls_errc::invalid_parameter_bits_per_sample:
        return "Invalid JPEG-LS stream: the bit per sample (sample precision) parameter is not in the range [2, 16]";

    case jpegls_errc::parameter_value_not_supported:
        return "The JPEG-LS stream is encoded with a parameter value that is not supported by the CharLS decoder";

    case jpegls_errc::destination_buffer_too_small:
        return "The destination buffer is too small to hold all the output";

    case jpegls_errc::source_buffer_too_small:
        return "The source buffer is too small, more input data was expected";

    case jpegls_errc::invalid_encoded_data:
        return "Invalid JPEG-LS stream, the encoded bit stream contains a general structural problem";

    case jpegls_errc::too_much_encoded_data:
        return "Invalid JPEG-LS stream, the decoding process is ready but the source buffer still contains encoded data";

    case jpegls_errc::invalid_operation:
        return "Method call is invalid for the current state";

    case jpegls_errc::bit_depth_for_transform_not_supported:
        return "The bit depth for the transformation is not supported";

    case jpegls_errc::color_transform_not_supported:
        return "The color transform is not supported";

    case jpegls_errc::encoding_not_supported:
        return "Invalid JPEG-LS stream: the JPEG stream is not encoded with the JPEG-LS algorithm";

    case jpegls_errc::unknown_jpeg_marker_found:
        return "Invalid JPEG-LS stream: an unknown JPEG marker code was found";

    case jpegls_errc::jpeg_marker_start_byte_not_found:
        return "Invalid JPEG-LS stream: the leading start byte (0xFF) for a JPEG marker was not found";

    case jpegls_errc::not_enough_memory:
        return "No memory could be allocated for an internal buffer";

    case jpegls_errc::unexpected_failure:
        return "An unexpected internal failure occurred";

    case jpegls_errc::invalid_parameter_width:
        return "Invalid JPEG-LS stream: the width (Number of samples per line) is already defined";

    case jpegls_errc::invalid_parameter_height:
        return "Invalid JPEG-LS stream: the height (Number of lines) is already defined";

    case jpegls_errc::invalid_parameter_component_count:
        return "Invalid JPEG-LS stream: component count in the SOF segment is outside the range [1, 255]";

    case jpegls_errc::invalid_parameter_interleave_mode:
        return "Invalid JPEG-LS stream: interleave mode is outside the range [0, 2] or conflicts with component count";

    case jpegls_errc::invalid_parameter_near_lossless:
        return "Invalid JPEG-LS stream: near-lossless is outside the range [0, min(255, MAXVAL/2)]";

    case jpegls_errc::invalid_parameter_jpegls_preset_parameters:
        return "Invalid JPEG-LS stream: JPEG-LS preset parameters segment contains invalid values";
    }

    return "Unknown";
}
