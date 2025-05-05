#pragma once

#include <vector>
#include <string>
#include <cstdint>

namespace nat2pro
{
    void decodeMSGNat(std::vector<uint8_t> &data, std::string pro_output_file);

    void decodeAVHRRNat(std::vector<uint8_t> &data, std::string pro_output_file);
    void decodeMHSNat(std::vector<uint8_t> &data, std::string pro_output_file);
    void decodeAMSUNat(std::vector<uint8_t> &data, std::string pro_output_file);
    void decodeHIRSNat(std::vector<uint8_t> &data, std::string pro_output_file);
    void decodeIASINat(std::vector<uint8_t> &data, std::string pro_output_file);
    void decodeGOMENat(std::vector<uint8_t> &data, std::string pro_output_file);
}
