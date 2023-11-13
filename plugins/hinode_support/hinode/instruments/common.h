#pragma once

#include "common/ccsds/ccsds.h"
#include "common/image/image.h"
#include "nlohmann/json.hpp"

/*
This whole part of the code is an implementation of the
Hinode JPEG image restoration algorithm, which turns out
to be absolutely required to get usable data out.
It is possible to extract data without this but even with
perfect SNR data still needs restoration! I did not want
to rewrite it all from scratch! :-)

It can be found at :
- https://data.darts.isas.jaxa.jp/pub/solarb/eis/staging/
- https://hesperia.gsfc.nasa.gov/ssw/

---------------------------------------------------------------------------------------------
Copyright 2020 SolarSoft Consortium

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
---------------------------------------------------------------------------------------------
*/
namespace hinode
{
    struct ScienceHeader
    { /* science data header */
        int DataType;
        int PacketSize;
        int SerialPacketNo;
        int MainID;
        int MainSQFlag;
        int MainSQCount;
        int NumOfPacket;
        int NumOfFrame;
        int SubID;
        int SubSQFlag;
        int SubSQCount;
        int FullImageSizeX;
        int FullImageSizeY;
        int BasePointCoorX;
        int BasePointCoorY;
        int PartImageSizeX;
        int PartImageSizeY;
        int BitCompMode;
        int ImageCompMode;
        int HTACNo;
        int HTDCNo;
        int QTNo;
        // unsigned char DataInfo[256];

        int restart_pixel; // number of pixels for restart, valid if ImageCompMode != 0
        int num_segment;   // number of restart segment in a image, valid if ImageCompMode != 0

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(ScienceHeader,
                                       DataType,
                                       PacketSize,
                                       SerialPacketNo,
                                       MainID,
                                       MainSQFlag,
                                       MainSQCount,
                                       NumOfPacket,
                                       NumOfFrame,
                                       SubID,
                                       SubSQFlag,
                                       SubSQCount,
                                       FullImageSizeX,
                                       FullImageSizeY,
                                       BasePointCoorX,
                                       BasePointCoorY,
                                       PartImageSizeX,
                                       PartImageSizeY,
                                       BitCompMode,
                                       ImageCompMode,
                                       HTACNo,
                                       HTDCNo,
                                       QTNo)
    };

    // Struct to hold a decoded image from Hinode
    struct DecodedImage
    {
        int apid = -1;
        ScienceHeader sci;
        image::Image<uint16_t> img;
    };

    class HinodeDepacketizer
    {
    private:
        struct rst_tbl_t
        {
            int pos;
            int No;
            int flg;
        };

    private:
        int before_sq_flag = -1;
        int before_sq_count = 1;
        int status;

        ScienceHeader sci_hdr;

        int ECS0_start = 0;
        int num_RST = 0;
        rst_tbl_t *rst_tbl_array;
        int restored_flag = 0;
        unsigned char *_chk_tbl;

        int buff_pos = 0;
        uint8_t *buffer;

    private:
        ScienceHeader parse_science_header(ccsds::CCSDSPacket &pkt);

        void fill_gap(const ccsds::CCSDSPacket &pkt, int data_length, int packetpos, int rst2ndNo);
        void clear_chktbl();
        void fill_chktbl();
        int trunc_recovered();
        void fill_tail();
        void add_rst(int p1, int p2, int p3);
        void add_eoi();

        void insert_dct_header();
        void insert_dpcm_header();

    public:
        HinodeDepacketizer();
        ~HinodeDepacketizer();

        int work(ccsds::CCSDSPacket &pkt, DecodedImage *result);

        int img_cnt = 0;
    };

    class ImageRecomposer
    {
    private:
        int current_main_id = -1;
        image::Image<uint16_t> full_image;

    public:
        int pushSegment(DecodedImage &seg, image::Image<uint16_t> *img)
        {
            if (seg.sci.FullImageSizeX == seg.sci.PartImageSizeX &&
                seg.sci.FullImageSizeY == seg.sci.PartImageSizeY)
                return 0;

            int ret = 0;

            if (seg.sci.MainID != current_main_id)
            {
                *img = full_image;
                ret = current_main_id;
                current_main_id = seg.sci.MainID;
                full_image.init(seg.sci.FullImageSizeX, seg.sci.FullImageSizeY, 1);
            }

            full_image.draw_image(0, seg.img, seg.sci.BasePointCoorX, seg.sci.BasePointCoorY);

            return ret;
        }

        image::Image<uint16_t> &getImage()
        {
            return full_image;
        }
    };
}