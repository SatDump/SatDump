#pragma once

/**
 * @file image_product.h
 */

#include "product.h"
#include "common/image/image.h"
#include "image/channel_transform.h"

#include "common/physics_constants.h"

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
            };

            std::vector<ImageHolder> images;

            bool save_as_matrix = false;

            /**
             * @brief Set geo projection config in the product
             * @param cfg projection config. Must contain all applicable metadata
             */
            void set_proj_cfg(nlohmann::json cfg)
            {
                contents["projection_cfg"] = cfg;
            }

            /**
             * @brief Set geo projection config in the product, with a TLE and timestamps
             * @param cfg projection config.
             * @param tle the TLE
             * @param timestamps the timestamps
             */
            void set_proj_cfg_tle_timestamps(nlohmann::json cfg, nlohmann::json tle, nlohmann::json timestamps)
            {
                contents["projection_cfg"] = cfg;
                contents["projection_cfg"]["tle"] = tle;
                contents["projection_cfg"]["timestamps"] = timestamps;
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
                return cfg;
            }

            /**
             * @brief Check if geo projection info is present
             * @return true if present
             */
            bool has_proj_cfg()
            {
                return contents.contains("projection_cfg");
            }

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
                // logger->error("Product Channel Index %d is not present!", index); // TODOREWORK
                throw satdump_exception("TODO");
            }

            /**
             * @brief Set channel frequency (internally, wavenumber)
             * @param index absolute channel index
             * @param frequency frequency value, in Hz
             */
            void set_channel_frequency(int index, double frequency)
            {
                set_channel_wavenumber(index, frequency / SPEED_OF_LIGHT_M_S);
            }

        public:
            bool d_no_not_save_images = false;
            bool d_no_not_load_images = false;
            virtual void save(std::string directory);
            virtual void load(std::string file);

            virtual ~ImageProduct();
        };
    }
}