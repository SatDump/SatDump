#include "maestro_reader.h"
#include "common/ccsds/ccsds_time.h"
#include "common/repack.h"

#include "logger.h"

namespace scisat1
{
    namespace maestro
    {
        MaestroReader::MaestroReader()
        {
            img_data1.resize(8196);
            img_data2.resize(8196);

            lines_1 = 0;
            lines_2 = 0;
        }

        MaestroReader::~MaestroReader()
        {
        }

        void MaestroReader::work(ccsds::CCSDSPacket &packet)
        {
            uint16_t mode_marker = packet.payload[1003] << 8 | packet.payload[1004];

            if (packet.payload.size() >= 17450) // Filter imagery
            {
                if (mode_marker == 0x18c1) // First mode
                {
                    repackBytesTo16bits(packet.payload.data() + 1054, 8196 * 2, &img_data1[lines_1 * 8196]);
                    lines_1++;
                    img_data1.resize((lines_1 + 1) * 8196);
                }
                else if (mode_marker == 0x0000) // Second mode
                {
                    repackBytesTo16bits(packet.payload.data() + 1054, 8196 * 2, &img_data2[lines_2 * 8196]);
                    lines_2++;
                    img_data2.resize((lines_2 + 1) * 8196);
                }
            }
        }

        image::Image MaestroReader::getImg1()
        {
            return image::Image(img_data1.data(), 16, 8196, lines_1, 1);
        }

        image::Image MaestroReader::getImg2()
        {
            return image::Image(img_data2.data(), 16, 8196, lines_2, 1);
        }
    } // namespace swap
} // namespace proba