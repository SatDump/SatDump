#include "logger.h"
#include "module.h"
#include "pipeline.h"
#include <signal.h>
#include <filesystem>
#include "nlohmann/json.hpp"
#include <fstream>
#include "sdr/sdr.h"
#include "init.h"
#include "live_pipeline.h"
#include <volk/volk_alloc.hh>

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage : %s ingestor_config.json\n", argv[0]);
        exit(1);
    }

    // SatDump init
    initLogger();
    initSatdump();

    logger->warn("Keep in mind this ingestor is still WIP! Features such as SDR selection are lacking and coming shortly.");

    // Init thread pool
    ctpl::thread_pool processThreadPool(8);

    // Init SDR Devices
    initSDRs();
    std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> devices = getAllDevices();
    for (std::tuple<std::string, sdr_device_type, uint64_t> dev : devices)
        logger->info(std::get<0>(dev));

    // Settings we're gonna be using
    if (!std::filesystem::exists("ingestor.json"))
    {
        logger->error("Could not find ingestor.json!");
        exit(1);
    }
    nlohmann::json ingestor_cfg;
    {
        std::ifstream istream("ingestor.json");
        istream >> ingestor_cfg;
        istream.close();
    }

    //sdr_device_type sdr_type = AIRSPY; // To be implemented
    float samplerate = std::stoi(ingestor_cfg["samplerate"].get<std::string>());
    float frequency = std::stof(ingestor_cfg["frequency"].get<std::string>());
    std::map<std::string, std::string> device_parameters = ingestor_cfg["sdr_settings"].get<std::map<std::string, std::string>>();
    std::string downlink_pipeline = ingestor_cfg["pipeline"].get<std::string>();
    std::string output_folder = ingestor_cfg["output_folder"].get<std::string>();

    // Prepare other parameters
    std::map<std::string, std::string> parameters;
    parameters.emplace("samplerate", std::to_string(samplerate));
    parameters.emplace("baseband_format", "f32");

    // Init the device
    std::string devID = getDeviceIDStringByID(devices, 0);
    logger->debug("Device parameters " + devID + ":");
    for (const std::pair<std::string, std::string> param : device_parameters)
        logger->debug("   - " + param.first + " : " + param.second);

    std::shared_ptr<SDRDevice> radio = getDeviceByID(devices, device_parameters, 0);
    radio->setFrequency(frequency * 1e6);
    radio->setSamplerate(samplerate);
    radio->start();

    // Init pipeline
    logger->info("Init pipeline...");
    std::vector<Pipeline>::iterator it = std::find_if(pipelines.begin(),
                                                      pipelines.end(),
                                                      [&downlink_pipeline](const Pipeline &e)
                                                      {
                                                          return e.name == downlink_pipeline;
                                                      });

    if (it == pipelines.end())
    {
        logger->error("Pipeline " + downlink_pipeline + " does not exist!");
        exit(1);
    }

    if (!std::filesystem::exists(output_folder))
        std::filesystem::create_directory(output_folder);

    std::shared_ptr<LivePipeline> live_pipeline = std::make_shared<LivePipeline>(*it, parameters, output_folder);

    // Start processing
#ifdef _WIN32
    logger->info("Setting process priority to Realtime");
    SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
#endif

    std::shared_ptr<dsp::stream<std::complex<float>>> pipelineStream = std::make_shared<dsp::stream<std::complex<float>>>();
    live_pipeline->start(pipelineStream, processThreadPool);

    // Loop to pass samples from the SDR into set buffers for the pipeline
    volk::vector<std::complex<float>> sample_buffer_vec;
    int cnt = 0;
    int buf_size = 8192;
    while (true)
    {
        if ((int)sample_buffer_vec.size() < buf_size)
        {
            cnt = radio->output_stream->read();
            if (cnt > 0)
            {
                sample_buffer_vec.insert(sample_buffer_vec.end(), radio->output_stream->readBuf, &radio->output_stream->readBuf[cnt]);
                radio->output_stream->flush();
            }
            else
            {
                continue;
            }
        }

        std::memcpy(pipelineStream->writeBuf, sample_buffer_vec.data(), buf_size * sizeof(std::complex<float>));
        pipelineStream->swap(buf_size);
        sample_buffer_vec.erase(sample_buffer_vec.begin(), sample_buffer_vec.begin() + buf_size);
    }

    logger->info("Done! Goodbye");
}