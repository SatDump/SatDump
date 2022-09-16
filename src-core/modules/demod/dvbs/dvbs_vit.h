#pragma once

#include "common/dsp/block.h"
#include "viterbi_all.h"

namespace dvbs
{
    class DVBSVitBlock : public dsp::Block<int8_t, uint8_t>
    {
    private:
        void work();

    public:
        viterbi::Viterbi_DVBS *viterbi;

    public:
        DVBSVitBlock(std::shared_ptr<dsp::stream<int8_t>> input);
        ~DVBSVitBlock();
    };
}