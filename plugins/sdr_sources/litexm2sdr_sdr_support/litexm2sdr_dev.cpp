#include "litexm2sdr_dev.h"
#include "common/dsp/complex.h"
#include "core/exception.h"
#include "dsp/block.h"
#include "m2sdr.h"
#include "nlohmann/json.hpp"
#include <cstdint>
#include <logger.h>
#include <string>

namespace satdump
{
    namespace ndsp
    {
        LiteXM2SDRDevBlock::LiteXM2SDRDevBlock() : DeviceBlock("litexm2sdr_dev_cc", {}, {{"RX", DSP_SAMPLE_TYPE_CF32}})
        {
            outputs[0].fifo = std::make_shared<DSPStream>(16); // TODOREWORK
        }

        LiteXM2SDRDevBlock::~LiteXM2SDRDevBlock()
        {
            stop(); // TODOREWORK (make sure to close device)
        }

        void LiteXM2SDRDevBlock::set_frequency()
        {
            if (is_open)
            {
                if (rx_ch_number > 0)
                {
                    m2sdr_set_rx_frequency(litexm2sdr_dev_obj, rx_frequency);
                    logger->debug("Set LiteXM2SDR RX frequency to %f", rx_frequency);
                }

                if (tx_ch_number > 0)
                {
                    m2sdr_set_tx_frequency(litexm2sdr_dev_obj, tx_frequency);
                    logger->debug("Set LiteXM2SDR TX frequency to %f", tx_frequency);
                }
            }
        }

        void LiteXM2SDRDevBlock::set_gains()
        {
            if (is_open)
            {
                for (int i = 0; i < 2; i++)
                {
                    if (rx_ch_number > 0)
                    {
                        if (rx_ch_number == 2 || rx_ch_id == i)
                        {
                            m2sdr_set_rx_gain_chan(litexm2sdr_dev_obj, i, rx_gain[i]);
                            logger->debug("Set LiteXM2SDR RX%d gain to %d", i + 1, rx_gain[i]);
                        }
                    }

                    if (tx_ch_number > 0)
                    {
                        if (tx_ch_number == 2 || tx_ch_id == i)
                        {
                            m2sdr_set_tx_att(litexm2sdr_dev_obj, tx_gain[i]);
                            logger->debug("Set LiteXM2SDR TX%d att to %d", i + 1, tx_gain[i]);
                        }
                    }
                }
            }
        }

        void LiteXM2SDRDevBlock::set_bw()
        {
            if (is_open)
            {
                m2sdr_set_bandwidth(litexm2sdr_dev_obj, manual_bw ? bandwidth : samplerate);
                logger->debug("Set LiteXM2SDR Bandwidth to %f", manual_bw ? bandwidth : samplerate);
            }
        }

        void LiteXM2SDRDevBlock::init()
        {
            set_frequency();
            set_gains();
        }

        void LiteXM2SDRDevBlock::start()
        {
            if (m2sdr_open(&litexm2sdr_dev_obj, dev_serial.c_str()) != 0)
                throw satdump_exception("Could not open LiteXM2SDR '" + std::string(dev_serial) + "' device!");
            is_open = true;

            is_dual_ch = rx_ch_number > 1 || tx_ch_number > 1;

            struct m2sdr_config cfg;
            m2sdr_config_init(&cfg);
            cfg.channel_layout = is_dual_ch ? M2SDR_CHANNEL_LAYOUT_2T2R : M2SDR_CHANNEL_LAYOUT_1T1R;
            cfg.sample_rate = samplerate;
            cfg.bandwidth = samplerate;
            cfg.clock_source = extclock ? M2SDR_CLOCK_SOURCE_EXTERNAL : M2SDR_CLOCK_SOURCE_INTERNAL;
            m2sdr_apply_config(litexm2sdr_dev_obj, &cfg);

            logger->debug("Set LiteXM2SDR Samplerate to %f", samplerate);
            m2sdr_set_sample_rate(litexm2sdr_dev_obj, samplerate);
            m2sdr_set_bandwidth(litexm2sdr_dev_obj, samplerate);

            init();

            samples_per_buf = m2sdr_bytes_to_samples(is_8bit ? M2SDR_FORMAT_SC8_Q7 : M2SDR_FORMAT_SC16_Q11, M2SDR_BUFFER_BYTES);

            if (rx_ch_number > 0)
            {
                int r = m2sdr_sync_config(litexm2sdr_dev_obj, M2SDR_RX, is_8bit ? M2SDR_FORMAT_SC8_Q7 : M2SDR_FORMAT_SC16_Q11, 0, samples_per_buf, 0, 4000);

                if (r != 0)
                    logger->error("Error setting RX stream config " + std::to_string(r));
            }
            if (tx_ch_number > 0)
            {
                int r = m2sdr_sync_config(litexm2sdr_dev_obj, M2SDR_TX, is_8bit ? M2SDR_FORMAT_SC8_Q7 : M2SDR_FORMAT_SC16_Q11, 0, samples_per_buf, 0, 4000);

                if (r != 0)
                    logger->error("Error setting TX stream config " + std::to_string(r));
            }

            if (rx_ch_number > 0)
            {
                thread_should_run_rx = true;
                work_thread_rx = std::thread(&LiteXM2SDRDevBlock::mainThread_rx, this);
            }
            if (tx_ch_number > 0)
            {
                work_thread_tx = std::thread(&LiteXM2SDRDevBlock::mainThread_tx, this);
            }
            is_started = true;
        }

        void LiteXM2SDRDevBlock::stop(bool stop_now, bool force)
        {
            if (is_started) // TODOREWORK Split wait & stop?
            {
                if (rx_ch_number > 0)
                {
                    if (stop_now)
                        thread_should_run_rx = false;
                    logger->info("Waiting for the RX thread...");
                    if (work_thread_rx.joinable())
                        work_thread_rx.join();
                    logger->info("RX Thread stopped");
                    for (int i = 0; i < rx_ch_number; i++)
                        outputs[i].fifo->wait_enqueue(outputs[i].fifo->newBufferTerminator());
                }

                if (tx_ch_number > 0)
                {
                    logger->info("Waiting for the TX thread...");
                    if (work_thread_tx.joinable())
                        work_thread_tx.join();
                    logger->info("TX Thread stopped");
                }

                m2sdr_close(litexm2sdr_dev_obj);

                is_started = false;
                is_open = false;
            }
        }

        std::vector<DeviceInfo> LiteXM2SDRDevBlock::listDevs()
        {
            std::vector<DeviceInfo> r;

            size_t devs_cnt = 0;
            struct m2sdr_devinfo devs_list[100];
            m2sdr_get_device_list(devs_list, 100, &devs_cnt);

            struct m2sdr_dev *m2sdr_dev_obj;
            for (int i = 0; i < devs_cnt; i++)
            {
                nlohmann::ordered_json c;

                // Open device
                if (m2sdr_open(&m2sdr_dev_obj, devs_list[i].path) != 0)
                {
                    logger->warn("Could not open LiteXM2SDR for probing!");
                    continue;
                }

                std::string name = "LiteXM2SDR " + std::string(devs_list[i].serial) + " (" + std::string(devs_list[i].path) + ")";

                nlohmann::json p;
                p["serial"] = std::string(devs_list[i].path);
                r.push_back({"litexm2sdr", name, p, c});

                m2sdr_close(m2sdr_dev_obj);
            }

            return r;
        }
    } // namespace ndsp
} // namespace satdump