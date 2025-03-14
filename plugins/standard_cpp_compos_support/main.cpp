#include "core/plugin.h"
#include "logger.h"
#include "core/module.h"

// #include "underlay_with_clouds.h"

class StandardCppCompos : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "standard_cpp_compos";
    }

    void init()
    {
        //    satdump::eventBus->register_handler<satdump::RequestCppCompositeEvent>(provideCppCompositeHandler);
        logger->critical("NO MORE CPP COMPOSITES!!!! For now? TODOREWORK REMOVE?");
    }

    // static void provideCppCompositeHandler(const satdump::RequestCppCompositeEvent &evt)
    // {
    //     // if (evt.id == "underlay_with_clouds")
    //     //     evt.compositors.push_back(cpp_compos::underlay_with_clouds);
    // }
};

PLUGIN_LOADER(StandardCppCompos)