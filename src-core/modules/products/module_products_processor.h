#pragma once

#include "core/module.h"
#include <complex>
#include <fstream>
#include "common/widgets/logger_sink.h"

namespace products
{
    class ProductsProcessorModule : public ProcessingModule
    {
    protected:
        // IDK Yet

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
        virtual std::string getIDM() { return getID(); };
        static std::vector<std::string> getParameters();
        static std::shared_ptr<ProcessingModule> getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters);
    };
}