#pragma once

/**
 * @file channel_transform.h
 */

#include "nlohmann/json.hpp"
#include "core/exception.h"

namespace satdump
{
    /**
     * @brief Class handling 2D transforms, mainly intended for image channels.
     * This covers a simple but pretty important problem :
     *
     * Quite often, instrument channels are NOT perfectly aligned... Or the same resolution.
     * This therefore requires the ability to shift them around and re-scale them. However,
     * it often gets more complicated than that. For example, MWS (on AWS) or KMSS are made
     * out of several detectors that are not scanning on the same axis, therefore they also
     * need to be rotated around to align channels.
     *
     * Instead of pre-aligning them and discarding data, or handling those cases specifically,
     * this class defines several common or arbitrary transforms to convert each channel to a "reference"
     * field. Therefore, by taking a channel's 2D X/Y coordinates, running it through its own reference's
     * forward and then the target channel's reverse will give the equivalent coordinates on
     * the other channel.
     *
     * Available transforms are :
     * - 0, Invalid : Default, throws an exception
     * - 1, None : Does nothing, use if no transform is required between channels
     * - 2, Affine : ax + b form applied on both axis, meant for simple translations and resolution changes
     * - 3, Affine Slant X : Combines the affine transform and a shift in y identical at any point in x
     */
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
        /**
         * @brief Initialize a NONE transform
         * @param
         */
        ChannelTransform &init_none();

        /**
         * @brief Initialize an AFFINE transform
         * @param a_x affine a for x axis
         * @param a_y affine a for y axis
         * @param b_x affine b for x axis
         * @param b_y affine b for y axis
         */
        ChannelTransform &init_affine(double a_x, double a_y, double b_x, double b_y);

        /**
         * @brief Initialize an AFFINE_SLANTX transform
         * @param a_x affine a for x axis
         * @param a_y affine a for y axis
         * @param b_x affine b for x axis
         * @param b_y affine b for y axis
         * @param slantx_center center of the slant on the x axis
         * @param slantx_ampli multiplicator applied after substracting center
         */
        ChannelTransform &init_affine_slantx(double a_x, double a_y, double b_x, double b_y, double slantx_center, double slantx_ampli);

    public:
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

        /**
         * @brief Get type of this transform
         * @return type of the transform
         */
        transform_type_t getType();

        /**
         * @brief Render ImGui UI to edit the transform
         * @param
         */
        void render();

    public: // Must be inline for performance reasons!
        /**
         * @brief Convert channel coordinates to reference coordinates
         * @param x coordinate to be transformed
         * @param y coordinate to be transformed
         */
        inline void forward(double *x, double *y)
        {
            if (d_type == TYPE_NONE)
                ;
            else if (d_type == TYPE_AFFINE)
            {
                *x = *x * affine_a_x + affine_b_x;
                *y = *y * affine_a_y + affine_b_y;
            }
            else if (d_type == TYPE_AFFINE_SLANTX)
            {
                *y -= (*x - slantx_center) * slantx_ampli;
                *x = *x * affine_a_x + affine_b_x;
                *y = *y * affine_a_y + affine_b_y;
            }
            else if (d_type == TYPE_INVALID)
                throw satdump_exception("Invalid Channel Transform!\n");
        }

        /**
         * @brief Convert reference coordinates to channel coordinates
         * @param x coordinate to be transformed
         * @param y coordinate to be transformed
         */
        inline void reverse(double *x, double *y)
        {
            if (d_type == TYPE_NONE)
                ;
            else if (d_type == TYPE_AFFINE)
            {
                *x = (*x - affine_b_x) / affine_a_x;
                *y = (*y - affine_b_y) / affine_a_y;
            }
            else if (d_type == TYPE_AFFINE_SLANTX)
            {
                *x = (*x - affine_b_x) / affine_a_x;
                *y = (*y - affine_b_y) / affine_a_y;
                *y += (*x - slantx_center) * slantx_ampli;
            }
            else if (d_type == TYPE_INVALID)
                throw satdump_exception("Invalid Channel Transform!\n");
        }
    };
}