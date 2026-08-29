#include "spectran_v6_dev.h"
#include "common/dsp/complex.h"
#include "utils/string.h"
#include <logger.h>

namespace satdump
{
    namespace ndsp
    {
        std::wstring get_spectran_samplerate_str(uint64_t rate)
        {
            // if (rate == SPECTRAN_SAMPLERATE_46M)
            //     return L"46MHz";
            if (rate == SPECTRAN_SAMPLERATE_61_44M)
                return L"49MHz";
            else if (rate == SPECTRAN_SAMPLERATE_92M)
                return L"92MHz";
            else if (rate == SPECTRAN_SAMPLERATE_122M)
                return L"122MHz";
            else if (rate == SPECTRAN_SAMPLERATE_184M)
                return L"184MHz";
            else if (rate == SPECTRAN_SAMPLERATE_245M)
                return L"245MHz";
            else
                throw satdump_exception("Invalid samplerate!");
        }
        std::wstring get_spectran_usbcomp_str(std::string mode)
        {
            if (mode == "auto")
                return L"auto";
            else if (mode == "raw")
                return L"raw";
            else if (mode == "compressed")
                return L"compressed";
            else
                throw satdump_exception("Invalid USB compression!");
        }

        std::wstring get_spectran_agc_str(std::string mode)
        {
            if (mode == "manual")
                return L"manual";
            else if (mode == "peak")
                return L"peak";
            else if (mode == "power")
                return L"power";
            else
                throw satdump_exception("Invalid AGC mode!");
        }

        SpectranV6DevBlock::SpectranV6DevBlock() : DeviceBlock("spectran_v6_dev_cc", {}, {{"out", DSP_SAMPLE_TYPE_CF32}})
        {
            outputs[0].fifo = std::make_shared<DSPStream>(16); // TODOREWORK
        }

        SpectranV6DevBlock::~SpectranV6DevBlock()
        {
            stop(); // TODOREWORK (make sure to close device)
        }

        void SpectranV6DevBlock::set_frequency()
        {
            if (is_open)
            {
                // Set frequency
                if (rtsa_api->AARTSAAPI_ConfigFind(&aaronia_device, &root, &config, L"main/centerfreq") == AARTSAAPI_OK)
                    rtsa_api->AARTSAAPI_ConfigSetFloat(&aaronia_device, &config, p_frequency);
                logger->debug("Set Aaronia frequency to %d", p_frequency);
            }
        }

        void SpectranV6DevBlock::set_gains()
        {
            if (is_open)
            {
                if (rtsa_api->AARTSAAPI_ConfigFind(&aaronia_device, &root, &config, L"calibration/preamp") == AARTSAAPI_OK)
                {
                    if (d_enable_amp && d_enable_preamp)
                        rtsa_api->AARTSAAPI_ConfigSetString(&aaronia_device, &config, L"Both");
                    else if (d_enable_preamp)
                        rtsa_api->AARTSAAPI_ConfigSetString(&aaronia_device, &config, L"Preamp");
                    else if (d_enable_amp)
                        rtsa_api->AARTSAAPI_ConfigSetString(&aaronia_device, &config, L"Amp");
                    else
                        rtsa_api->AARTSAAPI_ConfigSetString(&aaronia_device, &config, L"None");
                }

                // Set the reference level of the receiver
                if (rtsa_api->AARTSAAPI_ConfigFind(&aaronia_device, &root, &config, L"main/reflevel") == AARTSAAPI_OK)
                    rtsa_api->AARTSAAPI_ConfigSetFloat(&aaronia_device, &config, d_level);
                logger->debug("Set Aaronia reflevel to %f", d_level);

                if (rtsa_api->AARTSAAPI_ConfigFind(&aaronia_device, &root, &config, L"device/gaincontrol") == AARTSAAPI_OK)
                    rtsa_api->AARTSAAPI_ConfigSetString(&aaronia_device, &config, get_spectran_agc_str(d_agc_mode).c_str());
                logger->debug("Set Aaronia AGC mode to %s", d_agc_mode.c_str());
            }
        }

        void SpectranV6DevBlock::set_others()
        {
            if (is_open)
            {
                if (rtsa_api->AARTSAAPI_ConfigFind(&aaronia_device, &root, &config, L"device/usbcompression") == AARTSAAPI_OK)
                    rtsa_api->AARTSAAPI_ConfigSetString(&aaronia_device, &config, get_spectran_usbcomp_str(d_usb_compression).c_str());
                logger->debug("Set Aaronia USB compression mode to %s", d_usb_compression.c_str());
            }
        }

        void SpectranV6DevBlock::init()
        {
            set_frequency();
            set_gains();
            set_others();
        }

        void SpectranV6DevBlock::start()
        {
            if (rtsa_api->AARTSAAPI_Open(&aaronia_handle) != AARTSAAPI_OK)
                throw satdump_exception("Could not open AARTSAAPI handle!");

            if (rtsa_api->AARTSAAPI_RescanDevices(&aaronia_handle, 2000) != AARTSAAPI_OK)
                throw satdump_exception("Could not scan for Aaronia Devices!");

            bool foundDevice = false;
            std::wstring device_type;

            if (d_is_eco)
            {
                for (uint64_t i = 0; !foundDevice && rtsa_api->AARTSAAPI_EnumDevice(&aaronia_handle, L"spectranv6eco", i, &aaronia_dinfo) == AARTSAAPI_OK; i++)
                {
                    device_type = L"spectranv6eco";
                    foundDevice = true;
                }
            }
            else
            {
                if (!foundDevice)
                {
                    for (uint64_t i = 0; !foundDevice && rtsa_api->AARTSAAPI_EnumDevice(&aaronia_handle, L"spectranv6", i, &aaronia_dinfo) == AARTSAAPI_OK; i++)
                    {
                        device_type = L"spectranv6";
                        foundDevice = true;
                    }
                }
            }

            // No ECO and no PLUS found
            if (!foundDevice)
                throw satdump_exception("Could not find any Aaronia device (Spectran V6 Eco nor Spectran V6 Plus)!");

            std::wstring devString = device_type + L"/raw";

            if (rtsa_api->AARTSAAPI_OpenDevice(&aaronia_handle, &aaronia_device, devString.c_str(), aaronia_dinfo.serialNumber) != AARTSAAPI_OK)
                throw satdump_exception("Could not open Aaronia Device!");

            is_open = true;

            if (rtsa_api->AARTSAAPI_ConfigRoot(&aaronia_device, &root) != AARTSAAPI_OK)
                throw satdump_exception("Could not get Aaronia ConfigRoot!");

            if (rtsa_api->AARTSAAPI_ConfigFind(&aaronia_device, &root, &config, L"device/receiverchannel") == AARTSAAPI_OK)
            {
                if (d_rx_channel == "Rx1")
                    rtsa_api->AARTSAAPI_ConfigSetString(&aaronia_device, &config, L"Rx1");
                else if (d_rx_channel == "Rx2")
                    rtsa_api->AARTSAAPI_ConfigSetString(&aaronia_device, &config, L"Rx2");
                else
                    logger->error("Invalid channel " + d_rx_channel);
            }

            if (rtsa_api->AARTSAAPI_ConfigFind(&aaronia_device, &root, &config, L"device/outputformat") == AARTSAAPI_OK)
                rtsa_api->AARTSAAPI_ConfigSetString(&aaronia_device, &config, L"iq");

            if (d_is_eco)
            {
                if (rtsa_api->AARTSAAPI_ConfigFind(&aaronia_device, &root, &config, L"device/receiverclock") == AARTSAAPI_OK)
                    rtsa_api->AARTSAAPI_ConfigSetString(&aaronia_device, &config, get_spectran_samplerate_str(SPECTRAN_SAMPLERATE_61_44M).c_str());
                logger->info("Set Spectran receiver clock to %s", satdump::ws2s(get_spectran_samplerate_str(SPECTRAN_SAMPLERATE_61_44M)).c_str());
            }
            else
            {
                if (rtsa_api->AARTSAAPI_ConfigFind(&aaronia_device, &root, &config, L"device/receiverclock") == AARTSAAPI_OK)
                    rtsa_api->AARTSAAPI_ConfigSetString(&aaronia_device, &config, get_spectran_samplerate_str(p_samplerate < SPECTRAN_SAMPLERATE_92M ? SPECTRAN_SAMPLERATE_92M : p_samplerate).c_str());
                logger->info("Set Spectran receiver clock to %s", satdump::ws2s(get_spectran_samplerate_str(p_samplerate < SPECTRAN_SAMPLERATE_92M ? SPECTRAN_SAMPLERATE_92M : p_samplerate)).c_str());
            }

            int current_decimation = 1;
            if (d_is_eco)
            {
                if (p_samplerate < SPECTRAN_SAMPLERATE_61_44M)
                {
                    int decim = 1;
                    while (decim <= 128)
                    {
                        uint64_t samprate = SPECTRAN_SAMPLERATE_61_44M / decim;
                        logger->info("%llu %llu", p_samplerate, samprate);
                        if (samprate == p_samplerate)
                        {
                            current_decimation = decim;
                            break;
                        }
                        decim *= 2;
                    }
                }
            }
            else
            {
                if (p_samplerate < SPECTRAN_SAMPLERATE_92M)
                {
                    int decim = 1;
                    while (decim <= 128)
                    {
                        uint64_t samprate = SPECTRAN_SAMPLERATE_92M / decim;
                        logger->info("%llu %llu", p_samplerate, samprate);
                        if (samprate == p_samplerate)
                        {
                            current_decimation = decim;
                            break;
                        }
                        decim *= 2;
                    }
                }
            }
            std::wstring decimation_str = L"Full";
            if (current_decimation <= 1)
                decimation_str = L"Full";
            else if (current_decimation == 2)
                decimation_str = L"1 / 2";
            else if (current_decimation == 4)
                decimation_str = L"1 / 4";
            else if (current_decimation == 8)
                decimation_str = L"1 / 8";
            else if (current_decimation == 16)
                decimation_str = L"1 / 16";
            else if (current_decimation == 32)
                decimation_str = L"1 / 32";
            else if (current_decimation == 64)
                decimation_str = L"1 / 64";
            else if (current_decimation == 128)
                decimation_str = L"1 / 128";
            else if (current_decimation == 256)
                decimation_str = L"1 / 256";
            else if (current_decimation == 512)
                decimation_str = L"1 / 512";
            if (rtsa_api->AARTSAAPI_ConfigFind(&aaronia_device, &root, &config, L"main/decimation") == AARTSAAPI_OK)
                rtsa_api->AARTSAAPI_ConfigSetString(&aaronia_device, &config, decimation_str.c_str());
            logger->info("Set Spectran decimation to %d", current_decimation);

            if (rtsa_api->AARTSAAPI_ConfigFind(&aaronia_device, &root, &config, L"calibration/rffilter") == AARTSAAPI_OK)
                rtsa_api->AARTSAAPI_ConfigSetString(&aaronia_device, &config, L"Auto Extended");

            init();

            if (rtsa_api->AARTSAAPI_ConnectDevice(&aaronia_device) != AARTSAAPI_OK)
                throw satdump_exception("Could not connect to Aaronia device!");

            if (rtsa_api->AARTSAAPI_StartDevice(&aaronia_device) != AARTSAAPI_OK)
                throw satdump_exception("Could not start Aaronia device!");

            // Wait
            logger->info("Waiting for device to stream...");

            AARTSAAPI_Packet packet;
            while (rtsa_api->AARTSAAPI_GetPacket(&aaronia_device, 0, 0, &packet) == AARTSAAPI_EMPTY)
#ifdef _WIN32
                Sleep(1);
#else
                usleep(1000);
#endif
            logger->info("Started Aaronia Device!");

            thread_should_run = true;
            work_thread = std::thread(&SpectranV6DevBlock::mainThread, this);

            is_started = true;
        }

        void SpectranV6DevBlock::stop(bool stop_now, bool force)
        {
            if (stop_now && is_started) // TODOREWORK Split wait & stop?
            {
                thread_should_run = false;
                logger->info("Waiting for the thread...");
                if (work_thread.joinable())
                    work_thread.join();
                logger->info("Thread stopped");

                rtsa_api->AARTSAAPI_StopDevice(&aaronia_device);
                rtsa_api->AARTSAAPI_DisconnectDevice(&aaronia_device);
                rtsa_api->AARTSAAPI_CloseDevice(&aaronia_handle, &aaronia_device);
                rtsa_api->AARTSAAPI_Close(&aaronia_handle);

                is_started = false;
                is_open = false;
                outputs[0].fifo->wait_enqueue(outputs[0].fifo->newBufferTerminator());
            }
        }

        std::vector<DeviceInfo> SpectranV6DevBlock::listDevs()
        {
            std::vector<DeviceInfo> r;

            AARTSAAPI_Handle h;
            if (rtsa_api->AARTSAAPI_Open(&h) != AARTSAAPI_OK)
            {
                logger->error("Could not open AARTSAAPI handle");
                return r;
            }

            if (rtsa_api->AARTSAAPI_RescanDevices(&h, 2000) != AARTSAAPI_OK)
                logger->error("Could not scan for Aaronia Devices");

            AARTSAAPI_DeviceInfo dinfo;

            // Enumerate ECO first
            for (uint64_t i = 0; rtsa_api->AARTSAAPI_EnumDevice(&h, L"spectranv6eco", i, &dinfo) == AARTSAAPI_OK; i++)
            {
                std::stringstream ss;
                ss << std::hex << dinfo.serialNumber;

                nlohmann::json c;

                nlohmann::json p;
                p["serial"] = "eco_" + std::to_string(i);
                p["is_eco"] = true;
                r.push_back({"spectran_v6", "Spectran V6 ECO " + ss.str(), p, c});
            }

            // Then enumerate PLUS next
            for (uint64_t i = 0; rtsa_api->AARTSAAPI_EnumDevice(&h, L"spectranv6", i, &dinfo) == AARTSAAPI_OK; i++)
            {
                std::stringstream ss;
                ss << std::hex << dinfo.serialNumber;

                nlohmann::json c;

                nlohmann::json p;
                p["serial"] = "plus_" + std::to_string(i);
                p["is_eco"] = false;
                r.push_back({"spectran_v6", "Spectran V6 PLUS " + ss.str(), p, c});
            }

            rtsa_api->AARTSAAPI_Close(&h);

            return r;
        } // namespace ndsp
    } // namespace ndsp
} // namespace satdump