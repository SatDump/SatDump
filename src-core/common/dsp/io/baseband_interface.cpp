#include "baseband_interface.h"

namespace dsp
{
    BasebandType basebandTypeFromString(std::string type)
    {
        if (type == "s16")
            return IS_16;
        else if (type == "s8")
            return IS_8;
        else if (type == "f32")
            return CF_32;
        else if (type == "u8")
            return IU_8;
        else if (type == "w16")
            return WAV_16;
#ifdef BUILD_ZIQ
        else if (type == "ziq")
            return ZIQ;
#endif
        else if (type == "ziq2")
            return ZIQ2;
        else
            throw std::runtime_error("Unknown baseband type " + type);
    }
}