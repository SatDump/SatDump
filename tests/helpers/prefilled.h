#pragma once

/*
Construction over pre-filled memory, shared by the whole suite. Everything is
inline and lives in the test_helpers namespace, as every test file is linked
into the same executable.

Some src-core classes read members they never initialise (see
UNTESTED_ISSUES.md). On the stack or the heap those members hold whatever the
previous tests left there, so the result changes with the test order and from
run to run. Building the object over memory filled with a known byte gives
them a fixed value instead, chosen by the test to be harmless.
*/

#include <cstdint>
#include <cstring>
#include <memory>
#include <new>
#include <utility>

namespace test_helpers
{
    template <typename T>
    struct PrefilledDelete
    {
        void operator()(T *p) const
        {
            p->~T();
            ::operator delete(p, std::align_val_t(alignof(T)));
        }
    };

    template <typename T>
    using Prefilled = std::unique_ptr<T, PrefilledDelete<T>>;

    // Constructs a T over memory where every byte is `fill`
    template <typename T, typename... Args>
    inline Prefilled<T> makePrefilled(uint8_t fill, Args &&...args)
    {
        void *mem = ::operator new(sizeof(T), std::align_val_t(alignof(T)));
        memset(mem, fill, sizeof(T));
        try
        {
            return Prefilled<T>(new (mem) T(std::forward<Args>(args)...));
        }
        catch (...)
        {
            ::operator delete(mem, std::align_val_t(alignof(T)));
            throw;
        }
    }
} // namespace test_helpers
