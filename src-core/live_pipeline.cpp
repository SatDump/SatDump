#include "live_pipeline.h"
#include "logger.h"

LivePipeline::LivePipeline(Pipeline pipeline,
                           std::map<std::string, std::string> parameters,
                           std::string output_dir)
{
    logger->info("Starting live pipeline " + pipeline.readable_name);

    for (std::pair<int, int> currentModule : pipeline.live_cfg)
    {
        std::map<std::string, std::string> final_parameters = pipeline.steps[currentModule.first].modules[currentModule.second].parameters;
        for (const std::pair<std::string, std::string> param : parameters)
            if (final_parameters.count(param.first) > 0)
                final_parameters[param.first] = param.second;
            else
                final_parameters.emplace(param.first, param.second);

        logger->info("Module " + pipeline.steps[currentModule.first].modules[currentModule.second].module_name);
        logger->debug("Parameters :");
        for (const std::pair<std::string, std::string> param : final_parameters)
            logger->debug("   - " + param.first + " : " + param.second);

        modules.push_back(modules_registry[pipeline.steps[currentModule.first]
                                               .modules[currentModule.second]
                                               .module_name]("", output_dir + "/" + pipeline.name, final_parameters));
    }
}

LivePipeline::~LivePipeline()
{
}

void LivePipeline::start(std::shared_ptr<dsp::stream<std::complex<float>>> stream, ctpl::thread_pool &threadPool)
{
    // Init first module in the chain, always a demod...
    modules[0]->input_stream = stream;
    modules[0]->setInputType(DATA_DSP_STREAM);
    modules[0]->setOutputType(modules.size() > 1 ? DATA_STREAM : DATA_FILE);
    modules[0]->output_fifo = std::make_shared<dsp::RingBuffer<uint8_t>>(1000000);
    modules[0]->init();
    modules[0]->input_active = true;
    moduleFutures.push_back(threadPool.push([=](int)
                                            {
                                                logger->info("Start processing...");
                                                modules[0]->process();
                                            }));

    // Init whatever's in the middle
    for (int i = 1; i < (int)modules.size() - 1; i++)
    {
        modules[i]->input_fifo = modules[i - 1]->output_fifo;
        modules[i]->output_fifo = std::make_shared<dsp::RingBuffer<uint8_t>>(1000000);
        modules[i]->setInputType(DATA_STREAM);
        modules[i]->setOutputType(DATA_STREAM);
        modules[i]->init();
        modules[i]->input_active = true;
        moduleFutures.push_back(threadPool.push([=](int)
                                                {
                                                    logger->info("Start processing...");
                                                    modules[i]->process();
                                                }));
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
        moduleFutures.push_back(threadPool.push([=](int)
                                                {
                                                    logger->info("Start processing...");
                                                    modules[num]->process();
                                                }));
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
        //logger->info("mod");
        mod->stop();
        moduleFutures[i].get();
    }
}

std::vector<std::string> LivePipeline::getOutputFiles()
{
    int num = modules.size() - 1;
    return modules[num]->getOutputs();
}

nlohmann::json LivePipeline::getModulesStats()
{
    nlohmann::json stats;
    for (std::shared_ptr<ProcessingModule> mod : modules)
    {
        if (mod->module_stats.size() > 0)
            stats[mod->getIDM()] = mod->module_stats;
    }
    return stats;
}

std::vector<std::shared_ptr<ProcessingModule>> LivePipeline::getModules()
{
    return modules;
}

void LivePipeline::drawUIs()
{
    for (std::shared_ptr<ProcessingModule> mod : modules)
        mod->drawUI(true);
}