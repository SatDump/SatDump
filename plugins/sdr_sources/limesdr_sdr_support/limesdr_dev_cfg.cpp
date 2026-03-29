#include "core/exception.h"
#include "dsp/block.h"
#include "limesdr_dev.h"
#include "nlohmann/json.hpp"
#include <lime/LimeSuite.h>
#include <logger.h>
#include <string>

namespace satdump
{
    namespace ndsp
    {
        void LimeSDRDevBlock::setDevInfo(DeviceInfo i, device_mode_t mode)
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

        nlohmann::ordered_json LimeSDRDevBlock::get_cfg_list()
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

            for (int chn = 0; chn < rx_ch_number; chn++)
            {
                std::string name = "rx" + std::to_string(chn + 1) + "_";

                add_param_list(p, name + "path", "string", {"NONE", "LNAH", "LNAL", "LNAW", "Auto"}, "RX" + std::to_string(chn + 1) + " Path");
#if !LIMESUITENG
                add_param_simple(p, name + "manual_gain", "bool", "RX" + std::to_string(chn + 1) + " Manual Gain");

                if (rx_channel_cfgs[chn].gain_mode_manual)
                {
                    add_param_range(p, name + "lna_gain", "int", 0, 30, 1, "RX" + std::to_string(chn + 1) + " LNA Gain");
                    add_param_range(p, name + "tia_gain", "int", 0, 12, 1, "RX" + std::to_string(chn + 1) + " TIA Gain");
                    add_param_range(p, name + "pga_gain", "int", -12, 19, 1, "RX" + std::to_string(chn + 1) + " PGA Gain");
                }
                else
#endif
                {
                    add_param_range(p, name + "gain", "int", 0, 73, 1, "RX" + std::to_string(chn + 1) + " Gain");
                }
            }

            for (int chn = 0; chn < tx_ch_number; chn++)
            {
                std::string name = "tx" + std::to_string(chn + 1) + "_";

                add_param_list(p, name + "path", "string", {"NONE", "BAND1", "BAND2", "Auto"}, "TX" + std::to_string(chn + 1) + " Path");
                add_param_range(p, name + "gain", "int", 0, 73, 1, "TX" + std::to_string(chn + 1) + " Gain");
            }

            return p;
        }

        nlohmann::json LimeSDRDevBlock::get_cfg(std::string key)
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
                for (int chn = 0; chn < MAX_CHANNELS; chn++)
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
                for (int chn = 0; chn < MAX_CHANNELS; chn++)
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

        Block::cfg_res_t LimeSDRDevBlock::set_cfg(std::string key, nlohmann::json v)
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
                if (rx_ch_number > MAX_CHANNELS)
                    rx_ch_number = MAX_CHANNELS;

                int diff = rx_ch_number - rx_ch_number_old;

                if (diff != 0)
                {
                    outputs.clear();
                    if (rx_ch_number == 1)
                    {
                        outputs.push_back({"RX", DSP_SAMPLE_TYPE_CF32});
                    }
                    else
                    {
                        for (int i = 0; i < rx_ch_number; i++)
                        {
                            outputs.push_back({"RX" + std::to_string(i), DSP_SAMPLE_TYPE_CF32});
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
                if (tx_ch_number > MAX_CHANNELS)
                    tx_ch_number = MAX_CHANNELS;

                int diff = tx_ch_number - tx_ch_number_old;

                if (diff != 0)
                {
                    inputs.clear();
                    if (tx_ch_number == 1)
                    {
                        inputs.push_back({"TX", DSP_SAMPLE_TYPE_CF32});
                    }
                    else
                    {
                        for (int i = 0; i < tx_ch_number; i++)
                        {
                            inputs.push_back({"TX" + std::to_string(i), DSP_SAMPLE_TYPE_CF32});
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
                for (int chn = 0; chn < MAX_CHANNELS; chn++)
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
                for (int chn = 0; chn < MAX_CHANNELS; chn++)
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

        double LimeSDRDevBlock::getStreamSamplerate(int id, bool output)
        {
            if (output ? (id >= inputs.size()) : (id >= outputs.size()))
                throw satdump_exception("Stream ID must be <= channel count in direction!");
            return samplerate;
        }

        void LimeSDRDevBlock::setStreamSamplerate(int id, bool output, double samplerate)
        {
            if (output ? (id >= inputs.size()) : (id >= outputs.size()))
                throw satdump_exception("Stream ID must be <= channel count in direction!");
            set_cfg("samplerate", samplerate);
        }

        double LimeSDRDevBlock::getStreamFrequency(int id, bool output)
        {
            if (output ? (id >= inputs.size()) : (id >= outputs.size()))
                throw satdump_exception("Stream ID must be <= channel count in direction!");
            return output ? tx_frequency : rx_frequency;
        }

        void LimeSDRDevBlock::setStreamFrequency(int id, bool output, double frequency)
        {
            if (output ? (id >= inputs.size()) : (id >= outputs.size()))
                throw satdump_exception("Stream ID must be <= channel count in direction!");
            if (output)
                set_cfg("tx_frequency", frequency);
            else
                set_cfg("rx_frequency", frequency);
        }
    } // namespace ndsp
} // namespace satdump