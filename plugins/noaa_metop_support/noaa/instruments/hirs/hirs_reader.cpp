#include "hirs_reader.h"
#include <cmath>
#include "common/repack.h"
#include "common/calibration.h"
// #include "iostream"

namespace noaa
{
    namespace hirs
    {
        HIRSReader::HIRSReader(int year) : ttp(year)
        {
            for (int i = 0; i < 20; i++)
                channels[i].resize(56);
            c_sequences = {calib_sequence()};
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
                uint8_t tmp[33];
                shift_array_left(&HIRS_data[3], 33, 2, tmp);
                repackBytesTo13bits(tmp, 33, words13bit);

                for (int i = 0; i < 20; i++)
                    channels[HIRSChannels[i]][55 - elnum + 56 * line] = words13bit[i];

                for (int i = 0; i < 20; i++)
                {
                    if (encoder < 57 || encoder == 68 || encoder == 156 || encoder == 59 || encoder == 99)
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

                        if (encoder == 68 || encoder == 59)
                            spc_calib++;
                        if (encoder == 156 || encoder == 99)
                            bb_calib++;
                    }
                }

                if (current == 55 || (encoder == 0 && aux_counter > 10)) // 10 was choosen for best results
                {
                    if (spc_calib > 500)
                    {
                        spc_calib = 0;
                        c_sequences[c_sequences.size() - 1].spc_ready = true;
                        for (int c = 0; c < 19; c++)
                        {
                            c_sequences[c_sequences.size() - 1].calc_space(&channels[c][56 * line], c);
                            if(c_sequences[c_sequences.size() - 1].space[c]!=0){
                                for (int i = 0; i < 56; i++)
                                    channels[c][i + 56 * line] = 0;
                            }
                        }
                        for (int i = 0; i < 56; i++)
                            channels[19][i + 56 * line] = 0;
                    }
                    if (bb_calib > 500)
                    {
                        bb_calib = 0;
                        c_sequences[c_sequences.size() - 1].bb_ready = true;
                        for (int c = 0; c < 19; c++)
                        {
                            c_sequences[c_sequences.size() - 1].calc_bb(&channels[c][56 * line], c);
                            for (int i = 0; i < 56; i++)
                                channels[c][i + 56 * line] = 0;
                        }
                        for (int i = 0; i < 56; i++)
                            channels[19][i + 56 * line] = 0;
                    }

                    if (c_sequences[c_sequences.size() - 1].is_ready())
                    {
                        c_sequences[c_sequences.size() - 1].position = line;
                        c_sequences.push_back(calib_sequence());
                    }

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
            if (elnum == 58 || elnum == 59)
            {

                uint16_t words13bit[20] = {0};
                uint8_t tmp[33];
                shift_array_left(&HIRS_data[3], 33, 2, tmp);
                repackBytesTo13bits(tmp, 33, words13bit);
                if (elnum == 58)
                {
                    for (int i = 0; i < 20; i += 5)
                    {
                        if ((int)PRT_counts[i / 5].size() - line < -1)
                            PRT_counts[i / 5].push_back(0);
                        PRT_counts[i / 5].push_back(calib_sequence::calc_avg(&words13bit[i], 5));
                    }
                }
                else
                {
                    if ((int)PRT_counts[4].size() - line < -1)
                        PRT_counts[4].push_back(0);
                    PRT_counts[4].push_back(calib_sequence::calc_avg(&words13bit[10], 5));
                }
            }
        }

        image::Image HIRSReader::getChannel(int channel)
        {
            return image::Image(channels[channel].data(), 16, 56, line, 1);
        }

        // #################
        // ## calib stuff ##
        // #################

        void HIRSReader::calibrate(nlohmann::json calib_coef, bool HIRS3)
        {
            calib_out["calibrator"] = "noaa_hirs";
            if (c_sequences.size() > 0)
                c_sequences.pop_back(); // Last sequence is always incomplete

            for (int channel = 0; channel < 19; channel++)
            {
                for (uint8_t i = 0; i < c_sequences.size(); i++) // per channel per calib sequence
                {
                    c_sequences[i].PRT_temp = 0;
                    for (int p = 0; p < 5; p++)
                    {
                        uint16_t w_avg = 0;
                        for (int l = 0; l < 3; l++)
                            w_avg += PRT_counts[p][c_sequences[i].position - 1 + l] * ((l % 2) + 1);
                        w_avg /= 4;
                        for (int deg = 0; deg < 6; deg++)
                        {
                            c_sequences[i].PRT_temp += calib_coef["PRT_poly"][p][deg].get<double>() * pow(-w_avg, deg);
                        }
                    }
                    c_sequences[i].PRT_temp /= HIRS3 ? 4 : 5;

                    // std::cout << (int)i << ", " << channel << ", " << c_sequences[i].position << ", " << c_sequences[i].space[channel] << ", " << c_sequences[i].blackbody[channel] << std::endl;
                    c_sequences[i].PRT_temp = calib_coef["b"][channel].get<double>() + calib_coef["c"][channel].get<double>() * c_sequences[i].PRT_temp;
                }

                nlohmann::json ch;
                double a0 = 0, a1 = 0, rad = 0, ratio = 0;
                uint16_t current_cseq = 0;

                for (int cl = 0; cl < line; cl++)
                { // per line
                    nlohmann::json ln;
                    if (c_sequences.size() == 0)
                    {
                        ln["a0"] = -999.99;
                        ln["a1"] = -999.99;
                        ch.push_back(ln);
                        continue;
                    }

                    if (current_cseq < c_sequences.size() && cl == c_sequences[current_cseq].position)
                        current_cseq++;

                    if (current_cseq == 0)
                    {
                        if (c_sequences[current_cseq].blackbody[channel] == 0 || c_sequences[current_cseq].space[channel] == 0)
                        {
                            ln["a0"] = -999.99;
                            ln["a1"] = -999.99;
                            ch.push_back(ln);
                            continue;
                        }
                        rad = temperature_to_radiance(c_sequences[current_cseq].PRT_temp, calib_coef["wavenumber"][channel].get<double>());
                        a1 = rad / (c_sequences[current_cseq].blackbody[channel] - c_sequences[current_cseq].space[channel]);
                        a0 = -a1 * c_sequences[current_cseq].space[channel];
                    }
                    else if (current_cseq > c_sequences.size() - 1)
                    {
                        if (c_sequences[current_cseq - 1].blackbody[channel] == 0 || c_sequences[current_cseq - 1].space[channel] == 0)
                        {
                            ln["a0"] = -999.99;
                            ln["a1"] = -999.99;
                            ch.push_back(ln);
                            continue;
                        }
                        rad = temperature_to_radiance(c_sequences[current_cseq - 1].PRT_temp, calib_coef["wavenumber"][channel].get<double>());
                        a1 = rad / (c_sequences[current_cseq - 1].blackbody[channel] - c_sequences[current_cseq - 1].space[channel]);
                        a0 = -a1 * c_sequences[current_cseq - 1].space[channel];
                    }
                    else
                    { // interpolate
                        double bb0 = c_sequences[current_cseq].blackbody[channel], bb1 = c_sequences[current_cseq - 1].blackbody[channel], spc0 = c_sequences[current_cseq].space[channel], spc1 = c_sequences[current_cseq - 1].space[channel];

                        if (bb0 == 0 && bb1 != 0)
                            bb0 = bb1;
                        else if (bb1 == 0 && bb0 != 0)
                            bb1 = bb0;

                        if (spc0 == 0 && spc1 != 0)
                            spc0 = spc1;
                        else if (spc1 == 0 && spc0 != 0)
                            spc1 = spc0;

                        if ((bb0 == 0 && bb1 == 0) || (spc0 == 0 && spc1 == 0))
                        {
                            ln["a0"] = -999.99;
                            ln["a1"] = -999.99;
                            ch.push_back(ln);
                            continue;
                        }

                        rad = temperature_to_radiance(c_sequences[current_cseq].PRT_temp, calib_coef["wavenumber"][channel].get<double>());
                        ratio = (c_sequences[current_cseq].position - (cl + 2)) / 38.0;
                        a1 = rad / (bb1 - spc1) * ratio + rad / (bb0 - spc0) * (1 - ratio);
                        a0 = -a1 * spc1 * ratio - a1 * spc0 * (1 - ratio);
                    }

                    ln["a0"] = a0;
                    ln["a1"] = a1;
                    ch.push_back(ln);
                }
                calib_out["vars"]["perLine_perChannel"][channel] = ch;
            }
            calib_out["wavenumbers"] = calib_coef["wavenumber"];
            calib_out["vars"]["perChannel"] = calib_coef["visChannel"];
        }

        uint16_t calib_sequence::calc_avg(uint16_t *samples, int count)
        {
            /*
            This function calcualtes the average value for space and bb looks, using the 1-sigma criterion to discard bad samples
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

            std::pair<int, int> range = {mean - pow(variance, 0.5), mean + pow(variance, 0.5)};
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