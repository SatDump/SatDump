#include "core/module.h"
#include "core/plugin.h"
#include "logger.h"

#include "app.h"
#include "tracking.h"

class WipTrackingAppPlugin : public satdump::Plugin
{
public:
    std::string getID() { return "wiptracking_app"; }

    void init()
    {
        //   satdump::eventBus->register_handler<satdump::AddGUIApplicationEvent>(provideAppToRegister);
        satdump::eventBus->register_handler<satdump::viewer::RequestHandlersEvent>(provideAppToRegister);
    }

    static void provideAppToRegister(const satdump::viewer::RequestHandlersEvent &evt)
    {
        evt.h.push_back(std::make_shared<satdump::WipTrackingHandler>());
        //        evt.applications.push_back(std::make_shared<satdump::BitViewApplication>());
    }
};

PLUGIN_LOADER(WipTrackingAppPlugin)