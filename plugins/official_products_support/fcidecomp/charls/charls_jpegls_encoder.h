// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "jpegls_error.h"

#ifdef __cplusplus
#include <cstring>
#include <memory>
#else
#include <stddef.h>
#endif

#ifdef __cplusplus
struct charls_jpegls_encoder;
extern "C" {
#else
typedef struct charls_jpegls_encoder charls_jpegls_encoder;
#endif

// The following functions define the public C API of the CharLS library.
// The C++ API is defined after the C API.

/// <summary>
/// Creates a JPEG-LS encoder instance, when finished with the instance destroy it with the function
/// charls_jpegls_encoder_destroy.
/// </summary>
/// <returns>A reference to a new created encoder instance, or a null pointer when the creation fails.</returns>
CHARLS_CHECK_RETURN CHARLS_RET_MAY_BE_NULL CHARLS_API_IMPORT_EXPORT charls_jpegls_encoder*
    CHARLS_API_CALLING_CONVENTION charls_jpegls_encoder_create(CHARLS_C_VOID) CHARLS_NOEXCEPT;

/// <summary>
/// Destroys a JPEG-LS encoder instance created with charls_jpegls_encoder_create and releases all internal resources
/// attached to it.
/// </summary>
/// <param name="encoder">Instance to destroy. If a null pointer is passed as argument, no action occurs.</param>
CHARLS_API_IMPORT_EXPORT void CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_destroy(CHARLS_IN_OPT const charls_jpegls_encoder* encoder) CHARLS_NOEXCEPT;

/// <summary>
/// Configures the frame that needs to be encoded. This information will be written to the Start of Frame segment.
/// </summary>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <param name="frame_info">Information about the frame that needs to be encoded.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_set_frame_info(CHARLS_IN charls_jpegls_encoder* encoder,
                                     CHARLS_IN const charls_frame_info* frame_info) CHARLS_NOEXCEPT
    CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Configures the NEAR parameter the encoder should use. A value of 0 means lossless, 0 is also the default.
/// </summary>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <param name="near_lossless">Value of the NEAR parameter.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_set_near_lossless(CHARLS_IN charls_jpegls_encoder* encoder, int32_t near_lossless) CHARLS_NOEXCEPT
    CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Configures the encoding options the encoder should use. Default is charls_encoding_options::include_pc_parameters_jai
/// </summary>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <param name="encoding_options">Options to use.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_set_encoding_options(CHARLS_IN charls_jpegls_encoder* encoder,
                                           charls_encoding_options encoding_options) CHARLS_NOEXCEPT
    CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Configures the interleave mode the encoder should use. The default is none.
/// The encoder expects the input buffer in the same format as the interleave mode.
/// </summary>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <param name="interleave_mode">Value of the interleave mode.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_set_interleave_mode(CHARLS_IN charls_jpegls_encoder* encoder,
                                          charls_interleave_mode interleave_mode) CHARLS_NOEXCEPT
    CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Configures the preset coding parameters the encoder should use.
/// If not set the encoder will use the default preset coding parameters as defined by the JPEG-LS standard.
/// Only when the coding parameters are different than the default parameters or when `include_pc_parameters_jai` is set,
/// they will be written to the JPEG-LS stream during the encode phase.
/// </summary>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <param name="preset_coding_parameters">Reference to the preset coding parameters.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_set_preset_coding_parameters(CHARLS_IN charls_jpegls_encoder* encoder,
                                                   CHARLS_IN const charls_jpegls_pc_parameters* preset_coding_parameters)
    CHARLS_NOEXCEPT CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Configures the HP color transformation the encoder should use.
/// If not set the encoder will use no color transformation.
/// Color transformations are a HP extension and not defined by the JPEG-LS standard and can only be set for 3 component
/// encodings.
/// </summary>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <param name="color_transformation">The color transformation parameters.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_set_color_transformation(CHARLS_IN charls_jpegls_encoder* encoder,
                                               charls_color_transformation color_transformation) CHARLS_NOEXCEPT
    CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Returns the size in bytes, that the encoder expects are needed to hold the encoded image.
/// </summary>
/// <remarks>
/// Size for dynamic extras like SPIFF entries and other tables are not included in this size.
/// </remarks>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <param name="size_in_bytes">Reference to the size that will be set when the functions returns.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_get_estimated_destination_size(CHARLS_IN const charls_jpegls_encoder* encoder,
                                                     CHARLS_OUT size_t* size_in_bytes) CHARLS_NOEXCEPT
    CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Set the reference to the destination buffer that will contain the encoded JPEG-LS byte stream data after encoding.
/// This buffer needs to remain valid during the encoding process.
/// </summary>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <param name="destination_buffer">Reference to the start of the destination buffer.</param>
/// <param name="destination_size_bytes">Size of the destination buffer in bytes.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_ATTRIBUTE_ACCESS((access(write_only, 2, 3)))
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_set_destination_buffer(CHARLS_IN charls_jpegls_encoder* encoder,
                                             CHARLS_OUT_WRITES_BYTES(destination_size_bytes) void* destination_buffer,
                                             size_t destination_size_bytes) CHARLS_NOEXCEPT CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Writes a standard SPIFF header to the destination. The additional values are computed from the current encoder settings.
/// A SPIFF header is optional, but recommended for standalone JPEG-LS files.
/// </summary>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <param name="color_space">The color space of the image.</param>
/// <param name="resolution_units">The resolution units of the next 2 parameters.</param>
/// <param name="vertical_resolution">The vertical resolution.</param>
/// <param name="horizontal_resolution">The horizontal resolution.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_write_standard_spiff_header(CHARLS_IN charls_jpegls_encoder* encoder,
                                                  charls_spiff_color_space color_space,
                                                  charls_spiff_resolution_units resolution_units,
                                                  uint32_t vertical_resolution,
                                                  uint32_t horizontal_resolution) CHARLS_NOEXCEPT
    CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Writes a SPIFF header to the destination.
/// </summary>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <param name="spiff_header">Reference to a SPIFF header that will be written to the destination.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_write_spiff_header(CHARLS_IN charls_jpegls_encoder* encoder,
                                         CHARLS_IN const charls_spiff_header* spiff_header) CHARLS_NOEXCEPT
    CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Writes a SPIFF directory entry to the destination.
/// </summary>
/// <remarks>
/// Function should be called after writing a SPIFF header.
/// </remarks>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <param name="entry_tag">The entry tag of the directory entry.</param>
/// <param name="entry_data">The entry data of the directory entry.</param>
/// <param name="entry_data_size_bytes">The size in bytes of the directory entry [0-65528].</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_ATTRIBUTE_ACCESS((access(read_only, 3, 4)))
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_write_spiff_entry(CHARLS_IN charls_jpegls_encoder* encoder, uint32_t entry_tag,
                                        CHARLS_IN_READS_BYTES(entry_data_size_bytes) const void* entry_data,
                                        size_t entry_data_size_bytes) CHARLS_NOEXCEPT;

/// <summary>
/// Writes a SPIFF end of directory entry to the destination.
/// The encoder will normally does this automatically. It is made available
/// for the scenario to create SPIFF headers in front of existing JPEG-LS streams.
/// </summary>
/// <remarks>
/// The end of directory also includes a SOI marker. This marker should be skipped from the JPEG-LS stream.
/// </remarks>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_write_spiff_end_of_directory_entry(CHARLS_IN charls_jpegls_encoder* encoder) CHARLS_NOEXCEPT;

/// <summary>
/// Writes a comment (COM) segment to the destination.
/// </summary>
/// <remarks>
/// Function should be called before encoding the image data.
/// </remarks>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <param name="comment">The 'comment' bytes. Application specific, usually human readable string.</param>
/// <param name="comment_size_bytes">The size in bytes of the comment [0-65533].</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_ATTRIBUTE_ACCESS((access(read_only, 2, 3)))
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_write_comment(CHARLS_IN charls_jpegls_encoder* encoder,
                                    CHARLS_IN_READS_BYTES(comment_size_bytes) const void* comment,
                                    size_t comment_size_bytes) CHARLS_NOEXCEPT;

/// <summary>
/// Writes an application data (APPn) segment to the destination.
/// </summary>
/// <remarks>
/// Function should be called before encoding the image data.
/// </remarks>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <param name="application_data_id">The ID of the application data segment in the range [0..15].</param>
/// <param name="application_data">The 'application data' bytes. Application specific.</param>
/// <param name="application_data_size_bytes">The size in bytes of the comment [0-65533].</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_ATTRIBUTE_ACCESS((access(read_only, 3, 4)))
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_write_application_data(CHARLS_IN charls_jpegls_encoder* encoder,
                                             int32_t application_data_id,
                                             CHARLS_IN_READS_BYTES(application_data_size_bytes) const void* application_data,
                                             size_t application_data_size_bytes) CHARLS_NOEXCEPT;

/// <summary>
/// Encodes the passed buffer with the source image data to the destination.
/// </summary>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <param name="source_buffer">Byte array that holds the image data that needs to be encoded.</param>
/// <param name="source_size_bytes">Length of the array in bytes.</param>
/// <param name="stride">
/// The number of bytes from one row of pixels in memory to the next row of pixels in memory.
/// Stride is sometimes called pitch. If padding bytes are present, the stride is wider than the width of the image.
/// </param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_ATTRIBUTE_ACCESS((access(read_only, 2, 3)))
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_encode_from_buffer(CHARLS_IN charls_jpegls_encoder* encoder,
                                         CHARLS_IN_READS_BYTES(source_size_bytes) const void* source_buffer,
                                         size_t source_size_bytes, uint32_t stride) CHARLS_NOEXCEPT
    CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Returns the size in bytes, that are written to the destination.
/// </summary>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <param name="bytes_written">Reference to the size that will be set when the functions returns.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_get_bytes_written(CHARLS_IN const charls_jpegls_encoder* encoder,
                                        CHARLS_OUT size_t* bytes_written) CHARLS_NOEXCEPT CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Resets the write position of the destination buffer to the beginning.
/// </summary>
/// <param name="encoder">Reference to the encoder instance.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_rewind(CHARLS_IN charls_jpegls_encoder* encoder) CHARLS_NOEXCEPT CHARLS_ATTRIBUTE((nonnull));

// Note: The method below is considered obsolete and will be removed in the next major update.

/// <summary>
/// Encodes a byte array with pixel data to a JPEG-LS encoded (compressed) byte array.
/// </summary>
/// <remarks>This method is considered obsolete and will be removed in the next major update.</remarks>
/// <param name="destination">Byte array that holds the encoded bytes when the function returns.</param>
/// <param name="destination_length">
/// Length of the array in bytes. If the array is too small the function will return an error.
/// </param>
/// <param name="bytes_written">
/// This parameter will hold the number of bytes written to the destination byte array. Cannot be NULL.
/// </param>
/// <param name="source">Byte array that holds the pixels that should be encoded.</param>
/// <param name="source_length">Length of the array in bytes.</param>
/// <param name="params">Parameter object that describes the pixel data and how to encode it.</param>
/// <param name="error_message">
/// Character array of at least 256 characters or NULL. Hold the error message when a failure occurs, empty otherwise.
/// </param>
CHARLS_ATTRIBUTE_ACCESS((access(write_only, 1, 2), access(read_only, 4, 5)))
CHARLS_DEPRECATED
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT CharlsApiResultType CHARLS_API_CALLING_CONVENTION
JpegLsEncode(CHARLS_OUT_WRITES_BYTES(destination_length) void* destination, size_t destination_length,
             CHARLS_OUT size_t* bytes_written, CHARLS_IN_READS_BYTES(source_length) const void* source, size_t source_length,
             CHARLS_IN const JlsParameters* params, CHARLS_OUT_OPT char* error_message) CHARLS_NOEXCEPT
    CHARLS_ATTRIBUTE((nonnull(1, 3, 4, 6)));

#ifdef __cplusplus

} // extern "C"

namespace charls {

/// <summary>
/// JPEG-LS encoder class that encapsulates the C ABI interface calls and provide a native C++ interface.
/// </summary>
class jpegls_encoder final
{
public:
    /// <summary>
    /// Encoded pixel data in 1 simple operation into a JPEG-LS encoded buffer.
    /// </summary>
    /// <param name="source">Source container with the pixel data bytes that need to be encoded.</param>
    /// <param name="frame">Information about the frame that needs to be encoded.</param>
    /// <param name="interleave_mode">Configures the interleave mode the encoder should use.</param>
    /// <param name="options">Configures the special options the encoder should use.</param>
    /// <exception cref="charls::jpegls_error">An error occurred during the operation.</exception>
    /// <exception cref="std::bad_alloc">Thrown when memory for the encoder could not be allocated.</exception>
    /// <returns>Container with the JPEG-LS encoded bytes.</returns>
    template<typename Container, typename T = typename Container::value_type>
    static Container encode(const Container& source, const charls::frame_info& frame,
                            const charls::interleave_mode interleave_mode = charls::interleave_mode::none,
                            const encoding_options options = charls::encoding_options::none)
    {
        jpegls_encoder encoder;
        encoder.frame_info(frame).interleave_mode(interleave_mode).encoding_options(options);

        Container destination(encoder.estimated_destination_size());
        encoder.destination(destination);

        const size_t bytes_written{encoder.encode(source)};
        destination.resize(bytes_written);

        return destination;
    }

    /// <summary>
    /// Configures the frame that needs to be encoded.
    /// This information will be written to the Start of Frame (SOF) segment during the encode phase.
    /// </summary>
    /// <param name="frame_info">Information about the frame that needs to be encoded.</param>
    /// <exception cref="charls::jpegls_error">An error occurred during the operation.</exception>
    jpegls_encoder& frame_info(const frame_info& frame_info)
    {
        check_jpegls_errc(charls_jpegls_encoder_set_frame_info(encoder_.get(), &frame_info));
        return *this;
    }

    /// <summary>
    /// Configures the NEAR parameter the encoder should use. A value of 0 means lossless, this is also the default.
    /// </summary>
    /// <param name="near_lossless">Value of the NEAR parameter.</param>
    /// <exception cref="charls::jpegls_error">An error occurred during the operation.</exception>
    jpegls_encoder& near_lossless(const int32_t near_lossless)
    {
        check_jpegls_errc(charls_jpegls_encoder_set_near_lossless(encoder_.get(), near_lossless));
        return *this;
    }

    /// <summary>
    /// Configures the interleave mode the encoder should use. The default is none.
    /// The encoder expects the input buffer in the same format as the interleave mode.
    /// </summary>
    /// <param name="interleave_mode">Value of the interleave mode.</param>
    /// <exception cref="charls::jpegls_error">An error occurred during the operation.</exception>
    jpegls_encoder& interleave_mode(const interleave_mode interleave_mode)
    {
        check_jpegls_errc(charls_jpegls_encoder_set_interleave_mode(encoder_.get(), interleave_mode));
        return *this;
    }

    /// <summary>
    /// Configures the encoding options the encoder should use. Default is charls_encoding_options::include_pc_parameters_jai
    /// </summary>
    /// <param name="encoding_options">Options to use. Options can be combined.</param>
    /// <exception cref="charls::jpegls_error">An error occurred during the operation.</exception>
    jpegls_encoder& encoding_options(const encoding_options encoding_options)
    {
        check_jpegls_errc(charls_jpegls_encoder_set_encoding_options(encoder_.get(), encoding_options));
        return *this;
    }

    /// <summary>
    /// Configures the preset coding parameters the encoder should use.
    /// If not set the encoder will use the default preset coding parameters as defined by the JPEG-LS standard.
    /// Only when the coding parameters are different then the default parameters, they will be written to the
    /// JPEG-LS stream during the encode phase.
    /// </summary>
    /// <param name="preset_coding_parameters">Reference to the preset coding parameters.</param>
    jpegls_encoder& preset_coding_parameters(const jpegls_pc_parameters& preset_coding_parameters)
    {
        check_jpegls_errc(charls_jpegls_encoder_set_preset_coding_parameters(encoder_.get(), &preset_coding_parameters));
        return *this;
    }

    /// <summary>
    /// Configures the HP color transformation the encoder should use.
    /// If not set the encoder will use no color transformation.
    /// Color transformations are a HP extension and not defined by the JPEG-LS standard
    /// and can only be set for 3 component encodings.
    /// </summary>
    /// <param name="color_transformation">The color transformation parameters.</param>
    jpegls_encoder& color_transformation(const color_transformation color_transformation)
    {
        check_jpegls_errc(charls_jpegls_encoder_set_color_transformation(encoder_.get(), color_transformation));
        return *this;
    }

    /// <summary>
    /// Returns the size in bytes, that the encoder expects are needed to hold the encoded image.
    /// </summary>
    /// <remarks>
    /// Size for dynamic extras like SPIFF entries and other tables are not included in this size.
    /// </remarks>
    /// <returns>The estimated size in bytes needed to hold the encoded image.</returns>
    CHARLS_CHECK_RETURN size_t estimated_destination_size() const
    {
        size_t size_in_bytes;
        check_jpegls_errc(charls_jpegls_encoder_get_estimated_destination_size(encoder_.get(), &size_in_bytes));
        return size_in_bytes;
    }

    /// <summary>
    /// Set the reference to the destination buffer that will contain the encoded JPEG-LS byte stream data after encoding.
    /// This buffer needs to remain valid during the encoding process.
    /// </summary>
    /// <param name="destination_buffer">Reference to the start of the destination buffer.</param>
    /// <param name="destination_size_bytes">Size of the destination buffer in bytes.</param>
    CHARLS_ATTRIBUTE_ACCESS((access(write_only, 2, 3)))
    jpegls_encoder& destination(CHARLS_OUT_WRITES_BYTES(destination_size_bytes) void* destination_buffer,
                                const size_t destination_size_bytes)
    {
        check_jpegls_errc(
            charls_jpegls_encoder_set_destination_buffer(encoder_.get(), destination_buffer, destination_size_bytes));
        return *this;
    }

    /// <summary>
    /// Set the the container that will contain the encoded JPEG-LS byte stream data after encoding.
    /// This container needs to remain valid during the encoding process.
    /// </summary>
    /// <param name="destination_container">
    /// The STL like container, that supports the functions data() and size() and the typedef value_type.
    /// </param>
    template<typename Container, typename T = typename Container::value_type>
    jpegls_encoder& destination(Container& destination_container)
    {
        return destination(destination_container.data(),
                           destination_container.size() * sizeof(typename Container::value_type));
    }

    template<typename Container, typename T = typename Container::value_type>
    jpegls_encoder& destination(const Container& destination_container) = delete;

    /// <summary>
    /// Writes a standard SPIFF header to the destination. The additional values are computed from the current encoder
    /// settings.
    /// </summary>
    /// <param name="color_space">The color space of the image.</param>
    /// <param name="resolution_units">The resolution units of the next 2 parameters.</param>
    /// <param name="vertical_resolution">The vertical resolution.</param>
    /// <param name="horizontal_resolution">The horizontal resolution.</param>
    jpegls_encoder&
    write_standard_spiff_header(const spiff_color_space color_space,
                                const spiff_resolution_units resolution_units = spiff_resolution_units::aspect_ratio,
                                const uint32_t vertical_resolution = 1, const uint32_t horizontal_resolution = 1)
    {
        check_jpegls_errc(charls_jpegls_encoder_write_standard_spiff_header(encoder_.get(), color_space, resolution_units,
                                                                            vertical_resolution, horizontal_resolution));
        return *this;
    }

    /// <summary>
    /// Writes a SPIFF header to the destination.
    /// </summary>
    /// <param name="header">Reference to a SPIFF header that will be written to the destination.</param>
    jpegls_encoder& write_spiff_header(const spiff_header& header)
    {
        check_jpegls_errc(charls_jpegls_encoder_write_spiff_header(encoder_.get(), &header));
        return *this;
    }

    /// <summary>
    /// Writes a SPIFF directory entry to the destination.
    /// </summary>
    /// <param name="entry_tag">The entry tag of the directory entry.</param>
    /// <param name="entry_data">The entry data of the directory entry.</param>
    /// <param name="entry_data_size_bytes">The size in bytes of the directory entry [0-65528].</param>
    template<typename IntDerivedType>
    CHARLS_ATTRIBUTE_ACCESS((access(read_only, 3, 4)))
    jpegls_encoder& write_spiff_entry(const IntDerivedType entry_tag,
                                      CHARLS_IN_READS_BYTES(entry_data_size_bytes) const void* entry_data,
                                      const size_t entry_data_size_bytes)
    {
        check_jpegls_errc(charls_jpegls_encoder_write_spiff_entry(encoder_.get(), static_cast<uint32_t>(entry_tag),
                                                                  entry_data, entry_data_size_bytes));
        return *this;
    }

    /// <summary>
    /// Writes a SPIFF end of directory entry to the destination.
    /// The encoder will normally does this automatically. It is made available
    /// for the scenario to create SPIFF headers in front of existing JPEG-LS streams.
    /// </summary>
    /// <remarks>
    /// The end of directory also includes a SOI marker. This marker should be skipped from the JPEG-LS stream.
    /// </remarks>
    jpegls_encoder& write_spiff_end_of_directory_entry()
    {
        check_jpegls_errc(charls_jpegls_encoder_write_spiff_end_of_directory_entry(encoder_.get()));
        return *this;
    }

    /// <summary>
    /// Writes a string as JPEG comment to the JPEG-LS bit stream.
    /// </summary>
    /// <remarks>The null terminator is also written to the output destination, if the string is not empty.</remarks>
    /// <param name="comment">
    /// The text of the comment as null terminated string.
    /// Text encoding is application specific and not defined by the JPEG-LS standard.
    /// </param>
    jpegls_encoder& write_comment(CHARLS_IN_Z const char* comment)
    {
        const size_t size{std::strlen(comment)};
        return write_comment(comment, size == 0 ? 0 : size + 1);
    }

    /// <summary>
    /// Writes a JPEG comment to the JPEG-LS bit stream.
    /// </summary>
    /// <param name="comment">The bytes of the comment: application specific.</param>
    /// <param name="size">The size of the comment in bytes.</param>
    CHARLS_ATTRIBUTE_ACCESS((access(read_only, 2, 3)))
    jpegls_encoder& write_comment(CHARLS_IN_READS_BYTES(size) const void* comment, const size_t size)
    {
        check_jpegls_errc(charls_jpegls_encoder_write_comment(encoder_.get(), comment, size));
        return *this;
    }

    /// <summary>
    /// Writes a JPEG application data segment to the JPEG-LS bit stream.
    /// </summary>
    /// <param name="application_data_id">The ID of the application data segment.</param>
    /// <param name="application_data">The bytes of the application data: application specific.</param>
    /// <param name="size">The size of the comment in bytes.</param>
    CHARLS_ATTRIBUTE_ACCESS((access(read_only, 3, 4)))
    jpegls_encoder& write_application_data(const int32_t application_data_id,
                                           CHARLS_IN_READS_BYTES(size) const void* application_data, const size_t size)
    {
        check_jpegls_errc(
            charls_jpegls_encoder_write_application_data(encoder_.get(), application_data_id, application_data, size));
        return *this;
    }

    /// <summary>
    /// Encodes the passed buffer with the source image data to the destination.
    /// </summary>
    /// <param name="source_buffer">Byte array that holds the image data that needs to be encoded.</param>
    /// <param name="source_size_bytes">Length of the array in bytes.</param>
    /// <param name="stride">
    /// The number of bytes from one row of pixels in memory to the next row of pixels in memory.
    /// Stride is sometimes called pitch. If padding bytes are present, the stride is wider than the width of the image.
    /// </param>
    /// <returns>The number of bytes written to the destination.</returns>
    CHARLS_ATTRIBUTE_ACCESS((access(read_only, 2, 3)))
    size_t encode(CHARLS_IN_READS_BYTES(source_size_bytes) const void* source_buffer, const size_t source_size_bytes,
                  const uint32_t stride = 0) const
    {
        check_jpegls_errc(
            charls_jpegls_encoder_encode_from_buffer(encoder_.get(), source_buffer, source_size_bytes, stride));
        return bytes_written();
    }

    /// <summary>
    /// Encodes the passed STL like container with the source image data to the destination.
    /// </summary>
    /// <param name="source_container">Container that holds the image data that needs to be encoded.</param>
    /// <param name="stride">
    /// The number of bytes from one row of pixels in memory to the next row of pixels in memory.
    /// Stride is sometimes called pitch. If padding bytes are present, the stride is wider than the width of the image.
    /// </param>
    /// <returns>The number of bytes written to the destination.</returns>
    template<typename Container, typename T = typename Container::value_type>
    size_t encode(const Container& source_container, const uint32_t stride = 0) const
    {
        return encode(source_container.data(), source_container.size() * sizeof(typename Container::value_type), stride);
    }

    /// <summary>
    /// Returns the size in bytes, that are written to the destination.
    /// </summary>
    /// <returns>The bytes written.</returns>
    CHARLS_CHECK_RETURN size_t bytes_written() const
    {
        size_t bytes_written;
        check_jpegls_errc(charls_jpegls_encoder_get_bytes_written(encoder_.get(), &bytes_written));
        return bytes_written;
    }

    /// <summary>
    /// Resets the write position of the destination buffer to the beginning.
    /// </summary>
    void rewind() const
    {
        check_jpegls_errc(charls_jpegls_encoder_rewind(encoder_.get()));
    }

private:
    CHARLS_CHECK_RETURN static charls_jpegls_encoder* create_encoder()
    {
        charls_jpegls_encoder* encoder{charls_jpegls_encoder_create()};
        if (!encoder)
            throw std::bad_alloc();

        return encoder;
    }

    static void destroy_encoder(CHARLS_IN_OPT const charls_jpegls_encoder* encoder) noexcept
    {
        charls_jpegls_encoder_destroy(encoder);
    }

    std::unique_ptr<charls_jpegls_encoder, void (*)(const charls_jpegls_encoder*)> encoder_{create_encoder(),
                                                                                            &destroy_encoder};
};

} // namespace charls

#endif
