#include "lime_xtrx_sdr.h"

using namespace lime;

static void lime_log(LogLevel lvl, const std::string& msg)
{
    if (lvl <= LogLevel::Warning) logger->warn("%s", msg.c_str());
    else                          logger->trace("%s", msg.c_str());
}

LimeXTRXSDRSource::LimeXTRXSDRSource(dsp::SourceDescriptor source) : DSPSampleSource(source), samplerate_widget("Samplerate"), bandwidth_widget("Bandwidth")
{
    lime::registerLogHandler(lime_log);
}

LimeXTRXSDRSource::~LimeXTRXSDRSource()
{
    stop();
    close();
}

void LimeXTRXSDRSource::set_gains()
{
    if (!is_started)
        return;

    if (gain_mode_manual)
    {
        limeDevice->SetGain(moduleIndex, TRXDir::Rx, channel_id, eGainTypes::LNA, gain_lna);
        limeDevice->SetGain(moduleIndex, TRXDir::Rx, channel_id, eGainTypes::TIA, gain_tia);
        limeDevice->SetGain(moduleIndex, TRXDir::Rx, channel_id, eGainTypes::PGA, gain_pga);
        logger->debug("Set Lime XTRX SDR (LNA) Gain to %d", gain_lna);
        logger->debug("Set Lime XTRX SDR (TIA) Gain to %d", gain_tia);
        logger->debug("Set Lime XTRX SDR (PGA) Gain to %d", gain_pga);
    }
    else
    {
        limeDevice->SetGain(moduleIndex, TRXDir::Rx, channel_id, eGainTypes::LNA, 0);
        limeDevice->SetGain(moduleIndex, TRXDir::Rx, channel_id, eGainTypes::TIA, 0);
        limeDevice->SetGain(moduleIndex, TRXDir::Rx, channel_id, eGainTypes::PGA, 0);
        logger->error("Set Lime XTRX SDR (auto) Gain to %d (unsupported)", gain);
    }
}

void LimeXTRXSDRSource::set_others()
{
    if (!is_started)
        return;

    if (manual_bandwidth)
    {
        limeDevice->SetLowPassFilter(moduleIndex, TRXDir::Rx, channel_id, bandwidth_widget.get_value());
    }
    else
    {
        limeDevice->SetLowPassFilter(moduleIndex, TRXDir::Rx, channel_id, samplerate_widget.get_value());
    }
}

void LimeXTRXSDRSource::set_settings(nlohmann::json settings)
{
    d_settings = settings;

    gain = getValueOrDefault(d_settings["gain"], gain);
    gain_lna = getValueOrDefault(d_settings["lna_gain"], gain_lna);
    gain_tia = getValueOrDefault(d_settings["tia_gain"], gain_tia);
    gain_pga = getValueOrDefault(d_settings["pga_gain"], gain_pga);
    path_id = getValueOrDefault(d_settings["path_id"], path_id);
    manual_bandwidth = getValueOrDefault(d_settings["manual_bw"], manual_bandwidth);
    bandwidth_widget.set_value(getValueOrDefault(d_settings["manual_bw_value"], samplerate_widget.get_value()));
    channel_id = getValueOrDefault(d_settings["channel_id"], channel_id);

    if (is_started)
    {
        set_gains();
        set_others();
    }
}

nlohmann::json LimeXTRXSDRSource::get_settings()
{
    d_settings["gain"] = gain;
    d_settings["lna_gain"] = gain_lna;
    d_settings["tia_gain"] = gain_tia;
    d_settings["pga_gain"] = gain_pga;
    d_settings["path_id"] = path_id;
    d_settings["manual_bw"] = manual_bandwidth;
    d_settings["manual_bw_value"] = bandwidth_widget.get_value();
    d_settings["channel_id"] = channel_id;

    return d_settings;
}

void LimeXTRXSDRSource::open()
{
    is_open = true;

    // Set available samplerates
    std::vector<double> available_samplerates;
    for (int i = 1; i <= 120; i++)
        available_samplerates.push_back(i * 1e6);

    samplerate_widget.set_list(available_samplerates, true);
    bandwidth_widget.set_list(available_samplerates, true);
}

void LimeXTRXSDRSource::start()
{
    DSPSampleSource::start();

    uint64_t current_samplerate = samplerate_widget.get_value();

    if(!is_started)
    {
        auto handles = DeviceRegistry::enumerate();
        if (handles.empty())
            throw std::runtime_error("No Lime XTRX SDR devices found");

        limeDevice = DeviceRegistry::makeDevice(handles.at(stoi(d_sdr_id)));
        limeDevice->Init();
    }

    limeDevice->EnableChannel(moduleIndex, TRXDir::Rx, channel_id, true);
    limeDevice->SetAntenna(moduleIndex, TRXDir::Rx, channel_id, path_id);

    StreamConfig scfg;
    scfg.channels.at(TRXDir::Rx).push_back(channel_id);
    scfg.format     = DataFormat::F32;  // host: float IQ; change to I16 if your DSP expects int16
    scfg.linkFormat = DataFormat::I16;  // typical link format

    logger->debug("Set Lime XTRX SDR samplerate to " + std::to_string(current_samplerate));
    limeDevice->SetSampleRate(moduleIndex, TRXDir::Rx, channel_id, current_samplerate, 0);

    is_started = true;

    limeDevice->SetFrequency(moduleIndex, TRXDir::Rx, channel_id, d_frequency);

    set_gains();
    set_others();

    limeStream = limeDevice->StreamCreate(scfg, moduleIndex);
    limeStream->Start();

    set_gains(); // Needs to configure twice because of a bug in Lime Suite NG

    thread_should_run = true;
    work_thread = std::thread(&LimeXTRXSDRSource::mainThread, this);
}

void LimeXTRXSDRSource::stop()
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
        limeStream->Stop();
        limeDevice->EnableChannel(moduleIndex, TRXDir::Rx, channel_id, false);
        DeviceRegistry::freeDevice(limeDevice);
    }
    is_started = false;
}

void LimeXTRXSDRSource::close()
{
    is_open = false;
}

void LimeXTRXSDRSource::set_frequency(uint64_t frequency)
{
    if (is_started)
    {
        limeDevice->SetFrequency(moduleIndex, TRXDir::Rx, channel_id, frequency);

        logger->debug("Set Lime XTRX SDR frequency to %d", frequency);
    }
    DSPSampleSource::set_frequency(frequency);
}

void LimeXTRXSDRSource::drawControlUI()
{
    if (is_started)
        RImGui::beginDisabled();

    samplerate_widget.render();

    // Channel setting
    RImGui::Combo("Channel####limesdrchannel", &channel_id, "Channel 1\0"
                                                            "Channel 2\0");

    RImGui::Combo("Path####limesdrpath", &path_id, "NONE\0"
                                                   "LNAH\0"
                                                   "LNAL\0"
                                                   "LNAW\0");

    if (is_started)
        RImGui::endDisabled();

    // Gain settings
    bool gain_changed = false;

    // Fixme: LimeSuite NG doesn't support automatic gain
    //if (RImGui::RadioButton("Auto", !gain_mode_manual))
    //{
    //    gain_mode_manual = false;
    //    gain_changed = true;
    //}
    //RImGui::SameLine();
    //if (RImGui::RadioButton("Manual", gain_mode_manual))
    //{
    //    gain_mode_manual = true;
    //    gain_changed = true;
    //}
    gain_mode_manual = true; // Auto isn't supported in Lime Suite NG

    if (gain_mode_manual)
    {
        gain_changed |= RImGui::SteppedSliderInt("LNA Gain", &gain_lna, 0, 30);
        gain_changed |= RImGui::SteppedSliderInt("TIA Gain", &gain_tia, 0, 12);
        gain_changed |= RImGui::SteppedSliderInt("PGA Gain", &gain_pga, -12, 19);
    }
    else
    {
        gain_changed |= RImGui::SteppedSliderInt("Gain", &gain, 0, 73);
    }

    if (gain_changed)
        set_gains();

    bool bw_update = RImGui::Checkbox("Manual Bandwidth", &manual_bandwidth);
    if (manual_bandwidth)
        bw_update = bw_update || bandwidth_widget.render();
    if (bw_update && is_started)
        set_others();
}

void LimeXTRXSDRSource::set_samplerate(uint64_t samplerate)
{
    if (!samplerate_widget.set_value(samplerate, 100e6))
        throw satdump_exception("Unsupported samplerate : " + std::to_string(samplerate) + "!");
}

uint64_t LimeXTRXSDRSource::get_samplerate()
{
    return samplerate_widget.get_value();
}

std::vector<dsp::SourceDescriptor> LimeXTRXSDRSource::getAvailableSources()
{
    int k=0;
    std::vector<dsp::SourceDescriptor> results;
    auto handles = DeviceRegistry::enumerate();
    for (const auto& h : handles)
    {
        std::stringstream ss;
        ss << std::hex << h.Serialize();
        results.push_back({"limesdr", "LimeSDR " + ss.str(), std::to_string(k++)});
    }
    return results;
}

void LimeXTRXSDRSource::mainThread()
{
    int buffer_size = std::min<int>(samplerate_widget.get_value() / 250, dsp::STREAM_BUFFER_SIZE);
    logger->trace("Lime XTRX SDR Buffer size %d", buffer_size);

    StreamMeta md;

    while (thread_should_run)
    {
        auto* buf0 = reinterpret_cast<lime::complex32f_t*>(output_stream->writeBuf);
        lime::complex32f_t* dest[1] = { buf0 };
        uint32_t cnt = limeStream->StreamRx(dest, buffer_size, &md, std::chrono::milliseconds(2000));

        if (cnt > 0)
            output_stream->swap(cnt);
    }
}
