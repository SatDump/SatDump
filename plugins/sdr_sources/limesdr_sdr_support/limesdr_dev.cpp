#include "limesdr_dev.h"
#include "common/dsp/complex.h"
#include "core/exception.h"
#include "dsp/block.h"
#include "nlohmann/json.hpp"
#include <lime/ConnectionHandle.h>
#include <lime/ConnectionRegistry.h>
#include <lime/Streamer.h>
#include <lime/lms7_device.h>
#include <logger.h>
#include <memory>
#include <string>

namespace satdump
{
    namespace ndsp
    {
        LimeSDRDevBlock::LimeSDRDevBlock() : DeviceBlock("limesdr_dev_cc", {}, {{"RX", DSP_SAMPLE_TYPE_CF32}})
        {
            outputs[0].fifo = std::make_shared<DSPStream>(16); // TODOREWORK
        }

        LimeSDRDevBlock::~LimeSDRDevBlock() { stop(); }

        void LimeSDRDevBlock::set_frequency()
        {
            if (is_open)
            {
                // In current Lime devices, the RX and TX freq is the same on all channels
                if (rx_ch_number > 0)
                {
                    limesdr_dev_obj->SetFrequency(false, 0, rx_frequency);
                    double actual = limesdr_dev_obj->GetFrequency(false, 0);
                    logger->debug("Set LimeSDR RX frequency to %f, actual %f", rx_frequency, (double)actual);
                }

                if (tx_ch_number > 0)
                {
                    limesdr_dev_obj->SetFrequency(true, 0, tx_frequency);
                    double actual = limesdr_dev_obj->GetFrequency(true, 0);
                    logger->debug("Set LimeSDR TX frequency to %f, actual %f", tx_frequency, (double)actual);
                }
            }
        }

        void LimeSDRDevBlock::set_gains()
        {
            if (is_open)
            {
                for (int i = 0; i < 2; i++)
                {
                    for (int chn = 0; chn < rx_channel_cfgs.size(); chn++)
                    {
                        if (rx_channel_cfgs[chn].gain_mode_manual)
                        {
                            limesdr_dev_obj->SetGain(false, rx_ch_number == 1 ? rx_ch_id : chn, rx_channel_cfgs[chn].gain_lna, "LNA");
                            limesdr_dev_obj->SetGain(false, rx_ch_number == 1 ? rx_ch_id : chn, rx_channel_cfgs[chn].gain_tia, "TIA");
                            limesdr_dev_obj->SetGain(false, rx_ch_number == 1 ? rx_ch_id : chn, rx_channel_cfgs[chn].gain_pga, "PGA");
                            logger->debug("Set LimeSDR RX%d (LNA) Gain to %d", rx_ch_number == 1 ? rx_ch_id : chn, rx_channel_cfgs[chn].gain_lna);
                            logger->debug("Set LimeSDR RX%d (TIA) Gain to %d", rx_ch_number == 1 ? rx_ch_id : chn, rx_channel_cfgs[chn].gain_tia);
                            logger->debug("Set LimeSDR RX%d (PGA) Gain to %d", rx_ch_number == 1 ? rx_ch_id : chn, rx_channel_cfgs[chn].gain_pga);
                        }
                        else
                        {
                            limesdr_dev_obj->SetGain(false, rx_ch_number == 1 ? rx_ch_id : chn, rx_channel_cfgs[chn].gain, "");
                            logger->debug("Set LimeSDR RX%d (auto) Gain to %d", rx_ch_number == 1 ? rx_ch_id : chn, rx_channel_cfgs[chn].gain);
                        }
                    }

                    for (int chn = 0; chn < tx_channel_cfgs.size(); chn++)
                    {
                        limesdr_dev_obj->SetGain(true, rx_ch_number == 1 ? rx_ch_id : chn, tx_channel_cfgs[chn].gain, "");
                        logger->debug("Set LimeSDR TX%d (auto) Gain to %d", rx_ch_number == 1 ? rx_ch_id : chn, tx_channel_cfgs[chn].gain);
                    }
                }
            }
        }

        void LimeSDRDevBlock::set_paths()
        {
            if (is_open)
            {
                // Set path. Auto by default (end of the list)
                auto rx_paths = limesdr_dev_obj->GetPathNames(false);
                for (int ch = 0; ch < rx_ch_number; ch++)
                {
                    limesdr_dev_obj->SetPath(false, ch, rx_paths.size() - 1);
                    for (int p = 0; p < rx_paths.size(); p++)
                        if (rx_paths[p] == rx_channel_cfgs[ch].path)
                            limesdr_dev_obj->SetPath(false, ch, p);
                    logger->info("Set LimeSDR RX%d path to %s", ch, rx_paths[limesdr_dev_obj->GetPath(false, ch)].c_str());
                }

                // Set path. Auto by default (end of the list)
                auto tx_paths = limesdr_dev_obj->GetPathNames(true);
                for (int ch = 0; ch < tx_ch_number; ch++)
                {
                    limesdr_dev_obj->SetPath(true, ch, tx_paths.size() - 1);
                    for (int p = 0; p < tx_paths.size(); p++)
                        if (tx_paths[p] == tx_channel_cfgs[ch].path)
                            limesdr_dev_obj->SetPath(true, ch, p);
                    logger->info("Set LimeSDR TX%d path to %s", ch, rx_paths[limesdr_dev_obj->GetPath(true, ch)].c_str());
                }
            }
        }

        void LimeSDRDevBlock::set_bw()
        {
            if (is_open)
            {
                for (int ch = 0; ch < rx_ch_number; ch++)
                {
                    limesdr_dev_obj->SetLPF(false, ch, true, manual_bw ? bandwidth : samplerate);
                    logger->info("Set LimeSDR RX%d BW to %f", ch, manual_bw ? bandwidth : samplerate);
                }
                for (int ch = 0; ch < tx_ch_number; ch++)
                {
                    limesdr_dev_obj->SetLPF(true, ch, true, manual_bw ? bandwidth : samplerate);
                    logger->info("Set LimeSDR TX%d BW to %f", ch, manual_bw ? bandwidth : samplerate);
                }
            }
        }

        void LimeSDRDevBlock::init()
        {
            set_frequency();
            set_gains();
            set_paths();
            set_bw();
        }

        void LimeSDRDevBlock::start()
        {
            // Open device
            lime::ConnectionHandle handle;
            handle.serial = dev_serial;
            limesdr_dev_obj.reset();
            limesdr_dev_obj = std::shared_ptr<lime::LMS7_Device>(lime::LMS7_Device::CreateDevice(handle), [](auto *i) { delete i; });

            if (!limesdr_dev_obj)
            {
                logger->error("Could not open LimeSDR '" + std::string(dev_serial) + "' device! ");
                return;
            }
            is_open = true;

            // Init
            limesdr_dev_obj->Init();

            // Set samplerate, no oversample
            if (rx_ch_number > 0)
                limesdr_dev_obj->SetRate(false, samplerate, 0);
            if (tx_ch_number > 0)
                limesdr_dev_obj->SetRate(true, samplerate, 0);

            // Enable channels
            for (int ch = 0; ch < rx_ch_number; ch++)
                limesdr_dev_obj->EnableChannel(false, ch, true);
            for (int ch = 0; ch < tx_ch_number; ch++)
                limesdr_dev_obj->EnableChannel(true, ch, true);

            // Setup & start streams
            rx_streams.clear();
            for (int ch = 0; ch < rx_ch_number; ch++)
            {
                lime::StreamConfig stream_cfg;
                stream_cfg.isTx = false;
                stream_cfg.performanceLatency = 0.5;
                stream_cfg.bufferLength = 8192 * 10; // auto
                stream_cfg.channelID = rx_ch_number == 1 ? rx_ch_id : ch;
                stream_cfg.format = lime::StreamConfig::FMT_FLOAT32;
                auto nch = limesdr_dev_obj->SetupStream(stream_cfg);
                if (!nch)
                    throw satdump_exception("Error setting up stream!");
                rx_streams.push_back(nch);
            }

            tx_streams.clear();
            for (int ch = 0; ch < tx_ch_number; ch++)
            {
                lime::StreamConfig stream_cfg;
                stream_cfg.isTx = true;
                stream_cfg.performanceLatency = 0.5;
                stream_cfg.bufferLength = 8192 * 10; // auto
                stream_cfg.channelID = tx_ch_number == 1 ? tx_ch_id : ch;
                stream_cfg.format = lime::StreamConfig::FMT_FLOAT32;
                auto nch = limesdr_dev_obj->SetupStream(stream_cfg);
                if (!nch)
                    throw satdump_exception("Error setting up stream!");
                tx_streams.push_back(nch);
            }

            // Final settings
            init();

            // Start streams
            for (auto &s : rx_streams)
                if (s->Start() != 0)
                    throw satdump_exception("Error starting RX stream!");

            for (auto &s : tx_streams)
                if (s->Start() != 0)
                    throw satdump_exception("Error starting TX stream!");

            // Start threads
            if (rx_ch_number > 0)
            {
                thread_should_run_rx = true;
                work_thread_rx = std::thread(&LimeSDRDevBlock::mainThread_rx, this);
            }
            if (tx_ch_number > 0)
            {
                work_thread_tx = std::thread(&LimeSDRDevBlock::mainThread_tx, this);
            }
            is_started = true;

            // LimeSDR Mini 2.0 Fix
            set_bw();
        }

        void LimeSDRDevBlock::stop(bool stop_now, bool force)
        {
            if (is_started)
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

                for (auto &s : rx_streams)
                {
                    if (s->Stop() != 0)
                        logger->error("Error stopping stream!");
                    limesdr_dev_obj->DestroyStream(s);
                }

                for (auto &s : tx_streams)
                {
                    if (s->Stop() != 0)
                        logger->error("Error stopping stream!");
                    limesdr_dev_obj->DestroyStream(s);
                }

                for (int ch = 0; ch < rx_ch_number; ch++)
                    limesdr_dev_obj->EnableChannel(false, ch, false);
                for (int ch = 0; ch < tx_ch_number; ch++)
                    limesdr_dev_obj->EnableChannel(true, ch, false);

                limesdr_dev_obj.reset();

                is_started = false;
                is_open = false;
            }
        }

        std::vector<DeviceInfo> LimeSDRDevBlock::listDevs()
        {
            std::vector<DeviceInfo> r;

            auto conns = lime::ConnectionRegistry::findConnections();

            for (auto &dev : conns)
            {
                nlohmann::ordered_json c;

                lime::LMS7_Device *op_dev = lime::LMS7_Device::CreateDevice(dev);

                if (op_dev)
                {
                    // Get samplerates
                    auto samplerates_r = op_dev->GetRateRange();

                    std::vector<double> samplerates;
                    samplerates.push_back(samplerates_r.min);
                    for (double s = samplerates_r.min; s < samplerates_r.max; s += 1e6)
                    {
                        double sv = s - fmod(s, 1e6);
                        if (sv > 0)
                            samplerates.push_back(sv);
                    }
                    samplerates.push_back(samplerates_r.max);

                    c["samplerate"]["type"] = "samplerate";
                    c["samplerate"]["name"] = "Samplerate";
                    c["samplerate"]["list"] = samplerates;
                    c["samplerate"]["allow_manual"] = true;

                    // Also cover bandwidth
                    c["bandwidth"]["type"] = "samplerate";
                    c["bandwidth"]["name"] = "Bandwidth";
                    c["bandwidth"]["list"] = samplerates;
                    c["bandwidth"]["allow_manual"] = true;

                    // Get channels paths
                    for (int chn = 0; chn < op_dev->GetNumChannels(false); chn++)
                        add_param_list(c, "rx" + std::to_string(chn + 1) + "_path", "string", op_dev->GetPathNames(false), "RX" + std::to_string(chn + 1) + " Path");
                    for (int chn = 0; chn < op_dev->GetNumChannels(true); chn++)
                        add_param_list(c, "tx" + std::to_string(chn + 1) + "_path", "string", op_dev->GetPathNames(false), "TX" + std::to_string(chn + 1) + " Path");

                    delete op_dev;
                }
                else
                {
                    logger->error("Error opening LimeSDR device!");
                }

                std::string name = dev.name + " " + dev.serial;

                nlohmann::json p;
                p["serial"] = std::string(dev.serial);
                r.push_back({"limesdr", name, p, c});
            }

            return r;
        }
    } // namespace ndsp
} // namespace satdump