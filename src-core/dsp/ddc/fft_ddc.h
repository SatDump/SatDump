#pragma once

#include "common/dsp/complex.h"
#include "dsp/block.h"
#include "dsp/block_helpers.h"
#include "dsp/ddc/calc_filt.h"
#include "dsp/ddc/fft_helper.h"
#include <fftw3.h>
#include <volk/volk_alloc.hh>

namespace satdump
{
    namespace ndsp
    {
        class FFTDDCBlock : public Block
        {
        public:
            bool needs_reinit = false;

            int p_noutputs = 0;
            double frequency = 100e6;
            double samplerate = 6e6;
            std::mutex vfos_mtx;

        private:
            int buffer_size = 0;
            volk::vector<complex_t> buffer;

            size_t in_buffer;

            int d_filter_m = 7200;
            int d_ntaps = 0;
            int d_fftsize = 0;
            int d_nsamples = 0;

            std::shared_ptr<FFTHelper> fft_fwd;

        public:
            struct IOInfo
            {
            public:
                std::string id;
                bool forward_terminator;

            public:
                double frequency = 0;
                double bandwidth = 0;
                int decimation = 1;

                int sample_index = 0;

            public:
                void recalc_freq(double sr, double cf)
                {
                    double freq_shift = cf - frequency;
                    phase = complex_t(1, 0);
                    phase_delta = complex_t(cos(2.0 * M_PI * (freq_shift / sr)), sin(2.0 * M_PI * (freq_shift / sr)));
                }

                complex_t phase = 0, phase_delta = 0;

            public:
                std::shared_ptr<FFTHelper> fft_inv;

                volk::vector<complex_t> tail;
                volk::vector<complex_t> ffttaps_buffer;

                void initFFTFilter(FFTDDCBlock *tthis)
                {
                    if (!tthis->fft_fwd)
                        return;

                    // Init FFT
                    if (!fft_inv || fft_inv->size != tthis->d_fftsize)
                        fft_inv = std::make_shared<FFTHelper>(tthis->d_fftsize, false);

                    // Generate taps
                    double freq_off = frequency - tthis->frequency;
                    double fwT0 = 2 * M_PI * (freq_off / tthis->samplerate);
                    auto taps = filt::generateTaps(tthis->samplerate, 0, bandwidth / 2., tthis->d_filter_m, 50, fwT0);

                    // Init taps
                    float scale = 1.0 / tthis->d_fftsize;
                    for (int i = 0; i < tthis->d_ntaps; i++)
                        ((complex_t *)tthis->fft_fwd->fft_in)[i] = taps[i] * scale;
                    for (int i = tthis->d_ntaps; i < tthis->d_fftsize; i++)
                        ((complex_t *)tthis->fft_fwd->fft_in)[i] = complex_t(0.0f, 0.0f);

                    tthis->fft_fwd->execute();

                    ffttaps_buffer.resize(tthis->d_fftsize);
                    for (int i = 0; i < tthis->d_fftsize; i++)
                        ffttaps_buffer[i] = ((complex_t *)tthis->fft_fwd->fft_out)[i];

                    tail.resize(tthis->d_ntaps - 1);
                }

            public:
                IOInfo(std::string id, bool forward_terminator, double f = 0, double b = 0, double d = 0) : id(id), forward_terminator(forward_terminator), frequency(f), bandwidth(b), decimation(d) {}
            };

        private:
            bool work();

        public:
            FFTDDCBlock();
            ~FFTDDCBlock();

            void init();

        public:
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
                    {
                        ((IOInfo *)outputs[i].blkdata.get())->recalc_freq(samplerate, frequency);
                        ((IOInfo *)outputs[i].blkdata.get())->initFFTFilter(this);
                    }
                }
                else if (key == "samplerate")
                {
                    samplerate = v;
                    for (int i = 0; i < outputs.size(); i++)
                    {
                        ((IOInfo *)outputs[i].blkdata.get())->recalc_freq(samplerate, frequency);
                        ((IOInfo *)outputs[i].blkdata.get())->initFFTFilter(this);
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
                            ((IOInfo *)o.blkdata.get())->recalc_freq(samplerate, frequency);
                            ((IOInfo *)o.blkdata.get())->initFFTFilter(this);
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
                    i.initFFTFilter(this);
                }
                else if (sscanf(key.c_str(), "output_%d_bw%n", &o_n, &o_s) == 1 && o_s == key.size())
                {
                    auto &i = *((IOInfo *)outputs[o_n].blkdata.get());
                    i.bandwidth = v;
                    i.initFFTFilter(this);
                }
                else if (sscanf(key.c_str(), "output_%d_dec%n", &o_n, &o_s) == 1 && o_s == key.size())
                {
                    int dec = v;
                    if (dec < 1)
                        dec = 1;
                    ((IOInfo *)outputs[o_n].blkdata.get())->decimation = dec;
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
                ((IOInfo *)o.blkdata.get())->recalc_freq(samplerate, frequency);
                ((IOInfo *)o.blkdata.get())->initFFTFilter(this);
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
                        i.initFFTFilter(this);
                    }
                }
            }

            double get_vfo_freq(std::string id)
            {
                std::lock_guard<std::mutex> lock_mtx(vfos_mtx);

                for (auto &o : outputs)
                {
                    auto &i = *((IOInfo *)o.blkdata.get());
                    if (i.id == id)
                        return i.frequency;
                }

                return -1;
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
                        i.initFFTFilter(this);
                    }
                }
            }

            double get_vfo_bandwidth(std::string id)
            {
                std::lock_guard<std::mutex> lock_mtx(vfos_mtx);

                for (auto &o : outputs)
                {
                    auto &i = *((IOInfo *)o.blkdata.get());
                    if (i.id == id)
                        return i.bandwidth;
                }

                return -1;
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

            int get_vfo_dec(std::string id)
            {
                std::lock_guard<std::mutex> lock_mtx(vfos_mtx);

                for (auto &o : outputs)
                {
                    auto &i = *((IOInfo *)o.blkdata.get());
                    if (i.id == id)
                        return i.decimation;
                }

                return -1;
            }
        };
    } // namespace ndsp
} // namespace satdump