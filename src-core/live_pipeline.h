#pragma once

#include "pipeline.h"
#include "common/ctpl/ctpl_stl.h"

class LivePipeline
{
private:
    std::map<std::string, std::string> d_parameters;
    std::vector<std::shared_ptr<ProcessingModule>> modules;

public:
    LivePipeline(Pipeline pipeline,
                 std::map<std::string, std::string> parameters,
                 std::string output_dir);
    ~LivePipeline();

    void start(std::shared_ptr<dsp::stream<std::complex<float>>> stream, ctpl::thread_pool &threadPool);
    void stop();

    void drawUIs();
};