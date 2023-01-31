#pragma once

#include <cstring>
#include <vector>

namespace dsp
{
    class PolyphaseBank
    {
    private:
        bool is_init = false;

    public:
        int nfilt;
        int ntaps;
        float **taps;

    public:
        PolyphaseBank();
        void init(std::vector<float> rtaps, int nfilt);
        ~PolyphaseBank();
        PolyphaseBank(const PolyphaseBank &) = delete;
    };
} // namespace dsp
