#include "autotrack.h"
#include "logger.h"
#include "common/utils.h"
#include "utils/time.h"

void AutoTrackApp::add_vfo_live(std::string id, std::string name, double freq, satdump::Pipeline vpipeline, nlohmann::json vpipeline_params)
{
    vfos_mtx.lock();

    try
    {
        VFOInfo wipInfo;
        wipInfo.id = id;
        wipInfo.name = name;
        wipInfo.freq = freq;
        wipInfo.selected_pipeline = vpipeline;
        wipInfo.pipeline_params = vpipeline_params;
        wipInfo.lpool = std::make_shared<ctpl::thread_pool>(8);

        vpipeline_params["samplerate"] = get_samplerate();
        vpipeline_params["baseband_format"] = "cf32";
        vpipeline_params["buffer_size"] = dsp::STREAM_BUFFER_SIZE; // This is required, as we WILL go over the (usually) default 8192 size
        vpipeline_params["start_timestamp"] = (double)time(0);     // Some pipelines need this

        std::string output_dir = prepareAutomatedPipelineFolder(time(0), freq, vpipeline.name, d_output_folder);

        wipInfo.output_dir = output_dir;

        wipInfo.live_pipeline = std::make_shared<satdump::LivePipeline>(vpipeline, vpipeline_params, output_dir);
        splitter->add_vfo(id, get_samplerate(), frequency_hz - freq);
        wipInfo.live_pipeline->start(splitter->get_vfo_output(id), *wipInfo.lpool.get());
        splitter->set_vfo_enabled(id, true);

        if (fft)
            fft_plot->vfo_freqs.push_back({name, freq});

        vfo_list.push_back(wipInfo);
    }
    catch (std::exception &e)
    {
        logger->error("Error adding VFO : %s", e.what());
    }

    vfos_mtx.unlock();
}

void AutoTrackApp::add_vfo_reco(std::string id, std::string name, double freq, dsp::BasebandType type, int decimation)
{
    vfos_mtx.lock();

    try
    {
        VFOInfo wipInfo;
        wipInfo.id = id;
        wipInfo.name = name;
        wipInfo.freq = freq;

        splitter->add_vfo(id, get_samplerate(), frequency_hz - freq);

        if (decimation > 1)
            wipInfo.decim_ptr = std::make_shared<dsp::SmartResamplerBlock<complex_t>>(splitter->get_vfo_output(id), 1, decimation);
        wipInfo.file_sink = std::make_shared<dsp::FileSinkBlock>(decimation > 1 ? wipInfo.decim_ptr->output_stream : splitter->get_vfo_output(id));
        wipInfo.file_sink->set_output_sample_type(type);

        if (decimation > 1)
            wipInfo.decim_ptr->start();
        wipInfo.file_sink->start();

        wipInfo.file_sink->start_recording(d_output_folder + "/" + prepareBasebandFileName(satdump::getTime(), get_samplerate() / decimation, freq), get_samplerate() / decimation);

        splitter->set_vfo_enabled(id, true);

        if (fft)
            fft_plot->vfo_freqs.push_back({name, freq});

        vfo_list.push_back(wipInfo);
    }
    catch (std::exception &e)
    {
        logger->error("Error adding VFO : %s", e.what());
    }

    vfos_mtx.unlock();
}

void AutoTrackApp::del_vfo(std::string id)
{
    vfos_mtx.lock();
    auto it = std::find_if(vfo_list.begin(), vfo_list.end(), [&id](VFOInfo &c)
                           { return c.id == id; });
    if (it != vfo_list.end())
    {
        if (fft)
        {
            std::string name = it->name;
            auto it2 = std::find_if(fft_plot->vfo_freqs.begin(), fft_plot->vfo_freqs.end(), [&name](auto &c)
                                    { return c.first == name; });
            if (it2 != fft_plot->vfo_freqs.end())
                fft_plot->vfo_freqs.erase(it2);
        }

        if (it->file_sink)
            it->file_sink->stop_recording();

        splitter->set_vfo_enabled(it->id, false);

        if (it->selected_pipeline.name != "")
            it->live_pipeline->stop();

        if (it->file_sink)
        {
            it->file_sink->stop();
            if (it->decim_ptr)
                it->decim_ptr->stop();
        }

        splitter->del_vfo(it->id);

        if (it->selected_pipeline.name != "")
        {
            if (d_settings.contains("finish_processing") && d_settings["finish_processing"].get<bool>() && it->live_pipeline->getOutputFiles().size() > 0)
            {
                std::string input_file = it->live_pipeline->getOutputFiles()[0];
                satdump::Pipeline selected_pipeline_ = it->selected_pipeline;
                std::string pipeline_output_dir_ = it->output_dir;
                nlohmann::json pipeline_params_ = it->pipeline_params;
                auto fun = [selected_pipeline_, pipeline_output_dir_, input_file, pipeline_params_](int)
                {
                    satdump::Pipeline pipeline = selected_pipeline_;
                    int start_level = pipeline.live_cfg.normal_live[pipeline.live_cfg.normal_live.size() - 1].first;
                    std::string input_level = pipeline.steps[start_level].level_name;
                    pipeline.run(input_file, pipeline_output_dir_, pipeline_params_, input_level);
                };
                main_thread_pool.push(fun);
            }
        }

        vfo_list.erase(it);
    }
    if (vfo_list.size() == 0)
        if (fft)
            fft_plot->vfo_freqs.clear();
    vfos_mtx.unlock();
}