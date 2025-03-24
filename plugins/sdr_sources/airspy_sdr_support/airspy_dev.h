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
                nlohmann::json p = devInfo.params;

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

            nlohmann::json get_cfg()
            {
                nlohmann::json v;
                v["serial"] = p_serial;
                v["samplerate"] = p_samplerate;
                v["frequency"] = p_frequency;
                v["gain_type"] = p_gain_type;
                v["general_gain"] = p_general_gain;
                v["lna_gain"] = p_lna_gain;
                v["mixer_gain"] = p_mixer_gain;
                v["vga_gain"] = p_vga_gain;
                v["lna_agc"] = p_lna_agc;
                v["mixer_agc"] = p_mixer_agc;
                v["bias"] = p_bias;
                return v;
            }

            void set_cfg(nlohmann::json v)
            {
                setValFromJSONIfExists(p_serial, v["serial"]);
                setValFromJSONIfExists(p_samplerate, v["samplerate"]);
                setValFromJSONIfExists(p_frequency, v["frequency"]);
                setValFromJSONIfExists(p_gain_type, v["gain_type"]);
                setValFromJSONIfExists(p_general_gain, v["general_gain"]);
                setValFromJSONIfExists(p_lna_gain, v["lna_gain"]);
                setValFromJSONIfExists(p_mixer_gain, v["mixer_gain"]);
                setValFromJSONIfExists(p_vga_gain, v["vga_gain"]);
                setValFromJSONIfExists(p_lna_agc, v["lna_agc"]);
                setValFromJSONIfExists(p_mixer_agc, v["mixer_agc"]);
                setValFromJSONIfExists(p_bias, v["bias"]);
            }

            void drawUI()
            {
            }

            virtual void start();
            virtual void stop(bool stop_now = false);

        public:
            static std::vector<DeviceInfo> listDevs();
        };
    }
}