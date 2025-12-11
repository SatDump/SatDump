#include "bit_container.h"
#include "common/utils.h"
#include "core/exception.h"
#include "imgui/imgui_image.h"
#include "logger.h"

#include <cstdint>
#include <fcntl.h>
#include <filesystem>
#include <random>

#ifdef _WIN32
#define __USE_FILE_OFFSET64
#include "libs/mmap_windows.h"
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

namespace satdump
{
    BitContainer::BitContainer(std::string name, std::string file_path, std::vector<FrameDef> frms) : d_name(name), d_filepath(file_path), frames(frms)
    {
        // Generate unique ID
        std::random_device dev;
        std::mt19937 rng(dev());
        std::uniform_int_distribution<std::mt19937::result_type> check(65, 90);
        for (size_t i = 0; i < 15; i++)
            unique_id[i] = check(rng);

        // Buffer for creating textures
        wip_texture_buffer = new uint32_t[d_chunk_size * d_chunk_size];

        // Init mmap pointers
        d_file_memory_size = getFilesize(file_path);
        if (d_file_memory_size == 0)
            throw satdump_exception("Empty File!");
        fd = open(file_path.c_str(), O_RDONLY);
        d_file_memory_ptr = (uint8_t *)mmap(0, d_file_memory_size, PROT_READ, MAP_SHARED, fd, 0);
        if (d_file_memory_ptr == MAP_FAILED)
        {
            close(fd);
            throw satdump_exception("mmap failed! (" + file_path + ")");
        }

        d_frame_mode = frames.size();
        logger->critical("Frame mode? %d", d_frame_mode);

        // Default period
        init_display();

        // Initial start
        update = true;
    }

    BitContainer::~BitContainer()
    {
        delete[] wip_texture_buffer;
        munmap(d_file_memory_ptr, d_file_memory_size);
        close(fd);

        if (d_is_temporary && std::filesystem::exists(d_filepath))
        {
            try
            {
                std::filesystem::remove(d_filepath);
            }
            catch (std::exception &e)
            {
                logger->warn("Failed to delete temporary file: %s", e.what());
            }
        }
    }

    void BitContainer::init_display()
    {
        // Ensure chunk size is a multiple of display depth
        d_chunk_size = 512;
        if (d_display_bits > 1 && (d_chunk_size % d_display_bits != 0))
            d_chunk_size += d_display_bits - (d_chunk_size % d_display_bits);
        logger->debug("Using chunksize : %d", d_chunk_size);

        // Init chunk count in X/Y
        if (d_frame_mode)
        {
            img_parts_y = (frames.size() / d_chunk_size) + 1;

            size_t max_size = 0;
            for (auto &f : frames)
                if (f.size > max_size)
                    max_size = f.size;

            img_parts_x = (max_size / d_chunk_size) + 1;
        }
        else
        {
            size_t final_size = 0;
            img_parts_y = 0;
            while (final_size < d_file_memory_size * 8)
            {
                final_size += d_bitperiod * d_chunk_size;
                img_parts_y++;
            }

            final_size = 0;
            img_parts_x = 0;
            while (final_size < d_bitperiod)
            {
                final_size += d_chunk_size;
                img_parts_x++;
            }
        }

        image_parts.resize(img_parts_y * img_parts_x);

        printf("%d %d\n", int(img_parts_x * d_chunk_size), int(img_parts_y * d_chunk_size));

        // Ensure we force update
        for (auto &p : image_parts)
            p.need_update = true;
        force_update_all = true;
    }

    void BitContainer::doUpdateTextures()
    {
        if (force_update_all)
        {
            for (size_t ii = 0; ii < img_parts_y; ii++)
            {
                for (size_t iii = 0; iii < img_parts_x; iii++)
                {
                    auto &part = image_parts[ii * img_parts_x + iii];
                    if (part.image_id != 0)
                        deleteImageTexture(part.image_id);
                }
            }
            force_update_all = false;
            update = true;
        }

        const int max_update_num = 2;
        int updated_num = 0;

        if (update)
        {
            size_t offset = 0;

            for (size_t ii = 0; ii < img_parts_y; ii++)
            {
                for (size_t iii = 0; iii < img_parts_x; iii++)
                {
                    auto &part = image_parts[ii * img_parts_x + iii];

                    if (!part.need_update)
                        continue;
                    part.need_update = false;

                    size_t xoffset = iii * d_chunk_size;

                    if (part.visible)
                    {
                        // Only generation is slow, deleting a texture is fast
                        updated_num++;

                        if (part.image_id == 0)
                            part.image_id = makeImageTexture();

                        renderSegment(part, ii, iii, xoffset, offset);
                    }
                    else
                    {
                        if (part.image_id != 0)
                            deleteImageTexture(part.image_id);
                        part.image_id = 0;
                    }

                    part.pos1_x = d_chunk_size * (iii + 0);
                    part.pos1_y = d_chunk_size * (ii + 1);
                    part.pos2_x = d_chunk_size * (iii + 1);
                    part.pos2_y = d_chunk_size * (ii + 0);
                    part.i = ii * img_parts_x + iii;

                    if (updated_num >= max_update_num)
                        return; // TODOREWORK?
                }

                offset += d_bitperiod * d_chunk_size;
            }

            update = false;
        }
    }

    void BitContainer::doDrawPlotTextures(ImPlotRect c)
    {
        for (auto &part : image_parts)
        {
            if (part.i == -1)
            {
                part.need_update = true;
                continue;
            }

            bool status_before = part.visible;
            part.visible = false;
            if (c.Min().x > part.pos2_x || c.Max().y < part.pos2_y) {}
            else if (c.Max().x < part.pos1_x || c.Min().y > part.pos1_y) {}
            else if (!part.need_update)
            {
                // printf("%f %f - %f %f  ----  %f %f - %f %f ---- %d\n",
                //        c.Min().x, c.Min().y, c.Max().x, c.Max().y,
                //        part.pos1_x, part.pos1_y, part.pos2_x, part.pos2_y,
                //        part.i);
                ImPlot::PlotImage("Test", (void *)(intptr_t)part.image_id, {part.pos1_x, part.pos1_y}, {part.pos2_x, part.pos2_y});

                part.visible = true;
            }

            if (part.visible != status_before)
            {
                part.need_update = true;
                update = true;
            }
        }
    }
} // namespace satdump
