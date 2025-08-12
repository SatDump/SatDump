// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "decoder_strategy.h"
#include "process_line.h"

namespace charls {

// Purpose: Implements encoding to stream of bits. In encoding mode jls_codec inherits from encoder_strategy
class encoder_strategy
{
public:
    encoder_strategy(const frame_info& info, const coding_parameters& parameters) noexcept :
        frame_info_{info}, parameters_{parameters}
    {
    }

    virtual ~encoder_strategy() = default;

    encoder_strategy(const encoder_strategy&) = delete;
    encoder_strategy(encoder_strategy&&) = delete;
    encoder_strategy& operator=(const encoder_strategy&) = delete;
    encoder_strategy& operator=(encoder_strategy&&) = delete;

    virtual std::unique_ptr<process_line> create_process_line(byte_span stream_info, size_t stride) = 0;
    virtual void set_presets(const jpegls_pc_parameters& preset_coding_parameters, uint32_t restart_interval) = 0;
    virtual size_t encode_scan(std::unique_ptr<process_line> raw_data, byte_span destination) = 0;

    void on_line_begin(void* destination, const size_t pixel_count, const size_t pixel_stride) const
    {
        process_line_->new_line_requested(destination, pixel_count, pixel_stride);
    }

protected:
    void initialize(const byte_span destination) noexcept
    {
        free_bit_count_ = sizeof(bit_buffer_) * 8;
        bit_buffer_ = 0;

        position_ = destination.data;
        compressed_length_ = destination.size;
    }

    void append_to_bit_stream(const uint32_t bits, const int32_t bit_count)
    {
        ASSERT(bit_count < 32 && bit_count >= 0);
        ASSERT((!decoder_) || (bit_count == 0 && bits == 0) ||
               (static_cast<uint32_t>(decoder_->read_long_value(bit_count)) == bits));
#ifndef NDEBUG
        const uint32_t mask{(1U << bit_count) - 1U};
        ASSERT((bits | mask) == mask); // Not used bits must be set to zero.
#endif

        free_bit_count_ -= bit_count;
        if (free_bit_count_ >= 0)
        {
            bit_buffer_ |= bits << free_bit_count_;
        }
        else
        {
            // Add as much bits in the remaining space as possible and flush.
            bit_buffer_ |= bits >> -free_bit_count_;
            flush();

            // A second flush may be required if extra marker detect bits were needed and not all bits could be written.
            if (free_bit_count_ < 0)
            {
                bit_buffer_ |= bits >> -free_bit_count_;
                flush();
            }

            ASSERT(free_bit_count_ >= 0);
            bit_buffer_ |= bits << free_bit_count_;
        }
    }

    void end_scan()
    {
        flush();

        // if a 0xff was written, Flush() will force one unset bit anyway
        if (is_ff_written_)
        {
            append_to_bit_stream(0, (free_bit_count_ - 1) % 8);
        }

        flush();
        ASSERT(free_bit_count_ == 32);
    }

    void flush()
    {
        if (UNLIKELY(compressed_length_ < 4))
            impl::throw_jpegls_error(jpegls_errc::destination_buffer_too_small);

        for (int i{}; i < 4; ++i)
        {
            if (free_bit_count_ >= 32)
            {
                free_bit_count_ = 32;
                break;
            }

            if (is_ff_written_)
            {
                // JPEG-LS requirement (T.87, A.1) to detect markers: after a xFF value a single 0 bit needs to be inserted.
                *position_ = static_cast<uint8_t>(bit_buffer_ >> 25);
                bit_buffer_ = bit_buffer_ << 7;
                free_bit_count_ += 7;
            }
            else
            {
                *position_ = static_cast<uint8_t>(bit_buffer_ >> 24);
                bit_buffer_ = bit_buffer_ << 8;
                free_bit_count_ += 8;
            }

            is_ff_written_ = *position_ == jpeg_marker_start_byte;
            ++position_;
            --compressed_length_;
            ++bytes_written_;
        }
    }

    size_t get_length() const noexcept
    {
        return bytes_written_ - (static_cast<size_t>(free_bit_count_) - 32U) / 8U;
    }

    FORCE_INLINE void append_ones_to_bit_stream(const int32_t length)
    {
        append_to_bit_stream((1U << length) - 1U, length);
    }

    frame_info frame_info_;
    coding_parameters parameters_;
    std::unique_ptr<decoder_strategy> decoder_;
    std::unique_ptr<process_line> process_line_;

private:
    unsigned int bit_buffer_{};
    int32_t free_bit_count_{sizeof bit_buffer_ * 8};
    size_t compressed_length_{};

    // encoding
    uint8_t* position_{};
    bool is_ff_written_{};
    size_t bytes_written_{};
};

} // namespace charls
