#include "image_handler.h"

#include "imgui/imgui_stdlib.h"
#include "core/style.h"

#include "common/image/io.h" // TODOREWORK
#include "common/image/processing.h"

#include "nlohmann/json_utils.h"

// TODOREWORK
#include "common/projection/reprojector.h"
#include "common/widgets/json_editor.h"

// TODOREWORK
#include "common/projection/projs2/proj_json.h"
#include "common/projection/warp/warp.h"
#include "common/projection/warp/warp_bkd.h"
#include "projection/raytrace/gcp_compute.h"

#include "common/projection/gcp_compute/gcp_compute.h"

namespace satdump
{
    namespace viewer
    {
        ImageHandler::ImageHandler()
        {
        }

        ImageHandler::~ImageHandler()
        {
        }

        void ImageHandler::drawMenu()
        {
            bool needs_to_be_disabled = is_processing;

            if (ImGui::CollapsingHeader("Image"))
            {
                bool needs_to_update = false;

                if (needs_to_be_disabled)
                    style::beginDisabled();

                needs_to_update |= ImGui::Checkbox("Equalize", &equalize_img);
                needs_to_update |= ImGui::Checkbox("Individual Equalize", &equalize_perchannel_img);
                needs_to_update |= ImGui::Checkbox("White Balance", &white_balance_img);
                needs_to_update |= ImGui::Checkbox("Normalize", &normalize_img);
                needs_to_update |= ImGui::Checkbox("Median Blur", &median_blur_img);
                needs_to_update |= ImGui::Checkbox("Rotate 180", &rotate180_image);

                ImGui::Separator();
                needs_to_update |= ImGui::Checkbox("Warp Image", &warp_image);
                needs_to_update |= ImGui::Checkbox("Old GCP Math", &old_gcp_math);

                if (image_overlay_valib)
                {
                    ImGui::Separator();
                    needs_to_update |= overlay_handler.drawUI(); // TODOREWORK
                    needs_to_update |= ImGui::Checkbox("Rotate 180", &rotate180_image);
                    auto proj_json = image::get_metadata_proj_cfg(image);
                    widgets::JSONTreeEditor(proj_json, "ProjCfg");
                    image::set_metadata_proj_cfg(image, proj_json);
                }

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
            }

            if (imgview_needs_update)
            {
                image_view.update(get_current_img());
                imgview_needs_update = false;

                image_view.mouseCallback = [this](int x, int y)
                {
                    auto &img = get_current_img();
                    ImGui::BeginTooltip(); // TODOREWORK
                    additionalMouseCallback(x, y);
                    ImGui::Text("Raw : %d", img.get(0, x, y));
                    if (image_calib_valid && image.channels() == 1)
                    {
                        double val = image_calib.getVal(img.getf(0, x, y));
                        ImGui::Text("Unit : %f %s", val, image_calib.unit.c_str());
                    }
                    ImGui::EndTooltip();
                };
            }
        }

        void ImageHandler::drawMenuBar()
        {
            if (ImGui::MenuItem("Save Image"))
            {
                logger->warn("Saving image to /tmp/out.png!");
                image::save_img(get_current_img(), "/tmp/out.png");
            }
        }

        void ImageHandler::drawContents(ImVec2 win_size)
        {
            image_view.draw(win_size);
        }

        void ImageHandler::setConfig(nlohmann::json p)
        {
            equalize_img = getValueOrDefault(p["equalize"], false);
            equalize_perchannel_img = getValueOrDefault(p["individual_equalize"], false);
            white_balance_img = getValueOrDefault(p["white_balance"], false);
            normalize_img = getValueOrDefault(p["normalize"], false);
            median_blur_img = getValueOrDefault(p["median_blur"], false);
            rotate180_image = getValueOrDefault(p["rotate180_image"], false);
        }

        nlohmann::json ImageHandler::getConfig()
        {
            nlohmann::json p;
            p["equalize"] = equalize_img;
            p["individual_equalize"] = equalize_perchannel_img;
            p["white_balance"] = white_balance_img;
            p["normalize"] = normalize_img;
            p["median_blur"] = median_blur_img;
            p["rotate180_image"] = rotate180_image;
            return p;
        }

        void ImageHandler::updateImage(image::Image &img) // TODOREWORK
        {
            image = img;
            image_proj_valid = image_overlay_valib = image::has_metadata_proj_cfg(image); // TODOREWORK
            process();
        }

        void ImageHandler::do_process()
        {
            bool image_needs_processing = equalize_img |
                                          equalize_perchannel_img |
                                          white_balance_img |
                                          normalize_img |
                                          median_blur_img |
                                          rotate180_image |
                                          overlay_handler.enabled() |
                                          warp_image;

            if (image_needs_processing)
            {
                curr_image = image;

                if (equalize_img)
                    image::equalize(curr_image, false);
                if (equalize_perchannel_img)
                    image::equalize(curr_image, true);
                if (white_balance_img)
                    image::white_balance(curr_image);
                if (normalize_img)
                    image::normalize(curr_image);
                if (median_blur_img)
                    image::median_blur(curr_image);
                if (rotate180_image)
                    curr_image.mirror(true, true);

                if (image_proj_valid && warp_image)
                {
                    auto prj_cfg = image::get_metadata_proj_cfg(image);
                    warp::WarpOperation operation;
                    prj_cfg["width"] = image.width();                                                                                                    // TODOREWORK move to metadata already?
                    prj_cfg["height"] = image.height();                                                                                                  // TODOREWORK move to metadata already?
                    operation.ground_control_points = old_gcp_math ? satdump::gcp_compute::compute_gcps(prj_cfg) : satdump::proj::compute_gcps(prj_cfg); // img.width(), img.height());
                    operation.input_image = &curr_image;
                    operation.output_rgba = true;
                    // TODO : CHANGE!!!!!!
                    int l_width = prj_cfg.contains("f_width") ? prj_cfg["f_width"].get<int>() : std::max<int>(image.width(), 512) * 10;
                    operation.output_width = l_width;
                    operation.output_height = l_width / 2;

                    logger->trace("Warping size %dx%d", l_width, l_width / 2);

#if 1
                    satdump::warp::WarpResult result = satdump::warp::performSmartWarp(operation);
#else
                    warp::ImageWarper wrapper;
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
                    curr_image = result.output_image;
                }

                if (image_overlay_valib && overlay_handler.enabled())
                {
                    //  auto pfunc = reprojection::setupProjectionFunction(curr_image.width(), curr_image.height(), image::get_metadata_proj_cfg(curr_image), rotate180_image);
                    auto cfg = image::get_metadata_proj_cfg(curr_image);
                    cfg["width"] = curr_image.width();
                    cfg["height"] = curr_image.height();
                    proj::Projection p = cfg;
                    p.init(1, 0);
                    bool rotate = rotate180_image;
                    auto pfunc = [p, rotate](double lat, double lon, double h, double w) mutable -> std::pair<double, double>
                    {
                        double x, y;
                        if (p.forward(geodetic::geodetic_coords_t(lat, lon, 0, false), x, y) || x < 0 || x >= w || y < 0 || y >= h)
                            return {-1, -1};
                        else
                        {
                            if (rotate)
                                return {w - 1 - x, h - 1 - y};
                            else
                                return {x, y};
                        }
                    };

                    overlay_handler.clear_cache();
                    overlay_handler.apply(curr_image, pfunc);
                }
            }
            else
                curr_image.clear();

            has_second_image = image_needs_processing;
            imgview_needs_update = true;

            auto &im = get_current_img();
            if (image::has_metadata_proj_cfg(im))
            {
                //    logger->critical(image::get_metadata_proj_cfg(im).dump(4));
            }

            image_calib_valid = false;
            if (image::has_metadata_calib_cfg(im))
            {
                image_calib = image::get_metadata_calib_cfg(im);
                image_calib_valid = true;
            }
        }
    }
}
