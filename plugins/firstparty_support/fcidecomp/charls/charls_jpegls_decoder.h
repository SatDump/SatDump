// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "jpegls_error.h"
#include "validate_spiff_header.h"

#ifdef __cplusplus
#include <functional>
#include <memory>
#include <utility>
#else
#include <stddef.h>
#endif


#ifdef __cplusplus
struct charls_jpegls_decoder;
extern "C" {
#else
typedef struct charls_jpegls_decoder charls_jpegls_decoder;
#endif

// The following functions define the public C API of the CharLS library.
// The C++ API is defined after the C API.

/// <summary>
/// Creates a JPEG-LS decoder instance, when finished with the instance destroy it with the function
/// charls_jpegls_decoder_destroy.
/// </summary>
/// <returns>A reference to a new created decoder instance, or a null pointer when the creation fails.</returns>
CHARLS_CHECK_RETURN CHARLS_RET_MAY_BE_NULL CHARLS_API_IMPORT_EXPORT charls_jpegls_decoder*
    CHARLS_API_CALLING_CONVENTION charls_jpegls_decoder_create(CHARLS_C_VOID) CHARLS_NOEXCEPT;

/// <summary>
/// Destroys a JPEG-LS decoder instance created with charls_jpegls_decoder_create and releases all internal resources
/// attached to it.
/// </summary>
/// <param name="decoder">Instance to destroy. If a null pointer is passed as argument, no action occurs.</param>
CHARLS_API_IMPORT_EXPORT void CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_destroy(CHARLS_IN_OPT const charls_jpegls_decoder* decoder) CHARLS_NOEXCEPT;

/// <summary>
/// Set the reference to a source buffer that contains the encoded JPEG-LS byte stream data.
/// This buffer needs to remain valid until the buffer is fully decoded.
/// </summary>
/// <param name="decoder">Reference to the decoder instance.</param>
/// <param name="source_buffer">Reference to the start of the source buffer.</param>
/// <param name="source_size_bytes">Size of the source buffer in bytes.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_ATTRIBUTE_ACCESS((access(read_only, 2, 3)))
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_set_source_buffer(CHARLS_IN charls_jpegls_decoder* decoder,
                                        CHARLS_IN_READS_BYTES(source_size_bytes) const void* source_buffer,
                                        size_t source_size_bytes) CHARLS_NOEXCEPT CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Tries to read the SPIFF header from the source buffer.
/// If a SPIFF header exists its content will be put into the spiff_header parameter and header_found will be set to 1.
/// Call charls_jpegls_decoder_read_header to read the normal JPEG header afterwards.
/// </summary>
/// <param name="decoder">Reference to the decoder instance.</param>
/// <param name="spiff_header">Output argument, will hold the SPIFF header when one could be found.</param>
/// <param name="header_found">Output argument, will hold 1 if a SPIFF header could be found, otherwise 0.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_read_spiff_header(CHARLS_IN charls_jpegls_decoder* decoder,
                                        CHARLS_OUT charls_spiff_header* spiff_header,
                                        CHARLS_OUT int32_t* header_found) CHARLS_NOEXCEPT CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Reads the JPEG-LS header from the JPEG byte stream. After this function is called frame info can be retrieved.
/// </summary>
/// <param name="decoder">Reference to the decoder instance.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_read_header(CHARLS_IN charls_jpegls_decoder* decoder) CHARLS_NOEXCEPT CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Returns information about the frame stored in the JPEG-LS byte stream.
/// </summary>
/// <remarks>
/// Function should be called after calling the function charls_jpegls_decoder_read_header.
/// </remarks>
/// <param name="decoder">Reference to the decoder instance.</param>
/// <param name="frame_info">Output argument, will hold the frame info when the function returns.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_get_frame_info(CHARLS_IN const charls_jpegls_decoder* decoder,
                                     CHARLS_OUT charls_frame_info* frame_info) CHARLS_NOEXCEPT CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Returns the NEAR parameter that was used to encode the scan. A value of 0 means lossless.
/// </summary>
/// <remarks>
/// Function should be called after calling the function charls_jpegls_decoder_read_header.
/// </remarks>
/// <param name="decoder">Reference to the decoder instance.</param>
/// <param name="component">The component index for which the NEAR parameter should be retrieved.</param>
/// <param name="near_lossless">Reference that will hold the value of the NEAR parameter.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_get_near_lossless(CHARLS_IN const charls_jpegls_decoder* decoder, int32_t component,
                                        CHARLS_OUT int32_t* near_lossless) CHARLS_NOEXCEPT CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Returns the interleave mode that was used to encode the scan.
/// </summary>
/// <remarks>
/// Function should be called after calling the function charls_jpegls_decoder_read_header.
/// </remarks>
/// <param name="decoder">Reference to the decoder instance.</param>
/// <param name="interleave_mode">Reference that will hold the value of the interleave mode.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_get_interleave_mode(CHARLS_IN const charls_jpegls_decoder* decoder,
                                          CHARLS_OUT charls_interleave_mode* interleave_mode) CHARLS_NOEXCEPT
    CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Returns the preset coding parameters used to encode the first scan.
/// </summary>
/// <remarks>
/// Function should be called after calling the function charls_jpegls_decoder_read_header.
/// </remarks>
/// <param name="decoder">Reference to the decoder instance.</param>
/// <param name="reserved">Reserved. Should be set to 0.</param>
/// <param name="preset_coding_parameters">Reference that will hold the values preset coding parameters.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_get_preset_coding_parameters(CHARLS_IN const charls_jpegls_decoder* decoder, int32_t reserved,
                                                   CHARLS_OUT charls_jpegls_pc_parameters* preset_coding_parameters)
    CHARLS_NOEXCEPT CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Returns the color transformation that was used to encode the scan.
/// </summary>
/// <remarks>
/// Function should be called after calling the function charls_jpegls_decoder_read_header.
/// </remarks>
/// <param name="decoder">Reference to the decoder instance.</param>
/// <param name="color_transformation">Reference that will hold the value of the color transformation.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_get_color_transformation(CHARLS_IN const charls_jpegls_decoder* decoder,
                                               CHARLS_OUT charls_color_transformation* color_transformation) CHARLS_NOEXCEPT
    CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Returns the size required for the destination buffer in bytes to hold the decoded pixel data.
/// </summary>
/// <remarks>
/// Function should be called after calling the function charls_jpegls_decoder_read_header.
/// </remarks>
/// <param name="decoder">Reference to the decoder instance.</param>
/// <param name="stride">Number of bytes to the next line in the buffer, when zero, decoder will compute it.</param>
/// <param name="destination_size_bytes">Output argument, will hold the required size when the function returns.</param>
/// <returns>The result of the operation: success or a failure code.</returns>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_get_destination_size(CHARLS_IN const charls_jpegls_decoder* decoder, uint32_t stride,
                                           CHARLS_OUT size_t* destination_size_bytes) CHARLS_NOEXCEPT
    CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Will decode the JPEG-LS byte stream from the source buffer into the destination buffer.
/// </summary>
/// <remarks>
/// Function should be called after calling the function charls_jpegls_decoder_read_header.
/// </remarks>
/// <param name="decoder">Reference to the decoder instance.</param>
/// <param name="destination_buffer">Byte array that holds the encoded bytes when the function returns.</param>
/// <param name="destination_size_bytes">
/// Length of the array in bytes. If the array is too small the function will return an error.
/// </param>
/// <param name="stride">
/// Number of bytes to the next line in the buffer, when zero, decoder will compute it.
/// </param> <returns>The result of the operation: success or a failure code.</returns>
CHARLS_ATTRIBUTE_ACCESS((access(write_only, 2, 3)))
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_decode_to_buffer(CHARLS_IN charls_jpegls_decoder* decoder,
                                       CHARLS_OUT_WRITES_BYTES(destination_size_bytes) void* destination_buffer,
                                       size_t destination_size_bytes, uint32_t stride) CHARLS_NOEXCEPT
    CHARLS_ATTRIBUTE((nonnull));

/// <summary>
/// Will install a function that will be called when a comment (COM) segment is found.
/// </summary>
/// <remarks>
/// Pass NULL or nullptr to uninstall the callback function.
/// The callback should return 0 if there are no errors.
/// It can return a non-zero value to abort decoding with a callback_failed error code.
/// </remarks>
/// <param name="decoder">Reference to the decoder instance.</param>
/// <param name="handler">Function pointer to the callback function.</param>
/// <param name="user_context">Free to use context data that will be provided to the callback function.</param>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_at_comment(CHARLS_IN charls_jpegls_decoder* decoder, charls_at_comment_handler handler,
                                 void* user_context) CHARLS_NOEXCEPT CHARLS_ATTRIBUTE((nonnull(1)));


/// <summary>
/// Will install a function that will be called when an application data (APPn) segment is found.
/// </summary>
/// <remarks>
/// Pass NULL or nullptr to uninstall the callback function.
/// The callback should return 0 if there are no errors.
/// It can return a non-zero value to abort decoding with a callback_failed error code.
/// </remarks>
/// <param name="decoder">Reference to the decoder instance.</param>
/// <param name="handler">Function pointer to the callback function.</param>
/// <param name="user_context">Free to use context data that will be provided to the callback function.</param>
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT charls_jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_decoder_at_application_data(CHARLS_IN charls_jpegls_decoder* decoder,
                                          charls_at_application_data_handler handler, void* user_context) CHARLS_NOEXCEPT
    CHARLS_ATTRIBUTE((nonnull(1)));


// Note: The 3 methods below are considered obsolete and will be removed in the next major update.

/// <summary>
/// Retrieves the JPEG-LS header. This info can be used to pre-allocate the uncompressed output buffer.
/// </summary>
/// <remarks>This method will be removed in the next major update.</remarks>
/// <param name="source">Byte array that holds the JPEG-LS encoded data of which the header should be extracted.</param>
/// <param name="source_length">Length of the array in bytes.</param>
/// <param name="params">Parameter object that describes how the pixel data is encoded.</param>
/// <param name="error_message">
/// Character array of at least 256 characters or NULL. Hold the error message when a failure occurs, empty otherwise.
/// </param>
CHARLS_ATTRIBUTE_ACCESS((access(read_only, 1, 2)))
CHARLS_DEPRECATED CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT CharlsApiResultType CHARLS_API_CALLING_CONVENTION
JpegLsReadHeader(CHARLS_IN_READS_BYTES(source_length) const void* source, size_t source_length,
                 CHARLS_OUT JlsParameters* params, CHARLS_OUT_OPT char* error_message) CHARLS_NOEXCEPT
    CHARLS_ATTRIBUTE((nonnull(1, 3)));

/// <summary>
/// Decodes a JPEG-LS encoded byte array to uncompressed pixel data byte array.
/// </summary>
/// <remarks>This method will be removed in the next major update.</remarks>
/// <param name="destination">Byte array that holds the uncompressed pixel data bytes when the function returns.</param>
/// <param name="destination_length">
/// Length of the array in bytes. If the array is too small the function will return an error.
/// </param>
/// <param name="source">Byte array that holds the JPEG-LS encoded data that should be decoded.</param>
/// <param name="source_length">Length of the array in bytes.</param>
/// <param name="params">Parameter object that describes the pixel data and how to decode it.</param>
/// <param name="error_message">
/// Character array of at least 256 characters or NULL. Hold the error message when a failure occurs, empty otherwise.
/// </param>
CHARLS_ATTRIBUTE_ACCESS((access(write_only, 1, 2), access(read_only, 3, 4)))
CHARLS_DEPRECATED CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT CharlsApiResultType CHARLS_API_CALLING_CONVENTION
JpegLsDecode(CHARLS_OUT_WRITES_BYTES(destination_length) void* destination, size_t destination_length,
             CHARLS_IN_READS_BYTES(source_length) const void* source, size_t source_length,
             CHARLS_IN_OPT const JlsParameters* params, CHARLS_OUT_OPT char* error_message) CHARLS_NOEXCEPT
    CHARLS_ATTRIBUTE((nonnull(1, 3)));

/// <remarks>This method will be removed in the next major update.</remarks>
CHARLS_ATTRIBUTE_ACCESS((access(write_only, 1, 2), access(read_only, 3, 4)))
CHARLS_DEPRECATED
CHARLS_CHECK_RETURN CHARLS_API_IMPORT_EXPORT CharlsApiResultType CHARLS_API_CALLING_CONVENTION
JpegLsDecodeRect(CHARLS_OUT_WRITES_BYTES(destination_length) void* destination, size_t destination_length,
                 CHARLS_IN_READS_BYTES(source_length) const void* source, size_t source_length, JlsRect roi,
                 CHARLS_IN_OPT const JlsParameters* params, CHARLS_OUT_OPT char* error_message) CHARLS_NOEXCEPT
    CHARLS_ATTRIBUTE((nonnull(1, 3)));

#ifdef __cplusplus

} // extern "C"

namespace charls {

/// <summary>
/// JPEG-LS decoder class that encapsulates the C ABI interface calls and provide a native C++ interface.
/// </summary>
class jpegls_decoder final
{
public:
    /// <summary>
    /// Decodes a JPEG-LS buffer in 1 simple operation.
    /// </summary>
    /// <param name="source">Source container with the JPEG-LS encoded bytes.</param>
    /// <param name="destination">
    /// Destination container that will hold the image data on return. Container will be resized automatically.
    /// </param>
    /// <param name="maximum_size_in_bytes">
    /// The maximum output size that may be allocated, default is 94 MiB (enough to decode 8 bit color 8K image).
    /// </param>
    /// <exception cref="charls::jpegls_error">An error occurred during the operation.</exception>
    /// <exception cref="std::bad_alloc">Thrown when memory for the decoder could not be allocated.</exception>
    /// <returns>Frame info of the decoded image and the interleave mode.</returns>
    template<typename SourceContainer, typename DestinationContainer, typename T1 = typename SourceContainer::value_type,
             typename T2 = typename DestinationContainer::value_type>
    static std::pair<charls::frame_info, charls::interleave_mode>
    decode(const SourceContainer& source, DestinationContainer& destination,
           const size_t maximum_size_in_bytes = size_t{7680} * 4320 * 3)
    {
        jpegls_decoder decoder{source, true};

        const size_t destination_size{decoder.destination_size()};
        if (destination_size > maximum_size_in_bytes)
            impl::throw_jpegls_error(jpegls_errc::not_enough_memory);

        destination.resize(destination_size / sizeof(typename DestinationContainer::value_type));
        decoder.decode(destination);

        return std::make_pair(decoder.frame_info(), decoder.interleave_mode());
    }

    jpegls_decoder() = default;

    /// <summary>
    /// Constructs a jpegls_decoder instance.
    /// The passed container needs to remain valid until the stream is fully decoded.
    /// </summary>
    /// <param name="source_buffer">Reference to the start of the source buffer.</param>
    /// <param name="source_size_bytes">Size of the source buffer in bytes.</param>
    /// <param name="parse_header">
    /// If true the SPIFF and JPEG header will be directly read from the source.
    /// </param>
    /// <exception cref="charls::jpegls_error">An error occurred during the operation.</exception>
    /// <exception cref="std::bad_alloc">Thrown when memory for the decoder could not be allocated.</exception>
    jpegls_decoder(CHARLS_IN_READS_BYTES(source_size_bytes) const void* source_buffer, const size_t source_size_bytes,
                   const bool parse_header = true)
    {
        source(source_buffer, source_size_bytes);
        if (parse_header)
        {
            read_spiff_header();
            read_header();
        }
    }

    /// <summary>
    /// Constructs a jpegls_decoder instance.
    /// The passed container needs to remain valid until the stream is fully decoded.
    /// </summary>
    /// <param name="source_container">
    /// A STL like container that provides the functions data() and size() and the type value_type.
    /// </param>
    /// <param name="parse_header">
    /// If true the SPIFF and JPEG header will be directly read from the source.
    /// </param>
    /// <exception cref="charls::jpegls_error">An error occurred during the operation.</exception>
    /// <exception cref="std::bad_alloc">Thrown when memory for the decoder could not be allocated.</exception>
    template<typename Container, typename T = typename Container::value_type>
    jpegls_decoder(const Container& source_container, const bool parse_header) :
        jpegls_decoder(source_container.data(), source_container.size() * sizeof(typename Container::value_type),
                       parse_header)
    {
    }

    /// <summary>
    /// Set the reference to a source buffer that contains the encoded JPEG-LS byte stream data.
    /// This buffer needs to remain valid until the stream is fully decoded.
    /// </summary>
    /// <param name="source_buffer">Reference to the start of the source buffer.</param>
    /// <param name="source_size_bytes">Size of the source buffer in bytes.</param>
    /// <exception cref="charls::jpegls_error">An error occurred during the operation.</exception>
    CHARLS_ATTRIBUTE_ACCESS((access(read_only, 2, 3)))
    jpegls_decoder& source(CHARLS_IN_READS_BYTES(source_size_bytes) const void* source_buffer,
                           const size_t source_size_bytes)
    {
        check_jpegls_errc(charls_jpegls_decoder_set_source_buffer(decoder_.get(), source_buffer, source_size_bytes));
        return *this;
    }

    /// <summary>
    /// Set the reference to a source container that contains the encoded JPEG-LS byte stream data.
    /// This container needs to remain valid until the stream is fully decoded.
    /// </summary>
    /// <param name="source_container">
    /// A STL like container that provides the functions data() and size() and the type value_type.
    /// </param>
    /// <exception cref="charls::jpegls_error">An error occurred during the operation.</exception>
    template<typename Container, typename T = typename Container::value_type>
    jpegls_decoder& source(const Container& source_container)
    {
        return source(source_container.data(), source_container.size() * sizeof(typename Container::value_type));
    }

    /// <summary>
    /// Tries to read the SPIFF header from the JPEG-LS stream.
    /// If a SPIFF header exists its will be returned otherwise the struct will be filled with default values.
    /// The header_found parameter will be set to true if the spiff header could be read.
    /// Call read_header to read the normal JPEG header afterwards.
    /// </summary>
    /// <exception cref="charls::jpegls_error">An error occurred during the operation.</exception>
    /// <returns>True if a valid SPIFF header could be found.</returns>
    bool read_spiff_header()
    {
        std::error_code ec;
        read_spiff_header(ec);
        check_jpegls_errc(static_cast<jpegls_errc>(ec.value()));
        return spiff_header_has_value_;
    }

    /// <summary>
    /// Tries to read the SPIFF header from the JPEG-LS stream.
    /// If a SPIFF header exists its will be returned otherwise the struct will be filled with default values.
    /// The header_found parameter will be set to true if the spiff header could be read.
    /// Call read_header to read the normal JPEG header afterwards.
    /// </summary>
    /// <param name="ec">The out-parameter for error reporting.</param>
    /// <returns>True if a valid SPIFF header could be found.</returns>
    bool read_spiff_header(std::error_code& ec) noexcept
    {
        int32_t found;
        ec = charls_jpegls_decoder_read_spiff_header(decoder_.get(), &spiff_header_, &found);
        spiff_header_has_value_ = found != 0;
        return spiff_header_has_value_;
    }

    /// <summary>
    /// Reads the JPEG-LS header from the beginning of the JPEG-LS byte stream or after the SPIFF header.
    /// After this function is called frame info and other info can be retrieved.
    /// </summary>
    /// <exception cref="charls::jpegls_error">An error occurred during the operation.</exception>
    jpegls_decoder& read_header()
    {
        std::error_code ec;
        read_header(ec);
        check_jpegls_errc(static_cast<jpegls_errc>(ec.value()));
        return *this;
    }

    /// <summary>
    /// Reads the JPEG-LS header from the beginning of the JPEG-LS byte stream or after the SPIFF header.
    /// After this function is called frame info and other info can be retrieved.
    /// If a SPIFF header is present it will be validated against the information in the frame info.
    /// </summary>
    /// <param name="ec">The out-parameter for error reporting.</param>
    jpegls_decoder& read_header(CHARLS_OUT std::error_code& ec) noexcept
    {
        ec = charls_jpegls_decoder_read_header(decoder_.get());
        if (ec == jpegls_errc::success)
        {
            ec = charls_jpegls_decoder_get_frame_info(decoder_.get(), &frame_info_);
            if (ec == jpegls_errc::success && spiff_header_has_value_)
            {
                ec = charls_validate_spiff_header(&spiff_header_, &frame_info_);
            }
        }
        return *this;
    }

    /// <summary>
    /// Returns true if a valid SPIFF header was found.
    /// </summary>
    /// <returns>True of false, depending if a SPIFF header was found.</returns>
    CHARLS_CHECK_RETURN bool spiff_header_has_value() const noexcept
    {
        return spiff_header_has_value_;
    }

    /// <summary>
    /// Returns the SPIFF header, if read and found.
    /// Function can be called after read_spiff_header and spiff_header_has_value.
    /// </summary>
    /// <returns>The SPIFF header.</returns>
    CHARLS_CHECK_RETURN const charls::spiff_header& spiff_header() const& noexcept
    {
        return spiff_header_;
    }

    /// <summary>
    /// Returns the SPIFF header, if read and found.
    /// Function can be called after read_spiff_header and spiff_header_has_value.
    /// </summary>
    /// <returns>The SPIFF header.</returns>
    charls::spiff_header spiff_header() const&& noexcept  // Note: CHARLS_CHECK_RETURN causes false C6031 warnings [Visual Studio 2019 v16.11.19]
    {
        return spiff_header_;
    }

    /// <summary>
    /// Returns information about the frame stored in the JPEG-LS byte stream.
    /// Function can be called after read_header.
    /// </summary>
    /// <returns>The frame info that describes the image stored in the JPEG-LS byte stream.</returns>
    CHARLS_CHECK_RETURN const charls::frame_info& frame_info() const& noexcept
    {
        return frame_info_;
    }

    /// <summary>
    /// Returns information about the frame stored in the JPEG-LS byte stream.
    /// Function can be called after read_header.
    /// </summary>
    /// <returns>The frame info that describes the image stored in the JPEG-LS byte stream.</returns>
    charls::frame_info frame_info() const&& noexcept // Note: CHARLS_CHECK_RETURN causes false C6031 warnings [Visual Studio 2019 v16.11.19]
    {
        return frame_info_;
    }

    /// <summary>
    /// Returns the NEAR parameter that was used to encode the scan. A value of 0 means lossless.
    /// </summary>
    /// <param name="component">The component index for which the NEAR parameter should be retrieved.</param>
    /// <exception cref="charls::jpegls_error">An error occurred during the operation.</exception>
    /// <returns>The value of the NEAR parameter.</returns>
    CHARLS_CHECK_RETURN int32_t near_lossless(const int32_t component = 0) const
    {
        int32_t near_lossless;
        check_jpegls_errc(charls_jpegls_decoder_get_near_lossless(decoder_.get(), component, &near_lossless));
        return near_lossless;
    }

    /// <summary>
    /// Returns the interleave mode that was used to encode the scan.
    /// </summary>
    /// <exception cref="charls::jpegls_error">An error occurred during the operation.</exception>
    /// <returns>The value of the interleave mode.</returns>
    CHARLS_CHECK_RETURN charls::interleave_mode interleave_mode() const
    {
        charls::interleave_mode interleave_mode;
        check_jpegls_errc(charls_jpegls_decoder_get_interleave_mode(decoder_.get(), &interleave_mode));
        return interleave_mode;
    }

    /// <summary>
    /// Returns the preset coding parameters used to encode the first scan.
    /// </summary>
    /// <exception cref="charls::jpegls_error">An error occurred during the operation.</exception>
    /// <returns>The values of the JPEG-LS preset coding parameters.</returns>
    CHARLS_CHECK_RETURN jpegls_pc_parameters preset_coding_parameters() const
    {
        jpegls_pc_parameters preset_coding_parameters;
        check_jpegls_errc(charls_jpegls_decoder_get_preset_coding_parameters(decoder_.get(), 0, &preset_coding_parameters));
        return preset_coding_parameters;
    }

    /// <summary>
    /// Returns the HP color transformation that was used to encode the scan.
    /// </summary>
    /// <exception cref="charls::jpegls_error">An error occurred during the operation.</exception>
    /// <returns>The value of the color transformation.</returns>
    CHARLS_CHECK_RETURN charls::color_transformation color_transformation() const
    {
        charls::color_transformation color_transformation;
        check_jpegls_errc(charls_jpegls_decoder_get_color_transformation(decoder_.get(), &color_transformation));
        return color_transformation;
    }

    /// <summary>
    /// Returns the size required for the destination buffer in bytes to hold the decoded pixel data.
    /// Function can be called after read_header.
    /// </summary>
    /// <param name="stride">Number of bytes to the next line in the buffer, when zero, decoder will compute it.</param>
    /// <exception cref="charls::jpegls_error">An error occurred during the operation.</exception>
    /// <returns>The required size in bytes of the destination buffer.</returns>
    CHARLS_CHECK_RETURN size_t destination_size(const uint32_t stride = 0) const
    {
        size_t size_in_bytes;
        check_jpegls_errc(charls_jpegls_decoder_get_destination_size(decoder_.get(), stride, &size_in_bytes));
        return size_in_bytes;
    }

    /// <summary>
    /// Will decode the JPEG-LS byte stream set with source into the destination buffer.
    /// </summary>
    /// <param name="destination_buffer">Byte array that holds the encoded bytes when the function returns.</param>
    /// <param name="destination_size_bytes">
    /// Length of the array in bytes. If the array is too small the function will return an error.
    /// </param>
    /// <param name="stride">Number of bytes to the next line in the buffer, when zero, decoder will compute it.</param>
    /// <exception cref="charls::jpegls_error">An error occurred during the operation.</exception>
    CHARLS_ATTRIBUTE_ACCESS((access(write_only, 2, 3)))
    void decode(CHARLS_OUT_WRITES_BYTES(destination_size_bytes) void* destination_buffer,
                const size_t destination_size_bytes, const uint32_t stride = 0) const
    {
        check_jpegls_errc(
            charls_jpegls_decoder_decode_to_buffer(decoder_.get(), destination_buffer, destination_size_bytes, stride));
    }

    /// <summary>
    /// Will decode the JPEG-LS byte stream set with source into the destination container.
    /// </summary>
    /// <param name="destination_container">
    /// A STL like container that provides the functions data() and size() and the type value_type.
    /// </param>
    /// <param name="stride">Number of bytes to the next line in the buffer, when zero, decoder will compute it.</param>
    /// <exception cref="charls::jpegls_error">An error occurred during the operation.</exception>
    template<typename Container, typename T = typename Container::value_type>
    void decode(CHARLS_OUT Container& destination_container, const uint32_t stride = 0) const
    {
        decode(destination_container.data(), destination_container.size() * sizeof(typename Container::value_type), stride);
    }

    /// <summary>
    /// Will decode the JPEG-LS byte stream set with source and return a container with the decoded data.
    /// </summary>
    /// <param name="stride">Number of bytes to the next line in the buffer, when zero, decoder will compute it.</param>
    /// <exception cref="charls::jpegls_error">An error occurred during the operation.</exception>
    /// <returns>Container with the decoded data.</returns>
    template<typename Container, typename T = typename Container::value_type>
    CHARLS_CHECK_RETURN Container decode(const uint32_t stride = 0) const
    {
        Container destination(destination_size() / sizeof(typename Container::value_type));

        decode(destination.data(), destination.size() * sizeof(typename Container::value_type), stride);
        return destination;
    }

    /// <summary>
    /// Will install a function that will be called when a comment (COM) segment is found.
    /// </summary>
    /// <remarks>
    /// Pass a nullptr to uninstall the callback function.
    /// The callback can throw an exception to abort the decoding process.
    /// This abort will be returned as a callback_failed error code.
    /// </remarks>
    /// <param name="comment_handler">Function object to the comment handler.</param>
    /// <exception cref="charls::jpegls_error">An error occurred during the operation.</exception>
    jpegls_decoder& at_comment(std::function<void(const void* data, size_t size)> comment_handler)
    {
        comment_handler_ = std::move(comment_handler);
        check_jpegls_errc(
            charls_jpegls_decoder_at_comment(decoder_.get(), comment_handler_ ? &at_comment_callback : nullptr, this));
        return *this;
    }

    /// <summary>
    /// Will install a function that will be called when an application data (APPn) segment is found.
    /// </summary>
    /// <remarks>
    /// Pass a nullptr to uninstall the callback function.
    /// The callback can throw an exception to abort the decoding process.
    /// This abort will be returned as a callback_failed error code.
    /// </remarks>
    /// <param name="application_data_handler">Function object to the application data handler.</param>
    /// <exception cref="charls::jpegls_error">An error occurred during the operation.</exception>
    jpegls_decoder& at_application_data(
        std::function<void(int32_t application_data_id, const void* data, size_t size)> application_data_handler)
    {
        application_data_handler_ = std::move(application_data_handler);
        check_jpegls_errc(charls_jpegls_decoder_at_application_data(
            decoder_.get(), application_data_handler_ ? &at_application_data_callback : nullptr, this));
        return *this;
    }

private:
    CHARLS_CHECK_RETURN static charls_jpegls_decoder* create_decoder()
    {
        charls_jpegls_decoder* decoder{charls_jpegls_decoder_create()};
        if (!decoder)
            throw std::bad_alloc();

        return decoder;
    }

    static void destroy_decoder(CHARLS_IN_OPT const charls_jpegls_decoder* decoder) noexcept
    {
        charls_jpegls_decoder_destroy(decoder);
    }

    static int32_t CHARLS_API_CALLING_CONVENTION at_comment_callback(const void* data, const size_t size,
                                                                     void* user_context) noexcept
    {
        try
        {
            static_cast<jpegls_decoder*>(user_context)->comment_handler_(data, size);
            return 0;
        }
        catch (...)
        {
            return 1; // will trigger jpegls_errc::callback_failed.
        }
    }

    static int32_t CHARLS_API_CALLING_CONVENTION at_application_data_callback(const int32_t application_data_id,
                                                                              const void* data, const size_t size,
                                                                              void* user_context) noexcept
    {
        try
        {
            static_cast<jpegls_decoder*>(user_context)->application_data_handler_(application_data_id, data, size);
            return 0;
        }
        catch (...)
        {
            return 1; // will trigger jpegls_errc::callback_failed.
        }
    }

    std::unique_ptr<charls_jpegls_decoder, void (*)(const charls_jpegls_decoder*)> decoder_{create_decoder(),
                                                                                            &destroy_decoder};
    bool spiff_header_has_value_{};
    charls::spiff_header spiff_header_{};
    charls::frame_info frame_info_{};
    std::function<void(const void*, size_t)> comment_handler_{};
    std::function<void(int32_t, const void*, size_t)> application_data_handler_{};
};

} // namespace charls

#endif
