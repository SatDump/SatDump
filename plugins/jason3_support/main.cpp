#include "plugin.h"
#include "logger.h"
#include "module.h"

#include "jason3/instruments/poseidon/module_jason3_poseidon.h"
#include "jason3/instruments/amr2/module_jason3_amr2.h"
#include "jason3/instruments/lpt/module_jason3_lpt.h"

class Jason3Support : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "jason3_support";
    }

    void init()
    {
        logger->info("Jason3Support plugin loaded!");

        satdump::eventBus->register_handler<RegisterModulesEvent>(registerPluginsHandler);
    }

    static void registerPluginsHandler(const RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, jason3::poseidon::Jason3PoseidonDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, jason3::amr2::Jason3AMR2DecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, jason3::lpt::Jason3LPTDecoderModule);
    }
};

PLUGIN_LOADER(Jason3Support)