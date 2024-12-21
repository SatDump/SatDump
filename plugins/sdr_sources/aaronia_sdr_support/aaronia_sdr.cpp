#include "aaronia_sdr.h"
#include "common/utils.h"

#define SPECTRAN_SAMPLERATE_46M 46080000
#define SPECTRAN_SAMPLERATE_92M 92160000
#define SPECTRAN_SAMPLERATE_122M 122880000
#define SPECTRAN_SAMPLERATE_184M 184320000
#define SPECTRAN_SAMPLERATE_245M 245760000

std::wstring get_spectran_samplerate_str(uint64_t rate)
{
    if (rate == SPECTRAN_SAMPLERATE_46M)
        return L"46MHz";
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
std::wstring get_spectran_usbcomp_str(int mode)
{
    if (mode == 0)
        return L"auto";
    else if (mode == 1)
        return L"raw";
    else if (mode == 2)
        return L"compressed";
    else
        throw satdump_exception("Invalid USB compression!");
}

std::wstring get_spectran_agc_str(int mode)
{
    if (mode == 0)
        return L"manual";
    else if (mode == 1)
        return L"peak";
    else if (mode == 2)
        return L"power";
    else
        throw satdump_exception("Invalid AGC mode!");
}

void AaroniaSource::set_gains()
{
    if (!is_started)
        return;

    if (AARTSAAPI_ConfigFind(&aaronia_device, &root, &config, L"calibration/preamp") == AARTSAAPI_OK)
    {
        if (d_enable_amp && d_enable_preamp)
            AARTSAAPI_ConfigSetString(&aaronia_device, &config, L"Both");
        else if (d_enable_preamp)
            AARTSAAPI_ConfigSetString(&aaronia_device, &config, L"Preamp");
        else if (d_enable_amp)
            AARTSAAPI_ConfigSetString(&aaronia_device, &config, L"Amp");
        else
            AARTSAAPI_ConfigSetString(&aaronia_device, &config, L"None");

        if (d_enable_preamp)
            d_min_level = -38;
    }

    // Set the reference level of the receiver
    if (AARTSAAPI_ConfigFind(&aaronia_device, &root, &config, L"main/reflevel") == AARTSAAPI_OK)
        AARTSAAPI_ConfigSetFloat(&aaronia_device, &config, d_level);
    logger->debug("Set Aaronia reflevel to %f", d_level);

    if (AARTSAAPI_ConfigFind(&aaronia_device, &root, &config, L"device/gaincontrol") == AARTSAAPI_OK)
        AARTSAAPI_ConfigSetString(&aaronia_device, &config, get_spectran_agc_str(d_agc_mode).c_str());
    logger->debug("Set Aaronia AGC mode to %d", d_agc_mode);
}

void AaroniaSource::set_others()
{
    if (!is_started)
        return;

    if (AARTSAAPI_ConfigFind(&aaronia_device, &root, &config, L"device/usbcompression") == AARTSAAPI_OK)
        AARTSAAPI_ConfigSetString(&aaronia_device, &config, get_spectran_usbcomp_str(d_usb_compression).c_str());
    logger->debug("Set Aaronia USB compression mode to %d", d_usb_compression);
}

void AaroniaSource::set_settings(nlohmann::json settings)
{
    d_settings = settings;

    d_rx_channel = getValueOrDefault(d_settings["rx_channel"], d_rx_channel);
    d_level = getValueOrDefault(d_settings["ref_level"], d_level);
    d_usb_compression = getValueOrDefault(d_settings["usb_compression"], d_usb_compression);
    d_agc_mode = getValueOrDefault(d_settings["agc_mode"], d_agc_mode);
    d_enable_amp = getValueOrDefault(d_settings["enable_amp"], d_enable_amp);
    d_enable_preamp = getValueOrDefault(d_settings["enable_preamp"], d_enable_preamp);
    d_rescale = getValueOrDefault(d_settings["rescale"], d_rescale);

    if (is_started)
    {
        set_gains();
        set_others();
    }
}

nlohmann::json AaroniaSource::get_settings()
{
    d_settings["rx_channel"] = d_rx_channel;
    d_settings["ref_level"] = d_level;
    d_settings["usb_compression"] = d_level;
    d_settings["agc_mode"] = d_agc_mode;
    d_settings["enable_amp"] = d_enable_amp;
    d_settings["enable_preamp"] = d_enable_preamp;
    d_settings["rescale"] = d_rescale;

    return d_settings;
}

void AaroniaSource::open()
{
    // Get available samplerates
    available_samplerates.clear();
    available_samplerates.push_back(SPECTRAN_SAMPLERATE_92M / 128);
    available_samplerates.push_back(SPECTRAN_SAMPLERATE_92M / 64);
    available_samplerates.push_back(SPECTRAN_SAMPLERATE_92M / 32);
    available_samplerates.push_back(SPECTRAN_SAMPLERATE_92M / 16);
    available_samplerates.push_back(SPECTRAN_SAMPLERATE_92M / 8);
    available_samplerates.push_back(SPECTRAN_SAMPLERATE_92M / 4);
    available_samplerates.push_back(SPECTRAN_SAMPLERATE_92M / 2);
    available_samplerates.push_back(SPECTRAN_SAMPLERATE_92M);
    available_samplerates.push_back(SPECTRAN_SAMPLERATE_122M);
    available_samplerates.push_back(SPECTRAN_SAMPLERATE_184M);
    available_samplerates.push_back(SPECTRAN_SAMPLERATE_245M);

    // Init UI stuff
    samplerate_option_str = "";
    for (uint64_t samplerate : available_samplerates)
        samplerate_option_str += format_notated(samplerate, "sps") + '\0';

    is_open = true;
}

void AaroniaSource::start()
{
    DSPSampleSource::start();

    if (AARTSAAPI_Open(&aaronia_handle) != AARTSAAPI_OK)
        throw satdump_exception("Could not open AARTSAAPI handle!");

    if (AARTSAAPI_RescanDevices(&aaronia_handle, 2000) != AARTSAAPI_OK)
        throw satdump_exception("Could not scan for Aaronia Devices!");

    bool foundDevice = false;
    std::wstring device_type;

    // Try to find the ECO first
    for (uint64_t i = 0; !foundDevice && AARTSAAPI_EnumDevice(&aaronia_handle, L"spectranv6eco", i, &aaronia_dinfo) == AARTSAAPI_OK; i++)
    {
        device_type  = L"spectranv6eco";
        foundDevice  = true;
    }
    // If no ECO was found we try the PLUS next
    if (!foundDevice)
    {
        for (uint64_t i = 0; !foundDevice && AARTSAAPI_EnumDevice(&aaronia_handle, L"spectranv6", i, &aaronia_dinfo) == AARTSAAPI_OK; i++)
        {
            device_type = L"spectranv6";
            foundDevice = true;
        }
    }
    // No ECO and no PLUS found
    if (!foundDevice)
        throw satdump_exception("Could not find any Aaronia device (Spectran V6 Eco nor Spectran V6 Plus)!");

    std::wstring devString = device_type + L"/raw";

    if (AARTSAAPI_OpenDevice(&aaronia_handle, &aaronia_device, devString.c_str(), aaronia_dinfo.serialNumber) != AARTSAAPI_OK)
        throw satdump_exception("Could not open Aaronia Device!");

    is_started = true;

    if (AARTSAAPI_ConfigRoot(&aaronia_device, &root) != AARTSAAPI_OK)
        throw satdump_exception("Could not get Aaronia ConfigRoot!");

    if (AARTSAAPI_ConfigFind(&aaronia_device, &root, &config, L"device/receiverchannel") == AARTSAAPI_OK)
    {
        if (d_rx_channel == 0)
            AARTSAAPI_ConfigSetString(&aaronia_device, &config, L"Rx1");
        else if (d_rx_channel == 1)
            AARTSAAPI_ConfigSetString(&aaronia_device, &config, L"Rx2");
    }

    if (AARTSAAPI_ConfigFind(&aaronia_device, &root, &config, L"device/outputformat") == AARTSAAPI_OK)
        AARTSAAPI_ConfigSetString(&aaronia_device, &config, L"iq");

    if (AARTSAAPI_ConfigFind(&aaronia_device, &root, &config, L"device/receiverclock") == AARTSAAPI_OK)
        AARTSAAPI_ConfigSetString(&aaronia_device, &config, get_spectran_samplerate_str(current_samplerate < SPECTRAN_SAMPLERATE_46M ? SPECTRAN_SAMPLERATE_46M : current_samplerate).c_str());
    logger->info("Set Spectran receiver clock to %s", ws2s(get_spectran_samplerate_str(current_samplerate < SPECTRAN_SAMPLERATE_46M ? SPECTRAN_SAMPLERATE_46M : current_samplerate)).c_str());

    int current_decimation = 1;
    if (current_samplerate < SPECTRAN_SAMPLERATE_46M)
    {
        int decim = 1;
        while (decim <= 128)
        {
            uint64_t samprate = SPECTRAN_SAMPLERATE_46M / decim;
            logger->info("%llu %llu", current_samplerate, samprate);
            if (samprate == current_samplerate)
            {
                current_decimation = decim;
                break;
            }
            decim *= 2;
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
    if (AARTSAAPI_ConfigFind(&aaronia_device, &root, &config, L"main/decimation") == AARTSAAPI_OK)
        AARTSAAPI_ConfigSetString(&aaronia_device, &config, decimation_str.c_str());
    logger->info("Set Spectran decimation to %d", current_decimation);

    if (AARTSAAPI_ConfigFind(&aaronia_device, &root, &config, L"calibration/rffilter") == AARTSAAPI_OK)
        AARTSAAPI_ConfigSetString(&aaronia_device, &config, L"Auto Extended");

    set_frequency(d_frequency);
    set_others();
    set_gains();

    if (AARTSAAPI_ConnectDevice(&aaronia_device) != AARTSAAPI_OK)
        throw satdump_exception("Could not connect to Aaronia device!");

    if (AARTSAAPI_StartDevice(&aaronia_device) != AARTSAAPI_OK)
        throw satdump_exception("Could not start Aaronia device!");

    // Wait
    logger->info("Waiting for device to stream...");

    AARTSAAPI_Packet packet;
    while (AARTSAAPI_GetPacket(&aaronia_device, 0, 0, &packet) == AARTSAAPI_EMPTY)
#ifdef _WIN32
        Sleep(1);
#else
        usleep(1000);
#endif
    logger->info("Started Aaronia Device!");

    thread_should_run = true;
    work_thread = std::thread(&AaroniaSource::mainThread, this);
}

void AaroniaSource::stop()
{
    thread_should_run = false;
    logger->info("Waiting for the thread...");
    if (is_started)
        output_stream->stopWriter();
    if (work_thread.joinable())
        work_thread.join();
    logger->info("Thread stopped");
    if (is_started)
    {
        AARTSAAPI_StopDevice(&aaronia_device);
        AARTSAAPI_DisconnectDevice(&aaronia_device);
        AARTSAAPI_CloseDevice(&aaronia_handle, &aaronia_device);
        AARTSAAPI_Close(&aaronia_handle);
    }
    is_started = false;
}

void AaroniaSource::close()
{
}

void AaroniaSource::set_frequency(uint64_t frequency)
{
    if (is_started)
    {
        // Set frequency
        if (AARTSAAPI_ConfigFind(&aaronia_device, &root, &config, L"main/centerfreq") == AARTSAAPI_OK)
            AARTSAAPI_ConfigSetFloat(&aaronia_device, &config, frequency);
        logger->debug("Set Aaronia frequency to %d", frequency);
    }
    DSPSampleSource::set_frequency(frequency);
}

void AaroniaSource::drawControlUI()
{
    if (is_started)
        RImGui::beginDisabled();

    RImGui::Combo("Samplerate", &selected_samplerate, samplerate_option_str.c_str());
    current_samplerate = available_samplerates[selected_samplerate];

    // Channel
    if (RImGui::RadioButton("Rx1", d_rx_channel == 0))
        d_rx_channel = 0;
    else if (RImGui::RadioButton("Rx2", d_rx_channel == 1))
        d_rx_channel = 1;

    if (is_started)
        RImGui::endDisabled();

    // USB Compression
    if (RImGui::Combo("USB Compression##aaronia_usb_comp", &d_usb_compression, "Auto\0"
                                                                               "Raw\0"
                                                                               "Compressed\0"))
        set_others();

    // Gain settings
    bool gain_changed = false;
    gain_changed |= RImGui::SteppedSliderFloat("Ref Level##aaronia_ref_level", &d_level, d_min_level, 10.0f);
    gain_changed |= RImGui::Combo("AGC Mode##aaronia_agc_mode", &d_agc_mode, "Manual\0"
                                                                             "Peak\0"
                                                                             "Power\0");
    gain_changed |= RImGui::Checkbox("Amp##aaronia_amp", &d_enable_amp);
    gain_changed |= RImGui::Checkbox("Preamp##aaronia_preamp", &d_enable_preamp);
    if (gain_changed)
        set_gains();

    // Rescaling
    RImGui::Checkbox("Rescale##aaronia_rescale", &d_rescale);
}

void AaroniaSource::set_samplerate(uint64_t samplerate)
{
    for (int i = 0; i < (int)available_samplerates.size(); i++)
    {
        if (samplerate == available_samplerates[i])
        {
            selected_samplerate = i;
            current_samplerate = samplerate;
            return;
        }
    }

    throw satdump_exception("Unsupported samplerate : " + std::to_string(samplerate) + "!");
}

uint64_t AaroniaSource::get_samplerate()
{
    return current_samplerate;
}

std::vector<dsp::SourceDescriptor> AaroniaSource::getAvailableSources()
{
    std::vector<dsp::SourceDescriptor> results;

    AARTSAAPI_Handle h;
    if (AARTSAAPI_Open(&h) != AARTSAAPI_OK){
        logger->error("Could not open AARTSAAPI handle");
        return results;
    }

    if (AARTSAAPI_RescanDevices(&h, 2000) != AARTSAAPI_OK)
        logger->error("Could not scan for Aaronia Devices");

    AARTSAAPI_DeviceInfo dinfo;

    // Enumerate ECO first
    for (uint64_t i = 0; AARTSAAPI_EnumDevice(&h, L"spectranv6eco", i, &dinfo) == AARTSAAPI_OK; i++)
    {
        std::stringstream ss;
        ss << std::hex << dinfo.serialNumber; 
        results.push_back({"aaronia", "Spectran V6 ECO " + ss.str(), std::to_string(i)});
    }

    // Then enumerate PLUS next
    for (uint64_t i = 0; AARTSAAPI_EnumDevice(&h, L"spectranv6", i, &dinfo) == AARTSAAPI_OK; i++)
    {
        std::stringstream ss;
        ss << std::hex << dinfo.serialNumber;
        results.push_back({"aaronia", "Spectran V6 PLUS" + ss.str(), std::to_string(i)});
    }

    AARTSAAPI_Close(&h);

    return results;
}
