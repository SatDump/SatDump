#include "rtltcp.h"

#ifndef DISABLE_SDR_RTLTCP
#include <sstream>
#include "imgui/imgui.h"
#include "logger.h"

void SDRRtlTcp::runThread()
{
    while (should_run)
    {
        client.receiveSamples(samples8, 8192 * 2);
        for (int i = 0; i < 8192; i++)
            output_stream->writeBuf[i] = complex_t((samples8[i * 2 + 0] - 127) / 128.0f, (samples8[i * 2 + 1] - 127) / 128.0f);
        output_stream->swap(8192);
    }
}

SDRRtlTcp::SDRRtlTcp(std::map<std::string, std::string> parameters, uint64_t id) : SDRDevice(parameters, id)
{
    READ_PARAMETER_IF_EXISTS_FLOAT(gain, "gain");
    READ_PARAMETER_IF_EXISTS_FLOAT(agc, "agc");

    if (parameters.count("ip") == 0)
    {
        logger->error("No RTL-TCP IP provided! Things will not work!!!!");
    }

    if (parameters.count("port") == 0)
    {
        logger->error("No RTL-TCP Port provided! Things will not work!!!!");
    }

    if (!client.connectClient(parameters["ip"], std::stoi(parameters["port"])))
    {
        logger->critical("Could not open RTL-TCP server!");
        return;
    }
    logger->info("Opened RTL-TCP server!");
    std::fill(frequency, &frequency[100], 0);

    samples8 = new uint8_t[8192 * 2];
}

std::map<std::string, std::string> SDRRtlTcp::getParameters()
{
    d_parameters["gain"] = std::to_string(gain);
    d_parameters["agc"] = std::to_string((int)agc);

    return d_parameters;
}

void SDRRtlTcp::start()
{
    logger->info("Samplerate " + std::to_string(d_samplerate));
    client.setSamplerate(d_samplerate);

    logger->info("Frequency " + std::to_string(d_frequency));
    client.setFrequency(d_frequency);

    client.setGainMode(1);
    setGain(gain);
    should_run = true;
    workThread = std::thread(&SDRRtlTcp::runThread, this);
}

void SDRRtlTcp::stop()
{
    should_run = false;
    if (workThread.joinable())
        workThread.join();
}

SDRRtlTcp::~SDRRtlTcp()
{
    client.disconnect();
    delete[] samples8;
}

void SDRRtlTcp::drawUI()
{
    ImGui::Begin("RTL-TCP Control", NULL);

    //ImGui::SetNextItemWidth(100);
    ImGui::InputText("MHz", frequency, 100);

    ImGui::SameLine();

    if (ImGui::Button("Set"))
    {
        d_frequency = std::stof(frequency) * 1e6;
        setFrequency(d_frequency);
    }

    //ImGui::SetNextItemWidth(200);
    if (ImGui::SliderInt("Gain", &gain, 0, 49))
    {
        setGain(gain);
    }

    if (ImGui::Checkbox("AGC", &agc))
    {
        setGainMode(agc);
    }

    ImGui::End();
}

void SDRRtlTcp::setFrequency(float frequency)
{
    d_frequency = frequency;
    std::memcpy(this->frequency, std::to_string((float)d_frequency / 1e6).c_str(), std::to_string((float)d_frequency / 1e6).length());

    client.setFrequency(frequency);
}

void SDRRtlTcp::setGainMode(bool gainmode)
{
    client.setGainMode(gainmode);
}

void SDRRtlTcp::setGain(int gain)
{
    client.setGain(gain * 10);
}

void SDRRtlTcp::init()
{
}

std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> SDRRtlTcp::getDevices()
{
    std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> results;

    results.push_back({"RTL-TCP Server", RTLTCP, 0});

    return results;
}

char SDRRtlTcp::server_ip[100] = "localhost";
char SDRRtlTcp::server_port[100] = "1234";

std::map<std::string, std::string> SDRRtlTcp::drawParamsUI()
{
    ImGui::Text("Server IP");
    ImGui::SameLine();
    ImGui::InputText("##rtltcpip", server_ip, 100);

    ImGui::Text("Server Port");
    ImGui::SameLine();
    ImGui::InputText("##rtltcpport", server_port, 100);

    return {{"ip", std::string(server_ip)}, {"port", std::string(server_port)}};
}

std::string SDRRtlTcp::getID()
{
    return "rtltcp";
}
#endif