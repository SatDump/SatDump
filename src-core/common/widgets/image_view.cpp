#include "image_view.h"
#include "imgui/imgui_image.h"
#include "core/module.h"
#include "imgui/imgui_internal.h"
#include "imgui/implot/implot.h"
#include "imgui/implot/implot_internal.h"

ImageViewWidget::ImageViewWidget()
{
    id_str = "imgview_" + std::to_string(rand());
}

ImageViewWidget::~ImageViewWidget()
{
}

void ImageViewWidget::update(image::Image<uint16_t> image)
{
    image_mtx.lock();
    img_width = image.width();
    img_height = image.height();

    texture_buffer.resize(img_width * img_height);
    ushort_to_rgba(image.data(), texture_buffer.data(), img_width * img_height, image.channels());
    has_to_update = true;

    image_mtx.unlock();
}

void ImageViewWidget::update(image::Image<uint8_t> image)
{
    image_mtx.lock();
    img_width = image.width();
    img_height = image.height();

    texture_buffer.resize(img_width * img_height);
    uchar_to_rgba(image.data(), texture_buffer.data(), img_width * img_height, image.channels());
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

void ImageViewWidget::draw(ImVec2 win_size)
{
    image_mtx.lock();

    if (texture_id == 0)
        texture_id = makeImageTexture();

    if (has_to_update)
    {
        updateImageTexture(texture_id, texture_buffer.data(), img_width, img_height);
        texture_buffer.clear();
        has_to_update = false;
    }

    ImGui::BeginChild(id_str.c_str(), win_size, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    if (img_width > 0 && img_height > 0)
    {
        ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(0, 0));
        ImPlot::PushStyleVar(ImPlotStyleVar_PlotBorderSize, 0.0f);
        if (ImPlot::BeginPlot((id_str + "plot").c_str(), ImVec2(win_size.x, win_size.y - 16 * ui_scale), ImPlotFlags_NoLegend | ImPlotFlags_NoTitle | ImPlotFlags_CanvasOnly | ImPlotFlags_Equal))
        {
            ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_NoTickMarks | ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoGridLines, ImPlotAxisFlags_NoTickMarks | ImPlotAxisFlags_NoTickLabels | ImPlotAxisFlags_NoGridLines);
            ImPlot::PlotImage((id_str + "plotimg").c_str(), (void *)(intptr_t)texture_id, {0, 0}, ImVec2(img_width, img_height));
            auto pos = ImPlot::GetPlotMousePos(ImAxis_X1, ImAxis_Y1);
            if (pos.x >= 0 && pos.y >= 0 &&
                pos.x < img_width && pos.y < img_height)
                mouseCallback(pos.x, (img_height - 1) - pos.y);

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

            ImPlot::EndPlot();
        }
        ImPlot::PopStyleVar();
        ImPlot::PopStyleVar();
    }

    ImGui::EndChild();
    image_mtx.unlock();
}
