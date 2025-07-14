#include "explorer/processing/processing.h"
#include "recorder.h"

#include "logger.h"
#include "main_ui.h"
#include "utils/time.h"

#ifndef _MSC_VER
#include <sys/statvfs.h>
#else
#include <Windows.h>
#endif

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
        out["baseband_type"] = (std::string)baseband_format;
        if (fft_plot && waterfall_plot && fft)
        {
            out["fft_min"] = fft_plot->scale_min;
            out["fft_max"] = fft_plot->scale_max;
            out["fft_avgn"] = fft->avg_num;
        }

#if defined(BUILD_ZIQ) || defined(BUILD_ZIQ2)
        out["ziq_depth"] = baseband_format.ziq_depth;
#endif
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
            if (in.contains("fft_avgn"))
                fft->avg_num = in["fft_avgn"];
        }
        if (in.contains("fft_size"))
        {
            fft_size = in["fft_size"].get<int>();
            for (int i = 0; i < (int)fft_sizes_lut.size(); i++)
                if (fft_sizes_lut[i] == fft_size)
                    selected_fft_size = i;
        }
        if (in.contains("fft_rate"))
            fft_rate = in["fft_rate"];
        if (in.contains("waterfall_rate"))
            waterfall_rate = in["waterfall_rate"];
        if (in.contains("baseband_type"))
            baseband_format = in["baseband_type"].get<std::string>();
        if (in.contains("waterfall_palette"))
        {
            std::string name = in["waterfall_palette"].get<std::string>();
            for (int i = 0; i < (int)waterfall_palettes.size(); i++)
                if (waterfall_palettes[i].name == name)
                    selected_waterfall_palette = i;
            waterfall_plot->set_palette(waterfall_palettes[selected_waterfall_palette]);
        }
#if defined(BUILD_ZIQ) || defined(BUILD_ZIQ2)
        if (in.contains("ziq_depth"))
            baseband_format.ziq_depth = in["ziq_depth"];
#endif
    }

    void RecorderApplication::start()
    {
        if (is_started)
            return;

        set_frequency(frequency_hz);

        try
        {
            current_samplerate = source_ptr->get_samplerate();
            if (current_samplerate == 0)
                throw satdump_exception("Samplerate not set!");

            source_ptr->start();

            if (current_decimation > 1)
            {
                decim_ptr = std::make_shared<dsp::SmartResamplerBlock<complex_t>>(source_ptr->output_stream, 1, current_decimation);
                decim_ptr->start();
                logger->info("Setting up resampler...");
            }

            fft->set_fft_settings(fft_size, get_samplerate(), fft_rate);
            waterfall_plot->set_rate(fft_rate, waterfall_rate);
            fft_plot->bandwidth = get_samplerate();

            splitter->input_stream = current_decimation > 1 ? decim_ptr->output_stream : source_ptr->output_stream;
            splitter->start();
            is_started = true;
        }
        catch (std::runtime_error &e)
        {
            sdr_error.set_message(style::theme.red, e.what());
            logger->error(e.what());
        }
    }

    void RecorderApplication::stop()
    {
        if (!is_started)
            return;

        splitter->stop_tmp();
        if (current_decimation > 1)
            decim_ptr->stop();
        source_ptr->stop();
        is_started = false;
        satdump_cfg.main_cfg["user"]["recorder_sdr_settings"]["last_used_sdr"] = sources[sdr_select_id].name;
        satdump_cfg.main_cfg["user"]["recorder_sdr_settings"][sources[sdr_select_id].name] = source_ptr->get_settings();
        satdump_cfg.main_cfg["user"]["recorder_sdr_settings"][sources[sdr_select_id].name]["samplerate"] = source_ptr->get_samplerate();
        satdump_cfg.main_cfg["user"]["recorder_sdr_settings"][sources[sdr_select_id].name]["frequency"] = frequency_hz;
        satdump_cfg.main_cfg["user"]["recorder_sdr_settings"][sources[sdr_select_id].name]["xconverter_frequency"] = xconverter_frequency;
        satdump_cfg.main_cfg["user"]["recorder_sdr_settings"][sources[sdr_select_id].name]["decimation"] = current_decimation;
        satdump_cfg.saveUser();
    }

    void RecorderApplication::try_load_sdr_settings()
    {
        if (satdump_cfg.main_cfg["user"].contains("recorder_sdr_settings"))
        {
            if (satdump_cfg.main_cfg["user"]["recorder_sdr_settings"].contains(sources[sdr_select_id].name))
            {
                auto &cfg = satdump_cfg.main_cfg["user"]["recorder_sdr_settings"][sources[sdr_select_id].name];
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
                    frequency_hz = cfg["frequency"].get<uint64_t>();
                    set_frequency(frequency_hz);
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

    void RecorderApplication::start_processing()
    {
        if (pipeline_selector.outputdirselect.isValid() || automated_live_output_dir)
        {
            logger->trace("Start pipeline...");
            pipeline_params = pipeline_selector.getParameters();
            pipeline_params["samplerate"] = get_samplerate();
            pipeline_params["baseband_format"] = "cf32";
            pipeline_params["buffer_size"] = dsp::STREAM_BUFFER_SIZE; // This is required, as we WILL go over the (usually) default 8192 size
            pipeline_params["start_timestamp"] = (double)time(0);     // Some pipelines need this

            try
            {
                if (automated_live_output_dir)
                    pipeline_output_dir = prepareAutomatedPipelineFolder(time(0), source_ptr->d_frequency, pipeline_selector.selected_pipeline.id);
                else
                    pipeline_output_dir = pipeline_selector.outputdirselect.getPath();

                live_pipeline = std::make_unique<pipeline::LivePipeline>(pipeline_selector.selected_pipeline, pipeline_params, pipeline_output_dir);
                splitter->reset_output("live");
                live_pipeline->start(splitter->get_output("live"), ui_thread_pool);
                splitter->set_enabled("live", true);

                is_processing = true;
            }
            catch (std::runtime_error &e)
            {
                error.set_message(style::theme.red, e.what());
                logger->error(e.what());
            }
        }
        else
        {
            error.set_message(style::theme.red, "Please select a valid output directory!");
        }
    }

    void RecorderApplication::stop_processing()
    {
        if (is_processing)
        {
            is_stopping_processing = true;
            logger->trace("Stop pipeline...");
            splitter->set_enabled("live", false);
            live_pipeline->stop();
            is_stopping_processing = is_processing = false;

            if (satdump_cfg.main_cfg["user_interface"]["finish_processing_after_live"]["value"].get<bool>() && live_pipeline->getOutputFile().size() > 0)
            {
                pipeline::Pipeline pipeline = pipeline_selector.selected_pipeline;
                std::string input_file = live_pipeline->getOutputFile();
                logger->critical(input_file);
                int start_level = pipeline.live_cfg.normal_live[pipeline.live_cfg.normal_live.size() - 1];
                std::string input_level = pipeline.steps[start_level].level;
                eventBus->fire_event<explorer::ExplorerAddHandlerEvent>(
                    {std::make_shared<handlers::OffProcessingHandler>(pipeline, input_level, input_file, pipeline_output_dir, pipeline_params), false, true});
            }

            live_pipeline.reset();
        }
    }

    void RecorderApplication::start_recording()
    {
        splitter->set_enabled("record", true);
        load_rec_path_data();
        std::string filename = recording_path + prepareBasebandFileName(getTime(), get_samplerate(), frequency_hz);
        recorder_filename = file_sink->start_recording(filename, get_samplerate());
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
            load_rec_path_data();
        }
    }

    void RecorderApplication::load_rec_path_data()
    {
        recording_path = satdump_cfg.main_cfg["satdump_directories"]["recording_path"]["value"].get<std::string>();
#if defined(_MSC_VER)
        recording_path += "\\";
#elif defined(__ANDROID__)
        if (recording_path == ".")
            recording_path = "/storage/emulated/0";
        recording_path += "/";
#else
        recording_path += "/";
#endif

#ifdef _MSC_VER
        ULARGE_INTEGER bytes_available;
        if (GetDiskFreeSpaceEx(recording_path.c_str(), &bytes_available, NULL, NULL))
            disk_available = bytes_available.QuadPart;
#else
        struct statvfs stat_buffer;
        if (statvfs(recording_path.c_str(), &stat_buffer) == 0)
            disk_available = stat_buffer.f_bavail * stat_buffer.f_bsize;
#endif
    }

    void RecorderApplication::try_init_tracking_widget()
    {
        if (tracking_widget == nullptr)
        {
            tracking_widget = new TrackingWidget();

            tracking_widget->aos_callback = [this](AutoTrackCfg autotrack_cfg, SatellitePass, TrackedObject obj)
            {
                if (autotrack_cfg.multi_mode || obj.downlinks.size() > 1)
                {
                    if (!autotrack_cfg.multi_mode)
                    {
                        double center_freq = 0;
                        for (auto &dl : obj.downlinks)
                            center_freq += dl.frequency;
                        center_freq /= obj.downlinks.size();
                        set_frequency(center_freq);
                    }

                    for (auto &dl : obj.downlinks)
                    {
                        if (dl.live || dl.record)
                            if (!is_started)
                                start();

                        if (dl.live)
                        {
                            std::string id = std::to_string(obj.norad) + "_" + std::to_string(dl.frequency) + "_live";
                            std::string name = std::to_string(obj.norad);
                            std::optional<TLE> this_tle = satdump::general_tle_registry->get_from_norad(obj.norad);
                            if (this_tle.has_value())
                                name = this_tle->name;
                            name += " - " + format_notated(dl.frequency, "Hz");
                            // TODOPIPELINE           add_vfo_live(id, name, dl.frequency, dl.pipeline_selector->selected_pipeline, dl.pipeline_selector->getParameters());
                        }

                        if (dl.record)
                        {
                            std::string id = std::to_string(obj.norad) + "_" + std::to_string(dl.frequency) + "_record";
                            std::string name = std::to_string(obj.norad);
                            std::optional<TLE> this_tle = satdump::general_tle_registry->get_from_norad(obj.norad);
                            if (this_tle.has_value())
                                name = this_tle->name;
                            name += " - " + format_notated(dl.frequency, "Hz");
                            add_vfo_reco(id, name, dl.frequency, dl.baseband_format, dl.baseband_decimation);
                        }
                    }
                }
                else
                {
                    if (obj.downlinks[0].live)
                        stop_processing();
                    if (obj.downlinks[0].record)
                        stop_recording();

                    if (obj.downlinks[0].live || obj.downlinks[0].record)
                    {
                        frequency_hz = obj.downlinks[0].frequency;
                        if (is_started)
                            set_frequency(frequency_hz);
                        else
                            start();

                        // Catch situations where source could not start
                        if (!is_started)
                        {
                            logger->error("Could not start recorder/processor since the source could not be started!");
                            return;
                        }
                    }

                    if (obj.downlinks[0].live)
                    {
                        pipeline_selector.select_pipeline(obj.downlinks[0].pipeline_selector->selected_pipeline.id);
                        pipeline_selector.setParameters(obj.downlinks[0].pipeline_selector->getParameters());
                        pipeline_selector.selected_pipeline.steps = obj.downlinks[0].pipeline_selector->selected_pipeline.steps;
                        start_processing();
                    }

                    if (obj.downlinks[0].record)
                    {
                        file_sink->set_output_sample_type(obj.downlinks[0].baseband_format);
                        start_recording();
                    }
                }
            };

            tracking_widget->los_callback = [this](AutoTrackCfg autotrack_cfg, SatellitePass, TrackedObject obj)
            {
                if (autotrack_cfg.multi_mode || obj.downlinks.size() > 1)
                {
                    for (auto &dl : obj.downlinks)
                    {
                        if (dl.live)
                        {
                            std::string id = std::to_string(obj.norad) + "_" + std::to_string(dl.frequency) + "_live";
                            del_vfo(id);
                        }

                        if (dl.record)
                        {
                            std::string id = std::to_string(obj.norad) + "_" + std::to_string(dl.frequency) + "_record";
                            del_vfo(id);
                        }

                        if (dl.live || dl.record)
                            if (is_started && vfo_list.size() == 0 && autotrack_cfg.stop_sdr_when_idle)
                                stop();
                    }
                }
                else
                {
                    if (obj.downlinks[0].record)
                        stop_recording();
                    if (obj.downlinks[0].live)
                        stop_processing();
                    if (autotrack_cfg.stop_sdr_when_idle)
                        stop();
                }
            };
        }
    }
} // namespace satdump
