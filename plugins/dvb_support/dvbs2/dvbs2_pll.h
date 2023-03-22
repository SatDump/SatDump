#pragma once

#include "common/dsp/block.h"
#include "s2_defs.h"
#include "common/dsp/demod/constellation.h"

/*
DVB-S2 PLL, meant to frequency & phase-recover
a synchronized DVB-S2 frame.
*/
namespace dvbs2
{
    class S2PLLBlock : public dsp::Block<complex_t, complex_t>
    {
    private:
        float phase = 0, freq = 0;
        float alpha, beta;

        s2_sof sof;
        s2_plscodes pls;

        complex_t tmp_val;

        void work();

        int pilot_cnt = 0;

    public:
        int pls_code;
        int frame_slot_count;
        bool pilots;

        void update()
        {
            if (pilots)
            {
                int raw_size = (frame_slot_count - 90) / 90;
                pilot_cnt = 1;
                raw_size -= 16; // First pilot is 16 slots after the SOF

                while (raw_size > 16)
                {
                    raw_size -= 16; // The rest is 32 symbols further
                    pilot_cnt++;
                }
            }
        }

        std::shared_ptr<dsp::constellation_t> constellation;
        dsp::constellation_t constellation_pilots = dsp::constellation_t(dsp::QPSK);

    public:
        S2PLLBlock(std::shared_ptr<dsp::stream<complex_t>> input, float loop_bw);
        ~S2PLLBlock();

        float getFreq() { return freq; }
    };
}