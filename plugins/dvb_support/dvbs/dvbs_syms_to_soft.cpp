#include "dvbs_syms_to_soft.h"
#include "dvbs_defines.h"

namespace dvbs
{
    DVBSymToSoftBlock::DVBSymToSoftBlock(std::shared_ptr<dsp::stream<complex_t>> input, int bufsize)
        : Block(input)
    {
        sym_buffer = new int8_t[bufsize * 40];
    }

    DVBSymToSoftBlock::~DVBSymToSoftBlock()
    {
        delete[] sym_buffer;
    }

    void DVBSymToSoftBlock::work()
    {
        int nsamples = input_stream->read();
        if (nsamples <= 0)
        {
            input_stream->flush();
            return;
        }

        // Call callback
        syms_callback(input_stream->readBuf, nsamples);

        // To soft
        for (int i = 0; i < nsamples; i++)
        {
            sym_buffer[in_sym_buffer + 0] = clamp(input_stream->readBuf[i].real * 100);
            sym_buffer[in_sym_buffer + 1] = clamp(input_stream->readBuf[i].imag * 100);
            in_sym_buffer += 2;
        }

        input_stream->flush();

        // Process
        while (in_sym_buffer >= VIT_BUF_SIZE)
        {
            memcpy(output_stream->writeBuf, &sym_buffer[0], VIT_BUF_SIZE);
            output_stream->swap(VIT_BUF_SIZE); // Swap buffers ready to use for the viterbi

            if (in_sym_buffer - VIT_BUF_SIZE > 0)
                memmove(&sym_buffer[0], &sym_buffer[VIT_BUF_SIZE], in_sym_buffer - VIT_BUF_SIZE);
            in_sym_buffer -= VIT_BUF_SIZE;
        }
    }
}