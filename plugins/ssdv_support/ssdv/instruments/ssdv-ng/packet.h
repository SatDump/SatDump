#pragma once

#include "common/ccsds/ccsds.h"
#include "image/image.h"
#include "nlohmann/json.hpp"
#include <cstdint>

namespace ssdv
{
    struct PacketHeader
    {
        uint16_t ImageID;
        uint32_t PacketID;
        uint8_t ImageWidth;
        uint8_t ImageHeight;
        uint8_t Flags;
        uint8_t HuffmanProfile;
        uint8_t eoi;
        uint8_t Quality;
        uint8_t MCUMode;
        uint16_t MCUOffset;
        uint32_t MCUID;
        uint32_t MCUCount;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(PacketHeader,   //
                                       ImageID,        //
                                       PacketID,       //
                                       ImageWidth,     //
                                       ImageHeight,    //
                                       Flags,          //
                                       HuffmanProfile, //
                                       eoi,            //
                                       Quality,        //
                                       MCUMode,        //
                                       MCUOffset,      //
                                       MCUID,          //
                                       MCUCount)       //
    };

    // Struct to hold decoded images
    struct DecodedImage
    {
        PacketHeader pack;
        image::Image img;
    };

    class SSDVNGDepacketizer
    {
    private:
        int status;

        PacketHeader pack_hdr;

        int buffer_pos = 0;
        uint8_t *buffer;

    private:
        PacketHeader parse_packet_header(ccsds::CCSDSPacket &pkt);

        void insert_dct_header();

    public:
        SSDVNGDepacketizer();
        ~SSDVNGDepacketizer();

        int work(ccsds::CCSDSPacket &pkt, DecodedImage *result);

        int img_cnt = 0;
    };

} // namespace ssdv
