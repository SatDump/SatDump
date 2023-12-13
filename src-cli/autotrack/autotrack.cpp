#include "autotrack.h"
#include "common/dsp_source_sink/dsp_sample_source.h"
#include "core/live_pipeline.h"
#include <signal.h>
#include "logger.h"
#include "init.h"
#include "common/cli_utils.h"
#include <filesystem>
#include "common/dsp/path/splitter.h"
#include "common/dsp/fft/fft_pan.h"
#include "../webserver.h"

#include "common/tracking/obj_tracker/object_tracker.h"
#include "common/tracking/scheduler/scheduler.h"

#include "common/tracking/rotator/rotcl_handler.h"

// Catch CTRL+C to exit live properly!
bool autotrack_should_exit = false;
void sig_handler_autotrack(int signo)
{
    if (signo == SIGINT || signo == SIGTERM)
        autotrack_should_exit = true;
}

int main_autotrack(int argc, char *argv[])
{
    if (argc < 3) // Check overall command
    {
        logger->error("Usage : " + std::string(argv[0]) + " autotrack autotrack_config.json");
        logger->error("Sample command :");
        logger->error("./satdump autotrack autotrack_config.json");
        return 1;
    }

    logger->error("CLI AutoTrack is still WIP!");

    nlohmann::json settings = loadJsonFile(argv[2]);
    nlohmann::json parameters = settings["parameters"];
    std::string output_folder = settings["output_folder"];

    // Init SatDump
    satdump::initSatdump();
    completeLoggerInit();

    uint64_t samplerate;
    uint64_t initial_frequency;
    std::string handler_id;
    uint64_t hdl_dev_id = 0;

    try
    {
        samplerate = parameters["samplerate"].get<uint64_t>();
        initial_frequency = parameters["initial_frequency"].get<uint64_t>();
        handler_id = parameters["source"].get<std::string>();
        if (parameters.contains("source_id"))
            hdl_dev_id = parameters["source_id"].get<uint64_t>();
    }
    catch (std::exception &e)
    {
        logger->error("Error parsing arguments! %s", e.what());
        return 1;
    }

    // Create output dir
    if (!std::filesystem::exists(output_folder))
        std::filesystem::create_directories(output_folder);

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
#ifdef _WIN32 // Windows being cursed. TODO investigate further? It's uint64_t everywhere come on!
                char cmp_buff1[100];
                char cmp_buff2[100];

                snprintf(cmp_buff1, sizeof(cmp_buff1), "%d", hdl_dev_id);
                std::string cmp1 = cmp_buff1;
                snprintf(cmp_buff2, sizeof(cmp_buff2), "%d", src.unique_id);
                std::string cmp2 = cmp_buff2;
                if (cmp1 == cmp2)
#else
                if (hdl_dev_id == src.unique_id)
#endif
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
    source_ptr->set_frequency(initial_frequency);
    source_ptr->set_samplerate(samplerate);
    source_ptr->set_settings(parameters);

    // Init splitter
    std::unique_ptr<dsp::SplitterBlock> splitter;

    // Attempt to start the source and splitter
    try
    {
        source_ptr->start();
        splitter = std::make_unique<dsp::SplitterBlock>(source_ptr->output_stream);
        splitter->set_main_enabled(false);
        splitter->add_output("record");
        splitter->add_output("live");
        splitter->start();
    }
    catch (std::exception &e)
    {
        logger->error("Fatal error running device : " + std::string(e.what()));
        return 1;
    }

    // Live pipeline stuff
    std::mutex live_pipeline_mtx;
    std::unique_ptr<satdump::LivePipeline> live_pipeline;

    std::string pipeline_output_dir;
    ctpl::thread_pool general_thread_pool(8);
    nlohmann::json pipeline_params;
    int pipeline_id = 0;

    bool is_processing = false;
    auto start_processing = [&]()
    {
        live_pipeline_mtx.lock();
        logger->trace("Start pipeline...");
        pipeline_params["samplerate"] = source_ptr->get_samplerate();
        pipeline_params["baseband_format"] = "f32";
        pipeline_params["buffer_size"] = dsp::STREAM_BUFFER_SIZE; // This is required, as we WILL go over the (usually) default 8192 size
        pipeline_params["start_timestamp"] = (double)time(0);     // Some pipelines need this

        {
            const time_t timevalue = time(0);
            std::tm *timeReadable = gmtime(&timevalue);
            std::string timestamp = std::to_string(timeReadable->tm_year + 1900) + "-" +
                                    (timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1)) + "-" +
                                    (timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0" + std::to_string(timeReadable->tm_mday)) + "_" +
                                    (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) + "-" +
                                    (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min));
            pipeline_output_dir = output_folder + "/" +
                                  timestamp + "_" +
                                  satdump::pipelines[pipeline_id].name + "_" +
                                  std::to_string(long(source_ptr->d_frequency / 1e6)) + "Mhz";
            std::filesystem::create_directories(pipeline_output_dir);
            logger->info("Generated folder name : " + pipeline_output_dir);
        }

        try
        {
            live_pipeline = std::make_unique<satdump::LivePipeline>(satdump::pipelines[pipeline_id], pipeline_params, pipeline_output_dir);
            splitter->reset_output("live");
            live_pipeline->start(splitter->get_output("live"), general_thread_pool);
            splitter->set_enabled("live", true);

            is_processing = true;
        }
        catch (std::runtime_error &e)
        {
            logger->error(e.what());
        }
        live_pipeline_mtx.unlock();
    };

    auto stop_processing = [&]()
    {
        if (is_processing)
        {
            live_pipeline_mtx.lock();
            logger->trace("Stop pipeline...");
            is_processing = false;
            splitter->set_enabled("live", false);
            live_pipeline->stop();

            if (settings.contains("finish_processing") && settings["finish_processing"].get<bool>() && live_pipeline->getOutputFiles().size() > 0)
            {
                std::string input_file = live_pipeline->getOutputFiles()[0];
                auto fun = [pipeline_id, pipeline_output_dir, &input_file, pipeline_params](int)
                {
                    satdump::Pipeline pipeline = satdump::pipelines[pipeline_id];
                    int start_level = pipeline.live_cfg.normal_live[pipeline.live_cfg.normal_live.size() - 1].first;
                    std::string input_level = pipeline.steps[start_level].level_name;
                    pipeline.run(input_file, pipeline_output_dir, pipeline_params, input_level);
                };
                general_thread_pool.push(fun);
            }

            live_pipeline.reset();
            live_pipeline_mtx.unlock();
        }
    };

    // Init object tracker & scheduler
    satdump::ObjectTracker object_tracker(false); // TODO
    satdump::AutoTrackScheduler auto_scheduler;

    double qth_lon = settings["qth"]["lon"];
    double qth_lat = settings["qth"]["lat"];
    double qth_alt = settings["qth"]["alt"];

    logger->trace("Using QTH %f %f Alt %f", qth_lon, qth_lat, qth_alt);

    // Init Obj Tracker
    object_tracker.setQTH(qth_lon, qth_lat, qth_alt);
    // object_tracker.setRotator(rotator_handler);
    // object_tracker.setObject(object_tracker.TRACKING_SATELLITE, 25338);

    // Init scheduler
    auto_scheduler.eng_callback = [&](satdump::SatellitePass, satdump::TrackedObject obj)
    {
        // logger->critical(obj.norad);
        object_tracker.setObject(object_tracker.TRACKING_SATELLITE, obj.norad);
    };
    auto_scheduler.aos_callback = [&](satdump::SatellitePass pass, satdump::TrackedObject obj)
    {
        object_tracker.setObject(object_tracker.TRACKING_SATELLITE, obj.norad);

        if (obj.live)
            stop_processing();
        if (obj.record)
            logger->error("Recording Not Implemented Yet!"); // stop_recording();

        if (obj.live || obj.record)
        {
            source_ptr->set_frequency(obj.frequency * 1e6);
        }

        if (obj.live)
        {
            pipeline_params = obj.pipeline_selector->getParameters();
            pipeline_id = obj.pipeline_selector->pipeline_id;
            start_processing();
        }

        if (obj.record)
        {
            logger->error("Recording Not Implemented Yet!"); // start_recording();
        }
    };
    auto_scheduler.los_callback = [&](satdump::SatellitePass pass, satdump::TrackedObject obj)
    {
        if (obj.record)
            logger->error("Recording Not Implemented Yet!"); // stop_recording();
        if (obj.live)
            stop_processing();
    };

    auto_scheduler.setQTH(qth_lon, qth_lat, qth_alt);

    // Other config for the tracker and scheduler
    std::vector<satdump::TrackedObject> enabled_satellites = settings["tracked_objects"];
    nlohmann::json rotator_algo_cfg;
    if (settings["tracking"].contains("rotator_algo"))
        rotator_algo_cfg = settings["tracking"]["rotator_algo"];
    int autotrack_min_elevation = getValueOrDefault<int>(settings["tracking"]["min_elevation"], 0);

    auto_scheduler.setTracked(enabled_satellites);
    object_tracker.setRotatorConfig(rotator_algo_cfg);
    auto_scheduler.setMinElevation(autotrack_min_elevation);

    // Rotator
    std::shared_ptr<rotator::RotatorHandler> rotator_handler;

    rotator_handler = std::make_shared<rotator::RotctlHandler>();

    if (rotator_handler)
    {
        try
        {
            rotator_handler->set_settings(settings["tracking"]["rotator_cfg"]);
        }
        catch (std::exception &e)
        {
        }

        rotator_handler->connect();
        // rotator_handler->set_pos(0, 0);
        object_tracker.setRotator(rotator_handler);
        // object_tracker.setRotatorReqPos(0, 0);
        object_tracker.setRotatorEngaged(true);
        object_tracker.setRotatorTracking(true);
    }

    // Finally, start
    auto_scheduler.start();
    auto_scheduler.setEngaged(true, getTime());

    // If requested, boot up webserver
    if (settings.contains("http_server"))
    {
        std::string http_addr = settings["http_server"].get<std::string>();
        webserver::handle_callback = [&live_pipeline, &object_tracker, &source_ptr, &live_pipeline_mtx]()
        {
            nlohmann::json p;
            live_pipeline_mtx.lock();
            if (live_pipeline)
            {
                live_pipeline->updateModuleStats();
                p["live_pipeline"] = live_pipeline->stats;
            }
            live_pipeline_mtx.unlock();
            p["object_tracker"] = object_tracker.getStatus();
            p["frequency"] = source_ptr->get_frequency();
            return p.dump(4);
        };
        webserver::handle_callback_html = []()
        {
            return "There might be a webpage coming later :-). For now use /api";
        };
        logger->info("Start webserver on %s", http_addr.c_str());
        webserver::start(http_addr);
    }

    // Attach signal
    signal(SIGINT, sig_handler_autotrack);
    signal(SIGTERM, sig_handler_autotrack);

    logger->info("Setup Done!");

    // Now, we wait
    while (1)
    {
        if (autotrack_should_exit)
        {
            logger->warn("Signal Received. Stopping.");
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    stop_processing();

    // Stop cleanly
    source_ptr->stop();
    splitter->stop();
    source_ptr->close();

    if (parameters.contains("http_server"))
        webserver::stop();

    general_thread_pool.stop();
    for (int i = 0; i < general_thread_pool.size(); i++)
    {
        if (general_thread_pool.get_thread(i).joinable())
            general_thread_pool.get_thread(i).join();
    }

    return 0;
}