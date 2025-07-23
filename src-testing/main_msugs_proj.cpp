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
#include "init.h"
#include "products/image_product.h"
#include "image/io.h"
#include "image/processing.h"

#include "products/image/product_expression.h"
#include "products/image/image_calibrator.h"

#include "common/utils.h"

#include "projection/reprojector.h"

#include "nlohmann/json_utils.h"

#include "common/projection/projs2/proj_json.h"

#include "image/meta.h"

#include "common/projection/warp/warp.h"
#include "common/projection/warp/warp_bkd.h"
#include "projection/raytrace/gcp_compute.h"

#include "common/overlay_handler.h"

#include "normal_line_xy_proj.h"

#include "core/plugin.h"

int main(int argc, char *argv[])
{
    initLogger();

    logger->set_level(slog::LOG_OFF);
    satdump::initSatdump();
    completeLoggerInit();
    logger->set_level(slog::LOG_TRACE);

    auto v = [](const satdump::proj::RequestSatelliteRaytracerEvent &evt)
    {
        logger->info("REGISTER!!!! " + evt.id);
        if (evt.id == "normal_single_xy_line")
        {
            try
            {
                logger->info("CALLED!");
                evt.r.push_back(std::make_shared<satdump::proj::NormalLineXYSatProj>(evt.cfg));
            }
            catch (std::exception &e)
            {
                logger->error(e.what());
                throw e;
            }
        }
    };
    satdump::eventBus->register_handler<satdump::proj::RequestSatelliteRaytracerEvent>(v);

    satdump::products::ImageProduct pro;
    pro.load(argv[1]);

    auto proj_cfg = loadJsonFile(argv[2]);
    auto old_cfg = pro.get_proj_cfg(0);

    if (old_cfg.contains("tle"))
        proj_cfg["tle"] = old_cfg["tle"];
    if (old_cfg.contains("ephemeris"))
        proj_cfg["ephemeris"] = old_cfg["ephemeris"];
    if (old_cfg.contains("timestamps"))
        proj_cfg["timestamps"] = old_cfg["timestamps"];
    pro.set_proj_cfg(proj_cfg);

    pro.get_channel_image("2").ch_transform.init_affine_slantx(1.0008, 1, -8, -1789.5, 0, 0.0047);

    auto img = satdump::products::generate_expression_product_composite(&pro, "ch1 * 0 + ch2, ch2, ch1");

    {
        auto prj_cfg = image::get_metadata_proj_cfg(img);
        satdump::warp::WarpOperation operation;
        prj_cfg["width"] = img.width();
        prj_cfg["height"] = img.height();
        operation.ground_control_points = satdump::proj::compute_gcps(prj_cfg);
        operation.input_image = &img;
        operation.output_rgba = true;

        int l_width = prj_cfg.contains("f_width") ? prj_cfg["f_width"].get<int>() : std::max<int>(img.width(), 512) * 15;
        operation.output_width = l_width;
        operation.output_height = l_width / 2;

        logger->trace("Warping size %dx%d", l_width, l_width / 2);

#if 0
        satdump::warp::WarpResult result = satdump::warp::performSmartWarp(operation);
#else
        satdump::warp::ImageWarper wrapper;
        wrapper.op = operation;
        wrapper.update();
        satdump::warp::WarpResult result = wrapper.warp();
#endif

        auto src_proj = ::proj::projection_t();
        src_proj.type = ::proj::ProjType_Equirectangular;
        src_proj.proj_offset_x = result.top_left.lon;
        src_proj.proj_offset_y = result.top_left.lat;
        src_proj.proj_scalar_x = (result.bottom_right.lon - result.top_left.lon) / double(result.output_image.width());
        src_proj.proj_scalar_y = (result.bottom_right.lat - result.top_left.lat) / double(result.output_image.height());

        image::set_metadata_proj_cfg(result.output_image, src_proj);
        img = result.output_image;
    }

    // Reproj to GEOS
    {
        double mult = 4;

        int width = 2784 * mult, /* 5424,*/ height = 2784 * mult; // 5424;
        proj_cfg["type"] = "geos";
        proj_cfg["lon0"] = 76;
        proj_cfg["sweep_x"] = false; // should_sweep_x;

        //  proj_cfg["scalar_x"] = 2004.0594276757981;
        //  proj_cfg["scalar_y"] = -2004.0594276757981;
        //  proj_cfg["offset_x"] = -5433005.108429089;
        //  proj_cfg["offset_y"] = 5433005.108429089;

        proj_cfg["scalar_x"] = 4000.643016144421 / mult;
        proj_cfg["scalar_y"] = -4000.643016144421 / mult;
        proj_cfg["offset_x"] = -5568895.078473034;
        proj_cfg["offset_y"] = 5568895.078473034;

        proj_cfg["width"] = width;
        proj_cfg["height"] = height;
        proj_cfg["altitude"] = 35786023.00;

        satdump::proj::ReprojectionOperation op;
        op.img = &img;
        op.output_height = height;
        op.output_width = width;
        op.target_prj_info = proj_cfg;

        auto icfg = image::get_metadata_proj_cfg(img);
        icfg["width"] = img.width();
        icfg["height"] = img.height();
        image::set_metadata_proj_cfg(img, icfg);

        img = satdump::proj::reproject(op);
    }

    for (size_t v = 0; v < img.width() * img.height(); v++)
        img.set(3, v, 65535);

#if 0
    image::Image img2;
    // image::load_img(img2, "/home/alan/Downloads/SatDump_NEWPRODS/250104_1015/250104_1015_3.jpg");
    // image::load_img(img2, "/home/alan/Downloads/SatDump_NEWPRODS/2nd_electro/250104_1600/250104_1600_3.jpg");
    image::load_img(img2, "/home/alan/Downloads/SatDump_NEWPRODS/3rd_electro/250104_1300/250104_1300_3.jpg");
    img2 = img2.to16bits();
    img2.resize_bilinear(img.width(), img.height());
    img.draw_image(0, img2);
#endif

    {
        OverlayHandler overlay_handler;

        auto cfg = image::get_metadata_proj_cfg(img);
        //        cfg["width"] = img.width();
        //        cfg["height"] = img.height();
        satdump::proj::Projection p = cfg;
        p.init(1, 0);

        auto pfunc = [p](double lat, double lon, double h, double w) mutable -> std::pair<double, double>
        {
            double x, y;
            if (p.forward(geodetic::geodetic_coords_t(lat, lon, 0, false), x, y) || x < 0 || x >= w || y < 0 || y >= h)
                return {-1, -1};
            else
                return {x, y};
        };

        overlay_handler.draw_map_overlay = true;
        // overlay_handler.draw_shores_overlay = true;
        overlay_handler.draw_latlon_overlay = true;
        // overlay_handler.clear_cache();
        overlay_handler.apply(img, pfunc);
    }

    image::save_img(img, argv[3]);
}
