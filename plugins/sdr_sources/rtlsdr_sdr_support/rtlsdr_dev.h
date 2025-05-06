#pragma once

#include "core/exception.h"
#include "dsp/block.h"
#include "dsp/device/dev.h"
#include "logger.h"

#ifdef __ANDROID__
#include "rtl-sdr.h"
#else
#include <rtl-sdr.h>
#endif

// TODOREWORK Change the namespace?
namespace satdump
{
    namespace ndsp
    {
        class RTLSDRDevBlock : public DeviceBlock
        {
        public:
            std::string p_serial = "0";
            double p_frequency = 100e6;
            double p_samplerate = 1.024e6;

            int set_gain = 100; // Actual value to set
            int gain = 0;       // Internal only
            int last_ppm = 0;   // Internal only
            int ppm_val = 0;

            bool changed_agc = true; // Internal only
            bool bias_enabled = false;
            bool lna_agc_enabled = false;
            bool tuner_agc_enabled = false;

        private:
            bool is_open = false, is_started = false;
            rtlsdr_dev *rtlsdr_dev_obj;
            std::vector<int> available_gains = {0, 496};

            static void _rx_callback(unsigned char *buf, uint32_t len, void *ctx);

            std::thread work_thread;
            bool thread_should_run = false;
            void mainThread()
            {
                int buffer_size = 8192; // TODOREWORK calculate_buffer_size_from_samplerate(samplerate_widget.get_value());
                // std::min<int>(roundf(samplerate_widget.get_value() / (250 * 512)) * 512, dsp::STREAM_BUFFER_SIZE);
                logger->trace("RTL-SDR Buffer size %d", buffer_size);

                while (thread_should_run)
                    rtlsdr_read_async(rtlsdr_dev_obj, _rx_callback, this, 0, buffer_size);
            }

        public:
            RTLSDRDevBlock();
            ~RTLSDRDevBlock();

            void init();

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;

                p["frequency"]["type"] = "freq";

                p["serial"]["type"] = "string";
                p["serial"]["hide"] = devInfo.cfg.contains("serial");
                p["serial"]["disable"] = is_started;

                add_param_list(p, "samplerate", "samplerate", {250000, 1024000, 1536000, 1792000, 1920000, 2048000, 2160000, 2400000, 2560000, 2880000, 3200000});

                add_param_simple(p, "ppm_correction", "int", "PPM Correction");

                add_param_range(p, "gain", "float", 0, 49.6, 0.1, "Tuner Gain");
                if (devInfo.params.contains("gain"))
                    p["gain"]["range"] = devInfo.params["gain"]["range"];

                add_param_simple(p, "lna_agc", "bool", "LNA AGC");
                add_param_simple(p, "tuner_agc", "bool", "Tuner AGC");
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
                else if (key == "gain")
                    return set_gain / 10.0f;
                else if (key == "lna_agc")
                    return lna_agc_enabled;
                else if (key == "tuner_agc")
                    return tuner_agc_enabled;
                else if (key == "bias")
                    return bias_enabled;
                else if (key == "ppm_correction")
                    return ppm_val;
                else
                    throw satdump_exception(key);
            }

            void set_frequency();
            void set_gains();
            void set_bias();
            void set_ppm();

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
                else if (key == "gain")
                {
                    set_gain = v.get<float>() * 10.0f;
                    set_gains();
                }
                else if (key == "lna_agc")
                {
                    lna_agc_enabled = v;
                    changed_agc = true;
                    set_gains();
                }
                else if (key == "tuner_agc")
                {
                    tuner_agc_enabled = v;
                    changed_agc = true;
                    set_gains();
                }
                else if (key == "bias")
                {
                    bias_enabled = v;
                    set_bias();
                }
                else if (key == "ppm_correction")
                {
                    ppm_val = v;
                    set_ppm();
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