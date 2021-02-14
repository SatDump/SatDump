#include "noaa_deframer.h"

#define ST_IDLE 0
#define ST_SYNCED 1

#define HRPT_SYNC1 0x0284
#define HRPT_SYNC2 0x016F
#define HRPT_SYNC3 0x035C
#define HRPT_SYNC4 0x019D
#define HRPT_SYNC5 0x020F
#define HRPT_SYNC6 0x0095

#define HRPT_MINOR_FRAME_SYNC 0x0A116FD719D83C95LL

#define HRPT_SYNC_WORDS 6
#define HRPT_MINOR_FRAME_WORDS 11090
#define HRPT_BITS_PER_WORD 10

void NOAADeframer::enter_idle()
{
    d_state = ST_IDLE;
}

void NOAADeframer::enter_synced()
{
    d_state = ST_SYNCED;
    d_bit_count = HRPT_BITS_PER_WORD;
    d_word_count = HRPT_MINOR_FRAME_WORDS - HRPT_SYNC_WORDS;
    d_word = 0;
}

NOAADeframer::NOAADeframer()
{
    d_mid_bit = true;
    d_last_bit = 0;
    enter_idle();
};

std::vector<uint16_t> NOAADeframer::work(uint8_t *input, int length)
{
    std::vector<uint16_t> out;
    int i = 0;
    while (i < length)
    {
        char bit = input[i++];
        char diff = bit ^ d_last_bit;
        d_last_bit = bit;

        // Wait for transition if not synced, otherwise, alternate bits
        if (d_mid_bit && (diff | (d_state == ST_SYNCED)))
        {
            switch (d_state)
            {
            case ST_IDLE:
                d_shifter = (d_shifter << 1) | bit; // MSB transmitted first

                if ((d_shifter & 0x0FFFFFFFFFFFFFFFLL) == HRPT_MINOR_FRAME_SYNC)
                {
                    out.push_back(HRPT_SYNC1);
                    out.push_back(HRPT_SYNC2);
                    out.push_back(HRPT_SYNC3);
                    out.push_back(HRPT_SYNC4);
                    out.push_back(HRPT_SYNC5);
                    out.push_back(HRPT_SYNC6);
                    enter_synced();
                }
                break;

            case ST_SYNCED:
                d_word = (d_word << 1) | bit; // MSB transmitted first
                if (--d_bit_count == 0)
                {
                    out.push_back(d_word);
                    d_word = 0;
                    d_bit_count = HRPT_BITS_PER_WORD;
                    if (--d_word_count == 0)
                    {
                        enter_idle();
                    }
                }
                break;

            default:
                break;
            }

            d_mid_bit = false;
        }
        else
        {
            d_mid_bit = true;
        }
    }

    return out;
}