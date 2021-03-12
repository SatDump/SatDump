#include "fir.h"

namespace dsp
{
    CCFIRBlock::CCFIRBlock(dsp::stream<std::complex<float>> &input, int decimation, std::vector<float> taps) : Block(input), d_fir(decimation, taps)
    {
    }

    void CCFIRBlock::work()
    {
        int nsamples = input_stream.read();
        if (nsamples <= 0)
            return;
        int d_out = d_fir.work(input_stream.readBuf, nsamples, output_stream.writeBuf);
        input_stream.flush();
        output_stream.swap(d_out);
    }

    FFFIRBlock::FFFIRBlock(dsp::stream<float> &input, int decimation, std::vector<float> taps) : Block(input), d_fir(decimation, taps)
    {
    }

    void FFFIRBlock::work()
    {
        int nsamples = input_stream.read();
        if (nsamples <= 0)
            return;
        int d_out = d_fir.work(input_stream.readBuf, nsamples, output_stream.writeBuf);
        input_stream.flush();
        output_stream.swap(d_out);
    }
}