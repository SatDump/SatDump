#include "baseband_interface.h"
#include "core/exception.h"

namespace dsp
{
    BasebandType basebandTypeFromString(std::string type)
    {
        if (type == "cs16")
            return CS_16;
        else if (type == "cs8")
            return CS_8;
        else if (type == "cf32")
            return CF_32;
        else if (type == "cu8")
            return CU_8;
        else if (type == "w16")
            return WAV_16;
#ifdef BUILD_ZIQ
        else if (type == "ziq")
            return ZIQ;
#endif
        else if (type == "ziq2")
            return ZIQ2;
        else
            throw satdump_exception("Unknown baseband type " + type);
    }
}