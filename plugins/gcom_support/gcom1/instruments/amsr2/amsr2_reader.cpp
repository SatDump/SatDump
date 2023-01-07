#include "amsr2_reader.h"
#include "common/ccsds/ccsds_time.h"
#include "common/repack.h"

namespace gcom1
{
    namespace amsr2
    {
        AMSR2Reader::AMSR2Reader()
        {
            current_pkt = 0;
            lines = 0;
            for (int i = 0; i < 20; i++)
                channels[i].resize(243);
        }

        AMSR2Reader::~AMSR2Reader()
        {
        }

        void AMSR2Reader::work(ccsds::CCSDSPacket &packet)
        {
            if (packet.payload.size() < 1018)
                return;

            if (current_pkt < 20)
            {
                int nch = 20;
                int npx = 25;
                for (int i = 0; i < npx; i++)
                {
                    for (int c = 0; c < 20; c++)
                    {
                        int off = 10 + 2 * (i * nch + c);

                        uint16_t val = packet.payload[off + 0] << 8 | packet.payload[off + 1];

                        bool sign = (val >> 11) & 1;
                        val = val & 0b11111111111;

                        if (!sign)
                            val = 2048 + val;
                        // else
                        //     val = 2048 - val;

                        int px = current_pkt * npx + i;
                        if (px < 243)
                            channels[c][lines * 243 + px] = val << 4;
                    }
                }
            }

            current_pkt++;

            if (packet.header.sequence_flag == 0b01)
            {
                lines++;
                current_pkt = 0;
            }

            for (int i = 0; i < 20; i++)
                channels[i].resize((lines + 1) * 243);
        }

        image::Image<uint16_t> AMSR2Reader::getChannel(int c)
        {
            return image::Image<uint16_t>(channels[c].data(), 243, lines, 1);
        }
    } // namespace swap
} // namespace proba