#include "core/plugin.h"
#include "logger.h"

#include "apid/apid_sender.h"
#include "pipeline/module.h"

class APIDSender : public satdump::Plugin
{
public:
    std::string getID() { return "apid_sender"; }

    void init() { satdump::eventBus->register_handler<satdump::pipeline::RegisterModulesEvent>(registerPluginsHandler); }

    // static void registerPluginsHandler(const satdump::pipeline::RegisterModulesEvent &evt) { REGISTER_MODULE_EXTERNAL(evt.modules_registry, ssdv::instruments::SSDVInstrumentsDecoderModule); }
    static void registerPluginsHandler(const satdump::pipeline::RegisterModulesEvent &evt) { REGISTER_MODULE_EXTERNAL(evt.modules_registry, apid::APIDSenderModule); }
};

PLUGIN_LOADER(APIDSender)