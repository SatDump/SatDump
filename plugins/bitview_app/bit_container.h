#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include "imgui/implot/implot.h"

namespace satdump
{
    class BitContainer
    {
    private:
        const std::string d_name;
        const std::string d_filepath;
        const size_t d_chunk_size = 512;
        size_t d_file_memory_size;
        uint8_t *d_file_memory_ptr;
        int fd;
        char unique_id[16] = { 0 };

    private:
        struct PartImage
        {
            unsigned int image_id = 0;
            double pos1_x = 0;
            double pos1_y = 0;
            double pos2_x = 0;
            double pos2_y = 0;
            int i = -1;
            bool visible = false;
        };

        size_t img_parts_y = 0;
        size_t img_parts_x = 0;
        std::vector<PartImage> image_parts;

    private:
        uint32_t *wip_texture_buffer = nullptr;

        bool force_update_all = false;
        bool update = false;

    public:
        size_t d_bitperiod = 481280; // 256; // 481280
        int d_display_mode = 0;

        uint8_t *get_ptr() { return d_file_memory_ptr; }
        size_t get_ptr_size() { return d_file_memory_size; }

        bool d_is_temporary = false;

    public:
        BitContainer(std::string name, std::string file);
        ~BitContainer();

        std::string getName() { return d_name; }
        std::string getID() { return std::string(unique_id); }

        void init_bitperiod();
        void forceUpdateAll() { force_update_all = true; }

        void doUpdateTextures();
        void doDrawPlotTextures(ImPlotRect c);

    public:
        std::vector<std::shared_ptr<BitContainer>> all_bit_containers;
    };
}