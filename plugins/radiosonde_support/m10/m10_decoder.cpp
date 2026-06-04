#include "m10_decoder.h"
#include "m10.h"
#include "utils/binary.h"
#include <cstdint>

namespace satdump
{
    namespace ndsp
    {
        M10DecoderBlock::M10DecoderBlock() : Block("m10_decoder_hh", {{"in", DSP_SAMPLE_TYPE_S8}}, {{"out", DSP_SAMPLE_TYPE_S8}}) {}

        void M10DecoderBlock::init() { frm_cnt = 0; }

        M10DecoderBlock::~M10DecoderBlock() {}

        inline int corr_16(uint16_t v1, uint16_t v2)
        {
            int cor = 0;
            uint16_t diff = v1 ^ v2;
            for (; diff; cor++)
                diff &= diff - 1;
            return cor;
        }

        bool M10DecoderBlock::work()
        {
            DSPBuffer iblk = inputs[0].fifo->wait_dequeue();

            if (iblk.isTerminator())
            {
                if (iblk.terminatorShouldPropagate())
                    outputs[0].fifo->wait_enqueue(outputs[0].fifo->newBufferTerminator());
                inputs[0].fifo->free(iblk);
                return true;
            }

            uint8_t *ibuf = iblk.getSamples<uint8_t>();

            const auto sync = satdump::unsigned_to_bitvec<uint64_t>(radiosonde::M10_SYNCWORD);

            for (int ib = 0; ib < iblk.size; ib++)
            {
                memmove(shift_buf, shift_buf + 1, 1664 - 1);
                shift_buf[1663] = ibuf[ib];

                int best_corr = 0;
                int best_corr_pos = -1;
                for (int ii = 0; ii < 1664 - 48; ii++)
                {
                    int corr = 0;
                    for (int i = 0; i < 48; i++)
                        if (shift_buf[ii + i] == sync[ii + i])
                            corr++;

                    if (best_corr < corr)
                    {
                        best_corr = corr;
                        best_corr_pos = ii;
                    }
                }

                if (best_corr_pos == 0)
                {
                    DSPBuffer oblk = outputs[0].fifo->newBufferSamples(104, sizeof(int8_t));
                    uint8_t *obuf = oblk.getSamples<uint8_t>();
                    oblk.size = 104;

                    for (int i = 0; i < 1664; i++)
                        obuf2[i / 8] = obuf2[i / 8] << 1 | shift_buf[i];

                    radiosonde::m10_manchester_decode(obuf2, 208, obuf);
                    radiosonde::m10_frame_descramble(obuf);

                    if (((radiosonde::M10Frame *)obuf)->len != 0 && radiosonde::m10_frame_crccheck((radiosonde::M10Frame *)obuf))
                    {
                        outputs[0].fifo->wait_enqueue(oblk);
                        frm_cnt++;
                    }
                    else
                    {
                        outputs[0].fifo->free(oblk);
                    }
                }
            }

            inputs[0].fifo->free(iblk);

            return false;
        }
    } // namespace ndsp
} // namespace satdump