/*
* Plugin for Harogic SA devices
* Developped by Sebastien Dudek (@FlUxIuS) at @Penthertz
*/
#include "harogic_sdr.h"
#include "common/utils.h"
#include <sstream>
#include <iomanip>

HarogicSource::HarogicSource(dsp::SourceDescriptor source) : DSPSampleSource(source) {
}

HarogicSource::~HarogicSource() {
    stop();
    close();
}

// Device discovery function populates the UI dropdown.
std::vector<dsp::SourceDescriptor> HarogicSource::getAvailableSources() {
    std::vector<dsp::SourceDescriptor> results;
    BootProfile_TypeDef profile = {};
    profile.PhysicalInterface = PhysicalInterface_TypeDef::USB;
    profile.DevicePowerSupply = DevicePowerSupply_TypeDef::USBPortOnly;
    void* dev;
    BootInfo_TypeDef binfo;

    for (int i = 0; i < 128; i++) {
        if (Device_Open(&dev, i, &profile, &binfo) < 0) break;
        
        std::stringstream ss;
        ss << std::hex << std::uppercase << binfo.DeviceInfo.DeviceUID;
        std::string serial_str = ss.str();

        std::string label = "Harogic " + serial_str;
        results.push_back({getID(), label, std::to_string(i)});
        Device_Close(&dev);
    }
    return results;
}

void HarogicSource::open() {
    if (is_open) return;
    
    dev_index = 0; // We will always use the first device.

    // Get device capabilities and available sample rates
    BootProfile_TypeDef bprofile = {};
    bprofile.PhysicalInterface = PhysicalInterface_TypeDef::USB;
    bprofile.DevicePowerSupply = DevicePowerSupply_TypeDef::USBPortOnly;
    BootInfo_TypeDef binfo;
    void* dev_tmp;

    if (Device_Open(&dev_tmp, dev_index, &bprofile, &binfo) < 0) {
        // If device 0 isn't found, then no devices are present.
        throw satdump_exception("No Harogic devices found.");
    }
    dev_info = binfo.DeviceInfo;

    IQS_Profile_TypeDef default_profile;
    IQS_ProfileDeInit(&dev_tmp, &default_profile);

    available_samplerates.clear();
    for (int i = 0; i < 8; i++) {
        if (default_profile.NativeIQSampleRate_SPS > 0) {
            available_samplerates.push_back(default_profile.NativeIQSampleRate_SPS / (1 << i));
        }
    }
    
    // Create the samplerate string for the UI
    samplerate_option_str = "";
    for (uint64_t sr : available_samplerates) {
        samplerate_option_str += format_notated(sr, "S/s") + '\0';
    }

    Device_Close(&dev_tmp);
    is_open = true;
    logger->info("Harogic source is open. Will use first available device.");
}

void HarogicSource::start() {
    if (is_started) return;
    DSPSampleSource::start();

    // The index is already set to 0 in open()
    BootProfile_TypeDef bprofile = {};
    bprofile.PhysicalInterface = PhysicalInterface_TypeDef::USB;
    bprofile.DevicePowerSupply = DevicePowerSupply_TypeDef::USBPortAndPowerPort;
    BootInfo_TypeDef binfo;
    
    if (Device_Open(&dev_handle, dev_index, &bprofile, &binfo) < 0) {
        throw satdump_exception("Failed to open Harogic device for streaming.");
    }

    // Initialize the streaming profile
    IQS_ProfileDeInit(&dev_handle, &profile);
    profile.BusTimeout_ms = 1000;
    profile.TriggerSource = Bus;
    profile.TriggerMode = Adaptive;
    profile.Atten = -1; // Auto

    update_settings(); // Apply all settings

    IQS_StreamInfo_TypeDef info;
    if (IQS_Configuration(&dev_handle, &profile, &profile, &info) < 0) {
        Device_Close(&dev_handle);
        dev_handle = nullptr;
        throw satdump_exception("Failed to configure Harogic stream.");
    }
    
    if (IQS_BusTriggerStart(&dev_handle) < 0) {
        Device_Close(&dev_handle);
        dev_handle = nullptr;
        throw satdump_exception("Failed to start Harogic stream.");
    }

    is_started = true;
    thread_should_run = true;
    work_thread = std::thread(&HarogicSource::mainThread, this);
    logger->info("Started Harogic stream.");
}

void HarogicSource::stop() {
    if (!is_started) return;
    
    thread_should_run = false;
    output_stream->stopWriter();
    if (work_thread.joinable()) {
        work_thread.join();
    }
    
    if (dev_handle) {
        IQS_BusTriggerStop(&dev_handle);
        Device_Close(&dev_handle);
        dev_handle = nullptr;
    }

    is_started = false;
    logger->info("Stopped Harogic stream.");
}

void HarogicSource::close() {
    if (is_started) {
        stop();
    }
    is_open = false;
}

void HarogicSource::set_frequency(uint64_t frequency) {
    d_frequency = frequency;
    if (is_started) {
        update_settings();
    }
}

void HarogicSource::set_samplerate(uint64_t samplerate) {
    for (int i = 0; i < (int)available_samplerates.size(); i++) {
        if (samplerate == available_samplerates[i]) {
            selected_samplerate_idx = i;
            current_samplerate = samplerate;
            if (is_started) update_settings();
            return;
        }
    }
    throw satdump_exception("Unsupported samplerate: " + std::to_string(samplerate) + "!");
}

uint64_t HarogicSource::get_samplerate() {
    return current_samplerate;
}

void HarogicSource::update_settings() {
    if (!dev_handle) return; // Don't do anything if the stream isn't active
    
    // Set all current parameters into the profile struct
    current_samplerate = available_samplerates[selected_samplerate_idx];
    samps_int8 = (current_samplerate > RESOLTRIG);
    
    profile.DataFormat = samps_int8 ? Complex8bit : Complex16bit;
    profile.CenterFreq_Hz = d_frequency;
    profile.RefLevel_dBm = d_ref_level;
    profile.DecimateFactor = (uint32_t)(available_samplerates[0] / current_samplerate);
    profile.Preamplifier = (d_preamp == 1) ? AutoOn : ForcedOff;
    profile.EnableIFAGC = (d_if_agc == 1);
    profile.GainStrategy = (d_gain_strategy == 0) ? LowNoisePreferred : HighLinearityPreferred;

    RxPort_TypeDef antenna_ports[] = {ExternalPort, InternalPort, ANT_Port, TR_Port, SWR_Port, INT_Port};
    profile.RxPort = antenna_ports[d_antenna_idx];

    // Re-configure the device with the new settings
    IQS_StreamInfo_TypeDef info;
    int ret = IQS_Configuration(&dev_handle, &profile, &profile, &info);
    if (ret < 0) {
        logger->error("Failed to apply new settings to Harogic device: %d", ret);
        return; // Stop here if configuration failed
    }

    ret = IQS_BusTriggerStart(&dev_handle);
    if (ret < 0) {
        logger->error("Failed to re-start stream after settings change: %d", ret);
    } else {
        logger->debug("Applied new settings and restarted stream. Freq: %.2f MHz", profile.CenterFreq_Hz / 1e6);
    }
}

void HarogicSource::drawControlUI() {
    if (is_started) RImGui::beginDisabled();

    RImGui::Combo("Sample Rate", &selected_samplerate_idx, samplerate_option_str.c_str());
    if(!available_samplerates.empty()) {
        current_samplerate = available_samplerates[selected_samplerate_idx];
    }

    if (is_started) RImGui::endDisabled();

    bool settings_changed = false;
    settings_changed |= RImGui::SteppedSliderFloat("Ref Level (dBm)", &d_ref_level, -100.0f, 7.0f);
    settings_changed |= RImGui::Checkbox("Preamp (Auto)", (bool*)&d_preamp);
    settings_changed |= RImGui::Checkbox("IF AGC", (bool*)&d_if_agc);
    settings_changed |= RImGui::Combo("Gain Strategy", &d_gain_strategy, "Low Noise\0High Linearity\0");
    settings_changed |= RImGui::Combo("Antenna", &d_antenna_idx, "External\0Internal\0ANT\0T/R\0SWR\0INT\0");

    if (settings_changed && is_started) {
        update_settings();
    }
}

void HarogicSource::set_settings(nlohmann::json settings) {
    d_settings = settings;
    d_ref_level = getValueOrDefault(d_settings["ref_level"], d_ref_level);
    d_preamp = getValueOrDefault(d_settings["preamp"], d_preamp);
    d_if_agc = getValueOrDefault(d_settings["if_agc"], d_if_agc);
    d_gain_strategy = getValueOrDefault(d_settings["gain_strategy"], d_gain_strategy);
    d_antenna_idx = getValueOrDefault(d_settings["antenna"], d_antenna_idx);

    if (is_started) update_settings();
}

nlohmann::json HarogicSource::get_settings() {
    d_settings["ref_level"] = d_ref_level;
    d_settings["preamp"] = d_preamp;
    d_settings["if_agc"] = d_if_agc;
    d_settings["gain_strategy"] = d_gain_strategy;
    d_settings["antenna"] = d_antenna_idx;
    return d_settings;
}

void HarogicSource::mainThread() {
    IQStream_TypeDef iqs;
    while (thread_should_run) {
        int ret = IQS_GetIQStream_PM1(&dev_handle, &iqs);

        if (ret < 0) {
            if (ret == APIRETVAL_WARNING_BusTimeOut || ret == APIRETVAL_WARNING_IFOverflow) {
                continue; 
            }
            logger->error("Fatal streaming error on Harogic device: %d. Stopping thread.", ret);
            thread_should_run = false;
            break;
        }

        uint32_t samples_in_packet = iqs.IQS_StreamInfo.PacketSamples;
        if (samples_in_packet == 0 || iqs.AlternIQStream == nullptr) continue;

        if (samples_in_packet > dsp::STREAM_BUFFER_SIZE) {
            logger->error("Harogic packet size (%u) is larger than the stream buffer (%d), discarding.", samples_in_packet, dsp::STREAM_BUFFER_SIZE);
            continue;
        }

        complex_t* out_buf = (complex_t*)output_stream->writeBuf;
        
        if (samps_int8) {
            int8_t* in = (int8_t*)iqs.AlternIQStream;
            for (size_t i = 0; i < samples_in_packet; ++i) {
                out_buf[i] = {in[2*i] / 127.0f, in[2*i+1] / 127.0f};
            }
        } else {
            int16_t* in = (int16_t*)iqs.AlternIQStream;
            for (size_t i = 0; i < samples_in_packet; ++i) {
                out_buf[i] = {in[2*i] / 32767.0f, in[2*i+1] / 32767.0f};
            }
        }
        
        output_stream->swap(samples_in_packet);
    }
}