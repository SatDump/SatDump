#include "gps_ascii.h"
#include "common/ccsds/ccsds_time.h"

namespace proba
{
    namespace gps_ascii
    {
        GPSASCII::GPSASCII(std::string path)
        {
            output_txt = std::ofstream(path, std::ios::binary);
        }

        GPSASCII::~GPSASCII()
        {
            output_txt.close();
        }

        void GPSASCII::work(ccsds::CCSDSPacket &packet)
        {
            if (packet.payload.size() < 19)
                return;

            output_txt.write((char *)&packet.payload[18], packet.payload.size() - 18 - 3);
            output_txt.put('\n');
        }
    } // namespace swap
} // namespace proba