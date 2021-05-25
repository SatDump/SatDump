#include "spyserver.h"
#include <sstream>
#include "live.h"
#include "imgui/imgui.h"
#include "logger.h"
#include <volk/volk.h>

#ifdef BUILD_LIVE

SDRSpyServer::SDRSpyServer(std::map<std::string, std::string> parameters, uint64_t id) : SDRDevice(parameters, id)
{
    if (parameters.count("ip") == 0)
    {
        logger->error("No SpyServer IP provided! Things will not work!!!!");
    }

    if (parameters.count("port") == 0)
    {
        logger->error("No SpyServer Port provided! Things will not work!!!!");
    }

    client = std::make_shared<ss_client>(parameters["ip"], std::stoi(parameters["port"]), 1, 0, 0, 16);

    logger->info("Opened SpyServer device!");

    samples = new int16_t[8192 * 2];
}

void SDRSpyServer::runThread()
{
    while (should_run)
    {
        client->get_iq_data<int16_t>(8192, samples);
        volk_16i_s32f_convert_32f_u((float *)output_stream->writeBuf, (const int16_t *)samples, 1.0f / 0.00004f, 8192 * 2);
        output_stream->swap(8192);
    }
}

void SDRSpyServer::start()
{
    // Samplerate
    client->set_sample_rate(d_samplerate);
    client->set_sample_rate_by_decim_stage(0);

    // Frequency
    client->set_iq_center_freq(d_frequency);

    // Gain
    client->set_gain_mode(false);
    client->set_gain(gain);

    // Start
    should_run = true;
    workThread = std::thread(&SDRSpyServer::runThread, this);
    client->start();
}

void SDRSpyServer::stop()
{
    //airspy_stop_rx(dev);
    should_run = false;
    client->stop();
    if (workThread.joinable())
        workThread.join();
}

SDRSpyServer::~SDRSpyServer()
{
    //airspy_close(dev);
    delete[] samples;
}

void SDRSpyServer::drawUI()
{
    ImGui::Begin("SpyServer Control", NULL);

    ImGui::SetNextItemWidth(100);
    ImGui::InputText("MHz", frequency, 100);

    ImGui::SameLine();

    if (ImGui::Button("Set"))
    {
        d_frequency = std::stof(frequency) * 1e6;
        client->set_iq_center_freq(d_frequency);
    }

    ImGui::SetNextItemWidth(200);
    if (ImGui::SliderInt("Gain", &gain, 0, 22))
    {
        client->set_gain(gain);
    }

    ImGui::End();
}

void SDRSpyServer::setFrequency(int frequency)
{
    d_frequency = frequency;
    std::memcpy(this->frequency, std::to_string((float)d_frequency / 1e6).c_str(), std::to_string((float)d_frequency / 1e6).length());
}

void SDRSpyServer::init()
{
}

std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> SDRSpyServer::getDevices()
{
    std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> results;

    results.push_back({"SpyServer", SPYSERVER, 0});

    return results;
}

char SDRSpyServer::server_ip[100] = "localhost";
char SDRSpyServer::server_port[100] = "5555";

std::map<std::string, std::string> SDRSpyServer::drawParamsUI()
{
    ImGui::Text("Server IP");
    ImGui::SameLine();
    ImGui::InputText("##spyserverip", server_ip, 100);

    ImGui::Text("Server Port");
    ImGui::SameLine();
    ImGui::InputText("##spyserverport", server_port, 100);

    return {{"ip", std::string(server_ip)}, {"port", std::string(server_port)}};
}
#endif