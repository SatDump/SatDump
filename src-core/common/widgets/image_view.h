#pragma once

#include "common/image/image.h"
#include "imgui/imgui.h"
#include <functional>
#include <mutex>
#include <string>
#include <vector>

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

public:
    bool autoFitNextFrame = false;
    bool zoom_in_next = false;
    bool zoom_out_next = false;

    bool select_crop_next = false;
    bool isSelectingCrop = false;
    ImVec2 crop_initial_pos;
    ImVec2 crop_end_pos;

    std::function<void(int x1, int y1, int x2, int y2)> cropCallback = [](int, int, int, int) {};

public:
    ImageViewWidget();
    ~ImageViewWidget();

    std::function<void(int x, int y)> mouseCallback = [](int, int) {}; // Function that can be used to handle mouse events

    void update(image::Image &image);
    void draw(ImVec2 win_size);

    unsigned int getTextID() { return img_chunks.size() > 0 ? img_chunks[0].texture_id : 0; }
};