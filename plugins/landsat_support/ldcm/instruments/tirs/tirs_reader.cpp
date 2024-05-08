#include "tirs_reader.h"
#include "common/ccsds/ccsds_time.h"
#include "common/repack.h"
#include "logger.h"

namespace ldcm
{
    namespace tirs
    {
        TIRSReader::TIRSReader()
        {
            for (int i = 0; i < 3; i++)
                channels[i].resize(1280);
        }

        TIRSReader::~TIRSReader()
        {
        }

        void TIRSReader::work(ccsds::CCSDSPacket &pkt)
        {
            if (pkt.payload.size() < 5848)
                return;

            repackBytesTo12bits(&pkt.payload[31], 5848 - 31, tirs_line);

            for (int i = 0; i < 640; i++)
                channels[0][lines * 1280 + i * 2 + 0] = tirs_line[i] << 4;
            for (int i = 0; i < 640; i++)
                channels[0][lines * 1280 + i * 2 + 1] = tirs_line[1941 + i] << 4;

            for (int i = 0; i < 640; i++)
                channels[1][lines * 1280 + i * 2 + 0] = tirs_line[647 + i] << 4;
            for (int i = 0; i < 640; i++)
                channels[1][lines * 1280 + i * 2 + 1] = tirs_line[2588 + i] << 4;

            for (int i = 0; i < 640; i++)
                channels[2][lines * 1280 + i * 2 + 0] = tirs_line[1294 + i] << 4;
            for (int i = 0; i < 640; i++)
                channels[2][lines * 1280 + i * 2 + 1] = tirs_line[3235 + i] << 4;

            lines++;

            for (int i = 0; i < 3; i++)
                channels[i].resize((lines + 1) * 1280);
        }

        image::Image TIRSReader::getChannel(int channel)
        {
            return image::Image(channels[channel].data(), 16, 1280, lines, 1);
        }
    }
}