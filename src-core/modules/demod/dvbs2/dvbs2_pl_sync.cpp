#include "dvbs2_pl_sync.h"

#include "logger.h"

namespace dvbs2
{
    S2PLSyncBlock::S2PLSyncBlock(std::shared_ptr<dsp::stream<complex_t>> input, int slot_number, bool pilots)
        : Block(input), slot_number(slot_number)
    {
        ring_buffer.init(10000000);

        raw_frame_size = (slot_number + 1) * 90; // PL (90 Symbols) + slots of 90 symbols

        if (pilots)
        {
            int raw_size = (raw_frame_size - 90) / 90;
            int pilot_cnt = 1;
            raw_size -= 16; // First pilot is 16 slots after the SOF

            while (raw_size > 16)
            {
                raw_size -= 16; // The rest is 32 symbols further
                pilot_cnt++;
            }

            raw_frame_size += pilot_cnt * 36;

            logger->info("Pilots size (PLSYNC) : {:d}", raw_frame_size);
        }

        correlation_buffer = new complex_t[raw_frame_size];
    }

    S2PLSyncBlock::~S2PLSyncBlock()
    {
        delete[] correlation_buffer;
    }

    void S2PLSyncBlock::work()
    {
        int nsamples = input_stream->read();
        if (nsamples <= 0)
        {
            input_stream->flush();
            return;
        }

        ring_buffer.write(input_stream->readBuf, nsamples);

        input_stream->flush();
    }

    void S2PLSyncBlock::work2()
    {
        /*int got =*/ring_buffer.read(correlation_buffer, raw_frame_size);

        // Correlate the PLHeader
        int best_pos = 0;
        double best_match = 0;
        complex_t best_match_raw = 0;

        complex_t plheader_symbols[sof.LENGTH + pls.LENGTH];

        for (int ss = 0; ss < raw_frame_size - sof.LENGTH - pls.LENGTH; ss++)
        {
#if 0
            complex_t prev = 0;
            for (int i = 0; i < sof.LENGTH + pls.LENGTH; i++)
            {
                float phase2 = 0;
                // complex_t vco = complex_t(cosf(phase2), sinf(phase2));
                plheader_symbols[i] = conjprod(prev, correlation_buffer[ss + i] /* * vco*/);
                prev = correlation_buffer[ss + i] /* * vco*/;
                // phase2 += freq;
            }
#endif

            plheader_symbols[0] = 0;
            volk_32fc_conjugate_32fc((lv_32fc_t *)&plheader_symbols[1], (lv_32fc_t *)&correlation_buffer[ss], sof.LENGTH + pls.LENGTH - 1);
            volk_32fc_x2_multiply_32fc((lv_32fc_t *)plheader_symbols, (lv_32fc_t *)plheader_symbols, (lv_32fc_t *)&correlation_buffer[ss], sof.LENGTH + pls.LENGTH);

            double difference = 0;

            // complex_t d = correlate_sof_diff(sof_raw_syms);

            complex_t csof = correlate_sof_diff(plheader_symbols);
            complex_t cplsc = correlate_plscode_diff(&plheader_symbols[sof.LENGTH]);
            complex_t c0 = csof + cplsc; // Best when b7==0 (pilots off)
            complex_t c1 = csof - cplsc; // Best when b7==1 (pilots on)
            complex_t c = c0.norm() > c1.norm() ? c0 : c1;
            complex_t d = c * (1.0f / (26 - 1 + 64 / 2));

            difference = d.norm();
            // sof_difference = difference;

            if (difference > best_match && d.imag > 0)
            {
                best_match = difference;
                best_pos = ss;
                best_match_raw = d;

                current_position = best_pos;

                if (difference > thresold)
                    goto skip_slow_corr;
            }
        }

    skip_slow_corr:

        // logger->info(best_pos);

        if (best_pos != 0 && best_pos < raw_frame_size) // Safety
        {
            int pos = best_pos;
            memmove(&correlation_buffer[0], &correlation_buffer[pos], (raw_frame_size - pos) * sizeof(complex_t));
            ring_buffer.read(&correlation_buffer[raw_frame_size - pos], pos);
            best_pos = 0;
        }

        memcpy(output_stream->writeBuf, correlation_buffer, raw_frame_size * sizeof(complex_t));
        output_stream->swap(raw_frame_size);
    }

    complex_t S2PLSyncBlock::correlate_sof_diff(complex_t *diffs)
    {
        complex_t c = 0;
        const uint32_t dsof = sof.VALUE ^ (sof.VALUE >> 1);
        for (int i = 0; i < sof.LENGTH; ++i)
        {
            // Constant  odd bit => +PI/4
            // Constant even bit => -PI/4
            // Toggled   odd bit => -PI/4
            // Toggled  even bit => +PI/4
            if (((dsof >> (sof.LENGTH - 1 - i)) ^ i) & 1)
                c += diffs[i];
            else
                c -= diffs[i];
        }
        return c;
    }

    complex_t S2PLSyncBlock::correlate_plscode_diff(complex_t *diffs)
    {
        complex_t c = 0;
        uint64_t dscr = pls.SCRAMBLING ^ (pls.SCRAMBLING >> 1);
        for (int i = 1; i < pls.LENGTH; i += 2)
        {
            if ((dscr >> (pls.LENGTH - 1 - i)) & 1)
                c -= diffs[i];
            else
                c += diffs[i];
        }
        return c;
    }
}