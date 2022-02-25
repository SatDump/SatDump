#include "image_preview.h"
#include "imgui/imgui_image.h"
#include "module.h"

ImagePreviewWidget::ImagePreviewWidget(int width, int height)
    : d_width(width),
      d_height(height)
{
}

ImagePreviewWidget::~ImagePreviewWidget()
{
    imageMutex.lock();
    if (texture != 0)
        delete[] textureBuffer;
}

void ImagePreviewWidget::draw(uint16_t *buffer, int width, int height, int render_width, int render_height)
{
    imageMutex.lock();
    if (texture == 0)
    {
        texture = makeImageTexture();
        textureBuffer = new uint32_t[d_width * d_height];
    }

    if (hasToUpdate)
    {
        ushort_to_rgba(buffer, textureBuffer, d_width * height, 1);
        updateImageTexture(texture, textureBuffer, d_width, d_height);
        hasToUpdate = false;
    }

    ImGui::Image((void *)(intptr_t)texture, {render_width * ui_scale, render_height * ui_scale});
    imageMutex.unlock();
}