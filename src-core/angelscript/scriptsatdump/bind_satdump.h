#pragma once

#include "angelscript/angelscript.h"

namespace satdump
{
    namespace script
    {
        // TODOREWORK document
        void registerJson(asIScriptEngine *engine);
        void registerProj(asIScriptEngine *engine);
        void registerImage(asIScriptEngine *engine);

        inline void registerAll(asIScriptEngine *engine)
        {
            registerJson(engine);
            registerProj(engine);
            registerImage(engine);
        }
    }
}