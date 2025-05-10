#pragma once

#include "angelscript/angelscript.h"
#include <string>

namespace satdump
{
    namespace script
    {
        // TODOREWORK document
        void registerLogger(asIScriptEngine *engine);
        void registerSystem(asIScriptEngine *engine);
        void registerJson(asIScriptEngine *engine);
        void registerGeodetic(asIScriptEngine *engine);
        void registerProj(asIScriptEngine *engine);
        void registerImage(asIScriptEngine *engine);

        template <typename T, typename Factory>
        void registerProduct(std::string name, asIScriptEngine *engine);
        void registerImageProduct(asIScriptEngine *engine);

        void registerAll(asIScriptEngine *engine);
    } // namespace script
} // namespace satdump