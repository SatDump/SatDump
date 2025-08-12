#include "bind_satdump.h"

#include "angelscript/scriptarray/scriptarray.h"
#include "angelscript/scriptdictionary/scriptdictionary.h"
#include "angelscript/scriptstdstring/scriptstdstring.h"
#include "bind_product.h"

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

        template <typename T>
        void registerProduct(std::string name, asIScriptEngine *engine);
        void registerImageProduct(asIScriptEngine *engine);

        void registerAll(asIScriptEngine *engine)
        {
            RegisterStdString(engine);
            RegisterScriptArray(engine, true);
            RegisterScriptDictionary(engine);

            registerLogger(engine);
            registerSystem(engine);
            registerJson(engine);
            registerGeodetic(engine);
            registerImage(engine);
            registerProj(engine);

            registerProduct<CScriptProduct>("Product", engine);
            registerImageProduct(engine);
        }
    } // namespace script
} // namespace satdump