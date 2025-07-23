#include "image_handler.h"

#include "core/plugin.h"
#include "core/style.h"
#include "handlers/projection/projection_handler.h"
#include "handlers/vector/addmenu.h"
#include "image/hue_saturation.h"
#include "image/image_background.h"
#include "image/meta.h"
#include "imgui/imgui.h"
#include "imgui/imgui_stdlib.h"
#include "logger.h"

#include "image/brightness_contrast.h"
#include "image/earth_curvature.h"
#include "image/io.h" // TODOREWORK
#include "image/processing.h"

#include "nlohmann/json_utils.h"

#include "../vector/shapefile_handler.h"

#include "core/config.h"
#include "imgui/dialogs/pfd_utils.h" // TODOREWORK
#include "utils/string.h"

// TODOREWORK!
#include "handlers/vector/shapefile_handler.h"
#include "products/image/channel_transform.h"
#include <memory>
#include <utility>

// TODOREWORK
#include "../../../src-interface/explorer/explorer.h"

namespace satdump
{
    namespace handlers
    {
        ImageHandler::ImageHandler()
        {
            handler_tree_icon = u8"\uf7e8";
            setCanSubBeReorgTo(true);

            // TODOREWORK experimental?
            image_view.cropCallback = [this](int x1, int y1, int x2, int y2)
            {
                if (is_processing)
                {
                    logger->error("Cannot crop while processing!"); // TODOREWORK see when adding other functions - maybe a global image lock?
                    return;
                }

                if (x2 < x1)
                    std::swap(x1, x2);
                if (y2 < y1)
                    std::swap(y1, y2);

                logger->critical("CROPPING %d %d, %d %d, %d %d", x1, y1, x2, y2, getImage().width(), getImage().height());

                auto img = getImage().crop_to(x1, y1, x2, y2);
                auto proj_cfg = image::get_metadata_proj_cfg(getImage());
                if (proj_cfg.contains("transform2"))
                {
                    x1 += proj_cfg["transform2"]["bx"].get<double>();
                    y1 += proj_cfg["transform2"]["by"].get<double>();
                    proj_cfg["transform2"]["bx"] = x1;
                    proj_cfg["transform2"]["by"] = y1;
                }
                else
                {
                    proj_cfg["width"] = img.width();
                    proj_cfg["height"] = img.height();
                    proj_cfg["transform2"] = ChannelTransform().init_affine(1, 1, x1, y1);
                }
                image::set_metadata_proj_cfg(img, proj_cfg);
                geocorrect_image = false;
                setImage(img);
            };
        }

        ImageHandler::ImageHandler(image::Image img) : ImageHandler::ImageHandler() { setImage(img); }

        ImageHandler::ImageHandler(image::Image img, std::string name) : ImageHandler::ImageHandler(img) { image_name = name; }

        ImageHandler::~ImageHandler()
        {
            if (file_save_thread.joinable())
                file_save_thread.join();
        }

        void ImageHandler::drawMenu()
        {
            bool needs_to_be_disabled = is_processing;

            if (ImGui::CollapsingHeader("Image"))
            {
                bool needs_to_update = false;

                if (needs_to_be_disabled)
                    style::beginDisabled();

                needs_to_update |= ImGui::Checkbox("Median Blur", &median_blur_img);
                needs_to_update |= ImGui::Checkbox("Despeckle", &despeckle_img);
                needs_to_update |= ImGui::Checkbox("Rotate 180", &rotate180_image);
                needs_to_update |= ImGui::Checkbox("Geo Correct", &geocorrect_image); // TODOREWORK Disable if it can't be?
                needs_to_update |= ImGui::Checkbox("Equalize", &equalize_img);
                needs_to_update |= ImGui::Checkbox("Individual Equalize", &equalize_perchannel_img);
                needs_to_update |= ImGui::Checkbox("White Balance", &white_balance_img);
                needs_to_update |= ImGui::Checkbox("Normalize", &normalize_img);
                needs_to_update |= ImGui::Checkbox("Invert", &invert_img);

                needs_to_update |= ImGui::Checkbox("Brightness/contrast", &brightness_contrast_image);
                if (brightness_contrast_image)
                {
                    ImGui::SliderFloat("Brightness", &brightness_contrast_brightness_image, -2, 2);
                    needs_to_update |= ImGui::IsItemDeactivatedAfterEdit();
                    ImGui::SliderFloat("Contrast", &brightness_contrast_contrast_image, -2, 2);
                    needs_to_update |= ImGui::IsItemDeactivatedAfterEdit();
                }

                needs_to_update |= ImGui::Checkbox("Hue/Saturation", &huesaturation_img);
                if (huesaturation_img)
                {
                    for (int i = 0; i < 7; i++)
                    {
                        std::string cname[] = {"All", "Red", "Yellow", "Green", "Cyan", "Blue", "Magenta"};
                        float hue = huesaturation_cfg_img.hue[i] * 180.0;
                        float saturation = huesaturation_cfg_img.saturation[i] * 100.0;
                        float lightness = huesaturation_cfg_img.lightness[i] * 100.0;
                        ImGui::SliderFloat(std::string(cname[i] + " Hue").c_str(), &hue, -180, 180);
                        needs_to_update |= ImGui::IsItemDeactivatedAfterEdit();
                        ImGui::SliderFloat(std::string(cname[i] + " Saturation").c_str(), &saturation, -100, 100);
                        needs_to_update |= ImGui::IsItemDeactivatedAfterEdit();
                        ImGui::SliderFloat(std::string(cname[i] + " Lightness").c_str(), &lightness, -100, 100);
                        needs_to_update |= ImGui::IsItemDeactivatedAfterEdit();
                        huesaturation_cfg_img.hue[i] = hue / 180.0;
                        huesaturation_cfg_img.saturation[i] = saturation / 100.0;
                        huesaturation_cfg_img.lightness[i] = lightness / 100.0;
                    }

                    float overlap = huesaturation_cfg_img.overlap;
                    ImGui::SliderFloat("Overlap", &overlap, -100.0, 100.0);
                    needs_to_update |= ImGui::IsItemDeactivatedAfterEdit();
                    huesaturation_cfg_img.overlap = overlap;
                }

                needs_to_update |= ImGui::Checkbox("Remove Background", &remove_background_img);

                if (needs_to_be_disabled)
                    style::endDisabled();

                if (image_calib_valid)
                {
                    ImGui::Text("Calibration Unit %s", image_calib.unit.c_str());
                    ImGui::Text("Calibration Min %f", image_calib.min);
                    ImGui::Text("Calibration Max %f", image_calib.max);
                }

                if (needs_to_update)
                    asyncProcess();
                wasMenuTriggered = needs_to_update;
            }
        }

        void ImageHandler::drawSaveMenu()
        {
            bool needs_to_be_disabled = is_processing || file_save_thread_running;

            if (needs_to_be_disabled)
                style::beginDisabled();

            if (ImGui::MenuItem("Save Image"))
            {
                auto fun = [this]()
                {
                    file_save_thread_running = true;
                    // TODOREWORK!!!!
                    std::string save_type = "png";
                    satdump_cfg.tryAssignValueFromSatDumpGeneral(save_type, "image_format");
                    std::string default_path = satdump_cfg.getValueFromSatDumpDirectories<std::string>("default_image_output_directory");
                    std::string saved_at = save_image_dialog(getSaneName(), default_path, "Save Image", &getImage(), &save_type);
                    if (saved_at == "")
                        logger->info("Save cancelled");
                    else
                        logger->info("Saved current image at %s", saved_at.c_str());
                    file_save_thread_running = false;
                };

                if (file_save_thread.joinable())
                    file_save_thread.join();
                file_save_thread = std::thread(fun);
            }

            if (needs_to_be_disabled)
                style::endDisabled();
        }

        void ImageHandler::drawMenuBar()
        {
            drawSaveMenu();

            renderVectorOverlayMenu(this);
        }

        void ImageHandler::drawContents(ImVec2 win_size)
        {
            ImGui::BeginChild("ContentChild", win_size, false, ImGuiWindowFlags_MenuBar);

            if (imgview_needs_update)
            {
                image_view.update(getImage());
                imgview_needs_update = false;

                image_view.mouseCallback = [this](float x, float y)
                {
                    auto &img = getImage();
                    ImGui::BeginTooltip();
                    ImGui::Text("Raw : %d", img.get(0, x, y));
                    if (image_calib_valid && image.channels() == 1)
                    {
                        int xc = x; // Correction only needs to be undone for calib
                        if (correct_fwd_lut.size() > 0 && x > 0 && x < correct_fwd_lut.size())
                            xc = correct_fwd_lut[x];

                        double val = image_calib.getVal(img.getf(0, xc, y));
                        ImGui::Text("Unit : %f %s", val, image_calib.unit.c_str());
                    }
                    if (image_proj_valid)
                    {
                        geodetic::geodetic_coords_t pos;
                        if (image_proj.inverse(x, y, pos))
                        {
                            ImGui::Text("Lat : Invalid!");
                            ImGui::Text("Lon : Invalid!");
                        }
                        else
                        {
                            ImGui::Text("Lat : %f", pos.lat);
                            ImGui::Text("Lon : %f", pos.lon);
                        }
                    }
                    additionalMouseCallback(x, y);
                    ImGui::EndTooltip();
                };
            }

            if (ImGui::BeginMenuBar())
            {
                image_view.zoom_in_next |= ImGui::MenuItem(u8"\ueb81");
                image_view.zoom_out_next |= ImGui::MenuItem(u8"\ueb82");
                image_view.autoFitNextFrame |= ImGui::MenuItem("Fit");
                image_view.select_crop_next |= ImGui::MenuItem("Crop");

                if (image_proj_valid)
                {
                    if (ImGui::MenuItem("Add To Proj"))
                    { // TODOREWORK
                        std::shared_ptr<Handler> h;
                        eventBus->fire_event<explorer::GetLastSelectedOfTypeEvent>({"projection_handler", h});
                        if (h)
                            h->addSubHandler(std::make_shared<ImageHandler>(getImage(), getName()), true);
                        else
                        {
                            auto p = std::make_shared<ProjectionHandler>();
                            p->addSubHandler(std::make_shared<ImageHandler>(getImage(), getName()), true);
                            eventBus->fire_event<explorer::ExplorerAddHandlerEvent>({p});
                        }
                    }
                }

                ImGui::EndMenuBar();
            }

            image_view.draw(ImGui::GetContentRegionAvail());

            ImGui::EndChild();
        }

        void ImageHandler::setConfig(nlohmann::json p)
        {
            equalize_img = getValueOrDefault(p["equalize"], false);
            equalize_perchannel_img = getValueOrDefault(p["individual_equalize"], false);
            white_balance_img = getValueOrDefault(p["white_balance"], false);
            normalize_img = getValueOrDefault(p["normalize"], false);
            invert_img = getValueOrDefault(p["invert"], false);
            median_blur_img = getValueOrDefault(p["median_blur"], median_blur_img);
            despeckle_img = getValueOrDefault(p["despeckle"], despeckle_img);
            rotate180_image = getValueOrDefault(p["rotate180"], rotate180_image);
            geocorrect_image = getValueOrDefault(p["geocorrect"], geocorrect_image);
            brightness_contrast_image = getValueOrDefault(p["brightness_contrast"], false);
            brightness_contrast_brightness_image = getValueOrDefault(p["brightness_contrast_brightness"], 0);
            brightness_contrast_contrast_image = getValueOrDefault(p["brightness_contrast_contrast"], 0);
            huesaturation_img = getValueOrDefault(p["hue_saturation"], false);
            if (p.contains("hue_saturation_cfg"))
                huesaturation_cfg_img = p["hue_saturation_cfg"];
            remove_background_img = getValueOrDefault(p["remove_background"], false);
        }

        nlohmann::json ImageHandler::getConfig()
        {
            nlohmann::json p;
            p["equalize"] = equalize_img;
            p["individual_equalize"] = equalize_perchannel_img;
            p["white_balance"] = white_balance_img;
            p["normalize"] = normalize_img;
            p["invert"] = invert_img;
            p["median_blur"] = median_blur_img;
            p["despeckle"] = despeckle_img;
            p["rotate180"] = rotate180_image;
            p["geocorrect"] = geocorrect_image;
            p["brightness_contrast"] = brightness_contrast_image;
            p["brightness_contrast_brightness"] = brightness_contrast_brightness_image;
            p["brightness_contrast_contrast"] = brightness_contrast_contrast_image;
            p["hue_saturation"] = huesaturation_img;
            p["hue_saturation_cfg"] = huesaturation_cfg_img;
            p["remove_background"] = remove_background_img;
            return p;
        }

        void ImageHandler::setImage(image::Image &img) // TODOREWORK
        {
            image::set_metadata(image, {});
            image = img;
            process();
        }

        std::string ImageHandler::getSaneName()
        {
            std::string img_name = image_name;
            replaceAllStr(img_name, " ", "_");
            replaceAllStr(img_name, "/", "_");
            replaceAllStr(img_name, "\\", "_");
            return img_name;
        }

        // TODOREWORK?
        void ImageHandler::saveResult(std::string directory) { image::save_img_safe(getImage(), directory + "/" + getSaneName()); }

        void ImageHandler::do_process()
        {
            bool image_needs_processing = huesaturation_img | equalize_img | equalize_perchannel_img | white_balance_img | normalize_img | invert_img | median_blur_img | rotate180_image |
                                          geocorrect_image | despeckle_img | brightness_contrast_image | remove_background_img /*OVERLAY*/;

            correct_fwd_lut.clear();
            correct_rev_lut.clear();

            if (image_needs_processing)
            {
                curr_image = image;

                if (huesaturation_img)
                    image::hue_saturation(curr_image, huesaturation_cfg_img);
                if (equalize_img)
                    image::equalize(curr_image, false);
                if (equalize_perchannel_img)
                    image::equalize(curr_image, true);
                if (white_balance_img)
                    image::white_balance(curr_image);
                if (normalize_img)
                    image::normalize(curr_image);
                if (invert_img)
                    image::linear_invert(curr_image);
                if (median_blur_img)
                    image::median_blur(curr_image);
                if (despeckle_img)
                    image::kuwahara_filter(curr_image);
                if (brightness_contrast_image)
                    image::brightness_contrast(curr_image, brightness_contrast_brightness_image, brightness_contrast_contrast_image);
                if (remove_background_img)
                    image::remove_background(curr_image, nullptr); // TODOREWORK progress?
                if (rotate180_image)
                    curr_image.mirror(true, true);

                if (geocorrect_image)
                { // TODOREWORK handle disabling projs, etc
                    bool success = false;
                    curr_image = image::earth_curvature::perform_geometric_correction(curr_image, success, &correct_rev_lut, &correct_fwd_lut);
                    if (!success)
                    {
                        logger->error("Failed Geo-Correcting image!");
                        correct_fwd_lut.clear();
                        correct_rev_lut.clear();
                    }
                }
            }
            else
                curr_image.clear();

            ////////////////////////
            subhandlers_mtx.lock();
            bool image_has_overlays = false;

            for (auto &h : subhandlers)
                if (h->getID() == "shapefile_handler")
                    image_has_overlays = true;

            if (image_has_overlays)
            {
                if (curr_image.size() == 0)
                    curr_image = image;

                nlohmann::json cfg = image::get_metadata_proj_cfg(curr_image);
                cfg["width"] = curr_image.width();
                cfg["height"] = curr_image.height();
                std::unique_ptr<projection::Projection> p = std::make_unique<projection::Projection>();
                *p = cfg;
                p->init(1, 0);

                double rotate180 = rotate180_image;
                auto pfunc = [&p, rotate180](double lat, double lon, double h, double w) mutable -> std::pair<double, double>
                {
                    double x, y;
                    if (p->forward(geodetic::geodetic_coords_t(lat, lon, 0, false), x, y) || x < 0 || x >= w || y < 0 || y >= h)
                        return {-1, -1};
                    else
                    {
                        if (rotate180)
                            return {w - 1 - x, h - 1 - y};
                        else
                            return {x, y};
                    }
                };

                for (int i = subhandlers.size() - 1; i >= 0; i--)
                {
                    auto &h = subhandlers[i];
                    if (h->getID() == "shapefile_handler")
                    {
                        ShapefileHandler *sh_h = (ShapefileHandler *)h.get();
                        logger->critical("Drawing OVERLAY!");
                        sh_h->draw_to_image(curr_image, pfunc);
                    }
                }
            }

            subhandlers_mtx.unlock();
            ////////////////////////

            // Update ImgView
            has_second_image = image_needs_processing | image_has_overlays;
            imgview_needs_update = true;

            image_proj_valid = false;
            if (image::has_metadata_proj_cfg(image))
            {
                image_proj = image::get_metadata_proj_cfg(image);
                if (image_proj.init(0, 1))
                    image_proj_valid = true;
            }

            image_calib_valid = false;
            if (image::has_metadata_calib_cfg(image))
            {
                image_calib = image::get_metadata_calib_cfg(image);
                image_calib_valid = true;
            }
        }
    } // namespace handlers
} // namespace satdump
