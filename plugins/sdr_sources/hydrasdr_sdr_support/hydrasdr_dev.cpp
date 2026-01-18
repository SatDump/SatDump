#include "hydrasdr_dev.h"
#include "common/dsp/complex.h"
#include <logger.h>

namespace satdump
{
    namespace ndsp
    {
        HydraSDRDevBlock::HydraSDRDevBlock() : DeviceBlock("hydrasdr_dev_cc", {}, {{"out", DSP_SAMPLE_TYPE_CF32}})
        {
            outputs[0].fifo = std::make_shared<DSPStream>(16); // TODOREWORK
        }

        HydraSDRDevBlock::~HydraSDRDevBlock()
        {
            stop(); // TODOREWORK (make sure to close device)
        }

        void HydraSDRDevBlock::set_frequency()
        {
            if (is_open)
            {
                hydrasdr_set_freq(hydrasdr_dev_obj, p_frequency);
                logger->debug("Set HydraSDR frequency to %f", p_frequency);
            }
        }

        void HydraSDRDevBlock::set_gains()
        {
            if (is_open)
            {
                if (p_gain_type == "sensitive")
                {
                    hydrasdr_set_sensitivity_gain(hydrasdr_dev_obj, p_gain);
                    logger->debug("Set HydraSDR gain (sensitive) to %d", p_gain);
                }
                else if (p_gain_type == "linear")
                {
                    hydrasdr_set_linearity_gain(hydrasdr_dev_obj, p_gain);
                    logger->debug("Set HydraSDR gain (linear) to %d", p_gain);
                }
                else if (p_gain_type == "manual")
                {
                    hydrasdr_set_lna_gain(hydrasdr_dev_obj, p_lna_gain);
                    hydrasdr_set_mixer_gain(hydrasdr_dev_obj, p_mixer_gain);
                    hydrasdr_set_vga_gain(hydrasdr_dev_obj, p_vga_gain);
                    logger->debug("Set HydraSDR gain (manual) to %d, %d, %d", p_lna_gain, p_mixer_gain, p_vga_gain);
                }
            }
        }

        void HydraSDRDevBlock::set_others()
        {
            if (is_open)
            {
                hydrasdr_set_rf_bias(hydrasdr_dev_obj, p_bias);
                logger->debug("Set HydraSDR bias to %d", (int)p_bias);

                hydrasdr_set_lna_agc(hydrasdr_dev_obj, p_lna_agc);
                hydrasdr_set_mixer_agc(hydrasdr_dev_obj, p_mixer_agc);
                logger->debug("Set HydraSDR LNA AGC to %d", p_lna_agc);
                logger->debug("Set HydraSDR Mixer AGC to %d", p_mixer_agc);
            }
        }

        void HydraSDRDevBlock::init()
        {
            set_frequency();
            set_gains();
            set_others();
        }

        void HydraSDRDevBlock::start()
        {
            if (hydrasdr_open_sn(&hydrasdr_dev_obj, std::stoull(p_serial)) != HYDRASDR_SUCCESS)
                throw satdump_exception("Could not open HydraSDR device! Serial : " + p_serial);
            logger->info("Opened HydraSDR device! Serial : " + p_serial);
            is_open = true;

            hydrasdr_set_samplerate(hydrasdr_dev_obj, p_samplerate);
            logger->debug("Set HydraSDR samplerate to %d", p_samplerate);

            hydrasdr_set_packing(hydrasdr_dev_obj, 1);
            hydrasdr_set_sample_type(hydrasdr_dev_obj, HYDRASDR_SAMPLE_FLOAT32_IQ);

            init();

            hydrasdr_start_rx(hydrasdr_dev_obj, &_rx_callback, this);
            is_started = true;
        }

        void HydraSDRDevBlock::stop(bool stop_now)
        {
            if (stop_now && is_started) // TODOREWORK Split wait & stop?
            {
                hydrasdr_set_rf_bias(hydrasdr_dev_obj, false);
                hydrasdr_stop_rx(hydrasdr_dev_obj);
                hydrasdr_close(hydrasdr_dev_obj);
                is_started = false;
                is_open = false;
                outputs[0].fifo->wait_enqueue(outputs[0].fifo->newBufferTerminator());
            }
        }

        int HydraSDRDevBlock::_rx_callback(hydrasdr_transfer *t)
        {
            HydraSDRDevBlock *tthis = (HydraSDRDevBlock *)t->ctx;
            int &size = t->sample_count;

            if (t->sample_type == HYDRASDR_SAMPLE_FLOAT32_IQ)
            {
                DSPBuffer oblk = tthis->outputs[0].fifo->newBufferSamples(size, sizeof(complex_t));
                memcpy(oblk.getSamples<complex_t>(), t->samples, size * sizeof(complex_t));
                oblk.size = size;
                tthis->outputs[0].fifo->wait_enqueue(oblk);
            }
            else
            {
                logger->error("TODO IMPLEMENT SAMPLE TYPE %d\n", t->sample_type);
            }

            return 0;
        }

        std::vector<DeviceInfo> HydraSDRDevBlock::listDevs()
        {
            std::vector<DeviceInfo> r;

            uint64_t serials[100];
            int c = hydrasdr_list_devices(serials, 100);

            for (int i = 0; i < c; i++)
            {
                std::stringstream ss;
                ss << std::hex << serials[i];
                std::string name = "HydraSDR " + ss.str();

                nlohmann::json c;
                nlohmann::json p;

                hydrasdr_device *hydrasdr_dev_obj;
                if (hydrasdr_open_sn(&hydrasdr_dev_obj, serials[i]) == HYDRASDR_SUCCESS)
                {
                    // Get available samplerates
                    uint32_t samprate_cnt;
                    uint32_t dev_samplerates[10];
                    hydrasdr_get_samplerates(hydrasdr_dev_obj, &samprate_cnt, 0);
                    hydrasdr_get_samplerates(hydrasdr_dev_obj, dev_samplerates, samprate_cnt);
                    c["samplerate"]["type"] = "samplerate";

                    bool has_10msps = false;
                    for (int i = samprate_cnt - 1, i2 = 0; i >= 0; i--, i2++)
                    {
                        c["samplerate"]["list"][i2] = dev_samplerates[i];
                        if (dev_samplerates[i] == 10e6)
                            has_10msps = true;
                    }

                    if (!has_10msps)
                        c["samplerate"]["list"].push_back(10e6);

                    p["samplerate"] = dev_samplerates[0];

                    hydrasdr_close(hydrasdr_dev_obj);
                }
                else
                {
                    logger->warn("Could not probe HydraSDR device. Not providing specific parameters!");
                }

                p["serial"] = std::to_string(serials[i]);
                r.push_back({"hydrasdr", name, p, c});
            }

            return r;
        }
    } // namespace ndsp
} // namespace satdump