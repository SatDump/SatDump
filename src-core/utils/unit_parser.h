#pragma once

/**
 * @file unit_parser.h
 * @brief Utilities to help parse physical units
 */

#include <string>
#include <vector>

namespace satdump
{
    /**
     * @brief Unit information struct, that must be provided
     * in a vector with all possible options and their corresponding
     * power.
     */
    struct UnitInfo
    {
        std::string name;
        double scale;
    };

    /**
     * @brief Unit declaration (to be used in parseUnitFromString) for
     * Meters
     */
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

    /**
     * @brief Unit declaration (to be used in parseUnitFromString) for
     * Hertz
     */
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

    /**
     * @brief Parse an unit down to its SI base (eg, Hz)
     * from a string, for user-friendly input
     *
     * @param str input string
     * @param out output value
     * @param unit unit to parse (must contain all expected representations)
     * @return true if parsing was sucessful
     */
    bool parseUnitFromString(std::string str, double &out, std::vector<UnitInfo> unit);
} // namespace satdump