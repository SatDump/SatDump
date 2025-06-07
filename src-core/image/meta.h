#pragma once

#include "image.h"
#include "nlohmann/json.hpp"

namespace satdump
{
    namespace image
    {
        bool has_metadata(Image &img);
        void set_metadata(Image &img, nlohmann::json metadata);
        nlohmann::json get_metadata(const Image &img);
        void free_metadata(const Image &img);

        bool has_metadata_proj_cfg(Image &img);
        void set_metadata_proj_cfg(Image &img, nlohmann::json proj_cfg);
        nlohmann::json get_metadata_proj_cfg(const Image &img);

        struct ImgCalibHandler
        {
            std::string unit;
            double scale = 0;
            double offset = 0;

            double min = 0, max = 0;

            friend void to_json(nlohmann::json &j, const ImgCalibHandler &v)
            {
                j["unit"] = v.unit;

                if (v.offset == 0 && v.scale == 0)
                {
                    j["offset"] = v.min;
                    j["scale"] = v.max - v.min;
                }
                else
                {
                    j["offset"] = v.offset;
                    j["scale"] = v.scale;
                }
            }

            friend void from_json(const nlohmann::json &j, ImgCalibHandler &v)
            {
                v.unit = j["unit"];
                if (j.contains("offset"))
                    v.offset = j["offset"];
                if (j.contains("scale"))
                    v.scale = j["scale"];

                v.min = v.offset;
                v.max = v.offset + v.scale;
            }

            inline double getVal(double img_val) { return img_val * scale + offset; }

            inline double setVal(double cal_val) { return (cal_val - offset) / scale; }
        };

        bool has_metadata_calib_cfg(Image &img);
        void set_metadata_calib_cfg(Image &img, nlohmann::json calib_cfg);
        nlohmann::json get_metadata_calib_cfg(const Image &img);
    } // namespace image
} // namespace satdump