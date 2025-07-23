#pragma once

/**
 * @file io.h
 * @brief Image saving/loading
 */

#include "image.h"
#include <string>

namespace satdump
{
    namespace image
    {
        /**
         * @brief Save an image to a PNG file.
         *
         * @param img image to save
         * @param file filepath to save the image to with extension
         * @param fast whether to save using fast mode (slightly worse compression) or not
         */
        void save_png(Image &img, std::string file, bool fast = true);

        /**
         * @brief Load an image from a PNG file.
         *
         * @param img image to load to
         * @param file filepath to load the image from
         * @param disableIndexing ignore palette, as to get grayscale
         */
        void load_png(Image &img, std::string file, bool disableIndexing = false);

        /**
         * @brief Load an image from a PNG buffer.
         *
         * @param img image to load to
         * @param buffer buffer to load the image from
         * @param size size of the buffer
         * @param disableIndexing ignore palette, as to get grayscale
         */
        void load_png(Image &img, uint8_t *buffer, int size, bool disableIndexing = false);

        /**
         * @brief Load an image from a JPEG (8-bit!) file.
         *
         * @param img image to load to
         * @param file filepath to load the image from
         */
        void load_jpeg(Image &img, std::string file);

        /**
         * @brief Load an image from a JPEG (8-bit!) buffer.
         *
         * @param img image to load to
         * @param buffer buffer to load the image from
         * @param size size of the buffer
         */
        void load_jpeg(Image &img, uint8_t *buffer, int size);

        /**
         * @brief Save an image to a JPEG (8-bit!) file.
         *
         * @param img image to save
         * @param file filepath to save the image to with extension
         */
        void save_jpeg(Image &img, std::string file);

        /**
         * @brief Save an image to a TIFF file.
         * Supports part of GeoTIFF specification.
         *
         * @param img image to save
         * @param file filepath to save the image to with extension
         */
        void save_tiff(Image &img, std::string file);

        /**
         * @brief Load an image from a TIFF file.
         * Supports part of GeoTIFF specification.
         *
         * @param img image to load to
         * @param file filepath to load the image from
         */
        void load_tiff(Image &img, std::string file);

        /**
         * @brief Save an image to a PBM (and similar) file.
         *
         * @param img image to save
         * @param file filepath to save the image to with extension
         */
        void save_pbm(Image &img, std::string file);

        /**
         * @brief Load an image from a PBM (and similar) file.
         *
         * @param img image to load to
         * @param file filepath to load the image from
         */
        void load_pbm(Image &img, std::string file);

        /**
         * @brief Save an image to a JPEG2000 file.
         *
         * @param img image to save
         * @param file filepath to save the image to with extension
         */
        void save_j2k(Image &img, std::string file);

        /**
         * @brief Load an image from a JPEG2000 file.
         *
         * @param img image to load to
         * @param file filepath to load the image from
         */
        void load_j2k(Image &img, std::string file);

        /**
         * @brief Save an image to a QOI file.
         *
         * @param img image to save
         * @param file filepath to save the image to with extension
         */
        void save_qoi(Image &img, std::string file);

        /**
         * @brief Load an image from a QOI file.
         *
         * @param img image to load to
         * @param file filepath to load the image from
         */
        void load_qoi(Image &img, std::string file);

        /**
         * @brief Load an image from an image file,
         * automatically from the right format if supported.
         *
         * @param img image to load to
         * @param file filepath to load the image from
         */
        void load_img(Image &img, std::string file);

        /**
         * @brief Load an image from a memory buffer
         * automatically from the right format if supported.
         *
         * @param img image to load to
         * @param buffer buffer to load the image from
         * @param size size of the buffer
         */
        void load_img(Image &img, uint8_t *buffer, int size);

        /**
         * @brief Save an image to an image file
         * automatically to the right format if supported.
         *
         * @param img image to save
         * @param file filepath to save the image to with extension
         * @param fast whether to save using fast mode (slightly worse compression on PNG) or not
         */
        void save_img(Image &img, std::string file, bool fast = true);

        /**
         * @brief Save an image to an image file
         * automatically to the right format if supported.
         * This variant will NOT override existing files
         *
         * @param img image to save
         * @param file filepath to save the image to with extension
         * @param fast whether to save using fast mode (slightly worse compression on PNG) or not
         */
        void save_img_safe(Image &img, std::string file, bool fast = true);

        /**
         * @brief Append default image file format extension
         * as configured in satdump_cfg.json.
         *
         * @param file path of the image (with or without extension)
         * @param prod if true, returns the default file extesions for product images
         * @return true if there's already an extension
         */
        bool append_ext(std::string *file, bool prod = false);
    } // namespace image
} // namespace satdump