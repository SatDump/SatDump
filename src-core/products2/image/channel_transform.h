#pragma once

#include "nlohmann/json.hpp"
#include "core/exception.h"

namespace satdump
{
    class ChannelTransform
    {
    public:
        enum transform_type_t
        {
            TYPE_INVALID = 0,
            TYPE_NONE = 1,
            TYPE_AFFINE = 2,
            TYPE_AFFINE_SLANTX = 3,
        };

    private:
        transform_type_t d_type = TYPE_INVALID;

        double affine_a_x;
        double affine_a_y;
        double affine_b_x;
        double affine_b_y;

        double slantx_center;
        double slantx_ampli;

    public:
        // ChannelTransform();
        // ~ChannelTransform();

        ChannelTransform &init_affine(double a_x, double a_y, double b_x, double b_y)
        {
            d_type = TYPE_AFFINE;
            affine_a_x = a_x;
            affine_a_y = a_y;
            affine_b_x = b_x;
            affine_b_y = b_y;
            return *this;
        }

        ChannelTransform &init_affine_slantx(double a_x, double a_y, double b_x, double b_y, double slantx_center, double slantx_ampli)
        {
            d_type = TYPE_AFFINE_SLANTX;
            affine_a_x = a_x;
            affine_a_y = a_y;
            affine_b_x = b_x;
            affine_b_y = b_y;
            this->slantx_center = slantx_center;
            this->slantx_ampli = slantx_ampli;
            return *this;
        }

        friend void to_json(nlohmann::json &j, const ChannelTransform &v)
        {
            j["type"] = (int)v.d_type;

            if (v.d_type == TYPE_AFFINE || v.d_type == TYPE_AFFINE_SLANTX)
            {
                j["ax"] = v.affine_a_x;
                j["ay"] = v.affine_a_y;
                j["bx"] = v.affine_b_x;
                j["by"] = v.affine_b_y;
            }

            if (v.d_type == TYPE_AFFINE_SLANTX)
            {
                j["slantx_center"] = v.slantx_center;
                j["slantx_ampli"] = v.slantx_ampli;
            }
        }

        friend void from_json(const nlohmann::json &j, ChannelTransform &v)
        {
            v.d_type = (transform_type_t)j["type"].get<int>();

            if (v.d_type == TYPE_AFFINE || v.d_type == TYPE_AFFINE_SLANTX)
            {
                v.affine_a_x = j["ax"];
                v.affine_a_y = j["ay"];
                v.affine_b_x = j["bx"];
                v.affine_b_y = j["by"];
            }

            if (v.d_type == TYPE_AFFINE_SLANTX)
            {
                v.slantx_center = j["slantx_center"];
                v.slantx_ampli = j["slantx_ampli"];
            }
        }

        transform_type_t getType();

        // To reference
        void forward(double *x, double *y);
        // To reference
        void reverse(double *x, double *y);

        // UI
        void render();
    };
}