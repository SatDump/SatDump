#pragma once

#include <cstring>
#include <mutex>

/*
Simple resizeable buffer.
This was made for usecases where manual management is fine,
and std::vector too slow... Such as all the image processing.

The aim is not to automate memory management, but rather to be a
wrapper around reszing, without any performance impact compared
to a native array.

Operator overloading is utilized not to complexify things. This
should still be usable like a normal array.
*/
template <typename T>
class ResizeableBuffer
{
private:
    size_t d_size;
    size_t d_width;
    size_t d_headroom;

public:
    std::mutex buffer_lock;
    T *buf;

    bool destroyed = true;

public:
    ResizeableBuffer() { d_size = 0; }
    ~ResizeableBuffer() { destroy(); }

    void destroy()
    {
        if (!destroyed)
            delete[] buf;
        destroyed = true;
    }
    size_t size() { return d_size; }

    T &operator[](int i)
    {
        // check(i / d_width);
        return buf[i];
    }

    void create(size_t width, size_t headroom = 1000)
    {
        d_width = width;
        d_headroom = headroom;
        d_size = d_width * d_headroom;
        buf = new T[d_size];
        destroyed = false;
    }

    void resize(size_t newSize)
    {
        buffer_lock.lock();
        if (newSize > d_size)
        {
            T *newBuffer = new T[newSize];
            std::memcpy(newBuffer, buf, d_size * sizeof(T));
            delete[] buf;
            buf = newBuffer;
            d_size = newSize;
        }
        buffer_lock.unlock();
    }

    void check(size_t lines)
    {
        if (lines * d_width >= d_size) // Check for 1 extra!
            resize(d_width * (lines + d_headroom));
    }
};