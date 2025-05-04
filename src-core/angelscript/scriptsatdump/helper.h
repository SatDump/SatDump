#pragma once

#include "angelscript/angelscript.h"
#include <string>

namespace satdump
{
    namespace script
    {
        bool buildModuleFromFile(asIScriptEngine *engine, std::string module_id, std::string script_file);
        bool runVoidMainFromModule(asIScriptEngine *engine, std::string module_id);
    } // namespace script
} // namespace satdump