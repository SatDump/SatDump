#pragma once

#include <cstdint>
#include <cstring>

namespace viterbi
{
    class ViterbiSlidingBuffer
    {
    public:
        uint8_t *buffer_ptr;
        int in_buffer;

    public:
        ViterbiSlidingBuffer(int buffer_size)
        {
            buffer_ptr = new uint8_t[buffer_size];
            in_buffer = 0;
        }

        ~ViterbiSlidingBuffer()
        {
            delete[] buffer_ptr;
        }

        void add(uint8_t *buf, int len)
        {
            memcpy(&buffer_ptr[in_buffer], buf, len);
            in_buffer += len;
        }

        void del(int len)
        {
            memmove(buffer_ptr, buffer_ptr + len, in_buffer - len);
            in_buffer -= len;
            memset(buffer_ptr + in_buffer, 128, len > 100 ? 100 : len);
        }
    };
}
