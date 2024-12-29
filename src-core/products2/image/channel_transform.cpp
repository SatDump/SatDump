#include "channel_transform.h"

namespace satdump
{
    //  ChannelTransform::ChannelTransform()
    //  {
    //  }

    //  ChannelTransform::~ChannelTransform()
    //  {
    //  }

    ChannelTransform::transform_type_t ChannelTransform::getType()
    {
        return d_type;
    }

    // To reference
    void ChannelTransform::forward(double *x, double *y)
    {
        switch (d_type)
        {
        case TYPE_INVALID:
            throw satdump_exception("Invalid Channel Transform!\n");
            break;

        case TYPE_NONE:
            break;

        case TYPE_AFFINE:
            *x = *x * affine_a_x + affine_b_x;
            *y = *y * affine_a_y + affine_b_y;
            break;

        case TYPE_AFFINE_SLANTX:
            *y -= (*x - slantx_center) * slantx_ampli;
            *x = *x * affine_a_x + affine_b_x;
            *y = *y * affine_a_y + affine_b_y;
            break;
        }
    }

    // To reference
    void ChannelTransform::reverse(double *x, double *y)
    {
        switch (d_type)
        {
        case TYPE_INVALID:
            throw satdump_exception("Invalid Channel Transform!\n");
            break;

        case TYPE_NONE:
            break;

        case TYPE_AFFINE:
            *x = (*x - affine_b_x) / affine_a_x;
            *y = (*y - affine_b_y) / affine_a_y;
            break;

        case TYPE_AFFINE_SLANTX:
            *x = (*x - affine_b_x) / affine_a_x;
            *y = (*y - affine_b_y) / affine_a_y;
            *y += (*x - slantx_center) * slantx_ampli;
            break;
        }
    }
}