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

        private:
            template <typename T>
            T getVariable(std::string var_str);

        public:
            LuaVariableManager()
            {
                variables["samplerate"] = 6e6;
            }

            void drawVariables();

            double getDoubleVariable(std::string var_str);
            int getIntVariable(std::string var_str);
        };
    }
}