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
#include <vector>
#include <array>
#include <algorithm>
#include <cmath>

#include "common/ccsds/ccsds_weather/vcdu.h"
#include "common/ccsds/ccsds_weather/demuxer.h"

#include "common/image/jpeg12_utils.h"

#include "common/codings/reedsolomon/reedsolomon.h"

#if 0
template <typename T>
std::vector<size_t> idl_where(std::vector<T> x, T y)
{
    std::vector<size_t> v;
    for (size_t i = 0; i < x.size(); i++)
        if (x[i] == y)
            v.push_back(i);
    return v;
}

void image_decomp(image::Image<uint16_t> &img)
{
    float a0 = 1877.5;
    float b0 = 341.0;
    float c0 = -669298;
    float Nc0 = 2048;

    std::vector<float> data(img.width() * img.height());

    for (size_t i = 0; i < img.width() * img.height(); i++)
        data[i] = img[i]; //>> 4;

    std::vector<float> output = data; //(img.width() * img.height());

    auto ss = idl_where<float>(data, Nc0);
    if (ss.size() > 0)
    {
        for (auto &ssi : ss)
            output[ssi] = round(((data[ssi] - a0) * (data[ssi] - a0) - c0) / b0);
    }

    auto sss = idl_where<float>(data, 16383);
    if (sss.size() > 0)
    {
        for (auto &sssi : sss)
            output[sssi] = 16383;
    }

    for (size_t i = 0; i < img.width() * img.height(); i++)
        img[i] = int(output[i]); //<< 4;
}
#endif

int main(int argc, char *argv[])
{
    initLogger();

    uint8_t cadu[1024];

    std::ifstream data_in(argv[1], std::ios::binary);

    std::ofstream data_out(argv[2], std::ios::binary);

    ccsds::ccsds_weather::Demuxer demuxer_vcid4(880, true, 0);

    std::vector<uint8_t> wip_payload;

    int payload_cnt = 0;

    bool started = false;

    reedsolomon::ReedSolomon rs_check(reedsolomon::RS223);

    int skip = -1;

    while (!data_in.eof())
    {
        // Read buffer
        data_in.read((char *)cadu, 1024);

        int errors[4];
        rs_check.decode_interlaved(cadu, true, 4, errors);
        if (errors[0] < 0 || errors[1] < 0 || errors[2] < 0 || errors[3] < 0)
            continue;

        // Parse this transport frame
        ccsds::ccsds_weather::VCDU vcdu = ccsds::ccsds_weather::parseVCDU(cadu);

        // printf("VCID %d\n", vcdu.vcid);

        if (vcdu.vcid == 4)
        {
            // data_out.write((char *)cadu, 1024);

            std::vector<ccsds::CCSDSPacket> ccsdsFrames = demuxer_vcid4.work(cadu);
            for (ccsds::CCSDSPacket &pkt : ccsdsFrames)
            {
                printf("APID %d\n", pkt.header.apid);

                if (pkt.header.apid == 335) // 427) // 427) // 459)
                {

                    //  printf("LEN %d\n", pkt.payload.size());

                    int off = 3; // std::stoi(argv[3]);

                    // logger->critical(off);

                    if (pkt.payload.size() <= 4 + off)
                        continue;

                    // if (pkt.payload[0] != 0x07)
                    //     continue;

                    //  printf("TEST %d %d\n", pkt.payload.size(), pkt.header.packet_length + 1);

                    if (pkt.header.sequence_flag == 0b01) //&& pkt.payload.size() == 268)
                    {
#if 0
                        // Parse Header
                        {
                            uint8_t *header = &pkt.payload[10 + 8];

                            uint8_t datatype = header[0];
                            uint32_t packet_size = header[1] << 16 | header[2] << 8 | header[3];

                            uint16_t x_f = header[18 + 0] << 8 | header[18 + 1];
                            uint16_t y_f = header[20 + 0] << 8 | header[20 + 1];
                            uint16_t b_x = header[22 + 0] << 8 | header[22 + 1];
                            uint16_t b_y = header[24 + 0] << 8 | header[24 + 1];
                            uint16_t x_p = header[26 + 0] << 8 | header[26 + 1];
                            uint16_t y_p = header[28 + 0] << 8 | header[28 + 1];

                            logger->critical("DataType : %d", datatype);
                            logger->critical("PacketSize : %d", packet_size);

                            logger->critical("Full image size (Xf): %d", x_f);
                            logger->critical("Full image size (Yf): %d", y_f);
                            logger->critical("Base point (Xb): %d", b_x);
                            logger->critical("Base point (Yb): %d", b_y);
                            logger->critical("Part image size (Xp): %d", x_p);
                            logger->critical("Part image size (Yp): %d", y_p);
                        }
#endif

                        if (wip_payload.size() > 0)
                        {
#if 0
                            int pos = 0;

                            for (int i = 0; i < wip_payload.size() - 3; i++)
                                if (wip_payload[i + 0] == 0xFF && wip_payload[i + 1] == 0xD8 && wip_payload[i + 2] == 0xFF)
                                    pos = i + 1;

                            wip_payload.erase(wip_payload.begin(), wip_payload.begin() + pos);
#endif

                            auto img = image::decompress_jpeg12(wip_payload.data(), wip_payload.size());
                            //  image_decomp(img);
                            img.save_png("hinode_out/" + std::to_string(payload_cnt++) + ".png");
                            std::ofstream("hinode_out/" + std::to_string(payload_cnt++) + ".jpg", std::ios::binary).write((char *)wip_payload.data(), wip_payload.size());
                        }

                        wip_payload.clear();

                        // wip_payload.insert(wip_payload.end(), pkt.payload.begin() + 4, pkt.payload.end() - off);

                        started = true;
                    }
                    else if (pkt.header.sequence_flag == 0b00 && started)
                    {
                        wip_payload.insert(wip_payload.end(), pkt.payload.begin() + 4, pkt.payload.end() - off);
                    }
                    else if (pkt.header.sequence_flag == 0b10 && started)
                    {
                        wip_payload.insert(wip_payload.end(), pkt.payload.begin() + 4, pkt.payload.end() - off);

                        if (wip_payload.size() > 0)
                        {
#if 0
                            int pos = 0;

                            for (int i = 0; i < wip_payload.size() - 3; i++)
                                if (wip_payload[i + 0] == 0xFF && wip_payload[i + 1] == 0xD8 && wip_payload[i + 2] == 0xFF)
                                    pos = i;

                            wip_payload.erase(wip_payload.begin(), wip_payload.begin() + pos);
#endif

                            auto img = image::decompress_jpeg12(wip_payload.data(), wip_payload.size());
                            // image_decomp(img);
                            img.white_balance();
                            img.save_png("hinode_out/" + std::to_string(payload_cnt++) + ".png");
                            // std::ofstream("hinode_out/" + std::to_string(payload_cnt++) + ".jpg", std::ios::binary).write((char *)wip_payload.data(), wip_payload.size());

                            wip_payload.clear();
                        }

                        started = false;
                    }

                    printf("%d\n", pkt.header.sequence_flag);

                    // if (pkt.payload[4 + 0] == 0xFF && pkt.payload[4 + 1] == 0xD8 && pkt.payload[4 + 2] == 0xFF)
                    //  if (pkt.payload.size() == 2036)
                    // if (pkt.header.sequence_flag == 0b01)
                    if (pkt.payload.size() == pkt.header.packet_length + 1 && skip-- < 0)
                    {
                        printf("LEN %d\n", pkt.payload.size());
                        // pkt.payload.resize(3000);
                        data_out.write((char *)pkt.header.raw, 6);
                        data_out.write((char *)pkt.payload.data(), pkt.payload.size()); // 3000);
                    }
                }
            }
        }
    }
}