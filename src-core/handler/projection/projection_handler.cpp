#include "projection_handler.h"
#include "core/style.h"
#include "logger.h"

#include "../image/image_handler.h"
#include "../vector/shapefile_handler.h"

// TODOREWORK
#include "projection/reprojector.h"

namespace satdump
{
    namespace viewer
    {
        ProjectionHandler::ProjectionHandler() { handler_tree_icon = u8"\uf6e6"; }

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
            for (auto &h : subhandlers)
            {
                if (h->getID() == "image_handler")
                {
                    ImageHandler *im_h = (ImageHandler *)h.get();
                    logger->critical("Drawing IMAGE!");
                    //                    sh_h->draw_to_image(curr_image, pfunc);

                    // TODOREWORK!!!!
                    proj::ReprojectionOperation op;
                    op.img = &im_h->get_current_img();
                    op.target_prj_info = projui.get_proj();
                    // op.target_prj_info["width"] = width;
                    // op.target_prj_info["height"] = height;
                    op.output_width = op.target_prj_info["width"];
                    op.output_height = op.target_prj_info["height"];
                    auto cfg = image::get_metadata_proj_cfg(*op.img);
                    cfg["width"] = op.img->width();
                    cfg["height"] = op.img->height();
                    image::set_metadata_proj_cfg(*op.img, cfg);
                    auto img = proj::reproject(op);
                    logger->critical("DONE REPROJECTING!");
                    img_handler.updateImage(img);
                }
            }

            /* for (auto &h : subhandlers)
             {
                 if (h->getID() == "shapefile_handler")
                 {
                     ShapefileHandler *sh_h = (ShapefileHandler *)h.get();
                     logger->critical("Drawing OVERLAY!");
                     //                    sh_h->draw_to_image(curr_image, pfunc);
                 }
             }*/
        }

        void ProjectionHandler::drawMenuBar()
        {
            img_handler.drawMenuBar();
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
    } // namespace viewer
} // namespace satdump
