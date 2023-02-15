#include "dvbs_defra.h"
//#include "dvbs_defines.h"

//#include "logger.h"
//#include <fstream>
// std::ofstream bits_out("dvbs_vit_bits.bin");

namespace dvbs
{
    DVBSDefra::DVBSDefra(std::shared_ptr<dsp::stream<uint8_t>> input)
        : Block(input)
    {
    }

    DVBSDefra::~DVBSDefra()
    {
    }

    // float dst_to_last = 0;
    // int unsync_cnt = 0;

    void DVBSDefra::work()
    {
#if 1
        int nsamples = input_stream->read();
        if (nsamples <= 0)
        {
            input_stream->flush();
            return;
        }

        int frm_cnt = ts_deframer->work(input_stream->readBuf, nsamples, output_stream->writeBuf);

        // logger->critical(ts2_deframer.getState());

        input_stream->flush();
        output_stream->swap(frm_cnt * 204 * 8);
#else
        int nsamples = input_stream->read();
        if (nsamples <= 0)
        {
            input_stream->flush();
            return;
        }

        bits_out.write((char *)input_stream->readBuf, nsamples);
        // logger->critical(nsamples);

        // frm_cnt = 0;

        for (int biti = 0; biti < nsamples; biti++)
        {
            shifter_ts_sync = shifter_ts_sync << 1 | input_stream->readBuf[biti];

            dst_to_last++;

            if (in_frame)
            {
                write_bit(input_stream->readBuf[biti]);

                if (bit_of_frame == TS_SIZE) // Write frame out
                {
                    memcpy(&output_stream->writeBuf[frm_cnt * (TS_SIZE / 8)], curr_ts_buffer, TS_SIZE / 8);
                    frm_cnt++;

                    if (frm_cnt == 8)
                    {
                        if (polarity_top < 0.5)
                        {
                            for (int i = 0; i < frm_cnt * 204; i++)
                                output_stream->writeBuf[i] ^= 0xFF;
                        }

                        output_stream->swap(frm_cnt * 204);
                        frm_cnt = 0;
                    }
                }
                else if (bit_of_frame == TS_SIZE + TS_ASM_SIZE - 1) // Skip to the next ASM
                {
                    in_frame = false;
                }

                continue;
            }

            if (!synced)
            {
                bool det_pos = shifter_ts_sync == 0x47;
                bool det_neg = shifter_ts_sync == 0xB8;

                if (det_pos || det_neg)
                {
                    logger->critical(dst_to_last);
                    dst_to_last = 0;

                    in_frame = true;
                    curr_ts_buffer[0] = shifter_ts_sync;
                    bit_of_frame = 8;
                    synced = true;

                    if (det_pos)
                        polarity_top = polarity_top * 0.1 + 0.9;
                    else
                        polarity_top = polarity_top * 0.1 + 0.1;
                }
            }
            else
            {
                bool det_pos = compare_8(shifter_ts_sync, 0x47) <= 2;
                bool det_neg = compare_8(shifter_ts_sync, 0xB8) <= 2;

                if ((det_pos || det_neg))
                {
                    logger->critical(dst_to_last);
                    dst_to_last = 0;

                    in_frame = true;
                    curr_ts_buffer[0] = shifter_ts_sync;
                    bit_of_frame = 8;

                    if (det_pos)
                        polarity_top = polarity_top * 0.1 + 0.9;
                    else
                        polarity_top = polarity_top * 0.1 + 0.1;

                    if (dst_to_last != 1632)
                        unsync_cnt++;
                    else
                        unsync_cnt = 0;
                }

                if (unsync_cnt > 10)
                {
                    synced = false;
                }
            }
        }

        input_stream->flush();
#endif
    }
}