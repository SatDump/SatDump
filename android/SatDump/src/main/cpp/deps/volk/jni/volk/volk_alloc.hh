/* -*- C++ -*- */
/*
 * Copyright 2019 Free Software Foundation, Inc.
 *
 * This file is part of GNU Radio
 *
 * GNU Radio is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * GNU Radio is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GNU Radio; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street,
 * Boston, MA 02110-1301, USA.
 */

#ifndef INCLUDED_VOLK_ALLOC_H
#define INCLUDED_VOLK_ALLOC_H

#include <cstdlib>
#include <limits>
#include <new>
#include <vector>

#include <volk/volk.h>

namespace volk {

/*!
 * \brief C++11 allocator using volk_malloc and volk_free
 *
 * \details
 *   adapted from https://en.cppreference.com/w/cpp/named_req/Alloc
 */
template <class T>
struct alloc {
    typedef T value_type;

    alloc() = default;

    template <class U>
    constexpr alloc(alloc<U> const&) noexcept
    {
    }

    T* allocate(std::size_t n)
    {
        if (n > std::numeric_limits<std::size_t>::max() / sizeof(T))
            throw std::bad_alloc();

        if (auto p = static_cast<T*>(volk_malloc(n * sizeof(T), volk_get_alignment())))
            return p;

        throw std::bad_alloc();
    }

    void deallocate(T* p, std::size_t) noexcept { volk_free(p); }
};

template <class T, class U>
bool operator==(alloc<T> const&, alloc<U> const&)
{
    return true;
}

template <class T, class U>
bool operator!=(alloc<T> const&, alloc<U> const&)
{
    return false;
}


/*!
 * \brief type alias for std::vector using volk::alloc
 *
 * \details
 * example code:
 *   volk::vector<float> v(100); // vector using volk_malloc, volk_free
 */
template <class T>
using vector = std::vector<T, alloc<T>>;

} // namespace volk
#endif // INCLUDED_VOLK_ALLOC_H
