#include "delay_one_imag.h"

DelayOneImag::DelayOneImag()
{
    lastSamp = 0;
}

void DelayOneImag::work(std::complex<float> *in, size_t length, std::complex<float> *out)
{
    for (int i = 0; i < length; i++)
    {
        float imag = i == 0 ? lastSamp : in[i - 1].imag();
        float real = in[i].real();

        out[i] = std::complex<float>(real, imag);
    }

    lastSamp = in[length - 1].imag();
}