#pragma once

#include "nlohmann/json.hpp"

namespace satdump
{
    namespace ndsp
    {
        class LuaVariableManager
        {
        public:
            nlohmann::json variables;

        public:
            nlohmann::json getVariable(std::string var_str);

        public:
            LuaVariableManager()
            {
                variables["samplerate"] = 6e6;
            }

            void drawVariables();
        };
    }
}