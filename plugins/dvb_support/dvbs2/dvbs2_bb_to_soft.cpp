#include "dvbs2_bb_to_soft.h"

#include "logger.h"

namespace dvbs2
{
    S2BBToSoft::S2BBToSoft(std::shared_ptr<dsp::stream<complex_t>> input)
        : Block(input)
    {
        soft_slots_buffer = new int8_t[64800];
    }

    S2BBToSoft::~S2BBToSoft()
    {
        delete[] soft_slots_buffer;
    }

    void S2BBToSoft::work()
    {
        int nsamples = input_stream->read();
        if (nsamples <= 0)
        {
            input_stream->flush();
            return;
        }

        // Decode PLS. This should be moved to pl_sync later!
        int best_header = 0;
        {
            uint64_t plheader = 0;
            for (int y = 0; y < 64; y++)
            {
                bool value = (input_stream->readBuf[26 + y] * complex_t(cos(-M_PI / 4), sin(-M_PI / 4))).real > 0;
                plheader = plheader << 1 | !value;
            }

            int header_diffs = 64;
            for (int c = 0; c < 128; c++)
            {
                int differences = checkSyncMarker(pls.codewords[c], plheader);
                if (differences < header_diffs)
                {
                    best_header = c;
                    header_diffs = differences;
                }
            }
        }

        detect_modcod = best_header >> 2;
        detect_shortframes = best_header & 2;
        detect_pilots = best_header & 1;

        int pilots_offset = 0;

        // Derandomize and decode slots
        descrambler.reset();
        for (int i = 0; i < frame_slot_count * 90; i++)
        {
            if (i % 1476 == 0 && i != 0 && pilots)
                pilots_offset += 36;

            constellation->demod_soft_lut(descrambler.descramble(input_stream->readBuf[90 + i]), &soft_slots_buffer[(i - pilots_offset) * constellation->getBitsCnt()]);
        }

        // Deinterleave
        deinterleaver->deinterleave(soft_slots_buffer, output_stream->writeBuf);

        input_stream->flush();
        output_stream->swap(frame_slot_count * 90 * constellation->getBitsCnt());
    }
}