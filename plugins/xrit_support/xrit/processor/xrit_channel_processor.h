#pragma once

/**
 * @file xrit_channel_processor.h
 * @brief xRIT file group decoding (into images)
 */

#include "image/image.h"
#include "products/image_product.h"
#include "xrit/segment_decoder.h"
#include "xrit/xrit_file.h"
#include <mutex>

namespace satdump
{
    namespace xrit
    {
        /**
         * @brief All-in-on class to convert a set of xRIT files
         * provided in an arbitrary order into products.
         *
         * This will handle allocating segmented decoders for each
         * channel, calibration, metadata...
         *
         * Files provided should be provided sequentially for each 'groupid'
         * you wish to decode, as upon a change, the segmented decoders resets
         * themselves. This is well suited to live downlinks which are always
         * sequential, and offline decoding where files are already filtered by
         * group.
         *
         * @param directory directory to save output files into. MUST be set by the
         * user (unless in_memory_mode is true)
         * @param in_memory_mode If true, generated products are kept in memory. NOT
         * recommended for live processing, as RAM usage will only increase.
         * @param in_memory_products if in_memory_mode is enabled, contains the output
         * products
         */
        class XRITChannelProcessor
        {
        private:
            std::map<std::string, std::shared_ptr<SegmentedImageDecoder>> segmented_decoders;

        private:
            struct XRITProd
            {
                std::string directory_path;
                std::string pro_f_path;
                std::shared_ptr<products::ImageProduct> pro;
            };

        private:
            /**
             * @brief Util function to setup or load products
             * to write a new channel/data into them.
             * @param meta xRIT file info
             */
            XRITProd setupProduct(xrit::XRITFileInfo &meta);

            /**
             * @brief Save a metadata xRIT file (process into products!)
             * @param meta xRIT file info
             * @param file actual xRIT file
             */
            bool saveMeta(xrit::XRITFileInfo &meta, XRITFile &file);

            /**
             * @brief Save an image xRIT file (process into products!)
             * @param meta xRIT file info
             * @param file actual xRIT file
             */
            void saveImg(xrit::XRITFileInfo &meta, image::Image &img);

        private:
            // TODOREWORK DOCUMENT
            void process_composites(std::shared_ptr<products::ImageProduct> pro, std::string pro_path);

        public:
            enum xrit_image_status
            {
                RECEIVING,
                SAVING,
                IDLE
            };

            struct wip_images
            {
                xrit_image_status imageStatus = RECEIVING;
                const int img_width = 256, img_height = 256;

                // UI Stuff
                bool hasToUpdate = false;
                unsigned int textureID = 0;
                uint32_t *textureBuffer;
            };

            std::map<std::string, std::unique_ptr<wip_images>> all_wip_images;
            std::mutex ui_img_mtx;

        public:
            std::string directory = "";
            bool in_memory_mode = false;
            std::map<std::string, std::shared_ptr<products::ImageProduct>> in_memory_products;

        public:
            ~XRITChannelProcessor();

            /**
             * @brief Push a file to process. See class documenation for
             * more information on using this class.
             *
             * @param finfo xRIT file info
             * @param file actual xRIT file
             */
            void push(xrit::XRITFileInfo &finfo, XRITFile &file);

            /**
             * @brief Forces all segmented decoders that do hold data
             * to save their images now. Should be called when the decoder
             * is about to exit, to save lingering data.
             */
            void flush();
        };
    } // namespace xrit
} // namespace satdump