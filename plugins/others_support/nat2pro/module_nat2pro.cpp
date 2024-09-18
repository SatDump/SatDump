#include "module_nat2pro.h"
#include "logger.h"
#include "formats/formats.h"
#include "products/dataset.h"

namespace nat2pro
{
    Nat2ProModule::Nat2ProModule(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
        : ProcessingModule(input_file, output_file_hint, parameters)
    {
    }

    void Nat2ProModule::process()
    {
        std::string native_file = d_input_file;
        std::string pro_output_file = d_output_file_hint.substr(0, d_output_file_hint.rfind('/')) + "/";

        // We will in the future want to decode from memory, so load it all up in RAM
        std::vector<uint8_t> nat_file;
        {
            std::ifstream input_file(native_file, std::ios::binary);
            uint8_t byte;
            while (!input_file.eof())
            {
                input_file.read((char *)&byte, 1);
                nat_file.push_back(byte);
            }
        }

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
            decodeMSGNat(nat_file, pro_output_file);

        // MetOp AVHRR Nat
        else if (identifier[552] == 'A' &&
                 identifier[553] == 'V' &&
                 identifier[554] == 'H' &&
                 identifier[555] == 'R')
            decodeAVHRRNat(nat_file, pro_output_file);

        // MetOp MHS Nat
        else if (identifier[552] == 'M' &&
                 identifier[553] == 'H' &&
                 identifier[554] == 'S' &&
                 identifier[555] == 'x')
            decodeMHSNat(nat_file, pro_output_file);

        else
            logger->error("Unknown File Type!");

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

    void Nat2ProModule::drawUI(bool window)
    {
        ImGui::Begin("Data Format To Products (nat2pro)", NULL, window ? 0 : NOWINDOW_FLAGS);

        ImGui::End();
    }

    std::string Nat2ProModule::getID()
    {
        return "nat2pro";
    }

    std::vector<std::string> Nat2ProModule::getParameters()
    {
        return {};
    }

    std::shared_ptr<ProcessingModule> Nat2ProModule::getInstance(std::string input_file, std::string output_file_hint, nlohmann::json parameters)
    {
        return std::make_shared<Nat2ProModule>(input_file, output_file_hint, parameters);
    }
}
