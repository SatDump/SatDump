#pragma once

#include <string>
#include <vector>

namespace satdump
{
    namespace imgui_utils
    {
        struct FileDropEvent
        {
            std::vector<std::string> files;
        };
    } // namespace imgui_utils
} // namespace satdump