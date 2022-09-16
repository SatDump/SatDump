#include "composite.h"
#include "libs/muparser/muParser.h"
#include "logger.h"
#include "image.h"

namespace image
{
    // Generate a composite from channels and an equation
    template <typename T>
    Image<T> generate_composite_from_equ(std::vector<Image<T>> inputChannels, std::vector<std::string> channelNumbers, std::string equation, nlohmann::json offsets_cfg, float *progress)
    {
        // Equation parsing stuff
        mu::Parser rgbParser;
        int outValsCnt = 0;

        bool hasOffsets = !offsets_cfg.empty();
        std::map<std::string, int> offsets;
        if (hasOffsets)
        {
            std::map<std::string, int> offsetsStr = offsets_cfg.get<std::map<std::string, int>>();
            for (std::pair<std::string, int> currentOff : offsetsStr)
                offsets.emplace(currentOff.first, -currentOff.second);
        }

        // Compute channel variable names
        std::vector<std::string> channelNames;
        double *channelValues = new double[inputChannels.size()];
        for (int i = 0; i < (int)inputChannels.size(); i++)
        {
            channelValues[i] = 0;
            rgbParser.DefineVar("ch" + channelNumbers[i], &channelValues[i]);
        }

        try
        {
            // Set expression
            rgbParser.SetExpr(equation);
            rgbParser.Eval(outValsCnt); // Eval once for channel output count
        }
        catch (mu::ParserError &e)
        {
            logger->error(e.GetMsg());
            return Image<T>();
        }

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
        {
            image_scales.push_back({float(inputChannels[i].width()) / float(maxWidth), float(inputChannels[i].height()) / float(maxHeight)});
        }

        // Get output width
        int img_width = maxWidth;   // inputChannels[0].width();
        int img_height = maxHeight; // inputChannels[0].height();
        size_t img_fullch = img_width * img_height;

        // Output image
        bool isRgb = outValsCnt == 3;
        Image<T> rgb_output(img_width, img_height, isRgb ? 3 : 1);

        // Utils
        double R = 0;
        double G = 0;
        double B = 0;

        // Run though the entire image
        for (size_t line = 0; line < (size_t)img_height; line++)
        {
            for (size_t pixel = 0; pixel < (size_t)img_width; pixel++)
            {
                // Set variables and scale to 1.0
                for (int i = 0; i < (int)inputChannels.size(); i++)
                {
                    int line_ch = line * image_scales[i].first;
                    int pixe_ch = pixel * image_scales[i].second;

                    // If we have to offset some channels
                    if (hasOffsets)
                    {
                        if (offsets.count(channelNumbers[i]) > 0)
                        {
                            int currentPx = pixe_ch + offsets[channelNumbers[i]];

                            if (currentPx < 0)
                            {
                                channelValues[i] = 0;
                                continue;
                            }
                            else if (currentPx >= (int)inputChannels[i].width())
                            {
                                channelValues[i] = 0;
                                continue;
                            }

                            pixe_ch += offsets[channelNumbers[i]] * image_scales[i].second;
                        }
                    }

                    channelValues[i] = double(inputChannels[i][line_ch * inputChannels[i].width() + pixe_ch]) / double(std::numeric_limits<T>::max());
                }

                // Do the math
                double *rgbOut = rgbParser.Eval(outValsCnt);

                // Get output and scale back
                R = rgbOut[0] * double(std::numeric_limits<T>::max());
                if (isRgb)
                {
                    G = rgbOut[1] * double(std::numeric_limits<T>::max());
                    B = rgbOut[2] * double(std::numeric_limits<T>::max());
                }

                // Clamp
                if (R < 0)
                    R = 0;

                if (R > std::numeric_limits<T>::max())
                    R = std::numeric_limits<T>::max();
                if (isRgb)
                {
                    if (G < 0)
                        G = 0;
                    if (G > std::numeric_limits<T>::max())
                        G = std::numeric_limits<T>::max();
                    if (B < 0)
                        B = 0;
                    if (B > std::numeric_limits<T>::max())
                        B = std::numeric_limits<T>::max();
                }

                // Write output
                rgb_output[img_fullch * 0 + line * img_width + pixel] = R;
                if (isRgb)
                {
                    rgb_output[img_fullch * 1 + line * img_width + pixel] = G;
                    rgb_output[img_fullch * 2 + line * img_width + pixel] = B;
                }
            }

            if (progress != nullptr)
                *progress = (float)line / (float)img_height;
        }

        delete[] channelValues;

        return rgb_output;
    }

    template Image<uint8_t> generate_composite_from_equ<uint8_t>(std::vector<Image<uint8_t>>, std::vector<std::string>, std::string, nlohmann::json, float *);
    template Image<uint16_t> generate_composite_from_equ<uint16_t>(std::vector<Image<uint16_t>>, std::vector<std::string>, std::string, nlohmann::json, float *);

    // Generate a composite from channels and a LUT
    template <typename T>
    Image<T> generate_composite_from_lut(std::vector<Image<T>> inputChannels, std::vector<std::string> channelNumbers, std::string lut_path, nlohmann::json offsets_cfg, float *progress)
    {
        Image<T> lut;
        lut.load_png(lut_path);

        logger->critical(lut_path);

        bool hasOffsets = !offsets_cfg.empty();
        std::map<std::string, int> offsets;
        if (hasOffsets)
        {
            std::map<std::string, int> offsetsStr = offsets_cfg.get<std::map<std::string, int>>();
            for (std::pair<std::string, int> currentOff : offsetsStr)
                offsets.emplace(currentOff.first, -currentOff.second);
        }

        // Compute channel variable names
        std::vector<std::string> channelNames;
        double *channelValues = new double[inputChannels.size()];
        for (int i = 0; i < (int)inputChannels.size(); i++)
        {
            channelValues[i] = 0;
        }

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
        {
            image_scales.push_back({float(inputChannels[i].width()) / float(maxWidth), float(inputChannels[i].height()) / float(maxHeight)});
        }

        // Get output width
        int img_width = maxWidth;   // inputChannels[0].width();
        int img_height = maxHeight; // inputChannels[0].height();
        // size_t img_fullch = img_width * img_height;

        // Output image
        Image<T> rgb_output(img_width, img_height, std::min(3, lut.channels()));

        // Run though the entire image
        for (size_t line = 0; line < (size_t)img_height; line++)
        {
            for (size_t pixel = 0; pixel < (size_t)img_width; pixel++)
            {
                // Set variables and scale to 1.0
                for (int i = 0; i < (int)inputChannels.size(); i++)
                {
                    int line_ch = line * image_scales[i].first;
                    int pixe_ch = pixel * image_scales[i].second;

                    // If we have to offset some channels
                    if (hasOffsets)
                    {
                        if (offsets.count(channelNumbers[i]) > 0)
                        {
                            int currentPx = pixe_ch + offsets[channelNumbers[i]];

                            if (currentPx < 0)
                            {
                                channelValues[i] = 0;
                                continue;
                            }
                            else if (currentPx >= (int)inputChannels[i].width())
                            {
                                channelValues[i] = 0;
                                continue;
                            }

                            pixe_ch += offsets[channelNumbers[i]] * image_scales[i].second;
                        }
                    }

                    channelValues[i] = double(inputChannels[i][line_ch * inputChannels[i].width() + pixe_ch]) / double(std::numeric_limits<T>::max());
                }

                // Apply the LUT
                if (inputChannels.size() == 1) // 1D Case
                {
                    int position = channelValues[0] * lut.width();

                    if (position >= (int)lut.width())
                        position = (int)lut.width() - 1;

                    for (int c = 0; c < std::min(3, lut.channels()); c++)
                        rgb_output.channel(c)[line * img_width + pixel] = lut.channel(c)[position];
                }
                else if (inputChannels.size() == 2) // 2D Case
                {
                    int position_x = channelValues[0] * lut.width();
                    int position_y = channelValues[1] * lut.height();

                    if (position_x >= (int)lut.width())
                        position_x = (int)lut.width() - 1;

                    if (position_y >= (int)lut.height())
                        position_y = (int)lut.height() - 1;

                    for (int c = 0; c < std::min(3, lut.channels()); c++)
                        rgb_output.channel(c)[line * img_width + pixel] = lut.channel(c)[position_y * lut.width() + position_x];

                    // logger->critical("{:d}, {:d}, {:d}", rgb_output.channel(0)[line * img_width + pixel], rgb_output.channel(1)[line * img_width + pixel], rgb_output.channel(2)[line * img_width + pixel]);
                }
            }

            if (progress != nullptr)
                *progress = (float)line / (float)img_height;
        }

        delete[] channelValues;

        return rgb_output;
    }

    template Image<uint8_t> generate_composite_from_lut<uint8_t>(std::vector<Image<uint8_t>>, std::vector<std::string>, std::string, nlohmann::json, float *);
    template Image<uint16_t> generate_composite_from_lut<uint16_t>(std::vector<Image<uint16_t>>, std::vector<std::string>, std::string, nlohmann::json, float *);
}