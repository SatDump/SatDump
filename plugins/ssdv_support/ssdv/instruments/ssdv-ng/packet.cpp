#include "packet.h"
#include <cstdint>

namespace ssdv
{
    PacketHeader SSDVNGDepacketizer::parse_packet_header(ccsds::CCSDSPacket &pkt)
    {
        PacketHeader ph;

        uint8_t *p = &pkt.payload[0];
        ph.ImageID = p[0] << 8 | p[1];               // ImageID
        ph.PacketID = p[2] << 16 | p[3] << 8 | p[4]; // PacketID
        ph.ImageWidth = p[5] << 4;                   // ImageWidth
        ph.ImageHeight = p[6] << 4;                  // ImageHeight
        ph.Flags = p[7];                             // Flags
        ph.HuffmanProfile = (p[7] >> 6) & 0b1;       // HuffmanProfile
        ph.eoi = (p[7] >> 2) & 0b1;                  // eoi
        ph.Quality = ((p[7] >> 3) & 0b111) ^ 0b100;  // Quality
        ph.MCUMode = p[7] & 0b11;                    // MCUMode
        ph.MCUOffset = p[8] << 8 | p[9];             // MCUOffset
        ph.MCUID = p[10] << 16 | p[11] << 8 | p[12]; // MCUID
        ph.MCUCount = p[5] * p[6];                   // MCUCount

        {
            //
            //
        }

        return ph;
    }

    SSDVNGDepacketizer::SSDVNGDepacketizer()
    {
        // buffer = new uint8_t[]
    }

    SSDVNGDepacketizer::~SSDVNGDepacketizer()
    {
        // delete[] buffer;
    }

} // namespace ssdv
