#include "product_dotmap.h"
#include "common/image/io.h"
#include "common/image/meta.h"

namespace satdump
{
    namespace products
    {
        image::Image generate_dotmap_product_image(PunctiformProduct *product, std::string channel, int width, int height,
                                                   int dotsize, bool background, bool use_lut,
                                                   double min, double max, float *progress)
        {
            image::Image map;

            if (background)
            {
                image::load_jpeg(map, "resources/maps/nasa.jpg");
                if (width != -1 && height != -1)
                    map.resize_bilinear(width, height);
            }
            else
            {
                map.init(8, width, height, 3);
            }

            int chid = product->getChannelIndexByName(channel);
            auto &ch = product->getChannelByName(channel);
            int img_x = map.width();
            int img_y = map.height();

            for (int i = 0; i < ch.data.size(); i++)
            {
                auto satpos = product->get_sample_position(chid, i); // tracker.get_sat_position_at(v.timestamps[i]);

                int image_y = img_y - ((90.0f + satpos.lat) / 180.0f) * img_y;
                int image_x = (satpos.lon / 360) * img_x + (img_x / 2);

                image_y = image_y % img_y;
                image_x = image_x % img_x;

                // printf("%d %d %f\n", image_x, image_y, ch.data[i]);

                // std::vector<double> color = {(p.data[2].data[i].value - 12500) / 1000.0,
                //                              (p.data[1].data[i].value - 12600) / 1000.0,
                //                              (p.data[0].data[i].value - 12500) / 1000.0,
                //                              1};

                // std::vector<double> color = {(p.data[1].data[i].value - 12200) / 1000.0,
                //                              (p.data[1].data[i].value - 12200) / 1000.0,
                //                              (p.data[1].data[i].value - 12200) / 1000.0,
                //                              1};

                // std::vector<double> color = {(p.data[1].data[i].value - 13400) / 1000.0,
                //                              (p.data[1].data[i].value - 13400) / 1000.0,
                //                              (p.data[1].data[i].value - 13400) / 1000.0,
                //                              1};

                std::vector<double> color = {(ch.data[i] - min) / (max - min),
                                             (ch.data[i] - min) / (max - min),
                                             (ch.data[i] - min) / (max - min),
                                             1};

                for (auto &c : color)
                {
                    if (c > 1.0)
                        c = 1;
                    if (c < 0)
                        c = 0;
                }

                map.draw_circle(image_x % img_x, image_y, 2, color, true);
            }

            nlohmann::json proj_cfg;
            double tl_lon = -180;
            double tl_lat = 90;
            double br_lon = 180;
            double br_lat = -90;
            proj_cfg["type"] = "equirec";
            proj_cfg["offset_x"] = tl_lon;
            proj_cfg["offset_y"] = tl_lat;
            proj_cfg["scalar_x"] = (br_lon - tl_lon) / double(map.width());
            proj_cfg["scalar_y"] = (br_lat - tl_lat) / double(map.height());
            proj_cfg["width"] = map.width();
            proj_cfg["height"] = map.height();
            image::set_metadata_proj_cfg(map, proj_cfg);

            return map;
        }
    }
}