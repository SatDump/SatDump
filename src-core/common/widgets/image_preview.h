#pragma once

#include "imgui/imgui.h"
#include <cstdint>
#include <mutex>
#include "../resizeable_buffer.h"

class ImagePreviewWidget
{
private:
    unsigned int texture = 0;
    uint32_t *textureBuffer;

    int width;
    int height;

    std::mutex imageMutex;

public:
    bool hasToUpdate = false;

public:
    ImagePreviewWidget(int width = 500, int height = 500);
    ~ImagePreviewWidget();

    void draw(ResizeableBuffer<uint16_t> &buffer, int width, int height, int render_width, int render_height);
    void draw(uint16_t *buffer, int width, int height, int render_width, int render_height);
};