#include "autotrack.h"
#include "../webserver.h"
#include "logger.h"
#include "common/utils.h"

void AutoTrackApp::setup_webserver()
{
    // If requested, boot up webserver
    if (d_settings.contains("http_server"))
    {
        std::string http_addr = d_settings["http_server"].get<std::string>();

        webserver::handle_callback = [this]()
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

            vfos_mtx.lock();
            if (vfo_list.size() > 0)
            {
                for (auto &vfo : vfo_list)
                {
                    auto &pos = p["vfos"][vfo.id];
                    pos["frequency"] = vfo.freq;
                    if (vfo.pipeline_id != -1)
                    {
                        vfo.live_pipeline->updateModuleStats();
                        pos["live_pipeline"] = vfo.live_pipeline->stats;
                    }
                    if (vfo.file_sink)
                    {
                        pos["recording"]["written_size"] = file_sink->get_written();
                        pos["recording"]["written_raw_size"] = file_sink->get_written_raw();
                    }
                }
            }
            vfos_mtx.unlock();

            if (is_recording)
            {
                p["recording"]["written_size"] = file_sink->get_written();
                p["recording"]["written_raw_size"] = file_sink->get_written_raw();
            }

            return p.dump(4);
        };

        webserver::add_polarplot_handler = true;

        webserver::handle_callback_polarplot = [this]() -> std::vector<uint8_t>
        {
            std::vector<uint8_t> vec = object_tracker.getPolarPlotImg().save_jpeg_mem();
            return vec;
        };

        if (d_parameters.contains("fft_enable"))
            webserver::handle_callback_fft = [this]() -> std::vector<uint8_t>
            {
                if (!web_fft_is_enabled)
                {
                    splitter->set_enabled("fft", true);
                    web_fft_is_enabled = true;
                    logger->trace("Enabling FFT");
                }
                web_last_fft_access = time(nullptr);
                std::vector<uint8_t> vec = fft_plot->drawImg(512, 512).save_jpeg_mem();
                return vec;
            };

        webserver::handle_callback_schedule = [this]() -> std::vector<uint8_t>
        {
            std::vector<uint8_t> vec = auto_scheduler.getScheduleImage(512, getTime()).save_jpeg_mem();
            return vec;
        };

        webserver::handle_callback_html = [this](std::string uri) -> std::string
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

                if (d_parameters.contains("fft_enable"))
                    fft = (std::string) "<h2>FFT</h2><img src=\"fft.jpeg?r=" + std::to_string(cache_buster) + "\" class=\"resp-img\" height=\"600\" width=\"600\" />";

                std::string schedule = (std::string) "<h2>Schedule</h2><img src=\"schedule.jpeg?r=" + std::to_string(cache_buster) + "\" class=\"resp-img\" height=\"600\" width=\"600\" />";

                std::string page = (std::string) "<h2>Device</h2><p>Hardware: <span class=\"fakeinput\">" +
                                   selected_src.name + "</span></p>" +
                                   "<p>Started: <span class=\"fakeinput\">" +
                                   std::string(is_started ? "YES" : "NO") +
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
                                   schedule +
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
}

void AutoTrackApp::stop_webserver()
{
    if (d_settings.contains("http_server"))
        webserver::stop();
}

void AutoTrackApp::web_heartbeat()
{
    if (web_fft_is_enabled)
    {
        if (time(0) - web_last_fft_access > 10)
        {
            splitter->set_enabled("fft", false);
            web_fft_is_enabled = false;
            logger->trace("Shutting down FFT");
        }
    }
}