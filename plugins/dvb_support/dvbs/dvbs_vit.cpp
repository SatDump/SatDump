#include "dvbs_vit.h"
#include "dvbs_defines.h"

namespace dvbs
{
    DVBSVitBlock::DVBSVitBlock(std::shared_ptr<dsp::stream<int8_t>> input)
        : Block(input)
    {
    }

    DVBSVitBlock::~DVBSVitBlock()
    {
    }

    void DVBSVitBlock::work()
    {
        int nsamples = input_stream->read();
        if (nsamples <= 0)
        {
            input_stream->flush();
            return;
        }

        int size = viterbi->work(input_stream->readBuf, VIT_BUF_SIZE, output_stream->writeBuf);

        input_stream->flush();
        output_stream->swap(size);
    }
}