#pragma once

#include "common/image/image.h"
#include "common/utils.h"
#include "core/exception.h"
#include "logger.h"
#include "nlohmann/json.hpp"
#include <cstdint>
#include <vector>

namespace sstv
{
    class SSTVLineProc
    {
    public:
        int mode = 0;
        int output_width = 0;

        std::vector<uint8_t> getLine(std::vector<float> &line, float img_offset, float img_time, float line_time, int img_width)
        {
            int line_length = line.size();
            std::vector<uint8_t> l;
            for (int x = 0; x < img_width; x++)
            {
                float p = x / double(img_width - 1);
                int pos = ((img_offset + p * img_time) / line_time) * (line_length - 1);
                l.push_back(line[pos] * 255);
            }
            return l;
        }

    public: // COLOR Utils
        int clamp(int v)
        {
            if (v > 255)
                v = 255;
            if (v < 0)
                v = 0;
            return v;
        }

        void YUV2RGB(int Y, int U, int V, int &R, int &G, int &B)
        {
            Y -= 16;
            U -= 128;
            V -= 128;
            R = clamp((298 * Y + 409 * V + 128) >> 8);
            G = clamp((298 * Y - 100 * U - 208 * V + 128) >> 8);
            B = clamp((298 * Y + 516 * U + 128) >> 8);
        }

    public:
        struct GrayscaleCfg
        {
            float samplerate;
            float line_time;
            float img_offset;
            float img_time;
            int img_width;

            NLOHMANN_DEFINE_TYPE_INTRUSIVE(GrayscaleCfg, samplerate, line_time, img_offset, img_time, img_width)
        };

        GrayscaleCfg grayscale_cfg;

        std::vector<uint32_t> procGrayscale(GrayscaleCfg &cfg, std::vector<float> &line)
        {
            auto l = getLine(line, cfg.img_offset, cfg.img_time, cfg.line_time, cfg.img_width);
            std::vector<uint32_t> lo;
            for (auto &i : l)
                lo.push_back(i << 24 | i << 16 | i << 8 | 0);
            return lo;
        }

    public:
        struct RobotCfg
        {
            float samplerate;
            float line_time;
            float color_sync_offset;
            float color_sync_time;
            float color_offset_y;
            float color_offset_uv;
            float color_time_y;
            float color_time_uv;
            int img_width;

            std::vector<uint8_t> previous_y;
            std::vector<uint8_t> previous_uv;

            NLOHMANN_DEFINE_TYPE_INTRUSIVE(RobotCfg, samplerate, line_time, color_sync_offset, color_sync_time, color_offset_y, color_offset_uv, color_time_y, color_time_uv, img_width)
        };

        RobotCfg robot_cfg;

        std::vector<uint32_t> procRobot(RobotCfg &cfg, std::vector<float> &line)
        {
            int line_length = line.size();

            // Color Sync
            double color_sync = 0;
            int color_sync_length = int(cfg.color_sync_time * cfg.samplerate);

            std::vector<double> colrsync_wip;
            for (int i = 0; i < color_sync_length; i++)
            {
                int p_t = ((cfg.color_sync_offset + (i / double(color_sync_length - 1)) * cfg.color_sync_time) / cfg.line_time) * (line_length - 1);
                //  color_sync += newimg_sync.get(line * line_length + p_t) / 255.0;
                colrsync_wip.push_back(line[p_t]);
            }
            color_sync = get_median(colrsync_wip); // /= color_sync_length;

            bool color_sync_v = color_sync > 0.5;

            logger->trace("Color Sync %d", color_sync_v);

            // Y / UV lines
            auto l_y = getLine(line, cfg.color_offset_y, cfg.color_time_y, cfg.line_time, cfg.img_width);
            auto l_uv = getLine(line, cfg.color_offset_uv, cfg.color_time_uv, cfg.line_time, cfg.img_width);

            // We can only decode after we accumulated 2
            if (!color_sync_v)
            {
                cfg.previous_y = l_y;
                cfg.previous_uv = l_uv;

                return {};
            }
            else
            {
                std::vector<uint32_t> ol(cfg.img_width * 2);

                for (int x = 0; x < cfg.img_width; x++)
                {
                    int r1, g1, b1;
                    int r2, g2, b2;

                    YUV2RGB(cfg.previous_y[x], l_uv[x], cfg.previous_uv[x], r1, g1, b1);
                    YUV2RGB(l_y[x], l_uv[x], cfg.previous_uv[x], r2, g2, b2);

                    ol[cfg.img_width * 0 + x] = r1 << 24 | g1 << 16 | b1 << 8 | 0;
                    ol[cfg.img_width * 1 + x] = r2 << 24 | g2 << 16 | b2 << 8 | 0;
                }

                return ol;
            }
        }

    public:
        struct YUV2YCfg
        {
            float samplerate;
            float line_time;
            float color_offset_y1;
            float color_offset_y2;
            float color_offset_u;
            float color_offset_v;
            float color_time_y1;
            float color_time_y2;
            float color_time_u;
            float color_time_v;
            int img_width;

            NLOHMANN_DEFINE_TYPE_INTRUSIVE(YUV2YCfg, samplerate, line_time, color_offset_y1, color_offset_y2, color_offset_u, color_offset_v, color_time_y1, color_time_y2, color_time_u, color_time_v,
                                           img_width)
        };

        YUV2YCfg yuv2y_cfg;

        std::vector<uint32_t> procYUV2Y(YUV2YCfg &cfg, std::vector<float> &line)
        {
            int line_length = line.size();

            // Y / UV lines
            auto l_y1 = getLine(line, cfg.color_offset_y1, cfg.color_time_y1, cfg.line_time, cfg.img_width);
            auto l_u = getLine(line, cfg.color_offset_u, cfg.color_time_u, cfg.line_time, cfg.img_width);
            auto l_v = getLine(line, cfg.color_offset_v, cfg.color_time_v, cfg.line_time, cfg.img_width);
            auto l_y2 = getLine(line, cfg.color_offset_y2, cfg.color_time_y2, cfg.line_time, cfg.img_width);

            std::vector<uint32_t> ol(cfg.img_width * 2);

            for (int x = 0; x < cfg.img_width; x++)
            {
                int r1, g1, b1;
                int r2, g2, b2;

                YUV2RGB(l_y1[x], l_u[x], l_v[x], r1, g1, b1);
                YUV2RGB(l_y2[x], l_u[x], l_v[x], r2, g2, b2);

                ol[cfg.img_width * 0 + x] = r1 << 24 | g1 << 16 | b1 << 8 | 0;
                ol[cfg.img_width * 1 + x] = r2 << 24 | g2 << 16 | b2 << 8 | 0;
            }

            return ol;
        }

    public:
        void setCfg(nlohmann::json c)
        {
            if (c["mode"] == "grayscale")
            {
                mode = 0;
                grayscale_cfg = c;
            }
            else if (c["mode"] == "robot")
            {
                mode = 1;
                robot_cfg = c;
                robot_cfg.previous_y.resize(robot_cfg.img_width);
                robot_cfg.previous_uv.resize(robot_cfg.img_width);
            }
            else if (c["mode"] == "yuv_2y")
            {
                mode = 2;
                yuv2y_cfg = c;
            }
            else
                throw satdump_exception("Invalid SSTV lineproc mode!");

            output_width = c["img_width"];
        }

        std::vector<uint32_t> lineProc(std::vector<float> line)
        {
            if (mode == 0)
                return procGrayscale(grayscale_cfg, line);
            else if (mode == 1)
                return procRobot(robot_cfg, line);
            else if (mode == 2)
                return procYUV2Y(yuv2y_cfg, line);
            else
            {
                return {};
            }
        }
    };

    image::Image rgbaToImg(std::vector<uint32_t> &vec, int width)
    {
        image::Image img(8, width, vec.size() / width, 3);
        for (int i = 0; i < img.width() * img.height(); i++)
        {
            img.set(0, i, (vec[i] >> 24) & 0xFF);
            img.set(1, i, (vec[i] >> 16) & 0xFF);
            img.set(2, i, (vec[i] >> 8) & 0xFF);
        }
        return img;
    }
} // namespace sstv