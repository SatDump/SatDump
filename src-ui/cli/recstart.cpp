#include "recstart.h"
#include "core/plugin.h"
#include "explorer/explorer.h"
#include "recorder/recorder.h"

namespace satdump
{
    void RecStartCmdHandler::reg(CLI::App *app)
    {
        CLI::App *sub_module = app->add_subcommand("recstart", "Starts recorders on startup");
        sub_module->add_flag("--samplerate", samplerate, "Device samplerare");
        sub_module->add_flag("--start_recorder_device", start_recorder_device, "Start the device");
        sub_module->add_flag("--engage_autotrack", engage_autotrack, "Engage autotrack");
    }

    void RecStartCmdHandler::run(CLI::App *, CLI::App *subcom, bool)
    {
        nlohmann::json j;
        if (subcom->count("--samplerate"))
            j["samplerate"] = samplerate;
        if (subcom->count("--start_recorder_device"))
            j["start_recorder_device"] = start_recorder_device;
        if (subcom->count("--engage_autotrack"))
            j["engage_autotrack"] = engage_autotrack;

        std::shared_ptr<RecorderApplication> rec = std::make_shared<RecorderApplication>(j);

        eventBus->fire_event<explorer::ExplorerAddHandlerEvent>({rec, true});
    }
} // namespace satdump