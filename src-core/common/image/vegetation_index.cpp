#include "vegetation_index.h"

namespace image{
    namespace vegetation_index{
        cimg_library::CImg<unsigned short> NDVI(cimg_library::CImg<unsigned short> redIm, cimg_library::CImg<unsigned short> nirIm){
            cimg_library::CImg<unsigned short> out(redIm.width(), redIm.height(), 1, 1);
            for (unsigned int i = 0; i<redIm.size(); i++){
                float red = redIm[i];
                float nir = nirIm[i];
                out[i] = (((nir - red)/(nir + red))+1)*32726;
            }
            return out;
        }
        cimg_library::CImg<unsigned short> EVI2(cimg_library::CImg<unsigned short> redIm, cimg_library::CImg<unsigned short> nirIm){
            cimg_library::CImg<unsigned short> out(redIm.width(), redIm.height(), 1, 1);
            for (unsigned int i = 0; i<redIm.size(); i++){
                float red = redIm[i];
                float nir = nirIm[i];
                out[i] = 2.5*(((nir - red)/(nir + 2.4 * red + 1))+1)*32726;
            }
            return out;
        }
        cimg_library::CImg<unsigned short> EVI(cimg_library::CImg<unsigned short> redIm, cimg_library::CImg<unsigned short> nirIm, cimg_library::CImg<unsigned short> blueIm){
            cimg_library::CImg<unsigned short> out(redIm.width(), redIm.height(), 1, 1);
            for (unsigned int i = 0; i<redIm.size(); i++){
                float red = redIm[i];
                float nir = nirIm[i];
                float blue = blueIm[i];
                out[i] = 2.5*(((nir - red) / (nir + 6 * red - 7.5 * blue + 1))+1)*32726;
            }
            return out;
        }
    }
}