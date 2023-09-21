#include "core/plugin.h"
#include "logger.h"

#include "core/pipeline.h"

#include <filesystem>
#include "libs/sol2/sol.hpp"
#include "init.h"

void pipeline_done_processing_callback(const satdump::events::PipelineDoneProcessingEvent &evt)
{
    std::string script_path = satdump::user_path + "/scripts/pipeline_done_processing.lua";

    if (std::filesystem::exists(script_path))
    {
        logger->trace("Calling " + script_path);

        try
        {
            sol::state lua;

            lua.open_libraries(sol::lib::base);
            lua.open_libraries(sol::lib::string);
            lua.open_libraries(sol::lib::math);
            lua.open_libraries(sol::lib::io);

            lua["pipeline_id"] = evt.pipeline_id;
            lua["pipeline_output_directory"] = evt.output_directory;

            lua.script_file(script_path);
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
    std::string getID()
    {
        return "scripting_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<satdump::events::PipelineDoneProcessingEvent>(pipeline_done_processing_callback);
    }
};

PLUGIN_LOADER(ScriptingSupport)