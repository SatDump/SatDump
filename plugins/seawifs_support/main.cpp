#include "core/plugin.h"
#include "seawifs/module_seawifs_decoder.h"

class SeaWIFsSupport : public satdump::Plugin {
public:
    std::string getID() { return "seawifs_support"; }

    void init() { satdump::eventBus->register_handler<satdump::pipeline::RegisterModulesEvent>(registerPluginsHandler); }

    static void registerPluginsHandler(const satdump::pipeline::RegisterModulesEvent &evt) { REGISTER_MODULE_EXTERNAL(evt.modules_registry, seawifs::SeaWIFsProcessingModule); }
};

PLUGIN_LOADER(SeaWIFsSupport)