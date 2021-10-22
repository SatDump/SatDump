#pragma once

#include "pipeline.h"
#include "libs/ctpl/ctpl_stl.h"

class LivePipeline
{
private:
    std::map<std::string, std::string> d_parameters;
    std::vector<std::shared_ptr<ProcessingModule>> modules;
    std::vector<std::future<void>> moduleFutures;

public:
    LivePipeline(Pipeline pipeline,
                 std::map<std::string, std::string> parameters,
                 std::string output_dir);
    ~LivePipeline();

    void start(std::shared_ptr<dsp::stream<complex_t>> stream, ctpl::thread_pool &threadPool);
    void stop();

    std::vector<std::string> getOutputFiles();

    nlohmann::json getModulesStats();

    void drawUIs();
};