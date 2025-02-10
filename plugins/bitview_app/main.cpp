#include "core/plugin.h"
#include "logger.h"
#include "core/module.h"

#include "app.h"
#include "bitview.h"

class BitViewAppPlugin : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "bitview_app";
    }

    void init()
    {
        //   satdump::eventBus->register_handler<satdump::AddGUIApplicationEvent>(provideAppToRegister);
        satdump::eventBus->register_handler<satdump::viewer::RequestHandlersEvent>(provideAppToRegister);
    }

    static void provideAppToRegister(const satdump::viewer::RequestHandlersEvent &evt)
    {
        evt.h.push_back(std::make_shared<satdump::BitViewHandler>());
        //        evt.applications.push_back(std::make_shared<satdump::BitViewApplication>());
    }
};

PLUGIN_LOADER(BitViewAppPlugin)