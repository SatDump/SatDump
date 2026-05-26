#include "ssdv.h"

namespace ssdv
{
    namespace ssdvng
    {
        SSDVNGReader::SSDVNGReader() {}
        SSDVNGReader::~SSDVNGReader() {}

        void SSDVNGReader::work(ccsds::CCSDSPacket &packet)
        {
            // if (packet.payload.size() < 256)
            //     return;

            // full_pkt.insert(full_pkt.end(), packet.payload.begin(), packet.payload.end());

            // uint16_t image_id = packet.payload[0] << 8 | packet.payload[1];
            // uint32_t packet_id = packet.payload[2] << 16 | packet.payload[3] << 8 | packet.payload[4];
            // uint8_t img_width = packet.payload[5] << 4;
            // uint8_t img_height = packet.payload[6] << 4;
            // uint16_t mcu_count = packet.payload[5] * packet.payload[6];
            // uint8_t flags = packet.payload[7];
            // uint8_t huffman_profile = packet.payload[7] >> 7 & 0b1;
            // uint8_t quality = ((packet.payload[7] >> 4) & 0b111) ^ 0b100;
            // uint8_t mcu_mode = packet.payload[7] & 0b11;
            // uint16_t mcu_offset = packet.payload[8] << 8 | packet.payload[9];
            // uint32_t mcu_id = packet.payload[10] << 16 | packet.payload[11] << 8 | packet.payload[12];
            //
            // uint8_t *img = &packet.payload[13];

            // uint8_t *dat = &packet.payload[0];

            // if (packet.payload.size() )
        }
    } // namespace ssdvng
} // namespace ssdv
