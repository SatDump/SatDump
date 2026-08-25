#include "image_handler.h"
#include "../vector/shapefile_handler.h"
#include "common/widgets/menuitem_tooltip.h"
#include "core/config.h"
#include "core/plugin.h"
#include "core/style.h"
#include "explorer/explorer.h"
#include "handlers/image/image_filter.h"
#include "handlers/projection/projection_handler.h"
#include "handlers/vector/addmenu.h"
#include "handlers/vector/shapefile_handler.h"
#include "i18n.h"
#include "image/brightness_contrast.h"
#include "image/earth_curvature.h"
#include "image/hue_saturation.h"
#include "image/image_background.h"
#include "image/io.h"
#include "image/meta.h"
#include "image/processing.h"
#include "imgui/imgui.h"
#include "logger.h"
#include "nlohmann/json_utils.h"
#include "products/image/channel_transform.h"
#include "utils/string.h"
#include <exception>
#include <memory>
#include <string>
#include <utility>

namespace satdump
{
    namespace handlers
    {
        ImageHandler::ImageHandler()
        {
            handler_tree_icon = u8"\uf7e8";
            setCanSubBeReorgTo(true);

            // Image crop feature
            image_view.cropCallback = [this](int x1, int y1, int x2, int y2)
            {
                if (is_processing)
                {
                    logger->error(_("Cannot crop while processing!")); // TODOREWORK see when adding other functions - maybe a global image lock?
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

                auto sh = std::make_shared<ImageHandler>(img);
                sh->image_name = image_name + _(" Crop");

                if (removeProjectionInfoFromCrop)
                    image::set_metadata_proj_cfg(img, {});

                if (sendCropToRoot)
                {
                    eventBus->fire_event<explorer::ExplorerAddHandlerEvent>({sh, true});
                }
                else
                {
                    addSubHandler(sh);
                    eventBus->fire_event<explorer::ExplorerSelectHandlerEvent>({sh});
                }
            };

            // Load image filters
            image_filters = getImageFilters();
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

            if (ImGui::CollapsingHeader(_("Image")))
            {
                bool needs_to_update = false;

                if (needs_to_be_disabled)
                    style::beginDisabled();

                if (ImGui::RadioButton(_("Rotate 0"), rotate_image == 0))
                    needs_to_update = 1, rotate_image = 0;
                if (ImGui::RadioButton(_("Rotate 90"), rotate_image == 90))
                    needs_to_update = 1, rotate_image = 90;
                if (ImGui::RadioButton(_("Rotate 180"), rotate_image == 180))
                    needs_to_update = 1, rotate_image = 180;
                if (ImGui::RadioButton(_("Rotate 270"), rotate_image == 270))
                    needs_to_update = 1, rotate_image = 270;

                if (image_proj_valid)
                    needs_to_update |= ImGui::Checkbox(_("Geo Correct"), &geocorrect_image); // TODOREWORK Disable if it can't be?

                if (needs_to_be_disabled)
                    style::endDisabled();

                if (image_calib_valid)
                {
                    ImGui::Text(_("Calibration Unit %s"), image_calib.unit.c_str());
                    ImGui::Text(_("Calibration Min %f"), image_calib.min);
                    ImGui::Text(_("Calibration Max %f"), image_calib.max);
                }

                if (needs_to_update)
                {
                    if (file_save_thread_running)
                        logger->error(_("Please wait for saving to end first!"));
                    else
                        asyncProcess();
                }
                wasMenuTriggered = needs_to_update;
            }

            if (ImGui::CollapsingHeader(_("Filters")))
            {
                if (ImGui::BeginListBox("##filterscombo", {ImGui::GetContentRegionAvail().x, 0}))
                {
                    bool quit = false;

                    for (int i = 0; i < active_filters.size(); i++)
                    {
                        if (quit)
                            break;

                        auto &f = active_filters[i];

                        std::string name = image_filters[f.first].name;

                        ImGui::PushID(i);
                        ImGui::BeginGroup();

                        // Settings
                        if (ImGui::Button("\uF085"))
                        {
                            image_filter_configurator = image_filters[f.first].configMenuGetter();
                            if (image_filter_configurator)
                            {
                                image_filter_configurator->set(f.second);
                                image_filter_configurator_set_in = i;
                            }
                        }

                        if (f.second.size() && ImGui::IsItemHovered())
                            ImGui::SetTooltip("%s", f.second.dump(4).c_str());

                        ImGui::SameLine();

                        // Delete
                        if (ImGui::Button("\uF1F8") && i < active_filters.size())
                        {
                            active_filters.erase(active_filters.begin() + selected_filter);
                            quit = true;
                            asyncProcess();
                        }

                        ImGui::SameLine();

                        // Up
                        if (ImGui::Button("\uF062") && i > 0)
                        {
                            std::swap(active_filters[i], active_filters[i - 1]);
                            asyncProcess();
                        }

                        ImGui::SameLine();

                        // Down
                        if (ImGui::Button("\uF063") && i + 1 < active_filters.size())
                        {
                            std::swap(active_filters[i], active_filters[i + 1]);
                            asyncProcess();
                        }

                        ImGui::SameLine();

                        ImGui::Text("%s", name.c_str());

                        ImGui::EndGroup();
                        ImGui::PopID();
                        ImGui::Separator();
                    }
                    ImGui::EndListBox();
                }
            }
        }

        void ImageHandler::drawSaveMenu()
        {
            bool needs_to_be_disabled = is_processing || file_save_thread_running;

            if (needs_to_be_disabled)
                style::beginDisabled();

            if (widgets::MenuItemTooltip(u8"\ueb4b", _("Save Image")))
            {
                auto fun = [this]()
                {
                    set_is_processing(true);
                    file_save_thread_running = true;
                    // TODOREWORK!!!!
                    std::string save_type = "png";
                    satdump_cfg.tryAssignValueFromSatDumpGeneral(save_type, "image_format");
                    std::string default_path = satdump_cfg.getValueFromSatDumpDirectories<std::string>("default_image_output_directory");
                    std::string saved_at = save_image_dialog(getSaneName(), default_path, _("Save Image"), &getImage(), &save_type);
                    if (saved_at == "")
                        logger->info(_("Save cancelled"));
                    else
                        logger->info(_("Saved current image at %s"), saved_at.c_str());
                    file_save_thread_running = false;
                    set_is_processing(false);
                };

                if (file_save_thread.joinable())
                    file_save_thread.join();
                if (file_save_thread_running)
                    logger->error(_("Please wait for processing to end first!"));
                else
                    file_save_thread = std::thread(fun);
            }

            if (needs_to_be_disabled)
                style::endDisabled();
        }

        void ImageHandler::drawMenuBar()
        {
            drawSaveMenu();

            if (enableOverlayMenu && renderVectorOverlayMenu(this))
                asyncProcess();

            /////////////
            // Image Controls
            /////////////

            // Refresh button
            if (widgets::MenuItemTooltip(u8"\uf01e", _("Refresh (Image Only)")))
                asyncProcess();

            // Basic controls
            image_view.zoom_in_next |= widgets::MenuItemTooltip(u8"\ueb81", _("Zoom In"));
            image_view.zoom_out_next |= widgets::MenuItemTooltip(u8"\ueb82", _("Zoom Out"));
            image_view.autoFitNextFrame |= widgets::MenuItemTooltip(u8"\uF69E", _("Fit"));
            image_view.select_crop_next |= widgets::MenuItemTooltip(u8"\uF69D", _("Crop"), NULL, image_view.select_crop_next);

            if (image_proj_valid)
            {
                if (rotate_image) // Projs do not work with rotated imagery
                    style::beginDisabled();

                // Show a menu that allows putting this image on an existing or new projection
                if (widgets::BeginMenuTooltip(u8"\uf484", _("Add to projection")))
                {
                    std::vector<std::shared_ptr<Handler>> hs;
                    eventBus->fire_event<explorer::GetAllOfTypeEvent>({"projection_handler", hs});

                    int n = 0;
                    for (auto &h : hs)
                    {
                        std::string id = h->getName() + " (" + std::to_string(++n) + ")" + "##addtoproj";
                        if (ImGui::MenuItem(id.c_str()))
                            h->addSubHandler(std::make_shared<ImageHandler>(getImage(), getName()), true);
                    }

                    if (n > 0)
                        ImGui::Separator();

                    if (ImGui::MenuItem(_("New Projection")))
                    {
                        auto p = std::make_shared<ProjectionHandler>();
                        p->addSubHandler(std::make_shared<ImageHandler>(getImage(), getName()), true);
                        eventBus->fire_event<explorer::ExplorerAddHandlerEvent>({p});
                    }

                    ImGui::EndMenu();
                }

                if (rotate_image)
                    style::endDisabled();
            }

            // Render filters menu
            if (ImGui::BeginMenu(_("\uF0C3")))
            {
                for (auto &filter : image_filters)
                {
                    if (ImGui::MenuItem(filter.second.name.c_str()))
                    {
                        auto menu = filter.second.configMenuGetter();

                        if (menu)
                        {
                            menu->type = filter.first;
                            image_filter_configurator_set_in = -1;
                            image_filter_configurator = menu;
                        }
                        else
                        {
                            active_filters.push_back({filter.first, {}});
                            asyncProcess();
                        }
                    }
                }

                ImGui::EndMenu();
            }
        }

        void ImageHandler::drawContents(ImVec2 win_size)
        {
            if (ImGui::BeginChild("ContentChild", win_size, false, ImGuiWindowFlags_NoScrollbar))
            {
                if (imgview_needs_update)
                {
                    image_view.update(getImage());
                    imgview_needs_update = false;

                    image_view.mouseCallback = [this](float x, float y)
                    {
                        auto &img = getImage();
                        ImGui::BeginTooltip();

                        for (int i = 0; i < img.channels(); i++)
                            ImGui::Text(_("Raw %d : %d F %f"), i + 1, img.get(0, x, y), img.getf(0, x, y));

                        if (image_calib_valid && image.channels() == 1)
                        {
                            int xc = x; // Correction only needs to be undone for calib
                            if (correct_fwd_lut.size() > 0 && x > 0 && x < correct_fwd_lut.size())
                                xc = correct_fwd_lut[x];

                            double val = image_calib.getVal(img.getf(0, xc, y));
                            ImGui::Text(_("Unit : %f %s"), val, image_calib.unit.c_str());
                        }

                        // Handle rotations
                        if (rotate_image)
                        {
                            if (rotate_image == 180)
                            {
                                x = (img.width() - 1) - x;
                                y = (img.height() - 1) - y;
                            }
                            else if (rotate_image == 90)
                            {
                                auto x1 = y;
                                y = (img.width() - 1) - x;
                                x = x1;
                            }
                            else if (rotate_image == 270)
                            {
                                auto x1 = (img.height() - 1) - y;
                                y = x;
                                x = x1;
                            }
                        }

                        if (image_proj_valid)
                        {
                            geodetic::geodetic_coords_t pos;
                            if (image_proj.inverse(x, y, pos))
                            {
                                ImGui::Text(_("Lat : Invalid!"));
                                ImGui::Text(_("Lon : Invalid!"));
                            }
                            else
                            {
                                ImGui::Text(_("Lat : %f"), pos.lat);
                                ImGui::Text(_("Lon : %f"), pos.lon);
                            }
                        }
                        additionalMouseCallback(x, y);
                        ImGui::EndTooltip();
                    };
                }

                image_view.draw({ImGui::GetWindowSize().x, ImGui::GetWindowSize().y + 14 * ui_scale});

                ImGui::EndChild();
            }

            // Render image filter config
            if (image_filter_configurator)
            {
                ImGui::OpenPopup(_("Filter Config"));
                if (ImGui::BeginPopupModal(_("Filter Config")))
                {
                    image_filter_configurator->draw();

                    if (ImGui::Button(_("Apply")))
                    {
                        if (image_filter_configurator_set_in == -1)
                            active_filters.push_back({image_filter_configurator->type, image_filter_configurator->get()});
                        else if (image_filter_configurator_set_in < active_filters.size())
                            active_filters[image_filter_configurator_set_in].second = image_filter_configurator->get();
                        image_filter_configurator.reset();
                        asyncProcess();
                    }
                    ImGui::SameLine();
                    if (ImGui::Button(_("Cancel")))
                        image_filter_configurator.reset();

                    ImGui::EndPopup();
                }
            }
        }

        void ImageHandler::setConfig(nlohmann::json p)
        {
            rotate_image = getValueOrDefault(p["rotate180"], rotate_image);
            geocorrect_image = getValueOrDefault(p["geocorrect"], geocorrect_image);
            active_filters = p["filters"];
        }

        nlohmann::json ImageHandler::getConfig()
        {
            nlohmann::json p;
            p["rotate"] = rotate_image;
            p["geocorrect"] = geocorrect_image;
            p["filters"] = active_filters;
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
        bool ImageHandler::saveResult(std::string directory)
        {
            image::save_img_safe(getImage(), directory + "/" + getSaneName());
            return getImage().size();
        }

        void ImageHandler::do_process()
        {
            bool image_needs_processing = rotate_image | geocorrect_image | active_filters.size();

            correct_fwd_lut.clear();
            correct_rev_lut.clear();

            if (image_needs_processing)
            {
                curr_image = image;

                try
                {
                    for (auto &f : active_filters)
                    {
                        logger->info("Applying filter : " + f.first);
                        image_filters[f.first].perform(curr_image, f.second);
                    }

                    if (geocorrect_image)
                    { // TODOREWORK handle disabling projs, etc
                        bool success = false;
                        curr_image = image::earth_curvature::perform_geometric_correction(curr_image, success, &correct_rev_lut, &correct_fwd_lut);
                        if (!success)
                        {
                            logger->error(_("Failed Geo-Correcting image!"));
                            correct_fwd_lut.clear();
                            correct_rev_lut.clear();
                        }
                    }
                }
                catch (std::exception &e)
                {
                    logger->error(_("Error processing image! %s"), e.what());
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

                auto pfunc = [&p](double lat, double lon, double h, double w) mutable -> std::pair<double, double>
                {
                    double x, y;
                    if (p->forward(geodetic::geodetic_coords_t(lat, lon, 0, false), x, y) || x < 0 || x >= w || y < 0 || y >= h)
                        return {-1, -1};
                    else
                        return {x, y};
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

            // Special case for rotations, post-overlays
            try
            {
                if (rotate_image)
                    image::rotate(curr_image, rotate_image);
            }
            catch (std::exception &e)
            {
                logger->error("Error processing image! %s", e.what());
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
