#include "plugin.h"
#include "logger.h"

class Sample : public satdump::Plugin
{
public:
    std::string getID()
    {
        return "sample";
    }

    void init()
    {
        logger->info("Sample plugin!");
    }
};

PLUGIN_LOADER(Sample)