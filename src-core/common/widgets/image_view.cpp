#include "image_view.h"
#include "imgui/imgui_image.h"
#include "core/module.h"
#include "imgui/imgui_internal.h"

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

        if (first_run)
        {
            img_scale = (win_size.y) / (img_height * ui_scale);
            first_run = false;
        }
    }

    ImGui::BeginChild(id_str.c_str(), win_size, false, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_NoScrollWithMouse);

    ImGui::GetWindowDrawList()
        ->AddRectFilled(ImGui::GetCursorScreenPos(),
                        ImVec2(ImGui::GetCursorScreenPos().x + ImGui::GetScrollX() + ImGui::GetContentRegionAvail().x,
                               ImGui::GetCursorScreenPos().y + ImGui::GetScrollY() + ImGui::GetContentRegionAvail().y),
                        ImColor::HSV(0, 0, 0));

    ImGui::Image((void *)(intptr_t)texture_id, {img_width * ui_scale * img_scale, img_height * ui_scale * img_scale});

    float scale_before = img_scale;

    ImGuiContext &g = *ImGui::GetCurrentContext();
    ImGuiWindow *window = g.CurrentWindow;

    if (ImGui::IsWindowHovered() && allow_zoom_and_move)
    {
        if (ImGui::GetIO().MouseWheel > 0)
            img_scale *= 1.05;
        else if (ImGui::GetIO().MouseWheel < 0)
            img_scale *= 0.95;

        handleMouseDrag();
    }

    /*if (ImGui::GetIO().MouseWheel != 0)
    {
        ImGuiContext &g = *ImGui::GetCurrentContext();
        ImGuiWindow *window = g.CurrentWindow;
        float imagePosX = window->Scroll.x + ImGui::GetMousePos().x - window->Pos.x;
        float imagePosY = window->Scroll.y + ImGui::GetMousePos().y - window->Pos.y;

        imagePosX /= ui_scale * img_scale;
        imagePosY /= ui_scale * img_scale;

        logger->info("X {:f}, Y {:f}   MOUSE {:f} {:f},    MOUSEPOS {:f} {:f}", window->Scroll.x, window->Scroll.y,
                     ImGui::GetMousePos().x - window->Pos.x, ImGui::GetMousePos().y - window->Pos.y,
                     imagePosX, imagePosY);

        // imagePosX *= ui_scale * img_scale;
        // imagePosY *= ui_scale * img_scale;

        // window->Scroll.x = imagePosX - (win_size.x / 2);
        // window->Scroll.y = imagePosY - (win_size.y / 2);

        // ImGui::GetMousePos().x = window->Pos.x + (win_size.x / 2);
        // ImGui::GetMousePos().y = window->Pos.y + (win_size.y / 2);
        //  window->Scroll.y = ;
    }*/

    // Compute current pixel position
    float imagePosX = window->Scroll.x + ImGui::GetMousePos().x - window->Pos.x;
    float imagePosY = window->Scroll.y + ImGui::GetMousePos().y - window->Pos.y;
    imagePosX /= ui_scale * img_scale;
    imagePosY /= ui_scale * img_scale;
    if (imagePosX >= 0 && imagePosY >= 0 &&
        imagePosX < img_width && imagePosY < img_height &&
        ImGui::GetMousePos().x - window->Pos.x > 0 && ImGui::GetMousePos().y - window->Pos.y > 0)
        mouseCallback(imagePosX, imagePosY);

    if (ImGui::GetIO().MouseWheel != 0)
    {
        float ratio = img_scale / scale_before;

        float scrollX_Percent = (window->Scroll.x + win_size.x / 2) / window->ScrollMax.x;
        float scrollY_Percent = (window->Scroll.y + win_size.y / 2) / window->ScrollMax.y;

        window->Scroll.x = scrollX_Percent * window->ScrollMax.x * ratio - win_size.x / 2;
        window->Scroll.y = scrollY_Percent * window->ScrollMax.y * ratio - win_size.y / 2;
    }

#ifdef __ANDROID__
    ImGui::SetCursorPos({win_size.x - 130 * ui_scale + ImGui::GetScrollX(), win_size.y - 45 * ui_scale + ImGui::GetScrollY()});
    if (ImGui::Button("  -  ##smallerimage"))
        img_scale *= 0.9;
    ImGui::SameLine();
    ImGui::Text("%.0f %%", img_scale * 100);
    ImGui::SameLine();
    if (ImGui::Button(" + ##biggerimage"))
        img_scale *= 1.1;
#endif

    ImGui::EndChild();
    image_mtx.unlock();
}

// From https://github.com/ocornut/imgui/issues/3379
void ImageViewWidget::handleMouseDrag()
{
    ImVec2 delta = ImGui::GetIO().MouseDelta;
    ImGuiContext &g = *ImGui::GetCurrentContext();
    ImGuiWindow *window = g.CurrentWindow;

#ifdef __ANDROID__ // On Android, the "Mouse" is always down
    float length = sqrtf(delta.x * delta.x + delta.y * delta.y);
    if (ImGui::IsMouseDown(ImGuiMouseButton_Left) && length < 100)
#else
    if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
#endif
    {
        window->Scroll.x += -delta.x;
        window->Scroll.y += -delta.y;
    }
}