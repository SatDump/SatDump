#include "ssmis_reader.h"
#include "common/repack.h"
#include <cstdint>
#include <cstring>
#include <ctime>
#include <vector>

namespace dmsp
{
    namespace ssmis
    {
        SSMISReader::SSMISReader() { lines = 0; }

        SSMISReader::~SSMISReader() {}

        void SSMISReader::work(uint8_t *f, double last_timestamp)
        {
            std::vector<uint8_t> vec2(3375);

            shift_array_left(f, 3375, 5, vec2.data());

            for (auto &v : vec2)
                v ^= 0xFF;

            uint16_t all_words[3000];
            memset(all_words, 0, 2 * 3000);
            repackBytesTo12bits(vec2.data(), vec2.size(), all_words);

            for (int i = 0; i < 3000; i++)
            {
                uint16_t out = 0;
                for (int b = 0; b < 12; b++)
                {
                    uint8_t n = (all_words[i] >> b) & 1;
                    out = out << 1 | n;
                }

                all_words[i] = out;

                all_words[i] <<= 4;
                // all_words[i] = all_words[i] + 32768;
            }

            for (int c = 0; c < 24; c++)
            {
                if (c < 6)
                    ssmis_data[c].resize((lines + 1) * 180);
                else if (c < 11)
                    ssmis_data[c].resize((lines + 1) * 90);
                else
                    ssmis_data[c].resize((lines + 1) * 90);
            }

            for (int s = 0; s < 30; s++)
            {
                uint16_t *ptr = &all_words[90 + 72 * s];

#if 0
        for (int ii = 0; ii < 24; ii++)
        {
            // Channel X
            for (int i = 0; i < 3; i++)
                ssmis_data[ii][(lines * 300) + s * 3 + i] = ptr[ii * 3 + 2 - i];
        }
#else
                // Channel 1
                for (int i = 0; i < 3; i++)
                {
                    ssmis_data[0][(lines * 180) + s * 6 + i] = ptr[2 - i];
                    ssmis_data[0][(lines * 180) + s * 6 + i + 3] = ptr[3 + 2 - i];
                }

                // Channel 2
                for (int i = 0; i < 3; i++)
                {
                    ssmis_data[1][(lines * 180) + s * 6 + i] = ptr[6 + 2 - i];
                    ssmis_data[1][(lines * 180) + s * 6 + i + 3] = ptr[9 + 2 - i];
                }

                // Channel 3
                for (int i = 0; i < 3; i++)
                {
                    ssmis_data[2][(lines * 180) + s * 6 + i] = ptr[12 + 2 - i];
                    ssmis_data[2][(lines * 180) + s * 6 + i + 3] = ptr[15 + 2 - i];
                }

                // Channel 4
                for (int i = 0; i < 3; i++)
                {
                    ssmis_data[3][(lines * 180) + s * 6 + i] = ptr[18 + 2 - i];
                    ssmis_data[3][(lines * 180) + s * 6 + i + 3] = ptr[21 + 2 - i];
                }

                // Channel 5
                for (int i = 0; i < 3; i++)
                {
                    ssmis_data[4][(lines * 180) + s * 6 + i] = ptr[24 + 2 - i];
                    ssmis_data[4][(lines * 180) + s * 6 + i + 3] = ptr[27 + 2 - i];
                }

                // Channel 6
                for (int i = 0; i < 3; i++)
                {
                    ssmis_data[5][(lines * 180) + s * 6 + i] = ptr[30 + 2 - i];
                    ssmis_data[5][(lines * 180) + s * 6 + i + 3] = ptr[33 + 2 - i];
                }

                // Channel 7
                for (int i = 0; i < 3; i++)
                    ssmis_data[6][(lines * 90) + s * 3 + i] = ptr[36 + 2 - i];

                // Channel 8
                for (int i = 0; i < 3; i++)
                    ssmis_data[7][(lines * 90) + s * 3 + i] = ptr[39 + 2 - i];

                // Channel 9
                for (int i = 0; i < 3; i++)
                    ssmis_data[8][(lines * 90) + s * 3 + i] = ptr[42 + 2 - i];

                // Channel 10
                for (int i = 0; i < 3; i++)
                    ssmis_data[9][(lines * 90) + s * 3 + i] = ptr[45 + 2 - i];

                // Channel 11
                for (int i = 0; i < 3; i++)
                    ssmis_data[10][(lines * 90) + s * 3 + i] = ptr[48 + 2 - i];
#endif
            }

            lines++;
            timestamps.push_back(last_timestamp);
        }

        image::Image SSMISReader::getChannel(int id)
        {
            if (id < 6)
                return image::Image(ssmis_data[id].data(), 16, 180, lines, 1);
            else if (id < 11)
                return image::Image(ssmis_data[id].data(), 16, 90, lines, 1);
            else
                return image::Image(ssmis_data[id].data(), 16, 90, lines, 1);
        }

    } // namespace ssmis
} // namespace dmsp
