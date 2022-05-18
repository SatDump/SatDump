#include "module_products_processor.h"
#include "logger.h"
#include "imgui/imgui.h"

#include "products/processor/processor.h"

namespace products
{
    ProductsProcessorModule::ProductsProcessorModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters)
    {
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
        satdump::process_dataset(d_input_file);
    }

    void ProductsProcessorModule::drawUI(bool window)
    {
        ImGui::Begin("Products Processor", NULL, window ? NULL : NOWINDOW_FLAGS);

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