#include "agc.h"

namespace dsp
{
    AgcCC::AgcCC(float rate, float reference, float gain, float max_gain) : _rate(rate), _reference(reference), _gain(gain), _max_gain(max_gain)
    {
    }

    int AgcCC::work(std::complex<float> *in, int length, std::complex<float> *out)
    {
        scaleN(out, in, length);
        return length;
    }
} // namespace libdsp