#pragma once

#include "imgui/imgui.h"
#include "common/image/image.h"
#include <mutex>
#include <vector>

class ImageViewWidget
{
private:
    unsigned int texture_id = 0;
    std::vector<uint32_t> texture_buffer;

    int img_width = 0;
    int img_height = 0;

    bool has_to_update = false;
    std::mutex image_mtx;

    std::string id_str;

public:
    ImageViewWidget();
    ~ImageViewWidget();

    std::function<void(int x, int y)> mouseCallback = [](int, int) {}; // Function that can be used to handle mouse events

    void update(image::Image<uint16_t> image);
    void update(image::Image<uint8_t> image);
    void draw(ImVec2 win_size);

    unsigned int getTextID() { return texture_id; }

    bool allow_zoom_and_move = true;
};