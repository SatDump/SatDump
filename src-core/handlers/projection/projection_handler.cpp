#include "projection_handler.h"
#include "common/image/image.h"
#include "common/image/meta.h"
#include "core/style.h"
#include "logger.h"

#include "../image/image_handler.h"
#include "../vector/shapefile_handler.h"

// TODOREWORK
#include "projection/reproject_img.h"
#include "projection/reprojector.h"

namespace satdump
{
    namespace handlers
    {
        ProjectionHandler::ProjectionHandler()
        {
            handler_tree_icon = u8"\uf6e6";
            setCanSubBeReorgTo(true);
        }

        ProjectionHandler::~ProjectionHandler() {}

        void ProjectionHandler::drawMenu()
        {
            bool needs_to_be_disabled = is_processing;

            if (ImGui::CollapsingHeader("Projection"))
            {
                if (needs_to_be_disabled)
                    style::beginDisabled();

                projui.drawUI();

                //            needs_to_update |= TODO; // TODOREWORK move in top drawMenu?
                if (ImGui::Button("TEST"))
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
        }

        nlohmann::json ProjectionHandler::getConfig()
        {
            nlohmann::json p;

            p["image"] = img_handler.getConfig();

            return p;
        }

        void ProjectionHandler::do_process()
        {
            image::Image img;

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
                    auto im = proj::reprojectImage(im_h->getImage(), projui.get_proj());
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
                    std::unique_ptr<proj::Projection> p = std::make_unique<proj::Projection>();
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

        void ProjectionHandler::drawMenuBar()
        {
            img_handler.drawMenuBar(); // TODOREWORK remove this
            /*if (ImGui::MenuItem("Image To Handler"))
            {
                std::shared_ptr<ImageHandler> a = std::make_shared<ImageHandler>();
                a->setConfig(img_handler.getConfig());
                a->updateImage(img_handler.image);
                addSubHandler(a);
            }*/
            // TODOREWORK
        }

        void ProjectionHandler::drawContents(ImVec2 win_size) { img_handler.drawContents(win_size); }
    } // namespace handlers
} // namespace satdump
