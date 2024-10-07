#pragma once

#include "imgui/imgui.h"
#include "common/image/image.h"
#include <mutex>
#include <vector>
#include <string>
#include <functional>

class ImageViewWidget
{
private:
    struct ImageContainer
    {
        unsigned int texture_id = 0;
        std::vector<uint32_t> texture_buffer;

        int img_width = 0;
        int img_height = 0;

        int offset_x = 0;
        int offset_y = 0;
    };

private:
    std::vector<ImageContainer> img_chunks;

    int fimg_width = 0;
    int fimg_height = 0;

    bool has_to_update = false;
    std::mutex image_mtx;

    std::string id_str;

    bool autoFitNextFrame = false;

public:
    ImageViewWidget();
    ~ImageViewWidget();

    std::function<void(int x, int y)> mouseCallback = [](int, int) {}; // Function that can be used to handle mouse events

    void update(image::Image &image);
    void draw(ImVec2 win_size);

    unsigned int getTextID() { return img_chunks.size() > 0 ? img_chunks[0].texture_id : 0; }
};