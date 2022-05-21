#include "module_products_processor.h"
#include "logger.h"
#include "imgui/imgui.h"

#include "products/processor/processor.h"

namespace products
{
    ProductsProcessorModule::ProductsProcessorModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
    {
        logger_sink = std::make_shared<widgets::LoggerSinkWidget<std::mutex>>();
        logger_sink->set_pattern("[%D - %T] %^(%L) %v%$");
        logger_sink->max_lines = 500;
    }

    std::vector<ModuleDataType> ProductsProcessorModule::getInputTypes()
    {
        return {DATA_FILE};
    }

    std::vector<ModuleDataType> ProductsProcessorModule::getOutputTypes()
    {
        return {DATA_FILE};
    }

    ProductsProcessorModule::~ProductsProcessorModule()
    {
    }

    void ProductsProcessorModule::process()
    {
        std::vector<spdlog::sink_ptr> to_insert = {logger_sink};
        auto it = logger->sinks().insert(logger->sinks().end(), to_insert.begin(), to_insert.end());

        satdump::process_dataset(d_input_file);

        logger->sinks().erase(it);
    }

    void ProductsProcessorModule::drawUI(bool window)
    {
        ImGui::Begin("Products Processor", NULL, (window ? NULL : NOWINDOW_FLAGS) | ImGuiWindowFlags_NoScrollbar);
        logger_sink->draw();
        ImGui::SetScrollY(ImGui::GetScrollMaxY());
        ImGui::End();
    }

    std::string ProductsProcessorModule::getID()
    {
        return "products_processor";
    }

    std::vector<std::string> ProductsProcessorModule::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> ProductsProcessorModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<ProductsProcessorModule>(input_file, output_file_hint, parameters);
    }
}