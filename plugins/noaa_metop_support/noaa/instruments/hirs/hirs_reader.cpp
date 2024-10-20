#include "hirs_reader.h"
#include <cmath>
#include "common/repack.h"
// #include "iostream"

namespace noaa
{
    namespace hirs
    {
        HIRSReader::HIRSReader(int year) : ttp(year)
        {
            for (int i = 0; i < 20; i++)
                channels[i].resize(56);
            c_sequences.resize(1);
        }

        HIRSReader::~HIRSReader()
        {
            for (int i = 0; i < 20; i++)
                channels[i].clear();
            // delete[] imageBuffer;
        }

        void HIRSReader::work(uint8_t *buffer)
        {
            // get TIP timestamp
            uint16_t mf = ((buffer[4] & 1) << 8) | (buffer[5]);

            if (mf == 0)
            {
                int days = (buffer[8] << 1) | (buffer[9] >> 7);
                uint64_t millisec = ((buffer[9] & 0b00000111) << 24) | (buffer[10] << 16) | (buffer[11] << 8) | buffer[12];
                last_timestamp = ttp.getTimestamp(days, millisec);
            }

            uint8_t HIRS_data[36] = {};
            int pos = 0;
            for (int j : HIRSPositions)
            {
                HIRS_data[pos] = buffer[j];
                pos++;
                // std::cout<<pos<<std::endl;
            }

            uint16_t elnum = ((HIRS_data[2] % (int)pow(2, 5)) << 1) | (HIRS_data[3] >> 7);
            uint8_t encoder = HIRS_data[0];
            // std::cout << "element number:" << elnum << " encoder position:" << (unsigned int)HIRS_data[0] << std::endl;

            if (elnum < 56 && (HIRS_data[35] & 0b10) >> 1)
            {
                aux_counter++;
                sync += ((HIRS_data[3] & 0x40) >> 6);
                int current = ((buffer[22] % (int)pow(2, 5)) << 1) | (buffer[23] >> 7);
                // std::cout<<last << ", " << elnum << ", " << current <<std::endl;

                uint16_t words13bit[20] = {0};
                uint8_t tmp[32];
                shift_array_left(&HIRS_data[3], 32, 2, tmp);
                repackBytesTo13bits(tmp, 32, words13bit);

                for (int i = 0; i < 20; i++)
                    channels[HIRSChannels[i]][55 - elnum + 56 * line] = words13bit[i];

                for (int i = 0; i < 20; i++)
                {
                    if (encoder < 57)
                    {
                        if ((channels[i][55 - elnum + 56 * line] >> 12) == 1)
                        {
                            channels[i][55 - elnum + 56 * line] = (channels[i][55 - elnum + 56 * line] & 0b0000111111111111) + 4095;
                        }
                        else
                        {
                            int buffer = 4096 - ((channels[i][55 - elnum + 56 * line] & 0b0000111111111111));
                            channels[i][55 - elnum + 56 * line] = abs(buffer);
                        }
                    }
                    else
                    {
                        if (encoder == 68)
                            spc_calib++;
                        if (encoder == 156)
                            bb_calib++;
                        // channels[i][55 - elnum + 56 * line] = 0;
                    }
                }

                if (current == 55 || (encoder == 0 && aux_counter > 10)) // 10 was choosen for best results
                {
                    if (spc_calib > 40)

                        aux_counter = 0;
                    line++;
                    for (int l = 0; l < 20; l++)
                        channels[l].resize((line + 1) * 56);
                    if (!contains(timestamps, last_timestamp + (double)(mf / 64) * (last_timestamp != -1 ? 6.4 : 0)))
                        timestamps.push_back(last_timestamp + (double)(mf / 64) * (last_timestamp != -1 ? 6.4 : 0));
                    else
                        timestamps.push_back(-1);
                }
                // last = elnum;
            }
        }

        image::Image HIRSReader::getChannel(int channel)
        {
            return image::Image(channels[channel].data(), 16, 56, line, 1);
        }

        // #################
        // ## calib stuff ##
        // #################

        uint16_t calib_sequence::calc_avg(uint16_t *samples, int count)
        {
            /*
            This function calcualtes the average value for space and bb looks, using the 3-sigma criterion to discard bad samples
            */
            double mean = 0;
            double variance = 0;
            uint8_t ignored = 0;

            // calculate the mean
            for (int i = 0; i < count; i++)
            {
                if (samples[i] != 0)
                    mean += samples[i];
                else
                    ignored++;
            }
            mean /= (count - ignored);

            for (int i = 0; i < count; i++)
                if (samples[i] != 0)
                    variance += pow(samples[i] - mean, 2) / (count - ignored);

            std::pair<uint16_t, uint16_t> range = {mean - 3 * pow(variance, 0.5), mean + 3 * pow(variance, 0.5)};
            uint32_t avg = 0;
            uint8_t cnt = 0;

            for (int i = 0; i < count; i++)
            {
                if (samples[i] >= range.first && samples[i] <= range.second)
                {
                    avg += samples[i];
                    cnt++;
                }
            }
            if (cnt != 0)
                avg /= cnt;

            return avg;
        }

    } // namespace hirs
} // namespace noaa