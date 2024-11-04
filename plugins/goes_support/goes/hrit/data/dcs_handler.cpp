#include "../module_goes_lrit_data_decoder.h"
#include "logger.h"

namespace goes
{
    namespace hrit
    {
        bool GOESLRITDataDecoderModule::parseDCS(uint8_t *data, size_t size)
        {
            // DCS Header
            std::string name = std::string(&data[0], &data[32]);
            int file_size = std::stoi(std::string(&data[32], &data[40]));
            std::string source = std::string(&data[40], &data[44]);
            std::string type = std::string(&data[44], &data[48]);
            std::string exp_field = std::string(&data[48], &data[60]);
            bool header_crc_pass = (uint32_t)(data[63] << 24 | data[62] << 16 | data[61] << 8 | data[60]) == dcs_crc32.compute(data, 60);

            if (size != file_size)
            {
                logger->warn("Invalid DCS File size! Not parsing");
                return false;
            }

            // Trim whitepace
            name.erase(name.find_last_not_of(" ") + 1);
            source.erase(source.find_last_not_of(" ") + 1);
            type.erase(type.find_last_not_of(" ") + 1);
            exp_field.erase(exp_field.find_last_not_of(" ") + 1);

            return false;
        }
    }
}