#include "live.h"
#include "common/cli_utils.h"
#include "common/dsp/fft/fft_pan.h"
#include "common/dsp/path/splitter.h"
#include "common/dsp/path/splitter_vfo.h"
#include "common/dsp_source_sink/dsp_sample_source.h"
#include "init.h"
#include "logger.h"
#include "pipeline/live_pipeline.h"
#include "webserver.h"
#include <filesystem>
#include <signal.h>

// Catch CTRL+C to exit live properly!
bool live_should_exit = false;
void sig_handler_live(int signo)
{
    if (signo == SIGINT || signo == SIGTERM)
        live_should_exit = true;
}

// TODOREWORK the timeout and stats are partially broken here now!

int main_live(int argc, char *argv[])
{
    if (argc < 5) // Check overall command
    {
        logger->error("Usage : " + std::string(argv[0]) + " live [pipeline_id] [output_file_or_directory] [additional options as required]");
        logger->error("Extra options (examples. Any parameter used in modules or sources can be used here) :");
        logger->error(" --samplerate [baseband_samplerate] --baseband_format [f32/s16/s8/u8] --dc_block --iq_swap");
        logger->error(" --source [airspy/rtlsdr/etc] --gain 20 --bias");
        logger->error("As well as --timeout in seconds");
        logger->error("Sample command :");
        logger->error("./satdump live metop_ahrpt metop_output_directory --source airspy --samplerate 6e6 --frequency 1701.3e6 --general_gain 18 --bias --timeout 780");
        return 1;
    }

    std::string downlink_pipeline = argv[2];
    std::string output_file = argv[3];

    // Parse flags
    nlohmann::json parameters = parse_common_flags(argc - 4, &argv[4], {{"source_id", typeid(std::string)}});

    // Init SatDump
    satdump::tle_file_override = parameters.contains("tle_override") ? parameters["tle_override"].get<std::string>() : "";
    satdump::initSatdump();
    completeLoggerInit();

    if (parameters.contains("client"))
    {
        logger->info("Starting in client mode!");

        // Create output dir
        if (!std::filesystem::exists(output_file))
            std::filesystem::create_directories(output_file);

        // Get pipeline
        std::optional<satdump::pipeline::Pipeline> pipeline = satdump::pipeline::getPipelineFromID(downlink_pipeline);

        if (!pipeline.has_value())
        {
            logger->critical("Pipeline " + downlink_pipeline + " does not exist!");
            return 1;
        }

        // Init pipeline
        std::unique_ptr<satdump::pipeline::LivePipeline> live_pipeline = std::make_unique<satdump::pipeline::LivePipeline>(pipeline.value(), parameters, output_file);

        ctpl::thread_pool live_thread_pool(8);

        // Attempt to start the source and pipeline
        try
        {
            live_pipeline->start_client(live_thread_pool);
        }
        catch (std::exception &e)
        {
            logger->error("Fatal error running pipeline/device : " + std::string(e.what()));
            return 1;
        }

        // If requested, boot up webserver
        if (parameters.contains("http_server"))
        {
            std::string http_addr = parameters["http_server"].get<std::string>();
            webserver::handle_callback = [&live_pipeline]() { return live_pipeline->getModulesStats().dump(4); };
            logger->info("Start webserver on %s", http_addr.c_str());
            webserver::start(http_addr);
        }

        // Attach signal
        signal(SIGINT, sig_handler_live);
        signal(SIGTERM, sig_handler_live);

        // Now, we wait
        while (1)
        {
            if (live_should_exit)
            {
                logger->warn("Signal Received. Stopping.");
                break;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        // Stop cleanly
        live_pipeline->stop();

        if (parameters.contains("http_server"))
            webserver::stop();
    }
    else
    {
        uint64_t samplerate;
        uint64_t frequency;
        uint64_t timeout;
        std::string handler_id;
        std::string hdl_dev_id;

        try
        {
            samplerate = parameters["samplerate"].get<uint64_t>();
            frequency = parameters["frequency"].get<uint64_t>();
            timeout = parameters.contains("timeout") ? parameters["timeout"].get<uint64_t>() : 0;
            handler_id = parameters["source"].get<std::string>();
            if (parameters.contains("source_id"))
                hdl_dev_id = parameters["source_id"].get<std::string>();
        }
        catch (std::exception &e)
        {
            logger->error("Error parsing arguments! %s", e.what());
            return 1;
        }

        // Create output dir
        if (!std::filesystem::exists(output_file))
            std::filesystem::create_directories(output_file);

        // Get all sources
        dsp::registerAllSources();
        std::vector<dsp::SourceDescriptor> source_tr = dsp::getAllAvailableSources();
        dsp::SourceDescriptor selected_src;

        for (dsp::SourceDescriptor src : source_tr)
            logger->debug("Device " + src.name);

        // Try to find it and check it's usable
        bool src_found = false;
        for (dsp::SourceDescriptor src : source_tr)
        {
            if (handler_id == src.source_type)
            {
                if (parameters.contains("source_id"))
                {
                    if (hdl_dev_id == src.unique_id)
                    {
                        selected_src = src;
                        src_found = true;
                    }
                }
                else
                {
                    selected_src = src;
                    src_found = true;
                }
            }
        }

        if (!src_found)
        {
            logger->error("Could not find a handler for source type : %s!", handler_id.c_str());
            return 1;
        }

        // Init source
        std::shared_ptr<dsp::DSPSampleSource> source_ptr = getSourceFromDescriptor(selected_src);
        source_ptr->open();
        source_ptr->set_frequency(frequency);
        source_ptr->set_samplerate(samplerate);
        source_ptr->set_settings(parameters);

        if (parameters.contains("multi_vfo"))
        {
            logger->info("Starting in multi VFO mode!");
            logger->critical("This is still considered WIP!");

            nlohmann::json multi_cfg = loadJsonFile(parameters["multi_vfo"].get<std::string>());

            std::unique_ptr<dsp::VFOSplitterBlock> splitter_vfo;

            // Attempt to start the source and pipeline
            try
            {
                source_ptr->start();
                splitter_vfo = std::make_unique<dsp::VFOSplitterBlock>(source_ptr->output_stream);
                splitter_vfo->set_main_enabled(false);
                splitter_vfo->start();
            }
            catch (std::exception &e)
            {
                logger->error("Fatal error running pipeline/device : " + std::string(e.what()));
                return 1;
            }

            std::map<std::string, std::shared_ptr<satdump::pipeline::LivePipeline>> all_pipelines;
            ctpl::thread_pool live_thread_pool(128);

            for (auto cfg : multi_cfg.items())
            {
                double vfrequency = cfg.value()["frequency"];
                std::string vpipeline = cfg.value()["pipeline"];
                nlohmann::json vparams = cfg.value()["parameters"];

                double final_shift = double(frequency) - vfrequency;

                if (abs(final_shift) > (samplerate / 2))
                {
                    logger->error("Frequency shift for VFO %s is outside of samplerate range!", cfg.key().c_str());
                    exit(1);
                }

                std::optional<satdump::pipeline::Pipeline> pipeline = satdump::pipeline::getPipelineFromID(vpipeline);
                vparams["baseband_format"] = "cf32";
                vparams["buffer_size"] = dsp::STREAM_BUFFER_SIZE; // This is required, as we WILL go over the (usually) default 8192 size
                vparams["start_timestamp"] = (double)time(0);     // Some pipelines need this
                vparams["samplerate"] = samplerate;

                std::string path = output_file + "/" + cfg.key();
                if (!std::filesystem::exists(path))
                    std::filesystem::create_directories(path);

                std::shared_ptr<satdump::pipeline::LivePipeline> live_pipeline = std::make_shared<satdump::pipeline::LivePipeline>(pipeline.value(), vparams, path);

                bool server_mode = vparams.contains("server_address") || vparams.contains("server_port");

                splitter_vfo->add_vfo(cfg.key(), samplerate, final_shift);
                live_pipeline->start(splitter_vfo->get_vfo_output(cfg.key()), live_thread_pool, server_mode);
                splitter_vfo->set_vfo_enabled(cfg.key(), true);

                all_pipelines.emplace(cfg.key(), live_pipeline);
                logger->info("Added VFO for " + cfg.key() + " at %f", final_shift);
            }

            // If requested, boot up webserver
            if (parameters.contains("http_server"))
            {
                std::string http_addr = parameters["http_server"].get<std::string>();
                // if (!webserver_already_set)
                webserver::handle_callback = [&all_pipelines]()
                {
                    nlohmann::json stats;
                    for (auto &e : all_pipelines)
                    {
                        stats[e.first] = e.second->getModulesStats();
                    }
                    return stats.dump(4);
                };
                logger->info("Start webserver on %s", http_addr.c_str());
                webserver::start(http_addr);
            }

            // Attach signal
            signal(SIGINT, sig_handler_live);
            signal(SIGTERM, sig_handler_live);

            // Now, we wait
            uint64_t start_time = time(0);
            while (1)
            {
                if (timeout > 0)
                {
                    uint64_t elapsed_time = time(0) - start_time;
                    if (elapsed_time >= timeout)
                    {
                        logger->warn("Timeout is over! (%ds >= %ds) Stopping.", elapsed_time, timeout);
                        break;
                    }

                    // live_pipeline->stats["timeout_left"] = timeout - elapsed_time;
                }

                if (live_should_exit)
                {
                    logger->warn("Signal Received. Stopping.");
                    break;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            if (parameters.contains("http_server"))
                webserver::stop();

            // Stop cleanly
            source_ptr->stop();
            splitter_vfo->stop();
            for (auto &e : all_pipelines)
            {
                e.second->stop();
                splitter_vfo->del_vfo(e.first);
                logger->info("Stopped VFO " + e.first);
            }
        }
        else
        {
            // Get pipeline
            std::optional<satdump::pipeline::Pipeline> pipeline = satdump::pipeline::getPipelineFromID(downlink_pipeline);

            if (!pipeline.has_value())
            {
                logger->critical("Pipeline " + downlink_pipeline + " does not exist!");
                return 1;
            }

            // Init pipeline
            parameters["baseband_format"] = "cf32";
            parameters["buffer_size"] = dsp::STREAM_BUFFER_SIZE; // This is required, as we WILL go over the (usually) default 8192 size
            parameters["start_timestamp"] = (double)time(0);     // Some pipelines need this
            std::unique_ptr<satdump::pipeline::LivePipeline> live_pipeline = std::make_unique<satdump::pipeline::LivePipeline>(pipeline.value(), parameters, output_file);

            ctpl::thread_pool live_thread_pool(8);

            bool server_mode = parameters.contains("server_address") || parameters.contains("server_port");

            std::unique_ptr<dsp::SplitterBlock> splitter;
            std::unique_ptr<dsp::FFTPanBlock> fft;
            bool webserver_already_set = false;

            nlohmann::json live_pipeline_stats;

            // Attempt to start the source and pipeline
            try
            {
                source_ptr->start();

                std::shared_ptr<dsp::stream<complex_t>> final_stream = source_ptr->output_stream;

                // Optional FFT
                if (parameters.contains("fft_enable"))
                {
                    int fft_size = parameters.contains("fft_size") ? parameters["fft_size"].get<int>() : 512;
                    int fft_rate = parameters.contains("fft_rate") ? parameters["fft_rate"].get<int>() : 30;

                    splitter = std::make_unique<dsp::SplitterBlock>(source_ptr->output_stream);
                    splitter->add_output("fft");
                    splitter->set_enabled("fft", true);
                    final_stream = splitter->output_stream;
                    fft = std::make_unique<dsp::FFTPanBlock>(splitter->get_output("fft"));
                    fft->set_fft_settings(fft_size, samplerate, fft_rate);
                    if (parameters.contains("fft_avgn"))
                        fft->avg_num = parameters["fft_avgn"].get<float>();
                    splitter->start();
                    fft->start();

                    webserver::handle_callback = [&live_pipeline, &fft, fft_size, &live_pipeline_stats]()
                    {
                        live_pipeline_stats = live_pipeline->getModulesStats();
                        for (int i = 0; i < fft_size; i++)
                            live_pipeline_stats["fft_values"][i] = fft->output_stream->writeBuf[i];
                        return live_pipeline_stats.dump(4);
                    };

                    webserver_already_set = true;
                }

                live_pipeline->start(final_stream, live_thread_pool, server_mode);
            }
            catch (std::exception &e)
            {
                logger->error("Fatal error running pipeline/device : " + std::string(e.what()));
                return 1;
            }

            // If requested, boot up webserver
            if (parameters.contains("http_server"))
            {
                std::string http_addr = parameters["http_server"].get<std::string>();
                if (!webserver_already_set)
                    webserver::handle_callback = [&live_pipeline]() { return live_pipeline->getModulesStats().dump(4); };
                logger->info("Start webserver on %s", http_addr.c_str());
                webserver::start(http_addr);
            }

            // Attach signal
            signal(SIGINT, sig_handler_live);
            signal(SIGTERM, sig_handler_live);

            // Now, we wait
            uint64_t start_time = time(0);
            while (1)
            {
                if (timeout > 0)
                {
                    uint64_t elapsed_time = time(0) - start_time;
                    if (elapsed_time >= timeout)
                    {
                        logger->warn("Timeout is over! (%ds >= %ds) Stopping.", elapsed_time, timeout);
                        break;
                    }

                    live_pipeline_stats["timeout_left"] = timeout - elapsed_time;
                }

                if (live_should_exit)
                {
                    logger->warn("Signal Received. Stopping.");
                    break;
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            // Stop cleanly
            source_ptr->stop();
            if (parameters.contains("fft_enable"))
            {
                splitter->stop();
                fft->stop();
            }
            live_pipeline->stop();

            if ((parameters.contains("finish_processing") ? parameters["finish_processing"].get<bool>() : false) && !server_mode)
            {
                if (live_pipeline->getOutputFile().size() > 0 || std::filesystem::exists(output_file + "/dataset.json"))
                {
                    std::optional<satdump::pipeline::Pipeline> pipeline = satdump::pipeline::getPipelineFromID(downlink_pipeline);
                    std::string input_file = live_pipeline->getOutputFile().size() > 0 ? live_pipeline->getOutputFile() : (output_file + "/dataset.json");
                    int start_level = pipeline->live_cfg.normal_live[pipeline->live_cfg.normal_live.size() - 1];
                    std::string input_level = pipeline->steps[start_level].level;

                    logger->critical(input_level);

                    try
                    {
                        pipeline.value().run(input_file, output_file, parameters, input_level);
                    }
                    catch (std::exception &e)
                    {
                        logger->error("Fatal error running pipeline : " + std::string(e.what()));
                    }
                }
            }

            if (parameters.contains("http_server"))
                webserver::stop();
        }
    }

    return 0;
}