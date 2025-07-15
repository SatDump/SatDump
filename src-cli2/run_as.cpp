#include "run_as.h"
#include "angelscript/scriptsatdump/dump_predef.h"
#include "angelscript/scriptsatdump/helper.h"
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
    struct JSONDiags
    {
        bool lint;
        int ndiags = 0;
        nlohmann::json all_diags;
    };

    // Implement a simple message callback function
    void MessageCallback(const asSMessageInfo *msg, void *param)
    {
        JSONDiags *p = (JSONDiags *)param;

        if (p->lint)
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

            p->all_diags["diags"][p->ndiags++] = l;
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

    void ScriptCmdHandler::reg(CLI::App *app)
    {
        CLI::App *sub_run = app->add_subcommand("run", "Run a script");
        sub_run->add_option("script", "The script to run")->required();
        // auto sub_run_group = sub_run->add_option_group("--lua,--as");
        // sub_run_group->add_flag("--lua", "Run a Lua script");
        // sub_run_group->add_flag("--as", "Run an AngelScript script");
        // sub_run_group->require_option(1); TODOREWORK
        sub_run->add_flag("--lint", "Lint the script, without executing it");
        sub_run->add_flag("--predef", "Dump as.predefined");
        sub_run->require_option(1, 3);
    }

    void ScriptCmdHandler::run(CLI::App *app, CLI::App *subcom)
    {
        std::string script = subcom->get_option("script")->as<std::string>();
        runAngelScript(script, subcom->count("--lint"), subcom->count("--predef"));
    }

    int ScriptCmdHandler::runAngelScript(std::string file, bool lint, bool predef)
    {
        // Create the script engine
        asIScriptEngine *engine = asCreateScriptEngine();

        // Set the message callback to receive information on errors in human readable form.
        JSONDiags diags;
        diags.lint = lint;
        diags.all_diags["file"] = file;
        engine->SetMessageCallback(asFUNCTION(MessageCallback), &diags, asCALL_CDECL);

        // AngelScript doesn't have a built-in string type, as there is no definite standard
        // string type for C++ applications. Every developer is free to register its own string type.
        // The SDK do however provide a standard add-on for registering a string type, so it's not
        // necessary to implement the registration yourself if you don't want to.
        satdump::script::registerAll(engine);

        if (predef)
            script::GenerateScriptPredefined(engine, "../as.predefined");

        if (!script::buildModuleFromFile(engine, "script", file))
            return 1;

        if (lint)
        {
            printf("%s\n", diags.all_diags.dump(4).c_str());
            return 0;
        }

        if (!script::runVoidMainFromModule(engine, "script"))
            return 1;

        // Clean up
        engine->ShutDownAndRelease();

        return 0;
    }
} // namespace satdump