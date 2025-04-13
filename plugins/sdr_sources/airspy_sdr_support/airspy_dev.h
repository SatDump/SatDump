#pragma once

#include "dsp/device/dev.h"

#include <libairspy/airspy.h>

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

            nlohmann::json get_cfg_list()
            {
                nlohmann::json p;

                p = devInfo.params;

                p["serial"]["type"] = "string";

                p["frequency"]["type"] = "float";

                if (!p.contains("samplerate"))
                {
                    p["samplerate"]["type"] = "uint";
                    p["samplerate"]["list"] = {2.5e6, 3e6, 6e6, 10e6};
                }

                p["gain_type"]["type"] = "string";
                p["gain_type"]["list"] = {"sensitive", "linear", "manual"};

                p["general_gain"]["type"] = "int";
                p["general_gain"]["range"] = {0, 21, 1};

                p["lna_gain"]["type"] = "int";
                p["lna_gain"]["range"] = {0, 15, 1};

                p["mixer_gain"]["type"] = "int";
                p["mixer_gain"]["range"] = {0, 15, 1};

                p["vga_gain"]["type"] = "int";
                p["vga_gain"]["range"] = {0, 15, 1};

                p["lna_agc"]["type"] = "bool";

                p["mixer_agc"]["type"] = "bool";

                p["bias"]["type"] = "bool";

                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                nlohmann::json v;
                v["serial"] = p_serial;
                auto &r = v["rx0"];
                r["samplerate"] = p_samplerate;
                r["frequency"] = p_frequency;
                r["gain_type"] = p_gain_type;
                r["general_gain"] = p_general_gain;
                r["lna_gain"] = p_lna_gain;
                r["mixer_gain"] = p_mixer_gain;
                r["vga_gain"] = p_vga_gain;
                r["lna_agc"] = p_lna_agc;
                r["mixer_agc"] = p_mixer_agc;
                r["bias"] = p_bias;
                return v;
            }

            void set_cfg(std::string key, nlohmann::json v)
            {
                if (key == "serial")
                    p_serial = v;
                else if (key == "samplerate")
                    p_samplerate = v;
                else if (key == "frequency")
                    p_frequency = v;
                else if (key == "gain_type")
                    p_gain_type = v;
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
            }

            void drawUI()
            {
            }

            void start();
            void stop(bool stop_now = false);

        public:
            static std::vector<DeviceInfo> listDevs();
        };
    }
}