#include "module_nc2pro.h"
#include "logger.h"
#include "products/dataset.h"
#include "common/utils.h"

#include "mtg_fci.h"

namespace nc2pro
{
    Nc2ProModule::Nc2ProModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : ProcessingModule(input_file, output_file_hint, parameters)
    {
    }

    void Nc2ProModule::process()
    {
        std::string source_nc_file = d_input_file;
        std::string pro_output_file = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/";

        filesize = getFilesize(source_nc_file);

        // MTG only for now
        process_mtg_fci(source_nc_file, pro_output_file, &progress);

        if (std::filesystem::exists(pro_output_file + "/product.cbor"))
        {
            // Products dataset
            satdump::ProductDataSet dataset;
            dataset.satellite_name = "Generic Product (Data Store / Archive)";
            dataset.timestamp = time(0); // avg_overflowless(avhrr_reader.timestamps);
            dataset.products_list.push_back(".");
            dataset.save(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')));
        }
    }

    void Nc2ProModule::drawUI(bool window)
    {
        ImGui::Begin("Data Format To Products (nc2pro)", NULL, window ? 0 : NOWINDOW_FLAGS);

        ImGui::ProgressBar(progress);

        ImGui::End();
    }

    std::string Nc2ProModule::getID()
    {
        return "nc2pro";
    }

    std::vector<std::string> Nc2ProModule::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> Nc2ProModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<Nc2ProModule>(input_file, output_file_hint, parameters);
    }
}
