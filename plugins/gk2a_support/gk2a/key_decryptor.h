#pragma once

#include <string>

namespace gk2a
{
    namespace lrit
    {
        bool decrypt_key_file(std::string encrypted, std::string mac_address, std::string decrypted);
    }
}