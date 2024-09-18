#define DEFINE_COMPOSITE_UTILS 1
#include "composite.h"
#include "libs/muparser/muParser.h"
#include "logger.h"
#include "image.h"

#include "libs/sol2/sol.hpp"
#include "common/lua/lua_utils.h"
#include "common/projection/sat_proj/sat_proj.h"
#include "resources.h"

#include "io.h"

namespace image
{
    // Generate a composite from channels and an equation
    Image generate_composite_from_equ(std::vector<Image> &inputChannels, std::vector<std::string> channelNumbers, std::string equation, nlohmann::json offsets_cfg, float *progress)
    {
        // Equation parsing stuff
        mu::Parser rgbParser;
        int outValsCnt = 0;

        compo_cfg_t f = get_compo_cfg(inputChannels, channelNumbers, offsets_cfg);

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
            return Image();
        }

        size_t img_fullch = f.img_width * f.img_height;

        // Output image
        bool isRgb = outValsCnt == 3;
        bool isRgbA = outValsCnt == 4;
        Image rgb_output(f.img_depth, f.img_width, f.img_height, isRgbA ? 4 : (isRgb ? 3 : 1));

        // Utils
        double R = 0;
        double G = 0;
        double B = 0;
        double A = 0;

        // Run though the entire image
        for (size_t line = 0; line < (size_t)f.img_height; line++)
        {
            for (size_t pixel = 0; pixel < (size_t)f.img_width; pixel++)
            {
                get_channel_vals(channelValues, inputChannels, f, line, pixel);

                // Do the math
                double *rgbOut = rgbParser.Eval(outValsCnt);

                // Get output
                R = rgbOut[0];
                if (isRgb || isRgbA)
                {
                    G = rgbOut[1];
                    B = rgbOut[2];
                }
                if (isRgbA)
                    A = rgbOut[3];

                // Write output
                rgb_output.setf(img_fullch * 0 + line * f.img_width + pixel, rgb_output.clampf(R));
                if (isRgb || isRgbA)
                {
                    rgb_output.setf(img_fullch * 1 + line * f.img_width + pixel, rgb_output.clampf(G));
                    rgb_output.setf(img_fullch * 2 + line * f.img_width + pixel, rgb_output.clampf(B));
                }
                if (isRgbA)
                    rgb_output.setf(img_fullch * 3 + line * f.img_width + pixel, rgb_output.clampf(A));
            }

            if (progress != nullptr)
                *progress = (float)line / (float)f.img_height;
        }

        delete[] channelValues;

        return rgb_output;
    }

    // Generate a composite from channels and a LUT
    Image generate_composite_from_lut(std::vector<Image> &inputChannels, std::vector<std::string> channelNumbers, std::string lut_path, nlohmann::json offsets_cfg, float *progress)
    {
        Image lut;
        load_png(lut, lut_path);

        compo_cfg_t f = get_compo_cfg(inputChannels, channelNumbers, offsets_cfg);

        // Compute channel variable names
        double *channelValues = new double[inputChannels.size()];
        for (int i = 0; i < (int)inputChannels.size(); i++)
            channelValues[i] = 0;

        // Output image
        int channels = std::min(3, lut.channels());
        Image rgb_output(lut.depth(), f.img_width, f.img_height, channels);

        // Run though the entire image - One-channel
        if (inputChannels.size() == 1)
        {
            for (size_t line = 0; line < (size_t)f.img_height; line++)
            {
                for (size_t pixel = 0; pixel < (size_t)f.img_width; pixel++)
                {
                    get_channel_vals(channelValues, inputChannels, f, line, pixel);

                    // Apply the LUT
                    int position = channelValues[0] * lut.width();
                    if (position >= (int)lut.width())
                        position = (int)lut.width() - 1;

                    for (int c = 0; c < channels; c++)
                        rgb_output.set(c, pixel, line, lut.get(c, position));
                }

                if (progress != nullptr)
                    *progress = (float)line / (float)f.img_height;
            }
        }

        // Run though the entire image - Two-channel
        else if (inputChannels.size() == 2)
        {
            for (size_t line = 0; line < (size_t)f.img_height; line++)
            {
                for (size_t pixel = 0; pixel < (size_t)f.img_width; pixel++)
                {
                    get_channel_vals(channelValues, inputChannels, f, line, pixel);

                    // Apply the LUT
                    int position_x = channelValues[0] * lut.width();
                    int position_y = channelValues[1] * lut.height();

                    if (position_x >= (int)lut.width())
                        position_x = (int)lut.width() - 1;

                    if (position_y >= (int)lut.height())
                        position_y = (int)lut.height() - 1;

                    for (int c = 0; c < channels; c++)
                        rgb_output.set(c, pixel, line, lut.get(c, position_x, position_y));

                    // logger->critical("%d, %d, %d", rgb_output.channel(0)[line * img_width + pixel], rgb_output.channel(1)[line * img_width + pixel], rgb_output.channel(2)[line * img_width + pixel]);
                }

                if (progress != nullptr)
                    *progress = (float)line / (float)f.img_height;
            }
        }

        delete[] channelValues;

        return rgb_output;
    }

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
    Image generate_composite_from_lua(satdump::ImageProducts *img_pro, std::vector<Image> &inputChannels, std::vector<std::string> channelNumbers, std::string lua_path, nlohmann::json lua_vars, nlohmann::json offsets_cfg, std::vector<double> *final_timestamps, float *progress)
    {
        compo_cfg_t f = get_compo_cfg(inputChannels, channelNumbers, offsets_cfg);

        // Compute channel variable names
        double *channelValues = new double[inputChannels.size()];
        for (int i = 0; i < (int)inputChannels.size(); i++)
            channelValues[i] = 0;

        // Output image
        Image rgb_output; //(f.img_width, f.img_height, 3);

        try
        {
            sol::state lua;

            lua.open_libraries(sol::lib::base);
            lua.open_libraries(sol::lib::string);
            lua.open_libraries(sol::lib::math);

            lua_utils::bindLogger(lua);
            lua_utils::bindImageTypes(lua);
            lua_utils::bindGeoTypes(lua);
            lua_utils::bindSatProjType(lua);
            lua_utils::bindEquProjType(lua);

            lua["has_sat_proj"] = [img_pro]()
            { return img_pro->has_proj_cfg() /*&& img_pro->has_tle() && img_pro->has_timestamps*/; };
            if (final_timestamps != nullptr)
                lua["get_sat_proj"] = [img_pro, &final_timestamps]()
                { return satdump::get_sat_proj(img_pro->get_proj_cfg(), img_pro->get_tle(), *final_timestamps, true); };
            lua["get_resource_path"] = resources::getResourcePath;

            lua.script_file(lua_path);

            lua["lua_vars"] = lua_utils::mapJsonToLua(lua, lua_vars);

            int n_ch = lua["init"]().get<int>();
            if (n_ch == 0)
                return rgb_output;

            rgb_output.init(f.img_depth, f.img_width, f.img_height, n_ch);

            lua["rgb_output"] = rgb_output;
            lua["compo_cfg"] = f;
            lua["set_img_out"] = [&rgb_output](int c, size_t x, size_t y, double v)
            {
                if (y >= rgb_output.height())
                    return;
                if (x >= rgb_output.width())
                    return;
                rgb_output.setf(c, x, y, rgb_output.clampf(v));
            };
            lua["get_channel_value"] = [channelValues](int x)
            { return channelValues[x]; };
            lua["get_channel_values"] = [channelValues, &inputChannels, &channelNumbers, &f](size_t x, size_t y)
            { get_channel_vals(channelValues, inputChannels, f, y, x); };
            lua["get_channel_image"] = [&inputChannels](int ch)
            { return inputChannels[ch]; };
            lua["get_calibrated_image"] = [img_pro](int ch, std::string type, float min, float max)
            { 
                satdump::ImageProducts::calib_vtype_t ctype = satdump::ImageProducts::CALIB_VTYPE_AUTO;

                if(type == "auto")
                    ctype = satdump::ImageProducts::CALIB_VTYPE_AUTO;
                else if(type == "albedo")
                    ctype = satdump::ImageProducts::CALIB_VTYPE_ALBEDO;
                else if(type == "radiance")
                    ctype = satdump::ImageProducts::CALIB_VTYPE_RADIANCE;
                else if(type == "temperature")
                    ctype = satdump::ImageProducts::CALIB_VTYPE_TEMPERATURE;

                return img_pro->get_calibrated_image(ch, nullptr, ctype, {min, max}); };
            lua["get_calibrated_value"] = [img_pro](int ch, int x, int y, bool temp = false)
            { return img_pro->get_calibrated_value(ch, x, y, temp); };
            lua["set_progress"] = [&progress](float x, float y)
            { if(progress != nullptr) *progress = x / y; };

            lua["process"]();
        }
        catch (std::exception &e)
        {
            logger->error("Error generating composite! %s", e.what());
        }

        delete[] channelValues;

        return rgb_output;
    }
}