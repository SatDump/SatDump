// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "conditional_static_cast.h"
#include "util.h"

#include <array>
#include <cassert>

namespace charls {

// Tables for fast decoding of short Golomb Codes.
struct golomb_code final
{
    golomb_code() = default;

    golomb_code(const int32_t value, const uint32_t length) noexcept : value_{value}, length_{length}
    {
    }

    int32_t value() const noexcept
    {
        return value_;
    }

    uint32_t length() const noexcept
    {
        return length_;
    }

private:
    int32_t value_{};
    uint32_t length_{};
};


class golomb_code_table final
{
public:
    static constexpr size_t byte_bit_count{8};

    void add_entry(const uint8_t value, const golomb_code c) noexcept
    {
        const uint32_t length{c.length()};
        ASSERT(static_cast<size_t>(length) <= byte_bit_count);

        for (size_t i{}; i < conditional_static_cast<size_t>(1U) << (byte_bit_count - length); ++i)
        {
            ASSERT(types_[(static_cast<size_t>(value) << (byte_bit_count - length)) + i].length() == 0);
            types_[(static_cast<size_t>(value) << (byte_bit_count - length)) + i] = c;
        }
    }

    FORCE_INLINE const golomb_code& get(const uint32_t value) const noexcept
    {
        return types_[value];
    }

private:
    std::array<golomb_code, 1 << byte_bit_count> types_;
};

} // namespace charls
