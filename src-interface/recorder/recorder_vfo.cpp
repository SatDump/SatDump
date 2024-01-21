#include "recorder.h"

#include "main_ui.h"
#include "logger.h"
#include "processing.h"
#include "common/utils.h"

namespace satdump
{
    void RecorderApplication::add_vfo(std::string id, std::string name, double freq, int vpipeline_id, nlohmann::json vpipeline_params)
    {
        vfos_mtx.lock();

        try
        {
            VFOInfo wipInfo;
            wipInfo.id = id;
            wipInfo.name = name;
            wipInfo.freq = freq;
            wipInfo.pipeline_id = vpipeline_id;
            wipInfo.pipeline_params = vpipeline_params;
            wipInfo.lpool = std::make_shared<ctpl::thread_pool>(8);

            vpipeline_params["samplerate"] = get_samplerate();
            vpipeline_params["baseband_format"] = "f32";
            vpipeline_params["buffer_size"] = dsp::STREAM_BUFFER_SIZE; // This is required, as we WILL go over the (usually) default 8192 size
            vpipeline_params["start_timestamp"] = (double)time(0);     // Some pipelines need this

            std::string output_dir = prepareAutomatedPipelineFolder(time(0), freq, pipelines[vpipeline_id].name);

            wipInfo.output_dir = output_dir;

            wipInfo.live_pipeline = std::make_shared<LivePipeline>(pipelines[vpipeline_id], vpipeline_params, output_dir);
            splitter->add_vfo(id, get_samplerate(), frequency_hz - freq);
            wipInfo.live_pipeline->start(splitter->get_vfo_output(id), *wipInfo.lpool.get());
            splitter->set_vfo_enabled(id, true);

            fft_plot->vfo_freqs.push_back({name, freq});

            vfo_list.push_back(wipInfo);
        }
        catch (std::exception &e)
        {
            logger->error("Error adding VFO : %s", e.what());
        }

        vfos_mtx.unlock();
    }

    void RecorderApplication::del_vfo(std::string id)
    {
        vfos_mtx.lock();
        auto it = std::find_if(vfo_list.begin(), vfo_list.end(), [&id](VFOInfo &c)
                               { return c.id == id; });
        if (it != vfo_list.end())
        {
            std::string name = it->name;
            auto it2 = std::find_if(fft_plot->vfo_freqs.begin(), fft_plot->vfo_freqs.end(), [&name](auto &c)
                                    { return c.first == name; });
            if (it2 != fft_plot->vfo_freqs.end())
                fft_plot->vfo_freqs.erase(it2);

            splitter->set_vfo_enabled(it->id, false);
            it->live_pipeline->stop();
            splitter->del_vfo(it->id);

            if (config::main_cfg["user_interface"]["finish_processing_after_live"]["value"].get<bool>() && it->live_pipeline->getOutputFiles().size() > 0)
            {
                Pipeline pipeline = pipelines[it->pipeline_id];
                std::string input_file = it->live_pipeline->getOutputFiles()[0];
                int start_level = pipeline.live_cfg.normal_live[pipeline.live_cfg.normal_live.size() - 1].first;
                std::string input_level = pipeline.steps[start_level].level_name;
                std::string output_dir = it->output_dir;
                nlohmann::json pipeline_params = it->pipeline_params;
                ui_thread_pool.push([=](int)
                                    { processing::process(pipeline.name, input_level, input_file, output_dir, pipeline_params); });
            }

            vfo_list.erase(it);
        }
        vfos_mtx.unlock();
    }
}
