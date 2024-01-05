#include <fstream>
#include "nlohmann/json.hpp"
#include "nlohmann/json_utils.h"

int main(int argc, char *argv[])
{
    if (argc < 3)
        printf("Usage : cbor2json file.cbor file.json\n");

    if (argc == 3)
    {
        // Read file
        std::vector<uint8_t> cbor_data;
        std::ifstream in_file(argv[1], std::ios::binary);
        while (!in_file.eof())
        {
            uint8_t b;
            in_file.read((char *)&b, 1);
            cbor_data.push_back(b);
        }
        in_file.close();
        cbor_data.pop_back();
        nlohmann::json contents = nlohmann::json::from_cbor(cbor_data);

        std::ofstream output_file(argv[2], std::ios::binary);
        output_file << contents.dump(4);
        output_file.close();
    }
    else
    {
        nlohmann::json jsonFile = loadJsonFile(argv[1]);
        auto cbor_data = nlohmann::json::to_cbor(jsonFile);
        std::ofstream output_file(argv[2], std::ios::binary);
        output_file.write((char *)cbor_data.data(), cbor_data.size());
    }
}
