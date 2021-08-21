#include "logger.h"
#include "module.h"
#include "pipeline.h"
#include <filesystem>
#include "nlohmann/json.hpp"
#include <fstream>
#include "processing.h"
#include "main_ui.h"

namespace processing
{
    void process(std::string downlink_pipeline,
                 std::string input_level,
                 std::string input_file,
                 std::string output_level,
                 std::string output_file,
                 std::map<std::string, std::string> parameters)
    {
        satdumpUiStatus = OFFLINE_PROCESSING;

        logger->info("Starting processing pipeline " + downlink_pipeline + "...");
        logger->debug("Input file (" + input_level + ") : " + input_file);
        logger->debug("Output file (" + output_level + ") : " + output_file);

        if (!std::filesystem::exists(output_file))
            std::filesystem::create_directory(output_file);

        std::vector<Pipeline>::iterator it = std::find_if(pipelines.begin(),
                                                          pipelines.end(),
                                                          [&downlink_pipeline](const Pipeline &e)
                                                          {
                                                              return e.name == downlink_pipeline;
                                                          });

        if (it != pipelines.end())
        {
            it->run(input_file, output_file, parameters, input_level, true, uiCallList, uiCallListMutex);
        }
        else
        {
            logger->critical("Pipeline " + downlink_pipeline + " does not exist!");
        }

        logger->info("Done! Goodbye");
        satdumpUiStatus = MAIN_MENU;
    }
}