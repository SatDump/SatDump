#pragma once

#include "core/exception.h"
#include "dsp/block.h"
#include "dsp/device/dev.h"

#ifdef __ANDROID__
#include "hydrasdr.h"
#else
#include <libhydrasdr/hydrasdr.h>
#endif

// TODOREWORK Change the namespace?
namespace satdump
{
    namespace ndsp
    {
        class HydraSDRDevBlock : public DeviceBlock
        {
        public:
            std::string p_serial = "0";
            double p_frequency = 100e6;
            double p_samplerate = 2.5e6;

            std::string p_gain_type = "sensitive";
            int p_gain = 0;
            int p_lna_gain = 0;
            int p_mixer_gain = 0;
            int p_vga_gain = 0;

            bool p_bias = false;
            bool p_lna_agc = false;
            bool p_mixer_agc = false;

        private:
            bool is_open = false, is_started = false;
            hydrasdr_device *hydrasdr_dev_obj;

            static int _rx_callback(hydrasdr_transfer *t);

        public:
            HydraSDRDevBlock();
            ~HydraSDRDevBlock();

            void init();

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;

                p = devInfo.params;

                p["frequency"]["type"] = "freq";

                p["serial"]["type"] = "string";
                p["serial"]["hide"] = devInfo.cfg.contains("serial");
                p["serial"]["disable"] = is_started;

                if (!p.contains("samplerate"))
                    add_param_list(p, "samplerate", "samplerate", {2.5e6, 5e6, 10e6});
                p["samplerate"]["disable"] = is_started;

                add_param_list(p, "gain_type", "string", {"sensitive", "linear", "manual"}, "Gain Type");

                if (p_gain_type == "sensitive" || p_gain_type == "linear")
                {
                    add_param_range(p, "gain", "int", 0, 21, 1, "Gain");
                }
                else
                {
                    add_param_range(p, "lna_gain", "int", 0, 15, 1, "LNA Gain");
                    add_param_range(p, "mixer_gain", "int", 0, 15, 1, "Mixer Gain");
                    add_param_range(p, "vga_gain", "int", 0, 15, 1, "VGA Gain");
                }

                add_param_simple(p, "lna_agc", "bool", "LNA AGC");
                add_param_simple(p, "mixer_agc", "bool", "Mixer AGC");
                add_param_simple(p, "bias", "bool", "Bias");

                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "serial")
                    return p_serial;
                else if (key == "samplerate")
                    return p_samplerate;
                else if (key == "frequency")
                    return p_frequency;
                else if (key == "gain_type")
                    return p_gain_type;
                else if (key == "gain")
                    return p_gain;
                else if (key == "lna_gain")
                    return p_lna_gain;
                else if (key == "mixer_gain")
                    return p_mixer_gain;
                else if (key == "vga_gain")
                    return p_vga_gain;
                else if (key == "lna_agc")
                    return p_lna_agc;
                else if (key == "mixer_agc")
                    return p_mixer_agc;
                else if (key == "bias")
                    return p_bias;
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
                    p_serial = v;
                else if (key == "samplerate")
                    p_samplerate = v;
                else if (key == "frequency")
                {
                    p_frequency = v;
                    set_frequency();
                }
                else if (key == "gain_type")
                {
                    p_gain_type = v;
                    r = RES_LISTUPD;
                    set_gains();
                }
                else if (key == "gain")
                {
                    p_gain = v;
                    set_gains();
                }
                else if (key == "lna_gain")
                {
                    p_lna_gain = v;
                    set_gains();
                }
                else if (key == "mixer_gain")
                {
                    p_mixer_gain = v;
                    set_gains();
                }
                else if (key == "vga_gain")
                {
                    p_vga_gain = v;
                    set_gains();
                }
                else if (key == "lna_agc")
                {
                    p_lna_agc = v;
                    set_others();
                }
                else if (key == "mixer_agc")
                {
                    p_mixer_agc = v;
                    set_others();
                }
                else if (key == "bias")
                {
                    p_bias = v;
                    set_others();
                }
                else
                    throw satdump_exception(key);
                return r;
            }

            double getStreamSamplerate(int id, bool output)
            {
                if (id > 0 || output)
                    throw satdump_exception("Stream ID must be 0 and input only!");
                return p_samplerate;
            }

            virtual void setStreamSamplerate(int id, bool output, double samplerate)
            {
                if (id > 0 || output)
                    throw satdump_exception("Stream ID must be 0 and input only!");
                set_cfg("samplerate", samplerate);
            }

            virtual double getStreamFrequency(int id, bool output)
            {
                if (id > 0 || output)
                    throw satdump_exception("Stream ID must be 0 and input only!");
                return p_frequency;
            }

            virtual void setStreamFrequency(int id, bool output, double frequency)
            {
                if (id > 0 || output)
                    throw satdump_exception("Stream ID must be 0 and input only!");
                set_cfg("frequency", frequency);
            }

            void start();
            void stop(bool stop_now = false);

        public:
            static std::vector<DeviceInfo> listDevs();
        };
    } // namespace ndsp
} // namespace satdump