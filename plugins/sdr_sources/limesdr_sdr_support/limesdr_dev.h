#pragma once

#include "core/exception.h"
#include "dsp/block.h"
#include "dsp/device/dev.h"

#include <cstdint>
#include <cstdio>
#include <lime/lms7_device.h>
#include <string>
#include <vector>

#include "common/dsp/complex.h"
#include "logger.h"
#include "nlohmann/json.hpp"

// TODOREWORK Change the namespace?
namespace satdump
{
    namespace ndsp
    {
        class LimeSDRDevBlock : public DeviceBlock
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

            // Per-channel configs
            struct ChannelCfg
            {
                bool gain_mode_manual = false;
                int gain_lna = 0, gain_tia = 0, gain_pga = 0;
                int gain = 0;
                std::string path = "Auto";
            };
            std::vector<ChannelCfg> rx_channel_cfgs = {ChannelCfg()};
            std::vector<ChannelCfg> tx_channel_cfgs;

            // Bandwidth config
            bool manual_bw = false;
            double bandwidth = 6e6;

        private:
            std::thread work_thread_rx;
            bool thread_should_run_rx = false;
            void mainThread_rx()
            {
                int sample_buffer_size = std::min<int>(samplerate / 250, 1e6);

                while (thread_should_run_rx)
                {
                    int ch = 0;
                    for (auto &s : rx_streams)
                    {
                        lime::StreamChannel::Metadata md;

                        DSPBuffer oblk = outputs[ch].fifo->newBufferSamples(sample_buffer_size, sizeof(complex_t));

                        int v = s->Read(oblk.getSamples<complex_t>(), sample_buffer_size, &md);

                        oblk.size = v;
                        if (v > 0)
                            outputs[ch].fifo->wait_enqueue(oblk);
                        else
                            outputs[ch].fifo->free(oblk);
                        ch++;
                    }
                }
            }

            std::thread work_thread_tx;
            void mainThread_tx()
            {
                bool terminate = false;

                while (1)
                {
                    if (terminate)
                        break;

                    int ch = 0;
                    for (auto &s : tx_streams)
                    {
                        DSPBuffer iblk = inputs[ch].fifo->wait_dequeue();

                        if (iblk.isTerminator())
                        {
                            inputs[ch].fifo->free(iblk);
                            terminate = true;
                            break;
                        }

                        lime::StreamChannel::Metadata md;

                        int status = s->Write(iblk.getSamples<complex_t>(), iblk.size, &md);
                        if (status == 0)
                            logger->error("LimeSDR TX Timeout!");
                        else if (status <= 0)
                            logger->error("LimeSDR TX Error! %d", status);

                        inputs[ch].fifo->free(iblk);
                        ch++;
                    }
                }
            }

        private:
            bool is_open = false, is_started = false;
            std::shared_ptr<lime::LMS7_Device> limesdr_dev_obj;
            std::vector<lime::StreamChannel *> rx_streams;
            std::vector<lime::StreamChannel *> tx_streams;

        public:
            LimeSDRDevBlock();
            ~LimeSDRDevBlock();

            void init();

            void setDevInfo(DeviceInfo i, device_mode_t mode)
            {
                DeviceBlock::setDevInfo(i, mode);
                if (devMode == MODE_SINGLE_RX)
                {
                    set_cfg("rx_ch_number", 1);
                    set_cfg("tx_ch_number", 0);
                }
                else if (devMode == MODE_SINGLE_TX)
                {
                    set_cfg("rx_ch_number", 0);
                    set_cfg("tx_ch_number", 1);
                }
                else if (devMode == MODE_RX_TX)
                {
                    set_cfg("rx_ch_number", 1);
                    set_cfg("tx_ch_number", 1);
                }
            }

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;

                auto &op = devInfo.params;

                if (devMode != MODE_SINGLE_TX && rx_ch_number > 0)
                    add_param_simple(p, "rx_frequency", "freq", "RX Frequency");
                if (devMode != MODE_SINGLE_RX && tx_ch_number > 0)
                    add_param_simple(p, "tx_frequency", "freq", "TX Frequency");

                if (!devInfo.cfg.contains("serial"))
                {
                    p["serial"]["name"] = "Serial";
                    p["serial"]["type"] = "string";
                    p["serial"]["hide"] = devInfo.cfg.contains("serial");
                    p["serial"]["disable"] = is_started;
                }

                if (devMode == MODE_NORMAL)
                {
                    add_param_simple(p, "rx_ch_number", "uint", "RX Ch Number");
                    add_param_simple(p, "tx_ch_number", "uint", "TX Ch Number");
                    p["rx_ch_number"]["disable"] = is_started;
                    p["tx_ch_number"]["disable"] = is_started;
                }

                if (op.contains("samplerate"))
                    p["samplerate"] = op["samplerate"];
                else
                    add_param_simple(p, "samplerate", "uint", "Samplerate");
                p["samplerate"]["disable"] = is_started;
                p["samplerate"]["range_noslider"] = true;

                add_param_simple(p, "manual_bw", "bool", "Manual Bandwidth");
                if (manual_bw)
                {
                    if (op.contains("bandwidth"))
                        p["bandwidth"] = op["bandwidth"];
                    else
                        add_param_simple(p, "bandwidth", "uint", "Bandwidth");
                }

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

                for (int chn = 0; chn < rx_channel_cfgs.size(); chn++)
                {
                    std::string name = "rx" + std::to_string(chn + 1) + "_";

                    if (op.contains(name + "path"))
                        p[name + "path"] = op[name + "path"];
                    else
                        add_param_list(p, name + "path", "string", {"NONE", "LNAH", "LNAL", "LNAL_NC", "LNAW", "Auto"}, "RX" + std::to_string(chn + 1) + " Path");

                    if (op.contains(name + "manual_gain"))
                        p[name + "manual_gain"] = op[name + "manual_gain"];
                    else
                        add_param_simple(p, name + "manual_gain", "bool", "RX" + std::to_string(chn + 1) + " Manual Gain");

                    if (rx_channel_cfgs[chn].gain_mode_manual)
                    {
                        if (op.contains(name + "lna_gain"))
                            p[name + "lna_gain"] = op[name + "lna_gain"];
                        else
                            add_param_range(p, name + "lna_gain", "int", 0, 30, 1, "RX" + std::to_string(chn + 1) + " LNA Gain");

                        if (op.contains(name + "tia_gain"))
                            p[name + "tia_gain"] = op[name + "tia_gain"];
                        else
                            add_param_range(p, name + "tia_gain", "int", 0, 12, 1, "RX" + std::to_string(chn + 1) + " TIA Gain");

                        if (op.contains(name + "pga_gain"))
                            p[name + "pga_gain"] = op[name + "pga_gain"];
                        else
                            add_param_range(p, name + "pga_gain", "int", -12, 19, 1, "RX" + std::to_string(chn + 1) + " PGA Gain");
                    }
                    else
                    {
                        if (op.contains(name + "gain"))
                            p[name + "gain"] = op[name + "gain"];
                        else
                            add_param_range(p, name + "gain", "int", 0, 73, 1, "RX" + std::to_string(chn + 1) + " Gain");
                    }
                }

                for (int chn = 0; chn < tx_channel_cfgs.size(); chn++)
                {
                    std::string name = "tx" + std::to_string(chn + 1) + "_";

                    if (op.contains(name + "path"))
                        p[name + "path"] = op[name + "path"];
                    else
                        add_param_list(p, name + "path", "string", {"NONE", "BAND1", "BAND2", "Auto"}, "TX" + std::to_string(chn + 1) + " Path");

                    if (op.contains(name + "gain"))
                        p[name + "gain"] = op[name + "gain"];
                    else
                        add_param_range(p, name + "gain", "int", 0, 73, 1, "TX" + std::to_string(chn + 1) + " Gain");
                }

                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "serial")
                    return dev_serial;
                else if (key == "samplerate")
                    return samplerate;
                else if (key == "manual_bw")
                    return manual_bw;
                else if (key == "bandwidth")
                    return bandwidth;
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
                else
                {
                    // Handle RX Gain settings
                    for (int chn = 0; chn < rx_channel_cfgs.size(); chn++)
                    {
                        std::string name = "rx" + std::to_string(chn + 1) + "_";

                        if (key == (name + "path"))
                            return rx_channel_cfgs[chn].path;
                        else if (key == (name + "manual_gain"))
                            return rx_channel_cfgs[chn].gain_mode_manual;
                        else if (key == (name + "lna_gain"))
                            return rx_channel_cfgs[chn].gain_lna;
                        else if (key == (name + "tia_gain"))
                            return rx_channel_cfgs[chn].gain_tia;
                        else if (key == (name + "pga_gain"))
                            return rx_channel_cfgs[chn].gain_pga;
                        else if (key == (name + "gain"))
                            return rx_channel_cfgs[chn].gain;
                    }

                    // Handle TX Gain settings
                    for (int chn = 0; chn < tx_channel_cfgs.size(); chn++)
                    {
                        std::string name = "tx" + std::to_string(chn + 1) + "_";

                        if (key == (name + "path"))
                            return tx_channel_cfgs[chn].path;
                        else if (key == (name + "gain"))
                            return tx_channel_cfgs[chn].gain;
                    }

                    throw satdump_exception(key);
                }
            }

            void set_frequency();
            void set_gains();
            void set_paths();
            void set_bw();

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                cfg_res_t r = RES_OK;

                if (key == "serial")
                    dev_serial = v;
                else if (key == "samplerate")
                    samplerate = v;
                else if (key == "manual_bw")
                {
                    manual_bw = v;
                    r = RES_LISTUPD;
                    set_bw();
                }
                else if (key == "bandwidth")
                {
                    bandwidth = v;
                    set_bw();
                }
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
                        rx_channel_cfgs.clear();
                        if (rx_ch_number == 1)
                        {
                            outputs.push_back({"RX", DSP_SAMPLE_TYPE_CF32});
                            rx_channel_cfgs.push_back(ChannelCfg());
                        }
                        else
                        {
                            for (int i = 0; i < rx_ch_number; i++)
                            {
                                outputs.push_back({"RX" + std::to_string(i), DSP_SAMPLE_TYPE_CF32});
                                rx_channel_cfgs.push_back(ChannelCfg());
                            }
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
                        tx_channel_cfgs.clear();
                        if (tx_ch_number == 1)
                        {
                            inputs.push_back({"TX", DSP_SAMPLE_TYPE_CF32});
                            tx_channel_cfgs.push_back(ChannelCfg());
                        }
                        else
                        {
                            for (int i = 0; i < tx_ch_number; i++)
                            {
                                inputs.push_back({"TX" + std::to_string(i), DSP_SAMPLE_TYPE_CF32});
                                tx_channel_cfgs.push_back(ChannelCfg());
                            }
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
                else
                {
                    bool invalid = true;

                    // Handle RX Gain settings
                    for (int chn = 0; chn < rx_channel_cfgs.size(); chn++)
                    {
                        std::string name = "rx" + std::to_string(chn + 1) + "_";

                        if (key == (name + "path"))
                        {
                            rx_channel_cfgs[chn].path = v;
                            set_paths();
                            invalid = false;
                            r = RES_LISTUPD;
                        }
                        else if (key == (name + "manual_gain"))
                        {
                            rx_channel_cfgs[chn].gain_mode_manual = v;
                            set_gains();
                            invalid = false;
                            r = RES_LISTUPD;
                        }
                        else if (key == (name + "lna_gain"))
                        {
                            rx_channel_cfgs[chn].gain_lna = v;
                            set_gains();
                            invalid = false;
                        }
                        else if (key == (name + "tia_gain"))
                        {
                            rx_channel_cfgs[chn].gain_tia = v;
                            set_gains();
                            invalid = false;
                        }
                        else if (key == (name + "pga_gain"))
                        {
                            rx_channel_cfgs[chn].gain_pga = v;
                            set_gains();
                            invalid = false;
                        }
                        else if (key == (name + "gain"))
                        {
                            rx_channel_cfgs[chn].gain = v;
                            set_gains();
                            invalid = false;
                        }
                    }

                    // Handle TX Gain settings
                    for (int chn = 0; chn < tx_channel_cfgs.size(); chn++)
                    {
                        std::string name = "tx" + std::to_string(chn + 1) + "_";

                        if (key == (name + "path"))
                        {
                            tx_channel_cfgs[chn].path = v;
                            set_paths();
                            invalid = false;
                            r = RES_LISTUPD;
                        }
                        else if (key == (name + "gain"))
                        {
                            tx_channel_cfgs[chn].gain = v;
                            set_gains();
                            invalid = false;
                        }
                    }

                    if (invalid)
                        throw satdump_exception(key);
                }
                return r;
            }

            double getStreamSamplerate(int id, bool output)
            {
                if (output ? (id >= inputs.size()) : (id >= outputs.size()))
                    throw satdump_exception("Stream ID must be <= channel count in direction!");
                return samplerate;
            }

            virtual void setStreamSamplerate(int id, bool output, double samplerate)
            {
                if (output ? (id >= inputs.size()) : (id >= outputs.size()))
                    throw satdump_exception("Stream ID must be <= channel count in direction!");
                set_cfg("samplerate", samplerate);
            }

            virtual double getStreamFrequency(int id, bool output)
            {
                if (output ? (id >= inputs.size()) : (id >= outputs.size()))
                    throw satdump_exception("Stream ID must be <= channel count in direction!");
                return output ? tx_frequency : rx_frequency;
            }

            virtual void setStreamFrequency(int id, bool output, double frequency)
            {
                if (output ? (id >= inputs.size()) : (id >= outputs.size()))
                    throw satdump_exception("Stream ID must be <= channel count in direction!");
                if (output)
                    set_cfg("tx_frequency", frequency);
                else
                    set_cfg("rx_frequency", frequency);
            }

            void start();
            void stop(bool stop_now = false, bool force = false);
            bool is_async() { return rx_ch_number > 0; }

        public:
            static std::vector<DeviceInfo> listDevs();
        };
    } // namespace ndsp
} // namespace satdump