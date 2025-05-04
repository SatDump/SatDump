#pragma once

#include "angelscript/angelscript.h"
#include "angelscript/scriptarray/scriptarray.h"
#include "angelscript/scriptstdstring/scriptstdstring.h"

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

        inline void registerAll(asIScriptEngine *engine)
        {
            RegisterStdString(engine);
            RegisterScriptArray(engine, true);

            registerLogger(engine);
            registerSystem(engine);
            registerJson(engine);
            registerGeodetic(engine);
            registerProj(engine);
            registerImage(engine);
        }
    } // namespace script
} // namespace satdump