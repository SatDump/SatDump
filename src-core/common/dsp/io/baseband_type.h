#pragma once

#include <string>

namespace dsp
{
	    enum BasebandTypeEnum
    {
        CF_32,
        CS_32,
        CS_16,
        CS_8,
        CU_8,
        WAV_16,
#ifdef BUILD_ZIQ
        ZIQ,
#endif
#ifdef BUILD_ZIQ2
        ZIQ2,
#endif
    };

    class BasebandType
    {
    private:
        BasebandTypeEnum type;
        void from_string(const std::string &s);

    public:
        BasebandType() { type = CF_32; }
        BasebandType(const BasebandTypeEnum &e) { type = e; }
        BasebandType(const std::string &s) { from_string(s); }
        BasebandType(const char *s) { from_string(std::string(s)); }
        inline operator BasebandTypeEnum() const { return type; }
        inline BasebandType &operator= (const BasebandTypeEnum &e) { type = e; return *this; }
        inline BasebandType &operator= (const std::string &s) { from_string(s); return *this; }
        inline BasebandType &operator= (const char *s) { from_string(std::string(s)); return *this; }
        operator std::string() const;

        bool draw_playback_combo(const char *label = "Format");
        bool draw_record_combo(const char *label = "Format");

#if defined(BUILD_ZIQ) || defined(BUILD_ZIQ2)
        int ziq_depth = 8;
        BasebandType(BasebandTypeEnum e, int d) { type = e; ziq_depth = d; }
        friend struct std::less<BasebandType>;
#endif
    };
}