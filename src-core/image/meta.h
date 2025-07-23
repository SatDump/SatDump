#pragma once

/**
 * @file meta.h
 * @brief Image Metadata manipulation
 */

#include "image.h"
#include "nlohmann/json.hpp"

namespace satdump
{
    namespace image
    {
        /**
         * @brief Check if an image has metadata
         * @param img the image
         * @return true if the image has metadata
         */
        bool has_metadata(Image &img);

        /**
         * @brief Set an image's metadata
         * @param img the image
         * @param metadata to set as JSON object
         */
        void set_metadata(Image &img, nlohmann::json metadata);

        /**
         * @brief Get an image's metadata
         * @param img the image
         * @return metadata as JSON object
         */
        nlohmann::json get_metadata(const Image &img);

        /**
         * @brief Delete the image's metadata
         * @param img the image
         */
        void free_metadata(const Image &img);

        /**
         * @brief Check if an image has a projection
         * @param img the image
         * @return true if the image has a projection
         */
        bool has_metadata_proj_cfg(Image &img);

        /**
         * @brief Set an image's projection metadata
         * @param img the image
         * @param proj_cfg Projection to set
         */
        void set_metadata_proj_cfg(Image &img, nlohmann::json proj_cfg);

        /**
         * @brief Get an image's projection
         * @param img the image
         * @return the projection as JSON
         */
        nlohmann::json get_metadata_proj_cfg(const Image &img);

        /**
         * @brief Image Calibration Handler.
         * This handles converting between raw pixel values
         * and calibrated/physical units. It is a simple linear
         * transform.
         *
         * @param unit calibrated unit (or name)
         * @param scale scale (a of ax+b function)
         * @param offset offset (b of ax+b function)
         */
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

            /**
             * @brief Convert from raw image value to calibration
             * @param img_val the raw pixel value
             * @return calibrated value
             */
            inline double getVal(double img_val) { return img_val * scale + offset; }

            /**
             * @brief Convert from calibrated value to raw image value
             * @param cal_val the calibrated value
             * @return raw pixel value
             */
            inline double setVal(double cal_val) { return (cal_val - offset) / scale; }
        };

        /**
         * @brief Check if an image has a calibration config
         * @param img the image
         * @return true if the image has calibration
         */
        bool has_metadata_calib_cfg(Image &img);

        /**
         * @brief Set an image's calibration metadata
         * @param img the image
         * @param proj_cfg calibration to set
         */
        void set_metadata_calib_cfg(Image &img, nlohmann::json calib_cfg);

        /**
         * @brief Get an image's calibration
         * @param img the image
         * @return the calibration as JSON
         */
        nlohmann::json get_metadata_calib_cfg(const Image &img);
    } // namespace image
} // namespace satdump