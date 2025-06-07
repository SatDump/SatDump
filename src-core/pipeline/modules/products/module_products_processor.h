#pragma once

#include "common/widgets/logger_sink.h"
#include "pipeline/module.h"

namespace satdump
{
    namespace pipeline
    {
        namespace products
        {
            class ProductsProcessorModule : public ProcessingModule
            {
            public:
                ProductsProcessorModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
                ~ProductsProcessorModule();
                void process();
                void drawUI(bool window);
                std::vector<ModuleDataType> getInputTypes();
                std::vector<ModuleDataType> getOutputTypes();

                std::shared_ptr<widgets::LoggerSinkWidget> logger_sink;

            public:
                static std::string getID();
                virtual std::string getIDM() { return getID(); }
                static nlohmann::json getParams() { return {}; }
                static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
            };
        } // namespace products
    } // namespace pipeline
} // namespace satdump