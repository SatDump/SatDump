// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>

namespace charls {

// Default threshold values for JPEG-LS statistical modeling as defined in ISO/IEC 14495-1, Table C.3
// for the case MAXVAL = 255 and NEAR = 0.
// Can be overridden at compression time, however this is rarely done.
constexpr int default_threshold1{3};  // BASIC_T1
constexpr int default_threshold2{7};  // BASIC_T2
constexpr int default_threshold3{21}; // BASIC_T3

constexpr int default_reset_value{64}; // Default RESET value as defined in ISO/IEC 14495-1, table C.2

constexpr int maximum_component_count{255};
constexpr int minimum_bits_per_sample{2};
constexpr int maximum_bits_per_sample{16};
constexpr int maximum_near_lossless{255};
constexpr int32_t minimum_application_data_id{0};
constexpr int32_t maximum_application_data_id{15};

constexpr int max_k_value{16}; // This is an implementation limit (theoretical limit is 32)

constexpr int compute_maximum_near_lossless(const int maximum_sample_value) noexcept
{
    return std::min(maximum_near_lossless, maximum_sample_value / 2); // As defined by ISO/IEC 14495-1, C.2.3
}

// Computes the initial value for A. See ISO/IEC 14495-1, A.8, step 1.d and A.2.1
constexpr int32_t initialization_value_for_a(const int32_t range) noexcept
{
    return std::max(2, (range + 32) / 64);
}

// ISO/IEC 14495-1, section 4.8.1 defines the SPIFF version numbers to be used for the SPIFF header in combination with
// JPEG-LS.
constexpr uint8_t spiff_major_revision_number{2};
constexpr uint8_t spiff_minor_revision_number{};

constexpr uint8_t spiff_end_of_directory_entry_type{1};

// The size of a SPIFF header when serialized to a JPEG byte stream.
constexpr size_t spiff_header_size_in_bytes{34};

// The special value to indicate that the stride should be calculated.
constexpr size_t auto_calculate_stride{};

// The size in bytes of the segment length field.
constexpr size_t segment_length_size{sizeof(uint16_t)};

// The maximum size of the data bytes that fit in a segment.
constexpr size_t segment_max_data_size{std::numeric_limits<uint16_t>::max() - segment_length_size};

// Number of bits in an int32_t data type.
constexpr size_t int32_t_bit_count{sizeof(int32_t) * 8};

} // namespace charls
