#include "litexm2sdr_dev.h"

namespace satdump
{
    namespace ndsp
    {
        void LiteXM2SDRDevBlock::setDevInfo(DeviceInfo i, device_mode_t mode)
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

        nlohmann::ordered_json LiteXM2SDRDevBlock::get_cfg_list()
        {
            nlohmann::ordered_json p;

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

            add_param_simple(p, "samplerate", "uint", "Samplerate");
            p["samplerate"]["disable"] = is_started;
            p["samplerate"]["range_noslider"] = true;

            add_param_simple(p, "manual_bw", "bool", "Manual Bandwidth");
            if (manual_bw)
                add_param_simple(p, "bandwidth", "uint", "Bandwidth");

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
                add_param_simple(p, "tx1_gain", "float", "TX1 Attenuation");
            if (tx_ch_number >= 2 || tx_ch_id == 1)
                add_param_simple(p, "tx2_gain", "float", "TX2 Attenuation");

            add_param_simple(p, "extclock", "bool", "External Clock");
            p["extclock"]["disable"] = is_started;

            return p;
        }

        nlohmann::json LiteXM2SDRDevBlock::get_cfg(std::string key)
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
            else if (key == "rx1_gain")
                return rx_gain[0];
            else if (key == "rx2_gain")
                return rx_gain[1];
            else if (key == "tx1_gain")
                return tx_gain[0];
            else if (key == "tx2_gain")
                return tx_gain[1];
            else if (key == "extclock")
                return extclock;
            else
                throw satdump_exception(key);
        }

        Block::cfg_res_t LiteXM2SDRDevBlock::set_cfg(std::string key, nlohmann::json v)
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
            else if (key == "extclock")
                extclock = v;
            else
                throw satdump_exception(key);
            return r;
        }

        double LiteXM2SDRDevBlock::getStreamSamplerate(int id, bool output)
        {
            if (output ? (id >= inputs.size()) : (id >= outputs.size()))
                throw satdump_exception("Stream ID must be <= channel count in direction!");
            return samplerate;
        }

        void LiteXM2SDRDevBlock::setStreamSamplerate(int id, bool output, double samplerate)
        {
            if (output ? (id >= inputs.size()) : (id >= outputs.size()))
                throw satdump_exception("Stream ID must be <= channel count in direction!");
            set_cfg("samplerate", samplerate);
        }

        double LiteXM2SDRDevBlock::getStreamFrequency(int id, bool output)
        {
            if (output ? (id >= inputs.size()) : (id >= outputs.size()))
                throw satdump_exception("Stream ID must be <= channel count in direction!");
            return output ? tx_frequency : rx_frequency;
        }

        void LiteXM2SDRDevBlock::setStreamFrequency(int id, bool output, double frequency)
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