#include "csr_reader.h"
#include "common/ccsds/ccsds_time.h"
#include "common/repack.h"
#include "logger.h"

#include "common/utils.h"

namespace ominous
{
    namespace csr
    {
        CSRReader::CSRReader()
        {
            for (int i = 0; i < 10; i++)
                channels[i].resize(2048);
        }

        CSRReader::~CSRReader() {}

        void CSRReader::work(ccsds::CCSDSPacket &pkt)
        {
            if (pkt.payload.size() < 40970)
                return;

            for (int c = 0; c < 10; c++)
                for (int i = 0; i < 2048; i++)
                    channels[c][lines * 2048 + i] = pkt.payload[8 + (i * 10 + c) * 2 + 0] << 8 | pkt.payload[8 + (i * 10 + c) * 2 + 1];

            lines++;

            for (int i = 0; i < 10; i++)
                channels[i].resize((lines + 1) * 2048);
        }

        image::Image CSRReader::getChannel(int channel) { return image::Image(channels[channel].data(), 16, 2048, lines, 1); }
    } // namespace csr
} // namespace ominous