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

namespace noaa
{
    template <typename T>
    inline bool getBit(T &data, int &bit)
    {
        return (data >> bit) & 1;
    }

    // Compare 2 32-bits values bit per bit
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

    NOAADeframer::NOAADeframer(int thresold)
    {
        d_thresold = thresold;
        enter_idle();
    };

    std::vector<uint16_t> NOAADeframer::work(int8_t *input, int length)
    {
        std::vector<uint16_t> out;
        int i = 0;
        while (i < length)
        {
            char bit = input[i++] > 0;

            switch (d_state)
            {
            case ST_IDLE:
                d_shifter = (d_shifter << 1) | bit; // MSB transmitted first

                //if ((d_shifter & 0x0FFFFFFFFFFFFFFFLL) == HRPT_MINOR_FRAME_SYNC)
                if (checkSyncMarker(d_shifter & 0x0FFFFFFFFFFFFFFFLL, HRPT_MINOR_FRAME_SYNC) <= d_thresold)
                {
                    out.push_back(HRPT_SYNC1);
                    out.push_back(HRPT_SYNC2);
                    out.push_back(HRPT_SYNC3);
                    out.push_back(HRPT_SYNC4);
                    out.push_back(HRPT_SYNC5);
                    out.push_back(HRPT_SYNC6);
                    inverted = false;
                    enter_synced();
                }
                else if (checkSyncMarker(d_shifter & 0x0FFFFFFFFFFFFFFFLL, HRPT_MINOR_FRAME_SYNC ^ 0x0FFFFFFFFFFFFFFFLL) <= d_thresold) // Inverted
                {
                    out.push_back(HRPT_SYNC1);
                    out.push_back(HRPT_SYNC2);
                    out.push_back(HRPT_SYNC3);
                    out.push_back(HRPT_SYNC4);
                    out.push_back(HRPT_SYNC5);
                    out.push_back(HRPT_SYNC6);
                    inverted = true;
                    enter_synced();
                }
                break;

            case ST_SYNCED:
                d_word = (d_word << 1) | (inverted ? !bit : bit); // MSB transmitted first
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
        }

        return out;
    }
} // namespace noaa