#include "module_off2pro.h"
#include "logger.h"
#include "products/dataset.h"
#include "common/utils.h"

#include "../nat2pro/formats/formats.h"

#include "../file_utils/file_utils.h"
#include "../nc2pro/mtg_fci.h"
#include "../nc2pro/sc3_olci.h"
#include "../nc2pro/sc3_slstr.h"

#include "../nc2pro/goesr_abi.h"
#include "../nc2pro/gk2a_ami.h"
#include "../hsd2pro/himawari_ahi.h"

namespace off2pro
{
    Off2ProModule::Off2ProModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : ProcessingModule(input_file, output_file_hint, parameters)
    {
    }

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

            // MTG-I FCI
            if (eum_id == "FCI-1C-RRAD-FDHSI-FD" || eum_id == "FCI-1C-RRAD-HRFI-FD")
                nc2pro::process_mtg_fci(source_off_file, pro_output_file, &progress);

            // Sentinel-3 OCLI
            else if (eum_id.find("SEN3") != std::string::npos && eum_id.find("OL_1_EF") != std::string::npos)
                nc2pro::process_sc3_ocli(source_off_file, pro_output_file, &progress);

            else if (eum_id.find("SEN3") != std::string::npos && eum_id.find("SL_1_RBT") != std::string::npos)
                nc2pro::process_sc3_slstr(source_off_file, pro_output_file, product_paths, &progress);

            else
                logger->error("Unknown EUMETSAT file type! " + eum_id);
        }
        else if (source_off_path.extension() == ".nc")
        {
            std::string prefix = source_off_path.stem().string().substr(0, 14);
            if (prefix == "OR_ABI-L1b-Rad")
                nc2pro::process_goesr_abi(source_off_file, pro_output_file, &progress);
            else if(prefix == "gk2a_ami_le1b_")
                nc2pro::process_gk2a_ami(source_off_file, pro_output_file, &progress);
            else
                logger->error("Unknown .nc file type!");
        }
        else if (source_off_path.extension() == ".bz2")
        {
            std::string prefix = source_off_path.stem().string().substr(0, 4);
            if (prefix == "HS_H")
                hsd2pro::process_himawari_ahi(source_off_file, pro_output_file, &progress);
            else
                logger->error("Unknown .bz2 file type!");
        }
        // Otherwise, for now assume it's a .nat
        else
        {
            // We will in the future want to decode from memory, so load it all up in RAM
            std::vector<uint8_t> nat_file;
            {
                std::ifstream input_file(source_off_file, std::ios::binary);
                input_file.seekg(0, std::ios::end);
                const size_t nat_size = input_file.tellg();
                nat_file.resize(nat_size);
                input_file.seekg(0, std::ios::beg);
                input_file.read((char*)&nat_file[0], nat_size);
                input_file.close();
            }
            progress = 1;

            char *identifier = (char *)&nat_file[0];

            // MSG Nat
            if (identifier[0] == 'F' &&
                identifier[1] == 'o' &&
                identifier[2] == 'r' &&
                identifier[3] == 'm' &&
                identifier[4] == 'a' &&
                identifier[5] == 't' &&
                identifier[6] == 'N' &&
                identifier[7] == 'a' &&
                identifier[8] == 'm' &&
                identifier[9] == 'e')
                nat2pro::decodeMSGNat(nat_file, pro_output_file);

            // MetOp AVHRR Nat
            else if (identifier[552] == 'A' &&
                     identifier[553] == 'V' &&
                     identifier[554] == 'H' &&
                     identifier[555] == 'R')
                nat2pro::decodeAVHRRNat(nat_file, pro_output_file);

            // MetOp MHS Nat
            else if (identifier[552] == 'M' &&
                     identifier[553] == 'H' &&
                     identifier[554] == 'S' &&
                     identifier[555] == 'x')
                nat2pro::decodeMHSNat(nat_file, pro_output_file);

            // MetOp AMSU(A) Nat
            else if (identifier[552] == 'A' &&
                     identifier[553] == 'M' &&
                     identifier[554] == 'S' &&
                     identifier[555] == 'A')
                nat2pro::decodeAMSUNat(nat_file, pro_output_file);

            // MetOp HIRS Nat
            else if (identifier[552] == 'H' &&
                     identifier[553] == 'I' &&
                     identifier[554] == 'R' &&
                     identifier[555] == 'S')
                nat2pro::decodeHIRSNat(nat_file, pro_output_file);

            // MetOp IASI Nat
            else if (identifier[552] == 'I' &&
                     identifier[553] == 'A' &&
                     identifier[554] == 'S' &&
                     identifier[555] == 'I')
                nat2pro::decodeIASINat(nat_file, pro_output_file);

            // MetOp GOME Nat
            else if (identifier[552] == 'G' &&
                     identifier[553] == 'O' &&
                     identifier[554] == 'M' &&
                     identifier[555] == 'E')
                nat2pro::decodeGOMENat(nat_file, pro_output_file);

            else
                logger->error("Unknown File Type!");
        }

        if (std::filesystem::exists(pro_output_file + "/product.cbor") || product_paths.size() > 0)
        {
            // Products dataset
            satdump::ProductDataSet dataset;
            dataset.satellite_name = "Generic Product (Data Store / Archive)";
            dataset.timestamp = time(0); // avg_overflowless(avhrr_reader.timestamps);
            if (product_paths.size() == 0)
                dataset.products_list.push_back(".");
            else
                dataset.products_list = product_paths;
            dataset.save(d_output_file_hint.substr(0, d_output_file_hint.rfind('/')));
        }
    }

    void Off2ProModule::drawUI(bool window)
    {
        ImGui::Begin("Data Format To Products", NULL, window ? 0 : NOWINDOW_FLAGS);

        ImGui::ProgressBar(progress);

        ImGui::End();
    }

    std::string Off2ProModule::getID()
    {
        return "off2pro";
    }

    std::vector<std::string> Off2ProModule::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> Off2ProModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<Off2ProModule>(input_file, output_file_hint, parameters);
    }
}
