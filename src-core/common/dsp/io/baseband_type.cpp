#include <map>
#include <vector>
#include "baseband_type.h"
#include "imgui/imgui.h"
#include "core/exception.h"

// Needed to properly use BasebandType as the key in std::map
#if defined(BUILD_ZIQ) || defined(BUILD_ZIQ2)
namespace std
{
    template<> struct less<dsp::BasebandType>
    {
        bool operator() (const dsp::BasebandType &lhs, const dsp::BasebandType &rhs) const
        {
            return lhs.type + lhs.ziq_depth < rhs.type + rhs.ziq_depth;
        }
    };
}
#endif

namespace dsp
{
    // Helper function
    static std::map<BasebandType, size_t> reverse_lut(const std::vector<BasebandType> lut)
    {
        std::map<BasebandType, size_t> ret;
        for (size_t i = 0; i < lut.size(); i++)
            ret[lut[i]] = i;
        return ret;
    }

    static const std::vector<BasebandType> play_fwd_lut = {
        CF_32,
        CS_32,
        CS_16,
        CS_8,
        CU_8,
#ifdef BUILD_ZIQ
        ZIQ,
#endif
#ifdef BUILD_ZIQ2
        ZIQ2
#endif
    };

    static const std::vector<BasebandType> record_fwd_lut = {
        CF_32,
        CS_32,
        CS_16,
        CS_8,
        WAV_16,
#ifdef BUILD_ZIQ
        { ZIQ, 32 },
        { ZIQ, 16 },
        { ZIQ, 8 },
#endif
#ifdef BUILD_ZIQ2
        { ZIQ2, 16 },
        { ZIQ2, 8 }
#endif
    };

    static const std::map<BasebandType, size_t> record_rev_lut = reverse_lut(record_fwd_lut);
    static const std::map<BasebandType, size_t> play_rev_lut = reverse_lut(play_fwd_lut);

    BasebandType::operator std::string() const
    {
        switch (type)
        {
        case CF_32:
            return "cf32";
            break;
        case CS_32:
            return "cs32";
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
            return "ziq2";
            break;
#endif
        default:
            throw satdump_exception("Invalid baseband type!");
        };
    }

    void BasebandType::from_string(const std::string &s)
    {
        if (s == "cs32" || s == "s32")
            type = CS_32;
        else if (s == "cs16" || s == "s16")
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

    bool BasebandType::draw_playback_combo(const char *label)
    {
        int selected = play_rev_lut.at(*this);
        bool ret = ImGui::Combo(label, &selected,
            "cf32\0"
            "cs32\0"
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
            *this = play_fwd_lut[selected];

        return ret;
    }

    bool BasebandType::draw_record_combo(const char *label)
    {
        int selected = record_rev_lut.at(*this);
        bool ret = ImGui::Combo(label, &selected,
            "cf32\0"
            "cs32\0"
            "cs16\0"
            "cs8\0"
            "wav16\0"
#ifdef BUILD_ZIQ
            "ziq cf32\0"
            "ziq cs16\0"
            "ziq cs8\0"
#endif
#ifdef BUILD_ZIQ2
            "ziq2 cs16 (WIP)\0"
            "ziq2 cs8 (WIP)\0"
#endif
            "\0"
        );

        if (ret)
            *this = record_fwd_lut[selected];

        return ret;
    }
}