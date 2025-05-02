#include "run_as.h"
#include "logger.h"

#include "angelscript/angelscript.h"
#include "angelscript/scriptarray/scriptarray.h"
#include "angelscript/scriptbuilder/scriptbuilder.h"
#include "angelscript/scriptstdstring/scriptstdstring.h"

#include "angelscript/scriptsatdump/bind_satdump.h"
#include "nlohmann/json.hpp"
#include <cstdio>

// TODOREWORK, this is all temporary
namespace satdump
{
    // Implement a simple message callback function
    void MessageCallback(const asSMessageInfo *msg, void *param)
    {
        bool lint = *((bool *)param);

        if (lint)
        {
            nlohmann::json l;

            l["type"] = "error";
            if (msg->type == asMSGTYPE_WARNING)
                l["type"] = "warn";
            else if (msg->type == asMSGTYPE_INFORMATION)
                l["type"] = "info";

            l["file"] = msg->section;
            l["line"] = msg->row;
            l["col"] = msg->col;
            l["msg"] = msg->message;

            printf("%s\n", l.dump().c_str());
        }
        else
        {
            const char *type = "ERR";
            if (msg->type == asMSGTYPE_WARNING)
                type = "WARN";
            else if (msg->type == asMSGTYPE_INFORMATION)
                type = "INFO";
            // printf("%s (%d, %d) : %s : %s\n", msg->section, msg->row, msg->col, type, msg->message);

            if (msg->type == asMSGTYPE_INFORMATION)
                logger->info("[AngelScript] %s (%d, %d) : %s : %s", msg->section, msg->row, msg->col, type, msg->message);
            else if (msg->type == asMSGTYPE_WARNING)
                logger->warn("[AngelScript] %s (%d, %d) : %s : %s", msg->section, msg->row, msg->col, type, msg->message);
            else if (msg->type == asMSGTYPE_ERROR)
                logger->error("[AngelScript] %s (%d, %d) : %s : %s", msg->section, msg->row, msg->col, type, msg->message);
        }
    }

    namespace namelog
    {
        void trace(std::string &f) { logger->trace("[AngelScript] " + f); }
        void debug(std::string &f) { logger->debug("[AngelScript] " + f); }
        void info(std::string &f) { logger->info("[AngelScript] " + f); }
        void warn(std::string &f) { logger->warn("[AngelScript] " + f); }
        void error(std::string &f) { logger->error("[AngelScript] " + f); }
        void critical(std::string &f) { logger->critical("[AngelScript] " + f); }
    } // namespace namelog

    int runAngelScript(std::string file, bool lint)
    {
        // Create the script engine
        asIScriptEngine *engine = asCreateScriptEngine();

        // Set the message callback to receive information on errors in human readable form.
        int r = engine->SetMessageCallback(asFUNCTION(MessageCallback), &lint, asCALL_CDECL);

        // AngelScript doesn't have a built-in string type, as there is no definite standard
        // string type for C++ applications. Every developer is free to register its own string type.
        // The SDK do however provide a standard add-on for registering a string type, so it's not
        // necessary to implement the registration yourself if you don't want to.
        RegisterStdString(engine);
        RegisterScriptArray(engine, true);
        satdump::script::registerAll(engine);

        // Register logger
        engine->SetDefaultNamespace("logger");
        engine->RegisterGlobalFunction("void trace(const string &in)", asFUNCTION(namelog::trace), asCALL_CDECL);
        engine->RegisterGlobalFunction("void debug(const string &in)", asFUNCTION(namelog::debug), asCALL_CDECL);
        engine->RegisterGlobalFunction("void info(const string &in)", asFUNCTION(namelog::info), asCALL_CDECL);
        engine->RegisterGlobalFunction("void warn(const string &in)", asFUNCTION(namelog::error), asCALL_CDECL);
        engine->RegisterGlobalFunction("void error(const string &in)", asFUNCTION(namelog::error), asCALL_CDECL);
        engine->RegisterGlobalFunction("void critical(const string &in)", asFUNCTION(namelog::critical), asCALL_CDECL);

        // The CScriptBuilder helper is an add-on that loads the file,
        // performs a pre-processing pass if necessary, and then tells
        // the engine to build a script module.
        CScriptBuilder builder;
        r = builder.StartNewModule(engine, "MyModule");
        if (r < 0)
        {
            // If the code fails here it is usually because there
            // is no more memory to allocate the module
            printf("Unrecoverable error while starting a new module.\n");
            return 1;
        }
        r = builder.AddSectionFromFile(file.c_str());
        if (r < 0)
        {
            // The builder wasn't able to load the file. Maybe the file
            // has been removed, or the wrong name was given, or some
            // preprocessing commands are incorrectly written.
            printf("Please correct the errors in the script and try again.\n");
            return 1;
        }
        r = builder.BuildModule();
        if (r < 0)
        {
            // An error occurred. Instruct the script writer to fix the
            // compilation errors that were listed in the output stream.
            printf("Please correct the errors in the script and try again.\n");
            return 1;
        }

        //////////////
        if (lint)
            return 0;

        // Find the function that is to be called.
        asIScriptModule *mod = engine->GetModule("MyModule");
        asIScriptFunction *func = mod->GetFunctionByDecl("void main()");
        if (func == 0)
        {
            // The function couldn't be found. Instruct the script writer
            // to include the expected function in the script.
            printf("The script must have the function 'void main()'. Please add it and try again.\n");
            return 1;
        }

        // Create our context, prepare it, and then execute
        asIScriptContext *ctx = engine->CreateContext();
        ctx->Prepare(func);
        r = ctx->Execute();
        if (r != asEXECUTION_FINISHED)
        {
            // The execution didn't complete as expected. Determine what happened.
            if (r == asEXECUTION_EXCEPTION)
            {
                // An exception occurred, let the script writer know what happened so it can be corrected.
                printf("An exception '%s' occurred. Please correct the code and try again.\n", ctx->GetExceptionString());
            }
        }

        // Clean up
        ctx->Release();
        engine->ShutDownAndRelease();

        return 0;
    }
} // namespace satdump