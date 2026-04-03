#pragma once
#include "nlohmann/json.hpp"

namespace satdump
{
    namespace widgets
    {
        template <typename T>
        void JSONTreeEditor(T &json, const char *id, bool allow_add = true);

        bool JSONTableEditor(nlohmann::json &json, const char *id);
    } // namespace widgets
} // namespace satdump