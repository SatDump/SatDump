#pragma once

#include "angelscript/angelscript.h"
#include <string>

namespace satdump
{
    namespace script
    {
        void GenerateScriptPredefined(const asIScriptEngine *engine, const std::string &path);
    }
} // namespace satdump