// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "annotations.h"
#include "api_abi.h"

#ifdef __cplusplus

#include <cstddef>
#include <cstdint>
#include <system_error>

namespace charls {
namespace impl {

#else
#include <stddef.h>
#include <stdint.h>
#endif

// The following enum values are for C applications, for C++ the enums are defined after these definitions.
// For the documentation, see the C++ enums definitions.

CHARLS_RETURN_TYPE_SUCCESS(return == 0)
enum charls_jpegls_errc
{
    CHARLS_JPEGLS_ERRC_SUCCESS = 0,
    CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT = 1,
    CHARLS_JPEGLS_ERRC_PARAMETER_VALUE_NOT_SUPPORTED = 2,
    CHARLS_JPEGLS_ERRC_DESTINATION_BUFFER_TOO_SMALL = 3,
    CHARLS_JPEGLS_ERRC_SOURCE_BUFFER_TOO_SMALL = 4,
    CHARLS_JPEGLS_ERRC_INVALID_ENCODED_DATA = 5,
    CHARLS_JPEGLS_ERRC_TOO_MUCH_ENCODED_DATA = 6,
    CHARLS_JPEGLS_ERRC_INVALID_OPERATION = 7,
    CHARLS_JPEGLS_ERRC_BIT_DEPTH_FOR_TRANSFORM_NOT_SUPPORTED = 8,
    CHARLS_JPEGLS_ERRC_COLOR_TRANSFORM_NOT_SUPPORTED = 9,
    CHARLS_JPEGLS_ERRC_ENCODING_NOT_SUPPORTED = 10,
    CHARLS_JPEGLS_ERRC_UNKNOWN_JPEG_MARKER_FOUND = 11,
    CHARLS_JPEGLS_ERRC_JPEG_MARKER_START_BYTE_NOT_FOUND = 12,
    CHARLS_JPEGLS_ERRC_NOT_ENOUGH_MEMORY = 13,
    CHARLS_JPEGLS_ERRC_UNEXPECTED_FAILURE = 14,
    CHARLS_JPEGLS_ERRC_START_OF_IMAGE_MARKER_NOT_FOUND = 15,
    CHARLS_JPEGLS_ERRC_UNEXPECTED_MARKER_FOUND = 16,
    CHARLS_JPEGLS_ERRC_INVALID_MARKER_SEGMENT_SIZE = 17,
    CHARLS_JPEGLS_ERRC_DUPLICATE_START_OF_IMAGE_MARKER = 18,
    CHARLS_JPEGLS_ERRC_DUPLICATE_START_OF_FRAME_MARKER = 19,
    CHARLS_JPEGLS_ERRC_DUPLICATE_COMPONENT_ID_IN_SOF_SEGMENT = 20,
    CHARLS_JPEGLS_ERRC_UNEXPECTED_END_OF_IMAGE_MARKER = 21,
    CHARLS_JPEGLS_ERRC_INVALID_JPEGLS_PRESET_PARAMETER_TYPE = 22,
    CHARLS_JPEGLS_ERRC_JPEGLS_PRESET_EXTENDED_PARAMETER_TYPE_NOT_SUPPORTED = 23,
    CHARLS_JPEGLS_ERRC_MISSING_END_OF_SPIFF_DIRECTORY = 24,
    CHARLS_JPEGLS_ERRC_UNEXPECTED_RESTART_MARKER = 25,
    CHARLS_JPEGLS_ERRC_RESTART_MARKER_NOT_FOUND = 26,
    CHARLS_JPEGLS_ERRC_CALLBACK_FAILED = 27,
    CHARLS_JPEGLS_ERRC_END_OF_IMAGE_MARKER_NOT_FOUND = 28,
    CHARLS_JPEGLS_ERRC_INVALID_SPIFF_HEADER = 29,
    CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT_WIDTH = 100,
    CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT_HEIGHT = 101,
    CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT_COMPONENT_COUNT = 102,
    CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT_BITS_PER_SAMPLE = 103,
    CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT_INTERLEAVE_MODE = 104,
    CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT_NEAR_LOSSLESS = 105,
    CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT_JPEGLS_PC_PARAMETERS = 106,
    CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT_SIZE = 110,
    CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT_COLOR_TRANSFORMATION = 111,
    CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT_STRIDE = 112,
    CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT_ENCODING_OPTIONS = 113,
    CHARLS_JPEGLS_ERRC_INVALID_PARAMETER_WIDTH = 200,
    CHARLS_JPEGLS_ERRC_INVALID_PARAMETER_HEIGHT = 201,
    CHARLS_JPEGLS_ERRC_INVALID_PARAMETER_COMPONENT_COUNT = 202,
    CHARLS_JPEGLS_ERRC_INVALID_PARAMETER_BITS_PER_SAMPLE = 203,
    CHARLS_JPEGLS_ERRC_INVALID_PARAMETER_INTERLEAVE_MODE = 204,
    CHARLS_JPEGLS_ERRC_INVALID_PARAMETER_NEAR_LOSSLESS = 205,
    CHARLS_JPEGLS_ERRC_INVALID_PARAMETER_JPEGLS_PRESET_PARAMETERS = 206
};

enum charls_interleave_mode
{
    CHARLS_INTERLEAVE_MODE_NONE = 0,
    CHARLS_INTERLEAVE_MODE_LINE = 1,
    CHARLS_INTERLEAVE_MODE_SAMPLE = 2
};

enum charls_encoding_options
{
    CHARLS_ENCODING_OPTIONS_NONE = 0,
    CHARLS_ENCODING_OPTIONS_EVEN_DESTINATION_SIZE = 1,
    CHARLS_ENCODING_OPTIONS_INCLUDE_VERSION_NUMBER = 2,
    CHARLS_ENCODING_OPTIONS_INCLUDE_PC_PARAMETERS_JAI = 4
};

enum charls_color_transformation
{
    CHARLS_COLOR_TRANSFORMATION_NONE = 0,
    CHARLS_COLOR_TRANSFORMATION_HP1 = 1,
    CHARLS_COLOR_TRANSFORMATION_HP2 = 2,
    CHARLS_COLOR_TRANSFORMATION_HP3 = 3
};

enum charls_spiff_profile_id
{
    CHARLS_SPIFF_PROFILE_ID_NONE = 0,
    CHARLS_SPIFF_PROFILE_ID_CONTINUOUS_TONE_BASE = 1,
    CHARLS_SPIFF_PROFILE_ID_CONTINUOUS_TONE_PROGRESSIVE = 2,
    CHARLS_SPIFF_PROFILE_ID_BI_LEVEL_FACSIMILE = 3,
    CHARLS_SPIFF_PROFILE_ID_CONTINUOUS_TONE_FACSIMILE = 4
};

enum charls_spiff_color_space
{
    CHARLS_SPIFF_COLOR_SPACE_BI_LEVEL_BLACK = 0,
    CHARLS_SPIFF_COLOR_SPACE_YCBCR_ITU_BT_709_VIDEO = 1,
    CHARLS_SPIFF_COLOR_SPACE_NONE = 2,
    CHARLS_SPIFF_COLOR_SPACE_YCBCR_ITU_BT_601_1_RGB = 3,
    CHARLS_SPIFF_COLOR_SPACE_YCBCR_ITU_BT_601_1_VIDEO = 4,
    CHARLS_SPIFF_COLOR_SPACE_GRAYSCALE = 8,
    CHARLS_SPIFF_COLOR_SPACE_PHOTO_YCC = 9,
    CHARLS_SPIFF_COLOR_SPACE_RGB = 10,
    CHARLS_SPIFF_COLOR_SPACE_CMY = 11,
    CHARLS_SPIFF_COLOR_SPACE_CMYK = 12,
    CHARLS_SPIFF_COLOR_SPACE_YCCK = 13,
    CHARLS_SPIFF_COLOR_SPACE_CIE_LAB = 14,
    CHARLS_SPIFF_COLOR_SPACE_BI_LEVEL_WHITE = 15
};

enum charls_spiff_compression_type
{
    CHARLS_SPIFF_COMPRESSION_TYPE_UNCOMPRESSED = 0,
    CHARLS_SPIFF_COMPRESSION_TYPE_MODIFIED_HUFFMAN = 1,
    CHARLS_SPIFF_COMPRESSION_TYPE_MODIFIED_READ = 2,
    CHARLS_SPIFF_COMPRESSION_TYPE_MODIFIED_MODIFIED_READ = 3,
    CHARLS_SPIFF_COMPRESSION_TYPE_JBIG = 4,
    CHARLS_SPIFF_COMPRESSION_TYPE_JPEG = 5,
    CHARLS_SPIFF_COMPRESSION_TYPE_JPEG_LS = 6
};

enum charls_spiff_resolution_units
{
    CHARLS_SPIFF_RESOLUTION_UNITS_ASPECT_RATIO = 0,
    CHARLS_SPIFF_RESOLUTION_UNITS_DOTS_PER_INCH = 1,
    CHARLS_SPIFF_RESOLUTION_UNITS_DOTS_PER_CENTIMETER = 2
};

enum charls_spiff_entry_tag
{
    CHARLS_SPIFF_ENTRY_TAG_TRANSFER_CHARACTERISTICS = 2,
    CHARLS_SPIFF_ENTRY_TAG_COMPONENT_REGISTRATION = 3,
    CHARLS_SPIFF_ENTRY_TAG_IMAGE_ORIENTATION = 4,
    CHARLS_SPIFF_ENTRY_TAG_THUMBNAIL = 5,
    CHARLS_SPIFF_ENTRY_TAG_IMAGE_TITLE = 6,
    CHARLS_SPIFF_ENTRY_TAG_IMAGE_DESCRIPTION = 7,
    CHARLS_SPIFF_ENTRY_TAG_TIME_STAMP = 8,
    CHARLS_SPIFF_ENTRY_TAG_VERSION_IDENTIFIER = 9,
    CHARLS_SPIFF_ENTRY_TAG_CREATOR_IDENTIFICATION = 10,
    CHARLS_SPIFF_ENTRY_TAG_PROTECTION_INDICATOR = 11,
    CHARLS_SPIFF_ENTRY_TAG_COPYRIGHT_INFORMATION = 12,
    CHARLS_SPIFF_ENTRY_TAG_CONTACT_INFORMATION = 13,
    CHARLS_SPIFF_ENTRY_TAG_TILE_INDEX = 14,
    CHARLS_SPIFF_ENTRY_TAG_SCAN_INDEX = 15,
    CHARLS_SPIFF_ENTRY_TAG_SET_REFERENCE = 16
};

#ifdef __cplusplus
} // namespace impl
} // namespace charls

namespace charls {

/// <summary>
/// Defines the result values that are returned by the CharLS API functions.
/// </summary>
CHARLS_RETURN_TYPE_SUCCESS(return == 0)
enum class CHARLS_NO_DISCARD jpegls_errc
{
    /// <summary>
    /// The operation completed without errors.
    /// </summary>
    success = impl::CHARLS_JPEGLS_ERRC_SUCCESS,

    /// <summary>
    /// This error is returned when one of the arguments is invalid and no specific reason is available.
    /// </summary>
    invalid_argument = impl::CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT,

    /// <summary>
    /// This error is returned when the JPEG stream contains a parameter value that is not supported by this implementation.
    /// </summary>
    parameter_value_not_supported = impl::CHARLS_JPEGLS_ERRC_PARAMETER_VALUE_NOT_SUPPORTED,

    /// <summary>
    /// The destination buffer is too small to hold all the output.
    /// </summary>
    destination_buffer_too_small = impl::CHARLS_JPEGLS_ERRC_DESTINATION_BUFFER_TOO_SMALL,

    /// <summary>
    /// The source buffer is too small, more input data was expected.
    /// </summary>
    source_buffer_too_small = impl::CHARLS_JPEGLS_ERRC_SOURCE_BUFFER_TOO_SMALL,

    /// <summary>
    /// This error is returned when the encoded bit stream contains a general structural problem.
    /// </summary>
    invalid_encoded_data = impl::CHARLS_JPEGLS_ERRC_INVALID_ENCODED_DATA,

    /// <summary>
    /// Too much compressed data.The decoding process is ready but the input buffer still contains encoded data.
    /// </summary>
    too_much_encoded_data = impl::CHARLS_JPEGLS_ERRC_TOO_MUCH_ENCODED_DATA,

    /// <summary>
    /// This error is returned when a method call is invalid for the current state.
    /// </summary>
    invalid_operation = impl::CHARLS_JPEGLS_ERRC_INVALID_OPERATION,

    /// <summary>
    /// The bit depth for transformation is not supported.
    /// </summary>
    bit_depth_for_transform_not_supported = impl::CHARLS_JPEGLS_ERRC_BIT_DEPTH_FOR_TRANSFORM_NOT_SUPPORTED,

    /// <summary>
    /// The color transform is not supported.
    /// </summary>
    color_transform_not_supported = impl::CHARLS_JPEGLS_ERRC_COLOR_TRANSFORM_NOT_SUPPORTED,

    /// <summary>
    /// This error is returned when an encoded frame is found that is not encoded with the JPEG-LS algorithm.
    /// </summary>
    encoding_not_supported = impl::CHARLS_JPEGLS_ERRC_ENCODING_NOT_SUPPORTED,

    /// <summary>
    /// This error is returned when an unknown JPEG marker code is found in the encoded bit stream.
    /// </summary>
    unknown_jpeg_marker_found = impl::CHARLS_JPEGLS_ERRC_UNKNOWN_JPEG_MARKER_FOUND,

    /// <summary>
    /// This error is returned when the algorithm expect a 0xFF code (indicates start of a JPEG marker) but none was found.
    /// </summary>
    jpeg_marker_start_byte_not_found = impl::CHARLS_JPEGLS_ERRC_JPEG_MARKER_START_BYTE_NOT_FOUND,

    /// <summary>
    /// This error is returned when the implementation could not allocate memory for its internal buffers.
    /// </summary>
    not_enough_memory = impl::CHARLS_JPEGLS_ERRC_NOT_ENOUGH_MEMORY,

    /// <summary>
    /// This error is returned when the implementation encountered a failure it did not expect. No guarantees can be given
    /// for the state after this error.
    /// </summary>
    unexpected_failure = impl::CHARLS_JPEGLS_ERRC_UNEXPECTED_FAILURE,

    /// <summary>
    /// This error is returned when the first JPEG marker is not the SOI marker.
    /// </summary>
    start_of_image_marker_not_found = impl::CHARLS_JPEGLS_ERRC_START_OF_IMAGE_MARKER_NOT_FOUND,

    /// <summary>
    /// This error is returned when a JPEG marker is found that is not valid for the current state.
    /// </summary>
    unexpected_marker_found = impl::CHARLS_JPEGLS_ERRC_UNEXPECTED_MARKER_FOUND,

    /// <summary>
    /// This error is returned when the segment size of a marker segment is invalid.
    /// </summary>
    invalid_marker_segment_size = impl::CHARLS_JPEGLS_ERRC_INVALID_MARKER_SEGMENT_SIZE,

    /// <summary>
    /// This error is returned when the stream contains more then one SOI marker.
    /// </summary>
    duplicate_start_of_image_marker = impl::CHARLS_JPEGLS_ERRC_DUPLICATE_START_OF_IMAGE_MARKER,

    /// <summary>
    /// This error is returned when the stream contains more then one SOF marker.
    /// </summary>
    duplicate_start_of_frame_marker = impl::CHARLS_JPEGLS_ERRC_DUPLICATE_START_OF_FRAME_MARKER,

    /// <summary>
    /// This error is returned when the stream contains duplicate component identifiers in the SOF segment.
    /// </summary>
    duplicate_component_id_in_sof_segment = impl::CHARLS_JPEGLS_ERRC_DUPLICATE_COMPONENT_ID_IN_SOF_SEGMENT,

    /// <summary>
    /// This error is returned when the stream contains an unexpected EOI marker.
    /// </summary>
    unexpected_end_of_image_marker = impl::CHARLS_JPEGLS_ERRC_UNEXPECTED_END_OF_IMAGE_MARKER,

    /// <summary>
    /// This error is returned when the stream contains an invalid type parameter in the JPEG-LS segment.
    /// </summary>
    invalid_jpegls_preset_parameter_type = impl::CHARLS_JPEGLS_ERRC_INVALID_JPEGLS_PRESET_PARAMETER_TYPE,

    /// <summary>
    /// This error is returned when the stream contains an unsupported type parameter in the JPEG-LS segment.
    /// </summary>
    jpegls_preset_extended_parameter_type_not_supported =
        impl::CHARLS_JPEGLS_ERRC_JPEGLS_PRESET_EXTENDED_PARAMETER_TYPE_NOT_SUPPORTED,

    /// <summary>
    /// This error is returned when the stream contains a SPIFF header but not an SPIFF end-of-directory entry.
    /// </summary>
    missing_end_of_spiff_directory = impl::CHARLS_JPEGLS_ERRC_MISSING_END_OF_SPIFF_DIRECTORY,

    /// <summary>
    /// This error is returned when a restart marker is found outside the encoded entropy data.
    /// </summary>
    unexpected_restart_marker = impl::CHARLS_JPEGLS_ERRC_UNEXPECTED_RESTART_MARKER,

    /// <summary>
    /// This error is returned when an expected restart marker is not found. It may indicate data corruption in the JPEG-LS
    /// byte stream.
    /// </summary>
    restart_marker_not_found = impl::CHARLS_JPEGLS_ERRC_RESTART_MARKER_NOT_FOUND,

    /// <summary>
    /// This error is returned when a callback function returns a non zero value.
    /// </summary>
    callback_failed = impl::CHARLS_JPEGLS_ERRC_CALLBACK_FAILED,

    /// <summary>
    /// This error is returned when the End of Image (EOI) marker could not be found.
    /// </summary>
    end_of_image_marker_not_found = impl::CHARLS_JPEGLS_ERRC_END_OF_IMAGE_MARKER_NOT_FOUND,

    /// <summary>
    /// This error is returned when the SPIFF header is invalid.
    /// </summary>
    invalid_spiff_header = impl::CHARLS_JPEGLS_ERRC_INVALID_SPIFF_HEADER,

    /// <summary>
    /// The argument for the width parameter is outside the range [1, 65535].
    /// </summary>
    invalid_argument_width = impl::CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT_WIDTH,

    /// <summary>
    /// The argument for the height parameter is outside the range [1, 65535].
    /// </summary>
    invalid_argument_height = impl::CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT_HEIGHT,

    /// <summary>
    /// The argument for the component count parameter is outside the range [1, 255].
    /// </summary>
    invalid_argument_component_count = impl::CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT_COMPONENT_COUNT,

    /// <summary>
    /// The argument for the bit per sample parameter is outside the range [2, 16].
    /// </summary>
    invalid_argument_bits_per_sample = impl::CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT_BITS_PER_SAMPLE,

    /// <summary>
    /// The argument for the interleave mode is not (None, Sample, Line) or invalid in combination with component count.
    /// </summary>
    invalid_argument_interleave_mode = impl::CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT_INTERLEAVE_MODE,

    /// <summary>
    /// The argument for the near lossless parameter is outside the range [0, 255].
    /// </summary>
    invalid_argument_near_lossless = impl::CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT_NEAR_LOSSLESS,

    /// <summary>
    /// The argument for the JPEG-LS preset coding parameters is not valid, see ISO/IEC 14495-1,
    /// C.2.4.1.1, Table C.1 for the ranges of valid values.
    /// </summary>
    invalid_argument_jpegls_pc_parameters = impl::CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT_JPEGLS_PC_PARAMETERS,

    /// <summary>
    /// The argument for the size parameter is outside the valid range.
    /// </summary>
    invalid_argument_size = impl::CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT_SIZE,

    /// <summary>
    /// The argument for the color component is not (None, Hp1, Hp2, Hp3) or invalid in combination with component count.
    /// </summary>
    invalid_argument_color_transformation = impl::CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT_COLOR_TRANSFORMATION,

    /// <summary>
    /// The stride argument does not match with the frame info and buffer size.
    /// </summary>
    invalid_argument_stride = impl::CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT_STRIDE,

    /// <summary>
    /// The encoding options argument has an invalid value.
    /// </summary>
    invalid_argument_encoding_options = impl::CHARLS_JPEGLS_ERRC_INVALID_ARGUMENT_ENCODING_OPTIONS,

    /// <summary>
    /// This error is returned when the stream contains a width parameter defined more then once or in an incompatible way.
    /// </summary>
    invalid_parameter_width = impl::CHARLS_JPEGLS_ERRC_INVALID_PARAMETER_WIDTH,

    /// <summary>
    /// This error is returned when the stream contains a height parameter defined more then once in an incompatible way.
    /// </summary>
    invalid_parameter_height = impl::CHARLS_JPEGLS_ERRC_INVALID_PARAMETER_HEIGHT,

    /// <summary>
    /// This error is returned when the stream contains a component count parameter outside the range [1,255] for SOF or
    /// [1,4] for SOS.
    /// </summary>
    invalid_parameter_component_count = impl::CHARLS_JPEGLS_ERRC_INVALID_PARAMETER_COMPONENT_COUNT,

    /// <summary>
    /// This error is returned when the stream contains a bits per sample (sample precision) parameter outside the range
    /// [2,16]
    /// </summary>
    invalid_parameter_bits_per_sample = impl::CHARLS_JPEGLS_ERRC_INVALID_PARAMETER_BITS_PER_SAMPLE,

    /// <summary>
    /// This error is returned when the stream contains an interleave mode (ILV) parameter outside the range [0, 2]
    /// </summary>
    invalid_parameter_interleave_mode = impl::CHARLS_JPEGLS_ERRC_INVALID_PARAMETER_INTERLEAVE_MODE,

    /// <summary>
    /// This error is returned when the stream contains a near-lossless (NEAR) parameter outside the range [0, min(255,
    /// MAXVAL/2)]
    /// </summary>
    invalid_parameter_near_lossless = impl::CHARLS_JPEGLS_ERRC_INVALID_PARAMETER_NEAR_LOSSLESS,

    /// <summary>
    /// This error is returned when the stream contains an invalid JPEG-LS preset parameters segment.
    /// </summary>
    invalid_parameter_jpegls_preset_parameters = impl::CHARLS_JPEGLS_ERRC_INVALID_PARAMETER_JPEGLS_PRESET_PARAMETERS,

    // Legacy enumerator names, will be removed in next major release. Not tagged with [[deprecated]] as that is a C++17
    // extension.
    OK = success,
    InvalidJlsParameters = invalid_argument,
    ParameterValueNotSupported = parameter_value_not_supported,
    UncompressedBufferTooSmall = destination_buffer_too_small,
    CompressedBufferTooSmall = source_buffer_too_small,
    InvalidCompressedData = invalid_encoded_data,
    TooMuchCompressedData = too_much_encoded_data,
    UnsupportedColorTransform = color_transform_not_supported,
    UnsupportedEncoding = encoding_not_supported,
    UnknownJpegMarker = unknown_jpeg_marker_found,
    MissingJpegMarkerStart = jpeg_marker_start_byte_not_found,
    UnexpectedFailure = unexpected_failure
};


/// <summary>
/// Defines the interleave modes for multi-component (color) pixel data.
/// </summary>
enum class interleave_mode
{
    /// <summary>
    /// The data is encoded and stored as component for component: RRRGGGBBB.
    /// </summary>
    none = impl::CHARLS_INTERLEAVE_MODE_NONE,

    /// <summary>
    /// The interleave mode is by line. A full line of each component is encoded before moving to the next line.
    /// </summary>
    line = impl::CHARLS_INTERLEAVE_MODE_LINE,

    /// <summary>
    /// The data is encoded and stored by sample. For RGB color images this is the format like RGBRGBRGB.
    /// </summary>
    sample = impl::CHARLS_INTERLEAVE_MODE_SAMPLE,

    // Legacy enumerator names, will be removed in next major release. Not tagged with [[deprecated]] as that is a C++17
    // extension.
    None = none,
    Line = line,
    Sample = sample
};


namespace encoding_options_private {

/// <summary>
/// Defines options that can be enabled during the encoding process.
/// These options can be combined.
/// </summary>
enum class encoding_options : unsigned
{
    /// <summary>
    /// No special encoding option is defined.
    /// </summary>
    none = impl::CHARLS_ENCODING_OPTIONS_NONE,

    /// <summary>
    /// Ensures that the generated encoded data has an even size by adding
    /// an extra 0xFF byte to the End Of Image (EOI) marker.
    /// DICOM requires that data is always even. This can be done by adding a zero padding byte
    /// after the encoded data or with this option.
    /// This option is not default enabled.
    /// </summary>
    even_destination_size = impl::CHARLS_ENCODING_OPTIONS_EVEN_DESTINATION_SIZE,

    /// <summary>
    /// Add a comment (COM) segment with the content: "charls [version-number]" to the encoded data.
    /// Storing the used encoder version can be helpful for long term archival of images.
    /// This option is not default enabled.
    /// </summary>
    include_version_number = impl::CHARLS_ENCODING_OPTIONS_INCLUDE_VERSION_NUMBER,

    /// <summary>
    /// Writes explicitly the default JPEG-LS preset coding parameters when the
    /// bits per sample is larger then 12 bits.
    /// The Java Advanced Imaging (JAI) JPEG-LS codec has a defect that causes it to use invalid
    /// preset coding parameters for these types of images.
    /// Most users of this codec are aware of this problem and have implemented a work-around.
    /// This option is default enabled. Will not be default enabled in the next major version upgrade.
    /// </summary>
    include_pc_parameters_jai = impl::CHARLS_ENCODING_OPTIONS_INCLUDE_PC_PARAMETERS_JAI
};

constexpr encoding_options operator|(const encoding_options lhs, const encoding_options rhs) noexcept
{
    using T = std::underlying_type_t<encoding_options>;
    return static_cast<encoding_options>(static_cast<T>(lhs) | static_cast<T>(rhs));
}

CHARLS_CONSTEXPR encoding_options& operator|=(encoding_options& lhs, const encoding_options rhs) noexcept
{
    lhs = lhs | rhs;
    return lhs;
}

} // namespace encoding_options_private

using encoding_options = encoding_options_private::encoding_options;


/// <summary>
/// Defines color space transformations as defined and implemented by the JPEG-LS library of HP Labs.
/// These color space transformation decrease the correlation between the 3 color components, resulting in better encoding
/// ratio. These options are only implemented for backwards compatibility and NOT part of the JPEG-LS standard. The JPEG-LS
/// ISO/IEC 14495-1:1999 standard provides no capabilities to transport which color space transformation was used.
/// </summary>
enum class color_transformation
{
    /// <summary>
    /// No color space transformation has been applied.
    /// </summary>
    none = impl::CHARLS_COLOR_TRANSFORMATION_NONE,

    /// <summary>
    /// Defines the reversible lossless color transformation:
    /// G = G
    /// R = R - G
    /// B = B - G
    /// </summary>
    hp1 = impl::CHARLS_COLOR_TRANSFORMATION_HP1,

    /// <summary>
    /// Defines the reversible lossless color transformation:
    /// G = G
    /// B = B - (R + G) / 2
    /// R = R - G
    /// </summary>
    hp2 = impl::CHARLS_COLOR_TRANSFORMATION_HP2,

    /// <summary>
    /// Defines the reversible lossless color transformation of Y-Cb-Cr):
    /// R = R - G
    /// B = B - G
    /// G = G + (R + B) / 4
    /// </summary>
    hp3 = impl::CHARLS_COLOR_TRANSFORMATION_HP3,

    // Legacy enumerator names, will be removed in next major release. Not tagged with [[deprecated]] as that is a C++17
    // extension.
    None = none,
    HP1 = hp1,
    HP2 = hp2,
    HP3 = hp3
};


/// <summary>
/// Defines the Application profile identifier options that can be used in a SPIFF header v2, as defined in ISO/IEC 10918-3,
/// F.1.2
/// </summary>
enum class spiff_profile_id : int32_t
{
    /// <summary>
    /// No profile identified.
    /// This is the only valid option for JPEG-LS encoded images.
    /// </summary>
    none = impl::CHARLS_SPIFF_PROFILE_ID_NONE,

    /// <summary>
    /// Continuous-tone base profile (JPEG)
    /// </summary>
    continuous_tone_base = impl::CHARLS_SPIFF_PROFILE_ID_CONTINUOUS_TONE_BASE,

    /// <summary>
    /// Continuous-tone progressive profile
    /// </summary>
    continuous_tone_progressive = impl::CHARLS_SPIFF_PROFILE_ID_CONTINUOUS_TONE_PROGRESSIVE,

    /// <summary>
    /// Bi-level facsimile profile (MH, MR, MMR, JBIG)
    /// </summary>
    bi_level_facsimile = impl::CHARLS_SPIFF_PROFILE_ID_BI_LEVEL_FACSIMILE,

    /// <summary>
    /// Continuous-tone facsimile profile (JPEG)
    /// </summary>
    continuous_tone_facsimile = impl::CHARLS_SPIFF_PROFILE_ID_CONTINUOUS_TONE_FACSIMILE
};


/// <summary>
/// Defines the color space options that can be used in a SPIFF header v2, as defined in ISO/IEC 10918-3, F.2.1.1
/// </summary>
enum class spiff_color_space : int32_t
{
    /// <summary>
    /// Bi-level image. Each image sample is one bit: 0 = white and 1 = black.
    /// This option is not valid for JPEG-LS encoded images.
    /// </summary>
    bi_level_black = impl::CHARLS_SPIFF_COLOR_SPACE_BI_LEVEL_BLACK,

    /// <summary>
    /// The color space is based on recommendation ITU-R BT.709.
    /// </summary>
    ycbcr_itu_bt_709_video = impl::CHARLS_SPIFF_COLOR_SPACE_YCBCR_ITU_BT_709_VIDEO,

    /// <summary>
    /// Color space interpretation of the coded sample is none of the other options.
    /// </summary>
    none = impl::CHARLS_SPIFF_COLOR_SPACE_NONE,

    /// <summary>
    /// The color space is based on recommendation ITU-R BT.601-1. (RGB).
    /// </summary>
    ycbcr_itu_bt_601_1_rgb = impl::CHARLS_SPIFF_COLOR_SPACE_YCBCR_ITU_BT_601_1_RGB,

    /// <summary>
    /// The color space is based on recommendation ITU-R BT.601-1. (video).
    /// </summary>
    ycbcr_itu_bt_601_1_video = impl::CHARLS_SPIFF_COLOR_SPACE_YCBCR_ITU_BT_601_1_VIDEO,

    /// <summary>
    /// Grayscale – This is a single component sample with interpretation as grayscale value, 0 is minimum, 2bps -1 is
    /// maximum.
    /// </summary>
    grayscale = impl::CHARLS_SPIFF_COLOR_SPACE_GRAYSCALE,

    /// <summary>
    /// This is the color encoding method used in the Photo CD™ system.
    /// </summary>
    photo_ycc = impl::CHARLS_SPIFF_COLOR_SPACE_PHOTO_YCC,

    /// <summary>
    /// The encoded data consists of samples of (uncalibrated) R, G and B.
    /// </summary>
    rgb = impl::CHARLS_SPIFF_COLOR_SPACE_RGB,

    /// <summary>
    /// The encoded data consists of samples of Cyan, Magenta and Yellow samples.
    /// </summary>
    cmy = impl::CHARLS_SPIFF_COLOR_SPACE_CMY,

    /// <summary>
    /// The encoded data consists of samples of Cyan, Magenta, Yellow and Black samples.
    /// </summary>
    cmyk = impl::CHARLS_SPIFF_COLOR_SPACE_CMYK,

    /// <summary>
    /// Transformed CMYK type data (same as Adobe PostScript)
    /// </summary>
    ycck = impl::CHARLS_SPIFF_COLOR_SPACE_YCCK,

    /// <summary>
    /// The CIE 1976 (L* a* b*) color space.
    /// </summary>
    cie_lab = impl::CHARLS_SPIFF_COLOR_SPACE_CIE_LAB,

    /// <summary>
    /// Bi-level image. Each image sample is one bit: 1 = white and 0 = black.
    /// This option is not valid for JPEG-LS encoded images.
    /// </summary>
    bi_level_white = impl::CHARLS_SPIFF_COLOR_SPACE_BI_LEVEL_WHITE
};


/// <summary>
/// Defines the compression options that can be used in a SPIFF header v2, as defined in ISO/IEC 10918-3, F.2.1
/// </summary>
enum class spiff_compression_type : int32_t
{
    /// <summary>
    /// Picture data is stored in component interleaved format, encoded at BPS per sample.
    /// </summary>
    uncompressed = impl::CHARLS_SPIFF_COMPRESSION_TYPE_UNCOMPRESSED,

    /// <summary>
    /// Recommendation T.4, the basic algorithm commonly known as MH (Modified Huffman), only allowed for bi-level images.
    /// </summary>
    modified_huffman = impl::CHARLS_SPIFF_COMPRESSION_TYPE_MODIFIED_HUFFMAN,

    /// <summary>
    /// Recommendation T.4, commonly known as MR (Modified READ), only allowed for bi-level images.
    /// </summary>
    modified_read = impl::CHARLS_SPIFF_COMPRESSION_TYPE_MODIFIED_READ,

    /// <summary>
    /// Recommendation T .6, commonly known as MMR (Modified Modified READ), only allowed for bi-level images.
    /// </summary>
    modified_modified_read = impl::CHARLS_SPIFF_COMPRESSION_TYPE_MODIFIED_MODIFIED_READ,

    /// <summary>
    /// ISO/IEC 11544, commonly known as JBIG, only allowed for bi-level images.
    /// </summary>
    jbig = impl::CHARLS_SPIFF_COMPRESSION_TYPE_JBIG,

    /// <summary>
    /// ISO/IEC 10918-1 or ISO/IEC 10918-3, commonly known as JPEG.
    /// </summary>
    jpeg = impl::CHARLS_SPIFF_COMPRESSION_TYPE_JPEG,

    /// <summary>
    /// ISO/IEC 14495-1 or ISO/IEC 14495-2, commonly known as JPEG-LS. (extension defined in ISO/IEC 14495-1).
    /// This is the only valid option for JPEG-LS encoded images.
    /// </summary>
    jpeg_ls = impl::CHARLS_SPIFF_COMPRESSION_TYPE_JPEG_LS
};


/// <summary>
/// Defines the resolution units for the VRES and HRES parameters, as defined in ISO/IEC 10918-3, F.2.1
/// </summary>
enum class spiff_resolution_units : int32_t
{
    /// <summary>
    /// VRES and HRES are to be interpreted as aspect ratio.
    /// </summary>
    /// <remark>
    /// If vertical or horizontal resolutions are not known, use this option and set VRES and HRES
    /// both to 1 to indicate that pixels in the image should be assumed to be square.
    /// </remark>
    aspect_ratio = impl::CHARLS_SPIFF_RESOLUTION_UNITS_ASPECT_RATIO,

    /// <summary>
    /// Units of dots/samples per inch
    /// </summary>
    dots_per_inch = impl::CHARLS_SPIFF_RESOLUTION_UNITS_DOTS_PER_INCH,

    /// <summary>
    /// Units of dots/samples per centimeter.
    /// </summary>
    dots_per_centimeter = impl::CHARLS_SPIFF_RESOLUTION_UNITS_DOTS_PER_CENTIMETER
};


/// <summary>
/// Official defined SPIFF tags defined in Table F.5 (ISO/IEC 10918-3)
/// </summary>
enum class spiff_entry_tag : uint32_t
{
    /// <summary>
    /// This entry describes the opto-electronic transfer characteristics of the source image.
    /// </summary>
    transfer_characteristics = impl::CHARLS_SPIFF_ENTRY_TAG_TRANSFER_CHARACTERISTICS,

    /// <summary>
    /// This entry specifies component registration, the spatial positioning of samples within components relative to the
    /// samples of other components.
    /// </summary>
    component_registration = impl::CHARLS_SPIFF_ENTRY_TAG_COMPONENT_REGISTRATION,

    /// <summary>
    /// This entry specifies the image orientation (rotation, flip).
    /// </summary>
    image_orientation = impl::CHARLS_SPIFF_ENTRY_TAG_IMAGE_ORIENTATION,

    /// <summary>
    /// This entry specifies a reference to a thumbnail.
    /// </summary>
    thumbnail = impl::CHARLS_SPIFF_ENTRY_TAG_THUMBNAIL,

    /// <summary>
    /// This entry describes in textual form a title for the image.
    /// </summary>
    image_title = impl::CHARLS_SPIFF_ENTRY_TAG_IMAGE_TITLE,

    /// <summary>
    /// This entry refers to data in textual form containing additional descriptive information about the image.
    /// </summary>
    image_description = impl::CHARLS_SPIFF_ENTRY_TAG_IMAGE_DESCRIPTION,

    /// <summary>
    /// This entry describes the date and time of the last modification of the image.
    /// </summary>
    time_stamp = impl::CHARLS_SPIFF_ENTRY_TAG_TIME_STAMP,

    /// <summary>
    /// This entry describes in textual form a version identifier which refers to the number of revisions of the image.
    /// </summary>
    version_identifier = impl::CHARLS_SPIFF_ENTRY_TAG_VERSION_IDENTIFIER,

    /// <summary>
    /// This entry describes in textual form the creator of the image.
    /// </summary>
    creator_identification = impl::CHARLS_SPIFF_ENTRY_TAG_CREATOR_IDENTIFICATION,

    /// <summary>
    /// The presence of this entry, indicates that the image’s owner has retained copyright protection and usage rights for
    /// the image.
    /// </summary>
    protection_indicator = impl::CHARLS_SPIFF_ENTRY_TAG_PROTECTION_INDICATOR,

    /// <summary>
    /// This entry describes in textual form copyright information for the image.
    /// </summary>
    copyright_information = impl::CHARLS_SPIFF_ENTRY_TAG_COPYRIGHT_INFORMATION,

    /// <summary>
    /// This entry describes in textual form contact information for use of the image.
    /// </summary>
    contact_information = impl::CHARLS_SPIFF_ENTRY_TAG_CONTACT_INFORMATION,

    /// <summary>
    /// This entry refers to data containing a list of offsets into the file.
    /// </summary>
    tile_index = impl::CHARLS_SPIFF_ENTRY_TAG_TILE_INDEX,

    /// <summary>
    /// This entry refers to data containing the scan list.
    /// </summary>
    scan_index = impl::CHARLS_SPIFF_ENTRY_TAG_SCAN_INDEX,

    /// <summary>
    /// This entry contains a 96-bit reference number intended to relate images stored in separate files.
    /// </summary>
    set_reference = impl::CHARLS_SPIFF_ENTRY_TAG_SET_REFERENCE
};

// Legacy type names, will be removed in next major release.
using ApiResult CHARLS_DEPRECATED = jpegls_errc;
using InterleaveMode CHARLS_DEPRECATED = interleave_mode;
using ColorTransformation CHARLS_DEPRECATED = color_transformation;

} // namespace charls

template<>
struct std::is_error_code_enum<charls::jpegls_errc> final : std::true_type
{
};

using charls_jpegls_errc = charls::jpegls_errc;
using charls_interleave_mode = charls::interleave_mode;
using charls_encoding_options = charls::encoding_options;
using charls_color_transformation = charls::color_transformation;

using charls_spiff_profile_id = charls::spiff_profile_id;
using charls_spiff_color_space = charls::spiff_color_space;
using charls_spiff_compression_type = charls::spiff_compression_type;
using charls_spiff_resolution_units = charls::spiff_resolution_units;
using charls_spiff_entry_tag = charls::spiff_entry_tag;

// Legacy type names, will be removed in next major release.
using CharlsApiResultType = charls::jpegls_errc;
using CharlsInterleaveModeType = charls::interleave_mode;
using CharlsColorTransformationType = charls::color_transformation;

// Defines the size of the char buffer that should be passed to the legacy CharLS API to get the error message text.
// Note: this define will be removed in the next major release as it is not defined in the charls namespace.
CHARLS_CONSTEXPR_INLINE constexpr std::size_t ErrorMessageSize{256};

#else

typedef enum charls_jpegls_errc charls_jpegls_errc;
typedef enum charls_interleave_mode charls_interleave_mode;
typedef enum charls_encoding_options charls_encoding_options;
typedef enum charls_color_transformation charls_color_transformation;

typedef int32_t charls_spiff_profile_id;
typedef int32_t charls_spiff_color_space;
typedef int32_t charls_spiff_compression_type;
typedef int32_t charls_spiff_resolution_units;

// Legacy enum names, will be removed in next major release.
typedef enum charls_jpegls_errc CharlsApiResultType;
typedef enum charls_interleave_mode CharlsInterleaveModeType;
typedef enum charls_color_transformation CharlsColorTransformationType;

// Defines the size of the char buffer that should be passed to the legacy CharLS API to get the error message text.
#define CHARLS_ERROR_MESSAGE_SIZE 256

#endif


/// <summary>
/// Defines the information that can be stored in a SPIFF header as defined in ISO/IEC 10918-3, Annex F
/// </summary>
/// <remark>
/// The type I.8 is an unsigned 8 bit integer, the type I.32 is an 32 bit unsigned integer in the file header itself.
/// The type is indicated by the symbol “F.” are 4-byte parameters in “fixed point” notation.
/// The 16 most significant bits are essentially the same as a parameter of type I.16 and indicate the integer
/// part of this number.
/// The 16 least significant bits are essentially the same as an I.16 parameter and contain an unsigned integer that,
/// when divided by 65536, represents the fractional part of the fixed point number.
/// </remark>
struct charls_spiff_header CHARLS_FINAL
{
    charls_spiff_profile_id profile_id;   // P: Application profile, type I.8
    int32_t component_count;              // NC: Number of color components, range [1, 255], type I.8
    uint32_t height;                      // HEIGHT: Number of lines in image, range [1, 4294967295], type I.32
    uint32_t width;                       // WIDTH: Number of samples per line, range [1, 4294967295], type I.32
    charls_spiff_color_space color_space; // S: Color space used by image data, type is I.8
    int32_t bits_per_sample;              // BPS: Number of bits per sample, range (1, 2, 4, 8, 12, 16), type is I.8
    charls_spiff_compression_type compression_type; // C: Type of data compression used, type is I.8
    charls_spiff_resolution_units resolution_units; // R: Type of resolution units, type is I.8
    uint32_t vertical_resolution;   // VRES: Vertical resolution, range [1, 4294967295], type can be F or I.32
    uint32_t horizontal_resolution; // HRES: Horizontal resolution, range [1, 4294967295], type can be F or I.32
};


/// <summary>
/// Defines the information that can be stored in a JPEG-LS Frame marker segment that applies to all scans.
/// </summary>
/// <remark>
/// The JPEG-LS also allow to store sub-sampling information in a JPEG-LS Frame marker segment.
/// CharLS does not support JPEG-LS images that contain sub-sampled scans.
/// </remark>
struct charls_frame_info CHARLS_FINAL
{
    /// <summary>
    /// Width of the image, range [1, 65535].
    /// </summary>
    uint32_t width;

    /// <summary>
    /// Height of the image, range [1, 65535].
    /// </summary>
    uint32_t height;

    /// <summary>
    /// Number of bits per sample, range [2, 16]
    /// </summary>
    int32_t bits_per_sample;

    /// <summary>
    /// Number of components contained in the frame, range [1, 255]
    /// </summary>
    int32_t component_count;
};


/// <summary>
/// Defines the JPEG-LS preset coding parameters as defined in ISO/IEC 14495-1, C.2.4.1.1.
/// JPEG-LS defines a default set of parameters, but custom parameters can be used.
/// When used these parameters are written into the encoded bit stream as they are needed for the decoding process.
/// </summary>
struct charls_jpegls_pc_parameters CHARLS_FINAL
{
    /// <summary>
    /// Maximum possible value for any image sample in a scan.
    /// This must be greater than or equal to the actual maximum value for the components in a scan.
    /// </summary>
    int32_t maximum_sample_value;

    /// <summary>
    /// First quantization threshold value for the local gradients.
    /// </summary>
    int32_t threshold1;

    /// <summary>
    /// Second quantization threshold value for the local gradients.
    /// </summary>
    int32_t threshold2;

    /// <summary>
    /// Third quantization threshold value for the local gradients.
    /// </summary>
    int32_t threshold3;

    /// <summary>
    /// Value at which the counters A, B, and N are halved.
    /// </summary>
    int32_t reset_value;
};


/// <summary>
/// Defines the JPEG-LS preset coding parameters as defined in ISO/IEC 14495-1, C.2.4.1.1.
/// JPEG-LS defines a default set of parameters, but custom parameters can be used.
/// When used these parameters are written into the encoded bit stream as they are needed for the decoding process.
/// </summary>
struct JpegLSPresetCodingParameters
{
    /// <summary>
    /// Maximum possible value for any image sample in a scan.
    /// This must be greater than or equal to the actual maximum value for the components in a scan.
    /// </summary>
    int32_t MaximumSampleValue;

    /// <summary>
    /// First quantization threshold value for the local gradients.
    /// </summary>
    int32_t Threshold1;

    /// <summary>
    /// Second quantization threshold value for the local gradients.
    /// </summary>
    int32_t Threshold2;

    /// <summary>
    /// Third quantization threshold value for the local gradients.
    /// </summary>
    int32_t Threshold3;

    /// <summary>
    /// Value at which the counters A, B, and N are halved.
    /// </summary>
    int32_t ResetValue;
};

#ifndef __cplusplus
typedef struct JpegLSPresetCodingParameters JpegLSPresetCodingParameters;
#endif


struct JlsRect
{
    int32_t X;
    int32_t Y;
    int32_t Width;
    int32_t Height;
};


/// <summary>
/// Defines the parameters for the JPEG File Interchange Format.
/// The format is defined in the JPEG File Interchange Format v1.02 document by Eric Hamilton.
/// </summary>
/// <remarks>
/// The JPEG File Interchange Format is the de facto standard JPEG interchange format.
/// </remarks>
struct JfifParameters
{
    /// <summary>
    /// Version of the JPEG File Interchange Format.
    /// Should always be set to zero to not write a JFIF header (JFIF headers are replaced with SPIFF headers).
    /// </summary>
    int32_t version;

    /// <summary>
    /// Defines the units for the X and Y densities.
    /// 0: no units, X and Y specify the pixel aspect ratio.
    /// 1: X and Y are dots per inch.
    /// 2: X and Y are dots per cm.
    /// </summary>
    int32_t units;

    /// <summary>
    /// Horizontal pixel density
    /// </summary>
    int32_t Xdensity;

    /// <summary>
    /// Vertical pixel density
    /// </summary>
    int32_t Ydensity;

    /// <summary>
    /// Thumbnail horizontal pixel count.
    /// </summary>
    int32_t Xthumbnail;

    /// <summary>
    /// Thumbnail vertical pixel count.
    /// </summary>
    int32_t Ythumbnail;

    /// <summary>
    /// Reference to a buffer with thumbnail pixels of size Xthumbnail * Ythumbnail * 3(RGB).
    /// This parameter is only used when creating JPEG-LS encoded images.
    /// </summary>
    void* thumbnail;
};

#ifndef __cplusplus
typedef struct JfifParameters JfifParameters;
#endif


struct JlsParameters
{
    /// <summary>
    /// Width of the image in pixels.
    /// This parameter is called "Number of samples per line" in the JPEG-LS standard.
    /// </summary>
    int32_t width;

    /// <summary>
    /// Height of the image in pixels.
    /// This parameter is called "Number of lines" in the JPEG-LS standard.
    /// </summary>
    int32_t height;

    /// <summary>
    /// The number of valid bits per sample to encode.
    /// Valid range 2 - 16. When greater than 8, pixels are assumed to stored as two bytes per sample, otherwise one byte per
    /// sample is assumed. This parameter is called "Sample precision" in the JPEG-LS standard, often also called "Bit
    /// Depth".
    /// </summary>
    int32_t bitsPerSample;

    /// <summary>
    /// The stride is the number of bytes from one row of pixels in memory to the next row of pixels in memory.
    /// Stride is sometimes called pitch. If padding bytes are present, the stride is wider than the width of the image.
    /// </summary>
    int32_t stride;

    /// <summary>
    /// The number of components.
    /// Typical 1 for monochrome images and 3 for color images or 4 if an alpha channel is present.
    /// Up to 255 components are supported by the JPEG-LS standard.
    /// </summary>
    int32_t components;

    /// <summary>
    /// Defines the allowed lossy error. Value 0 defines lossless.
    /// </summary>
    int32_t allowedLossyError;

    /// <summary>
    /// Determines the order of the color components in the compressed stream.
    /// </summary>
    CharlsInterleaveModeType interleaveMode;

    /// <summary>
    /// Color transformation used in the compressed stream. The color transformations are all lossless and
    /// are an HP proprietary extension of the standard. Do not use the color transformations unless
    /// you know the decoder is capable of decoding it. Color transform typically improve compression ratios only
    /// for synthetic images (non - photo-realistic computer generated images).
    /// </summary>
    CharlsColorTransformationType colorTransformation;

    /// <summary>
    /// If set to true RGB images will be decoded to BGR. BGR is the standard ordering in MS Windows bitmaps.
    /// </summary>
    char outputBgr;

    JpegLSPresetCodingParameters custom;
    JfifParameters jfif;
};

#ifdef __cplusplus

/// <summary>
/// Function definition for a callback handler that will be called when a comment (COM) segment is found.
/// </summary>
/// <remarks>
/// </remarks>
/// <param name="data">Reference to the data of the COM segment.</param>
/// <param name="size">Size in bytes of the data of the COM segment.</param>
/// <param name="user_context">Free to use context information that can be set during the installation of the
/// handler.</param>
using charls_at_comment_handler = int32_t(CHARLS_API_CALLING_CONVENTION*)(const void* data, size_t size, void* user_context);

/// <summary>
/// Function definition for a callback handler that will be called when an application data (APPn) segment is found.
/// </summary>
/// <remarks>
/// </remarks>
/// <param name="application_data_id">Id of the APPn segment [0 .. 15].</param>
/// <param name="data">Reference to the data of the APPn segment.</param>
/// <param name="size">Size in bytes of the data of the APPn segment.</param>
/// <param name="user_context">Free to use context information that can be set during the installation of the
/// handler.</param>
using charls_at_application_data_handler = int32_t(CHARLS_API_CALLING_CONVENTION*)(int32_t application_data_id,
                                                                                   const void* data, size_t size,
                                                                                   void* user_context);

namespace charls {

using spiff_header = charls_spiff_header;
using frame_info = charls_frame_info;
using jpegls_pc_parameters = charls_jpegls_pc_parameters;
using at_comment_handler = charls_at_comment_handler;
using at_application_data_handler = charls_at_application_data_handler;

static_assert(sizeof(spiff_header) == 40, "size of struct is incorrect, check padding settings");
static_assert(sizeof(frame_info) == 16, "size of struct is incorrect, check padding settings");
static_assert(sizeof(jpegls_pc_parameters) == 20, "size of struct is incorrect, check padding settings");

} // namespace charls

#else

typedef int32_t(CHARLS_API_CALLING_CONVENTION* charls_at_comment_handler)(const void* data, size_t size, void* user_context);
typedef int32_t(CHARLS_API_CALLING_CONVENTION* charls_at_application_data_handler)(int32_t application_data_id,
                                                                                   const void* data, size_t size,
                                                                                   void* user_context);

typedef struct charls_spiff_header charls_spiff_header;
typedef struct charls_frame_info charls_frame_info;
typedef struct charls_jpegls_pc_parameters charls_jpegls_pc_parameters;

typedef struct JlsParameters JlsParameters;
typedef struct JlsRect JlsRect;

#endif
