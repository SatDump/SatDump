#include "vegetation_index.h"

namespace image
{
    namespace vegetation_index
    {
        Image<uint16_t> NDVI(Image<uint16_t> redIm, Image<uint16_t> nirIm)
        {
            Image<uint16_t> out(redIm.width(), redIm.height(), 1);
            for (int i = 0; i < redIm.size(); i++)
            {
                float red = redIm[i];
                float nir = nirIm[i];
                out[i] = (((nir - red) / (nir + red)) + 1) * 32726;
            }
            return out;
        }
        Image<uint16_t> EVI2(Image<uint16_t> redIm, Image<uint16_t> nirIm)
        {
            Image<uint16_t> out(redIm.width(), redIm.height(), 1);
            for (int i = 0; i < redIm.size(); i++)
            {
                float red = redIm[i];
                float nir = nirIm[i];
                out[i] = 2.5 * (((nir - red) / (nir + 2.4 * red + 1)) + 1) * 32726;
            }
            return out;
        }
        Image<uint16_t> EVI(Image<uint16_t> redIm, Image<uint16_t> nirIm, Image<uint16_t> blueIm)
        {
            Image<uint16_t> out(redIm.width(), redIm.height(), 1);
            for (int i = 0; i < redIm.size(); i++)
            {
                float red = redIm[i];
                float nir = nirIm[i];
                float blue = blueIm[i];
                out[i] = 2.5 * (((nir - red) / (nir + 6 * red - 7.5 * blue + 1)) + 1) * 32726;
            }
            return out;
        }
    }
}