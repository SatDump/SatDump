#pragma once

#include "common/dsp/block.h"
#include "s2_defs.h"
#include "common/dsp/demod/constellation.h"
#include "common/codings/dvb-s2/s2_scrambling.h"
#include "common/codings/dvb-s2/s2_deinterleaver.h"
#include "common/utils.h"

/*
DVB-S2 PLL, meant to frequency & phase-recover
a synchronized DVB-S2 frame.
*/
namespace dvbs2
{
    class S2BBToSoft : public dsp::Block<complex_t, int8_t>
    {
    private:
        s2_plscodes pls;
        dvbs2::S2Scrambling descrambler;

        int8_t *soft_slots_buffer;

        void work();

        int checkSyncMarker(uint64_t marker, uint64_t totest)
        {
            int errors = 0;
            for (int i = 59; i >= 0; i--)
            {
                bool markerBit, testBit;
                markerBit = getBit<uint64_t>(marker, i);
                testBit = getBit<uint64_t>(totest, i);
                if (markerBit != testBit)
                    errors++;
            }
            return errors;
        }

    public:
        int detect_modcod;
        bool detect_shortframes;
        bool detect_pilots;

        bool pilots = false;

        int frame_slot_count;
        std::shared_ptr<dsp::constellation_t> constellation;
        std::shared_ptr<dvbs2::S2Deinterleaver> deinterleaver;

    public:
        S2BBToSoft(std::shared_ptr<dsp::stream<complex_t>> input);
        ~S2BBToSoft();
    };
}