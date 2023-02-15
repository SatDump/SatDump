#pragma once

#include "common/dsp/block.h"
#include "common/codings/deframing/dvbs_ts_deframer.h"

namespace dvbs
{
    class DVBSDefra : public dsp::Block<uint8_t, uint8_t>
    {
    private:
        void work();

#if 0
        uint8_t curr_ts_buffer[204];

        int bit_of_frame = 0;
        void write_bit(uint8_t b)
        {
            curr_ts_buffer[bit_of_frame / 8] = curr_ts_buffer[bit_of_frame / 8] << 1 | b;
            bit_of_frame++;
        }

        uint8_t shifter_ts_sync = 0;
        bool in_frame = false;

        int compare_8(uint8_t v1, uint8_t v2)
        {
            int cor = 0;
            uint8_t diff = v1 ^ v2;
            for (; diff; cor++)
                diff &= diff - 1;
            return cor;
        }

        const int TS_SIZE = 204 * 8;
        const int TS_ASM_SIZE = 8;
        bool synced = false;

        int frm_cnt = 0;

        float polarity_top = 0.5;
#endif

    public:
        deframing::DVBS_TS_Deframer *ts_deframer;

    public:
        DVBSDefra(std::shared_ptr<dsp::stream<uint8_t>> input);
        ~DVBSDefra();
    };
}