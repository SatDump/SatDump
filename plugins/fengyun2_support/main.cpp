#include "core/plugin.h"
#include "logger.h"

#include "fengyun2/svissr/module_svissr_decoder.h"
#include "fengyun2/svissr/module_svissr_image_decoder.h"

class FengYun2Support : public satdump::Plugin
{
public:
    std::string getID() { return "fengyun2_support"; }

    void init() { satdump::eventBus->register_handler<satdump::pipeline::RegisterModulesEvent>(registerPluginsHandler); }

    static void registerPluginsHandler(const satdump::pipeline::RegisterModulesEvent &evt)
    {
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, fengyun_svissr::SVISSRDecoderModule);
        REGISTER_MODULE_EXTERNAL(evt.modules_registry, fengyun_svissr::SVISSRImageDecoderModule);
    }
};

PLUGIN_LOADER(FengYun2Support)