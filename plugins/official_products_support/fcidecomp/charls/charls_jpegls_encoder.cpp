// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#include "charls_jpegls_encoder.h"
#include "version.h"
#include "encoder_strategy.h"
#include "jls_codec_factory.h"
#include "jpeg_stream_writer.h"
#include "jpegls_preset_coding_parameters.h"
#include "util.h"

#include <new>

using namespace charls;
using impl::throw_jpegls_error;

namespace charls { namespace {

constexpr bool has_option(encoding_options options, encoding_options option_to_test)
{
    using T = std::underlying_type_t<encoding_options>;
    return (static_cast<encoding_options>(static_cast<T>(options) & static_cast<T>(option_to_test))) == option_to_test;
}

}}

struct charls_jpegls_encoder final
{
    void destination(const byte_span destination)
    {
        check_argument(destination.data || destination.size == 0);
        check_operation(state_ == state::initial);

        writer_.destination(destination);
        state_ = state::destination_set;
    }

    void frame_info(const charls_frame_info& frame_info)
    {
        check_argument(frame_info.width > 0, jpegls_errc::invalid_argument_width);
        check_argument(frame_info.height > 0, jpegls_errc::invalid_argument_height);
        check_argument(frame_info.bits_per_sample >= minimum_bits_per_sample &&
                           frame_info.bits_per_sample <= maximum_bits_per_sample,
                       jpegls_errc::invalid_argument_bits_per_sample);
        check_argument(frame_info.component_count > 0 && frame_info.component_count <= maximum_component_count,
                       jpegls_errc::invalid_argument_component_count);

        frame_info_ = frame_info;
    }

    void interleave_mode(const charls::interleave_mode interleave_mode)
    {
        check_interleave_mode(interleave_mode, jpegls_errc::invalid_argument_interleave_mode);

        interleave_mode_ = interleave_mode;
    }

    void near_lossless(const int32_t near_lossless)
    {
        check_argument(near_lossless >= 0 && near_lossless <= maximum_near_lossless,
                       jpegls_errc::invalid_argument_near_lossless);

        near_lossless_ = near_lossless;
    }

    void encoding_options(const charls::encoding_options encoding_options)
    {
        constexpr charls::encoding_options all_options = encoding_options::even_destination_size |
                                                         encoding_options::include_version_number |
                                                         encoding_options::include_pc_parameters_jai;
        check_argument(encoding_options >= encoding_options::none && encoding_options <= all_options,
                       jpegls_errc::invalid_argument_encoding_options);

        encoding_options_ = encoding_options;
    }

    void preset_coding_parameters(const jpegls_pc_parameters& preset_coding_parameters) noexcept
    {
        // Note: validation will be done just before decoding, as more info is needed for the validation.
        user_preset_coding_parameters_ = preset_coding_parameters;
    }

    void color_transformation(const charls::color_transformation color_transformation)
    {
        check_argument(color_transformation >= charls::color_transformation::none &&
                           color_transformation <= charls::color_transformation::hp3,
                       jpegls_errc::invalid_argument_color_transformation);

        color_transformation_ = color_transformation;
    }

    size_t estimated_destination_size() const
    {
        check_operation(is_frame_info_configured());
        return checked_mul(checked_mul(checked_mul(frame_info_.width, frame_info_.height), frame_info_.component_count),
                           bit_to_byte_count(frame_info_.bits_per_sample)) +
               1024 + spiff_header_size_in_bytes;
    }

    void write_spiff_header(const spiff_header& spiff_header)
    {
        check_argument(spiff_header.height > 0, jpegls_errc::invalid_argument_height);
        check_argument(spiff_header.width > 0, jpegls_errc::invalid_argument_width);
        check_operation(state_ == state::destination_set);

        writer_.write_start_of_image();
        writer_.write_spiff_header_segment(spiff_header);
        state_ = state::spiff_header;
    }

    void write_standard_spiff_header(const spiff_color_space color_space, const spiff_resolution_units resolution_units,
                                     const uint32_t vertical_resolution, const uint32_t horizontal_resolution)
    {
        check_operation(is_frame_info_configured());
        write_spiff_header({spiff_profile_id::none, frame_info_.component_count, frame_info_.height, frame_info_.width,
                            color_space, frame_info_.bits_per_sample, spiff_compression_type::jpeg_ls, resolution_units,
                            vertical_resolution, horizontal_resolution});
    }

    void write_spiff_entry(const uint32_t entry_tag, CHARLS_IN_READS_BYTES(entry_data_size_bytes) const void* entry_data,
                           const size_t entry_data_size_bytes)
    {
        check_argument(entry_data || entry_data_size_bytes == 0);
        check_argument(entry_tag != spiff_end_of_directory_entry_type);
        check_argument(entry_data_size_bytes <= 65528, jpegls_errc::invalid_argument_size);
        check_operation(state_ == state::spiff_header);

        writer_.write_spiff_directory_entry(entry_tag, entry_data, entry_data_size_bytes);
    }

    void write_spiff_end_of_directory_entry()
    {
        check_operation(state_ == state::spiff_header);
        transition_to_tables_and_miscellaneous_state();
    }

    void write_comment(const const_byte_span comment)
    {
        check_argument(comment.data() || comment.empty());
        check_argument(comment.size() <= segment_max_data_size, jpegls_errc::invalid_argument_size);
        check_operation(state_ >= state::destination_set && state_ < state::completed);

        transition_to_tables_and_miscellaneous_state();
        writer_.write_comment_segment(comment);
    }

    void write_application_data(const int32_t application_data_id, const const_byte_span application_data)
    {
        check_argument(application_data_id >= minimum_application_data_id &&
                       application_data_id <= maximum_application_data_id);
        check_argument(application_data.data() || application_data.empty());
        check_argument(application_data.size() <= segment_max_data_size, jpegls_errc::invalid_argument_size);
        check_operation(state_ >= state::destination_set && state_ < state::completed);

        transition_to_tables_and_miscellaneous_state();
        writer_.write_application_data_segment(application_data_id, application_data);
    }

    void encode(byte_span source, size_t stride)
    {
        check_argument(source.data || source.size == 0);
        check_operation(is_frame_info_configured() && state_ != state::initial);
        check_interleave_mode_against_component_count();

        const int32_t maximum_sample_value{calculate_maximum_sample_value(frame_info_.bits_per_sample)};
        if (UNLIKELY(
                !is_valid(user_preset_coding_parameters_, maximum_sample_value, near_lossless_, &preset_coding_parameters_)))
            throw_jpegls_error(jpegls_errc::invalid_argument_jpegls_pc_parameters);

        if (stride == auto_calculate_stride)
        {
            stride = calculate_stride();
        }
        else
        {
            check_stride(stride, source.size);
        }

        transition_to_tables_and_miscellaneous_state();

        if (color_transformation_ != charls::color_transformation::none)
        {
            if (UNLIKELY(frame_info_.bits_per_sample != 8 && frame_info_.bits_per_sample != 16))
                throw_jpegls_error(jpegls_errc::bit_depth_for_transform_not_supported);

            writer_.write_color_transform_segment(color_transformation_);
        }

        const bool oversized_image{writer_.write_start_of_frame_segment(frame_info_)};
        if (oversized_image)
        {
            writer_.write_jpegls_preset_parameters_segment(frame_info_.height, frame_info_.width);
        }

        if (!is_default(user_preset_coding_parameters_, compute_default(maximum_sample_value, near_lossless_)) ||
            (has_option(encoding_options::include_pc_parameters_jai) && frame_info_.bits_per_sample > 12))
        {
            // Write the actual used values to the stream. The user parameters may use 0 (=default) values.
            // This reduces the risk for decoding by other implementations.
            writer_.write_jpegls_preset_parameters_segment(preset_coding_parameters_);
        }

        if (interleave_mode_ == charls::interleave_mode::none)
        {
            const size_t byte_count_component{stride * frame_info_.height};
            const int32_t last_component{frame_info_.component_count - 1};
            for (int32_t component{}; component != frame_info_.component_count; ++component)
            {
                writer_.write_start_of_scan_segment(1, near_lossless_, interleave_mode_);
                encode_scan(source, stride, 1);

                // Synchronize the source stream (encode_scan works on a local copy)
                if (component != last_component)
                {
                    skip_bytes(source, byte_count_component);
                }
            }
        }
        else
        {
            writer_.write_start_of_scan_segment(frame_info_.component_count, near_lossless_, interleave_mode_);
            encode_scan(source, stride, frame_info_.component_count);
        }

        writer_.write_end_of_image(has_option(encoding_options::even_destination_size));
        state_ = state::completed;
    }

    size_t bytes_written() const noexcept
    {
        return writer_.bytes_written();
    }

    void rewind() noexcept
    {
        if (state_ == state::initial)
            return; // Nothing to do, stay in the same state.

        writer_.rewind();
        state_ = state::destination_set;
    }

private:
    enum class state
    {
        initial,
        destination_set,
        spiff_header,
        tables_and_miscellaneous,
        completed
    };

    bool is_frame_info_configured() const noexcept
    {
        return frame_info_.width != 0;
    }

    void encode_scan(const byte_span source, const size_t stride, const int32_t component_count)
    {
        const charls::frame_info frame_info{frame_info_.width, frame_info_.height, frame_info_.bits_per_sample,
                                            component_count};

        const auto codec{jls_codec_factory<encoder_strategy>().create_codec(
            frame_info, {near_lossless_, 0, interleave_mode_, color_transformation_, false}, preset_coding_parameters_)};
        std::unique_ptr<process_line> process_line(codec->create_process_line(source, stride));
        const size_t bytes_written{codec->encode_scan(std::move(process_line), writer_.remaining_destination())};

        // Synchronize the destination encapsulated in the writer (encode_scan works on a local copy)
        writer_.seek(bytes_written);
    }

    size_t calculate_stride() const noexcept
    {
        const auto stride{static_cast<size_t>(frame_info_.width) * bit_to_byte_count(frame_info_.bits_per_sample)};
        if (interleave_mode_ == charls::interleave_mode::none)
            return stride;

        return stride * frame_info_.component_count;
    }

    void check_stride(const size_t stride, const size_t source_size) const
    {
        const size_t minimum_stride{calculate_stride()};
        if (UNLIKELY(stride < minimum_stride))
            throw_jpegls_error(jpegls_errc::invalid_argument_stride);

        // Simple check to verify user input, and prevent out-of-bound read access.
        // Stride parameter defines the number of bytes on a scan line.
        if (interleave_mode_ == charls::interleave_mode::none)
        {
            const size_t minimum_source_size{stride * frame_info_.component_count * frame_info_.height - (stride - minimum_stride)};
            if (UNLIKELY(source_size < minimum_source_size))
                throw_jpegls_error(jpegls_errc::invalid_argument_stride);
        }
        else
        {
            const size_t minimum_source_size{stride * frame_info_.height - (stride - minimum_stride)};
            if (UNLIKELY(source_size < minimum_source_size))
                throw_jpegls_error(jpegls_errc::invalid_argument_stride);
        }
    }

    void check_interleave_mode_against_component_count() const
    {
        if (UNLIKELY(frame_info_.component_count == 1 && interleave_mode_ != interleave_mode::none))
            throw_jpegls_error(jpegls_errc::invalid_argument_interleave_mode);
    }

    void transition_to_tables_and_miscellaneous_state()
    {
        if (state_ == state::tables_and_miscellaneous)
            return;

        if (state_ == state::spiff_header)
        {
            writer_.write_spiff_end_of_directory_entry();
        }
        else
        {
            writer_.write_start_of_image();
        }

        if (has_option(encoding_options::include_version_number))
        {
            const char* version_number{"charls " TO_STRING(CHARLS_VERSION_MAJOR) "." TO_STRING(
                CHARLS_VERSION_MINOR) "." TO_STRING(CHARLS_VERSION_PATCH)};
            writer_.write_comment_segment({version_number, strlen(version_number) + 1});
        }

        state_ = state::tables_and_miscellaneous;
    }

    bool has_option(const charls::encoding_options option_to_test) const noexcept
    {
        return ::has_option(encoding_options_, option_to_test);
    }

    charls_frame_info frame_info_{};
    int32_t near_lossless_{};
    charls::interleave_mode interleave_mode_{};
    charls::color_transformation color_transformation_{};
    charls::encoding_options encoding_options_{encoding_options::include_pc_parameters_jai};
    state state_{};
    jpeg_stream_writer writer_;
    jpegls_pc_parameters user_preset_coding_parameters_{};
    jpegls_pc_parameters preset_coding_parameters_{};
};

extern "C" {

USE_DECL_ANNOTATIONS charls_jpegls_encoder* CHARLS_API_CALLING_CONVENTION charls_jpegls_encoder_create() noexcept
{
    MSVC_WARNING_SUPPRESS_NEXT_LINE(26402 26409)     // don't use new and delete + scoped object and move
    return new (std::nothrow) charls_jpegls_encoder; // NOLINT(cppcoreguidelines-owning-memory)
}


USE_DECL_ANNOTATIONS void CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_destroy(const charls_jpegls_encoder* encoder) noexcept
{
    MSVC_WARNING_SUPPRESS_NEXT_LINE(26401 26409) // don't use new and delete + non-owner.
    delete encoder;                              // NOLINT(cppcoreguidelines-owning-memory)
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_encoder_set_destination_buffer(
    charls_jpegls_encoder* encoder, void* destination_buffer, const size_t destination_size_bytes) noexcept
try
{
    check_pointer(encoder)->destination({destination_buffer, destination_size_bytes});
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_set_frame_info(charls_jpegls_encoder* encoder, const charls_frame_info* frame_info) noexcept
try
{
    check_pointer(encoder)->frame_info(*check_pointer(frame_info));
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_set_near_lossless(charls_jpegls_encoder* encoder, const int32_t near_lossless) noexcept
try
{
    check_pointer(encoder)->near_lossless(near_lossless);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_encoder_set_encoding_options(
    charls_jpegls_encoder* encoder, const charls_encoding_options encoding_options) noexcept
try
{
    check_pointer(encoder)->encoding_options(encoding_options);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_encoder_set_interleave_mode(
    charls_jpegls_encoder* encoder, const charls_interleave_mode interleave_mode) noexcept
try
{
    check_pointer(encoder)->interleave_mode(interleave_mode);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_encoder_set_preset_coding_parameters(
    charls_jpegls_encoder* encoder, const charls_jpegls_pc_parameters* preset_coding_parameters) noexcept
try
{
    check_pointer(encoder)->preset_coding_parameters(*check_pointer(preset_coding_parameters));
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_encoder_set_color_transformation(
    charls_jpegls_encoder* encoder, const charls_color_transformation color_transformation) noexcept
try
{
    check_pointer(encoder)->color_transformation(color_transformation);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_get_estimated_destination_size(const charls_jpegls_encoder* encoder, size_t* size_in_bytes) noexcept
try
{
    *check_pointer(size_in_bytes) = check_pointer(encoder)->estimated_destination_size();
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_get_bytes_written(const charls_jpegls_encoder* encoder, size_t* bytes_written) noexcept
try
{
    *check_pointer(bytes_written) = check_pointer(encoder)->bytes_written();
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_encode_from_buffer(charls_jpegls_encoder* encoder, const void* source_buffer,
                                         const size_t source_size_bytes, const uint32_t stride) noexcept
try
{
    check_pointer(encoder)->encode({source_buffer, source_size_bytes}, stride);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_write_spiff_header(charls_jpegls_encoder* encoder, const charls_spiff_header* spiff_header) noexcept
try
{
    check_pointer(encoder)->write_spiff_header(*check_pointer(spiff_header));
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_encoder_write_standard_spiff_header(
    charls_jpegls_encoder* encoder, const charls_spiff_color_space color_space,
    const charls_spiff_resolution_units resolution_units, const uint32_t vertical_resolution,
    const uint32_t horizontal_resolution) noexcept
try
{
    check_pointer(encoder)->write_standard_spiff_header(color_space, resolution_units, vertical_resolution,
                                                        horizontal_resolution);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_write_spiff_entry(charls_jpegls_encoder* encoder, const uint32_t entry_tag, const void* entry_data,
                                        const size_t entry_data_size_bytes) noexcept
try
{
    check_pointer(encoder)->write_spiff_entry(entry_tag, entry_data, entry_data_size_bytes);
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_write_spiff_end_of_directory_entry(charls_jpegls_encoder* encoder) noexcept
try
{
    check_pointer(encoder)->write_spiff_end_of_directory_entry();
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION charls_jpegls_encoder_write_comment(
    charls_jpegls_encoder* encoder, const void* comment, const size_t comment_size_bytes) noexcept
try
{
    check_pointer(encoder)->write_comment({comment, comment_size_bytes});
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_write_application_data(charls_jpegls_encoder* encoder, const int32_t application_data_id,
                                             const void* application_data, const size_t application_data_size_bytes) noexcept
try
{
    check_pointer(encoder)->write_application_data(application_data_id, {application_data, application_data_size_bytes});
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION
charls_jpegls_encoder_rewind(charls_jpegls_encoder* encoder) noexcept
try
{
    check_pointer(encoder)->rewind();
    return jpegls_errc::success;
}
catch (...)
{
    return to_jpegls_errc();
}


USE_DECL_ANNOTATIONS jpegls_errc CHARLS_API_CALLING_CONVENTION
JpegLsEncode(void* destination, const size_t destination_length, size_t* bytes_written, const void* source,
             const size_t source_length, const JlsParameters* params, char* error_message) noexcept
try
{
    check_argument(check_pointer(params)->jfif.version == 0);

    charls_jpegls_encoder encoder;
    encoder.destination({check_pointer(destination), destination_length});
    encoder.near_lossless(params->allowedLossyError);

    encoder.frame_info({static_cast<uint32_t>(params->width), static_cast<uint32_t>(params->height), params->bitsPerSample,
                        params->components});
    encoder.interleave_mode(params->interleaveMode);
    encoder.color_transformation(params->colorTransformation);
    const auto& pc{params->custom};
    encoder.preset_coding_parameters({pc.MaximumSampleValue, pc.Threshold1, pc.Threshold2, pc.Threshold3, pc.ResetValue});

    encoder.encode({check_pointer(source), source_length}, static_cast<size_t>(params->stride));
    *check_pointer(bytes_written) = encoder.bytes_written();

    clear_error_message(error_message);
    return jpegls_errc::success;
}
catch (...)
{
    return set_error_message(to_jpegls_errc(), error_message);
}
}
