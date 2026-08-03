#pragma once

#include "common/dsp/filter/firdes.h"
#include "dsp/block.h"
#include "dsp/filter/fft.h"
#include "dsp/filter/fir.h"
#include <complex.h>
#include <string>

namespace satdump
{
    namespace ndsp
    {
        class NotchBankBlock : public FFTFilterBlock<complex_t, complex_t>
        {
        private:
            double frequency = 100e6;
            double samplerate = 6e6;

            struct NotchCfg
            {
                double frequency = 101e6;
                double bandwidth = 5000;
                double transition_width = 2000;
            };

            int notch_cnt = 1;
            std::vector<NotchCfg> notches = {NotchCfg()};

        public:
            NotchBankBlock() : FFTFilterBlock<complex_t, complex_t>("notch_bank") {}

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                add_param_simple(p, "frequency", "float", "Frequency");
                add_param_simple(p, "samplerate", "float", "Samplerate");
                add_param_simple(p, "notch_cnt", "int", "Notch Count");

                for (int i = 0; i < notches.size(); i++)
                {
                    add_param_simple(p, "notch_" + std::to_string(i + 1) + "_freq", "float", "Notch " + std::to_string(i + 1) + " Freq");
                    add_param_simple(p, "notch_" + std::to_string(i + 1) + "_bw", "float", "Notch " + std::to_string(i + 1) + " Bw");
                    add_param_simple(p, "notch_" + std::to_string(i + 1) + "_tr", "float", "Notch " + std::to_string(i + 1) + " Tra");
                }

                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "frequency")
                    return frequency;
                else if (key == "samplerate")
                    return samplerate;
                else if (key == "notch_cnt")
                    return notch_cnt;
                else
                {
                    for (int i = 0; i < notches.size(); i++)
                    {
                        if (key == ("notch_" + std::to_string(i + 1) + "_freq"))
                            return notches[i].frequency;
                        else if (key == ("notch_" + std::to_string(i + 1) + "_bw"))
                            return notches[i].bandwidth;
                        else if (key == ("notch_" + std::to_string(i + 1) + "_tr"))
                            return notches[i].transition_width;
                    }

                    return FFTFilterBlock::get_cfg(key);
                }
            }

            Block::cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                cfg_res_t res = RES_OK;

                if (key == "frequency")
                    frequency = v;
                else if (key == "samplerate")
                    samplerate = v;
                else if (key == "notch_cnt")
                {
                    notch_cnt = v;
                    if (notch_cnt < 1)
                        notch_cnt = 1;
                    notches.resize(notch_cnt);
                    res = RES_LISTUPD;
                }
                else
                {
                    // Check size
                    for (int i = 0; i < 500; i++)
                    {
                        int max = 0;
                        if (key == ("notch_" + std::to_string(i + 1) + "_freq") || key == ("notch_" + std::to_string(i + 1) + "_bw") || key == ("notch_" + std::to_string(i + 1) + "_tr"))
                            max = i + 1;

                        if (max > notch_cnt)
                        {
                            notch_cnt = max;
                            notches.resize(notch_cnt);
                            res = RES_LISTUPD;
                        }
                    }

                    for (int i = 0; i < notches.size(); i++)
                    {
                        if (key == ("notch_" + std::to_string(i + 1) + "_freq"))
                            notches[i].frequency = v;
                        else if (key == ("notch_" + std::to_string(i + 1) + "_bw"))
                            notches[i].bandwidth = v;
                        else if (key == ("notch_" + std::to_string(i + 1) + "_tr"))
                            notches[i].transition_width = v;
                    }
                }

                if (key == "buffer_size")
                    return FFTFilterBlock::set_cfg(key, v);
                else
                {
                    std::vector<std::vector<complex_t>> all_taps;

                    size_t largest_filter = 0;

                    for (int i = 0; i < notches.size(); i++)
                    {
                        auto taps = dsp::firdes::high_pass(1, samplerate, notches[i].bandwidth, notches[i].transition_width);

                        double freq_off = notches[i].frequency - frequency;
                        double fwT0 = 2 * M_PI * (freq_off / samplerate);

                        int c = (taps.size() - 1) / 2;

                        std::vector<complex_t> ctaps;

                        for (int j = 0; j < taps.size(); j++)
                        {
                            complex_t v = float(taps[j]) * exp(std::complex<float>(0, (j - c) * fwT0));
                            ctaps.push_back(v);
                        }

                        if (largest_filter < ctaps.size())
                            largest_filter = ctaps.size();

                        all_taps.push_back(ctaps);
                    }

                    for (auto &t : all_taps)
                    {
                        size_t pre = (largest_filter - t.size()) / 2;
                        t.insert(t.begin(), pre, complex_t(0));
                        t.resize(largest_filter, complex_t(0));
                    }

                    std::vector<complex_t> taps(largest_filter, 0);

                    for (auto &t : all_taps)
                        for (int i = 0; i < largest_filter; i++)
                            taps[i] += t[i];

                    taps[(largest_filter - 1) / 2] -= complex_t(notches.size() - 1, 0);

                    res = std::max(FFTFilterBlock::set_cfg("taps", taps), res);
                }

                return res;
            }
        };
    } // namespace ndsp
} // namespace satdump