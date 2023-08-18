/**********************************************************************
 * This file is used for testing random stuff without running the
 * whole of SatDump, which comes in handy for debugging individual
 * elements before putting them all together in modules...
 *
 * If you are an user, ignore this file which will not be built by
 * default, and if you're a developper in need of doing stuff here...
 * Go ahead!
 *
 * Don't judge the code you might see in there! :)
 **********************************************************************/

#include "logger.h"
#include <fstream>

#include "common/ccsds/ccsds_weather/vcdu.h"
#include "common/ccsds/ccsds_weather/demuxer.h"

#include "common/image/image.h"

extern "C"
{
#include <libs/aec/szlib.h>
}

int main(int argc, char *argv[])
{
    initLogger();

    std::ifstream data_in(argv[1], std::ios::binary);
    std::ofstream data_out(argv[2], std::ios::binary);

    uint8_t cadu[1024];

    ccsds::ccsds_weather::Demuxer demuxer_vcid1(884, true, 0);
    ccsds::ccsds_weather::Demuxer demuxer_vcid2(884, true, 0);

    uint16_t *suvi_img_buffer = new uint16_t[4074 * 423];
    int suvi_img_cnt = 0;

    while (!data_in.eof())
    {
        data_in.read((char *)cadu, 1024);

        // Parse this transport frame
        ccsds::ccsds_weather::VCDU vcdu = ccsds::ccsds_weather::parseVCDU(cadu);

        printf("VCID %d\n", vcdu.vcid);

        if (vcdu.vcid == 2)
        {
            // data_out.write((char *)cadu, 1024);

            std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid2.work(cadu);
            for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
            {
                printf("APID %d\n", pkt.header.apid);

                if (pkt.header.apid == 810)
                {
                    // atms_reader.work(pkt);
                    // printf("LEN %d\n", pkt.header.packet_length);
                    // pkt.payload.resize(8186);
                    // data_out.write((char *)pkt.header.raw, 6);
                    // data_out.write((char *)pkt.payload.data(), 8186);

                    int pkt_counter = pkt.payload[22] << 8 | pkt.payload[23];

                    // printf("cnt1 %d\n", pkt_counter);

                    if (pkt_counter < 423)
                    {
                        for (int i = 0; i < 4074; i++)
                            suvi_img_buffer[pkt_counter * 4074 + i] =
                                pkt.payload[38 + i * 2 + 1] << 8 | pkt.payload[38 + i * 2 + 0];
                    }

                    if (pkt_counter == 422)
                    {
                        image::Image<uint16_t> suvi_img(suvi_img_buffer + 105, 1330, 1295, 1);
                        suvi_img.crop(0, 3, 0 + 1280, 3 + 1284);
                        for (int i = 0; i < suvi_img.size(); i++)
                            suvi_img[i] = suvi_img.clamp(int(suvi_img[i]) << 5);
                        suvi_img.save_png("SUVI_RAW/suvi_img_" + std::to_string(suvi_img_cnt++) + ".png");
                    }
                }
            }
        }
        if (vcdu.vcid == 1)
        {
            // data_out.write((char *)cadu, 1024);

            std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid2.work(cadu);
            for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
            {
                if (pkt.header.apid == 505)
                {
                    printf("LEN %d\n", pkt.header.packet_length);
#if 0
                    pkt.payload.resize(8186);
                    data_out.write((char *)pkt.header.raw, 6);
                    data_out.write((char *)pkt.payload.data(), 8186);
#else
                    SZ_com_t rice_parameters;
                    rice_parameters.bits_per_pixel = 13;
                    rice_parameters.pixels_per_block = 32;
                    rice_parameters.pixels_per_scanline = 408; // 128 * 32;
                    rice_parameters.options_mask = SZ_ALLOW_K13_OPTION_MASK | SZ_MSB_OPTION_MASK | SZ_NN_OPTION_MASK | SZ_RAW_OPTION_MASK;

                    std::vector<uint8_t> ppayload = pkt.payload;

                    pkt.payload.resize(8186);

                    int off = 30;

#if 0
                    size_t destlen = pkt.payload.size() - off;
                    SZ_BufftoBuffDecompress(pkt.payload.data() + off,
                                            &destlen,
                                            ppayload.data() + off,
                                            ppayload.size() - off,
                                            &rice_parameters);
#else
                    size_t destlen = pkt.payload.size() - off;
                    aec_stream aec_cfg;

                    aec_cfg.bits_per_sample = 13;
                    aec_cfg.block_size = 32;
                    aec_cfg.rsi = 128;
                    aec_cfg.flags = AEC_DATA_MSB | AEC_DATA_PREPROCESS;

                    aec_cfg.next_in = (const unsigned char *)&ppayload[off];
                    aec_cfg.avail_in = ppayload.size() - off - 1;
                    aec_cfg.next_out = (unsigned char *)pkt.payload.data();
                    aec_cfg.avail_out = destlen;
                    aec_decode_init(&aec_cfg);
                    aec_decode(&aec_cfg, AEC_FLUSH);
                    aec_decode_end(&aec_cfg);
#endif

                    // for (int i = 0; i < (pkt.payload.size() - off) / 2; i++)
                    //     pkt.payload[off + i] = (pkt.payload[off + i * 2 + 0] << 8 | pkt.payload[off + i * 2 + 1]) >> 5;

                    data_out.write((char *)pkt.header.raw, 6);
                    data_out.write((char *)pkt.payload.data(), 8186);
#endif
                }
            }
        }
    }
}