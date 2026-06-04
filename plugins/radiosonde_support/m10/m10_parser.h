#pragma once

#include "common/simple_deframer.h"
#include "dsp/block.h"
#include <cstdint>

namespace satdump
{
    namespace ndsp
    {
        class M10ParserBlock : public Block
        {
        private:
            uint8_t frame_shifter[204];

            std::string time;
            double lat, lon, alt;

        private:
            bool work();

        public:
            M10ParserBlock();
            ~M10ParserBlock();

            void init();

            nlohmann::ordered_json get_cfg_list()
            {
                nlohmann::ordered_json p;
                add_param_simple(p, "time", "stat");
                add_param_simple(p, "lat", "stat");
                add_param_simple(p, "lon", "stat");
                add_param_simple(p, "alt", "stat");
                return p;
            }

            nlohmann::json get_cfg(std::string key)
            {
                if (key == "time")
                    return time;
                else if (key == "lat")
                    return lat;
                else if (key == "lon")
                    return lon;
                else if (key == "alt")
                    return alt;
                else
                    throw satdump_exception(key);
            }

            cfg_res_t set_cfg(std::string key, nlohmann::json v)
            {
                throw satdump_exception(key);
                return RES_OK;
            }
        };
    } // namespace ndsp
} // namespace satdump