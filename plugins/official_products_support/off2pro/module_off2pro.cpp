#include "module_off2pro.h"
#include "common/utils.h"
#include "logger.h"
#include "products/dataset.h"

#include "../file_utils/file_utils.h"
#include "../nc2pro/sc3/sc3_olci.h"
#include "../nc2pro/sc3/sc3_slstr.h"

#include "../nc2pro/ami/gk2a_ami.h"

#include "../nc2pro/jpss/jpss_viirs.h"

namespace off2pro
{
    Off2ProModule::Off2ProModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters) : ProcessingModule(input_file, output_file_hint, parameters) {}

    void Off2ProModule::process()
    {
        std::string source_off_file = d_input_file;
        std::string pro_output_file = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/";

        filesize = getFilesize(source_off_file);
        std::filesystem::path source_off_path(source_off_file);

        std::vector<std::string> product_paths;

        // Perhaps a zip?
        if (satdump::file_is_zip(source_off_file))
        {
            std::string eum_id = satdump::try_get_eumetsat_id(source_off_file);

            // Sentinel-3 OCLI
            if (eum_id.find("SEN3") != std::string::npos && eum_id.find("OL_1_EF") != std::string::npos)
                nc2pro::process_sc3_ocli(source_off_file, pro_output_file, &progress);

            else if (eum_id.find("SEN3") != std::string::npos && eum_id.find("SL_1_RBT") != std::string::npos)
                nc2pro::process_sc3_slstr(source_off_file, pro_output_file, product_paths, &progress);

            else
                logger->error("Unknown EUMETSAT file type! " + eum_id);
        }
        else if (source_off_path.extension() == ".nc")
        {
            std::string prefix = source_off_path.stem().string().substr(0, 14);
            std::string prefix2 = source_off_path.stem().string().substr(0, 5);
            if (prefix == "gk2a_ami_le1b_")
                nc2pro::process_gk2a_ami(source_off_file, pro_output_file, &progress);
            else if (prefix2 == "VNP02")
                nc2pro::process_jpss_viirs(source_off_file, pro_output_file, &progress);
            else
                logger->error("Unknown .nc file type!");
        }

        if (/*std::filesystem::exists(pro_output_file + "/product.cbor") ||*/ product_paths.size() > 0)
        {
            // Products dataset
            satdump::products::DataSet dataset;
            dataset.satellite_name = "Generic Product (Data Store / Archive)";
            dataset.timestamp = time(0); // avg_overflowless(avhrr_reader.timestamps);
            if (product_paths.size() == 0)
                dataset.products_list.push_back(".");
            else
                dataset.products_list = product_paths;
            dataset.save(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')));
            d_output_file = pro_output_file + "/dataset.json";
        }
        else if (std::filesystem::exists(pro_output_file + "/product.cbor"))
        {
            d_output_file = pro_output_file + "/product.cbor";
        }
    }

    void Off2ProModule::drawUI(bool window)
    {
        ImGui::Begin("Data Format To Products", NULL, window ? 0 : NOWINDOW_FLAGS);

        ImGui::ProgressBar(progress);

        ImGui::End();
    }

    std::string Off2ProModule::getID() { return "off2pro"; }

    std::shared_ptr<satdump::pipeline::ProcessingModule> Off2ProModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<Off2ProModule>(input_file, output_file_hint, parameters);
    }
} // namespace off2pro
