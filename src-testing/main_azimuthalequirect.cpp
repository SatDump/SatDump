/**********************************************************************
 * This file is used for testing random stuff without running the
 * whole of SatDump, which comes in handy for debugging individual
 * elements before putting them all together in modules...
 *
 * If you are an user, ignore this file which will not be built by
 * default, and if you're a developper in need of doing stuff here...
 * Go ahead!
 *
 * Don't judge the code you might see in there! :)
 **********************************************************************/

#include "logger.h"
#include "common/image/image.h"
#include "common/projection/projs/azimuthal_equidistant.h"
#include "common/projection/projs/equirectangular.h"

int main(int /*argc*/, char *argv[])
{
    initLogger();

    image::Image<uint8_t> in_img;
    in_img.load_jpeg("./resources/maps/nasa_hd.jpg");

    image::Image<uint8_t> outimg(4096, 2048, 3);

    geodetic::projection::AzimuthalEquidistantProjection aep;
    aep.init(1000, 1000, 151, -33);

    geodetic::projection::EquirectangularProjection erp;
    erp.init(4096, 2048, -180, 90, 180, -90);
    
    for (int x = 0; x < outimg.width(); x++)
    {
        for (int y = 0; y < outimg.height(); y++)
        {
            float lat, lon;
            //aep.reverse(x, y, lon, lat);
            int px, py;
            erp.reverse(x, y, lon ,lat);
            //if (px >= 0 && px < in_img.width() && py >= 0 && py < in_img.height() && lat != -1 && lon != -1){
                uint8_t color[3] = {((lat+90)*255)/180, ((lon+180)*255)/360, 0};
                //for (int i = 0; i < 3; i++)
                    //color[i] = in_img.channel(i)[py*in_img.width() + px];
                //logger->info(lon);
                outimg.draw_pixel(x, y, color);
            //}
        }
    }
    /*
    for (float lon = -180; lon <= 180; lon+=0.1)
    {
        for (float lat = -90; lat <= 90; lat+=0.1)
        {
            int x, y;
            aep.forward(lon, lat, x, y);
            uint8_t color[3] = {lat+90, (lon+180)/2, 0};
            outimg.draw_pixel(x, y, color);
        }
    }
    */
    outimg.save_png("projtest.png");
}
