#pragma once

#include "core/exception.h"
#include "dsp/block.h"
#include "dsp/device/dev.h"

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <litex_m2sdr/libm2sdr/m2sdr.h>
#include <string>
#include <volk/volk_alloc.hh>

#include "common/dsp/complex.h"
#include "logger.h"
#include "nlohmann/json.hpp"

// TODOREWORK Change the namespace?
namespace satdump
{
    namespace ndsp
    {
        class LiteXM2SDRDevBlock : public DeviceBlock
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

            // Gains are actually split
            int rx_gain[2] = {10, 10};
            int tx_gain[2] = {10, 10};

            bool extclock = false;

            // Bandwidth config
            bool manual_bw = false;
            double bandwidth = 6e6;

            // TODOREWORK, not here but in the config add back bandwidth/manual banwidth?

        private:
            bool is_dual_ch = false;
            bool is_8bit = false;
            int samples_per_buf;

            std::thread work_thread_rx;
            bool thread_should_run_rx = false;
            void mainThread_rx();

            std::thread work_thread_tx;
            void mainThread_tx();

        private:
            bool is_open = false, is_started = false;
            m2sdr_dev *litexm2sdr_dev_obj;

        public:
            LiteXM2SDRDevBlock();
            ~LiteXM2SDRDevBlock();

            void init();

            void setDevInfo(DeviceInfo i, device_mode_t mode);
            nlohmann::ordered_json get_cfg_list();
            nlohmann::json get_cfg(std::string key);
            cfg_res_t set_cfg(std::string key, nlohmann::json v);

            void set_frequency();
            void set_gains();
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