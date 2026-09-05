#pragma once

#include "common/dsp/complex.h"
#include "core/exception.h"
#include "dsp/block.h"
#include "dsp/device/dev.h"
#include "dynload.h"
#include <unistd.h>

#ifdef __ANDROID__
#include "airspy.h"
#else
#include <libairspy/airspy.h>
#endif

// #define SPECTRAN_SAMPLERATE_46M 46080000
#define SPECTRAN_SAMPLERATE_61_44M 61440000
#define SPECTRAN_SAMPLERATE_92M 92160000
#define SPECTRAN_SAMPLERATE_122M 122880000
#define SPECTRAN_SAMPLERATE_184M 184320000
#define SPECTRAN_SAMPLERATE_245M 245760000

// TODOREWORK Change the namespace?
namespace satdump
{
    namespace ndsp
    {
        class SpectranV6DevBlock : public DeviceBlock
        {
        public:
            bool d_is_eco = false;

            std::string p_serial = "0";
            double p_frequency = 100e6;
            double p_samplerate = 2.5e6;

            std::string d_rx_channel = "Rx1";

            float d_min_level = -20;

            float d_level = -20;
            std::string d_usb_compression = "auto";
            std::string d_agc_mode = "manual";
            bool d_enable_amp = 0;
            bool d_enable_preamp = 0;

            bool d_rescale = 0;
            float d_rescale_val = 1;

        private:
            bool is_open = false, is_started = false;

            AARTSAAPI_Handle aaronia_handle;
            AARTSAAPI_DeviceInfo aaronia_dinfo;
            AARTSAAPI_Device aaronia_device;
            AARTSAAPI_Config config, root;

            std::thread work_thread;
            bool thread_should_run = false;
            void mainThread()
            {
                AARTSAAPI_Packet packet;
                AARTSAAPI_Result res;

                while (thread_should_run)
                {
                    while ((res = rtsa_api->AARTSAAPI_GetPacket(&aaronia_device, 0, 0, &packet)) == AARTSAAPI_EMPTY)
#ifdef _WIN32
                        Sleep(1);
#else
                        usleep(1000);
#endif

                    if (res == AARTSAAPI_OK)
                    {
                        int cnt = packet.num;

                        if (cnt > 0)
                        {
                            DSPBuffer oblk = outputs[0].fifo->newBufferSamples(cnt, sizeof(complex_t));

                            // Optionally re-scale to be in the more "standard" 1.0f range
                            if (d_rescale)
                                volk_32fc_s32fc_multiply_32fc((lv_32fc_t *)oblk.getSamples<complex_t>(), (lv_32fc_t *)packet.fp32, d_rescale_val, cnt);
                            else
                                memcpy(oblk.getSamples<complex_t>(), (complex_t *)packet.fp32, cnt * sizeof(complex_t));

                            oblk.size = cnt;
                            outputs[0].fifo->wait_enqueue(oblk);
                        }

                        rtsa_api->AARTSAAPI_ConsumePackets(&aaronia_device, 0, 1);
                    }
                }
            }

        public:
            SpectranV6DevBlock();
            ~SpectranV6DevBlock();

            void init();

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;

                p = devInfo.params;

                p["frequency"]["type"] = "freq";

                if (!devInfo.cfg.contains("is_eco"))
                    add_param_simple(p, "is_eco", "bool", "Is ECO");

                p["serial"]["type"] = "string";
                p["serial"]["hide"] = devInfo.cfg.contains("serial");
                p["serial"]["disable"] = is_started;

                if (d_is_eco)
                    add_param_list(p, "samplerate", "samplerate",
                                   {
                                       SPECTRAN_SAMPLERATE_61_44M / 128,
                                       SPECTRAN_SAMPLERATE_61_44M / 64,
                                       SPECTRAN_SAMPLERATE_61_44M / 32,
                                       SPECTRAN_SAMPLERATE_61_44M / 16,
                                       SPECTRAN_SAMPLERATE_61_44M / 8,
                                       SPECTRAN_SAMPLERATE_61_44M / 4,
                                       SPECTRAN_SAMPLERATE_61_44M / 2,
                                       SPECTRAN_SAMPLERATE_61_44M,
                                   },
                                   "Samplerate");
                else
                {
                    add_param_list(p, "samplerate", "samplerate",
                                   {
                                       SPECTRAN_SAMPLERATE_92M / 128,
                                       SPECTRAN_SAMPLERATE_92M / 64,
                                       SPECTRAN_SAMPLERATE_92M / 32,
                                       SPECTRAN_SAMPLERATE_92M / 16,
                                       SPECTRAN_SAMPLERATE_92M / 8,
                                       SPECTRAN_SAMPLERATE_92M / 4,
                                       SPECTRAN_SAMPLERATE_92M / 2,
                                       SPECTRAN_SAMPLERATE_92M,
                                       SPECTRAN_SAMPLERATE_122M,
                                       SPECTRAN_SAMPLERATE_184M,
                                       SPECTRAN_SAMPLERATE_245M,
                                   },
                                   "Samplerate");
                }
                p["samplerate"]["disable"] = is_started;

                add_param_list(p, "channel", "string", {"Rx1", "Rx2"}, "Channel");
                p["channel"]["disable"] = is_started;

                add_param_list(p, "usb_compression", "string", {"auto", "raw", "compressed"}, "USB Compression");

                if (d_enable_preamp)
                    d_min_level = -38;
                else
                    d_min_level = -20;

                add_param_range(p, "ref_level", "float", d_min_level, 10.0f, 1, "Ref Level");
                add_param_list(p, "agc_mode", "string", {"manual", "peak", "power"}, "AGC Mode");
                add_param_simple(p, "enable_amp", "bool", "Amp");
                add_param_simple(p, "enable_preamp", "bool", "Preamp");

                add_param_simple(p, "rescale", "bool", "Rescale");
                if (d_rescale)
                    add_param_range(p, "rescale_val", "float", 1.0, 10000, 1, "Rescale Val");

                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "is_eco")
                    return d_is_eco;
                else if (key == "serial")
                    return p_serial;
                else if (key == "samplerate")
                    return p_samplerate;
                else if (key == "frequency")
                    return p_frequency;
                else if (key == "channel")
                    return d_rx_channel;
                else if (key == "usb_compression")
                    return d_usb_compression;
                else if (key == "ref_level")
                    return d_level;
                else if (key == "agc_mode")
                    return d_agc_mode;
                else if (key == "enable_amp")
                    return d_enable_amp;
                else if (key == "enable_preamp")
                    return d_enable_preamp;
                else if (key == "rescale")
                    return d_rescale;
                else if (key == "rescale_val")
                    return d_rescale_val;
                else
                    throw satdump_exception(key);
            }

            void set_frequency();
            void set_gains();
            void set_others();

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                cfg_res_t r = RES_OK;
                if (key == "is_eco")
                {
                    d_is_eco = v;
                    r = RES_LISTUPD;
                }
                else if (key == "serial")
                    p_serial = v;
                else if (key == "samplerate")
                    p_samplerate = v;
                else if (key == "frequency")
                {
                    p_frequency = v;
                    set_frequency();
                }
                else if (key == "channel")
                    d_rx_channel = v;
                else if (key == "usb_compression")
                {
                    d_usb_compression = v;
                    set_others();
                }
                else if (key == "ref_level")
                {
                    d_level = v;
                    set_gains();
                }
                else if (key == "agc_mode")
                {
                    d_agc_mode = v;
                    set_gains();
                }
                else if (key == "enable_amp")
                {
                    d_enable_amp = v;
                    r = RES_LISTUPD;
                    set_gains();
                }
                else if (key == "enable_preamp")
                {
                    d_enable_preamp = v;
                    r = RES_LISTUPD;
                    set_gains();
                }
                else if (key == "rescale")
                {
                    d_rescale = v;
                    r = RES_LISTUPD;
                }
                else if (key == "rescale_val")
                {
                    d_rescale_val = v;
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
            void stop(bool stop_now = false, bool force = false);

        public:
            static std::vector<DeviceInfo> listDevs();
        };
    } // namespace ndsp
} // namespace satdump