// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "jpegls_error.h"
#include "jpeg_marker_code.h"
#include "process_line.h"
#include "util.h"

#include <cassert>
#include <memory>

namespace charls {

// Purpose: Implements encoding to stream of bits. In encoding mode jls_codec inherits from decoder_strategy
class decoder_strategy
{
public:
    decoder_strategy(const frame_info& frame, const coding_parameters& parameters) noexcept :
        frame_info_{frame}, parameters_{parameters}
    {
    }

    virtual ~decoder_strategy() = default;

    decoder_strategy(const decoder_strategy&) = delete;
    decoder_strategy(decoder_strategy&&) = delete;
    decoder_strategy& operator=(const decoder_strategy&) = delete;
    decoder_strategy& operator=(decoder_strategy&&) = delete;

    virtual std::unique_ptr<process_line> create_process_line(byte_span destination, size_t stride) = 0;
    virtual void set_presets(const jpegls_pc_parameters& preset_coding_parameters, uint32_t restart_interval) = 0;
    virtual size_t decode_scan(std::unique_ptr<process_line> output_data, const JlsRect& size, const_byte_span encoded_source) = 0;

    void initialize(const const_byte_span source)
    {
        position_ = source.data();
        end_position_ = source.end();

        find_jpeg_marker_start_byte();
        fill_read_cache();
    }

    void reset()
    {
        valid_bits_ = 0;
        read_cache_ = 0;

        find_jpeg_marker_start_byte();
        fill_read_cache();
    }

    FORCE_INLINE void skip(const int32_t length) noexcept
    {
        ASSERT(length);

        valid_bits_ -= length; // Note: valid_bits_ may become negative to indicate that extra bits are needed.
        read_cache_ = read_cache_ << length;
    }

    void on_line_end(const void* source, const size_t pixel_count, const size_t pixel_stride) const
    {
        process_line_->new_line_decoded(source, pixel_count, pixel_stride);
    }

    void end_scan()
    {
        if (UNLIKELY(position_ >= end_position_))
            impl::throw_jpegls_error(jpegls_errc::source_buffer_too_small);

        if (*position_ != jpeg_marker_start_byte)
        {
            read_bit();

            if (UNLIKELY(*position_ != jpeg_marker_start_byte))
                impl::throw_jpegls_error(jpegls_errc::too_much_encoded_data);
        }

        if (UNLIKELY(read_cache_ != 0))
            impl::throw_jpegls_error(jpegls_errc::too_much_encoded_data);
    }

    const uint8_t* get_cur_byte_pos() const noexcept
    {
        int32_t valid_bits{valid_bits_};
        const uint8_t* compressed_bytes{position_};

        for (;;)
        {
            const int32_t last_bits_count{compressed_bytes[-1] == jpeg_marker_start_byte ? 7 : 8};

            if (valid_bits < last_bits_count)
                return compressed_bytes;

            valid_bits -= last_bits_count;
            --compressed_bytes;
        }
    }

    FORCE_INLINE int32_t read_value(const int32_t length)
    {
        if (valid_bits_ < length)
        {
            fill_read_cache();
            if (UNLIKELY(valid_bits_ < length))
                impl::throw_jpegls_error(jpegls_errc::invalid_encoded_data);
        }

        ASSERT(length != 0 && length <= valid_bits_);
        ASSERT(length < 32);
        const auto result{static_cast<int32_t>(read_cache_ >> (cache_t_bit_count - length))};
        skip(length);
        return result;
    }

    FORCE_INLINE int32_t peek_byte()
    {
        if (valid_bits_ < 8)
        {
            fill_read_cache();
        }

        return static_cast<int32_t>(read_cache_ >> max_readable_cache_bits);
    }

    FORCE_INLINE bool read_bit()
    {
        if (valid_bits_ <= 0)
        {
            fill_read_cache();
        }

        const bool set{(read_cache_ & (static_cast<cache_t>(1) << (cache_t_bit_count - 1))) != 0};
        skip(1);
        return set;
    }

    FORCE_INLINE int32_t peek_0_bits()
    {
        if (valid_bits_ < 16)
        {
            fill_read_cache();
        }

#if defined(_MSC_VER) || defined(__GNUC__)
        const auto count{countl_zero(read_cache_)};
        return count < 16 ? count : -1;
#else
        cache_t val_test = read_cache_;

        for (int32_t count{}; count < 16; ++count)
        {
            if ((val_test & (static_cast<cache_t>(1) << (cache_t_bit_count - 1))) != 0)
                return count;

            val_test <<= 1;
        }
        return -1;
#endif
    }

    FORCE_INLINE int32_t read_high_bits()
    {
        const int32_t count{peek_0_bits()};
        if (count >= 0)
        {
            skip(count + 1);
            return count;
        }
        skip(15);

        for (int32_t high_bits_count{15};; ++high_bits_count)
        {
            if (read_bit())
                return high_bits_count;
        }
    }

    int32_t read_long_value(const int32_t length)
    {
        if (length <= 24)
            return read_value(length);

        return (read_value(length - 24) << 24) + read_value(24);
    }

    uint8_t read_byte()
    {
        if (UNLIKELY(position_ == end_position_))
            impl::throw_jpegls_error(jpegls_errc::source_buffer_too_small);

        const uint8_t value = *position_;
        ++position_;
        return value;
    }

protected:
    frame_info frame_info_;
    coding_parameters parameters_;
    std::unique_ptr<process_line> process_line_;

private:
    using cache_t = size_t;

    void fill_read_cache()
    {
        ASSERT(valid_bits_ <= max_readable_cache_bits);

        if (fill_read_cache_optimistic())
            return;

        do
        {
            if (position_ >= end_position_)
            {
                if (UNLIKELY(valid_bits_ == 0))
                {
                    // Decoding process expects at least some bits to be added to the cache.
                    impl::throw_jpegls_error(jpegls_errc::invalid_encoded_data);
                }

                return;
            }

            const cache_t new_byte_value{*position_};

            // JPEG-LS bit stream rule: if FF is followed by a 1 bit then it is a marker
            if (new_byte_value == jpeg_marker_start_byte &&
                (position_ == end_position_ - 1 || (position_[1] & 0x80) != 0))
            {
                if (UNLIKELY(valid_bits_ <= 0))
                {
                    // Decoding process expects at least some bits to be added to the cache.
                    impl::throw_jpegls_error(jpegls_errc::invalid_encoded_data);
                }

                // End of buffer or marker detected. Typical found markers are EOI, SOS (next scan) or RSTm.
                return;
            }

            read_cache_ |= new_byte_value << (max_readable_cache_bits - valid_bits_);
            valid_bits_ += 8;
            ++position_;

            if (new_byte_value == jpeg_marker_start_byte)
            {
                // The next bit after an 0xFF needs to be ignored, compensate for the next read (see ISO/IEC 14495-1,A.1)
                --valid_bits_;
            }

        } while (valid_bits_ < max_readable_cache_bits);

        find_jpeg_marker_start_byte();
    }

    FORCE_INLINE bool fill_read_cache_optimistic() noexcept
    {
        // Easy & fast: if there is no 0xFF byte in sight, we can read without bit stuffing
        if (position_ < position_ff_ - (sizeof(cache_t) - 1))
        {
            read_cache_ |= read_big_endian_unaligned<cache_t>(position_) >> valid_bits_;
            const int bytes_to_read{(cache_t_bit_count - valid_bits_) / 8};
            position_ += bytes_to_read;
            valid_bits_ += bytes_to_read * 8;
            ASSERT(valid_bits_ >= max_readable_cache_bits);
            return true;
        }
        return false;
    }

    void find_jpeg_marker_start_byte() noexcept
    {
        // Use memchr to find next start byte (0xFF). memchr is optimized on some platforms to search faster.
        position_ff_ = static_cast<const uint8_t*>(memchr(position_, jpeg_marker_start_byte, end_position_ - position_));
        if (!position_ff_)
        {
            position_ff_ = end_position_;
        }
    }

    static constexpr auto cache_t_bit_count{static_cast<int32_t>(sizeof(cache_t) * 8)};
    static constexpr int32_t max_readable_cache_bits{cache_t_bit_count - 8};

    // decoding
    cache_t read_cache_{};
    int32_t valid_bits_{};
    const uint8_t* position_{};
    const uint8_t* end_position_{};
    const uint8_t* position_ff_{};
};

} // namespace charls
