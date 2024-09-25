// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include "annotations.h"

#include <cstddef>
#include <cstdint>

namespace charls {

/// <summary>Simplified span class for C++14 (std::span<std::byte>).</summary>
struct byte_span final
{
    byte_span() = default;

    constexpr byte_span(void* data_arg, const size_t size_arg) noexcept : data{static_cast<uint8_t*>(data_arg)}, size{size_arg}
    {
    }

    constexpr byte_span(const void* data_arg, const size_t size_arg) noexcept :
        data{static_cast<uint8_t*>(const_cast<void*>(data_arg))}, size{size_arg}
    {
    }

    CHARLS_CHECK_RETURN constexpr uint8_t* begin() const noexcept
    {
        return data;
    }

    CHARLS_CHECK_RETURN constexpr uint8_t* end() const noexcept
    {
        return data + size;
    }

    uint8_t* data{};
    size_t size{};
};


/// <summary>Simplified span class for C++14 (std::span<const std::byte>).</summary>
class const_byte_span final
{
public:
    using iterator = const uint8_t*;

    const_byte_span() = default;

    constexpr const_byte_span(const void* data, const size_t size) noexcept : data_{static_cast<const uint8_t*>(data)}, size_{size}
    {
    }

    template<typename It>
    constexpr const_byte_span(It first, It last) noexcept : const_byte_span(first, last - first)
    {
    }

    CHARLS_CHECK_RETURN constexpr size_t size() const noexcept
    {
        return size_;
    }

    CHARLS_CHECK_RETURN constexpr const uint8_t* data() const noexcept
    {
        return data_;
    }

    CHARLS_CHECK_RETURN constexpr iterator begin() const noexcept
    {
        return data_;
    }

    CHARLS_CHECK_RETURN constexpr iterator end() const noexcept
    {
        return data_ + size_;
    }

    CHARLS_CHECK_RETURN constexpr bool empty() const noexcept
    {
        return size_ == 0;
    }

private:
    const uint8_t* data_{};
    size_t size_{};
};

} // namespace charls
