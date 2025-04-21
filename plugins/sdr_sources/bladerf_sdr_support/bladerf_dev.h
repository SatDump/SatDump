#pragma once

#include "core/exception.h"
#include "dsp/block.h"
#include "dsp/device/dev.h"

#include <cstdio>
#include <libbladeRF.h>
#include <string>

#include "common/dsp/complex.h"

// TODOREWORK Change the namespace?
namespace satdump
{
    namespace ndsp
    {
        class BladeRFDevBlock : public DeviceBlock
        {
        public:
            std::string dev_serial = "";

            // Only one samplerate for all channels
            double samplerate = 6e6;

            // RX1/RX2 frequencies are bound together
            double rx_frequency = 100e6;
            double tx_frequency = 100e6;

            int rx_ch_number = 1;
            int tx_ch_number = 0;

            // Only applicable when rx_ch_number = 1 (or TX)
            int rx_ch_id = 0;
            int tx_ch_id = 0;

            // Gains are actually split
            int rx_gain[2] = {10, 10};
            int tx_gain[2] = {10, 10};

            // It seems bias-tees are linked together too on the blade
            bool rx_bias = false;
            bool tx_bias = false;

            bool extclock = false;

        private:
            int sample_buffer_size = 8192; // TODOREWORK

            std::thread work_thread_rx;
            bool thread_should_run_rx = false;
            void mainThread_rx()
            {
                int16_t *sample_buffer;
                bladerf_metadata meta;

                {
                    sample_buffer = new int16_t[sample_buffer_size * 2 * rx_ch_number];

                    while (thread_should_run_rx)
                    {
                        if (bladerf_sync_rx(bladerf_dev_obj, sample_buffer, sample_buffer_size, &meta, 4000) != 0)
                            std::this_thread::sleep_for(std::chrono::seconds(1)); // TODOREWORK handle errors better?

                        if (rx_ch_number == 1)
                        {
                            DSPBuffer oblk = DSPBuffer::newBufferSamples<complex_t>(sample_buffer_size);
                            volk_16i_s32f_convert_32f((float *)oblk.getSamples<complex_t>(), sample_buffer, 4096.0f, sample_buffer_size * 2);
                            oblk.size = meta.actual_count;
                            outputs[0].fifo->wait_enqueue(oblk);
                        }
                        else if (rx_ch_number == 2)
                        {
                            int nsam = meta.actual_count / 2;
                            for (int s = 0; s < 2; s++)
                            {
                                DSPBuffer oblk = DSPBuffer::newBufferSamples<complex_t>(sample_buffer_size);
                                auto c = oblk.getSamples<complex_t>();
                                for (int i = 0; i < nsam; i++)
                                {
                                    c[i].real = sample_buffer[(i * 2 + s) * 2 + 0] / 4096.0f;
                                    c[i].imag = sample_buffer[(i * 2 + s) * 2 + 1] / 4096.0f;
                                }
                                oblk.size = nsam;
                                outputs[s].fifo->wait_enqueue(oblk);
                            }
                        }
                        else
                            throw satdump_exception("BladeRX RX can't have more than 2 channels!");
                    }

                    delete[] sample_buffer;
                }
            }

            std::thread work_thread_tx;
            bool thread_should_run_tx = false;
            void mainThread_tx()
            {
                int16_t *sample_buffer;
                bladerf_metadata meta;

                {
                    sample_buffer = new int16_t[sample_buffer_size * 2 * tx_ch_number];

                    while (thread_should_run_rx)
                    {
                        if (tx_ch_number == 1)
                        {
                            DSPBuffer iblk;
                            inputs[0].fifo->wait_dequeue(iblk);

                            if (iblk.isTerminator())
                            {
                                iblk.free();
                                break;
                            }

                            volk_32f_s32f_convert_16i(sample_buffer, (float *)iblk.getSamples<complex_t>(), 4096.0f, iblk.size * 2);
                            if (bladerf_sync_tx(bladerf_dev_obj, sample_buffer, iblk.size, &meta, 4000) != 0)
                                std::this_thread::sleep_for(std::chrono::seconds(1));

                            iblk.free();
                        }
                        else
                            throw satdump_exception("BladeRX TX can't have more than one channel (YET)!");
                    }

                    delete[] sample_buffer;
                }
            }

        private:
            bool is_open = false, is_started = false;
            bladerf *bladerf_dev_obj;

        public:
            BladeRFDevBlock();
            ~BladeRFDevBlock();

            void init();

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;

                p = devInfo.params;

                p["serial"]["type"] = "string";
                p["serial"]["hide"] = devInfo.cfg.contains("serial");
                p["serial"]["disable"] = is_started;

                add_param_simple(p, "rx_ch_number", "uint", "RX Ch Number");
                add_param_simple(p, "tx_ch_number", "uint", "TX Ch Number");
                p["rx_ch_number"]["disable"] = is_started;
                p["tx_ch_number"]["disable"] = is_started;

                if (!p.contains("samplerate"))
                    add_param_simple(p, "samplerate", "uint", "Samplerate");
                p["samplerate"]["disable"] = is_started;

                if (rx_ch_number > 0)
                    add_param_simple(p, "rx_frequency", "float", "RX Frequency");
                if (tx_ch_number > 0)
                    add_param_simple(p, "tx_frequency", "float", "TX Frequency");

                if (rx_ch_number == 1)
                {
                    add_param_simple(p, "rx_channel_id", "uint", "RX Channel");
                    p["rx_channel_id"]["disable"] = is_started;
                }
                if (tx_ch_number == 1)
                {
                    add_param_simple(p, "tx_channel_id", "uint", "TX Channel");
                    p["tx_channel_id"]["disable"] = is_started;
                }

                if (rx_ch_number >= 1 && rx_ch_id == 0)
                    add_param_simple(p, "rx1_gain", "int", "RX1 Gain");
                if (rx_ch_number >= 2 || rx_ch_id == 1)
                    add_param_simple(p, "rx2_gain", "int", "RX2 Gain");
                if (tx_ch_number >= 1 && tx_ch_id == 0)
                    add_param_simple(p, "tx1_gain", "int", "TX1 Gain");
                if (tx_ch_number >= 2 || tx_ch_id == 1)
                    add_param_simple(p, "tx2_gain", "int", "TX2 Gain");

                if (rx_ch_number > 0)
                    add_param_simple(p, "rx_bias", "bool", "RX Bias");
                if (tx_ch_number > 0)
                    add_param_simple(p, "tx_bias", "bool", "TX Bias");

                add_param_simple(p, "extclock", "bool", "External Clock");
                p["extclock"]["disable"] = is_started;

                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "serial")
                    return dev_serial;
                else if (key == "samplerate")
                    return samplerate;
                else if (key == "rx_frequency")
                    return rx_frequency;
                else if (key == "tx_frequency")
                    return tx_frequency;
                else if (key == "rx_ch_number")
                    return rx_ch_number;
                else if (key == "tx_ch_number")
                    return tx_ch_number;
                else if (key == "rx_channel_id")
                    return rx_ch_id;
                else if (key == "tx_channel_id")
                    return tx_ch_id;
                else if (key == "rx1_gain")
                    return rx_gain[0];
                else if (key == "rx2_gain")
                    return rx_gain[1];
                else if (key == "tx1_gain")
                    return tx_gain[0];
                else if (key == "tx2_gain")
                    return tx_gain[1];
                else if (key == "rx_bias")
                    return rx_bias;
                else if (key == "tx_bias")
                    return tx_bias;
                else if (key == "extclock")
                    return extclock;
                else
                    throw satdump_exception(key);
            }

            void set_frequency();
            void set_gains();
            void set_others();

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                cfg_res_t r = RES_OK;

                if (key == "serial")
                    dev_serial = v;
                else if (key == "samplerate")
                    samplerate = v;
                else if (key == "rx_frequency")
                {
                    rx_frequency = v;
                    set_frequency();
                }
                else if (key == "tx_frequency")
                {
                    tx_frequency = v;
                    set_frequency();
                }
                else if (key == "rx_ch_number")
                {
                    int rx_ch_number_old = rx_ch_number;
                    rx_ch_number = v;
                    if (rx_ch_number < 0)
                        rx_ch_number = 0;
                    if (rx_ch_number > 2)
                        rx_ch_number = 2;

                    int diff = rx_ch_number - rx_ch_number_old;

                    if (diff != 0)
                    {
                        outputs.clear();
                        if (rx_ch_number == 1)
                        {
                            outputs.push_back({"RX", DSP_SAMPLE_TYPE_CF32});
                        }
                        else if (rx_ch_number == 2)
                        {
                            outputs.push_back({"RX1", DSP_SAMPLE_TYPE_CF32});
                            outputs.push_back({"RX2", DSP_SAMPLE_TYPE_CF32});
                        }
                    }

                    r = RES_IOUPD;
                }
                else if (key == "tx_ch_number")
                {
                    int tx_ch_number_old = tx_ch_number;
                    tx_ch_number = v;
                    if (tx_ch_number < 0)
                        tx_ch_number = 0;
                    if (tx_ch_number > 2)
                        tx_ch_number = 2;

                    int diff = tx_ch_number - tx_ch_number_old;

                    if (diff != 0)
                    {
                        inputs.clear();
                        if (tx_ch_number == 1)
                        {
                            inputs.push_back({"TX", DSP_SAMPLE_TYPE_CF32});
                        }
                        else if (tx_ch_number == 2)
                        {
                            inputs.push_back({"TX1", DSP_SAMPLE_TYPE_CF32});
                            inputs.push_back({"TX2", DSP_SAMPLE_TYPE_CF32});
                        }
                    }

                    r = RES_IOUPD;
                }
                else if (key == "rx_channel_id")
                {
                    rx_ch_id = v;
                    if (rx_ch_id < 0)
                        rx_ch_id = 0;
                    if (rx_ch_id > 1)
                        rx_ch_id = 1;
                }
                else if (key == "tx_channel_id")
                {
                    tx_ch_id = v;
                    if (tx_ch_id < 0)
                        tx_ch_id = 0;
                    if (tx_ch_id > 1)
                        tx_ch_id = 1;
                }
                else if (key == "rx1_gain")
                {
                    rx_gain[0] = v;
                    set_gains();
                }
                else if (key == "rx2_gain")
                {
                    rx_gain[1] = v;
                    set_gains();
                }
                else if (key == "tx1_gain")
                {
                    tx_gain[0] = v;
                    set_gains();
                }
                else if (key == "tx2_gain")
                {
                    tx_gain[1] = v;
                    set_gains();
                }
                else if (key == "rx_bias")
                {
                    rx_bias = v;
                    set_others();
                }
                else if (key == "tx_bias")
                {
                    tx_bias = v;
                    set_others();
                }
                else if (key == "extclock")
                {
                    set_others();
                    extclock = v;
                }
                else
                    throw satdump_exception(key);
                return r;
            }

            double getStreamSamplerate(int id, bool output)
            {
                if (id > 0 || output)
                    throw satdump_exception("Stream ID must be 0 and input only!");
                return samplerate;
            }

            void start();
            void stop(bool stop_now = false);

        public:
            static std::vector<DeviceInfo> listDevs();
        };
    } // namespace ndsp
} // namespace satdump