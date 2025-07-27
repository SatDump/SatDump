#include "core/plugin.h"

class XRITSupport : public satdump::Plugin
{
public:
    std::string getID() { return "xrit_support"; }

    void init() {}
};

PLUGIN_LOADER(XRITSupport)