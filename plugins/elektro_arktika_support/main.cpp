#include "core/plugin.h"
#include "logger.h"
#include "core/module.h"

#include "elektro_arktika/instruments/msugs/module_msugs.h"
#include "elektro_arktika/lrit/module_elektro_lrit_data_decoder.h"

// #include "msugs_natural_color.h"
// #include "msugs_color_ir_merge.h"

class ElektroArktikaSupport : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "elektro_arktika_support";
    }

    void init()
    {
        satdump::eventBus->register_handler<RegisterModulesEvent>(registerPluginsHandler);
        logger->critical("NO MORE CPP COMPOSITES TODOREWORK"); //   satdump::eventBus->register_handler<satdump::RequestCppCompositeEvent>(provideCppCompositeHandler);
    }

    static void registerPluginsHandler(const RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, elektro_arktika::msugs::MSUGSDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, elektro::lrit::ELEKTROLRITDataDecoderModule);
    }

    // static void provideCppCompositeHandler(const satdump::RequestCppCompositeEvent &evt)
    // {
    //     if (evt.id == "msugs_natural_color")
    //         evt.compositors.push_back(elektro::msuGsNaturalColorCompositor);
    //     else if (evt.id == "msugs_color_ir_merge")
    //         evt.compositors.push_back(elektro::msuGsFalseColorIRMergeCompositor);
    // }
};

PLUGIN_LOADER(ElektroArktikaSupport)