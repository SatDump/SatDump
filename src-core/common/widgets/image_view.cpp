#include "image_view.h"
#include "core/style.h"
#include "imgui/imgui.h"
#include "imgui/imgui_image.h"
#include "imgui/imgui_internal.h"
#include "imgui/implot/implot.h"
#include "imgui/implot/implot_internal.h"
#include "logger.h"
#include <cmath>
#include <cstdio>

ImageViewWidget::ImageViewWidget()
{
    static int id_num = 0;
    id_str = "imgview_" + std::to_string(id_num);
    id_num++;
}

ImageViewWidget::~ImageViewWidget() {}

void ImageViewWidget::update(image::Image &image)
{
    image_mtx.lock();
    if (image.width() == 0 || image.height() == 0)
    {
        img_chunks.resize(0);
    }
    else if (image.width() <= maxTextureSize && image.height() <= maxTextureSize)
    {
        img_chunks.resize(1);
        fimg_width = img_chunks[0].img_width = image.width();
        fimg_height = img_chunks[0].img_height = image.height();

        img_chunks[0].texture_buffer.resize(img_chunks[0].img_width * img_chunks[0].img_height);
        image::image_to_rgba(image, img_chunks[0].texture_buffer.data());
    }
    else
    {
        fimg_width = image.width();
        fimg_height = image.height();

        int chunksx = fimg_width / (maxTextureSize / 2);
        int chunksy = fimg_height / (maxTextureSize / 2);
        if (chunksx == 0)
            chunksx = 1;
        if (chunksy == 0)
            chunksy = 1;
        img_chunks.resize(chunksx * chunksy);

        for (int ix = 0; ix < chunksx; ix++)
        {
            for (int iy = 0; iy < chunksy; iy++)
            {
                int i = iy * chunksx + ix;
                int height_start = ((double)iy / (double)chunksy) * (double)fimg_height;
                int height_end = (((double)iy + 1.0) / (double)chunksy) * (double)fimg_height;
                int width_start = ((double)ix / (double)chunksx) * (double)fimg_width;
                int width_end = (((double)ix + 1.0) / (double)chunksx) * (double)fimg_width;

                img_chunks[i].img_width = width_end - width_start;
                img_chunks[i].img_height = height_end - height_start;

                img_chunks[i].texture_buffer.resize(img_chunks[i].img_width * img_chunks[i].img_height);
                auto crop = image.crop_to(width_start, height_start, width_end, height_end);
                image::image_to_rgba(crop, img_chunks[i].texture_buffer.data());

                img_chunks[i].offset_x = width_start;
                img_chunks[i].offset_y = fimg_height - height_end;
            }
        }
    }
    has_to_update = true;
    image_mtx.unlock();
}

inline void addPlotScroll(ImPlotPlot &plot, float wheel_scroll)
{
    ImPlotContext &gp = *ImPlot::GetCurrentContext();
    const bool axis_equal = ImHasFlag(plot.Flags, ImPlotFlags_Equal);

    float zoom_rate = gp.InputMap.ZoomRate;
    if (wheel_scroll == 0.0f)
        zoom_rate = 0;
    else if (wheel_scroll > 0)
        zoom_rate = (-zoom_rate) / (1.0f + (2.0f * zoom_rate));
    ImVec2 rect_size = plot.PlotRect.GetSize();
    float tx = ImRemap(plot.PlotRect.Max.x - plot.PlotRect.Min.x, plot.PlotRect.Min.x, plot.PlotRect.Max.x, 0.0f, 1.0f);
    float ty = ImRemap(plot.PlotRect.Max.y - plot.PlotRect.Min.y, plot.PlotRect.Min.y, plot.PlotRect.Max.y, 0.0f, 1.0f);

    for (int i = 0; i < IMPLOT_NUM_X_AXES; i++)
    {
        ImPlotAxis &x_axis = plot.XAxis(i);
        const bool equal_zoom = axis_equal && x_axis.OrthoAxis != nullptr;
        const bool equal_locked = (equal_zoom != false) && x_axis.OrthoAxis->IsInputLocked();
        if (!x_axis.IsInputLocked() && !equal_locked)
        {
            ImGui::SetKeyOwner(ImGuiKey_MouseWheelY, plot.ID);
            if (zoom_rate != 0.0f)
            {
                float correction = (plot.Hovered && equal_zoom) ? 0.5f : 1.0f;
                const double plot_l = x_axis.PixelsToPlot(plot.PlotRect.Min.x - rect_size.x * tx * zoom_rate * correction);
                const double plot_r = x_axis.PixelsToPlot(plot.PlotRect.Max.x + rect_size.x * (1 - tx) * zoom_rate * correction);
                x_axis.SetMin(x_axis.IsInverted() ? plot_r : plot_l);
                x_axis.SetMax(x_axis.IsInverted() ? plot_l : plot_r);
                if (axis_equal && x_axis.OrthoAxis != nullptr)
                    x_axis.OrthoAxis->SetAspect(x_axis.GetAspect());
            }
        }
    }
    for (int i = 0; i < IMPLOT_NUM_Y_AXES; i++)
    {
        ImPlotAxis &y_axis = plot.YAxis(i);
        const bool equal_zoom = axis_equal && y_axis.OrthoAxis != nullptr;
        const bool equal_locked = equal_zoom && y_axis.OrthoAxis->IsInputLocked();
        if (!y_axis.IsInputLocked() && !equal_locked)
        {
            ImGui::SetKeyOwner(ImGuiKey_MouseWheelY, plot.ID);
            if (zoom_rate != 0.0f)
            {
                float correction = (plot.Hovered && equal_zoom) ? 0.5f : 1.0f;
                const double plot_t = y_axis.PixelsToPlot(plot.PlotRect.Min.y - rect_size.y * ty * zoom_rate * correction);
                const double plot_b = y_axis.PixelsToPlot(plot.PlotRect.Max.y + rect_size.y * (1 - ty) * zoom_rate * correction);
                y_axis.SetMin(y_axis.IsInverted() ? plot_t : plot_b);
                y_axis.SetMax(y_axis.IsInverted() ? plot_b : plot_t);
                if (axis_equal && y_axis.OrthoAxis != nullptr)
                    y_axis.OrthoAxis->SetAspect(y_axis.GetAspect());
            }
        }
    }
}

static double TickStep(double x)
{
    // 1/2/5 * 10^n step
    if (x <= 0.0)
        return 1.0;
    const double e = floor(log10(x));
    const double p = pow(10.0, e);
    const double f = x / p;
    double nf;
    if (f < 1.5)
        nf = 1.0;
    else if (f < 3.0)
        nf = 2.0;
    else if (f < 7.0)
        nf = 5.0;
    else
        nf = 10.0;
    return nf * p;
}

static void SetGridTicks(ImPlotPlot &plot, float target_px = 48.0f)
{
    ImPlotAxis &x = plot.XAxis(0);
    ImPlotAxis &y = plot.YAxis(0);

    const double xmin = x.Range.Min, xmax = x.Range.Max;
    const double ymin = y.Range.Min, ymax = y.Range.Max;
    const double xspan = xmax - xmin;
    if (xspan <= 0.0)
        return;

    const float wpx = plot.PlotRect.GetWidth();
    const float hpx = plot.PlotRect.GetHeight();
    if (wpx <= 0 || hpx <= 0)
        return;

    const double units_per_px = xspan / (double)wpx;
    double step = TickStep(units_per_px * (double)target_px);
    if (!(step > 0.0) || !std::isfinite(step))
        return;

    const int MAX_TICKS = 2048;
    if (xspan / step > MAX_TICKS)
        step = TickStep(xspan / MAX_TICKS);

    static thread_local std::vector<double> xticks;
    static thread_local std::vector<double> yticks;
    xticks.clear();
    yticks.clear();

    const double x0 = floor(xmin / step) * step;
    const double y0 = floor(ymin / step) * step;

    for (double v = x0; v <= xmax + step * 0.5; v += step)
        xticks.push_back(v);
    for (double v = y0; v <= ymax + step * 0.5; v += step)
        yticks.push_back(v);

    ImPlot::SetupAxisTicks(ImAxis_X1, xticks.data(), (int)xticks.size(), nullptr, false);
    ImPlot::SetupAxisTicks(ImAxis_Y1, yticks.data(), (int)yticks.size(), nullptr, false);
}

void ImageViewWidget::draw(ImVec2 win_size)
{
    image_mtx.lock();

    for (auto &chunk : img_chunks)
        if (chunk.texture_id == 0)
            chunk.texture_id = makeImageTexture();

    if (has_to_update)
    {
        for (auto &chunk : img_chunks)
        {
            updateMMImageTexture(chunk.texture_id, chunk.texture_buffer.data(), chunk.img_width, chunk.img_height);
            chunk.texture_buffer.clear();
        }
        has_to_update = false;
    }

    ImGui::BeginChild(id_str.c_str(), win_size, 0, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    if (img_chunks.size() > 0)
    {
        ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0, 0));
        ImPlot::PushStyleVar(ImPlotStyleVar_PlotBorderSize, 0.0f);
        if (ImPlot::BeginPlot((id_str + "plot").c_str(), ImVec2(win_size.x, win_size.y - 16 * ui_scale),
                              ImPlotFlags_NoLegend | ImPlotFlags_NoTitle | ImPlotFlags_CanvasOnly | ImPlotFlags_Equal | ImPlotFlags_NoFrame))
        {
            if (autoFitNextFrame)
            {
                ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoTickMarks | ImPlotAxisFlags_NoTickLabels /*| ImPlotAxisFlags_NoGridLines*/ | ImPlotAxisFlags_AutoFit,
                                  ImPlotAxisFlags_NoTickMarks | ImPlotAxisFlags_NoTickLabels /*| ImPlotAxisFlags_NoGridLines*/ | ImPlotAxisFlags_AutoFit);
                autoFitNextFrame = false;
            }
            else
                ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoTickMarks | ImPlotAxisFlags_NoTickLabels /*| ImPlotAxisFlags_NoGridLines*/ | (isSelectingCrop ? ImPlotAxisFlags_Lock : 0),
                                  ImPlotAxisFlags_NoTickMarks | ImPlotAxisFlags_NoTickLabels /*| ImPlotAxisFlags_NoGridLines*/ | (isSelectingCrop ? ImPlotAxisFlags_Lock : 0));

            auto *p = ImPlot::GetCurrentPlot();
            if (p && ImHasFlag(p->Flags, ImPlotFlags_Equal))
            {
                float target_px = ImClamp(48.0f * ui_scale, 24.0f, 96.0f);
                SetGridTicks(*p, target_px);
            }

            for (auto &chunk : img_chunks)
                ImPlot::PlotImage((id_str + "plotimg").c_str(), (void *)(intptr_t)chunk.texture_id, ImPlotPoint(chunk.offset_x, chunk.offset_y),
                                  ImPlotPoint(chunk.offset_x + chunk.img_width, chunk.offset_y + chunk.img_height));

            auto pos = ImPlot::GetPlotMousePos(ImAxis_X1, ImAxis_Y1);
            auto rpos = ImGui::GetMousePos() - ImGui::GetWindowPos();
            if (pos.x >= 0 && pos.y >= 0 && pos.x < fimg_width && pos.y < fimg_height)
                if (rpos.x >= 0 && rpos.y >= 0 && rpos.x < ImGui::GetWindowWidth() && rpos.y < ImGui::GetWindowHeight())
                    mouseCallback(pos.x, (fimg_height - 1) - pos.y);

#ifdef __ANDROID__
            auto pre_pos = ImGui::GetCursorPos();
            ImGui::SetCursorPos({win_size.x - 130 * ui_scale + ImGui::GetScrollX(), win_size.y - 45 * ui_scale + ImGui::GetScrollY()});
            if (ImGui::Button("  -  ##smallerimage"))
                addPlotScroll(*ImPlot::GetCurrentPlot(), -2);
            ImGui::SameLine();
            ImGui::Text("      ");
            ImGui::SameLine();
            if (ImGui::Button(" + ##biggerimage"))
                addPlotScroll(*ImPlot::GetCurrentPlot(), 2);
            ImGui::SetCursorPos(pre_pos);
#endif

            if (zoom_in_next)
            {
                addPlotScroll(*ImPlot::GetCurrentPlot(), 2);
                zoom_in_next = false;
            }

            if (zoom_out_next)
            {
                addPlotScroll(*ImPlot::GetCurrentPlot(), -2);
                zoom_out_next = false;
            }

            if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Right))
                autoFitNextFrame = true;

            if (select_crop_next && ImGui::IsMouseClicked(ImGuiMouseButton_Left))
            {
                isSelectingCrop = true;
                crop_initial_pos = ImVec2(ImPlot::GetPlotMousePos().x, ImPlot::GetPlotMousePos().y);
                select_crop_next = false;
            }

            if (isSelectingCrop)
            {
                ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);

                if (!ImGui::IsMouseReleased(ImGuiMouseButton_Left))
                {
                    crop_end_pos = ImVec2(ImPlot::GetPlotMousePos().x, ImPlot::GetPlotMousePos().y);

                    if (crop_initial_pos.x < 0)
                        crop_initial_pos.x = 0;
                    if (crop_initial_pos.y < 0)
                        crop_initial_pos.y = 0;

                    if (crop_initial_pos.x >= fimg_width)
                        crop_initial_pos.x = fimg_width - 1;
                    if (crop_initial_pos.y >= fimg_height)
                        crop_initial_pos.y = fimg_height - 1;

                    if (crop_end_pos.x < 0)
                        crop_end_pos.x = 0;
                    if (crop_end_pos.y < 0)
                        crop_end_pos.y = 0;

                    if (crop_end_pos.x >= fimg_width)
                        crop_end_pos.x = fimg_width - 1;
                    if (crop_end_pos.y >= fimg_height)
                        crop_end_pos.y = fimg_height - 1;

                    auto p1 = ImPlot::PlotToPixels(crop_initial_pos);
                    auto p2 = ImPlot::PlotToPixels(crop_end_pos);
                    ImPlot::GetPlotDrawList()->AddQuad(p1, {p2.x, p1.y}, p2, {p1.x, p2.y}, ImColor(255, 255, 255));
                }
                else
                {
                    logger->trace("Crop %d %d, %d %d\n", (int)crop_initial_pos.x, (fimg_height - 1) - (int)crop_initial_pos.y, (int)crop_end_pos.x, (fimg_height - 1) - (int)crop_end_pos.y);
                    if (abs(crop_initial_pos.x - crop_end_pos.x) <= 0 || abs(((fimg_height - 1) - crop_initial_pos.y) - ((fimg_height - 1) - crop_end_pos.y)) <= 0)
                        logger->error("Crop must be bigger than 0!");
                    else
                        cropCallback(crop_initial_pos.x, (fimg_height - 1) - crop_initial_pos.y, crop_end_pos.x, (fimg_height - 1) - crop_end_pos.y);
                    isSelectingCrop = false;
                }
            }

            ImPlot::EndPlot();
        }
        ImPlot::PopStyleVar();
        ImPlot::PopStyleVar();
    }

    ImGui::Dummy(ImVec2(0.0f, 0.0f)); // To avoid triggerring a weird assertion

    ImGui::EndChild();
    image_mtx.unlock();
}
