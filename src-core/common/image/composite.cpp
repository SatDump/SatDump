#include "composite.h"
#include "libs/muparser/muParser.h"
#include "logger.h"
#include "image.h"

namespace image
{
    // Generate a composite from channels and an equation
    template <typename T>
    Image<T> generate_composite_from_equ(std::vector<Image<T>> inputChannels, std::vector<std::string> channelNumbers, std::string equation, nlohmann::json parameters, float *progress)
    {
        // Equation parsing stuff
        mu::Parser rgbParser;
        int outValsCnt = 0;

        // Get other parameters such as equalization, etc
        // bool equalize = parameters.count("equalize") > 0 ? parameters["equalize"].get<bool>() : false;
        // bool pre_equalize = parameters.count("pre_equalize") > 0 ? parameters["pre_equalize"].get<bool>() : false;
        // bool normalize = parameters.count("normalize") > 0 ? parameters["normalize"].get<bool>() : false;
        // bool white_balance = parameters.count("white_balance") > 0 ? parameters["white_balance"].get<bool>() : false;
        bool hasOffsets = parameters.count("offsets") > 0;
        std::map<std::string, int> offsets;
        if (hasOffsets)
        {
            std::map<std::string, int> offsetsStr = parameters["offsets"].get<std::map<std::string, int>>();
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

            // Also equalize if requested
            // if (pre_equalize)
            //    inputChannels[i].equalize();
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

        std::vector<std::pair<float, float>> image_scales;

        for (int i = 0; i < (int)inputChannels.size(); i++)
        {
            image_scales.push_back({float(inputChannels[i].width()) / float(maxWidth), float(inputChannels[i].height()) / float(maxHeight)});
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
        for (size_t line = 0; line < img_height; line++)
        {
            for (size_t pixel = 0; pixel < img_width; pixel++)
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
                            else if (currentPx >= inputChannels[i].width())
                            {
                                channelValues[i] = 0;
                                continue;
                            }

                            pixe_ch += offsets[channelNumbers[i]];
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

        // if (white_balance)
        //     rgb_output.white_balance();

        // if (equalize)
        //     rgb_output.equalize();

        // if (normalize)
        //     rgb_output.normalize();

        return rgb_output;
    }

    template Image<uint8_t> generate_composite_from_equ<uint8_t>(std::vector<Image<uint8_t>>, std::vector<std::string>, std::string, nlohmann::json, float *);
    template Image<uint16_t> generate_composite_from_equ<uint16_t>(std::vector<Image<uint16_t>>, std::vector<std::string>, std::string, nlohmann::json, float *);
}