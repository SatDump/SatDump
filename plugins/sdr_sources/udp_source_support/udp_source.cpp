#include "udp_source.h"

void UDPSource::set_settings(nlohmann::json settings)
{
    d_settings = settings;

    port = getValueOrDefault(d_settings["port"], port);
}

nlohmann::json UDPSource::get_settings()
{
    d_settings["port"] = port;

    return d_settings;
}

void UDPSource::open()
{
    // Nothing to do
    is_open = true;
}

void UDPSource::run_thread()
{
    while (should_run)
    {
        if (is_started)
        {
            int bytes = udp_server->recv((uint8_t *)output_stream->writeBuf, 8192 * sizeof(complex_t));
            // logger->info(bytes);
            output_stream->swap(bytes / sizeof(complex_t));
            // logger->info("done");
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

void UDPSource::start()
{
    udp_server = std::make_shared<net::UDPServer>(port);

    DSPSampleSource::start();

    set_frequency(d_frequency);

    is_started = true;
}

void UDPSource::stop()
{
    udp_server.reset();
    is_started = false;
}

void UDPSource::close()
{
    is_open = false;
}

void UDPSource::set_frequency(uint64_t frequency)
{
    // if (is_open && is_connected)
    // {
    //     client->setFrequency(frequency);
    //     logger->debug("Set SDR++ Server frequency to %d", frequency);
    // }
    DSPSampleSource::set_frequency(frequency);
}

void UDPSource::drawControlUI()
{
    if (is_started)
        style::beginDisabled();

    ImGui::InputInt("Samplerate", &current_samplerate, 0);

    ImGui::InputInt("Port", &port);

    if (is_started)
        style::endDisabled();
}

void UDPSource::set_samplerate(uint64_t samplerate)
{
    current_samplerate = samplerate;
}

uint64_t UDPSource::get_samplerate()
{
    return current_samplerate;
}

std::vector<dsp::SourceDescriptor> UDPSource::getAvailableSources()
{
    std::vector<dsp::SourceDescriptor> results;

    results.push_back({"udp_source", "UDP Source", 0});

    return results;
}