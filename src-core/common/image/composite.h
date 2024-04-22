#pragma once

#include <cstdint>
#include "image.h"
#include <string>
#include "nlohmann/json.hpp"
#include "products/image_products.h"

namespace image
{
    // Generate a composite from channels and an equation
    template <typename T>
    Image<T> generate_composite_from_equ(std::vector<Image<T>> &inputChannels, std::vector<std::string> channelNumbers, std::string equation, nlohmann::json offsets_cfg, float *progress = nullptr);

    // Generate a composite from channels and a LUT
    template <typename T>
    Image<T> generate_composite_from_lut(std::vector<Image<T>> &inputChannels, std::vector<std::string> channelNumbers, std::string lut_path, nlohmann::json offsets_cfg, float *progress = nullptr);

    // Generate a composite from channels and a Lua script
    template <typename T>
    Image<T> generate_composite_from_lua(satdump::ImageProducts *img_pro, std::vector<Image<T>> &inputChannels, std::vector<std::string> channelNumbers, std::string lua_path, nlohmann::json lua_vars, nlohmann::json offsets_cfg, std::vector<double> *final_timestamps = nullptr, float *progress = nullptr);

#if DEFINE_COMPOSITE_UTILS
    struct compo_cfg_t
    {
        bool hasOffsets;
        std::map<std::string, int> offsets;
        int maxWidth;
        int maxHeight;
        std::vector<std::pair<float, float>> image_scales;
        int img_width;
        int img_height;
    };

    template <typename T>
    compo_cfg_t get_compo_cfg(std::vector<Image<T>> &inputChannels, /*std::vector<std::string> &channelNumbers, */ nlohmann::json &offsets_cfg)
    {
        bool hasOffsets = !offsets_cfg.empty();
        std::map<std::string, int> offsets;
        if (hasOffsets)
        {
            bool has_actual_offset = false;
            std::map<std::string, int> offsetsStr = offsets_cfg.get<std::map<std::string, int>>();
            for (std::pair<std::string, int> currentOff : offsetsStr)
            {
                offsets.emplace(currentOff.first, -currentOff.second);
                has_actual_offset |= currentOff.second != 0;
            }
            hasOffsets = has_actual_offset;
        }

        // Compute channel variable names
        double *channelValues = new double[inputChannels.size()];
        for (size_t i = 0; i < inputChannels.size(); i++)
            channelValues[i] = 0;

        // Get maximum image size, and resize them all to that. Also acts as basic safety
        int maxWidth = 0, maxHeight = 0;
        for (int i = 0; i < (int)inputChannels.size(); i++)
        {
            if ((int)inputChannels[i].width() > maxWidth)
                maxWidth = inputChannels[i].width();

            if ((int)inputChannels[i].height() > maxHeight)
                maxHeight = inputChannels[i].height();
        }

        std::vector<std::pair<float, float>> image_scales;
        for (int i = 0; i < (int)inputChannels.size(); i++)
            image_scales.push_back({float(inputChannels[i].width()) / float(maxWidth), float(inputChannels[i].height()) / float(maxHeight)});

        // Get output width
        int img_width = maxWidth;
        int img_height = maxHeight;

        delete[] channelValues;

        return {hasOffsets,
                offsets,
                maxWidth,
                maxHeight,
                image_scales,
                img_width,
                img_height};
    }

    template <typename T>
    inline void get_channel_vals(double *channelValues, std::vector<Image<T>> &inputChannels, std::vector<std::string> &channelNumbers, compo_cfg_t &f, size_t &line, size_t &pixel)
    {
        // Set variables and scale to 1.0
        for (int i = 0; i < (int)inputChannels.size(); i++)
        {
            int line_ch = line * f.image_scales[i].first;
            int pixe_ch = pixel * f.image_scales[i].second;

            // If we have to offset some channels
            if (f.hasOffsets && f.offsets.count(channelNumbers[i]) > 0 && f.offsets[channelNumbers[i]] != 0)
            {
                int currentPx = pixe_ch + f.offsets[channelNumbers[i]];
                if (currentPx < 0 || currentPx >= (int)inputChannels[i].width())
                {
                    channelValues[i] = 0;
                    continue;
                }

                pixe_ch += f.offsets[channelNumbers[i]] * f.image_scales[i].second;
            }

            channelValues[i] = double(inputChannels[i][line_ch * inputChannels[i].width() + pixe_ch]) / double(std::numeric_limits<T>::max());
        }
    }

    template <typename T>
    inline void get_channel_vals_raw(T *channelValues, std::vector<Image<T>> &inputChannels, std::vector<std::string> &channelNumbers, compo_cfg_t &f, size_t &line, size_t &pixel)
    {
        // Set variables and scale to 1.0
        for (int i = 0; i < (int)inputChannels.size(); i++)
        {
            int line_ch = line * f.image_scales[i].first;
            int pixe_ch = pixel * f.image_scales[i].second;

            // If we have to offset some channels
            if (f.hasOffsets && f.offsets.count(channelNumbers[i]) > 0 && f.offsets[channelNumbers[i]] != 0)
            {
                int currentPx = pixe_ch + f.offsets[channelNumbers[i]];
                if (currentPx < 0 || currentPx >= (int)inputChannels[i].width())
                {
                    channelValues[i] = 0;
                    continue;
                }

                pixe_ch += f.offsets[channelNumbers[i]] * f.image_scales[i].second;
            }

            channelValues[i] = inputChannels[i][line_ch * inputChannels[i].width() + pixe_ch];
        }
    }
#endif
}