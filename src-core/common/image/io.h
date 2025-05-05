#pragma once

/**
 * @file  io.h
 */

#include <string>
#include "image.h"

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

    void load_jpeg(Image &img, std::string file);
    void load_jpeg(Image &img, uint8_t *buffer, int size);
    void save_jpeg(Image &img, std::string file);

    void save_tiff(Image &img, std::string file);
    void load_tiff(Image &img, std::string file);

    void save_pbm(Image &img, std::string file);
    void load_pbm(Image &img, std::string file);

    void save_j2k(Image &img, std::string file);
    void load_j2k(Image &img, std::string file);

    void save_qoi(Image &img, std::string file);
    void load_qoi(Image &img, std::string file);

    void load_img(Image &img, std::string file);
    void load_img(Image &img, uint8_t *buffer, int size);
    void save_img(Image &img, std::string file, bool fast = true);

    bool append_ext(std::string *file, bool prod = false);
}