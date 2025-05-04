#include "core/plugin.h"
#include "logger.h"

#include "core/pipeline.h"

#include "angelscript/scriptsatdump/bind_satdump.h"
#include "angelscript/scriptsatdump/helper.h"
#include "init.h"
#include <filesystem>

#include "core/config.h"

bool scription_plugin_trigger_pipeline_done_processing = true;

void pipeline_done_processing_callback(const satdump::events::PipelineDoneProcessingEvent &evt)
{
    if (!scription_plugin_trigger_pipeline_done_processing)
        return;

    std::string script_path = satdump::user_path + "/scripts/pipeline_done_processing.as";

    if (std::filesystem::exists(script_path))
    {
        logger->trace("Calling " + script_path);

        try
        {
            // TODOREWORK, maybe switch to using a function instead?
            asIScriptEngine *engine = asCreateScriptEngine();

            satdump::script::registerAll(engine);

            engine->SetDefaultNamespace("");
            engine->RegisterGlobalProperty("string pipeline_id", (void *)&evt.pipeline_id);
            engine->RegisterGlobalProperty("string pipeline_output_directory", (void *)&evt.output_directory);

            if (!satdump::script::buildModuleFromFile(engine, "script", script_path))
                return;

            if (!satdump::script::runVoidMainFromModule(engine, "script"))
                return;

            engine->ShutDownAndRelease();
        }
        catch (std::exception &e)
        {
            logger->error("Error calling event script! %s", e.what());
        }
    }
}

class ScriptingSupport : public satdump::Plugin
{
public:
    std::string getID() { return "scripting_support"; }

    void init()
    {
        satdump::eventBus->register_handler<satdump::events::PipelineDoneProcessingEvent>(pipeline_done_processing_callback);
        satdump::eventBus->register_handler<satdump::config::RegisterPluginConfigHandlersEvent>(registerConfigHandler);
    }

    static void renderConfig() { ImGui::Checkbox("Trigger Pipeline Done Processing", &scription_plugin_trigger_pipeline_done_processing); }

    static void save() {}

    static void registerConfigHandler(const satdump::config::RegisterPluginConfigHandlersEvent &evt)
    {
        evt.plugin_config_handlers.push_back({"Scripting Plugin", ScriptingSupport::renderConfig, ScriptingSupport::save});
    }
};

PLUGIN_LOADER(ScriptingSupport)