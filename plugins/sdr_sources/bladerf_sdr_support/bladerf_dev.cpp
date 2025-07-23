#include "bladerf_dev.h"
#include "common/dsp/complex.h"
#include "dsp/block.h"
#include "nlohmann/json.hpp"
#include <bladeRF2.h>
#include <libbladeRF.h>
#include <logger.h>
#include <string>

namespace satdump
{
    namespace ndsp
    {
        BladeRFDevBlock::BladeRFDevBlock() : DeviceBlock("bladerf_dev_cc", {}, {{"RX", DSP_SAMPLE_TYPE_CF32}})
        {
            outputs[0].fifo = std::make_shared<DspBufferFifo>(16); // TODOREWORK
        }

        BladeRFDevBlock::~BladeRFDevBlock()
        {
            stop(); // TODOREWORK (make sure to close device)
        }

        void BladeRFDevBlock::set_frequency()
        {
            if (is_open)
            {
                if (rx_ch_number > 0)
                {
                    bladerf_frequency actual;
                    bladerf_set_frequency(bladerf_dev_obj, BLADERF_CHANNEL_RX(0), rx_frequency);
                    bladerf_get_frequency(bladerf_dev_obj, BLADERF_CHANNEL_RX(0), &actual);
                    logger->debug("Set BladeRF RX frequency to %f, actual %f", rx_frequency, (double)actual);
                }

                if (tx_ch_number > 0)
                {
                    bladerf_frequency actual;
                    bladerf_set_frequency(bladerf_dev_obj, BLADERF_CHANNEL_TX(0), tx_frequency);
                    bladerf_get_frequency(bladerf_dev_obj, BLADERF_CHANNEL_TX(0), &actual);
                    logger->debug("Set BladeRF TX frequency to %f, actual %f", tx_frequency, (double)actual);
                }
            }
        }

        void BladeRFDevBlock::set_gains()
        {
            if (is_open)
            {
                for (int i = 0; i < 2; i++)
                {
                    if (rx_ch_number > 0)
                    {
                        if (rx_ch_number == 2 || rx_ch_id == i)
                        {
                            bladerf_set_gain_mode(bladerf_dev_obj, BLADERF_CHANNEL_RX(i), BLADERF_GAIN_MANUAL);
                            bladerf_set_gain(bladerf_dev_obj, BLADERF_CHANNEL_RX(i), rx_gain[i]);
                            logger->debug("Set BladeRF RX%d gain to %d", i + 1, rx_gain[i]);
                        }
                    }

                    if (tx_ch_number > 0)
                    {
                        if (tx_ch_number == 2 || tx_ch_id == i)
                        {
                            bladerf_set_gain(bladerf_dev_obj, BLADERF_CHANNEL_TX(i), tx_gain[i]);
                            logger->debug("Set BladeRF TX%d gain to %d", i + 1, tx_gain[i]);
                        }
                    }
                }
            }
        }

        void BladeRFDevBlock::set_others()
        {
            if (is_open)
            {
                // if (bladerf_model == 2) // TODOREWORK?
                {
                    bladerf_set_pll_enable(bladerf_dev_obj, extclock);
                    logger->debug("Set BladeRF External Clock to %d", (int)extclock);
                }

                if (rx_ch_number > 0)
                {
                    bladerf_set_bias_tee(bladerf_dev_obj, BLADERF_CHANNEL_RX(0), rx_bias);
                    logger->debug("Set BladeRF RX bias to %d", rx_bias);
                }
                if (tx_ch_number > 0)
                {
                    bladerf_set_bias_tee(bladerf_dev_obj, BLADERF_CHANNEL_TX(0), tx_bias);
                    logger->debug("Set BladeRF TX bias to %d", rx_bias);
                }
            }
        }

        void BladeRFDevBlock::init()
        {
            set_frequency();
            set_gains();
            set_others();
        }

        void BladeRFDevBlock::start()
        {
            struct bladerf_devinfo info;
            bladerf_init_devinfo(&info);
            if (dev_serial.size() >= BLADERF_SERIAL_LENGTH - 1)
            {
                strncpy(info.serial, dev_serial.c_str(), BLADERF_SERIAL_LENGTH - 1);
                info.serial[BLADERF_SERIAL_LENGTH - 1] = '\0';
            }
            info.serial[BLADERF_SERIAL_LENGTH - 1] = '\0';

            int r = 0;
            if (r = bladerf_open_with_devinfo(&bladerf_dev_obj, &info))
                throw satdump_exception("Could not open BladeRF '" + std::string(info.serial) + "' device! " + std::string(bladerf_strerror(r)));
            is_open = true;

            // TODOREWORK all channels
            bladerf_set_sample_rate(bladerf_dev_obj, BLADERF_CHANNEL_RX(0), samplerate, NULL);
            logger->info("Actual samplerate %f (precise)", samplerate);
            bladerf_set_bandwidth(bladerf_dev_obj, BLADERF_CHANNEL_RX(0), samplerate, NULL);

            sample_buffer_size = samplerate / 250;
            sample_buffer_size = (sample_buffer_size / 1024) * 1024;
            if (sample_buffer_size < 1024)
                sample_buffer_size = 1024;
            logger->trace("BladeRF Buffer size %d", sample_buffer_size);

            init();

            if (rx_ch_number > 0)
            {
                bladerf_sync_config(bladerf_dev_obj, rx_ch_number == 2 ? BLADERF_RX_X2 : BLADERF_RX_X1, BLADERF_FORMAT_SC16_Q11, 16, sample_buffer_size, 8, 4000);
                if (rx_ch_number == 2 || rx_ch_id == 0)
                    bladerf_enable_module(bladerf_dev_obj, BLADERF_CHANNEL_RX(0), true);
                if (rx_ch_number == 2 || rx_ch_id == 1)
                    bladerf_enable_module(bladerf_dev_obj, BLADERF_CHANNEL_RX(1), true);
            }
            if (tx_ch_number > 0)
            {
                bladerf_sync_config(bladerf_dev_obj, tx_ch_number == 2 ? BLADERF_TX_X2 : BLADERF_TX_X1, BLADERF_FORMAT_SC16_Q11, 16, sample_buffer_size, 8, 4000);
                if (tx_ch_number == 2 || tx_ch_id == 0)
                    bladerf_enable_module(bladerf_dev_obj, BLADERF_CHANNEL_TX(0), true);
                if (tx_ch_number == 2 || tx_ch_id == 1)
                    bladerf_enable_module(bladerf_dev_obj, BLADERF_CHANNEL_TX(1), true);
            }

            if (rx_ch_number > 0)
            {
                thread_should_run_rx = true;
                work_thread_rx = std::thread(&BladeRFDevBlock::mainThread_rx, this);
            }
            if (tx_ch_number > 0)
            {
                work_thread_tx = std::thread(&BladeRFDevBlock::mainThread_tx, this);
            }
            is_started = true;
        }

        void BladeRFDevBlock::stop(bool stop_now)
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
                        outputs[i].fifo->wait_enqueue(DSPBuffer::newBufferTerminator());
                }

                if (tx_ch_number > 0)
                {
                    logger->info("Waiting for the TX thread...");
                    if (work_thread_tx.joinable())
                        work_thread_tx.join();
                    logger->info("TX Thread stopped");
                }

                bladerf_set_bias_tee(bladerf_dev_obj, BLADERF_CHANNEL_RX(0), false);
                bladerf_set_bias_tee(bladerf_dev_obj, BLADERF_CHANNEL_TX(0), false);

                if (rx_ch_number > 0)
                {
                    if (rx_ch_number == 2 || rx_ch_id == 0)
                        bladerf_enable_module(bladerf_dev_obj, BLADERF_CHANNEL_RX(0), false);
                    if (rx_ch_number == 2 || rx_ch_id == 1)
                        bladerf_enable_module(bladerf_dev_obj, BLADERF_CHANNEL_RX(1), false);
                }
                if (tx_ch_number > 0)
                {
                    if (tx_ch_number == 2 || tx_ch_id == 0)
                        bladerf_enable_module(bladerf_dev_obj, BLADERF_CHANNEL_TX(0), false);
                    if (tx_ch_number == 2 || tx_ch_id == 1)
                        bladerf_enable_module(bladerf_dev_obj, BLADERF_CHANNEL_TX(1), false);
                }

                bladerf_close(bladerf_dev_obj);

                is_started = false;
                is_open = false;
            }
        }

        std::vector<DeviceInfo> BladeRFDevBlock::listDevs()
        {
            std::vector<DeviceInfo> r;

            bladerf_devinfo *devs_list = nullptr;
            int devs_cnt = bladerf_get_device_list(&devs_list);

            bladerf *bladerf_dev_obj;
            for (int i = 0; i < devs_cnt; i++)
            {
                nlohmann::ordered_json c;

                // Open device
                if (bladerf_open_with_devinfo(&bladerf_dev_obj, &devs_list[i]) != 0)
                {
                    logger->warn("Could not open BladeRF for probing!");
                    continue;
                }

                // Get channel count
                int rx_channel_cnt = bladerf_get_channel_count(bladerf_dev_obj, BLADERF_RX);
                int tx_channel_cnt = bladerf_get_channel_count(bladerf_dev_obj, BLADERF_TX);

                // Get the board version
                int bladerf_model;
                const char *bladerf_name = bladerf_get_board_name(bladerf_dev_obj);
                if (std::string(bladerf_name) == "bladerf1")
                    bladerf_model = 1;
                else if (std::string(bladerf_name) == "bladerf2")
                    bladerf_model = 2;
                else
                    bladerf_model = 0;

                // Get all ranges, per channel
                for (int tx_rx = 0; tx_rx < 2; tx_rx++)
                {
                    const bladerf_range *bladerf_range_samplerate;
                    const bladerf_range *bladerf_range_bandwidth;
                    const bladerf_range *bladerf_range_gain_rx;
                    const bladerf_range *bladerf_range_gain_tx;

                    bladerf_get_sample_rate_range(bladerf_dev_obj, BLADERF_CHANNEL_RX(0), &bladerf_range_samplerate);
                    bladerf_get_bandwidth_range(bladerf_dev_obj, BLADERF_CHANNEL_RX(0), &bladerf_range_bandwidth);
                    bladerf_get_gain_range(bladerf_dev_obj, BLADERF_CHANNEL_RX(0), &bladerf_range_gain_rx);
                    bladerf_get_gain_range(bladerf_dev_obj, BLADERF_CHANNEL_TX(0), &bladerf_range_gain_tx);

                    // Get available samplerates
                    std::vector<double> available_samplerates;
                    available_samplerates.push_back(bladerf_range_samplerate->min);
                    for (int i = 1e6; i < bladerf_range_samplerate->max; i += 1e6)
                        available_samplerates.push_back(i);
                    available_samplerates.push_back(bladerf_range_samplerate->max);

                    //                    add_param_list(c, "samplerate", "uint", available_samplerates, "Samplerate");
                    //                    add_param_range(c, "samplerate", "uint", bladerf_range_samplerate->min, bladerf_range_samplerate->max, bladerf_range_samplerate->step, "Samplerate");
                    //                    c["samplerate"]["range_noslider"] = true;
                    c["samplerate"]["type"] = "samplerate";
                    c["samplerate"]["name"] = "Samplerate";
                    c["samplerate"]["list"] = available_samplerates;
                    c["samplerate"]["allow_manual"] = true;

                    add_param_range(c, "rx1_gain", "int", bladerf_range_gain_rx->min, bladerf_range_gain_rx->max, bladerf_range_gain_rx->step, "RX1 Gain");
                    if (bladerf_model == 2)
                        add_param_range(c, "rx2_gain", "int", bladerf_range_gain_rx->min, bladerf_range_gain_rx->max, bladerf_range_gain_rx->step, "RX2 Gain");
                    add_param_range(c, "tx1_gain", "float", bladerf_range_gain_tx->min * bladerf_range_gain_tx->scale, bladerf_range_gain_tx->max * bladerf_range_gain_tx->scale,
                                    bladerf_range_gain_tx->step * bladerf_range_gain_tx->scale, "TX1 Gain");
                    if (bladerf_model == 2)
                        add_param_range(c, "tx2_gain", "float", bladerf_range_gain_tx->min * bladerf_range_gain_tx->scale, bladerf_range_gain_tx->max * bladerf_range_gain_tx->scale,
                                        bladerf_range_gain_tx->step * bladerf_range_gain_tx->scale, "TX2 Gain");
                }

                std::string name = "BladeRF";
                if (bladerf_model == 1)
                    name += "1";
                else if (bladerf_model == 2)
                    name += "2";
                name += " " + std::string(devs_list[i].serial);

                nlohmann::json p;
                p["serial"] = std::string(devs_list[i].serial);
                r.push_back({"bladerf", name, p, c});

                bladerf_close(bladerf_dev_obj);
            }

            if (devs_list != nullptr && devs_cnt > 0)
                bladerf_free_device_list(devs_list);

            return r;
        }
    } // namespace ndsp
} // namespace satdump