#include "baseband_interface.h"
#include "imgui/imgui.h"
#include "core/exception.h"

namespace dsp
{
    BasebandType::operator std::string() const
    {
        switch (type)
        {
        case CF_32:
            return "cf32";
            break;
        case CS_16:
            return "cs16";
            break;
        case CS_8:
            return "cs8";
            break;
        case CU_8:
            return "cu8";
            break;
        case WAV_16:
            return "wav";
            break;
#ifdef BUILD_ZIQ
        case ZIQ:
            return "ziq";
            break;
#endif
#ifdef BUILD_ZIQ2
        case ZIQ2:
            return "ziq";
            break;
#endif
        default:
            throw satdump_exception("Invalid baseband type!");
        };
    }

    void BasebandType::from_string(const std::string &s)
    {
        if (s == "cs16" || s == "s16")
            type = CS_16;
        else if (s == "cs8" || s == "s8")
            type = CS_8;
        else if (s == "cf32" || s == "f32")
            type = CF_32;
        else if (s == "cu8" || s == "u8")
            type = CU_8;
        else if (s == "w16" || s == "wav")
            type = WAV_16;
#ifdef BUILD_ZIQ
        else if (s == "ziq")
            type = ZIQ;
#endif
#ifdef BUILD_ZIQ2
        else if (s == "ziq2")
            type = ZIQ2;
#endif
        else
            throw satdump_exception("Unknown baseband type " + s);
    }

    bool BasebandType::drawPlaybackCombo()
    {
        int select = type;
        if (select >= WAV_16)
            select--;
        bool ret = ImGui::Combo("Format###basebandplayerformat", &select,
            "cf32\0"
            "cs16\0"
            "cs8\0"
            "cu8\0"
#ifdef BUILD_ZIQ
            "ziq\0"
#endif
#ifdef BUILD_ZIQ2
            "ziq2\0"
#endif
            "\0"
        );
        if (ret)
        {
            if (select >= WAV_16)
                select++;
            type = (BasebandTypeEnum)select;
        }
        return ret;
    }

    BasebandTypeEnum BasebandType::drawRecordCombo()
    {
        //TODO
        return type;
    }
}