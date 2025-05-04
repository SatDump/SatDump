#include "helper.h"

#include "angelscript/scriptbuilder/scriptbuilder.h"
#include "logger.h"
#include <string>

namespace satdump
{
    namespace script
    {
        bool buildModuleFromFile(asIScriptEngine *engine, std::string module_id, std::string script_file)
        {
            // The CScriptBuilder helper is an add-on that loads the file,
            // performs a pre-processing pass if necessary, and then tells
            // the engine to build a script module.
            CScriptBuilder builder;
            int r = builder.StartNewModule(engine, module_id.c_str());
            if (r < 0)
            {
                // If the code fails here it is usually because there
                // is no more memory to allocate the module
                logger->error("Unrecoverable error while starting a new module.");
                return false;
            }

            r = builder.AddSectionFromFile(script_file.c_str());
            if (r < 0)
            {
                // The builder wasn't able to load the file. Maybe the file
                // has been removed, or the wrong name was given, or some
                // preprocessing commands are incorrectly written.
                logger->error("Please correct the errors in the script and try again.");
                return false;
            }

            r = builder.BuildModule();
            if (r < 0)
            {
                // An error occurred. Instruct the script writer to fix the
                // compilation errors that were listed in the output stream.
                logger->error("Please correct the errors in the script and try again.");
                return false;
            }

            return true;
        }

        bool runVoidMainFromModule(asIScriptEngine *engine, std::string module_id)
        {
            // Find the function that is to be called.
            asIScriptModule *mod = engine->GetModule(module_id.c_str());
            asIScriptFunction *func = mod->GetFunctionByDecl("void main()");
            if (func == 0)
            {
                // The function couldn't be found. Instruct the script writer
                // to include the expected function in the script.
                logger->error("The script must have the function 'void main()'. Please add it and try again.");
                return false;
            }

            // Create our context, prepare it, and then execute
            asIScriptContext *ctx = engine->CreateContext();
            ctx->Prepare(func);
            int r = ctx->Execute();
            if (r != asEXECUTION_FINISHED)
            {
                // The execution didn't complete as expected. Determine what happened.
                if (r == asEXECUTION_EXCEPTION)
                {
                    // An exception occurred, let the script writer know what happened so it can be corrected.
                    logger->error("An exception '%s' occurred. Please correct the code and try again.", ctx->GetExceptionString());
                }
            }

            // Clean up
            ctx->Release();

            return true;
        }
    } // namespace script
} // namespace satdump