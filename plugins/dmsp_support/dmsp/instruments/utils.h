#pragma once

#include "common/simple_deframer.h"
#include "utils/binary.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <map>
#include <vector>

namespace dmsp
{
    struct RTDFrame
    {
        uint16_t sync;       // 13 bits of sync
        bool tag_bit;        // 1 tag bit
        uint8_t fine[15];    // 15 6-bits samples of fine data
        uint8_t smooth[3];   // 3 8-bits samples of smooth data
        uint8_t transition;  // 6 transition bits
        uint8_t wow_flutter; // 8-bits wow/flutter
        uint8_t terdats;     // 8-bits TERDATS

        RTDFrame(uint8_t *frm)
        {
            sync = 0;
            tag_bit = 0;
            memset(fine, 0, 15);
            memset(smooth, 0, 3);
            transition = 0;
            wow_flutter = 0;
            terdats = 0;

            // Not super efficient, but a good way to do it easily
            for (int i = 0; i < 150; i++)
            {
                uint8_t b = (frm[i / 8] >> (7 - (i % 8))) & 1;

                if (i < 13)
                    sync = sync << 1 | b;
                else if (i == 13)
                    tag_bit = b;
                else if (i < 104)
                {
                    int pos = (i - 14);
                    fine[pos / 6] = fine[pos / 6] << 1 | b;
                }
                else if (i < 128)
                {
                    int pos = (i - 104);
                    smooth[pos / 8] = smooth[pos / 8] << 1 | b;
                }
                else if (i < 134)
                    transition = transition << 1 | b;
                else if (i < 142)
                    wow_flutter = wow_flutter << 1 | b;
                else if (i < 150)
                    terdats = terdats << 1 | b;
            }
        }
    };

#define TERDATS_NODATA 0b00
#define TERDATS_DMDM 0b01
#define TERDATS_SSP 0b10
#define TERDATS_UNUSED 0b11

#define SENSORID_SSMIS 0b0100
#define SENSORID_SSF 0b1010
#define SENSORID_SSM 0b1001
#define SENSORID_SSIES3 0b0001
#define SENSORID_SSJ5 0b0111
#define SENSORID_SSULI 0b1000
#define SENSORID_SSUSI 0b1011

    class TERDATSDemuxer
    {
    private:
        uint8_t terdats_shifter = 0;
        int in_terdats_shifter = 0;
        def::SimpleDeframer terdats_def;

    private:
        struct ChannelAccumulator
        {
            uint8_t shifter = 0;
            uint8_t in_shifter = 0;
        };

        std::map<int, ChannelAccumulator> accumulators;

    public:
        TERDATSDemuxer() : terdats_def(0b1111010001110101111101000111010111110100011101011111010001110101, 64, 32768, 0) {}

        std::map<int, std::vector<uint8_t>> work(RTDFrame &f)
        {
            std::map<int, std::vector<uint8_t>> r;

            if ((f.terdats & 0b11) == TERDATS_SSP)
            {
                for (int i = 0; i < 6; i++)
                {
                    uint8_t b = ((f.terdats >> 2) >> (5 - i)) & 1;

                    terdats_shifter = terdats_shifter << 1 | b;
                    in_terdats_shifter++;

                    if (in_terdats_shifter == 8)
                    {
                        in_terdats_shifter = 0;
                        //  data_ou.write((char *)&shifter, 1);

                        auto nf = terdats_def.work(&terdats_shifter, 1);
                        for (auto ff : nf)
                        {
                            // The frame header is reversed 16-bit words!
                            for (int cc = 0; cc < 18 * 2; cc += 2)
                            {
                                uint8_t v1 = satdump::reverseBits(ff[cc + 0]);
                                uint8_t v2 = satdump::reverseBits(ff[cc + 1]);
                                ff[cc + 1] = v1;
                                ff[cc + 0] = v2;
                            }

                            std::vector<std::pair<int, int>> sects;

                            // Payloads are dynamically allocated (on command), parse the header to ID them & extract
                            printf("Format Words : ");
                            for (int c = 0; c < 12; c++)
                            {
                                uint16_t fword = ff[12 + c * 2 + 0] << 8 | ff[12 + c * 2 + 1];
                                int no_of_36bits_words = ((fword >> 5) & 0b1111111111) * 36;
                                int sensor_id = fword & 0xF;
                                printf(" %d (%d), ", sensor_id, no_of_36bits_words);
                                sects.push_back({sensor_id, no_of_36bits_words});
                            }
                            printf("\n");

                            // They are also randomized!
                            for (int cc = 18 * 2; cc < ff.size(); cc++)
                                ff[cc] ^= 0b10101010;

                            // Extract, bit-wise as it can be be non-byte amounts!
                            int bitpos = 0;
                            for (auto &sec : sects)
                            {
                                if (accumulators.count(sec.first) == 0)
                                    accumulators.emplace(sec.first, ChannelAccumulator());
                                auto &acc = accumulators[sec.first];

                                for (int i = 0; i < sec.second; i++)
                                {
                                    uint8_t b = (ff[36 + (bitpos / 8)] >> (7 - (bitpos % 8))) & 1;
                                    bitpos++;

                                    acc.shifter = acc.shifter << 1 | b;
                                    acc.in_shifter++;

                                    if (acc.in_shifter == 8)
                                    {
                                        acc.in_shifter = 0;

                                        if (r.count(sec.first) == 0)
                                            r.emplace(sec.first, std::vector<uint8_t>());
                                        r[sec.first].push_back(acc.shifter);
                                    }
                                }
                            }

                            //    logger->info(sensor_id);
                            // data_ou.write((char *)ff.data(), 32768 / 8);
                        }
                    }
                }
            }

            return r;
        }
    };
} // namespace dmsp