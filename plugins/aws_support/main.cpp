#include "core/plugin.h"
#include "logger.h"

#include "aws/module_aws_instruments.h"

class AWSSupport : public satdump::Plugin
{
public:
    std::string getID() { return "aws_support"; }

    void init() { satdump::eventBus->register_handler<satdump::pipeline::RegisterModulesEvent>(registerPluginsHandler); }

    static void registerPluginsHandler(const satdump::pipeline::RegisterModulesEvent &evt) { REGISTER_MODULE_EXTERNAL(evt.modules_registry, aws::AWSInstrumentsDecoderModule); }
};

PLUGIN_LOADER(AWSSupport)