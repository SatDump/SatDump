#include "spyserver.h"

#ifndef DISABLE_SDR_SPYSERVER
#include <sstream>
#include "imgui/imgui.h"
#include "logger.h"
#include <volk/volk.h>

const int SpyServerFormatsBitDepths[] = {8, 16, 32};

SDRSpyServer::SDRSpyServer(std::map<std::string, std::string> parameters, uint64_t id) : SDRDevice(parameters, id)
{
    READ_PARAMETER_IF_EXISTS_FLOAT(gain, "gain");
    READ_PARAMETER_IF_EXISTS_FLOAT(digital_gain, "digital_gain");

    if (parameters.count("ip") == 0)
    {
        logger->error("No SpyServer IP provided! Things will not work!!!!");
    }

    if (parameters.count("port") == 0)
    {
        logger->error("No SpyServer Port provided! Things will not work!!!!");
    }

    if (parameters.count("sample_format") == 0)
    {
        logger->error("No SpyServer bit depth provided! Using F32.");
        sampleFormat = SpyServerStreamFormat::SPYSERVER_STREAM_FORMAT_FLOAT;
    }
    else
    {
        sampleFormat = (SpyServerStreamFormat)std::stoi(parameters["sample_format"]);
    }

    client = spyserver::connect(parameters["ip"], std::stoi(parameters["port"]), output_stream.get());

    logger->info("Opened SpyServer device!");

    if (!client->waitForDevInfo(4000))
        logger->critical("Didn't get device infos");

    std::fill(frequency, &frequency[100], 0);
}

std::map<std::string, std::string> SDRSpyServer::getParameters()
{
    d_parameters["gain"] = std::to_string(gain);
    d_parameters["digital_gain"] = std::to_string(digital_gain);

    return d_parameters;
}

void SDRSpyServer::start()
{
    // Format
    client->setSetting(SPYSERVER_SETTING_IQ_FORMAT, sampleFormat);
    client->setSetting(SPYSERVER_SETTING_STREAMING_MODE, SPYSERVER_STREAM_MODE_IQ_ONLY);

    // Samplerate
    stageToUse = client->devInfo.MinimumIQDecimation;
    {
        std::vector<float> samplerates;
        for (int i = client->devInfo.MinimumIQDecimation; i <= (int)client->devInfo.DecimationStageCount; i++)
        {
            float samplerate = client->devInfo.MaximumSampleRate / (float)(1 << i);
            samplerates.push_back(samplerate);
            logger->trace(samplerate);
        }

        if (std::find(samplerates.begin(), samplerates.end(), d_samplerate) != samplerates.end())
        {
            stageToUse = std::find(samplerates.begin(), samplerates.end(), d_samplerate) - samplerates.begin();
        }
        else
        {
            logger->error("Desired samplerate not available. Default to max.");
        }
    }

    client->setSetting(SPYSERVER_SETTING_IQ_DECIMATION, stageToUse);

    // Frequency
    client->setSetting(SPYSERVER_SETTING_IQ_FREQUENCY, d_frequency);

    // Gain
    client->setSetting(SPYSERVER_SETTING_GAIN, gain);
    if (digital_gain == 0)
        digital_gain = client->computeDigitalGain(SpyServerFormatsBitDepths[sampleFormat], gain, stageToUse);
    client->setSetting(SPYSERVER_SETTING_IQ_DIGITAL_GAIN, digital_gain);

    client->startStream();
}

void SDRSpyServer::stop()
{
    client->stopStream();
}

SDRSpyServer::~SDRSpyServer()
{
    client->close();
}

void SDRSpyServer::drawUI()
{
    ImGui::Begin("SpyServer Control", NULL);

    //ImGui::SetNextItemWidth(100);
    ImGui::InputText("MHz", frequency, 100);

    ImGui::SameLine();

    if (ImGui::Button("Set"))
    {
        d_frequency = std::stof(frequency) * 1e6;
        client->setSetting(SPYSERVER_SETTING_IQ_FREQUENCY, d_frequency);
    }

    //ImGui::SetNextItemWidth(200);
    if (ImGui::SliderInt("Gain", &gain, 0, client->devInfo.MaximumGainIndex))
    {
        client->setSetting(SPYSERVER_SETTING_GAIN, gain);
        digital_gain = client->computeDigitalGain(SpyServerFormatsBitDepths[sampleFormat], gain, stageToUse);
        client->setSetting(SPYSERVER_SETTING_IQ_DIGITAL_GAIN, digital_gain);
    }

    //ImGui::SetNextItemWidth(200);
    if (ImGui::SliderInt("Digital Gain", &digital_gain, 0, client->devInfo.MaximumGainIndex))
    {
        client->setSetting(SPYSERVER_SETTING_IQ_DIGITAL_GAIN, digital_gain);
    }

    ImGui::End();
}

void SDRSpyServer::setFrequency(float frequency)
{
    d_frequency = frequency;
    std::memcpy(this->frequency, std::to_string((float)d_frequency / 1e6).c_str(), std::to_string((float)d_frequency / 1e6).length());
    client->setSetting(SPYSERVER_SETTING_IQ_FREQUENCY, d_frequency);
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

char SDRSpyServer::server_ip[100] = "0.0.0.0";
char SDRSpyServer::server_port[100] = "5555";
SpyServerStreamFormat SDRSpyServer::serverSampleFormat = SpyServerStreamFormat::SPYSERVER_STREAM_FORMAT_UINT8;
int serverSampleFormatId = 0;

std::map<std::string, std::string> SDRSpyServer::drawParamsUI()
{
    ImGui::Text("Server IP");
    ImGui::SameLine();
    ImGui::InputText("##spyserverip", server_ip, 100);

    ImGui::Text("Server Port");
    ImGui::SameLine();
    ImGui::InputText("##spyserverport", server_port, 100);

    ImGui::Text("Sample format");
    ImGui::SameLine();
    if (ImGui::Combo("##sampleformatspyserver", &serverSampleFormatId, "uint8\0"
                                                                       "int16\0"
                                                                       "float32\0"))
    {
        if (serverSampleFormatId == 0)
            serverSampleFormat = SpyServerStreamFormat::SPYSERVER_STREAM_FORMAT_UINT8;
        else if (serverSampleFormatId == 1)
            serverSampleFormat = SpyServerStreamFormat::SPYSERVER_STREAM_FORMAT_INT16;
        else if (serverSampleFormatId == 2)
            serverSampleFormat = SpyServerStreamFormat::SPYSERVER_STREAM_FORMAT_FLOAT;
    }

    return {{"ip", std::string(server_ip)}, {"port", std::string(server_port)}, {"sample_format", std::to_string(serverSampleFormat)}};
}

std::string SDRSpyServer::getID()
{
    return "spyserver";
}
#endif