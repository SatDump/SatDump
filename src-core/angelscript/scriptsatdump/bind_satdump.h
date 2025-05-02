#pragma once

#include "angelscript/angelscript.h"

namespace satdump
{
    namespace script
    {
        // TODOREWORK document
        void registerSystem(asIScriptEngine *engine);
        void registerJson(asIScriptEngine *engine);
        void registerGeodetic(asIScriptEngine *engine);
        void registerProj(asIScriptEngine *engine);
        void registerImage(asIScriptEngine *engine);

        inline void registerAll(asIScriptEngine *engine)
        {
            registerSystem(engine);
            registerJson(engine);
            registerGeodetic(engine);
            registerProj(engine);
            registerImage(engine);
        }
    } // namespace script
} // namespace satdump