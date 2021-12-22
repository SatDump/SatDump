#include "composite.h"
#include "libs/muparser/muParser.h"
#include "logger.h"
#include "image.h"

namespace image
{
    // Generate a composite from channels and an equation
    template <typename T>
    Image<T> generate_composite_from_equ(std::vector<Image<T>> inputChannels, std::vector<int> channelNumbers, std::string equation, nlohmann::json parameters)
    {
        // Equation parsing stuff
        mu::Parser rgbParser;
        double *rgbOut;
        int outValsCnt = 0;

        // Get other parameters such as equalization, etc
        bool equalize = parameters.count("equalize") > 0 ? parameters["equalize"].get<bool>() : false;
        bool pre_equalize = parameters.count("pre_equalize") > 0 ? parameters["pre_equalize"].get<bool>() : false;
        bool normalize = parameters.count("normalize") > 0 ? parameters["normalize"].get<bool>() : false;
        bool white_balance = parameters.count("white_balance") > 0 ? parameters["white_balance"].get<bool>() : false;
        bool hasOffsets = parameters.count("offsets") > 0;
        std::map<int, int> offsets;
        if (hasOffsets)
        {
            std::map<std::string, int> offsetsStr = parameters["offsets"].get<std::map<std::string, int>>();
            for (std::pair<std::string, int> currentOff : offsetsStr)
                offsets.emplace(std::stoi(currentOff.first), -currentOff.second);
        }

        // Compute channel variable names
        std::vector<std::string> channelNames;
        double *channelValues = new double[inputChannels.size()];
        for (int i = 0; i < (int)inputChannels.size(); i++)
        {
            channelValues[i] = 0;
            rgbParser.DefineVar("ch" + std::to_string(channelNumbers[i]), &channelValues[i]);

            // Also equalize if requested
            if (pre_equalize)
                inputChannels[i].equalize();
        }

        // Set expression
        rgbParser.SetExpr(equation);
        rgbParser.Eval(outValsCnt); // Eval once for channel output count

        // Get maximum image size, and resize them all to that. Also acts as basic safety
        int maxWidth = 0, maxHeight = 0;
        for (int i = 0; i < (int)inputChannels.size(); i++)
        {
            if (inputChannels[i].width() > maxWidth)
                maxWidth = inputChannels[i].width();

            if (inputChannels[i].height() > maxHeight)
                maxHeight = inputChannels[i].height();
        }

        for (int i = 0; i < (int)inputChannels.size(); i++)
        {
            inputChannels[i].resize(maxWidth, maxHeight);
        }

        // Get output width
        int img_width = inputChannels[0].width();
        int img_height = inputChannels[0].height();
        size_t img_fullch = img_width * img_height;

        // Output image
        bool isRgb = outValsCnt == 3;
        Image<T> rgb_output(img_width, img_height, isRgb ? 3 : 1);

        // Utils
        double R = 0;
        double G = 0;
        double B = 0;

        // Run though the entire image
        for (size_t px = 0; px < img_fullch; px++)
        {
            // Set variables and scale to 1.0
            for (int i = 0; i < (int)inputChannels.size(); i++)
            {
                int fpx = px;

                // If we have to offset some channels
                if (hasOffsets)
                {
                    if (offsets.count(channelNumbers[i]) > 0)
                    {
                        int currentPx = fpx % img_width + offsets[channelNumbers[i]];

                        if (currentPx < 0)
                        {
                            channelValues[i] = 0;
                            continue;
                        }
                        else if (currentPx >= img_width)
                        {
                            channelValues[i] = 0;
                            continue;
                        }

                        fpx += offsets[channelNumbers[i]];
                    }
                }

                channelValues[i] = double(inputChannels[i][fpx]) / double(std::numeric_limits<T>::max());
            }

            // Do the math
            rgbOut = rgbParser.Eval(outValsCnt);

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
            rgb_output[img_fullch * 0 + px] = R;
            if (isRgb)
            {
                rgb_output[img_fullch * 1 + px] = G;
                rgb_output[img_fullch * 2 + px] = B;
            }
        }

        delete[] channelValues;

        if (white_balance)
            rgb_output.white_balance();

        if (equalize)
            rgb_output.equalize();

        if (normalize)
            rgb_output.normalize();

        return rgb_output;
    }

    template Image<uint8_t> generate_composite_from_equ<uint8_t>(std::vector<Image<uint8_t>>, std::vector<int>, std::string, nlohmann::json);
    template Image<uint16_t> generate_composite_from_equ<uint16_t>(std::vector<Image<uint16_t>>, std::vector<int>, std::string, nlohmann::json);
}