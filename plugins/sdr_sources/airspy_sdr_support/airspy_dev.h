#pragma once

#include "core/exception.h"
#include "dsp/block.h"
#include "dsp/device/dev.h"

#ifdef __ANDROID__
#include "airspy.h"
#else
#include <libairspy/airspy.h>
#endif

// TODOREWORK Change the namespace?
namespace satdump
{
    namespace ndsp
    {
        class AirspyDevBlock : public DeviceBlock
        {
        public:
            std::string p_serial = " 0";
            double p_frequency = 100e6;
            double p_samplerate = 2.5e6;

            std::string p_gain_type = "sensitive";
            int p_general_gain = 0;
            int p_lna_gain = 0;
            int p_mixer_gain = 0;
            int p_vga_gain = 0;

            bool p_bias = false;
            bool p_lna_agc = false;
            bool p_mixer_agc = false;

        private:
            bool is_open = false, is_started = false;
            airspy_device *airspy_dev_obj;

            static int _rx_callback(airspy_transfer *t);

        public:
            AirspyDevBlock();
            ~AirspyDevBlock();

            void init();

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;

                p = devInfo.params;

                p["serial"]["type"] = "string";
                if (devInfo.cfg.contains("serial"))
                    p["serial"]["hide"] = true;

                p["frequency"]["type"] = "float";

                if (!p.contains("samplerate"))
                {
                    p["samplerate"]["type"] = "uint";
                    p["samplerate"]["list"] = {2.5e6, 3e6, 6e6, 10e6};
                }
                p["samplerate"]["disable"] = is_started;

                p["gain_type"]["type"] = "string";
                p["gain_type"]["list"] = {"sensitive", "linear", "manual"};

                if (p_gain_type == "sensitive" || p_gain_type == "linear")
                {
                    p["general_gain"]["type"] = "int";
                    p["general_gain"]["range"] = {0, 21, 1};
                }
                else
                {
                    p["lna_gain"]["type"] = "int";
                    p["lna_gain"]["range"] = {0, 15, 1};

                    p["mixer_gain"]["type"] = "int";
                    p["mixer_gain"]["range"] = {0, 15, 1};

                    p["vga_gain"]["type"] = "int";
                    p["vga_gain"]["range"] = {0, 15, 1};
                }

                p["lna_agc"]["type"] = "bool";

                p["mixer_agc"]["type"] = "bool";

                p["bias"]["type"] = "bool";

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
                else if (key == "general_gain")
                    return p_general_gain;
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

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                cfg_res_t r = RES_OK;
                if (key == "serial")
                    p_serial = v;
                else if (key == "samplerate")
                    p_samplerate = v;
                else if (key == "frequency")
                    p_frequency = v;
                else if (key == "gain_type")
                {
                    p_gain_type = v;
                    r = RES_LISTUPD;
                }
                else if (key == "general_gain")
                    p_general_gain = v;
                else if (key == "lna_gain")
                    p_lna_gain = v;
                else if (key == "mixer_gain")
                    p_mixer_gain = v;
                else if (key == "vga_gain")
                    p_vga_gain = v;
                else if (key == "lna_agc")
                    p_lna_agc = v;
                else if (key == "mixer_agc")
                    p_mixer_agc = v;
                else if (key == "bias")
                    p_bias = v;
                else
                    throw satdump_exception(key);
                init();
                return r;
            }

            double getStreamSamplerate(int id, bool output)
            {
                if (id > 0 || output)
                    throw satdump_exception("Stream ID must be 0 and input only!");
                return p_samplerate;
            }

            void start();
            void stop(bool stop_now = false);

        public:
            static std::vector<DeviceInfo> listDevs();
        };
    } // namespace ndsp
} // namespace satdump