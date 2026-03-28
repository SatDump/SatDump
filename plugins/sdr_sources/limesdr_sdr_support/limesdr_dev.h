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
            void mainThread_rx();

            std::thread work_thread_tx;
            void mainThread_tx();

        private:
            bool is_open = false, is_started = false;
            std::shared_ptr<lime::LMS7_Device> limesdr_dev_obj;
            std::vector<lime::StreamChannel *> rx_streams;
            std::vector<lime::StreamChannel *> tx_streams;

        public:
            LimeSDRDevBlock();
            ~LimeSDRDevBlock();

            void init();

            void setDevInfo(DeviceInfo i, device_mode_t mode);
            nlohmann::ordered_json get_cfg_list();
            nlohmann::json get_cfg(std::string key);
            cfg_res_t set_cfg(std::string key, nlohmann::json v);

            void set_frequency();
            void set_gains();
            void set_paths();
            void set_bw();

            double getStreamSamplerate(int id, bool output);
            void setStreamSamplerate(int id, bool output, double samplerate);
            double getStreamFrequency(int id, bool output);
            void setStreamFrequency(int id, bool output, double frequency);

            void start();
            void stop(bool stop_now = false, bool force = false);
            bool is_async() { return rx_ch_number > 0; }

        public:
            static std::vector<DeviceInfo> listDevs();
        };
    } // namespace ndsp
} // namespace satdump