#pragma once

#include <string>
#include <vector>

namespace satdump
{
    struct UnitInfo
    {
        std::string name;
        double scale;
    };

    const std::vector<UnitInfo> UNIT_METER = {
        {"Tm", 1e12},  //
        {"Gm", 1e9},   //
        {"Mm", 1e6},   //
        {"km", 1e3},   //
        {"hm", 1e2},   //
        {"dam", 1e1},  //
        {"m", 1},      //
        {"dm", 1e-1},  //
        {"cm", 1e-2},  //
        {"mm", 1e-3},  //
        {"um", 1e-6},  //
        {"nm", 1e-9},  //
        {"pm", 1e-12}, //
    };

    const std::vector<UnitInfo> UNIT_HERTZ = {
        {"THz", 1e12},  //
        {"GHz", 1e9},   //
        {"MHz", 1e6},   //
        {"kHz", 1e3},   //
        {"hHz", 1e2},   //
        {"daHz", 1e1},  //
        {"Hz", 1},      //
        {"dHz", 1e-1},  //
        {"cHz", 1e-2},  //
        {"mHz", 1e-3},  //
        {"uHz", 1e-6},  //
        {"nHz", 1e-9},  //
        {"pHz", 1e-12}, //
    };

    bool parseUnitFromString(std::string str, double &out, std::vector<UnitInfo> unit);
} // namespace satdump