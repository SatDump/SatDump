#include "net_source.h"
#include "imgui/imgui_stdlib.h"

void NetSource::set_settings(nlohmann::json settings)
{
    d_settings = settings;

    if (d_settings.contains("mode"))
    {
        if (d_settings["mode"].get<std::string>() == "udp")
        {
            mode = MODE_UDP;
            port = getValueOrDefault(d_settings["port"], port);
        }
        else if (d_settings["mode"].get<std::string>() == "nng_sub")
        {
            mode = MODE_NNGSUB;
            address = getValueOrDefault(d_settings["address"], address);
            port = getValueOrDefault(d_settings["port"], port);
        }
    }
}

nlohmann::json NetSource::get_settings()
{

    if (mode == MODE_UDP)
    {
        d_settings["mode"] = "udp";
        d_settings["port"] = port;
    }
    else if (mode == MODE_NNGSUB)
    {
        d_settings["mode"] = "nng_sub";
        d_settings["address"] = address;
        d_settings["port"] = port;
    }

    return d_settings;
}

void NetSource::open()
{
    // Nothing to do
    is_open = true;
}

void NetSource::run_thread()
{
    while (should_run)
    {
        if (is_started)
        {
            if (mode == MODE_UDP)
            {
                int bytes = udp_server->recv((uint8_t *)output_stream->writeBuf, 8192 * sizeof(complex_t));
                output_stream->swap(bytes / sizeof(complex_t));
            }
            else if (mode == MODE_NNGSUB)
            {
                size_t lpkt_size;
                nng_recv(n_sock, output_stream->writeBuf, &lpkt_size, (int)0);
                output_stream->swap(lpkt_size / sizeof(complex_t));
            }
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

void NetSource::start()
{
    if (mode == MODE_UDP)
    {
        udp_server = std::make_shared<net::UDPServer>(port);
    }
    else if (mode == MODE_NNGSUB)
    {
        logger->info("Opening TCP socket on " + std::string("tcp://" + address + ":" + std::to_string(port)));
        nng_sub0_open_raw(&n_sock);
        nng_dialer_create(&n_dialer, n_sock, std::string("tcp://" + address + ":" + std::to_string(port)).c_str());
        nng_dialer_start(n_dialer, (int)0);
    }

    DSPSampleSource::start();

    set_frequency(d_frequency);

    is_started = true;
}

void NetSource::stop()
{
    if (!is_started)
        return;

    is_started = false;

    if (mode == MODE_UDP)
    {
        udp_server.reset();
    }
    else if (mode == MODE_NNGSUB)
    {
        nng_dialer_close(n_dialer);
        nng_close(n_sock);
    }

    output_stream->flush();
}

void NetSource::close()
{
    is_open = false;
}

void NetSource::set_frequency(uint64_t frequency)
{
    // if (is_open && is_connected)
    // {
    //     client->setFrequency(frequency);
    //     logger->debug("Set SDR++ Server frequency to %d", frequency);
    // }
    DSPSampleSource::set_frequency(frequency);
}

void NetSource::drawControlUI()
{
    if (is_started)
        style::beginDisabled();

    current_samplerate.draw();

    if (ImGui::RadioButton("UDP##netsource", mode == MODE_UDP))
        mode = MODE_UDP;
    if (ImGui::RadioButton("NNG Sub##netsource", mode == MODE_NNGSUB))
        mode = MODE_NNGSUB;

    if (mode == MODE_UDP)
    {
        ImGui::InputInt("Port", &port);
    }
    else if (mode == MODE_NNGSUB)
    {
        ImGui::InputText("Address", &address);
        ImGui::InputInt("Port", &port);
    }

    if (is_started)
        style::endDisabled();
}

void NetSource::set_samplerate(uint64_t samplerate)
{
    current_samplerate.set(samplerate);
}

uint64_t NetSource::get_samplerate()
{
    return current_samplerate.get();
}

std::vector<dsp::SourceDescriptor> NetSource::getAvailableSources()
{
    std::vector<dsp::SourceDescriptor> results;

    results.push_back({"net_source", "Network Source", "0", false});

    return results;
}
