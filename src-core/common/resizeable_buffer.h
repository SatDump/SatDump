#pragma once

#include <cstring>

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

public:
    T *buf;

public:
    ResizeableBuffer()
    {
        d_size = 0;
    }

    ~ResizeableBuffer()
    {
    }

    void create(size_t size)
    {
        buf = new T[size];
        d_size = size;
    }

    void destroy()
    {
        delete[] buf;
    }

    void resize(size_t newSize)
    {
        if (newSize > d_size)
        {
            T *newBuffer = new T[newSize];
            std::memcpy(newBuffer, buf, d_size * sizeof(T));
            delete[] buf;
            buf = newBuffer;
            d_size = newSize;
        }
    }

    size_t size()
    {
        return d_size;
    }

    T &operator[](int i)
    {
        return buf[i];
    }
};