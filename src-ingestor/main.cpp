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
#ifdef _WIN32
#include <windows.h>
#endif
#include <nng/nng.h>
#include <nng/supplemental/http/http.h>
#include <nng/supplemental/util/platform.h>

std::shared_ptr<LivePipeline> live_pipeline;

bool should_stop = false;

#ifndef _WIN32
// SIGINT Handler
void sigint_handler(int /*s*/)
{
    should_stop = true;
}
#endif

// HTTP Handler for stats
void http_handle(nng_aio *aio)
{
    std::string jsonstr = live_pipeline->getModulesStats().dump(4);

    nng_http_res *res;
    nng_http_res_alloc(&res);
    nng_http_res_copy_data(res, jsonstr.c_str(), jsonstr.size());
    nng_http_res_set_header(res, "Content-Type", "application/json; charset=utf-8");
    nng_aio_set_output(aio, 0, res);
    nng_aio_finish(aio, 0);
}

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        printf("Usage : %s ingestor_config.json\n", argv[0]);
        exit(1);
    }

    std::string ingestor_cfg_path = argv[1];

    // SatDump init
    initLogger();
    initSatdump();

#ifndef _WIN32
    // Setup SIGINT handler
    struct sigaction siginthandler;
    siginthandler.sa_handler = sigint_handler;
    sigemptyset(&siginthandler.sa_mask);
    siginthandler.sa_flags = 0;
    sigaction(SIGINT, &siginthandler, NULL);
#endif

    logger->warn("Keep in mind this ingestor is still WIP! Features such as SDR selection are lacking and coming shortly.");

    // Init thread pool
    ctpl::thread_pool processThreadPool(8);

    // Init SDR Devices
    initSDRs();
    std::vector<std::tuple<std::string, sdr_device_type, uint64_t>> devices = getAllDevices();
    for (std::tuple<std::string, sdr_device_type, uint64_t> dev : devices)
        logger->info(std::get<0>(dev));

    // Settings we're gonna be using
    if (!std::filesystem::exists(ingestor_cfg_path))
    {
        logger->error("Could not find config file " + ingestor_cfg_path + "!");
        exit(1);
    }
    nlohmann::json ingestor_cfg;
    {
        std::ifstream istream(ingestor_cfg_path);
        istream >> ingestor_cfg;
        istream.close();
    }

    sdr_device_type sdr_type = ingestor_cfg.count("sdr_type") > 0 ? getDeviceIDbyIDString(ingestor_cfg["sdr_type"].get<std::string>()) : NONE;
    float samplerate = std::stoi(ingestor_cfg["samplerate"].get<std::string>());
    float frequency = std::stof(ingestor_cfg["frequency"].get<std::string>());
    std::map<std::string, std::string> device_parameters = ingestor_cfg["sdr_settings"].get<std::map<std::string, std::string>>();
    std::string downlink_pipeline = ingestor_cfg["pipeline"].get<std::string>();
    std::string output_folder = ingestor_cfg["output_folder"].get<std::string>();
    std::string http_server_url = "http://" + ingestor_cfg["http_server"].get<std::string>();

    if (sdr_type == NONE)
    {
        logger->warn("SDR Type is invalid / unspecified! Using first device found.");
    }

    // Prepare other parameters
    nlohmann::json parameters;
    parameters["samplerate"] = samplerate;
    parameters["baseband_format"] = "f32";

    // Init the device
    std::string devID = sdr_type == NONE ? getDeviceIDStringByID(devices, 0) : ingestor_cfg["sdr_type"].get<std::string>();
    logger->debug("Device parameters " + devID + ":");
    for (const std::pair<std::string, std::string> param : device_parameters)
        logger->debug("   - " + param.first + " : " + param.second);

    std::shared_ptr<SDRDevice> radio;
    if (sdr_type == NONE)
    {
        radio = getDeviceByID(devices, device_parameters, 0);
    }
    else
    {
        bool found = false;
        for (int i = 0; i < (int)devices.size(); i++)
        {
            std::tuple<std::string, sdr_device_type, uint64_t> &devListing = devices[i];
            if (std::get<1>(devListing) == sdr_type)
            {
                radio = getDeviceByID(devices, device_parameters, i);
                found = true;
            }
        }

        if (!found)
        {
            logger->error("Could not find requested SDR Device!");
            exit(1);
        }
    }
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

    live_pipeline = std::make_shared<LivePipeline>(*it, parameters, output_folder);

    // Start processing
#ifdef _WIN32
    logger->info("Setting process priority to Realtime");
    SetPriorityClass(GetCurrentProcess(), REALTIME_PRIORITY_CLASS);
#endif

    std::shared_ptr<dsp::stream<complex_t>> pipelineStream = std::make_shared<dsp::stream<complex_t>>();
    live_pipeline->start(pipelineStream, processThreadPool);

    // HTTP
    logger->info("Starting HTTP Server...");
    nng_http_server *http_server;
    nng_url *url;
    nng_http_handler *handler;
    nng_url_parse(&url, http_server_url.c_str());
    nng_http_server_hold(&http_server, url);
    nng_http_handler_alloc(&handler, url->u_path, http_handle);
    nng_http_handler_set_method(handler, "GET");
    nng_http_server_add_handler(http_server, handler);
    nng_http_server_start(http_server);
    nng_url_free(url);

    // Loop to pass samples from the SDR into set buffers for the pipeline
    volk::vector<complex_t> sample_buffer_vec;
    int cnt = 0;
    int buf_size = 8192;
    while (!should_stop)
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

            if ((int)sample_buffer_vec.size() < buf_size) // Still too small
                continue;
        }

        std::memcpy(pipelineStream->writeBuf, sample_buffer_vec.data(), buf_size * sizeof(complex_t));
        pipelineStream->swap(buf_size);
        sample_buffer_vec.erase(sample_buffer_vec.begin(), sample_buffer_vec.begin() + buf_size);
    }

    logger->info("Stop pipeline...");
    live_pipeline->stop();

    //logger->info("Stopping SDR...");
    //radio->output_stream->clearReadStop();
    //radio->output_stream->clearWriteStop();
    //radio->stop();

    logger->info("Done! Goodbye");
    exit(0); // Some SDRs leave some threads hanging... Causing it to hang
}
