// Copyright (c) Team CharLS.
// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <type_traits>

// Some cross platform builds require an explicit static_cast, while others don't.
// These templates can be used to keep code compatible with the GCC useless-cast warning and ReSharper.

namespace charls {

template<typename T, typename U, std::enable_if_t<!std::is_same<T, U>::value, int> = 0>
T conditional_static_cast(U value) noexcept
{
    return static_cast<T>(value);
}

template<typename T, typename U, std::enable_if_t<std::is_same<T, U>::value, int> = 0>
T conditional_static_cast(U value) noexcept
{
    return value;
}

} // namespace charls
