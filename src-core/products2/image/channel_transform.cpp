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

        interpx_fwd_interpolator = std::make_shared<projection::VizGeorefSpline2D>(1);
        interpx_rev_interpolator = std::make_shared<projection::VizGeorefSpline2D>(1);

        for (auto &p : interpx_points)
        {
            interpx_fwd_interpolator->add_point(p.first, p.first, &p.second);
            interpx_rev_interpolator->add_point(p.second, p.second, &p.first);
        }

        interpx_fwd_interpolator->solve();
        interpx_rev_interpolator->solve();

        return *this;
    }
}