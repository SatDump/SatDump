#pragma once

#include "proj.h"
#include "nlohmann/json.hpp"

namespace proj
{
    inline void to_json(nlohmann::json &j, const projection_t &p)
    {
        if (p.type == ProjType_Equirectangular)
            j["type"] = "equirec";
        else if (p.type == ProjType_Stereographic)
            j["type"] = "stereo";
        else if (p.type == ProjType_UniversalTransverseMercator)
            j["type"] = "utm";
        else if (p.type == ProjType_Geos)
            j["type"] = "geos";
        // else if (p.type == ProjType_LambertConformalConic)
        //     j["type"] = "lamcc";

        if (p.type == ProjType_UniversalTransverseMercator)
        {
            j["zone"] = p.params.zone;
            j["south"] = p.params.south;
        }

        if (p.type == ProjType_Geos)
        {
            j["altitude"] = p.params.altitude;
            j["sweep_x"] = p.params.sweep_x;
        }

        if (p.proj_offset_x != 0)
            j["offset_x"] = p.proj_offset_x;
        if (p.proj_offset_y != 0)
            j["offset_y"] = p.proj_offset_y;
        if (p.proj_scalar_x != 1)
            j["scalar_x"] = p.proj_scalar_x;
        if (p.proj_scalar_y != 1)
            j["scalar_y"] = p.proj_scalar_y;

        if (p.lam0 != 0)
            j["lon0"] = p.lam0 * RAD2DEG;
        if (p.phi0 != 0)
            j["lat0"] = p.phi0 * RAD2DEG;
    }

    inline void from_json(const nlohmann::json &j, projection_t &p)
    {
        if (j["type"].get<std::string>() == "equirec")
            p.type = ProjType_Equirectangular;
        else if (j["type"].get<std::string>() == "stereo")
            p.type = ProjType_Stereographic;
        else if (j["type"].get<std::string>() == "utm")
            p.type = ProjType_UniversalTransverseMercator;
        else if (j["type"].get<std::string>() == "geos")
            p.type = ProjType_Geos;
        // else if (j["type"].get<std::string>() == "lamcc")
        //     p.type = ProjType_LambertConformalConic;

        if (p.type == ProjType_UniversalTransverseMercator)
        {
            if (j.contains("zone"))
                p.params.zone = j["zone"];
            if (j.contains("south"))
                p.params.south = j["south"];
        }

        if (p.type == ProjType_Geos)
        {
            if (j.contains("altitude"))
                p.params.altitude = j["altitude"];
            if (j.contains("sweep_x"))
                p.params.sweep_x = j["sweep_x"];
        }

        if (j.contains("offset_x"))
            p.proj_offset_x = j["offset_x"];
        if (j.contains("offset_y"))
            p.proj_offset_y = j["offset_y"];
        if (j.contains("scalar_x"))
            p.proj_scalar_x = j["scalar_x"];
        if (j.contains("scalar_y"))
            p.proj_scalar_y = j["scalar_y"];

        if (j.contains("lon0"))
            p.lam0 = j["lon0"].get<double>() * DEG2RAD;
        if (j.contains("lat0"))
            p.phi0 = j["lat0"].get<double>() * DEG2RAD;
    }
}