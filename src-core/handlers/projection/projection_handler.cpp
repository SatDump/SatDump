#include "projection_handler.h"
#include "core/config.h"
#include "core/style.h"
#include "handlers/vector/addmenu.h"
#include "image/image.h"
#include "image/meta.h"
#include "logger.h"

#include "../image/image_handler.h"
#include "../vector/shapefile_handler.h"

// TODOREWORK
#include "projection/raytrace/gcp_compute.h"
#include "projection/reproject_img.h"
#include "projection/reprojector.h"

#include "projection/standard/proj_json.h"
#include <exception>

namespace satdump
{
    namespace handlers
    {
        ProjectionHandler::ProjectionHandler()
        {
            handler_tree_icon = u8"\uf6e6";
            setCanSubBeReorgTo(true);

            if (!satdump_cfg.main_cfg["user"]["projection_defaults"].is_null())
                setConfig(satdump_cfg.main_cfg["user"]["projection_defaults"]);
        }

        ProjectionHandler::~ProjectionHandler()
        {
            satdump_cfg.main_cfg["user"]["projection_defaults"] = getConfig();
            satdump_cfg.saveUser();
        }

        void ProjectionHandler::drawMenu()
        {
            bool needs_to_be_disabled = is_processing;

            if (ImGui::CollapsingHeader("Projection", ImGuiTreeNodeFlags_DefaultOpen))
            {
                if (needs_to_be_disabled)
                    style::beginDisabled();

                projui.drawUI();

                //            needs_to_update |= TODO; // TODOREWORK move in top drawMenu?
                if (ImGui::Button("Project"))
                    needs_to_update = true;

                if (needs_to_be_disabled)
                    style::endDisabled();
            }

            /// TODOREWORK UPDATE
            if (needs_to_update)
            {
                asyncProcess();
                needs_to_update = false;
            }

            img_handler.drawMenu();
        }

        void ProjectionHandler::setConfig(nlohmann::json p)
        {
            if (p.contains("image"))
                img_handler.setConfig(p["image"]);
            if (p.contains("proj"))
                projui = p["proj"];
        }

        nlohmann::json ProjectionHandler::getConfig()
        {
            nlohmann::json p;

            p["image"] = img_handler.getConfig();
            p["proj"] = projui;

            return p;
        }

        // TODOREWORK
        namespace
        {
            struct ProjBounds
            {
                double min_lon;
                double max_lon;
                double min_lat;
                double max_lat;
                bool valid = false;
            };

            ProjBounds determineProjectionBounds(image::Image &img)
            {
                {
                    auto cfg = image::get_metadata_proj_cfg(img);
                    cfg["width"] = img.width();
                    cfg["height"] = img.height();
                    image::set_metadata_proj_cfg(img, cfg);
                }

                if (!image::has_metadata(img))
                    return {0, 0, 0, 0, false};

                if (!image::get_metadata(img).contains("proj_cfg"))
                    return {0, 0, 0, 0, false};

                try
                {
                    nlohmann::json params = image::get_metadata(img)["proj_cfg"];

                    ::proj::projection_t proj;
                    bool proj_err = false;
                    try
                    {
                        proj = params;
                    }
                    catch (std::exception &)
                    {
                        proj_err = true;
                    }

                    ProjBounds bounds;
                    if (!::proj::projection_setup(&proj) && !proj_err)
                    {
                        if (proj.type == ::proj::ProjType_Geos)
                        {
                            bounds.min_lat = -90;
                            bounds.max_lat = 90;
                            bounds.min_lon = proj.lam0 * RAD2DEG - 90;
                            bounds.max_lon = proj.lam0 * RAD2DEG + 90;
                        }
                        else
                        {
                            ::proj::projection_perform_inv(&proj, 0, 0, &bounds.min_lon, &bounds.max_lat);
                            ::proj::projection_perform_inv(&proj, img.width(), img.height(), &bounds.max_lon, &bounds.min_lat);
                        }
                        bounds.valid = true;
                        ::proj::projection_free(&proj);
                    }
                    else
                    {
                        auto gcps = satdump::projection::compute_gcps(params);

                        bounds.min_lon = 180;
                        bounds.max_lon = -180;
                        bounds.min_lat = 90;
                        bounds.max_lat = -90;
                        for (auto &gcp : gcps)
                        {
                            if (bounds.min_lon > gcp.lon)
                                bounds.min_lon = gcp.lon;
                            if (bounds.max_lon < gcp.lon)
                                bounds.max_lon = gcp.lon;
                            if (bounds.min_lat > gcp.lat)
                                bounds.min_lat = gcp.lat;
                            if (bounds.max_lat < gcp.lat)
                                bounds.max_lat = gcp.lat;
                        }
                        if (gcps.size() > 2)
                            bounds.valid = true;
                    }

                    bounds.min_lon -= 1;
                    bounds.max_lon += 1;
                    bounds.min_lat -= 1;
                    bounds.max_lat += 1;

                    if (bounds.min_lon < -180)
                        bounds.min_lon = -180;
                    if (bounds.max_lon > 180)
                        bounds.max_lon = 180;
                    if (bounds.min_lat < -90)
                        bounds.min_lat = -90;
                    if (bounds.max_lat > 90)
                        bounds.max_lat = 90;

                    return bounds;
                }
                catch (std::exception &e)
                {
                    logger->info("Could not get Bounds! %s", e.what());
                }

                return {0, 0, 0, 0, false};
            }

            struct Pos
            {
                double lon;
                double lat;
            };

            void computePositionCenter(std::vector<Pos> &gcps, double &lon, double &lat)
            {
                double x_total = 0;
                double y_total = 0;
                double z_total = 0;

                for (auto &pt : gcps)
                {
                    x_total += cos(pt.lat * DEG_TO_RAD) * cos(pt.lon * DEG_TO_RAD);
                    y_total += cos(pt.lat * DEG_TO_RAD) * sin(pt.lon * DEG_TO_RAD);
                    z_total += sin(pt.lat * DEG_TO_RAD);
                }

                x_total /= gcps.size();
                y_total /= gcps.size();
                z_total /= gcps.size();

                lon = atan2(y_total, x_total) * RAD_TO_DEG;
                double hyp = sqrt(x_total * x_total + y_total * y_total);
                lat = atan2(z_total, hyp) * RAD_TO_DEG;
            }

            void tryAutoTuneProjection(ProjBounds bounds, nlohmann::json &params)
            {
                ::proj::projection_t p_main = params;
                if (p_main.type == ::proj::ProjType_Equirectangular)
                {
                    params["offset_x"] = bounds.min_lon;
                    params["offset_y"] = bounds.max_lat;
                    if (!params.contains("width") || !params.contains("height"))
                    {
                        double scale_x = params.contains("scale_x") ? params["scale_x"].get<double>() : 0.016;
                        double scale_y = params.contains("scale_y") ? params["scale_y"].get<double>() : 0.016;
                        params["scalar_x"] = scale_x;
                        params["scalar_y"] = -scale_y;
                        params["width"] = (bounds.max_lon - bounds.min_lon) / scale_x;
                        params["height"] = (bounds.max_lat - bounds.min_lat) / scale_y;
                    }
                    else
                    {
                        double width = params["width"];
                        double height = params["height"];
                        params["scalar_x"] = (bounds.max_lon - bounds.min_lon) / width;
                        params["scalar_y"] = -(bounds.max_lat - bounds.min_lat) / height;
                    }
                }
                else if (p_main.type == ::proj::ProjType_Stereographic)
                {
                    std::vector<Pos> posi;
                    posi.push_back({bounds.max_lon, bounds.max_lat});
                    posi.push_back({bounds.min_lon, bounds.min_lat});
                    posi.push_back({bounds.min_lon, bounds.max_lat});
                    posi.push_back({bounds.max_lon, bounds.min_lat});
                    double center_lon = 0, center_lat = 0;
                    computePositionCenter(posi, center_lon, center_lat);

                    params["lon0"] = center_lon;
                    params["lat0"] = center_lat;
                    params["offset_x"] = 0.0;
                    params["offset_y"] = 0.0;
                    params["scalar_x"] = 1.0;
                    params["scalar_y"] = 1.0;
                    ::proj::projection_t proj = params;
                    if (!::proj::projection_setup(&proj))
                    {
                        double x1, x2;
                        double y1, y2;
                        ::proj::projection_perform_fwd(&proj, bounds.max_lon, bounds.max_lat, &x1, &y1);
                        ::proj::projection_perform_fwd(&proj, bounds.min_lon, bounds.min_lat, &x2, &y2);

                        double dist1 = sqrt(x1 * x1 + y1 * y1);
                        double dist2 = sqrt(x2 * x2 + y2 * y2);
                        double max_dist = std::max(dist1, dist2);

                        if (!params.contains("width") || !params.contains("height"))
                        {
                            double scale_x = params.contains("scale_x") ? params["scale_x"].get<double>() : 0.016;
                            double scale_y = params.contains("scale_y") ? params["scale_y"].get<double>() : 0.016;
                            params["scalar_x"] = scale_x;
                            params["scalar_y"] = -scale_y;
                            params["width"] = (max_dist * 2) / scale_x;
                            params["height"] = (max_dist * 2) / scale_y;
                        }
                        else
                        {
                            double width = params["width"];
                            double height = params["height"];
                            double scale_x = params["scalar_x"] = (max_dist * 2) / width;
                            double scale_y = params["scalar_y"] = -(max_dist * 2) / height;
                            params["offset_x"] = -width * 0.5 * scale_x;
                            params["offset_y"] = height * 0.5 * -scale_y;
                        }
                    }
                }
                /*else if (p_main.type == proj::ProjType_UniversalTransverseMercator)
                {
                    params["offset_x"] = 0.0;
                    params["offset_y"] = 0.0;
                    params["scalar_x"] = 1.0;
                    params["scalar_y"] = 1.0;
                    proj::projection_t proj = params;
                    if (!proj::projection_setup(&proj))
                    {
                        double dummy;
                        double tl_x, tl_y;
                        double br_x, br_y;
                        if (!proj::projection_perform_fwd(&proj, bounds.max_lon, bounds.max_lat, &dummy, &tl_y) &&
                            !proj::projection_perform_fwd(&proj, bounds.max_lon, bounds.min_lat, &br_x, &dummy) &&
                            !proj::projection_perform_fwd(&proj, bounds.min_lon, bounds.min_lat, &tl_x, &dummy) &&
                            !proj::projection_perform_fwd(&proj, (bounds.min_lon + bounds.max_lon) / 2.0, bounds.min_lat, &dummy, &br_y))
                        {
                            logger->trace("Final Bounds are : %f, %f - %f, %f", tl_x, tl_y, br_x, br_y);
                            params["offset_x"] = tl_x;
                            params["offset_y"] = tl_y;
                            params["scalar_x"] = -(tl_x - br_x) / vx;
                            params["scalar_y"] = -(tl_y - br_y) / vy;
                        }
                        proj::projection_free(&proj);
                    }
                }*/
            }
        } // namespace

        void ProjectionHandler::do_process()
        {
            try
            {
                image::Image img;

                // TODOREWORK
                auto proj = projui.get_proj();
                proj["width"] = projui.projections_image_width;
                proj["height"] = projui.projections_image_height;

                {
                    auto &target_cfg = proj;

                    // Automatic projection settings!
                    if (projui.projection_auto_mode && (target_cfg["type"] == "equirec" || target_cfg["type"] == "stereo"))
                    {
                        ProjBounds bounds;
                        bounds.min_lon = 180;
                        bounds.max_lon = -180;
                        bounds.min_lat = 90;
                        bounds.max_lat = -90;

                        for (int i = subhandlers.size() - 1; i >= 0; i--)
                        {
                            auto &h = subhandlers[i];
                            if (h->getID() == "image_handler")
                            {
                                ImageHandler *im_h = (ImageHandler *)h.get();

                                auto boundshere = determineProjectionBounds(im_h->getImage());
                                if (boundshere.valid)
                                {
                                    if (boundshere.min_lon < bounds.min_lon)
                                        bounds.min_lon = boundshere.min_lon;
                                    if (boundshere.max_lon > bounds.max_lon)
                                        bounds.max_lon = boundshere.max_lon;
                                    if (boundshere.min_lat < bounds.min_lat)
                                        bounds.min_lat = boundshere.min_lat;
                                    if (boundshere.max_lat > bounds.max_lat)
                                        bounds.max_lat = boundshere.max_lat;
                                }
                            }
                        }

                        logger->trace("Final Bounds are : %f, %f - %f, %f", bounds.min_lon, bounds.min_lat, bounds.max_lon, bounds.max_lat);

                        if (projui.projection_auto_scale_mode)
                        {
                            if (target_cfg.contains("width"))
                                target_cfg.erase("width");
                            if (target_cfg.contains("height"))
                                target_cfg.erase("height");
                        }
                        else
                        {
                            if (target_cfg.contains("scale_x"))
                                target_cfg.erase("scale_x");
                            if (target_cfg.contains("scale_y"))
                                target_cfg.erase("scale_y");
                        }

                        tryAutoTuneProjection(bounds, target_cfg);
                        if (target_cfg.contains("width"))
                            projui.projections_image_width = target_cfg["width"];
                        if (target_cfg.contains("height"))
                            projui.projections_image_height = target_cfg["height"];

                        logger->debug("\n%s", target_cfg.dump(4).c_str());
                    }
                }

                // TODOREWORK

                std::vector<image::Image> all_imgs;
                for (int i = subhandlers.size() - 1; i >= 0; i--)
                {
                    auto &h = subhandlers[i];
                    if (h->getID() == "image_handler")
                    {
                        ImageHandler *im_h = (ImageHandler *)h.get();
                        logger->critical("Drawing IMAGE!");
                        //                    sh_h->draw_to_image(curr_image, pfunc);

                        // TODOREWORK!!!!
                        logger->trace("Proj : \n%s\n", proj.dump(4).c_str());
                        auto im = projection::reprojectImage(im_h->getImage(), proj);
                        all_imgs.push_back(im);
                        logger->critical("DONE REPROJECTING!");
                    }
                }

                if (all_imgs.size() > 0)
                {
                    img.init(all_imgs[0].depth(), all_imgs[0].width(), all_imgs[0].height(), 3);
                    image::set_metadata_proj_cfg(img, image::get_metadata_proj_cfg(all_imgs[0]));
                }

                for (int i = 0; i < all_imgs.size(); i++)
                {
                    logger->trace("Draw %d", i);
                    img.draw_image_alpha(all_imgs[i]);
                }

                for (int i = subhandlers.size() - 1; i >= 0; i--)
                {
                    auto &h = subhandlers[i];
                    if (h->getID() == "shapefile_handler")
                    {
                        ShapefileHandler *sh_h = (ShapefileHandler *)h.get();
                        logger->critical("Drawing OVERLAY!");

                        nlohmann::json cfg = image::get_metadata_proj_cfg(img);
                        cfg["width"] = img.width();
                        cfg["height"] = img.height();
                        std::unique_ptr<projection::Projection> p = std::make_unique<projection::Projection>();
                        *p = cfg;
                        p->init(1, 0);

                        auto pfunc = [&p](double lat, double lon, double h, double w) mutable -> std::pair<double, double>
                        {
                            double x, y;
                            if (p->forward(geodetic::geodetic_coords_t(lat, lon, 0, false), x, y) || x < 0 || x >= w || y < 0 || y >= h)
                                return {-1, -1};
                            else
                                return {x, y};
                        };
                        sh_h->draw_to_image(img, pfunc);
                    }
                }

                img_handler.setImage(img);
            }
            catch (std::exception &e)
            {
                logger->error("Error projecting %s", e.what());
            }
        }

        void ProjectionHandler::drawMenuBar()
        {
            img_handler.drawSaveMenu(); // TODOREWORK remove this
                                        /*if (ImGui::MenuItem("Image To Handler"))
                                        {
                                            std::shared_ptr<ImageHandler> a = std::make_shared<ImageHandler>();
                                            a->setConfig(img_handler.getConfig());
                                            a->updateImage(img_handler.image);
                                            addSubHandler(a);
                                        }*/
                                        // TODOREWORK

            renderVectorOverlayMenu(this);
        }

        void ProjectionHandler::drawContents(ImVec2 win_size) { img_handler.drawContents(win_size); }
    } // namespace handlers
} // namespace satdump
