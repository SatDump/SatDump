/*
 * MemoryOp.cpp
 *
 *  Created on: 06/01/2017
 *      Author: Lucas Teske
 */

#include "MemoryOp.h"

#include <cstdlib>
#include <iostream>
#include <errno.h>
#include <cstdio>
#include <cstring>

#ifdef _WIN32
#include <malloc.h>
#endif

namespace sathelper
{
    inline static void c_free(void *ptr)
    {
        free(ptr);
    }

    void *MemoryOp::alignedAlloc(size_t size, size_t alignment)
    {
        if (alignment == 1)
        {
            return malloc(size);
        }

#ifdef _WIN32
#ifdef __MINGW32__
        return __mingw_aligned_malloc(size, alignment);
#else
        return _aligned_malloc(size, alignment);
#endif
#else
        void *ptr;
        int err = posix_memalign(&ptr, alignment, size);
        if (err == 0)
        {
            return ptr;
        }
        else
        {
            std::cerr << "Error alocating memory (" << err << "): " << strerror(err) << std::endl;
            return NULL;
        }
#endif
    }

    void MemoryOp::free(void *data)
    {
#ifdef _WIN32
#ifdef __MINGW32__
        __mingw_aligned_free(data);
#else
        _aligned_free(data);
#endif
#else
        c_free(data);
#endif
    }

    int MemoryOp::getAligment()
    {
#ifdef MEMORY_OP_X86
        return 32;
#elif defined(MEMORY_OP_ARM)
        return 1;
#else
        return 1;
#endif
    }

} // namespace sathelper
