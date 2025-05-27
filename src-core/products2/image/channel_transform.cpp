#include "channel_transform.h"

namespace satdump
{
    ChannelTransform::transform_type_t ChannelTransform::getType()
    {
        return d_type;
    }

    ChannelTransform &ChannelTransform::init_none()
    {
        d_type = TYPE_NONE;
        return *this;
    }

    ChannelTransform &ChannelTransform::init_affine(double a_x, double a_y, double b_x, double b_y)
    {
        d_type = TYPE_AFFINE;
        affine_a_x = a_x;
        affine_a_y = a_y;
        affine_b_x = b_x;
        affine_b_y = b_y;
        return *this;
    }

    ChannelTransform &ChannelTransform::init_affine_slantx(double a_x, double a_y, double b_x, double b_y, double slantx_center, double slantx_ampli)
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

    ChannelTransform &ChannelTransform::init_affine_interpx(double a_x, double a_y, double b_x, double b_y, std::vector<std::pair<double, double>> interpx_points)
    {
        d_type = TYPE_AFFINE_INTERPX;
        affine_a_x = a_x;
        affine_a_y = a_y;
        affine_b_x = b_x;
        affine_b_y = b_y;

        this->interpx_points = interpx_points;

        interp_fwd_interpolator = std::make_shared<projection::VizGeorefSpline2D>(1);
        interp_rev_interpolator = std::make_shared<projection::VizGeorefSpline2D>(1);

        for (auto &p : interpx_points)
        {
            interp_fwd_interpolator->add_point(p.first, p.first, &p.second);
            interp_rev_interpolator->add_point(p.second, p.second, &p.first);
        }

        interp_fwd_interpolator->solve();
        interp_rev_interpolator->solve();

        return *this;
    }

    ChannelTransform &ChannelTransform::init_interp_xy(std::vector<std::pair<std::pair<double, double>, std::pair<double, double>>> interp_xy_points)
    {
        d_type = TYPE_INTERP_XY;

        this->interp_xy_points = interp_xy_points;

        interp_fwd_interpolator = std::make_shared<projection::VizGeorefSpline2D>(2);
        interp_rev_interpolator = std::make_shared<projection::VizGeorefSpline2D>(2);

        for (auto &p : interp_xy_points)
        {
            printf("Point %f %f => %f %f\n", p.first.first, p.first.second, p.second.first, p.second.second);
            double p1[2] = {p.second.first, p.second.second};
            interp_fwd_interpolator->add_point(p.first.first, p.first.second, p1);
            double p2[2] = {p.first.first, p.first.second};
            interp_rev_interpolator->add_point(p.second.first, p.second.second, p2);
        }

        interp_fwd_interpolator->solve();
        interp_rev_interpolator->solve();

        return *this;
    }
}