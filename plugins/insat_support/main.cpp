#include "core/plugin.h"
#include "logger.h"

#include "insat/module_insat_imager_decoder.h"

class INSATSupport : public satdump::Plugin
{
public:
    std::string getID() { return "insat_support"; }

    void init() { satdump::eventBus->register_handler<satdump::pipeline::RegisterModulesEvent>(registerPluginsHandler); }

    static void registerPluginsHandler(const satdump::pipeline::RegisterModulesEvent &evt) { REGISTER_MODULE_EXTERNAL(evt.modules_registry, insat::INSATImagerDecoderModule); }
};

PLUGIN_LOADER(INSATSupport)