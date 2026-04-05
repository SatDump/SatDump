#include "limesdr_dev.h"
#include "dsp/block.h"
#include "nlohmann/json.hpp"
#if LIMESUITENG
#include <limesuiteng/DeviceRegistry.h>
#include <limesuiteng/SDRDevice.h>
#include <limesuiteng/limesuiteng.hpp>
#include <limesuiteng/types.h>
#else
#ifdef __ANDROID__
#include "API/lms7_device.h"
#include "lime/LimeSuite.h"
#include "ConnectionRegistry/ConnectionRegistry.h"
#else
#include <lime/ConnectionHandle.h>
#include <lime/ConnectionRegistry.h>
#include <lime/lms7_device.h>
#endif
#endif
#include <lime/LimeSuite.h>
#include <logger.h>
#include <memory>
#include <string>

namespace satdump
{
    namespace ndsp
    {
        LimeSDRDevBlock::LimeSDRDevBlock() : DeviceBlock(LIMESUITENG ? "limesdrng_dev_cc" : "limesdr_dev_cc", {}, {{"RX", DSP_SAMPLE_TYPE_CF32}})
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
                    LMS_SetLOFrequency(limesdr_dev_obj, false, 0, rx_frequency);
                    double actual = 0;
                    LMS_GetLOFrequency(limesdr_dev_obj, false, 0, &actual);
                    logger->debug("Set LimeSDR RX frequency to %f, actual %f", rx_frequency, (double)actual);
                }

                if (tx_ch_number > 0)
                {
                    LMS_SetLOFrequency(limesdr_dev_obj, true, 0, tx_frequency);
                    double actual = 0;
                    LMS_GetLOFrequency(limesdr_dev_obj, true, 0, &actual);
                    logger->debug("Set LimeSDR TX frequency to %f, actual %f", tx_frequency, (double)actual);
                }
            }
        }

        void LimeSDRDevBlock::set_gains()
        {
            if (is_open)
            {
#if !LIMESUITENG
                lime::LMS7_Device *lms = (lime::LMS7_Device *)limesdr_dev_obj;
#endif

                for (int chn = 0; chn < rx_ch_number; chn++)
                {
#if !LIMESUITENG
                    if (rx_channel_cfgs[chn].gain_mode_manual)
                    {
                        lms->SetGain(false, rx_ch_number == 1 ? rx_ch_id : chn, rx_channel_cfgs[chn].gain_lna, "LNA");
                        lms->SetGain(false, rx_ch_number == 1 ? rx_ch_id : chn, rx_channel_cfgs[chn].gain_tia, "TIA");
                        lms->SetGain(false, rx_ch_number == 1 ? rx_ch_id : chn, rx_channel_cfgs[chn].gain_pga, "PGA");
                        logger->debug("Set LimeSDR RX%d (LNA) Gain to %d", rx_ch_number == 1 ? rx_ch_id : chn, rx_channel_cfgs[chn].gain_lna);
                        logger->debug("Set LimeSDR RX%d (TIA) Gain to %d", rx_ch_number == 1 ? rx_ch_id : chn, rx_channel_cfgs[chn].gain_tia);
                        logger->debug("Set LimeSDR RX%d (PGA) Gain to %d", rx_ch_number == 1 ? rx_ch_id : chn, rx_channel_cfgs[chn].gain_pga);
                    }
                    else
#endif
                    {
                        LMS_SetGaindB(limesdr_dev_obj, false, rx_ch_number == 1 ? rx_ch_id : chn, rx_channel_cfgs[chn].gain);
                        logger->debug("Set LimeSDR RX%d (auto) Gain to %d", rx_ch_number == 1 ? rx_ch_id : chn, rx_channel_cfgs[chn].gain);
                    }
                }

                for (int chn = 0; chn < tx_ch_number; chn++)
                {
                    LMS_SetGaindB(limesdr_dev_obj, true, tx_ch_number == 1 ? tx_ch_id : chn, tx_channel_cfgs[chn].gain);
                    logger->debug("Set LimeSDR TX%d (auto) Gain to %d", tx_ch_number == 1 ? tx_ch_id : chn, tx_channel_cfgs[chn].gain);
                }
            }
        }

        int antFromStr(std::string ant_str)
        {
            if (ant_str == "NONE")
                return LMS_PATH_NONE;
            else if (ant_str == "LNAH")
                return LMS_PATH_LNAH;
            else if (ant_str == "LNAL")
                return LMS_PATH_LNAL;
            else if (ant_str == "LNAW")
                return LMS_PATH_LNAW;
            else if (ant_str == "BAND1")
                return LMS_PATH_TX1;
            else if (ant_str == "BAND2")
                return LMS_PATH_TX2;
            else
                return LMS_PATH_AUTO;
        }

        void LimeSDRDevBlock::set_paths()
        {
            if (is_open)
            {
                // Set path. Auto by default (end of the list)
                for (int ch = 0; ch < rx_ch_number; ch++)
                {
                    int a = antFromStr(rx_channel_cfgs[ch].path);
                    LMS_SetAntenna(limesdr_dev_obj, false, ch, a);
                    logger->info("Set LimeSDR RX%d path to %d", ch, a);
                }

                // Set path. Auto by default (end of the list)
                for (int ch = 0; ch < tx_ch_number; ch++)
                {
                    int a = antFromStr(tx_channel_cfgs[ch].path);
                    LMS_SetAntenna(limesdr_dev_obj, true, ch, a);
                    logger->info("Set LimeSDR TX%d path to %d", ch, a);
                }
            }
        }

        void LimeSDRDevBlock::set_bw()
        {
            if (is_open)
            {
                for (int ch = 0; ch < rx_ch_number; ch++)
                {
                    LMS_SetLPFBW(limesdr_dev_obj, false, ch, manual_bw ? bandwidth : samplerate);
                    LMS_SetLPF(limesdr_dev_obj, false, ch, true);
                    logger->info("Set LimeSDR RX%d BW to %f", ch, manual_bw ? bandwidth : samplerate);
                }
                for (int ch = 0; ch < tx_ch_number; ch++)
                {
                    LMS_SetLPFBW(limesdr_dev_obj, true, ch, manual_bw ? bandwidth : samplerate);
                    LMS_SetLPF(limesdr_dev_obj, true, ch, true);
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
            limesdr_dev_obj = NULL;
            LMS_Open(&limesdr_dev_obj, dev_serial == "" ? NULL : dev_serial.c_str(), NULL);
            int err = LMS_Init(limesdr_dev_obj);

            // LimeSuite Bug
            if (err)
            {
                LMS_Close(limesdr_dev_obj);
                LMS_Open(&limesdr_dev_obj, dev_serial == "" ? NULL : dev_serial.c_str(), NULL);
                err = LMS_Init(limesdr_dev_obj);
            }

            if (!limesdr_dev_obj || err)
            {
                logger->error("Could not open LimeSDR '" + std::string(dev_serial) + "' device! ");
                LMS_Close(limesdr_dev_obj);
                return;
            }
            is_open = true;

            // Init
            LMS_Init(limesdr_dev_obj);

            // Set samplerate, no oversample
            LMS_SetSampleRate(limesdr_dev_obj, samplerate, 0);

            // Setup & start streams
            for (int ch = 0; ch < rx_ch_number; ch++)
            {
                LMS_EnableChannel(limesdr_dev_obj, false, ch, true);
                LMS_Calibrate(limesdr_dev_obj, LMS_CH_RX, ch, samplerate, 0);

                lms_stream_t stream_cfg;
                stream_cfg.isTx = false;
                stream_cfg.throughputVsLatency = 0.5;
                stream_cfg.fifoSize = 8192 * 10; // auto
                stream_cfg.channel = rx_ch_number == 1 ? rx_ch_id : ch;
                stream_cfg.dataFmt = stream_cfg.LMS_FMT_F32;
                if (LMS_SetupStream(limesdr_dev_obj, &stream_cfg) == -1)
                    logger->error("Error setting up stream!");
                rx_streams[ch] = stream_cfg;
            }

            for (int ch = 0; ch < tx_ch_number; ch++)
            {
                LMS_EnableChannel(limesdr_dev_obj, true, ch, true);
                LMS_Calibrate(limesdr_dev_obj, LMS_CH_TX, ch, samplerate, 0);

                lms_stream_t stream_cfg;
                stream_cfg.isTx = true;
                stream_cfg.throughputVsLatency = 0.5;
                stream_cfg.fifoSize = 8192 * 10; // auto
                stream_cfg.channel = tx_ch_number == 1 ? tx_ch_id : ch;
                stream_cfg.dataFmt = stream_cfg.LMS_FMT_F32;
                if (LMS_SetupStream(limesdr_dev_obj, &stream_cfg) == -1)
                    logger->error("Error setting up stream!");
                tx_streams[ch] = stream_cfg;
            }

            // Final settings
            init();

            // Start streams
            for (int ch = 0; ch < rx_ch_number; ch++)
                if (LMS_StartStream(&rx_streams[ch]) == -1)
                    logger->error("Error starting RX stream!");

            for (int ch = 0; ch < tx_ch_number; ch++)
                if (LMS_StartStream(&tx_streams[ch]) == -1)
                    logger->error("Error starting TX stream!");

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

                for (int ch = 0; ch < rx_ch_number; ch++)
                {
                    if (LMS_StopStream(&rx_streams[ch]) == -1)
                        logger->error("Error stopping stream!");
                    LMS_DestroyStream(limesdr_dev_obj, &rx_streams[ch]);
                    LMS_EnableChannel(limesdr_dev_obj, false, ch, false);
                }

                for (int ch = 0; ch < tx_ch_number; ch++)
                {
                    if (LMS_StopStream(&tx_streams[ch]) == -1)
                        logger->error("Error stopping stream!");
                    LMS_DestroyStream(limesdr_dev_obj, &tx_streams[ch]);
                    LMS_EnableChannel(limesdr_dev_obj, true, ch, false);
                }

                LMS_Close(limesdr_dev_obj);

                is_started = false;
                is_open = false;
            }
        }

        std::vector<DeviceInfo> LimeSDRDevBlock::listDevs()
        {
            std::vector<DeviceInfo> r;

#if LIMESUITENG
            auto conns = lime::DeviceRegistry::enumerate();

            for (auto &dev : conns)
            {
                nlohmann::ordered_json c;

                lime::SDRDevice *op_dev = lime::DeviceRegistry::makeDevice(dev);

                if (op_dev)
                {
                    // Get samplerates
                    // auto samplerates_r = op_dev->Ra  GetRateRange();

                    std::vector<double> samplerates;
                    samplerates.push_back(1e6);
                    for (double s = 1e6; s < 150e6; s += 1e6)
                    {
                        double sv = s - fmod(s, 1e6);
                        if (sv > 0)
                            samplerates.push_back(sv);
                    }
                    samplerates.push_back(150e6);

                    c["samplerate"]["type"] = "samplerate";
                    c["samplerate"]["name"] = "Samplerate";
                    c["samplerate"]["list"] = samplerates;
                    c["samplerate"]["allow_manual"] = true;

                    // Also cover bandwidth
                    c["bandwidth"]["type"] = "samplerate";
                    c["bandwidth"]["name"] = "Bandwidth";
                    c["bandwidth"]["list"] = samplerates;
                    c["bandwidth"]["allow_manual"] = true;

                    delete op_dev;
                }
                else
                {
                    logger->error("Error opening LimeSDR device!");
                }

                std::string name = dev.name + " (NG) " + dev.serial;

                nlohmann::json p;
                p["serial"] = dev.Serialize(); // std::string(dev.serial);
                r.push_back({"limesdrng", name, p, c});
            }
#else
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

                    delete op_dev;
                }
                else
                {
                    logger->error("Error opening LimeSDR device!");
                }

                std::string name = dev.name + " " + dev.serial;

                nlohmann::json p;
                p["serial"] = dev.serialize(); // std::string(dev.serial);
                r.push_back({"limesdr", name, p, c});
            }
#endif

            return r;
        }
    } // namespace ndsp
} // namespace satdump