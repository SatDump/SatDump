#pragma once

/**
 * @file image_product.h
 * @brief ImageProduct implementation
 */

#include "image/channel_transform.h"
#include "image/image.h"
#include "product.h"
#include "utils/stats.h"

#include "common/physics_constants.h"
#include "image/calibration_units.h" // TODOREWORK MOVE!!!! + Conversion

// TODOREWORK MOVE
#include "common/calibration.h"
#include "utils/unit_parser.h"
#include <string>

namespace satdump
{
    namespace products
    {
        /**
         * @brief SatDump image product class.
         *
         * This is mainly intended for Earth imaging instruments, with one or more
         * spectral channel (AVHRR, MODIS...).
         *
         * It contains and allows if possible :
         * - Geographic referencing
         * - Spectral calibration or other physical units
         * - Dynamic correlation between channels with transforms
         *
         * @param images raw image structs containing channel descriptions and images
         * @param save_as_matrix if enabled, saves all image channels in a single matrix. All images MUST be of the exact same resolution (useful for eg, IASI)
         * @param d_no_not_save_images advanced parameter, telling it to save the product without saving the image channels, mainly meant for saving in async ways
         * @param d_no_not_load_images advanced parameter, telling it not to load images but only the CBOR, mainly meant to edit it more efficiently
         */
        class ImageProduct : public Product
        {
        public:
            /**
             * @brief Struct holding both the image
             * and some metadata
             *
             * @param abs_index Absolute image index, for calibration, etc. -2 means no calibration possible on this channel
             * @param filename Filename & path of the image. Relative to the product file
             * @param channel_name Name of the channel, this is usually just a number but can be a string
             * @param image The image itself
             * @param bit_depth Image bit depth
             * @param ch_transform Channel transform to reference field
             * @param wavenumber Wavenumber
             * @param calibration_type optiona, calibration output type, goes hand-in-hand with unit, but do note
             * a calibration type enforces a specific unit! If this isn't a standard ID, specify your unit here instead
             */
            struct ImageHolder
            {
                int abs_index = -1;
                std::string filename;
                std::string channel_name;
                image::Image image;
                int bit_depth = 16;
                ChannelTransform ch_transform = ChannelTransform().init_none();
                double wavenumber = -1;
                std::string calibration_type = "";
            };

            std::vector<ImageHolder> images;

            bool save_as_matrix = false;

        public:
            /**
             * @brief Set geo projection config in the product
             * @param cfg projection config. Must contain all applicable metadata
             */
            void set_proj_cfg(nlohmann::json cfg)
            {
                contents["projection_cfg"] = cfg;
                if (cfg.contains("tle") && cfg["tle"].contains("name"))
                    if (!has_product_source())
                        set_product_source(cfg["tle"]["name"]);
                if (cfg.contains("timestamps") && !has_product_timestamp())
                    set_product_timestamp(get_median(cfg["timestamps"].get<std::vector<double>>()));
            }

            /**
             * @brief Set geo projection config in the product, with a TLE and timestamps
             * @param cfg projection config.
             * @param tle the TLE
             * @param timestamps the timestamps
             */
            void set_proj_cfg_tle_timestamps(nlohmann::json cfg, nlohmann::json tle, nlohmann::json timestamps)
            {
                cfg["tle"] = tle;
                cfg["timestamps"] = timestamps;
                set_proj_cfg(cfg);
            }

            /**
             * @brief Get geo projection config in the product, if present
             * @param channel channel absolute index TODOREWORK
             * @return projection config as JSON object
             */
            nlohmann::json get_proj_cfg(int channel)
            {
                // TODO CHANNEL SPECIFICS
                auto cfg = contents["projection_cfg"];
                cfg["transform"] = get_channel_image(channel).ch_transform;
                cfg["width"] = get_channel_image(channel).image.width();
                cfg["height"] = get_channel_image(channel).image.height();
                if (has_product_timestamp())
                    cfg["proj_timestamp"] = get_product_timestamp();
                return cfg;
            }

            /**
             * @brief Check if geo projection info is present
             * @return true if present
             */
            bool has_proj_cfg() { return contents.contains("projection_cfg"); }

        public:
            void set_calibration(std::string calibrator, nlohmann::json cfg)
            {
                contents["calibration"] = cfg;
                contents["calibration"]["calibrator"] = calibrator;
            }

            std::pair<std::string, nlohmann::json> get_calibration() { return {contents["calibration"]["calibrator"], contents["calibration"]}; }

            /**
             * @brief Check if calibration info is present
             * @return true if present
             */
            bool has_calibration() { return contents.contains("calibration"); }

            // TODOREWORK DOCUMENT
            nlohmann::json get_calibration_raw() { return contents.contains("calibration") ? contents["calibration"] : nlohmann::json(); }

        public:
            /**
             * @brief Get image channel by absolute ID
             * @param index absolute channel index
             * @return the image channel struct
             */
            ImageHolder &get_channel_image(int index)
            {
                for (auto &img : images)
                    if (img.abs_index == index)
                        return img;
                throw satdump_exception("Product Channel Index " + std::to_string(index) + " is not present!");
            }

            /**
             * @brief Get image channel by name
             * @param name channel name
             * @return the image channel struct
             */
            ImageHolder &get_channel_image(std::string name)
            {
                for (auto &img : images)
                    if (img.channel_name == name)
                        return img;
                throw satdump_exception("Product Channel Name " + name + " is not present!");
            }

            /**
             * @brief Get image channel by wavenumber. Returns the closest one
             * @param double wavenumber
             * @return the image channel struct
             */
            ImageHolder &get_channel_image_by_wavenumber(double wavenumber)
            {
                double best = 1e40;
                ImageHolder *out = nullptr;
                for (auto &img : images)
                {
                    if (img.wavenumber != -1 && best > abs(img.wavenumber - wavenumber))
                    {
                        out = &img;
                        best = abs(img.wavenumber - wavenumber);
                    }
                }
                if (out == nullptr)
                    throw satdump_exception("Product Channel Close to Wavelength " + std::to_string(wavenumber) + " is not present!");
                return *out;
            }

            /**
             * @brief Get image channel by unit string. Returns the closest one
             * @param str Unit string
             * @return the image channel struct
             */
            ImageHolder &get_channel_image_by_unitstr(std::string str)
            {
                double val = 0;

                if (parseUnitFromString(str, val, UNIT_METER))
                    val = freq_to_wavenumber(SPEED_OF_LIGHT_M_S / val);
                else if (parseUnitFromString(str, val, UNIT_HERTZ))
                    val = freq_to_wavenumber(val);
                else
                    throw satdump_exception("Couldn't parse unit and value from " + str);

                return get_channel_image_by_wavenumber(val);
            }

            /**
             * @brief Get image channel raw ID by name
             * @param name channel name
             * @return the image channel ID (NOT abs_id!!)
             */
            int get_channel_image_idx(std::string name)
            {
                for (int i = 0; i < (int)images.size(); i++)
                    if (images[i].channel_name == name)
                        return i;
                throw satdump_exception("Product Channel Name " + name + " is not present!");
            }

            /**
             * @brief Get raw channel count
             * @param idx channel index (NOT abs_idx!!!)
             * @param x position in image
             * @param y position in image
             */
            inline int get_raw_channel_val(int idx, int x, int y)
            {
                auto &i = images[idx];
                int depth_diff = i.bit_depth - i.image.depth();
                if (depth_diff > 0)
                    return i.image.get(0, x, y) << depth_diff;
                else
                    return i.image.get(0, x, y) >> -depth_diff;
            }

            /**
             * @brief Set channel wavenumber
             * @param index absolute channel index
             * @param wavenumber wavenumber value
             */
            void set_channel_wavenumber(int index, double wavenumber)
            {
                for (auto &img : images)
                    if (img.abs_index == index)
                    {
                        img.wavenumber = wavenumber;
                        return;
                    }
                throw satdump_exception("Product Channel Index " + std::to_string(index) + " is not present!");
            }

            /**
             * @brief Set channel frequency (internally, wavenumber)
             * @param index absolute channel index
             * @param frequency frequency value, in Hz
             */
            void set_channel_frequency(int index, double frequency) { set_channel_wavenumber(index, freq_to_wavenumber(frequency)); }

            /**
             * @brief Get channel wavenumber
             * @param index absolute channel index
             * @return channel wavenumber
             */
            double get_channel_wavenumber(int index)
            {
                for (auto &img : images)
                    if (img.abs_index == index)
                        return img.wavenumber;
                throw satdump_exception("Product Channel Index " + std::to_string(index) + " is not present!");
            }

            /**
             * @brief Get channel frequency
             * @param index absolute channel index
             * @return channel frequency in Hz
             */
            double get_channel_frequency(int index) { return wavenumber_to_freq(get_channel_wavenumber(index)); }

            /**
             * @brief Set channel calibration unit
             * @param index absolute channel index
             * @param type type of calibration (ID). If non-standard,
             * this can be an arbitrary string such as the unit instead
             */
            void set_channel_unit(int index, std::string type_or_unit)
            {
                // TODOREWORK, may want to enforce unit automatically?
                for (auto &img : images)
                    if (img.abs_index == index)
                    {
                        img.calibration_type = type_or_unit;
                        return;
                    }
                throw satdump_exception("Product Channel Index " + std::to_string(index) + " is not present!");
            }

        public:
            bool d_no_not_save_images = false;
            bool d_no_not_load_images = false;
            virtual void save(std::string directory);
            virtual void load(std::string file);

            virtual ~ImageProduct();
        };
    } // namespace products
} // namespace satdump