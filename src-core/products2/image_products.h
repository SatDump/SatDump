#pragma once

#include "products.h"
#include "common/image/image.h"
#include "image/channel_transform.h"

#include "common/physics_constants.h"

namespace satdump
{
    namespace products
    {
        class ImageProducts : public Products
        {
        public:
            struct ImageHolder
            {
                int abs_index = -1;            // Absolute image index, for calibration, etc. -2 means no calibration possible on this channel
                std::string filename;          // Filename & path of the image. Relative to the product file
                std::string channel_name;      // Name of the channel, this is usually just a number but can be a string
                image::Image image;            // The image itself
                int bit_depth = 16;            // Image bit depth
                ChannelTransform ch_transform; // Channel transform to reference field
                double wavenumber = -1;        // Wavenumber
            };

            std::vector<ImageHolder> images;

            bool save_as_matrix = false;

            ///////////////////////// Projection

            void set_proj_cfg(nlohmann::ordered_json cfg)
            {
                contents["projection_cfg"] = cfg;
            }

            nlohmann::ordered_json get_proj_cfg(int channel)
            {
                // TODO CHANNEL SPECIFICS
                return contents["projection_cfg"];
            }

            bool has_proj_cfg()
            {
                return contents.contains("projection_cfg");
            }

            ///////////////////////// Channels

            ImageHolder &get_channel_image(int index)
            {
                for (auto &img : images)
                    if (img.abs_index == index)
                        return img;
                printf("Product Channel Index %d is not present!\n", index); // TODOREWORK
                throw satdump_exception("TODO");
            }

            ImageHolder &get_channel_image(std::string name)
            {
                for (auto &img : images)
                    if (img.channel_name == name)
                        return img;
                printf("Product Channel Name %s is not present!\n", name.c_str()); // TODOREWORK
                throw satdump_exception("TODO");
            }

            ///////////////////////// Wavenumbers

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

            void set_channel_frequency(int index, double frequency)
            {
                set_channel_wavenumber(index, frequency / SPEED_OF_LIGHT_M_S);
            }

        public:
            bool d_no_not_save_images = false;
            bool d_no_not_load_images = false;
            virtual void save(std::string directory);
            virtual void load(std::string file);

            virtual ~ImageProducts();
        };
    }
}