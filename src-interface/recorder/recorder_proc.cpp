#include "recorder.h"

#include "main_ui.h"
#include "logger.h"
#include "processing.h"

namespace satdump
{
    nlohmann::json RecorderApplication::serialize_config()
    {
        nlohmann::json out;
        out["show_waterfall"] = show_waterfall;
        out["waterfall_ratio"] = waterfall_ratio;
        out["panel_ratio"] = panel_ratio;
        out["fft_size"] = fft_size;
        out["fft_rate"] = fft_rate;
        out["waterfall_rate"] = waterfall_rate;
        out["waterfall_palette"] = waterfall_palettes[selected_waterfall_palette].name;
        out["select_sample_format"] = select_sample_format;
        if (fft_plot && waterfall_plot && fft)
        {
            out["fft_min"] = fft_plot->scale_min;
            out["fft_max"] = fft_plot->scale_max;
            out["fft_avg"] = fft->avg_rate;
        }
        return out;
    }

    void RecorderApplication::deserialize_config(nlohmann::json in)
    {
        show_waterfall = in["show_waterfall"].get<bool>();
        waterfall_ratio = in["waterfall_ratio"].get<float>();
        panel_ratio = in["panel_ratio"].get<float>();
        if (fft_plot && waterfall_plot && fft)
        {
            if (in.contains("fft_min"))
                fft_plot->scale_min = in["fft_min"];
            if (in.contains("fft_max"))
                fft_plot->scale_max = in["fft_max"];
            if (in.contains("fft_avg"))
                fft->avg_rate = in["fft_avg"];
        }
        if (in.contains("fft_size"))
        {
            fft_size = in["fft_size"].get<int>();
            for (int i = 0; i < 4; i++)
                if (fft_sizes_lut[i] == fft_size)
                    selected_fft_size = i;
        }
        if (in.contains("fft_rate"))
            fft_rate = in["fft_rate"];
        if (in.contains("waterfall_rate"))
            waterfall_rate = in["waterfall_rate"];
        if (in.contains("select_sample_format"))
            select_sample_format = in["select_sample_format"];
        if (in.contains("waterfall_palette"))
        {
            std::string name = in["waterfall_palette"].get<std::string>();
            for (int i = 0; i < (int)waterfall_palettes.size(); i++)
                if (waterfall_palettes[i].name == name)
                    selected_waterfall_palette = i;
            waterfall_plot->set_palette(waterfall_palettes[selected_waterfall_palette]);
        }
    }

    void RecorderApplication::start()
    {
        set_frequency(frequency_mhz);
        try
        {
            source_ptr->start();

            current_samplerate = source_ptr->get_samplerate();

            if (current_decimation > 1)
            {
                decim_ptr = std::make_shared<dsp::SmartResamplerBlock<complex_t>>(source_ptr->output_stream, 1, current_decimation);
                decim_ptr->start();
                logger->info("Setting up resampler...");
            }

            fft->set_fft_settings(fft_size, get_samplerate(), fft_rate);
            waterfall_plot->set_rate(fft_rate, waterfall_rate);
            fft_plot->bandwidth = current_samplerate / current_decimation;
            // fft_plot->frequency = frequency_mhz * 1e6;

            splitter->input_stream = current_decimation > 1 ? decim_ptr->output_stream : source_ptr->output_stream;
            splitter->start();
            is_started = true;
            sdr_error = "";
        }
        catch (std::runtime_error &e)
        {
            sdr_error = e.what();
            logger->error(e.what());
        }
    }

    void RecorderApplication::stop()
    {
        splitter->stop_tmp();
        if (current_decimation > 1)
            decim_ptr->stop();
        source_ptr->stop();
        is_started = false;
        config::main_cfg["user"]["recorder_sdr_settings"][sources[sdr_select_id].name] = source_ptr->get_settings();
        config::main_cfg["user"]["recorder_sdr_settings"][sources[sdr_select_id].name]["samplerate"] = source_ptr->get_samplerate();
        config::main_cfg["user"]["recorder_sdr_settings"][sources[sdr_select_id].name]["frequency"] = frequency_mhz * 1e6;
        config::main_cfg["user"]["recorder_sdr_settings"][sources[sdr_select_id].name]["xconverter_frequency"] = xconverter_frequency;
        config::main_cfg["user"]["recorder_sdr_settings"][sources[sdr_select_id].name]["decimation"] = current_decimation;
        config::saveUserConfig();
    }

    void RecorderApplication::try_load_sdr_settings()
    {
        if (config::main_cfg["user"].contains("recorder_sdr_settings"))
        {
            if (config::main_cfg["user"]["recorder_sdr_settings"].contains(sources[sdr_select_id].name))
            {
                auto cfg = config::main_cfg["user"]["recorder_sdr_settings"][sources[sdr_select_id].name];
                source_ptr->set_settings(cfg);
                if (cfg.contains("samplerate"))
                {
                    try
                    {
                        source_ptr->set_samplerate(cfg["samplerate"]);
                    }
                    catch (std::exception &)
                    {
                    }
                }
                if (cfg.contains("frequency"))
                {
                    frequency_mhz = cfg["frequency"].get<uint64_t>() / 1e6;
                    set_frequency(frequency_mhz);
                }
                if (cfg.contains("xconverter_frequency"))
                    xconverter_frequency = cfg["xconverter_frequency"].get<double>();
                else
                    xconverter_frequency = 0;
                if (cfg.contains("decimation"))
                    current_decimation = cfg["decimation"].get<int>();
                else
                    current_decimation = 1;
            }
        }
    }

    void RecorderApplication::set_output_sample_format()
    {
        int type_lut[] = {
            0,
            1,
            2,
            3,
#ifdef BUILD_ZIQ
            4,
            5,
            6,
#endif
#ifdef BUILD_ZIQ2
            7,
            8,
#endif
        };

        int f = type_lut[select_sample_format];

        if (f == 0)
            file_sink->set_output_sample_type(dsp::CF_32);
        else if (f == 1)
            file_sink->set_output_sample_type(dsp::IS_16);
        else if (f == 2)
            file_sink->set_output_sample_type(dsp::IS_8);
        else if (f == 3)
            file_sink->set_output_sample_type(dsp::WAV_16);
#ifdef BUILD_ZIQ
        else if (f == 4)
        {
            file_sink->set_output_sample_type(dsp::ZIQ);
            ziq_bit_depth = 8;
        }
        else if (f == 5)
        {
            file_sink->set_output_sample_type(dsp::ZIQ);
            ziq_bit_depth = 16;
        }
        else if (f == 6)
        {
            file_sink->set_output_sample_type(dsp::ZIQ);
            ziq_bit_depth = 32;
        }
#endif
#ifdef BUILD_ZIQ2
        else if (f == 7)
        {
            file_sink->set_output_sample_type(dsp::ZIQ2);
            ziq_bit_depth = 8;
        }
        else if (f == 8)
        {
            file_sink->set_output_sample_type(dsp::ZIQ2);
            ziq_bit_depth = 16;
        }
#endif
    }

    void RecorderApplication::start_processing()
    {
        if (pipeline_selector.outputdirselect.isValid() || automated_live_output_dir)
        {
            logger->trace("Start pipeline...");
            pipeline_params = pipeline_selector.getParameters();
            pipeline_params["samplerate"] = get_samplerate();
            pipeline_params["baseband_format"] = "f32";
            pipeline_params["buffer_size"] = dsp::STREAM_BUFFER_SIZE; // This is required, as we WILL go over the (usually) default 8192 size
            pipeline_params["start_timestamp"] = (double)time(0);     // Some pipelines need this

            if (automated_live_output_dir)
            {
                const time_t timevalue = time(0);
                std::tm *timeReadable = gmtime(&timevalue);
                std::string timestamp = std::to_string(timeReadable->tm_year + 1900) + "-" +
                                        (timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1)) + "-" +
                                        (timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0" + std::to_string(timeReadable->tm_mday)) + "_" +
                                        (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) + "-" +
                                        (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min));
                pipeline_output_dir = config::main_cfg["satdump_directories"]["live_processing_path"]["value"].get<std::string>() + "/" +
                                      timestamp + "_" +
                                      pipelines[pipeline_selector.pipeline_id].name + "_" +
                                      std::to_string(long(source_ptr->d_frequency / 1e6)) + "Mhz";
                std::filesystem::create_directories(pipeline_output_dir);
                logger->info("Generated folder name : " + pipeline_output_dir);
            }
            else
            {
                pipeline_output_dir = pipeline_selector.outputdirselect.getPath();
            }

            try
            {
                live_pipeline = std::make_unique<LivePipeline>(pipelines[pipeline_selector.pipeline_id], pipeline_params, pipeline_output_dir);
                splitter->reset_output("live");
                live_pipeline->start(splitter->get_output("live"), ui_thread_pool);
                splitter->set_enabled("live", true);

                is_processing = true;
            }
            catch (std::runtime_error &e)
            {
                error = e.what();
                logger->error(e.what());
            }
        }
        else
        {
            error = "Please select a valid output directory!";
        }
    }

    void RecorderApplication::stop_processing()
    {
        if (is_processing)
        {
            logger->trace("Stop pipeline...");
            is_processing = false;
            splitter->set_enabled("live", false);
            live_pipeline->stop();

            if (config::main_cfg["user_interface"]["finish_processing_after_live"]["value"].get<bool>() && live_pipeline->getOutputFiles().size() > 0)
            {
                Pipeline pipeline = pipelines[pipeline_selector.pipeline_id];
                std::string input_file = live_pipeline->getOutputFiles()[0];
                int start_level = pipeline.live_cfg.normal_live[pipeline.live_cfg.normal_live.size() - 1].first;
                std::string input_level = pipeline.steps[start_level].level_name;
                ui_thread_pool.push([=](int)
                                    { processing::process(pipeline.name, input_level, input_file, pipeline_output_dir, pipeline_params); });
            }

            live_pipeline.reset();
        }
    }

    void RecorderApplication::start_recording()
    {
        splitter->set_enabled("record", true);

        double timeValue_precise = 0;
        {
            auto time = std::chrono::system_clock::now();
            auto since_epoch = time.time_since_epoch();
            auto millis = std::chrono::duration_cast<std::chrono::milliseconds>(since_epoch);
            timeValue_precise = millis.count() / 1e3;
        }

        const time_t timevalue = timeValue_precise;
        std::tm *timeReadable = gmtime(&timevalue);
        std::string timestamp = std::to_string(timeReadable->tm_year + 1900) + "-" +
                                (timeReadable->tm_mon + 1 > 9 ? std::to_string(timeReadable->tm_mon + 1) : "0" + std::to_string(timeReadable->tm_mon + 1)) + "-" +
                                (timeReadable->tm_mday > 9 ? std::to_string(timeReadable->tm_mday) : "0" + std::to_string(timeReadable->tm_mday)) + "_" +
                                (timeReadable->tm_hour > 9 ? std::to_string(timeReadable->tm_hour) : "0" + std::to_string(timeReadable->tm_hour)) + "-" +
                                (timeReadable->tm_min > 9 ? std::to_string(timeReadable->tm_min) : "0" + std::to_string(timeReadable->tm_min)) + "-" +
                                (timeReadable->tm_sec > 9 ? std::to_string(timeReadable->tm_sec) : "0" + std::to_string(timeReadable->tm_sec));

        if (config::main_cfg["user_interface"]["recorder_baseband_filename_millis_precision"]["value"].get<bool>())
        {
            std::ostringstream ss;

            double ms_val = fmod(timeValue_precise, 1.0) * 1e3;
            ss << "-" << std::fixed << std::setprecision(0) << std::setw(3) << std::setfill('0') << ms_val;
            timestamp += ss.str();
        }

        std::string filename = config::main_cfg["satdump_directories"]["recording_path"]["value"].get<std::string>() +
                               "/" + timestamp + "_" + std::to_string(get_samplerate()) + "SPS_" +
                               std::to_string(long(frequency_mhz * 1e6)) + "Hz";

        recorder_filename = file_sink->start_recording(filename, get_samplerate(), ziq_bit_depth);

        logger->info("Recording to " + recorder_filename);

        is_recording = true;
    }

    void RecorderApplication::stop_recording()
    {
        if (is_recording)
        {
            file_sink->stop_recording();
            splitter->set_enabled("record", false);
            recorder_filename = "";
            is_recording = false;
        }
    }

    void RecorderApplication::try_init_tracking_widget()
    {
        if (tracking_widget == nullptr)
        {
            tracking_widget = new TrackingWidget();

            tracking_widget->aos_callback = [this](tracking::SatellitePass, tracking::TrackedObject obj)
            {
                if (obj.live)
                    stop_processing();
                if (obj.record)
                    stop_recording();

                if (obj.live || obj.record)
                {
                    frequency_mhz = obj.frequency;
                    if (is_started)
                        set_frequency(frequency_mhz);
                    else
                        start();

                    // Catch situations where source could not start
                    if (!is_started)
                    {
                        logger->error("Could not start recorder/processor since the source could not be started!");
                        return;
                    }
                }

                if (obj.live)
                {
                    pipeline_selector.select_pipeline(pipelines[obj.pipeline_selector->pipeline_id].name);
                    pipeline_selector.setParameters(obj.pipeline_selector->getParameters());
                    start_processing();
                }

                if (obj.record)
                {
                    start_recording();
                }
            };

            tracking_widget->los_callback = [this](tracking::SatellitePass, tracking::TrackedObject obj)
            {
                if (obj.record)
                    stop_recording();
                if (obj.live)
                    stop_processing();
            };
        }
    }
}