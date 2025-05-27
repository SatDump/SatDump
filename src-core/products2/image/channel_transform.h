#pragma once

/**
 * @file channel_transform.h
 */

#include "nlohmann/json.hpp"
#include "core/exception.h"

#include "projection/thinplatespline.h" // TODOREWORK Move to here?

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
     * - 4, Affine + Arbtitrary interpolation on the X axis (eg, for VIIRS with discountinuties between channels)
     * - 5, TODOREWORK
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
            TYPE_AFFINE_INTERPX = 4,
            TYPE_INTERP_XY = 5,
        };

    private:
        transform_type_t d_type = TYPE_INVALID;

        double affine_a_x;
        double affine_a_y;
        double affine_b_x;
        double affine_b_y;

        double slantx_center;
        double slantx_ampli;

        std::vector<std::pair<double, double>> interpx_points;
        std::vector<std::pair<std::pair<double, double>, std::pair<double, double>>> interp_xy_points;
        std::shared_ptr<projection::VizGeorefSpline2D> interp_fwd_interpolator;
        std::shared_ptr<projection::VizGeorefSpline2D> interp_rev_interpolator;

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

        /**
         * @brief Initialize an AFFINE_INTERPX transform
         * @param a_x affine a for x axis
         * @param a_y affine a for y axis
         * @param b_x affine b for x axis
         * @param b_y affine b for y axis
         * @param interpx_points points for x axis interpolation
         */
        ChannelTransform &init_affine_interpx(double a_x, double a_y, double b_x, double b_y, std::vector<std::pair<double, double>> interpx_points);

        // TODOREWORK
        ChannelTransform &init_interp_xy(std::vector<std::pair<std::pair<double, double>, std::pair<double, double>>> interp_xy_points);

    public:
        friend void to_json(nlohmann::json &j, const ChannelTransform &v)
        {
            j["type"] = (int)v.d_type;

            if (v.d_type == TYPE_AFFINE ||
                v.d_type == TYPE_AFFINE_SLANTX ||
                v.d_type == TYPE_AFFINE_INTERPX)
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

            if (v.d_type == TYPE_AFFINE_INTERPX)
            {
                j["interpx_points"] = v.interpx_points; // v.fwd_interpolator.get_points();
            }

            if (v.d_type == TYPE_INTERP_XY)
            {
                j["interp_xy_points"] = v.interp_xy_points; // v.fwd_interpolator.get_points();
            }
        }

        friend void from_json(const nlohmann::json &j, ChannelTransform &v)
        {
            v.d_type = (transform_type_t)j["type"].get<int>();

            if (v.d_type == TYPE_AFFINE ||
                v.d_type == TYPE_AFFINE_SLANTX ||
                v.d_type == TYPE_AFFINE_INTERPX)
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

            if (v.d_type == TYPE_AFFINE_INTERPX)
            {
                v.interpx_points = j["interpx_points"].get<std::vector<std::pair<double, double>>>();

                v.interp_fwd_interpolator = std::make_shared<projection::VizGeorefSpline2D>(1);
                v.interp_rev_interpolator = std::make_shared<projection::VizGeorefSpline2D>(1);

                for (auto &p : v.interpx_points)
                {
                    v.interp_fwd_interpolator->add_point(p.first, p.first, &p.second);
                    v.interp_rev_interpolator->add_point(p.second, p.second, &p.first);
                }

                v.interp_fwd_interpolator->solve();
                v.interp_rev_interpolator->solve();
            }

            if (v.d_type == TYPE_INTERP_XY)
            {
                v.interp_xy_points = j["interp_xy_points"].get<std::vector<std::pair<std::pair<double, double>, std::pair<double, double>>>>();

                v.interp_fwd_interpolator = std::make_shared<projection::VizGeorefSpline2D>(2);
                v.interp_rev_interpolator = std::make_shared<projection::VizGeorefSpline2D>(2);

                for (auto &p : v.interp_xy_points)
                {
                    printf("Point %f %f => %f %f\n", p.first.first, p.first.second, p.second.first, p.second.second);
                    double p1[2] = {p.second.first, p.second.second};
                    v.interp_fwd_interpolator->add_point(p.first.first, p.first.second, p1);
                    double p2[2] = {p.first.first, p.first.second};
                    v.interp_rev_interpolator->add_point(p.second.first, p.second.second, p2);
                }

                v.interp_fwd_interpolator->solve();
                v.interp_rev_interpolator->solve();
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
        inline void forward(double *x, double *y) const
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
            else if (d_type == TYPE_AFFINE_INTERPX)
            {
                *x = *x * affine_a_x + affine_b_x;
                *y = *y * affine_a_y + affine_b_y;
                interp_fwd_interpolator->get_point(*x, *x, x);
            }
            else if (d_type == TYPE_INTERP_XY)
            {
                double out[2]; // TODOREWORK
                printf("%f %f FWD => ", *x, *y);
                interp_fwd_interpolator->get_point(*x, *y, out);
                *x = out[0];
                *y = out[1];
                printf("%f %f\n ", *x, *y);
            }
            else if (d_type == TYPE_INVALID)
                throw satdump_exception("Invalid Channel Transform!\n");
        }

        /**
         * @brief Convert reference coordinates to channel coordinates
         * @param x coordinate to be transformed
         * @param y coordinate to be transformed
         */
        inline void reverse(double *x, double *y) const
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
            else if (d_type == TYPE_AFFINE_INTERPX)
            {
                interp_rev_interpolator->get_point(*x, *x, x);
                *x = (*x - affine_b_x) / affine_a_x;
                *y = (*y - affine_b_y) / affine_a_y;
            }
            else if (d_type == TYPE_INTERP_XY)
            {
                double out[2]; // TODOREWORK
                printf("%f %f REV => ", *x, *y);
                interp_rev_interpolator->get_point(*x, *y, out);
                *x = out[0];
                *y = out[1];
                printf("%f %f\n ", *x, *y);
            }
            else if (d_type == TYPE_INVALID)
                throw satdump_exception("Invalid Channel Transform!\n");
        }
    };
}