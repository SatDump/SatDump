#pragma once

#include <string>

namespace satdump
{
    namespace error
    {
        extern bool hasError;
        void set_error(std::string title, std::string description);
        void render();
    }
}