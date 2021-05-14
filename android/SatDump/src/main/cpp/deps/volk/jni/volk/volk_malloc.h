/* -*- c -*- */
/*
 * Copyright 2014, 2020 Free Software Foundation, Inc.
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

#ifndef INCLUDED_VOLK_MALLOC_H
#define INCLUDED_VOLK_MALLOC_H

#include <stdlib.h>
#include <volk/volk_common.h>

__VOLK_DECL_BEGIN

/*!
 * \brief Allocate \p size bytes of data aligned to \p alignment.
 *
 * \details
 * We use C11 and want to rely on C11 library features,
 * namely we use `aligned_alloc` to allocate aligned memory.
 * see: https://en.cppreference.com/w/c/memory/aligned_alloc
 *
 * Not all platforms support this feature.
 * For Apple Clang, we fall back to `posix_memalign`.
 * see: https://linux.die.net/man/3/aligned_alloc
 * For MSVC, we fall back to `_aligned_malloc`.
 * see:
 * https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/aligned-malloc?view=vs-2019
 *
 * Because of the ways in which volk_malloc may allocate memory, it is
 * important to always free volk_malloc pointers using volk_free.
 * Mainly, in case MSVC is used. Consult corresponding documentation
 * in case you use MSVC.
 *
 * \param size The number of bytes to allocate.
 * \param alignment The byte alignment of the allocated memory.
 * \return pointer to aligned memory.
 */
VOLK_API void* volk_malloc(size_t size, size_t alignment);

/*!
 * \brief Free's memory allocated by volk_malloc.
 *
 * \details
 * We rely on C11 syntax and compilers and just call `free` in case
 * memory was allocated with `aligned_alloc` or `posix_memalign`.
 * Thus, in this case `volk_free` inherits the same behavior `free` exhibits.
 * see: https://en.cppreference.com/w/c/memory/free
 * In case `_aligned_malloc` was used, we call `_aligned_free`.
 * see:
 * https://docs.microsoft.com/en-us/cpp/c-runtime-library/reference/aligned-free?view=vs-2019
 *
 * \param aptr The aligned pointer allocated by volk_malloc.
 */
VOLK_API void volk_free(void* aptr);

__VOLK_DECL_END

#endif /* INCLUDED_VOLK_MALLOC_H */
