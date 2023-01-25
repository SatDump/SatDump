#include "composite.h"
#include "libs/muparser/muParser.h"
#include "logger.h"
#include "image.h"

#include "libs/sol2/sol.hpp"
#include "common/lua/lua_utils.h"
#include "common/projection/sat_proj/sat_proj.h"
#include "resources.h"

namespace image
{
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
            std::map<std::string, int> offsetsStr = offsets_cfg.get<std::map<std::string, int>>();
            for (std::pair<std::string, int> currentOff : offsetsStr)
                offsets.emplace(currentOff.first, -currentOff.second);
        }

        // Compute channel variable names
        double *channelValues = new double[inputChannels.size()];
        for (int i = 0; i < (int)inputChannels.size(); i++)
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
            if (f.hasOffsets)
            {
                if (f.offsets.count(channelNumbers[i]) > 0)
                {
                    int currentPx = pixe_ch + f.offsets[channelNumbers[i]];

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

                    pixe_ch += f.offsets[channelNumbers[i]] * f.image_scales[i].second;
                }
            }

            channelValues[i] = double(inputChannels[i][line_ch * inputChannels[i].width() + pixe_ch]) / double(std::numeric_limits<T>::max());
        }
    }

    // Generate a composite from channels and an equation
    template <typename T>
    Image<T> generate_composite_from_equ(std::vector<Image<T>> inputChannels, std::vector<std::string> channelNumbers, std::string equation, nlohmann::json offsets_cfg, float *progress)
    {
        // Equation parsing stuff
        mu::Parser rgbParser;
        int outValsCnt = 0;

        compo_cfg_t f = get_compo_cfg(inputChannels, /*channelNumbers, */ offsets_cfg);

        // Compute channel variable names
        double *channelValues = new double[inputChannels.size()];
        for (int i = 0; i < (int)inputChannels.size(); i++)
        {
            channelValues[i] = 0;
            rgbParser.DefineVar(channelNumbers[i], &channelValues[i]);
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

        size_t img_fullch = f.img_width * f.img_height;

        // Output image
        bool isRgb = outValsCnt == 3;
        Image<T> rgb_output(f.img_width, f.img_height, isRgb ? 3 : 1);

        // Utils
        double R = 0;
        double G = 0;
        double B = 0;

        // Run though the entire image
        for (size_t line = 0; line < (size_t)f.img_height; line++)
        {
            for (size_t pixel = 0; pixel < (size_t)f.img_width; pixel++)
            {
                get_channel_vals(channelValues, inputChannels, channelNumbers, f, line, pixel);

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
                rgb_output[img_fullch * 0 + line * f.img_width + pixel] = R;
                if (isRgb)
                {
                    rgb_output[img_fullch * 1 + line * f.img_width + pixel] = G;
                    rgb_output[img_fullch * 2 + line * f.img_width + pixel] = B;
                }
            }

            if (progress != nullptr)
                *progress = (float)line / (float)f.img_height;
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

        compo_cfg_t f = get_compo_cfg(inputChannels, /*channelNumbers, */ offsets_cfg);

        // Compute channel variable names
        double *channelValues = new double[inputChannels.size()];
        for (int i = 0; i < (int)inputChannels.size(); i++)
            channelValues[i] = 0;

        // Output image
        Image<T> rgb_output(f.img_width, f.img_height, std::min(3, lut.channels()));

        // Run though the entire image
        for (size_t line = 0; line < (size_t)f.img_height; line++)
        {
            for (size_t pixel = 0; pixel < (size_t)f.img_width; pixel++)
            {
                get_channel_vals(channelValues, inputChannels, channelNumbers, f, line, pixel);

                // Apply the LUT
                if (inputChannels.size() == 1) // 1D Case
                {
                    int position = channelValues[0] * lut.width();

                    if (position >= (int)lut.width())
                        position = (int)lut.width() - 1;

                    for (int c = 0; c < std::min(3, lut.channels()); c++)
                        rgb_output.channel(c)[line * f.img_width + pixel] = lut.channel(c)[position];
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
                        rgb_output.channel(c)[line * f.img_width + pixel] = lut.channel(c)[position_y * lut.width() + position_x];

                    // logger->critical("{:d}, {:d}, {:d}", rgb_output.channel(0)[line * img_width + pixel], rgb_output.channel(1)[line * img_width + pixel], rgb_output.channel(2)[line * img_width + pixel]);
                }
            }

            if (progress != nullptr)
                *progress = (float)line / (float)f.img_height;
        }

        delete[] channelValues;

        return rgb_output;
    }

    template Image<uint8_t> generate_composite_from_lut<uint8_t>(std::vector<Image<uint8_t>>, std::vector<std::string>, std::string, nlohmann::json, float *);
    template Image<uint16_t> generate_composite_from_lut<uint16_t>(std::vector<Image<uint16_t>>, std::vector<std::string>, std::string, nlohmann::json, float *);

    void bindCompoCfgType(sol::state &lua)
    {
        sol::usertype<compo_cfg_t> type = lua.new_usertype<compo_cfg_t>("compo_cfg_t");

        type["hasOffsets"] = &compo_cfg_t::hasOffsets;
        type["offsets"] = &compo_cfg_t::offsets;
        type["maxWidth"] = &compo_cfg_t::maxWidth;
        type["maxHeight"] = &compo_cfg_t::maxHeight;
        type["image_scales"] = &compo_cfg_t::image_scales;
        type["img_width"] = &compo_cfg_t::img_width;
        type["img_height"] = &compo_cfg_t::img_height;
    }

    // Generate a composite from channels and a Lua script
    template <typename T>
    Image<T> generate_composite_from_lua(satdump::ImageProducts *img_pro, std::vector<Image<T>> inputChannels, std::vector<std::string> channelNumbers, std::string lua_path, nlohmann::json lua_vars, nlohmann::json offsets_cfg, float *progress)
    {
        compo_cfg_t f = get_compo_cfg(inputChannels, /*channelNumbers, */ offsets_cfg);

        // Compute channel variable names
        double *channelValues = new double[inputChannels.size()];
        for (int i = 0; i < (int)inputChannels.size(); i++)
            channelValues[i] = 0;

        // Output image
        Image<T> rgb_output; //(f.img_width, f.img_height, 3);

        try
        {
            sol::state lua;

            lua.open_libraries(sol::lib::base);
            lua.open_libraries(sol::lib::string);
            lua.open_libraries(sol::lib::math);

            lua_utils::bindImageTypes(lua);
            lua_utils::bindGeoTypes(lua);
            lua_utils::bindSatProjType(lua);
            lua_utils::bindEquProjType(lua);

            lua["has_sat_proj"] = [img_pro]()
            { return img_pro->has_proj_cfg() && img_pro->has_tle() && img_pro->has_timestamps; };
            lua["get_sat_proj"] = [img_pro]()
            { return satdump::get_sat_proj(img_pro->get_proj_cfg(), img_pro->get_tle(), img_pro->get_timestamps()); };
            lua["get_resource_path"] = resources::getResourcePath;

            lua.script_file(lua_path);

            lua["lua_vars"] = lua_utils::mapJsonToLua(lua, lua_vars);

            int n_ch = lua["init"]().get<int>();

            rgb_output.init(f.img_width, f.img_height, n_ch);

            lua["rgb_output"] = rgb_output;
            lua["compo_cfg"] = f;
            lua["set_img_out"] = [&rgb_output](int c, size_t x, size_t y, double v)
            {
                if (y >= rgb_output.height())
                    return;
                if (x >= rgb_output.width())
                    return;
                rgb_output.channel(c)[y * rgb_output.width() + x] = rgb_output.clamp(v * double(std::numeric_limits<T>::max()));
            };
            lua["get_channel_value"] = [channelValues](int x)
            { return channelValues[x]; };
            lua["get_channel_values"] = [channelValues, &inputChannels, &channelNumbers, &f](size_t x, size_t y)
            { get_channel_vals(channelValues, inputChannels, channelNumbers, f, y, x); };
            lua["set_progress"] = [&progress](float x, float y)
            { if(progress != nullptr) *progress = x / y; };

            lua["process"]();
        }
        catch (std::exception &e)
        {
            logger->error("Error generating composite! {:s}", e.what());
        }

        delete[] channelValues;

        return rgb_output;
    }

    template Image<uint8_t> generate_composite_from_lua<uint8_t>(satdump::ImageProducts *, std::vector<Image<uint8_t>>, std::vector<std::string>, std::string, nlohmann::json, nlohmann::json, float *);
    template Image<uint16_t> generate_composite_from_lua<uint16_t>(satdump::ImageProducts *, std::vector<Image<uint16_t>>, std::vector<std::string>, std::string, nlohmann::json, nlohmann::json, float *);
}