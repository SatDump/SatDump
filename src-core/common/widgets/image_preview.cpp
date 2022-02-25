#include "image_preview.h"
#include "imgui/imgui_image.h"
#include "module.h"

ImagePreviewWidget::ImagePreviewWidget(int width, int height)
    : width(width),
      height(height)
{
}

ImagePreviewWidget::~ImagePreviewWidget()
{
    imageMutex.lock();
    if (texture != 0)
        delete[] textureBuffer;
}

void ImagePreviewWidget::draw(ResizeableBuffer<uint16_t> &buffer, int width, int height, int render_width, int render_height)
{
    buffer.buffer_lock.lock();
    draw(buffer.buf, width, height, render_width, render_height);
    buffer.buffer_lock.unlock();
}

void ImagePreviewWidget::draw(uint16_t *buffer, int width, int height, int render_width, int render_height)
{
    imageMutex.lock();
    if (texture == 0)
    {
        texture = makeImageTexture();
        textureBuffer = new uint32_t[this->width * this->height];
    }

    if (hasToUpdate)
    {
        ushort_to_rgba(buffer, textureBuffer, this->width * height, 1);
        updateImageTexture(texture, textureBuffer, this->width, this->height);
        hasToUpdate = false;
    }

    ImGui::Image((void *)(intptr_t)texture, {render_width * ui_scale, render_height * ui_scale});
    imageMutex.unlock();
}