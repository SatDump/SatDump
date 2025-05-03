#include "core/module.h"
#include "core/plugin.h"
#include "logger.h"

#include "elektro_arktika/instruments/msugs/module_msugs.h"
#include "elektro_arktika/lrit/module_elektro_lrit_data_decoder.h"

class ElektroArktikaSupport : public satdump::Plugin
{
public:
    std::string getID() { return "elektro_arktika_support"; }

    void init() { satdump::eventBus->register_handler<RegisterModulesEvent>(registerPluginsHandler); }

    static void registerPluginsHandler(const RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, elektro_arktika::msugs::MSUGSDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, elektro::lrit::ELEKTROLRITDataDecoderModule);
    }
};

PLUGIN_LOADER(ElektroArktikaSupport)