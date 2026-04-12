#pragma once

#include "common/dsp/filter/firdes.h"
#include "core/exception.h"
#include "dsp/block.h"
#include "dsp/block_helpers.h"
#include "dsp/filter/fir.h"
#include <memory>
#include <mutex>
#include <string>

namespace satdump
{
    namespace ndsp
    {
        class DDC_Block : public Block
        {
        public:
            int p_noutputs = 0;
            double frequency = 100e6;
            double samplerate = 6e6;
            std::mutex vfos_mtx;

        public:
            struct IOInfo
            {
                std::string id;
                bool forward_terminator;

                double frequency = 0;
                double bandwidth = 0;
                int decimation = 1;

                void recalc_freq(double sr, double cf)
                {
                    double freq_shift = cf - frequency;
                    phase = complex_t(1, 0);
                    phase_delta = complex_t(cos(2.0 * M_PI * (freq_shift / sr)), sin(2.0 * M_PI * (freq_shift / sr)));
                }

                int sample_index = 0;
                complex_t phase_delta = 0;
                complex_t phase = 0;

                void recalc_bw(double sr)
                {
                    if (bandwidth == 0)
                        return;
                    filter.set_cfg("taps", dsp::firdes::low_pass(1, sr, bandwidth / 2, 1e3));
                }

                FIRBlock<complex_t> filter;

                IOInfo(std::string id, bool forward_terminator, double f = 0, double b = 0, double d = 0) : id(id), forward_terminator(forward_terminator), frequency(f), bandwidth(b), decimation(d) {}
            };

        private:
            bool work();

        public:
            DDC_Block();
            ~DDC_Block();

            void init() {}

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;

                add_param_simple(p, "frequency", "freq");
                add_param_simple(p, "samplerate", "float");

                add_param_simple(p, "noutputs", "int");
                p["noutputs"]["disable"] = is_work_running();

                for (int i = 0; i < outputs.size(); i++)
                {
                    auto &io = *((IOInfo *)outputs[i].blkdata.get());
                    add_param_simple(p, "output_" + std::to_string(i) + "_freq", "freq");
                    add_param_simple(p, "output_" + std::to_string(i) + "_bw", "float");
                    add_param_simple(p, "output_" + std::to_string(i) + "_dec", "int");
                }

                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                int o_n = 0, o_s = 0;
                if (key == "frequency")
                    return frequency;
                else if (key == "samplerate")
                    return samplerate;
                else if (key == "noutputs")
                    return p_noutputs;
                else if (sscanf(key.c_str(), "output_%d_freq%n", &o_n, &o_s) == 1 && o_s == key.size())
                    return ((IOInfo *)outputs[o_n].blkdata.get())->frequency;
                else if (sscanf(key.c_str(), "output_%d_bw%n", &o_n, &o_s) == 1 && o_s == key.size())
                    return ((IOInfo *)outputs[o_n].blkdata.get())->bandwidth;
                else if (sscanf(key.c_str(), "output_%d_dec%n", &o_n, &o_s) == 1 && o_s == key.size())
                    return ((IOInfo *)outputs[o_n].blkdata.get())->decimation;
                throw satdump_exception(key);
            }

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                std::lock_guard<std::mutex> lock_mtx(vfos_mtx);

                int o_n = 0, o_s = 0;
                if (key == "frequency")
                {
                    frequency = v;
                    for (int i = 0; i < outputs.size(); i++)
                        ((IOInfo *)outputs[i].blkdata.get())->recalc_freq(samplerate, frequency);
                }
                else if (key == "samplerate")
                {
                    samplerate = v;
                    for (int i = 0; i < outputs.size(); i++)
                    {
                        ((IOInfo *)outputs[i].blkdata.get())->recalc_freq(samplerate, frequency);
                        ((IOInfo *)outputs[i].blkdata.get())->recalc_bw(samplerate);
                    }
                }
                else if (key == "noutputs")
                {
                    int no = p_noutputs;
                    p_noutputs = v;

                    if (p_noutputs < 0)
                        p_noutputs = 0;

                    if (no != p_noutputs)
                    {
                        Block::outputs.clear();
                        for (int i = 0; i < p_noutputs; i++)
                        {
                            BlockIO o = {"out" + std::to_string(i + 1), getTypeSampleType<complex_t>()};
                            o.blkdata = std::make_shared<IOInfo>(std::to_string(i + 1), true);
                            o.fifo = std::make_shared<DSPStream>(4); // TODOREWORK
                            Block::outputs.push_back(o);
                        }
                    }
                    return RES_IOUPD;
                }
                else if (sscanf(key.c_str(), "output_%d_freq%n", &o_n, &o_s) == 1 && o_s == key.size())
                {
                    auto &i = *((IOInfo *)outputs[o_n].blkdata.get());
                    i.frequency = v;
                    i.recalc_freq(samplerate, frequency);
                }
                else if (sscanf(key.c_str(), "output_%d_bw%n", &o_n, &o_s) == 1 && o_s == key.size())
                {
                    auto &i = *((IOInfo *)outputs[o_n].blkdata.get());
                    i.bandwidth = v;
                    i.recalc_bw(samplerate);
                }
                else if (sscanf(key.c_str(), "output_%d_dec%n", &o_n, &o_s) == 1 && o_s == key.size())
                {
                    ((IOInfo *)outputs[o_n].blkdata.get())->decimation = v;
                }
                else
                    throw satdump_exception(key);
                return RES_OK;
            }

        public:
            BlockIO &add_output(std::string id, double freq, double bw, int dec, bool forward_terminator = true)
            {
                std::lock_guard<std::mutex> lock_mtx(vfos_mtx);

                BlockIO o = {id, getTypeSampleType<complex_t>()};
                o.blkdata = std::make_shared<IOInfo>(id, forward_terminator, freq, bw, dec);
                o.fifo = std::make_shared<DSPStream>(4); // TODOREWORK
                Block::outputs.push_back(o);
                return outputs[outputs.size() - 1];
            }

            void del_output(std::string id, bool send_terminator) // TODOREWORk maybe a terminator_type_none, _propag, etc? Also maybe a handleTerminator doing it?
            {
                std::lock_guard<std::mutex> lock_mtx(vfos_mtx);

                for (int i = 0; i < outputs.size(); i++)
                    if (((IOInfo *)outputs[i].blkdata.get())->id == id)
                    {
                        if (send_terminator)
                            (outputs.begin() + i)->fifo->wait_enqueue((outputs.begin() + i)->fifo->newBufferTerminator());
                        outputs.erase(outputs.begin() + i);
                    }
            }

            BlockIO &get_output_by_id(std::string id)
            {
                std::lock_guard<std::mutex> lock_mtx(vfos_mtx);

                for (auto &o : outputs)
                    if (((IOInfo *)o.blkdata.get())->id == id)
                        return o;

                throw satdump_exception("Couldn't find splitter output with id " + id);
            }

            void set_vfo_freq(std::string id, double freq)
            {
                std::lock_guard<std::mutex> lock_mtx(vfos_mtx);

                for (auto &o : outputs)
                {
                    auto &i = *((IOInfo *)o.blkdata.get());
                    if (i.id == id)
                    {
                        i.frequency = freq;
                        i.recalc_freq(samplerate, frequency);
                    }
                }
            }

            void set_vfo_bandwidth(std::string id, double bw)
            {
                std::lock_guard<std::mutex> lock_mtx(vfos_mtx);

                for (auto &o : outputs)
                {
                    auto &i = *((IOInfo *)o.blkdata.get());
                    if (i.id == id)
                    {
                        i.bandwidth = bw;
                        i.recalc_bw(samplerate);
                    }
                }
            }

            void set_vfo_dec(std::string id, int dec)
            {
                std::lock_guard<std::mutex> lock_mtx(vfos_mtx);

                for (auto &o : outputs)
                {
                    auto &i = *((IOInfo *)o.blkdata.get());
                    if (i.id == id)
                    {
                        i.decimation = dec;
                    }
                }
            }
        };
    } // namespace ndsp
} // namespace satdump