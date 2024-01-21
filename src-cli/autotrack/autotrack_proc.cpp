#include "autotrack.h"
#include "logger.h"
#include "common/utils.h"

void AutoTrackApp::start_processing()
{
    if (is_processing)
        return;

    live_pipeline_mtx.lock();
    logger->trace("Start pipeline...");
    pipeline_params["samplerate"] = get_samplerate();
    pipeline_params["baseband_format"] = "f32";
    pipeline_params["buffer_size"] = dsp::STREAM_BUFFER_SIZE; // This is required, as we WILL go over the (usually) default 8192 size
    pipeline_params["start_timestamp"] = (double)time(0);     // Some pipelines need this

    pipeline_output_dir = prepareAutomatedPipelineFolder(time(0), source_ptr->d_frequency, satdump::pipelines[pipeline_id].name, d_output_folder);

    try
    {
        live_pipeline = std::make_unique<satdump::LivePipeline>(satdump::pipelines[pipeline_id], pipeline_params, pipeline_output_dir);
        splitter->reset_output("live");
        live_pipeline->start(splitter->get_output("live"), main_thread_pool);
        splitter->set_enabled("live", true);

        is_processing = true;
    }
    catch (std::runtime_error &e)
    {
        logger->error(e.what());
    }

    live_pipeline_mtx.unlock();
}

void AutoTrackApp::stop_processing()
{
    if (is_processing)
    {
        live_pipeline_mtx.lock();
        logger->trace("Stop pipeline...");
        is_processing = false;
        splitter->set_enabled("live", false);
        live_pipeline->stop();

        if (d_settings.contains("finish_processing") && d_settings["finish_processing"].get<bool>() && live_pipeline->getOutputFiles().size() > 0)
        {
            std::string input_file = live_pipeline->getOutputFiles()[0];
            int pipeline_id_ = pipeline_id;
            std::string pipeline_output_dir_ = pipeline_output_dir;
            nlohmann::json pipeline_params_ = pipeline_params;
            auto fun = [pipeline_id_, pipeline_output_dir_, input_file, pipeline_params_](int)
            {
                satdump::Pipeline pipeline = satdump::pipelines[pipeline_id_];
                int start_level = pipeline.live_cfg.normal_live[pipeline.live_cfg.normal_live.size() - 1].first;
                std::string input_level = pipeline.steps[start_level].level_name;
                pipeline.run(input_file, pipeline_output_dir_, pipeline_params_, input_level);
            };
            main_thread_pool.push(fun);
        }

        live_pipeline.reset();
        live_pipeline_mtx.unlock();
    }
}