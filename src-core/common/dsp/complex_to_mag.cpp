#include "complex_to_mag.h"
#include <volk/volk.h>

namespace dsp
{
    ComplexToMagBlock::ComplexToMagBlock(std::shared_ptr<dsp::stream<complex_t>> input) : Block(input)
    {
    }

    ComplexToMagBlock::~ComplexToMagBlock()
    {
    }

    void ComplexToMagBlock::work()
    {
        int nsamples = input_stream->read();
        if (nsamples <= 0)
        {
            input_stream->flush();
            return;
        }

        volk_32fc_magnitude_32f_u(output_stream->writeBuf, (lv_32fc_t *)input_stream->readBuf, nsamples);

        input_stream->flush();
        output_stream->swap(nsamples);
    }
}