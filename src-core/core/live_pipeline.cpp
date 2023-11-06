#include "live_pipeline.h"
#include "logger.h"

namespace satdump
{
    LivePipeline::LivePipeline(Pipeline pipeline,
                               nlohmann::json parameters,
                               std::string output_dir)
        : d_pipeline(pipeline),
          d_parameters(parameters),
          d_output_dir(output_dir)
    {
    }

    LivePipeline::~LivePipeline()
    {
    }

    void LivePipeline::prepare_modules(std::vector<std::pair<int, int>> mods)
    {
        for (std::pair<int, int> currentModule : mods)
        {
            if ((int)d_pipeline.steps.size() <= currentModule.first)
                throw std::runtime_error("Invalid live pipeline step!");

            if ((int)d_pipeline.steps[currentModule.first].modules.size() <= currentModule.second)
                throw std::runtime_error("Invalid live pipeline module!");

            // Prep parameters
            nlohmann::json final_parameters = Pipeline::prepareParameters(
                d_pipeline.steps[currentModule.first].modules[currentModule.second].parameters,
                d_parameters);

            // Log it all out
            logger->info("Module " + d_pipeline.steps[currentModule.first].modules[currentModule.second].module_name);
            logger->debug("Parameters :");
            for (const nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::json>> &param : final_parameters.items())
                logger->debug("   - " + param.key() + " : " + param.value().dump());

            modules.push_back(modules_registry[d_pipeline.steps[currentModule.first]
                                                   .modules[currentModule.second]
                                                   .module_name]("", d_output_dir + "/" + d_pipeline.name, final_parameters));
        }
    }

    void LivePipeline::prepare_module(std::string id)
    {
        // Log it all out
        logger->info("Module " + id);
        logger->debug("Parameters :");
        for (const nlohmann::detail::iteration_proxy_value<nlohmann::detail::iter_impl<nlohmann::json>> &param : d_parameters.items())
            logger->debug("   - " + param.key() + " : " + param.value().dump());

        modules.push_back(modules_registry[id]("", d_output_dir + "/" + d_pipeline.name, d_parameters));
    }

    void LivePipeline::start(std::shared_ptr<dsp::stream<complex_t>> stream, ctpl::thread_pool &tp, bool server)
    {
        if (server) // Server mode
        {
            if (d_pipeline.live_cfg.server_live.size() == 0)
                throw std::runtime_error("Pipeline does not support server mode!");

            prepare_modules(d_pipeline.live_cfg.server_live);
            d_parameters["pkt_size"] = d_pipeline.live_cfg.pkt_size;
            prepare_module("network_server");
        }
        else // Normal mode
        {
            prepare_modules(d_pipeline.live_cfg.normal_live);
        }

        // Init first module in the chain, always a demod...
        modules[0]->input_stream = stream;
        modules[0]->setInputType(DATA_DSP_STREAM);
        modules[0]->setOutputType(modules.size() > 1 ? DATA_STREAM : DATA_FILE);
        modules[0]->output_fifo = std::make_shared<dsp::RingBuffer<uint8_t>>(1000000);
        modules[0]->init();
        modules[0]->input_active = true;
        module_futs.push_back(tp.push([=](int)
                                      {
                                        logger->info("Start processing...");
                                        modules[0]->process(); }));

        // Init whatever's in the middle
        for (int i = 1; i < (int)modules.size() - 1; i++)
        {
            modules[i]->input_fifo = modules[i - 1]->output_fifo;
            modules[i]->output_fifo = std::make_shared<dsp::RingBuffer<uint8_t>>(1000000);
            modules[i]->setInputType(DATA_STREAM);
            modules[i]->setOutputType(DATA_STREAM);
            modules[i]->init();
            modules[i]->input_active = true;
            module_futs.push_back(tp.push([=](int)
                                          {
                                            logger->info("Start processing...");
                                            modules[i]->process(); }));
        }

        // Init the last module
        if (modules.size() > 1)
        {
            int num = modules.size() - 1;
            modules[num]->input_fifo = modules[num - 1]->output_fifo;
            modules[num]->setInputType(DATA_STREAM);
            modules[num]->setOutputType(DATA_FILE);
            modules[num]->init();
            modules[num]->input_active = true;
            module_futs.push_back(tp.push([=](int)
                                          {
                                            logger->info("Start processing...");
                                            modules[num]->process(); }));
        }
    }

    void LivePipeline::start_client(ctpl::thread_pool &tp)
    {
        // Init modules
        {
            if (d_pipeline.live_cfg.client_live.size() == 0)
                throw std::runtime_error("Pipeline does not support client mode!");

            d_parameters["pkt_size"] = d_pipeline.live_cfg.pkt_size;
            prepare_module("network_client");
            prepare_modules(d_pipeline.live_cfg.client_live);
        }

        // Init the first and whatever's in the middle
        for (int i = 0; i < (int)modules.size() - 1; i++)
        {
            if (i > 0)
                modules[i]->input_fifo = modules[i - 1]->output_fifo;
            modules[i]->output_fifo = std::make_shared<dsp::RingBuffer<uint8_t>>(1000000);
            modules[i]->setInputType(DATA_STREAM);
            modules[i]->setOutputType(DATA_STREAM);
            modules[i]->init();
            modules[i]->input_active = true;
            module_futs.push_back(tp.push([=](int)
                                          {
                                            logger->info("Start processing...");
                                            modules[i]->process(); }));
        }

        // Init the last module
        if (modules.size() > 1)
        {
            int num = modules.size() - 1;
            modules[num]->input_fifo = modules[num - 1]->output_fifo;
            modules[num]->setInputType(DATA_STREAM);
            modules[num]->setOutputType(DATA_FILE);
            modules[num]->init();
            modules[num]->input_active = true;
            module_futs.push_back(tp.push([=](int)
                                          {
                                            logger->info("Start processing...");
                                            modules[num]->process(); }));
        }
    }

    void LivePipeline::stop()
    {
        logger->info("Stop processing");
        for (int i = 0; i < (int)modules.size(); i++)
        {
            std::shared_ptr<ProcessingModule> mod = modules[i];

            mod->input_active = false;

            if (mod->getInputType() == DATA_DSP_STREAM)
            {
                mod->input_stream->stopReader();
                mod->input_stream->stopWriter();
            }
            else if (mod->getInputType() == DATA_STREAM)
            {
                mod->input_fifo->stopReader();
                mod->input_fifo->stopWriter();
            }
            mod->stop();
            module_futs[i].get();
        }
    }

    std::vector<std::string> LivePipeline::getOutputFiles()
    {
        int num = modules.size() - 1;
        return modules[num]->getOutputs();
    }

    void LivePipeline::updateModuleStats()
    {
        for (std::shared_ptr<ProcessingModule> mod : modules)
            if (mod->module_stats.size() > 0)
                stats[mod->getIDM()] = mod->module_stats;
    }

    void LivePipeline::drawUIs()
    {
        for (std::shared_ptr<ProcessingModule> mod : modules)
            if (mod->hasUI())
                mod->drawUI(true);
    }
}
