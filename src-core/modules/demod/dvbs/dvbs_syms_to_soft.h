#pragma once

#include "common/dsp/block.h"
#include <functional>

namespace dvbs
{
    class DVBSymToSoftBlock : public dsp::Block<complex_t, int8_t>
    {
    private:
        void work();

        int in_sym_buffer = 0;
        int8_t *sym_buffer;

        // Util
        int8_t clamp(float x)
        {
            if (x < -128.0)
                return -127;
            if (x > 127.0)
                return 127;
            return x;
        }

    public:
        std::function<void(complex_t *, int)> syms_callback;

    public:
        DVBSymToSoftBlock(std::shared_ptr<dsp::stream<complex_t>> input, int bufsize);
        ~DVBSymToSoftBlock();
    };
}