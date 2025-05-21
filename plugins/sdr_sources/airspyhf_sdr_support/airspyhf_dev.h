#pragma once

#include "core/exception.h"
#include "dsp/block.h"
#include "dsp/device/dev.h"

#ifdef __ANDROID__
#include "airspyhf.h"
#else
#include <libairspyhf/airspyhf.h>
#endif

// TODOREWORK Change the namespace?
namespace satdump
{
    namespace ndsp
    {
        class AirspyHFDevBlock : public DeviceBlock
        {
        public:
            std::string p_serial = "0";
            double p_frequency = 100e6;
            double p_samplerate = 768e3;

            std::string agc_mode = "off";
            int attenuation = 0;
            bool hf_lna_enabled = false;

        private:
            bool is_open = false, is_started = false;
            airspyhf_device *airspyhf_dev_obj;

            static int _rx_callback(airspyhf_transfer_t *t);

        public:
            AirspyHFDevBlock();
            ~AirspyHFDevBlock();

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
                    add_param_list(p, "samplerate", "samplerate", {192e3, 256e3, 384e3, 768e3});
                p["samplerate"]["disable"] = is_started;

                add_param_range(p, "attenuation", "int", 0, 48, 1, "Attenuation");
                add_param_list(p, "agc_mode", "string", {"off", "low", "high"}, "AGC Mode");
                add_param_simple(p, "hf_lna", "bool", "HF LNA");

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
                else if (key == "agc_mode")
                    return agc_mode;
                else if (key == "attenuation")
                    return attenuation;
                else if (key == "hf_lna")
                    return hf_lna_enabled;
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
                else if (key == "agc_mode")
                {
                    agc_mode = v;
                    set_others();
                }
                else if (key == "attenuation")
                {
                    attenuation = v;
                    r = RES_LISTUPD;
                    set_gains();
                }
                else if (key == "hf_lna")
                {
                    hf_lna_enabled = v;
                    set_gains();
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