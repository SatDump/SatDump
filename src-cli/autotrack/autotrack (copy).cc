#include "autotrack.h"

#include <signal.h>
#include <filesystem>
#include "logger.h"
#include "init.h"
#include "core/live_pipeline.h"
#include "common/utils.h"
#include "../webserver.h"

#include "common/dsp_source_sink/dsp_sample_source.h"
#include "common/cli_utils.h"
#include "common/dsp/path/splitter.h"
#include "common/dsp/fft/fft_pan.h"
#include "common/tracking/obj_tracker/object_tracker.h"
#include "common/tracking/scheduler/scheduler.h"
#include "common/tracking/rotator/rotcl_handler.h"
#include "common/widgets/fft_plot.h"

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

    // Init source
    std::shared_ptr<dsp::DSPSampleSource> source_ptr = getSourceFromDescriptor(selected_src);
    source_ptr->open();
    source_ptr->set_frequency(initial_frequency);
    source_ptr->set_samplerate(samplerate);
    source_ptr->set_settings(parameters);

    // Init splitter
    std::unique_ptr<dsp::SplitterBlock> splitter;
    std::unique_ptr<dsp::FFTPanBlock> fft;
    std::unique_ptr<widgets::FFTPlot> fft_plot;
    double last_fft_access = 0;
    bool fft_is_enabled = false;

    // Attempt to start the source and splitter
    try
    {
        splitter = std::make_unique<dsp::SplitterBlock>(source_ptr->output_stream);
        splitter->set_main_enabled(false);
        splitter->add_output("record");
        splitter->add_output("live");

        // Optional FFT
        if (parameters.contains("fft_enable"))
        {
            int fft_size = parameters.contains("fft_size") ? parameters["fft_size"].get<int>() : 512;
            int fft_rate = parameters.contains("fft_rate") ? parameters["fft_rate"].get<int>() : 30;

            splitter->add_output("fft");
            fft = std::make_unique<dsp::FFTPanBlock>(splitter->get_output("fft"));
            fft->set_fft_settings(fft_size, samplerate, fft_rate);
            if (parameters.contains("fft_avgn"))
                fft->avg_num = parameters["fft_avgn"].get<float>();

            fft_plot = std::make_unique<widgets::FFTPlot>(fft->output_stream->writeBuf, fft_size, -150, 150, 40);
            logger->critical("FFT GOOD!");
        }

        if (parameters.contains("fft_enable"))
            fft->start();
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
                auto fun = [pipeline_id, pipeline_output_dir, input_file, pipeline_params](int)
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
    bool source_started = false;

    auto_scheduler.eng_callback = [&](satdump::AutoTrackCfg, satdump::SatellitePass, satdump::TrackedObject obj)
    {
        // logger->critical(obj.norad);
        object_tracker.setObject(object_tracker.TRACKING_SATELLITE, obj.norad);
    };
    auto_scheduler.aos_callback = [&](satdump::AutoTrackCfg, satdump::SatellitePass, satdump::TrackedObject obj)
    {
        object_tracker.setObject(object_tracker.TRACKING_SATELLITE, obj.norad);

        if (autotrack_cfg.vfo_mode)
        {
        }
        else
        {
            if (obj.downlinks[0].live)
                stop_processing();
            if (obj.downlinks[0].record)
                logger->error("Recording Not Implemented Yet!"); // stop_recording();

            if (obj.downlinks[0].live || obj.downlinks[0].record)
            {
                if (!source_started)
                {
                    try
                    {
                        logger->info("Starting source...");
                        source_ptr->start();
                        splitter->input_stream = source_ptr->output_stream;
                        splitter->start();
                        source_started = true;
                    }
                    catch (std::runtime_error &e)
                    {
                        logger->error("%s", e.what());
                    }
                }

                source_ptr->set_frequency(obj.downlinks[0].frequency);
            }

            if (obj.downlinks[0].live)
            {
                pipeline_params = obj.downlinks[0].pipeline_selector->getParameters();
                pipeline_id = obj.downlinks[0].pipeline_selector->pipeline_id;
                start_processing();
            }

            if (obj.downlinks[0].record)
            {
                logger->error("Recording Not Implemented Yet!"); // start_recording();
            }
        }
    };
    auto_scheduler.los_callback = [&](satdump::AutoTrackCfg autotrack_cfg, satdump::SatellitePass, satdump::TrackedObject obj)
    {
        if (autotrack_cfg.vfo_mode)
        {
        }
        else
        {
            if (obj.downlinks[0].record)
                logger->error("Recording Not Implemented Yet!"); // stop_recording();
            if (obj.downlinks[0].live)
                stop_processing();
            if (source_started && autotrack_cfg.stop_sdr_when_idle)
            {
                logger->info("Stopping source...");
                splitter->stop_tmp();
                source_ptr->stop();
                source_started = false;
            }
        }
    };

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
        webserver::add_polarplot_handler = true;
        webserver::handle_callback_polarplot = [&object_tracker]() -> std::vector<uint8_t>
        {
            std::vector<uint8_t> vec = object_tracker.getPolarPlotImg().save_jpeg_mem();
            return vec;
        };
        if (parameters.contains("fft_enable"))
            webserver::handle_callback_fft = [&fft_plot, &splitter, &fft_is_enabled, &last_fft_access]() -> std::vector<uint8_t>
            {
                if (!fft_is_enabled)
                {
                    splitter->set_enabled("fft", true);
                    fft_is_enabled = true;
                    logger->trace("Enabling FFT");
                }
                last_fft_access = time(nullptr);
                std::vector<uint8_t> vec = fft_plot->drawImg(512, 512).save_jpeg_mem();
                return vec;
            };
        webserver::handle_callback_html = [&selected_src, &parameters, &live_pipeline, &object_tracker, &source_ptr, &live_pipeline_mtx, &source_started](std::string uri) -> std::string
        {
            if (uri == "/status")
            {
                live_pipeline_mtx.lock();
                auto status = object_tracker.getStatus();
                std::string rot_engaged;
                std::string rot_tracking;
                std::string aos_in;
                std::string los_in;
                std::string fft;

                time_t cache_buster = time(nullptr);

                if (status["rotator_engaged"].get<bool>() == true)
                    rot_engaged = "<span class=\"fakeinput true\">engaged</span>";
                else
                    rot_engaged = "<span class=\"fakeinput false\">not engaged</span>";

                if (status["rotator_tracking"].get<bool>() == true)
                    rot_tracking = "<span class=\"fakeinput true\">tracking</span>";
                else
                    rot_tracking = "<span class=\"fakeinput false\">not tracking</span>";

                if (status["next_event_is_aos"].get<bool>() == true)
                {
                    aos_in = "(in <span class=\"fakeinput\">" +
                             std::to_string((int)(status["next_event_in"].get<double>())) +
                             "</span> seconds)";
                    los_in = "";
                }
                else
                {
                    los_in = "(in <span class=\"fakeinput\">" +
                             std::to_string((int)(status["next_event_in"].get<double>())) +
                             "</span> seconds)";
                    aos_in = "";
                }

                if (parameters.contains("fft_enable"))
                    fft = (std::string) "<h2>FFT</h2><img src=\"fft.jpeg?r=" + std::to_string(cache_buster) + "\" class=\"resp-img\" height=\"600\" width=\"600\" />";

                std::string page = (std::string) "<h2>Device</h2><p>Hardware: <span class=\"fakeinput\">" +
                                   selected_src.name + "</span></p>" +
                                   "<p>Started: <span class=\"fakeinput\">" +
                                   std::string(source_started ? "YES" : "NO") +
                                   "</span></p>" +
                                   "<p>Sample rate: <span class=\"fakeinput\">" +
                                   std::to_string(source_ptr->get_samplerate() / 1e6) +
                                   "</span> Msps</p>" +
                                   "<p>Frequency: <span class=\"fakeinput\">" +
                                   std::to_string(source_ptr->get_frequency() / 1e6) +
                                   "</span> MHz</p>" +
                                   "<h2>Object Tracker</h2>" +
                                   "<div class=\"image-div\"><img src=\"polarplot.jpeg?r=" + std::to_string(cache_buster) + "\" width=256 height=256/></div>" +
                                   "<p>Next AOS time: <span class=\"fakeinput\">" +
                                   timestamp_to_string(status["next_aos_time"].get<double>()) +
                                   "</span>" +
                                   aos_in +
                                   "</p>" +
                                   "<p>Next LOS time: <span class=\"fakeinput\">" +
                                   timestamp_to_string(status["next_los_time"].get<double>()) +
                                   "</span>" +
                                   los_in +
                                   "</p>" +
                                   "<p>Current object: <span class=\"fakeinput\">" +
                                   status["object_name"].get<std::string>() +
                                   "</span></p>" +
                                   "<p>Current position:<br />" +
                                   "Azimuth <span class=\"fakeinput\">" +
                                   svformat("%.2f", (status["sat_current_pos"]["az"].get<double>())) +
                                   "</span> °<br />Elevation <span class=\"fakeinput\">" +
                                   svformat("%.2f", (status["sat_current_pos"]["el"].get<double>())) +
                                   "</span> °<br /> Range <span class=\"fakeinput\">" +
                                   svformat("%.2f", (status["sat_current_range"].get<double>())) +
                                   "</span> km</p>" +
                                   "<h2>Rotator Control</h2>" +
                                   "<p>Status:" + rot_engaged +
                                   ", <span class=\"fakeinput false\">" +
                                   rot_tracking + "</span></p>" +
                                   "<p>Azimuth: requested <span class=\"fakeinput\">" +
                                   svformat("%.2f", (status["rot_current_req_pos"]["az"].get<double>())) +
                                   "</span> °, actual <span class=\"fakeinput\">" +
                                   svformat("%.2f", (status["rot_current_pos"]["az"].get<double>())) +
                                   "</span> °</p>" +
                                   "<p>Elevation: requested <span class=\"fakeinput\">" +
                                   svformat("%.2f", (status["rot_current_req_pos"]["el"].get<double>())) +
                                   "</span> °, actual <span class=\"fakeinput\">" +
                                   svformat("%.2f", (status["rot_current_pos"]["el"].get<double>())) +
                                   "</span> °</p>" +
                                   fft;

                live_pipeline_mtx.unlock();
                return page;
            }
            else if (uri == "/")
            {
                std::string page = (std::string) "<!DOCTYPE html><html lang=\"EN\"><head>" +
                                   "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">" +
                                   "<meta charset=\"utf-8\"><title>SatDump Status Page</title>" +
                                   "<script type=\"text/javascript\">" +
                                   "function xhr() {\n" +
                                   "var http;\n" +
                                   "if (window.XMLHttpRequest) {\n" +
                                   "http = new XMLHttpRequest();\n" +
                                   "} else {\n" +
                                   "http = new ActiveXObject(\"Microsoft.XMLHTTP\");\n" +
                                   "}" +
                                   "var url = \"/status\";\n" +
                                   "http.open(\"GET\", url, true);\n" +
                                   "http.onreadystatechange = function() { \n" +
                                   "if (http.readyState == 4 && http.status == 200) {\n" +
                                   "document.getElementById('main-content').innerHTML = http.responseText;\n" +
                                   "}\n" +
                                   "}\n" +
                                   "http.setRequestHeader(\"If-Modified-Since\", \"Sat, 1 Jan 2000 00:00:00 GMT\");\n" +
                                   "http.send(null);\n" +
                                   "}\n" +
                                   "window.onload = function() {\n" +
                                   "xhr();\n"
                                   "setInterval(\"xhr()\", 1000)\n" +
                                   "}\n" +
                                   "</script>" +
                                   "<!--[if lt IE 7 ]><style>body{width:600px;}</style><![endif]-->"
                                   "<style>body{background-color:#111;font-family:sans-serif;color:#ddd;" +
                                   "max-width:600px;margin-left:auto;margin-right:auto}h1{text-align:center}" +
                                   "h2{padding:5px;border-radius:5px;background-color:#3e3e43}" +
                                   ".fakeinput{padding:2px;border-radius:1px;background-color:#232526}" +
                                   ".resp-img{max-width:100%;height:auto}img{vertical-align:middle}" +
                                   ".true{color:#0f0}.false{color:red}.image-div{background-color:black;width:256px;height:256px;}</style></head>" +
                                   "<div id=\"main-content\"><h2>Loading...</h2><p>If you see this, your browser does not support JavaScript. <a href=\"/status\">Click here</a> to view the status (you will need to refresh it manually) :)</p></div>" +
                                   "</body></html>";
                return page;
            }
            else
            {
                return "Error 404";
            }
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

        if (fft_is_enabled)
        {
            if (time(0) - last_fft_access > 10)
            {
                splitter->set_enabled("fft", false);
                fft_is_enabled = false;
                logger->trace("Shutting down FFT");
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // Stop cleanly
    if (settings.contains("http_server"))
        webserver::stop();

    stop_processing();
    splitter->input_stream = std::make_shared<dsp::stream<complex_t>>();
    splitter->stop();
    if (parameters.contains("fft_enable"))
        fft->stop();

    if (source_started)
    {
        source_ptr->stop();
        source_started = false;
    }

    source_ptr->close();

    general_thread_pool.stop();
    for (int i = 0; i < general_thread_pool.size(); i++)
    {
        if (general_thread_pool.get_thread(i).joinable())
            general_thread_pool.get_thread(i).join();
    }

    return 0;
}